/*
 * svdrp.c: Simple Video Disk Recorder Protocol
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * The "Simple Video Disk Recorder Protocol" (SVDRP) was inspired
 * by the "Simple Mail Transfer Protocol" (SMTP) and is fully ASCII
 * text based. Therefore you can simply 'telnet' to your VDR port
 * and interact with the Video Disk Recorder - or write a full featured
 * graphical interface that sits on top of an SVDRP connection.
 *
 * $Id: svdrp.c 1.22 2001/09/01 09:50:03 kls Exp $
 */

#define _GNU_SOURCE

#include "svdrp.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include "config.h"
#include "interface.h"
#include "tools.h"

// --- cSocket ---------------------------------------------------------------

cSocket::cSocket(int Port, int Queue)
{
  port = Port;
  sock = -1;
}

cSocket::~cSocket()
{
  Close();
}

void cSocket::Close(void)
{
  if (sock >= 0) {
     close(sock);
     sock = -1;
     }
}

bool cSocket::Open(void)
{
  if (sock < 0) {
     // create socket:
     sock = socket(PF_INET, SOCK_STREAM, 0);
     if (sock < 0) {
        LOG_ERROR;
        port = 0;
        return false;
        }
     // allow it to always reuse the same port:
     int ReUseAddr = 1;
     setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &ReUseAddr, sizeof(ReUseAddr));
     //
     struct sockaddr_in name;
     name.sin_family = AF_INET;
     name.sin_port = htons(port);
     name.sin_addr.s_addr = htonl(INADDR_ANY);
     if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
        LOG_ERROR;
        Close();
        return false;
        }
     // make it non-blocking:
     int oldflags = fcntl(sock, F_GETFL, 0);
     if (oldflags < 0) {
        LOG_ERROR;
        return false;
        }
     oldflags |= O_NONBLOCK;
     if (fcntl(sock, F_SETFL, oldflags) < 0) {
        LOG_ERROR;
        return false;
        }
     // listen to the socket:
     if (listen(sock, queue) < 0) {
        LOG_ERROR;
        return false;
        }
     }
  return true;
}

int cSocket::Accept(void)
{
  if (Open()) {
     struct sockaddr_in clientname;
     uint size = sizeof(clientname);
     int newsock = accept(sock, (struct sockaddr *)&clientname, &size);
     if (newsock > 0)
        isyslog(LOG_INFO, "connect from %s, port %hd", inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port));
     else if (errno != EINTR)
        LOG_ERROR;
     return newsock;
     }
  return -1;
}

// --- cSVDRP ----------------------------------------------------------------

#define MAXHELPTOPIC 10

const char *HelpPages[] = {
  "CHAN [ + | - | <number> | <name> ]\n"
  "    Switch channel up, down or to the given channel number or name.\n"
  "    Without option (or after successfully switching to the channel)\n"
  "    it returns the current channel number and name.",
  "DELC <number>\n"
  "    Delete channel.",
  "DELT <number>\n"
  "    Delete timer.",
  "GRAB <filename> [ jpeg | pnm [ <quality> [ <sizex> <sizey> ] ] ]\n"
  "    Grab the current frame and save it to the given file. Images can\n"
  "    be stored as JPEG (default) or PNM, at the given quality (default\n"
  "    is 'maximum', only applies to JPEG) and size (default is full screen).",
  "HELP [ <topic> ]\n"
  "    The HELP command gives help info.",
  "HITK [ <key> ]\n"
  "    Hit the given remote control key. Without option a list of all\n"
  "    valid key names is given.",
  "LSTC [ <number> | <name> ]\n"
  "    List channels. Without option, all channels are listed. Otherwise\n"
  "    only the given channel is listed. If a name is given, all channels\n"
  "    containing the given string as part of their name are listed.",
  "LSTE\n"
  "    List EPG data.",
  "LSTT [ <number> ]\n"
  "    List timers. Without option, all timers are listed. Otherwise\n"
  "    only the given timer is listed.",
  "MESG [ <message> ]\n"
  "    Displays the given message on the OSD. If message is omitted, the\n"
  "    currently pending message (if any) will be returned. The message\n"
  "    will be displayed for a few seconds as soon as the OSD has become\n"
  "    idle. If a new MESG command is entered while the previous message\n"
  "    has not yet been displayed, the old message will be overwritten.",
  "MODC <number> <settings>\n"
  "    Modify a channel. Settings must be in the same format as returned\n"
  "    by the LSTC command.",
  "MODT <number> on | off | <settings>\n"
  "    Modify a timer. Settings must be in the same format as returned\n"
  "    by the LSTT command. The special keywords 'on' and 'off' can be\n"
  "    used to easily activate or deactivate a timer.",
  "MOVC <number> <to>\n"
  "    Move a channel to a new position.",
  "MOVT <number> <to>\n"
  "    Move a timer to a new position.",
  "NEWC <settings>\n"
  "    Create a new channel. Settings must be in the same format as returned\n"
  "    by the LSTC command.",
  "NEWT <settings>\n"
  "    Create a new timer. Settings must be in the same format as returned\n"
  "    by the LSTT command. It is an error if a timer with the same channel,\n"
  "    day, start and stop time already exists.",
  "NEXT [ abs | rel ]\n"
  "    Show the next timer event. If no option is given, the output will be\n"
  "    in human readable form. With option 'abs' the absolute time of the next\n"
  "    event will be given as the number of seconds since the epoch (time_t\n"
  "    format), while with option 'rel' the relative time will be given as the\n"
  "    number of seconds from now until the event. If the absolute time given\n"
  "    is smaller than the current time, or if the relative time is less than\n"
  "    zero, this means that the timer is currently recording and has started\n"
  "    at the given time. The first value in the resulting line is the number\n"
  "    of the timer.",
  "OVLF <sizex> <sizey> <fbaddr> <bpp> <palette>\n"
  "    Set the size, address depth and palette of the overlay.",
  "OVLG <sizex> <sizey> <posx> <posy>\n"
  "    Set the size and position of the overlay.",
  "OVLC <clipcount> <base16-CRect-array>\n"
  "    Set the overlay clipping rectangles.",
  "OVLP <brightness> <colour> <hue> <contrast>\n"
  "    Set the picture parameters for the overlay.",
  "OVLO 0 | 1\n"
  "    Switch the overlay on or off.",
  "UPDT <settings>\n"
  "    Updates a timer. Settings must be in the same format as returned\n"
  "    by the LSTT command. If a timer with the same channel, day, start\n"
  "    and stop time does not yet exists, it will be created.",
  "QUIT\n"
  "    Exit vdr (SVDRP).\n"
  "    You can also hit Ctrl-D to exit.",
  NULL
  };

/* SVDRP Reply Codes:

 214 Help message
 215 EPG data record
 220 VDR service ready
 221 VDR service closing transmission channel
 250 Requested VDR action okay, completed
 451 Requested action aborted: local error in processing
 500 Syntax error, command unrecognized
 501 Syntax error in parameters or arguments
 502 Command not implemented
 504 Command parameter not implemented
 550 Requested action not taken
 554 Transaction failed

*/

const char *GetHelpTopic(const char *HelpPage)
{
  static char topic[MAXHELPTOPIC];
  const char *q = HelpPage;
  while (*q) {
        if (isspace(*q)) {
           uint n = q - HelpPage;
           if (n >= sizeof(topic))
              n = sizeof(topic) - 1;
           strncpy(topic, HelpPage, n);
           topic[n] = 0;
           return topic;
           }
        q++;
        }
  return NULL;
}

const char *GetHelpPage(const char *Cmd)
{
  const char **p = HelpPages;
  while (*p) {
        const char *t = GetHelpTopic(*p);
        if (strcasecmp(Cmd, t) == 0)
           return *p;
        p++;
        }
  return NULL;
}

cSVDRP::cSVDRP(int Port)
:socket(Port)
{
  numChars = 0;
  message = NULL;
  lastActivity = 0;
  isyslog(LOG_INFO, "SVDRP listening on port %d", Port);
}

cSVDRP::~cSVDRP()
{
  Close();
  delete message;
}

void cSVDRP::Close(bool Timeout)
{
  if (file.IsOpen()) {
     //TODO how can we get the *full* hostname?
     char buffer[MAXCMDBUFFER];
     gethostname(buffer, sizeof(buffer));
     Reply(221, "%s closing connection%s", buffer, Timeout ? " (timeout)" : "");
     isyslog(LOG_INFO, "closing SVDRP connection"); //TODO store IP#???
     file.Close();
     }
}

bool cSVDRP::Send(const char *s, int length)
{
  if (length < 0)
     length = strlen(s);
  int wbytes = safe_write(file, s, length);
  if (wbytes == length)
     return true;
  if (wbytes < 0) {
     LOG_ERROR;
     file.Close();
     cDvbApi::PrimaryDvbApi->OvlO(false);
     }
  else //XXX while...???
     esyslog(LOG_ERR, "Wrote %d bytes to client while expecting %d\n", wbytes, length);
  return false;
}

void cSVDRP::Reply(int Code, const char *fmt, ...)
{
  if (file.IsOpen()) {
     if (Code != 0) {
        va_list ap;
        va_start(ap, fmt);
        char *buffer;
        vasprintf(&buffer, fmt, ap);
        char *nl = strchr(buffer, '\n');
        if (Code > 0 && nl && *(nl + 1)) // trailing newlines don't count!
           Code = -Code;
        char number[16];
        sprintf(number, "%03d%c", abs(Code), Code < 0 ? '-' : ' ');
        const char *s = buffer;
        while (s && *s) {
              const char *n = strchr(s, '\n');
              if (!(Send(number) && Send(s, n ? n - s : -1) && Send("\r\n"))) {
                 Close();
                 break;
                 }
              s = n ? n + 1 : NULL;
              }
        delete buffer;
        va_end(ap);
        }
     else {
        Reply(451, "Zero return code - looks like a programming error!");
        esyslog(LOG_ERR, "SVDRP: zero return code!");
        }
     }
}

void cSVDRP::CmdCHAN(const char *Option)
{
  if (*Option) {
     int n = -1;
     if (isnumber(Option)) {
        int o = strtol(Option, NULL, 10);
        if (o >= 1 && o <= Channels.MaxNumber())
           n = o;
        }
     else if (strcmp(Option, "-") == 0) {
        n = cDvbApi::CurrentChannel();
        if (n > 1)
           n--;
        }
     else if (strcmp(Option, "+") == 0) {
        n = cDvbApi::CurrentChannel();
        if (n < Channels.MaxNumber())
           n++;
        }
     else {
        int i = 1;
        cChannel *channel;
        while ((channel = Channels.GetByNumber(i)) != NULL) {
              if (strcasecmp(channel->name, Option) == 0) {
                 n = i;
                 break;
                 }
              i++;
              }
        }
     if (n < 0) {
        Reply(501, "Undefined channel \"%s\"", Option);
        return;
        }
     if (Interface->Recording()) {
        Reply(550, "Can't switch channel, interface is recording");
        return;
        }
     cChannel *channel = Channels.GetByNumber(n);
     if (channel) {
        if (!channel->Switch()) {
           Reply(554, "Error switching to channel \"%d\"", channel->number);
           return;
           }
        }
     else {
        Reply(550, "Unable to find channel \"%s\"", Option);
        return;
        }
     }
  cChannel *channel = Channels.GetByNumber(cDvbApi::CurrentChannel());
  if (channel)
     Reply(250, "%d %s", channel->number, channel->name);
  else
     Reply(550, "Unable to find channel \"%d\"", cDvbApi::CurrentChannel());
}

void cSVDRP::CmdDELC(const char *Option)
{
  //TODO combine this with menu action (timers must be updated)
  Reply(502, "DELC not yet implemented");
}

void cSVDRP::CmdDELT(const char *Option)
{
  if (*Option) {
     if (isnumber(Option)) {
        cTimer *timer = Timers.Get(strtol(Option, NULL, 10) - 1);
        if (timer) {
           if (!timer->recording) {
              Timers.Del(timer);
              Timers.Save();
              isyslog(LOG_INFO, "timer %s deleted", Option);
              Reply(250, "Timer \"%s\" deleted", Option);
              }
           else
              Reply(550, "Timer \"%s\" is recording", Option);
           }
        else
           Reply(501, "Timer \"%s\" not defined", Option);
        }
     else
        Reply(501, "Error in timer number \"%s\"", Option);
     }
  else
     Reply(501, "Missing timer number");
}

void cSVDRP::CmdGRAB(const char *Option)
{
  char *FileName = NULL;
  bool Jpeg = true;
  int Quality = -1, SizeX = -1, SizeY = -1;
  if (*Option) {
     char buf[strlen(Option) + 1];
     char *p = strcpy(buf, Option);
     const char *delim = " \t";
     FileName = strtok(p, delim);
     if ((p = strtok(NULL, delim)) != NULL) {
        if (strcasecmp(p, "JPEG") == 0)
           Jpeg = true;
        else if (strcasecmp(p, "PNM") == 0)
           Jpeg = false;
        else {
           Reply(501, "Unknown image type \"%s\"", p);
           return;
           }
        }
     if ((p = strtok(NULL, delim)) != NULL) {
        if (isnumber(p))
           Quality = atoi(p);
        else {
           Reply(501, "Illegal quality \"%s\"", p);
           return;
           }
        }
     if ((p = strtok(NULL, delim)) != NULL) {
        if (isnumber(p))
           SizeX = atoi(p);
        else {
           Reply(501, "Illegal sizex \"%s\"", p);
           return;
           }
        if ((p = strtok(NULL, delim)) != NULL) {
           if (isnumber(p))
              SizeY = atoi(p);
           else {
              Reply(501, "Illegal sizey \"%s\"", p);
              return;
              }
           }
        else {
           Reply(501, "Missing sizey");
           return;
           }
        }
     if ((p = strtok(NULL, delim)) != NULL) {
        Reply(501, "Unexpected parameter \"%s\"", p);
        return;
        }
     if (cDvbApi::PrimaryDvbApi->GrabImage(FileName, Jpeg, Quality, SizeX, SizeY))
        Reply(250, "Grabbed image %s", Option);
     else
        Reply(451, "Grab image failed");
     }
  else
     Reply(501, "Missing filename");
}

void cSVDRP::CmdHELP(const char *Option)
{
  if (*Option) {
     const char *hp = GetHelpPage(Option);
     if (hp)
        Reply(214, hp);
     else {
        Reply(504, "HELP topic \"%s\" unknown", Option);
        return;
        }
     }
  else {
     Reply(-214, "This is VDR version %s", VDRVERSION);
     Reply(-214, "Topics:");
     const char **hp = HelpPages;
     int NumPages = 0;
     while (*hp) {
           NumPages++;
           hp++;
           }
     const int TopicsPerLine = 5;
     int x = 0;
     for (int y = 0; (y * TopicsPerLine + x) < NumPages; y++) {
         char buffer[TopicsPerLine * (MAXHELPTOPIC + 5)];
         char *q = buffer;
         for (x = 0; x < TopicsPerLine && (y * TopicsPerLine + x) < NumPages; x++) {
             const char *topic = GetHelpTopic(HelpPages[(y * TopicsPerLine + x)]);
             if (topic)
                q += sprintf(q, "    %s", topic);
             }
         x = 0;
         Reply(-214, buffer);
         }
     Reply(-214, "To report bugs in the implementation send email to");
     Reply(-214, "    vdr-bugs@cadsoft.de");
     }
  Reply(214, "End of HELP info");
}

void cSVDRP::CmdHITK(const char *Option)
{
  if (*Option) {
     eKeys k = Keys.Translate(Option);
     if (k != kNone) {
        Interface->PutKey(k);
        Reply(250, "Key \"%s\" accepted", Option);
        }
     else
        Reply(504, "Unknown key: \"%s\"", Option);
     }
  else {
     Reply(-214, "Valid <key> names for the HITK command:");
     for (int i = 0; i < kNone; i++) {
         Reply(-214, "    %s", Keys.keys[i].name);
         }
     Reply(214, "End of key list");
     }
}

void cSVDRP::CmdLSTC(const char *Option)
{
  if (*Option) {
     if (isnumber(Option)) {
        cChannel *channel = Channels.GetByNumber(strtol(Option, NULL, 10));
        if (channel)
           Reply(250, "%d %s", channel->number, channel->ToText());
        else
           Reply(501, "Channel \"%s\" not defined", Option);
        }
     else {
        int i = 1;
        cChannel *next = NULL;
        while (i <= Channels.MaxNumber()) {
              cChannel *channel = Channels.GetByNumber(i);
              if (channel) {
                 if (strcasestr(channel->name, Option)) {
                    if (next)
                       Reply(-250, "%d %s", next->number, next->ToText());
                    next = channel;
                    }
                 }
              else {
                 Reply(501, "Channel \"%d\" not found", i);
                 return;
                 }
              i++;
              }
        if (next)
           Reply(250, "%d %s", next->number, next->ToText());
        }
     }
  else if (Channels.MaxNumber() >= 1) {
     for (int i = 1; i <= Channels.MaxNumber(); i++) {
         cChannel *channel = Channels.GetByNumber(i);
        if (channel)
           Reply(i < Channels.MaxNumber() ? -250 : 250, "%d %s", channel->number, channel->ToText());
        else
           Reply(501, "Channel \"%d\" not found", i);
         }
     }
  else
     Reply(550, "No channels defined");
}

void cSVDRP::CmdLSTE(const char *Option)
{
  cThreadLock ThreadLock;
  const cSchedules *Schedules = cDvbApi::PrimaryDvbApi->Schedules(&ThreadLock);
  if (Schedules) {
     FILE *f = fdopen(file, "w");
     if (f) {
        Schedules->Dump(f, "215-");
        fflush(f);
        Reply(215, "End of EPG data");
        // don't 'fclose(f)' here!
        }
     else
        Reply(451, "Can't open file connection");
     }
  else
     Reply(451, "Can't get EPG data");
}

void cSVDRP::CmdLSTT(const char *Option)
{
  if (*Option) {
     if (isnumber(Option)) {
        cTimer *timer = Timers.Get(strtol(Option, NULL, 10) - 1);
        if (timer)
           Reply(250, "%d %s", timer->Index() + 1, timer->ToText());
        else
           Reply(501, "Timer \"%s\" not defined", Option);
        }
     else
        Reply(501, "Error in timer number \"%s\"", Option);
     }
  else if (Timers.Count()) {
     for (int i = 0; i < Timers.Count(); i++) {
         cTimer *timer = Timers.Get(i);
        if (timer)
           Reply(i < Timers.Count() - 1 ? -250 : 250, "%d %s", timer->Index() + 1, timer->ToText());
        else
           Reply(501, "Timer \"%d\" not found", i + 1);
         }
     }
  else
     Reply(550, "No timers defined");
}

void cSVDRP::CmdMESG(const char *Option)
{
  if (*Option) {
     delete message;
     message = strdup(Option);
     isyslog(LOG_INFO, "SVDRP message: '%s'", message);
     Reply(250, "Message stored");
     }
  else if (message)
     Reply(250, "%s", message);
  else
     Reply(550, "No pending message");
}

void cSVDRP::CmdMODC(const char *Option)
{
  if (*Option) {
     char *tail;
     int n = strtol(Option, &tail, 10);
     if (tail && tail != Option) {
        tail = skipspace(tail);
        cChannel *channel = Channels.GetByNumber(n);
        if (channel) {
           cChannel c = *channel;
           if (!c.Parse(tail)) {
              Reply(501, "Error in channel settings");
              return;
              }
           *channel = c;
           Channels.Save();
           isyslog(LOG_INFO, "channel %d modified", channel->number);
           Reply(250, "%d %s", channel->number, channel->ToText());
           }
        else
           Reply(501, "Channel \"%d\" not defined", n);
        }
     else
        Reply(501, "Error in channel number");
     }
  else
     Reply(501, "Missing channel settings");
}

void cSVDRP::CmdMODT(const char *Option)
{
  if (*Option) {
     char *tail;
     int n = strtol(Option, &tail, 10);
     if (tail && tail != Option) {
        tail = skipspace(tail);
        cTimer *timer = Timers.Get(n - 1);
        if (timer) {
           cTimer t = *timer;
           if (strcasecmp(tail, "ON") == 0)
              t.active = 1;
           else if (strcasecmp(tail, "OFF") == 0)
              t.active = 0;
           else if (!t.Parse(tail)) {
              Reply(501, "Error in timer settings");
              return;
              }
           *timer = t;
           Timers.Save();
           isyslog(LOG_INFO, "timer %d modified (%s)", timer->Index() + 1, timer->active ? "active" : "inactive");
           Reply(250, "%d %s", timer->Index() + 1, timer->ToText());
           }
        else
           Reply(501, "Timer \"%d\" not defined", n);
        }
     else
        Reply(501, "Error in timer number");
     }
  else
     Reply(501, "Missing timer settings");
}

void cSVDRP::CmdMOVC(const char *Option)
{
  //TODO combine this with menu action (timers must be updated)
  Reply(502, "MOVC not yet implemented");
}

void cSVDRP::CmdMOVT(const char *Option)
{
  //TODO combine this with menu action
  Reply(502, "MOVT not yet implemented");
}

void cSVDRP::CmdNEWC(const char *Option)
{
  if (*Option) {
     cChannel *channel = new cChannel;
     if (channel->Parse(Option)) {
        Channels.Add(channel);
        Channels.ReNumber();
        Channels.Save();
        isyslog(LOG_INFO, "channel %d added", channel->number);
        Reply(250, "%d %s", channel->number, channel->ToText());
        }
     else
        Reply(501, "Error in channel settings");
     }
  else
     Reply(501, "Missing channel settings");
}

void cSVDRP::CmdNEWT(const char *Option)
{
  if (*Option) {
     cTimer *timer = new cTimer;
     if (timer->Parse(Option)) {
        cTimer *t = Timers.GetTimer(timer);
        if (!t) {
           Timers.Add(timer);
           Timers.Save();
           isyslog(LOG_INFO, "timer %d added", timer->Index() + 1);
           Reply(250, "%d %s", timer->Index() + 1, timer->ToText());
           return;
           }
        else
           Reply(550, "Timer already defined: %d %s", t->Index() + 1, t->ToText());
        }
     else
        Reply(501, "Error in timer settings");
     delete timer;
     }
  else
     Reply(501, "Missing timer settings");
}

void cSVDRP::CmdNEXT(const char *Option)
{
  cTimer *t = Timers.GetNextActiveTimer();
  if (t) {
     time_t Start = t->StartTime();
     int Number = t->Index() + 1;
     if (!*Option) {
        char *s = ctime(&Start);
        s[strlen(s) - 1] = 0; // strip trailing newline
        Reply(250, "%d %s", Number, s);
        }
     else if (strcasecmp(Option, "ABS") == 0)
        Reply(250, "%d %ld", Number, Start);
     else if (strcasecmp(Option, "REL") == 0)
        Reply(250, "%d %ld", Number, Start - time(NULL));
     else
        Reply(501, "Unknown option: \"%s\"", Option);
     }
  else
     Reply(550, "No active timers");
}

void cSVDRP::CmdOVLF(const char *Option)
{
  if (*Option) {
     int SizeX = 0, SizeY = 0, Bpp = 0, Palette = 0, FbAddr = 0;
     if (5 == sscanf(Option, "%d %d %x %d %d", &SizeX, &SizeY, &FbAddr, &Bpp, &Palette)) {
        //somehow_set_overlay_geometry;
        if (cDvbApi::PrimaryDvbApi->OvlF(SizeX, SizeY, FbAddr, Bpp, Palette))
           Reply(250, "Overlay framebuffer set");      
        else
           Reply(451, "Illegal overlay framebuffer settings");
        }
     else
        Reply(501, "Could not parse overlay framebuffer settings");
     }
  else
     Reply(501, "Missing overlay framebuffer settings");
}

void cSVDRP::CmdOVLG(const char *Option)
{
  if (*Option) {
     int SizeX = 0, SizeY = 0, PosX = 0, PosY = 0;
     if (4 == sscanf(Option, "%d %d %d %d", &SizeX, &SizeY, &PosX, &PosY)) {
        //somehow_set_overlay_geometry;
        if (cDvbApi::PrimaryDvbApi->OvlG(SizeX, SizeY, PosX, PosY))
           Reply(250, "Overlay geometry set"); 
        else
           Reply(451, "Illegal overlay geometry settings");
        }
     else
        Reply(501, "Could not parse overlay geometry settings");
     }
  else
     Reply(501, "Missing overlay geometry settings");
}

void cSVDRP::CmdOVLC(const char *Option)
{
  if (*Option) {
     int ClipCount = 0;
     unsigned char s[2 * MAXCLIPRECTS * sizeof(CRect) + 2];
     if (2 == sscanf(Option, "%d %s", &ClipCount, s)) {
        // Base16-decoding of CRect-array:
        unsigned char *p = (unsigned char*)ovlClipRects;
        int i = 0, size = sizeof(CRect)*ClipCount;
        for (int j = 0; i < size; i++) {
            p[i]  = (s[j++] - 65);
            p[i] += (s[j++] - 65) << 4;
            }
        if (((unsigned)ClipCount == (i / sizeof(CRect))) && (ClipCount >= 0)) {
           // apply it:
           if (cDvbApi::PrimaryDvbApi->OvlC(ClipCount, ovlClipRects))
              Reply(250, "Overlay-Clipping set");
           else
              Reply(451, "Illegal overlay clipping settings");
           return;
           }
        }
     Reply(501, "Error parsing Overlay-Clipping settings");
     }
  else
     Reply(501, "Missing Clipping settings");
}

void cSVDRP::CmdOVLP(const char *Option)
{
  if (*Option) {
     int Brightness = 0, Colour = 0, Hue = 0, Contrast = 0;
     if (4 == sscanf(Option, "%d %d %d %d", &Brightness, &Colour, &Hue, &Contrast)) {
        //somehow_set_overlay_picture_settings;
        if (cDvbApi::PrimaryDvbApi->OvlP(Brightness, Colour, Hue, Contrast))
           Reply(250, "Overlay picture settings set"); 
        else
           Reply(451, "Illegal overlay picture settings");
        }
     else
        Reply(501, "Could not parse overlay picture settings");
     }
  else
     Reply(501, "Missing overlay picture settings");
}

void cSVDRP::CmdOVLO(const char *Option)
{
  if (*Option) {
     int Value;
     if (1 == sscanf(Option, "%d", &Value)) {
        //somehow_set_overlay_picture_settings;
        if (cDvbApi::PrimaryDvbApi->OvlO(Value))
           Reply(250, "Overlay capture set");  
        else
           Reply(451, "Error setting overlay capture");
        }
     else
        Reply(501, "Could not parse status");
     }
  else
     Reply(501, "Missing overlay capture status");
}

void cSVDRP::CmdUPDT(const char *Option)
{
  if (*Option) {
     cTimer *timer = new cTimer;
     if (timer->Parse(Option)) {
        cTimer *t = Timers.GetTimer(timer);
        if (t) {
           t->Parse(Option);
           delete timer;
           timer = t;
           isyslog(LOG_INFO, "timer %d updated", timer->Index() + 1);
           }
        else {
           Timers.Add(timer);
           isyslog(LOG_INFO, "timer %d added", timer->Index() + 1);
           }
        Timers.Save();
        Reply(250, "%d %s", timer->Index() + 1, timer->ToText());
        return;
        }
     else
        Reply(501, "Error in timer settings");
     delete timer;
     }
  else
     Reply(501, "Missing timer settings");
}

#define CMD(c) (strcasecmp(Cmd, c) == 0)

void cSVDRP::Execute(char *Cmd)
{
  // skip leading whitespace:
  Cmd = skipspace(Cmd);
  // find the end of the command word:
  char *s = Cmd;
  while (*s && !isspace(*s))
        s++;
  if (*s)
     *s++ = 0;
  if      (CMD("CHAN"))  CmdCHAN(s);
  else if (CMD("DELC"))  CmdDELC(s);
  else if (CMD("DELT"))  CmdDELT(s);
  else if (CMD("GRAB"))  CmdGRAB(s);
  else if (CMD("HELP"))  CmdHELP(s);
  else if (CMD("HITK"))  CmdHITK(s);
  else if (CMD("LSTC"))  CmdLSTC(s);
  else if (CMD("LSTE"))  CmdLSTE(s);
  else if (CMD("LSTT"))  CmdLSTT(s);
  else if (CMD("MESG"))  CmdMESG(s);
  else if (CMD("MODC"))  CmdMODC(s);
  else if (CMD("MODT"))  CmdMODT(s);
  else if (CMD("MOVC"))  CmdMOVC(s);
  else if (CMD("MOVT"))  CmdMOVT(s);
  else if (CMD("NEWC"))  CmdNEWC(s);
  else if (CMD("NEWT"))  CmdNEWT(s);
  else if (CMD("NEXT"))  CmdNEXT(s);
  else if (CMD("OVLF"))  CmdOVLF(s);
  else if (CMD("OVLG"))  CmdOVLG(s);
  else if (CMD("OVLC"))  CmdOVLC(s);
  else if (CMD("OVLP"))  CmdOVLP(s);
  else if (CMD("OVLO"))  CmdOVLO(s);
  else if (CMD("UPDT"))  CmdUPDT(s);
  else if (CMD("QUIT"))  Close();
  else                   Reply(500, "Command unrecognized: \"%s\"", Cmd);
}

void cSVDRP::Process(void)
{
  bool NewConnection = !file.IsOpen();
  bool SendGreeting = NewConnection;

  if (file.IsOpen() || file.Open(socket.Accept())) {
     if (SendGreeting) {
        //TODO how can we get the *full* hostname?
        char buffer[MAXCMDBUFFER];
        gethostname(buffer, sizeof(buffer));
        time_t now = time(NULL);
        Reply(220, "%s SVDRP VideoDiskRecorder %s; %s", buffer, VDRVERSION, ctime(&now));
        }
     if (NewConnection)
        lastActivity = time(NULL);
     if (file.Ready(false)) {
        unsigned char c;
        int r = safe_read(file, &c, 1);
        if (r > 0) {
           if (c == '\n' || c == 0x00) {
              // strip trailing whitespace:
              while (numChars > 0 && strchr(" \t\r\n", cmdLine[numChars - 1]))
                    cmdLine[--numChars] = 0;
              // make sure the string is terminated:
              cmdLine[numChars] = 0;
              // showtime!
              Execute(cmdLine);
              numChars = 0;
              }
           else if (c == 0x04 && numChars == 0) {
              // end of file (only at beginning of line)
              Close();
              }
           else if (c == 0x08 || c == 0x7F) {
              // backspace or delete (last character)
              if (numChars > 0)
                 numChars--;
              }
           else if (c <= 0x03 || c == 0x0D) {
              // ignore control characters
              }
           else if (numChars < sizeof(cmdLine) - 1) {
              cmdLine[numChars++] = c;
              cmdLine[numChars] = 0;
              }
           else {
              Reply(501, "Command line too long");
              esyslog(LOG_ERR, "SVDRP: command line too long: '%s'", cmdLine);
              numChars = 0;
              }
           lastActivity = time(NULL);
           }
        else if (r <= 0) {
           isyslog(LOG_INFO, "lost connection to SVDRP client");
           Close();
           }
        }
     else if (Setup.SVDRPTimeout && time(NULL) - lastActivity > Setup.SVDRPTimeout) {
        isyslog(LOG_INFO, "timeout on SVDRP connection");
        Close(true);
        }
     }
}

char *cSVDRP::GetMessage(void)
{
  char *s = message;
  message = NULL;
  return s;
}

//TODO more than one connection???
