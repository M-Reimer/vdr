/*
 * epg.c: Electronic Program Guide
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * Original version (as used in VDR before 1.3.0) written by
 * Robert Schneider <Robert.Schneider@web.de> and Rolf Hakenes <hakenes@hippomi.de>.
 *
 * $Id: epg.c 1.19 2004/05/22 12:37:07 kls Exp $
 */

#include "epg.h"
#include "libsi/si.h"
#include "timers.h"
#include <ctype.h>
#include <time.h>

// --- cEvent ----------------------------------------------------------------

cEvent::cEvent(tChannelID ChannelID, u_int16_t EventID)
{
  channelID = ChannelID;
  eventID = EventID;
  tableID = 0;
  version = 0xFF; // actual version numbers are 0..31
  runningStatus = 0;
  title = NULL;
  shortText = NULL;
  description = NULL;
  startTime = 0;
  duration = 0;
  vps = 0;
}

cEvent::~cEvent()
{
  free(title);
  free(shortText);
  free(description);
}

bool cEvent::operator< (const cListObject &ListObject)
{
  cEvent *e = (cEvent *)&ListObject;
  return startTime < e->startTime;
}

void cEvent::SetEventID(u_int16_t EventID)
{
  eventID = EventID;
}

void cEvent::SetTableID(uchar TableID)
{
  tableID = TableID;
}

void cEvent::SetVersion(uchar Version)
{
  version = Version;
}

void cEvent::SetRunningStatus(int RunningStatus, cChannel *Channel)
{
  if (Channel && runningStatus != RunningStatus && (RunningStatus > SI::RunningStatusNotRunning || runningStatus > SI::RunningStatusUndefined))
     if (Channel->Number() <= 30)//XXX maybe log only those that have timers???
     isyslog("channel %d (%s) event %s '%s' status %d", Channel->Number(), Channel->Name(), GetTimeString(), Title(), RunningStatus);
  runningStatus = RunningStatus;
}

void cEvent::SetTitle(const char *Title)
{
  title = strcpyrealloc(title, Title);
}

void cEvent::SetShortText(const char *ShortText)
{
  shortText = strcpyrealloc(shortText, ShortText);
}

void cEvent::SetDescription(const char *Description)
{
  description = strcpyrealloc(description, Description);
}

void cEvent::SetStartTime(time_t StartTime)
{
  startTime = StartTime;
}

void cEvent::SetDuration(int Duration)
{
  duration = Duration;
}

void cEvent::SetVps(time_t Vps)
{
  vps = Vps;
}

bool cEvent::HasTimer(void) const
{
  for (cTimer *t = Timers.First(); t; t = Timers.Next(t)) {
      if (t->Event() == this)
         return true;
      }
  return false;
}

bool cEvent::IsRunning(bool OrAboutToStart) const
{
  return runningStatus >= (OrAboutToStart ? SI::RunningStatusStartsInAFewSeconds : SI::RunningStatusPausing);
}

const char *cEvent::GetDateString(void) const
{
  static char buf[32];
  struct tm tm_r;
  tm *tm = localtime_r(&startTime, &tm_r);
  char *p = stpcpy(buf, WeekDayName(tm->tm_wday));
  *p++ = ' ';
  strftime(p, sizeof(buf) - (p - buf), "%d.%m.%Y", tm);
  return buf;
}

const char *cEvent::GetTimeString(void) const
{
  static char buf[25];
  struct tm tm_r;
  strftime(buf, sizeof(buf), "%R", localtime_r(&startTime, &tm_r));
  return buf;
}

const char *cEvent::GetEndTimeString(void) const
{
  static char buf[25];
  time_t EndTime = startTime + duration;
  struct tm tm_r;
  strftime(buf, sizeof(buf), "%R", localtime_r(&EndTime, &tm_r));
  return buf;
}

const char *cEvent::GetVpsString(void) const
{
  static char buf[25];
  struct tm tm_r;
  strftime(buf, sizeof(buf), "%d.%m %R", localtime_r(&vps, &tm_r));
  return buf;
}

void cEvent::Dump(FILE *f, const char *Prefix) const
{
  if (startTime + duration >= time(NULL)) {
     fprintf(f, "%sE %u %ld %d %X\n", Prefix, eventID, startTime, duration, tableID);
     if (!isempty(title))
        fprintf(f, "%sT %s\n", Prefix, title);
     if (!isempty(shortText))
        fprintf(f, "%sS %s\n", Prefix, shortText);
     if (!isempty(description)) {
        strreplace(description, '\n', '|');
        fprintf(f, "%sD %s\n", Prefix, description);
        strreplace(description, '|', '\n');
        }
     if (vps)
        fprintf(f, "%sV %ld\n", Prefix, vps);
     fprintf(f, "%se\n", Prefix);
     }
}

bool cEvent::Read(FILE *f, cSchedule *Schedule)
{
  if (Schedule) {
     cEvent *Event = NULL;
     char *s;
     while ((s = readline(f)) != NULL) {
           char *t = skipspace(s + 1);
           switch (*s) {
             case 'E': if (!Event) {
                          unsigned int EventID;
                          time_t StartTime;
                          int Duration;
                          unsigned int TableID = 0;
                          int n = sscanf(t, "%u %ld %d %X", &EventID, &StartTime, &Duration, &TableID);
                          if (n == 3 || n == 4) {
                             Event = (cEvent *)Schedule->GetEvent(EventID, StartTime);
                             if (!Event)
                                Event = Schedule->AddEvent(new cEvent(Schedule->ChannelID(), EventID));
                             if (Event) {
                                Event->SetTableID(TableID);
                                Event->SetStartTime(StartTime);
                                Event->SetDuration(Duration);
                                }
                             }
                          }
                       break;
             case 'T': if (Event)
                          Event->SetTitle(t);
                       break;
             case 'S': if (Event)
                          Event->SetShortText(t);
                       break;
             case 'D': if (Event) {
                          strreplace(t, '|', '\n');
                          Event->SetDescription(t);
                          }
                       break;
             case 'V': if (Event)
                          Event->SetVps(atoi(t));
                       break;
             case 'e': Event = NULL;
                       break;
             case 'c': // to keep things simple we react on 'c' here
                       return true;
             default:  esyslog("ERROR: unexpected tag while reading EPG data: %s", s);
                       return false;
             }
           }
     esyslog("ERROR: unexpected end of file while reading EPG data");
     }
  return false;
}

#define MAXEPGBUGFIXSTATS 8
#define MAXEPGBUGFIXCHANS 100
struct tEpgBugFixStats {
  int hits;
  int n;
  tChannelID channelIDs[MAXEPGBUGFIXCHANS];
  tEpgBugFixStats(void) { hits = n = 0; }
  };

tEpgBugFixStats EpgBugFixStats[MAXEPGBUGFIXSTATS];

static void EpgBugFixStat(int Number, tChannelID ChannelID)
{
  if (0 <= Number && Number < MAXEPGBUGFIXSTATS) {
     tEpgBugFixStats *p = &EpgBugFixStats[Number];
     p->hits++;
     int i = 0;
     for (; i < p->n; i++) {
         if (p->channelIDs[i] == ChannelID)
            break;
         }
     if (i == p->n && p->n < MAXEPGBUGFIXCHANS)
        p->channelIDs[p->n++] = ChannelID;
     }
}

void ReportEpgBugFixStats(bool Reset)
{
  if (Setup.EPGBugfixLevel > 0) {
     bool GotHits = false;
     char buffer[1024];
     for (int i = 0; i < MAXEPGBUGFIXSTATS; i++) {
         const char *delim = "\t";
         tEpgBugFixStats *p = &EpgBugFixStats[i];
         if (p->hits) {
            bool PrintedStats = false;
            char *q = buffer;
            *buffer = 0;
            for (int c = 0; c < p->n; c++) {
                cChannel *channel = Channels.GetByChannelID(p->channelIDs[c], true);
                if (channel) {
                   if (!GotHits) {
                      dsyslog("=====================");
                      dsyslog("EPG bugfix statistics");
                      dsyslog("=====================");
                      dsyslog("IF SOMEBODY WHO IS IN CHARGE OF THE EPG DATA FOR ONE OF THE LISTED");
                      dsyslog("CHANNELS READS THIS: PLEASE TAKE A LOOK AT THE FUNCTION cEvent::FixEpgBugs()");
                      dsyslog("IN VDR/epg.c TO LEARN WHAT'S WRONG WITH YOUR DATA, AND FIX IT!");
                      dsyslog("=====================");
                      dsyslog("Fix\tHits\tChannels");
                      GotHits = true;
                      }
                   if (!PrintedStats) {
                      q += snprintf(q, sizeof(buffer) - (q - buffer), "%d\t%d", i, p->hits);
                      PrintedStats = true;
                      }
                   q += snprintf(q, sizeof(buffer) - (q - buffer), "%s%s", delim, channel->Name());
                   delim = ", ";
                   if (q - buffer > 80) {
                      q += snprintf(q, sizeof(buffer) - (q - buffer), "%s...", delim);
                      break;
                      }
                   }
                }
            if (*buffer)
               dsyslog("%s", buffer);
            }
         if (Reset)
            p->hits = p->n = 0;
         }
     if (GotHits)
        dsyslog("=====================");
     }
}

void cEvent::FixEpgBugs(void)
{
  // VDR can't usefully handle newline characters in the title and shortText of EPG
  // data, so let's always convert them to blanks (independent of the setting of EPGBugfixLevel):
  strreplace(title, '\n', ' ');
  strreplace(shortText, '\n', ' ');
  // Same for control characters:
  strreplace(title, '\x86', ' ');
  strreplace(title, '\x87', ' ');
  strreplace(shortText, '\x86', ' ');
  strreplace(shortText, '\x87', ' ');
  strreplace(description, '\x86', ' ');
  strreplace(description, '\x87', ' ');

  if (Setup.EPGBugfixLevel == 0)
     return;

  // Some TV stations apparently have their own idea about how to fill in the
  // EPG data. Let's fix their bugs as good as we can:
  if (title) {

     // Some channels put too much information into the ShortText and leave the
     // Description empty:
     //
     // Title
     // (NAT, Year Min')[ ["ShortText". ]Description]
     //
     if (shortText && !description) {
        if (*shortText == '(') {
           char *e = strchr(shortText + 1, ')');
           if (e) {
              if (*(e + 1)) {
                 if (*++e == ' ')
                    if (*(e + 1) == '"')
                       e++;
                 }
              else
                 e = NULL;
              char *s = e ? strdup(e) : NULL;
              free(shortText);
              shortText = s;
              EpgBugFixStat(0, ChannelID());
              // now the fixes #1 and #2 below will handle the rest
              }
           }
        }

     // Some channels put the ShortText in quotes and use either the ShortText
     // or the Description field, depending on how long the string is:
     //
     // Title
     // "ShortText". Description
     //
     if ((shortText == NULL) != (description == NULL)) {
        char *p = shortText ? shortText : description;
        if (*p == '"') {
           const char *delim = "\".";
           char *e = strstr(p + 1, delim);
           if (e) {
              *e = 0;
              char *s = strdup(p + 1);
              char *d = strdup(e + strlen(delim));
              free(shortText);
              free(description);
              shortText = s;
              description = d;
              EpgBugFixStat(1, ChannelID());
              }
           }
        }

     // Some channels put the Description into the ShortText (preceeded
     // by a blank) if there is no actual ShortText and the Description
     // is short enough:
     //
     // Title
     //  Description
     //
     if (shortText && !description) {
        if (*shortText == ' ') {
           memmove(shortText, shortText + 1, strlen(shortText));
           description = shortText;
           shortText = NULL;
           EpgBugFixStat(2, ChannelID());
           }
        }

     // Sometimes they repeat the Title in the ShortText:
     //
     // Title
     // Title
     //
     if (shortText && strcmp(title, shortText) == 0) {
        free(shortText);
        shortText = NULL;
        EpgBugFixStat(3, ChannelID());
        }

     // Some channels put the ShortText between double quotes, which is nothing
     // but annoying (some even put a '.' after the closing '"'):
     //
     // Title
     // "ShortText"[.]
     //
     if (shortText && *shortText == '"') {
        int l = strlen(shortText);
        if (l > 2 && (shortText[l - 1] == '"' || (shortText[l - 1] == '.' && shortText[l - 2] == '"'))) {
           memmove(shortText, shortText + 1, l);
           char *p = strrchr(shortText, '"');
           if (p)
              *p = 0;
           EpgBugFixStat(4, ChannelID());
           }
        }

     if (Setup.EPGBugfixLevel <= 1)
        return;

     // Some channels apparently try to do some formatting in the texts,
     // which is a bad idea because they have no way of knowing the width
     // of the window that will actually display the text.
     // Remove excess whitespace:
     title = compactspace(title);
     shortText = compactspace(shortText);
     description = compactspace(description);
     // Remove superfluous hyphens:
     if (description) {
        char *p = description;
        while (*p && *(p + 1) && *(p + 2)) {
              if (*p == '-' && *(p + 1) == ' ' && *(p + 2) && islower(*(p - 1)) && islower(*(p + 2))) {
                 if (!startswith(p + 2, "und ")) { // special case in German, as in "Lach- und Sachgeschichten"
                    memmove(p, p + 2, strlen(p + 2) + 1);
                    EpgBugFixStat(5, ChannelID());
                    }
                 }
              p++;
              }
        }

#define MAX_USEFUL_EPISODE_LENGTH 40
     // Some channels put a whole lot of information in the ShortText and leave
     // the Description totally empty. So if the ShortText length exceeds
     // MAX_USEFUL_EPISODE_LENGTH, let's put this into the Description
     // instead:
     if (!isempty(shortText) && isempty(description)) {
        if (strlen(shortText) > MAX_USEFUL_EPISODE_LENGTH) {
           free(description);
           description = shortText;
           shortText = NULL;
           EpgBugFixStat(6, ChannelID());
           }
        }

     // Some channels put the same information into ShortText and Description.
     // In that case we delete one of them:
     if (shortText && description && strcmp(shortText, description) == 0) {
        if (strlen(shortText) > MAX_USEFUL_EPISODE_LENGTH) {
           free(shortText);
           shortText = NULL;
           }
        else {
           free(description);
           description = NULL;
           }
        EpgBugFixStat(7, ChannelID());
        }

     // Some channels use the ` ("backtick") character, where a ' (single quote)
     // would be normally used. Actually, "backticks" in normal text don't make
     // much sense, so let's replace them:
     strreplace(title, '`', '\'');
     strreplace(shortText, '`', '\'');
     strreplace(description, '`', '\'');
     }
}

// --- cSchedule -------------------------------------------------------------

cSchedule::cSchedule(tChannelID ChannelID)
{
  channelID = ChannelID;
  hasRunning = false;;
}

cEvent *cSchedule::AddEvent(cEvent *Event)
{
  events.Add(Event);
  return Event;
}

const cEvent *cSchedule::GetPresentEvent(bool CheckRunningStatus) const
{
  const cEvent *pe = NULL;
  time_t now = time(NULL);
  for (cEvent *p = events.First(); p; p = events.Next(p)) {
      if (p->StartTime() <= now && now < p->EndTime()) {
         pe = p;
         if (!CheckRunningStatus)
            break;
         }
      if (CheckRunningStatus && p->RunningStatus() >= SI::RunningStatusPausing)
         return p;
      }
  return pe;
}

const cEvent *cSchedule::GetFollowingEvent(bool CheckRunningStatus) const
{
  const cEvent *p = GetPresentEvent(CheckRunningStatus);
  if (p)
     p = events.Next(p);
  return p;
}

const cEvent *cSchedule::GetEvent(u_int16_t EventID, time_t StartTime) const
{
  // Returns either the event info with the given EventID or, if that one can't
  // be found, the one with the given StartTime (or NULL if neither can be found)
  cEvent *pt = NULL;
  for (cEvent *pe = events.First(); pe; pe = events.Next(pe)) {
      if (pe->EventID() == EventID)
         return pe;
      if (StartTime > 0 && pe->StartTime() == StartTime) // 'StartTime < 0' is apparently used with NVOD channels
         pt = pe;
      }
  return pt;
}

const cEvent *cSchedule::GetEventAround(time_t Time) const
{
  const cEvent *pe = NULL;
  time_t delta = INT_MAX;
  for (cEvent *p = events.First(); p; p = events.Next(p)) {
      time_t dt = Time - p->StartTime();
      if (dt >= 0 && dt < delta && p->EndTime() >= Time) {
         delta = dt;
         pe = p;
         }
      }
  return pe;
}

void cSchedule::SetRunningStatus(cEvent *Event, int RunningStatus, cChannel *Channel)
{
  for (cEvent *p = events.First(); p; p = events.Next(p)) {
      if (p == Event)
         p->SetRunningStatus(RunningStatus, Channel);
      else if (RunningStatus >= SI::RunningStatusPausing && p->RunningStatus() > SI::RunningStatusNotRunning)
         p->SetRunningStatus(SI::RunningStatusNotRunning);
      }
  if (RunningStatus >= SI::RunningStatusPausing)
     hasRunning = true;
}

void cSchedule::ClrRunningStatus(cChannel *Channel)
{
  if (hasRunning) {
     for (cEvent *p = events.First(); p; p = events.Next(p)) {
         if (p->RunningStatus() >= SI::RunningStatusPausing) {
            p->SetRunningStatus(SI::RunningStatusNotRunning, Channel);
            hasRunning = false;
            break;
            }
         }
     }
}

void cSchedule::ResetVersions(void)
{
  for (cEvent *p = events.First(); p; p = events.Next(p))
      p->SetVersion(0xFF);
}

void cSchedule::Sort(void)
{
  events.Sort();
}

void cSchedule::Cleanup(void)
{
  Cleanup(time(NULL));
}

void cSchedule::Cleanup(time_t Time)
{
  cEvent *Event;
  for (int a = 0; true ; a++) {
      Event = events.Get(a);
      if (!Event)
         break;
      if (!Event->HasTimer() && Event->EndTime() + Setup.EPGLinger * 60 + 3600 < Time) { // adding one hour for safety
         events.Del(Event);
         a--;
         }
      }
}

void cSchedule::Dump(FILE *f, const char *Prefix, eDumpMode DumpMode, time_t AtTime) const
{
  cChannel *channel = Channels.GetByChannelID(channelID, true);
  if (channel) {
     fprintf(f, "%sC %s %s\n", Prefix, channel->GetChannelID().ToString(), channel->Name());
     const cEvent *p;
     switch (DumpMode) {
       case dmAll: {
            for (p = events.First(); p; p = events.Next(p))
                p->Dump(f, Prefix);
            }
            break;
       case dmPresent: {
            if ((p = GetPresentEvent()) != NULL)
               p->Dump(f, Prefix);
            }
            break;
       case dmFollowing: {
            if ((p = GetFollowingEvent()) != NULL)
               p->Dump(f, Prefix);
            }
            break;
       case dmAtTime: {
            if ((p = GetEventAround(AtTime)) != NULL)
               p->Dump(f, Prefix);
            }
            break;
       }
     fprintf(f, "%sc\n", Prefix);
     }
}

bool cSchedule::Read(FILE *f, cSchedules *Schedules)
{
  if (Schedules) {
     char *s;
     while ((s = readline(f)) != NULL) {
           if (*s == 'C') {
              s = skipspace(s + 1);
              char *p = strchr(s, ' ');
              if (p)
                 *p = 0; // strips optional channel name
              if (*s) {
                 tChannelID channelID = tChannelID::FromString(s);
                 if (channelID.Valid()) {
                    cSchedule *p = Schedules->AddSchedule(channelID);
                    if (p) {
                       if (!cEvent::Read(f, p))
                          return false;
                       p->Sort();
                       }
                    }
                 else {
                    esyslog("ERROR: illegal channel ID: %s", s);
                    return false;
                    }
                 }
              }
           else {
              esyslog("ERROR: unexpected tag while reading EPG data: %s", s);
              return false;
              }
           }
     return true;
     }
  return false;
}

// --- cSchedulesLock --------------------------------------------------------

cSchedulesLock::cSchedulesLock(bool WriteLock, int TimeoutMs)
{
  locked = cSchedules::schedules.rwlock.Lock(WriteLock, TimeoutMs);
}

cSchedulesLock::~cSchedulesLock()
{
  if (locked)
     cSchedules::schedules.rwlock.Unlock();
}

// --- cSchedules ------------------------------------------------------------

cSchedules cSchedules::schedules;
const char *cSchedules::epgDataFileName = NULL;
time_t cSchedules::lastCleanup = time(NULL);
time_t cSchedules::lastDump = time(NULL);

const cSchedules *cSchedules::Schedules(cSchedulesLock &SchedulesLock)
{
  return SchedulesLock.Locked() ? &schedules : NULL;
}

void cSchedules::SetEpgDataFileName(const char *FileName)
{
  delete epgDataFileName;
  if (FileName)
     epgDataFileName = strdup(FileName);
}

void cSchedules::Cleanup(bool Force)
{
  if (Force)
     lastDump = 0;
  time_t now = time(NULL);
  struct tm tm_r;
  struct tm *ptm = localtime_r(&now, &tm_r);
  if (now - lastCleanup > 3600 && ptm->tm_hour == 5) {
     isyslog("cleaning up schedules data");
     cSchedulesLock SchedulesLock(true, 1000);
     cSchedules *s = (cSchedules *)Schedules(SchedulesLock);
     if (s) {
        for (cSchedule *p = s->First(); p; p = s->Next(p))
            p->Cleanup(now);
        }
     lastCleanup = now;
     ReportEpgBugFixStats(true);
     }
  if (epgDataFileName && now - lastDump > 600) {
     cSafeFile f(epgDataFileName);
     if (f.Open()) {
        Dump(f);
        f.Close();
        }
     else
        LOG_ERROR;
     lastDump = now;
     }
}

void cSchedules::ResetVersions(void)
{
  cSchedulesLock SchedulesLock(true);
  cSchedules *s = (cSchedules *)Schedules(SchedulesLock);
  if (s) {
     for (cSchedule *Schedule = s->First(); Schedule; Schedule = s->Next(Schedule))
         Schedule->ResetVersions();
     }
}

bool cSchedules::ClearAll(void)
{
  cSchedulesLock SchedulesLock(true, 1000);
  cSchedules *s = (cSchedules *)Schedules(SchedulesLock);
  if (s) {
     s->Clear();
     return true;
     }
  return false;
}

bool cSchedules::Dump(FILE *f, const char *Prefix, eDumpMode DumpMode, time_t AtTime)
{
  cSchedulesLock SchedulesLock;
  cSchedules *s = (cSchedules *)Schedules(SchedulesLock);
  if (s) {
     for (cSchedule *p = s->First(); p; p = s->Next(p))
         p->Dump(f, Prefix, DumpMode, AtTime);
     return true;
     }
  return false;
}

bool cSchedules::Read(FILE *f)
{
  cSchedulesLock SchedulesLock(true, 1000);
  cSchedules *s = (cSchedules *)Schedules(SchedulesLock);
  if (s) {
     bool OwnFile = f == NULL;
     if (OwnFile) {
        if (epgDataFileName && access(epgDataFileName, R_OK) == 0) {
           dsyslog("reading EPG data from %s", epgDataFileName);
           if ((f = fopen(epgDataFileName, "r")) == NULL) {
              LOG_ERROR;
              return false;
              }
           }
        else
           return false;
        }
     bool result = cSchedule::Read(f, s);
     if (OwnFile)
        fclose(f);
     return result;
     }
  return false;
}

cSchedule *cSchedules::AddSchedule(tChannelID ChannelID)
{
  ChannelID.ClrRid();
  cSchedule *p = (cSchedule *)GetSchedule(ChannelID);
  if (!p) {
     p = new cSchedule(ChannelID);
     Add(p);
     }
  return p;
}

const cSchedule *cSchedules::GetSchedule(tChannelID ChannelID) const
{
  ChannelID.ClrRid();
  for (cSchedule *p = First(); p; p = Next(p)) {
      if (p->ChannelID() == ChannelID)
         return p;
      }
  return NULL;
}

