/*
 * vdr.c: Video Disk Recorder main program
 *
 * Copyright (C) 2000 Klaus Schmidinger
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 * 
 * The author can be reached at kls@cadsoft.de
 *
 * The project's page is at http://www.cadsoft.de/people/kls/vdr
 *
 * $Id: vdr.c 1.41 2000/11/01 14:31:32 kls Exp $
 */

#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"
#include "dvbapi.h"
#include "interface.h"
#include "menu.h"
#include "recording.h"
#include "tools.h"
#include "videodir.h"

#ifdef REMOTE_KBD
#define KEYS_CONF "keys-pc.conf"
#else
#define KEYS_CONF "keys.conf"
#endif

static int Interrupted = 0;

static void SignalHandler(int signum)
{
  if (signum != SIGPIPE)
     Interrupted = signum;
  signal(signum, SignalHandler);
}

int main(int argc, char *argv[])
{
  // Command line options:

#define DEFAULTSVDRPPORT 2001

  int SVDRPport = DEFAULTSVDRPPORT;
  const char *ConfigDirectory = NULL;
  bool DaemonMode = false;

  static struct option long_options[] = {
      { "config", required_argument, NULL, 'c' },
      { "daemon", no_argument,       NULL, 'd' },
      { "help",   no_argument,       NULL, 'h' },
      { "log",    required_argument, NULL, 'l' },
      { "port",   required_argument, NULL, 'p' },
      { "video",  required_argument, NULL, 'v' },
      { 0 }
    };
  
  int c;
  int option_index = 0;
  while ((c = getopt_long(argc, argv, "c:dhl:p:v:", long_options, &option_index)) != -1) {
        switch (c) {
          case 'c': ConfigDirectory = optarg;
                    break;
          case 'd': DaemonMode = true; break;
          case 'h': printf("Usage: vdr [OPTION]\n\n"           // for easier orientation, this is column 80|
                           "  -c DIR,   --config=DIR  read config files from DIR (default is to read them\n"
                           "                          from the video directory)\n"
                           "  -h,       --help        display this help and exit\n"
                           "  -d,       --daemon      run in daemon mode\n"
                           "  -l LEVEL, --log=LEVEL   set log level (default: 3)\n"
                           "                          0 = no logging, 1 = errors only,\n"
                           "                          2 = errors and info, 3 = errors, info and debug\n"
                           "  -p PORT,  --port=PORT   use PORT for SVDRP (default: %d)\n"
                           "                          0 turns off SVDRP\n"
                           "  -v DIR,   --video=DIR   use DIR as video directory (default is %s)\n"
                           "\n"
                           "Report bugs to <vdr-bugs@cadsoft.de>\n",
                           DEFAULTSVDRPPORT,
                           VideoDirectory
                           );
                    return 0;
                    break;
          case 'l': if (isnumber(optarg)) {
                       int l = atoi(optarg);
                       if (0 <= l && l <= 3) {
                          SysLogLevel = l;
                          break;
                          }
                       }
                    fprintf(stderr, "vdr: invalid log level: %s\n", optarg);
                    abort();
                    break;
          case 'p': if (isnumber(optarg))
                       SVDRPport = atoi(optarg);
                    else {
                       fprintf(stderr, "vdr: invalid port number: %s\n", optarg);
                       abort();
                       }
                    break;
          case 'v': VideoDirectory = optarg;
                    while (optarg && *optarg && optarg[strlen(optarg) - 1] == '/')
                          optarg[strlen(optarg) - 1] = 0;
                    break;
          default:  abort();
          }
        }

  // Log file:
  
  if (SysLogLevel > 0)
     openlog("vdr", LOG_PID | LOG_CONS, LOG_USER);

  // Check the video directory:

  if (!DirectoryOk(VideoDirectory, true)) {
     fprintf(stderr, "vdr: can't access video directory %s\n", VideoDirectory);
     abort();
     }

  // Daemon mode:

  if (DaemonMode) {
#if !defined(DEBUG_OSD) && !defined(REMOTE_KBD)
     pid_t pid = fork();
     if (pid < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        esyslog(LOG_ERR, "ERROR: %s", strerror(errno));
        abort();
        }
     if (pid != 0)
        return 0; // initial program immediately returns
     fclose(stdin);
     fclose(stdout);
     fclose(stderr);
#else
     fprintf(stderr, "vdr: can't run in daemon mode with DEBUG_OSD or REMOTE_KBD on!\n");
     abort();
#endif
     }
  isyslog(LOG_INFO, "VDR version %s started", VDRVERSION);

  // DVB interfaces:

  if (!cDvbApi::Init())
     abort();

  // Configuration data:

  if (!ConfigDirectory)
     ConfigDirectory = VideoDirectory;

  Setup.Load(AddDirectory(ConfigDirectory, "setup.conf"));
  Channels.Load(AddDirectory(ConfigDirectory, "channels.conf"));
  Timers.Load(AddDirectory(ConfigDirectory, "timers.conf"));
#ifdef REMOTE_LIRC
  Keys.SetDummyValues();
#else
  bool KeysLoaded = Keys.Load(AddDirectory(ConfigDirectory, KEYS_CONF));
#endif

  cDvbApi::SetPrimaryDvbApi(Setup.PrimaryDVB);

  Channels.SwitchTo(CurrentChannel);

  // User interface:

  Interface = new cInterface(SVDRPport);
#ifndef REMOTE_LIRC
  if (!KeysLoaded)
     Interface->LearnKeys();
#endif

  // Signal handlers:

  if (signal(SIGHUP,  SignalHandler) == SIG_IGN) signal(SIGHUP,  SIG_IGN);
  if (signal(SIGINT,  SignalHandler) == SIG_IGN) signal(SIGINT,  SIG_IGN);
  if (signal(SIGTERM, SignalHandler) == SIG_IGN) signal(SIGTERM, SIG_IGN);
  if (signal(SIGPIPE, SignalHandler) == SIG_IGN) signal(SIGPIPE, SIG_IGN);

  // Main program loop:

  cOsdBase *Menu = NULL;
  cReplayControl *ReplayControl = NULL;
  int LastChannel = -1;
  int PreviousChannel = CurrentChannel;

  while (!Interrupted) {
        // Channel display:
        if (CurrentChannel != LastChannel) {
           if (!Menu)
              Menu = new cDisplayChannel(CurrentChannel, LastChannel > 0);
           PreviousChannel = LastChannel;
           LastChannel = CurrentChannel;
           }
        // Timers and Recordings:
        if (!Menu) {
           cTimer *Timer = cTimer::GetMatch();
           if (Timer) {
              if (!cRecordControls::Start(Timer)) {
                 //TODO need to do something to prevent the timer from hitting over and over again...
                 }
              }
           cRecordControls::Process();
           }
        // User Input:
        cOsdBase **Interact = Menu ? &Menu : (cOsdBase **)&ReplayControl;
        eKeys key = Interface->GetKey(!*Interact || !(*Interact)->NeedsFastResponse());
        if (*Interact) {
           switch ((*Interact)->ProcessKey(key)) {
             case osMenu:   DELETENULL(Menu);
                            Menu = new cMenuMain(ReplayControl);
                            break;
             case osRecord: DELETENULL(Menu);
                            if (!cRecordControls::Start())
                               Interface->Error("No free DVB device to record!");
                            break;
             case osRecordings:
                            DELETENULL(Menu);
                            DELETENULL(ReplayControl);
                            Menu = new cMenuRecordings;
                            break;
             case osReplay: DELETENULL(Menu);
                            DELETENULL(ReplayControl);
                            ReplayControl = new cReplayControl;
                            break;
             case osStopReplay:
                            DELETENULL(*Interact);
                            DELETENULL(ReplayControl);
                            break;
             case osSwitchDvb:
                            DELETENULL(*Interact);
                            Interface->Info("Switching primary DVB...");
                            cDvbApi::SetPrimaryDvbApi(Setup.PrimaryDVB);
                            break;
             case osBack:
             case osEnd:    DELETENULL(*Interact);
                            break;
             default:       ;
             }
           }
        else {
           switch (key) {
             // Toggle channels:
             case k0:
                  if (PreviousChannel != CurrentChannel)
                     Channels.SwitchTo(PreviousChannel);
                  break;
             // Direct Channel Select:
             case k1 ... k9:
                  if (!Interface->Recording())
                     Menu = new cDisplayChannel(key);
                  break;
             // Left/Right rotates trough channel groups:
             case kLeft|k_Repeat:
             case kLeft:
             case kRight|k_Repeat:
             case kRight: if (!Interface->Recording()) {
                             int SaveGroup = CurrentGroup;
                             if (NORMALKEY(key) == kRight)
                                CurrentGroup = Channels.GetNextGroup(CurrentGroup) ; 
                             else
                                CurrentGroup = Channels.GetPrevGroup(CurrentGroup < 1 ? 1 : CurrentGroup);
                             if (CurrentGroup < 0)
                                CurrentGroup = SaveGroup;
                             Menu = new cDisplayChannel(CurrentGroup, false, true);
                             }
                          break;
             // Up/Down Channel Select:
             case kUp|k_Repeat:
             case kUp:
             case kDown|k_Repeat:
             case kDown: if (!Interface->Recording()) {
                            int n = CurrentChannel + (NORMALKEY(key) == kUp ? 1 : -1);
                            cChannel *channel = Channels.GetByNumber(n);
                            if (channel)
                               channel->Switch();
                            }
                         break;
             // Menu Control:
             case kMenu: Menu = new cMenuMain(ReplayControl); break;
             // Viewing Control:
             case kOk:   LastChannel = -1; break; // forces channel display
             default:    break;
             }
           }
        }
  isyslog(LOG_INFO, "caught signal %d", Interrupted);
  delete Menu;
  delete ReplayControl;
  delete Interface;
  cDvbApi::Cleanup();
  isyslog(LOG_INFO, "exiting");
  if (SysLogLevel > 0)
     closelog();
  return 0;
}
