/*
 * epg.h: Electronic Program Guide
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * Original version (as used in VDR before 1.3.0) written by
 * Robert Schneider <Robert.Schneider@web.de> and Rolf Hakenes <hakenes@hippomi.de>.
 *
 * $Id: epg.h 1.4 2004/01/09 15:21:05 kls Exp $
 */

#ifndef __EPG_H
#define __EPG_H

#include "channels.h"
#include "thread.h"
#include "tools.h"

#define MAXEPGBUGFIXLEVEL 2

class cSchedule;

class cEvent : public cListObject {
private:
  tChannelID channelID;  // Channel ID of program for this event
  u_int16_t eventID;     // Event ID of this event
  uchar tableID;         // Table ID this event came from
  uchar version;         // Version number of section this event came from
  //XXX present/following obsolete???
  bool isPresent;        // true if this is the present event running
  bool isFollowing;      // true if this is the next event on this channel
  char *title;           // Title of this event
  char *shortText;       // Short description of this event (typically the episode name in case of a series)
  char *description;     // Description of this event
  time_t startTime;      // Start time of this event
  int duration;          // Duration of this event in seconds
  //XXX find an other solution, avoiding channelNumber???
  int channelNumber;     // the actual channel number from VDR's channel list (used in cMenuSchedule for sorting by channel number)
public:
  cEvent(tChannelID ChannelID, u_int16_t EventID);
  ~cEvent();
  tChannelID ChannelID(void) const { return channelID; }
  u_int16_t EventID(void) const { return eventID; }
  uchar TableID(void) const { return tableID; }
  uchar Version(void) const { return version; }
  bool IsPresent(void) const { return isPresent; }
  bool IsFollowing(void) const { return isFollowing; }
  const char *Title(void) const { return title; }
  const char *ShortText(void) const { return shortText; }
  const char *Description(void) const { return description; }
  time_t StartTime(void) const { return startTime; }
  int Duration(void) const { return duration; }
  int ChannelNumber(void) const { return channelNumber; }
  const char *GetDateString(void) const;
  const char *GetTimeString(void) const;
  const char *GetEndTimeString(void) const;
  void SetEventID(u_int16_t EventID);
  void SetTableID(uchar TableID);
  void SetVersion(uchar Version);
  void SetIsPresent(bool IsPresent);
  void SetIsFollowing(bool IsFollowing);
  void SetTitle(const char *Title);
  void SetShortText(const char *ShortText);
  void SetDescription(const char *Description);
  void SetStartTime(time_t StartTime);
  void SetDuration(int Duration);
  void SetChannelNumber(int ChannelNumber) const { ((cEvent *)this)->channelNumber = ChannelNumber; } // doesn't modify the EIT data, so it's ok to make it 'const' //XXX
  void Dump(FILE *f, const char *Prefix = "") const;
  static bool Read(FILE *f, cSchedule *Schedule);
  void FixEpgBugs(void);
  };

class cSchedules;

class cSchedule : public cListObject  {
private:
  tChannelID channelID;
  cList<cEvent> events;
  cEvent *present;
  cEvent *following;
public:
  cSchedule(tChannelID ChannelID);
  tChannelID ChannelID(void) const { return channelID; }
  bool SetPresentEvent(cEvent *Event);
  bool SetFollowingEvent(cEvent *Event);
  void ResetVersions(void);
  void Cleanup(time_t Time);
  void Cleanup(void);
  cEvent *AddEvent(cEvent *Event);
  const cEvent *GetPresentEvent(void) const;
  const cEvent *GetFollowingEvent(void) const;
  const cEvent *GetEvent(u_int16_t EventID, time_t StartTime = 0) const;
  const cEvent *GetEventAround(time_t Time) const;
  const cEvent *GetEventNumber(int n) const { return events.Get(n); }
  int NumEvents(void) const { return events.Count(); }
  void Dump(FILE *f, const char *Prefix = "") const;
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
public:
  static void SetEpgDataFileName(const char *FileName);
  static const cSchedules *Schedules(cSchedulesLock &SchedulesLock);
         ///< Caller must provide a cSchedulesLock which has to survive the entire
         ///< time the returned cSchedules is accessed. Once the cSchedules is no
         ///< longer used, the cSchedulesLock must be destroyed.
  static void Cleanup(bool Force = false);
  static void ResetVersions(void);
  static bool ClearAll(void);
  static bool Dump(FILE *f, const char *Prefix = "");
  static bool Read(FILE *f = NULL);
  cSchedule *AddSchedule(tChannelID ChannelID);
  const cSchedule *GetSchedule(tChannelID ChannelID) const;
  };

void ReportEpgBugFixStats(bool Reset = false);

#endif //__EPG_H
