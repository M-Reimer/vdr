/*
 * dvbdevice.h: The DVB device interface
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: dvbdevice.h 1.27 2004/04/17 11:56:22 kls Exp $
 */

#ifndef __DVBDEVICE_H
#define __DVBDEVICE_H

#include <linux/dvb/frontend.h>
#include <linux/dvb/version.h>
#include "device.h"
#include "dvbspu.h"

#if DVB_API_VERSION != 3
#error VDR requires Linux DVB driver API version 3!
#endif

#define MAXDVBDEVICES  4

class cDvbTuner;

/// The cDvbDevice implements a DVB device which can be accessed through the Linux DVB driver API.

class cDvbDevice : public cDevice {
private:
  static bool Probe(const char *FileName);
         ///< Probes for existing DVB devices.
public:
  static bool Initialize(void);
         ///< Initializes the DVB devices.
         ///< Must be called before accessing any DVB functions.
         ///< \return True if any devices are available.
private:
  fe_type_t frontendType;
  int fd_osd, fd_audio, fd_video, fd_dvr, fd_stc;
protected:
  virtual void MakePrimaryDevice(bool On);
public:
  cDvbDevice(int n);
  virtual ~cDvbDevice();
  virtual int ProvidesCa(const cChannel *Channel) const;
  virtual bool HasDecoder(void) const;

// SPU facilities

private:
  cDvbSpuDecoder *spuDecoder;
public:
  virtual cSpuDecoder *GetSpuDecoder(void);

// Channel facilities

private:
  cDvbTuner *dvbTuner;
  void TurnOffLiveMode(void);
public:
  virtual bool ProvidesSource(int Source) const;
  virtual bool ProvidesTransponder(const cChannel *Channel) const;
  virtual bool ProvidesChannel(const cChannel *Channel, int Priority = -1, bool *NeedsDetachReceivers = NULL) const;
protected:
  virtual bool SetChannelDevice(const cChannel *Channel, bool LiveView);
public:
  virtual bool HasLock(void);

// PID handle facilities

protected:
  virtual bool SetPid(cPidHandle *Handle, int Type, bool On);

// Section filter facilities

protected:
  virtual int OpenFilter(u_short Pid, u_char Tid, u_char Mask);

// Image Grab facilities

private:
  static int devVideoOffset;
  int devVideoIndex;
public:
  virtual bool GrabImage(const char *FileName, bool Jpeg = true, int Quality = -1, int SizeX = -1, int SizeY = -1);

// Video format facilities

public:
  virtual void SetVideoFormat(bool VideoFormat16_9);
  virtual eVideoSystem GetVideoSystem(void);

// Audio facilities

private:
  int aPid1, aPid2;
protected:
  virtual void SetVolumeDevice(int Volume);
  virtual int NumAudioTracksDevice(void) const;
  virtual const char **GetAudioTracksDevice(int *CurrentTrack = NULL) const;
  virtual void SetAudioTrackDevice(int Index);

// Player facilities

protected:
  ePlayMode playMode;
  virtual bool CanReplay(void) const;
  virtual bool SetPlayMode(ePlayMode PlayMode);
public:
  virtual int64_t GetSTC(void);
  virtual void TrickSpeed(int Speed);
  virtual void Clear(void);
  virtual void Play(void);
  virtual void Freeze(void);
  virtual void Mute(void);
  virtual void StillPicture(const uchar *Data, int Length);
  virtual bool Poll(cPoller &Poller, int TimeoutMs = 0);
  virtual int PlayVideo(const uchar *Data, int Length);
  virtual void PlayAudio(const uchar *Data, int Length);

// Receiver facilities

private:
  cTSBuffer *tsBuffer;
protected:
  virtual bool OpenDvr(void);
  virtual void CloseDvr(void);
  virtual bool GetTSPacket(uchar *&Data);
  };

#endif //__DVBDEVICE_H
