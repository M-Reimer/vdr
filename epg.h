/*
 * epg.h: Electronic Program Guide
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * Original version (as used in VDR before 1.3.0) written by
 * Robert Schneider <Robert.Schneider@web.de> and Rolf Hakenes <hakenes@hippomi.de>.
 *
 * $Id: epg.h 1.17 2004/10/31 16:17:10 kls Exp $
 */

#ifndef __EPG_H
#define __EPG_H

#include "channels.h"
#include "thread.h"
#include "tools.h"

#define MAXEPGBUGFIXLEVEL 2

enum eDumpMode { dmAll, dmPresent, dmFollowing, dmAtTime };

class cSchedule;

class cEvent : public cListObject {
private:
  tChannelID channelID;  // Channel ID of program for this event
  u_int16_t eventID;     // Event ID of this event
  uchar tableID;         // Table ID this event came from
  uchar version;         // Version number of section this event came from
  int runningStatus;     // 0=undefined, 1=not running, 2=starts in a few seconds, 3=pausing, 4=running
  char *title;           // Title of this event
  char *shortText;       // Short description of this event (typically the episode name in case of a series)
  char *description;     // Description of this event
  time_t startTime;      // Start time of this event
  int duration;          // Duration of this event in seconds
  time_t vps;            // Video Programming Service timestamp (VPS, aka "Programme Identification Label", PIL)
  time_t seen;           // When this event was last seen in the data stream
public:
  cEvent(tChannelID ChannelID, u_int16_t EventID);
  ~cEvent();
  virtual int Compare(const cListObject &ListObject) const;
  tChannelID ChannelID(void) const { return channelID; }
  u_int16_t EventID(void) const { return eventID; }
  uchar TableID(void) const { return tableID; }
  uchar Version(void) const { return version; }
  int RunningStatus(void) const { return runningStatus; }
  const char *Title(void) const { return title; }
  const char *ShortText(void) const { return shortText; }
  const char *Description(void) const { return description; }
  time_t StartTime(void) const { return startTime; }
  time_t EndTime(void) const { return startTime + duration; }
  int Duration(void) const { return duration; }
  time_t Vps(void) const { return vps; }
  time_t Seen(void) const { return seen; }
  bool HasTimer(void) const;
  bool IsRunning(bool OrAboutToStart = false) const;
  const char *GetDateString(void) const;
  const char *GetTimeString(void) const;
  const char *GetEndTimeString(void) const;
  const char *GetVpsString(void) const;
  void SetEventID(u_int16_t EventID);
  void SetTableID(uchar TableID);
  void SetVersion(uchar Version);
  void SetRunningStatus(int RunningStatus, cChannel *Channel = NULL);
  void SetTitle(const char *Title);
  void SetShortText(const char *ShortText);
  void SetDescription(const char *Description);
  void SetStartTime(time_t StartTime);
  void SetDuration(int Duration);
  void SetVps(time_t Vps);
  void SetSeen(void);
  void Dump(FILE *f, const char *Prefix = "") const;
  static bool Read(FILE *f, cSchedule *Schedule);
  void FixEpgBugs(void);
  };

class cSchedules;

class cSchedule : public cListObject  {
private:
  tChannelID channelID;
  cList<cEvent> events;
  bool hasRunning;
  time_t modified;
public:
  cSchedule(tChannelID ChannelID);
  tChannelID ChannelID(void) const { return channelID; }
  time_t Modified(void) const { return modified; }
  void SetModified(void) { modified = time(NULL); }
  void SetRunningStatus(cEvent *Event, int RunningStatus, cChannel *Channel = NULL);
  void ClrRunningStatus(cChannel *Channel = NULL);
  void ResetVersions(void);
  void Sort(void);
  void Cleanup(time_t Time);
  void Cleanup(void);
  cEvent *AddEvent(cEvent *Event);
  const cList<cEvent> *Events(void) const { return &events; }
  const cEvent *GetPresentEvent(bool CheckRunningStatus = false) const;
  const cEvent *GetFollowingEvent(bool CheckRunningStatus = false) const;
  const cEvent *GetEvent(u_int16_t EventID, time_t StartTime = 0) const;
  const cEvent *GetEventAround(time_t Time) const;
  void Dump(FILE *f, const char *Prefix = "", eDumpMode DumpMode = dmAll, time_t AtTime = 0) const;
  static bool Read(FILE *f, cSchedules *Schedules);
  };

class cSchedulesLock {
private:
  bool locked;
public:
  cSchedulesLock(bool WriteLock = false, int TimeoutMs = 0);
  ~cSchedulesLock();
  bool Locked(void) { return locked; }
  };

class cSchedules : public cList<cSchedule> {
  friend class cSchedule;
  friend class cSchedulesLock;
private:
  cRwLock rwlock;
  static cSchedules schedules;
  static const char *epgDataFileName;
  static time_t lastCleanup;
  static time_t lastDump;
  static time_t modified;
public:
  static void SetEpgDataFileName(const char *FileName);
  static const cSchedules *Schedules(cSchedulesLock &SchedulesLock);
         ///< Caller must provide a cSchedulesLock which has to survive the entire
         ///< time the returned cSchedules is accessed. Once the cSchedules is no
         ///< longer used, the cSchedulesLock must be destroyed.
  static time_t Modified(void) { return modified; }
  static void SetModified(cSchedule *Schedule);
  static void Cleanup(bool Force = false);
  static void ResetVersions(void);
  static bool ClearAll(void);
  static bool Dump(FILE *f, const char *Prefix = "", eDumpMode DumpMode = dmAll, time_t AtTime = 0);
  static bool Read(FILE *f = NULL);
  cSchedule *AddSchedule(tChannelID ChannelID);
  const cSchedule *GetSchedule(tChannelID ChannelID) const;
  };

void ReportEpgBugFixStats(bool Reset = false);

#endif //__EPG_H
