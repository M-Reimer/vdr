/*
 * dvd.c: Functions for handling DVDs
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * Initially written by Andreas Schultz <aschultz@warp10.net>
 *
 * $Id: dvd.c 1.4 2001/11/10 13:38:50 kls Exp $
 */

#ifdef DVDSUPPORT

//#define DVDSUPPORTDEBUG 1
//#define DEBUG_BUFFER    1

#include <fcntl.h>
#include <linux/cdrom.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "dvd.h"
#include "tools.h"

// --- cDVD ----------------------------------------------------------------------------

const char *cDVD::deviceName = "/dev/dvd";
cDVD *cDVD::dvdInstance = NULL;

cDVD *cDVD::getDVD(void)
{
  if (!dvdInstance)
     new cDVD;
  return dvdInstance;
}

cDVD::cDVD(void)
{
  dvd = NULL;
  title = NULL;
  vmg_file = NULL;
  vts_file = NULL;
  dvdInstance = this;
}

cDVD::~cDVD()
{
  Close();
}

int cDVD::Command(int Cmd)
{
  int result = -1;
  int f;
  if ((f = open(deviceName, O_RDONLY | O_NONBLOCK)) > 0) {
     result = ioctl(f, Cmd, 0);
     close(f);
     }
  return result;
}

void cDVD::SetDeviceName(const char *DeviceName)
{
  deviceName = strdup(DeviceName);
}

const char *cDVD::DeviceName(void)
{
  return deviceName;
}

bool cDVD::DriveExists(void)
{
  return access(deviceName, F_OK) == 0;
}

bool cDVD::DiscOk(void)
{
  return Command(CDROM_DRIVE_STATUS) == CDS_DISC_OK;
}

void cDVD::Eject(void)
{
  if (dvdInstance)
     dvdInstance->Close();
  Command(CDROMEJECT);
}

void cDVD::Open(void)
{
  if (!dvd)
     dvd = DVDOpen(deviceName);
}

void cDVD::Close(void)
{
#ifdef DVDSUPPORTDEBUG
  dsyslog(LOG_INFO, "DVD: cDVD::Close(%p): vts: %p, vmg: %p, title: %p, dvd: %p", this, vts_file, vmg_file, title, dvd);
#endif
  if (vts_file)
     ifoClose(vts_file);
  if (vmg_file)
     ifoClose(vmg_file);
  if (title)
     DVDCloseFile(title);
  if (dvd)
     DVDClose(dvd);
  vts_file = NULL;
  vmg_file = NULL;
  title = NULL;
  dvd = NULL;
}

ifo_handle_t *cDVD::openVMG(void)
{
  if (!isValid())
     return NULL;
  if (!vmg_file)
     vmg_file = ifoOpen(dvd, 0);
  return vmg_file;
}

ifo_handle_t *cDVD::openVTS(int TitleSet)
{
  if (!isValid())
     return NULL;
  if (vts_file && (titleset != TitleSet)) {
     ifoClose(vts_file);
     vts_file = NULL;
     }
  if (!vts_file) {
     titleset = TitleSet;
     vts_file = ifoOpen(dvd, TitleSet);
     }
  return vts_file;
}

dvd_file_t *cDVD::openTitle(int Title, dvd_read_domain_t domain)
{
  if (!isValid())
     return NULL;
  if (title)
     DVDCloseFile(title);
  title = DVDOpenFile(dvd, Title, domain);
  return title;
}

int cDVD::getAudioTrack(int stream)
{
  if (getVTS()) {
     switch (getVTS()->vtsi_mat->vts_audio_attr[stream].audio_format) {
       case 0: // ac3
               return aAC3;
       case 2: // mpeg1
       case 3: // mpeg2ext
               return aMPEG;
       case 4: // lpcm
               return aLPCM;
       case 6: // dts
               return aDTS;
       default:
               esyslog(LOG_ERR, "ERROR: unknown Audio stream info");
       }
     }
  return 0;
}

#endif //DVDSUPPORT
