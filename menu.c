/*
 * menu.c: The actual menu implementations
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: menu.c 1.208 2002/09/06 14:07:58 kls Exp $
 */

#include "menu.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "cutter.h"
#include "eit.h"
#include "i18n.h"
#include "menuitems.h"
#include "plugin.h"
#include "recording.h"
#include "status.h"
#include "videodir.h"

#define MENUTIMEOUT     120 // seconds
#define MAXWAIT4EPGINFO  10 // seconds
#define MODETIMEOUT       3 // seconds

#define MAXRECORDCONTROLS (MAXDEVICES * MAXRECEIVERS)
#define MAXINSTANTRECTIME (24 * 60 - 1) // 23:59 hours

#define CHNUMWIDTH  (Channels.Count() > 999 ? 5 : 4) // there are people with more than 999 channels...

// --- cMenuEditChanItem -----------------------------------------------------

class cMenuEditChanItem : public cMenuEditIntItem {
protected:
  virtual void Set(void);
public:
  cMenuEditChanItem(const char *Name, int *Value);
  };

cMenuEditChanItem::cMenuEditChanItem(const char *Name, int *Value)
:cMenuEditIntItem(Name, Value, 1, Channels.MaxNumber())
{
  Set();
}

void cMenuEditChanItem::Set(void)
{
  char buf[255];
  cChannel *channel = Channels.GetByNumber(*value);
  if (channel)
     snprintf(buf, sizeof(buf), "%d %s", *value, channel->name);
  else
     *buf = 0;
  SetValue(buf);
}

// --- cMenuEditTranItem -----------------------------------------------------

class cMenuEditTranItem : public cMenuEditChanItem {
private:
  int number;
  int transponder;
public:
  cMenuEditTranItem(const char *Name, int *Value);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuEditTranItem::cMenuEditTranItem(const char *Name, int *Value)
:cMenuEditChanItem(Name, Value)
{
  number = 0;
  transponder = *Value;
  cChannel *channel = Channels.First();
  while (channel) {
        if (!channel->groupSep && ISTRANSPONDER(channel->frequency, *Value)) {
           number = channel->number;
           break;
           }
        channel = (cChannel *)channel->Next();
        }
  *Value = number;
  Set();
  *Value = transponder;
}

eOSState cMenuEditTranItem::ProcessKey(eKeys Key)
{
  *value = number;
  eOSState state = cMenuEditChanItem::ProcessKey(Key);
  number = *value;
  cChannel *channel = Channels.GetByNumber(*value);
  if (channel)
     transponder = channel->frequency;
  *value = transponder;
  return state;
}

// --- cMenuEditDayItem ------------------------------------------------------

class cMenuEditDayItem : public cMenuEditIntItem {
protected:
  static int days[];
  int d;
  virtual void Set(void);
public:
  cMenuEditDayItem(const char *Name, int *Value);
  virtual eOSState ProcessKey(eKeys Key);
  };

int cMenuEditDayItem::days[] ={ cTimer::ParseDay("M------"),
                                cTimer::ParseDay("-T-----"),
                                cTimer::ParseDay("--W----"),
                                cTimer::ParseDay("---T---"),
                                cTimer::ParseDay("----F--"),
                                cTimer::ParseDay("-----S-"),
                                cTimer::ParseDay("------S"),
                                cTimer::ParseDay("MTWTF--"),
                                cTimer::ParseDay("MTWTFS-"),
                                cTimer::ParseDay("MTWTFSS"),
                                cTimer::ParseDay("-----SS"),
                                0 };

cMenuEditDayItem::cMenuEditDayItem(const char *Name, int *Value)
:cMenuEditIntItem(Name, Value, -INT_MAX, 31)
{
  d = -1;
  if (*value < 0) {
     int n = 0;
     while (days[n]) {
           if (days[n] == *value) {
              d = n;
              break;
              }
           n++;
           }
     }
  Set();
}

void cMenuEditDayItem::Set(void)
{
  SetValue(cTimer::PrintDay(*value));
}

eOSState cMenuEditDayItem::ProcessKey(eKeys Key)
{
  switch (Key) {
    case kLeft|k_Repeat:
    case kLeft:  if (d > 0)
                    *value = days[--d];
                 else if (d == 0) {
                    *value = 31;
                    d = -1;
                    }
                 else if (*value == 1) {
                    d = sizeof(days) / sizeof(int) - 2;
                    *value = days[d];
                    }
                 else
                    return cMenuEditIntItem::ProcessKey(Key);
                 Set();
                 break;
    case kRight|k_Repeat:
    case kRight: if (d >= 0) {
                    *value = days[++d];
                    if (*value == 0) {
                       *value = 1;
                       d = -1;
                       }
                    }
                 else if (*value == 31) {
                    d = 0;
                    *value = days[d];
                    }
                 else
                    return cMenuEditIntItem::ProcessKey(Key);
                 Set();
                 break;
    default : return cMenuEditIntItem::ProcessKey(Key);
    }
  return osContinue;
}

// --- cMenuEditDateItem -----------------------------------------------------

class cMenuEditDateItem : public cMenuEditItem {
protected:
  time_t *value;
  virtual void Set(void);
public:
  cMenuEditDateItem(const char *Name, time_t *Value);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuEditDateItem::cMenuEditDateItem(const char *Name, time_t *Value)
:cMenuEditItem(Name)
{
  value = Value;
  Set();
}

void cMenuEditDateItem::Set(void)
{
#define DATEBUFFERSIZE 32
  char buf[DATEBUFFERSIZE];
  if (*value) {
     struct tm tm_r;
     localtime_r(value, &tm_r);
     strftime(buf, DATEBUFFERSIZE, "%Y-%m-%d ", &tm_r);
     strcat(buf, WeekDayName(tm_r.tm_wday));
     }
  else
     *buf = 0;
  SetValue(buf);
}

eOSState cMenuEditDateItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     if (NORMALKEY(Key) == kLeft) { // TODO might want to increase the delta if repeated quickly?
        *value -= SECSINDAY;
        if (*value < time(NULL))
           *value = 0;
        }
     else if (NORMALKEY(Key) == kRight) {
        if (!*value)
           *value = cTimer::SetTime(time(NULL), 0);
        *value += SECSINDAY;
        }
     else
        return state;
     Set();
     state = osContinue;
     }
  return state;
}

// --- cMenuEditTimeItem -----------------------------------------------------

class cMenuEditTimeItem : public cMenuEditItem {
protected:
  int *value;
  int hh, mm;
  int pos;
  virtual void Set(void);
public:
  cMenuEditTimeItem(const char *Name, int *Value);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuEditTimeItem::cMenuEditTimeItem(const char *Name, int *Value)
:cMenuEditItem(Name)
{
  value = Value;
  hh = *value / 100;
  mm = *value % 100;
  pos = 0;
  Set();
}

void cMenuEditTimeItem::Set(void)
{
  char buf[10];
  switch (pos) {
    case 1:  snprintf(buf, sizeof(buf), "%01d-:--", hh / 10); break;
    case 2:  snprintf(buf, sizeof(buf), "%02d:--", hh); break;
    case 3:  snprintf(buf, sizeof(buf), "%02d:%01d-", hh, mm / 10); break;
    default: snprintf(buf, sizeof(buf), "%02d:%02d", hh, mm);
    }
  SetValue(buf);
}

eOSState cMenuEditTimeItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     if (k0 <= Key && Key <= k9) {
        if (fresh || pos > 3) {
           pos = 0;
           fresh = false;
           }
        int n = Key - k0;
        switch (pos) {
          case 0: if (n <= 2) {
                     hh = n * 10;
                     mm = 0;
                     pos++;
                     }
                  break;
          case 1: if (hh + n <= 23) {
                     hh += n;
                     pos++;
                     }
                  break;
          case 2: if (n <= 5) {
                     mm += n * 10;
                     pos++;
                     }
                  break;
          case 3: if (mm + n <= 59) {
                     mm += n;
                     pos++;
                     }
                  break;
          }
        }
     else if (NORMALKEY(Key) == kLeft) { // TODO might want to increase the delta if repeated quickly?
        if (--mm < 0) {
           mm = 59;
           if (--hh < 0)
              hh = 23;
           }
        fresh = true;
        }
     else if (NORMALKEY(Key) == kRight) {
        if (++mm > 59) {
           mm = 0;
           if (++hh > 23)
              hh = 0;
           }
        fresh = true;
        }
     else
        return state;
     *value = hh * 100 + mm;
     Set();
     state = osContinue;
     }
  return state;
}

// --- cMenuEditCaItem -------------------------------------------------------

class cMenuEditCaItem : public cMenuEditIntItem {
private:
  const cCaDefinition *ca;
  bool allowCardNr;
protected:
  virtual void Set(void);
public:
  cMenuEditCaItem(const char *Name, int *Value, bool AllowCardNr = false);
  eOSState ProcessKey(eKeys Key);
  };

cMenuEditCaItem::cMenuEditCaItem(const char *Name, int *Value, bool AllowCardNr)
:cMenuEditIntItem(Name, Value, 0)
{
  ca = CaDefinitions.Get(*Value);
  allowCardNr = AllowCardNr;
  Set();
}

void cMenuEditCaItem::Set(void)
{
  if (ca)
     SetValue(ca->Description());
  else
     cMenuEditIntItem::Set();
}

eOSState cMenuEditCaItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     if (NORMALKEY(Key) == kLeft) { // TODO might want to increase the delta if repeated quickly?
        if (ca && ca->Prev()) {
           ca = (cCaDefinition *)ca->Prev();
           *value = ca->Number();
           }
        }
     else if (NORMALKEY(Key) == kRight) {
        if (ca && ca->Next() && (allowCardNr || ((cCaDefinition *)ca->Next())->Number() > MAXDEVICES)) {
           ca = (cCaDefinition *)ca->Next();
           *value = ca->Number();
           }
        }
     else
        return cMenuEditIntItem::ProcessKey(Key);
     Set();
     state = osContinue;
     }
  return state;
}

// --- cMenuEditChannel ------------------------------------------------------

class cMenuEditChannel : public cOsdMenu {
private:
  cChannel *channel;
  cChannel data;
public:
  cMenuEditChannel(int Index);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuEditChannel::cMenuEditChannel(int Index)
:cOsdMenu(tr("Edit channel"), 14)
{
  channel = Channels.Get(Index);
  if (channel) {
     data = *channel;
     Add(new cMenuEditStrItem( tr("Name"),          data.name, sizeof(data.name), tr(FileNameChars)));
     Add(new cMenuEditIntItem( tr("Frequency"),    &data.frequency));
     Add(new cMenuEditChrItem( tr("Polarization"), &data.polarization, "hv"));
     Add(new cMenuEditIntItem( tr("DiSEqC"),       &data.diseqc, 0, 10)); //TODO exact limits???
     Add(new cMenuEditIntItem( tr("Srate"),        &data.srate));
     Add(new cMenuEditIntItem( tr("Vpid"),         &data.vpid,  0, 0x1FFF));
     Add(new cMenuEditIntItem( tr("Apid1"),        &data.apid1, 0, 0x1FFF));
     Add(new cMenuEditIntItem( tr("Apid2"),        &data.apid2, 0, 0x1FFF));
     Add(new cMenuEditIntItem( tr("Dpid1"),        &data.dpid1, 0, 0x1FFF));
     Add(new cMenuEditIntItem( tr("Dpid2"),        &data.dpid2, 0, 0x1FFF));
     Add(new cMenuEditIntItem( tr("Tpid"),         &data.tpid,  0, 0x1FFF));
     Add(new cMenuEditCaItem(  tr("CA"),           &data.ca, true));
     Add(new cMenuEditIntItem( tr("Pnr"),          &data.pnr, 0));
     }
}

eOSState cMenuEditChannel::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     if (Key == kOk) {
        if (channel)
           *channel = data;
        Channels.Save();
        state = osBack;
        }
     }
  return state;
}

// --- cMenuChannelItem ------------------------------------------------------

class cMenuChannelItem : public cOsdItem {
private:
  int index;
  cChannel *channel;
public:
  cMenuChannelItem(int Index, cChannel *Channel);
  virtual void Set(void);
  void SetIndex(int Index);
  };

cMenuChannelItem::cMenuChannelItem(int Index, cChannel *Channel)
{
  index = Index;
  channel = Channel;
  if (channel->groupSep)
     SetColor(clrCyan, clrBackground);
  Set();
}

void cMenuChannelItem::Set(void)
{
  char *buffer = NULL;
  if (!channel->groupSep)
     asprintf(&buffer, "%d\t%s", channel->number, channel->name );
  else
     asprintf(&buffer, "---\t%s ----------------------------------------------------------------", channel->name);
  SetText(buffer, false);
}

void cMenuChannelItem::SetIndex(int Index)
{
  index = Index;
  Set();
}

// --- cMenuChannels ---------------------------------------------------------

class cMenuChannels : public cOsdMenu {
protected:
  eOSState Switch(void);
  eOSState Edit(void);
  eOSState New(void);
  eOSState Del(void);
  virtual void Move(int From, int To);
public:
  cMenuChannels(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuChannels::cMenuChannels(void)
:cOsdMenu(tr("Channels"), CHNUMWIDTH)
{
  //TODO
  int i = 0;
  cChannel *channel;
  int curr = ((channel = Channels.GetByNumber(cDevice::CurrentChannel())) != NULL) ? channel->Index() : -1;

  while ((channel = Channels.Get(i)) != NULL) {
        Add(new cMenuChannelItem(i, channel), i == curr);
        i++;
        }
  SetHelp(tr("Edit"), tr("New"), tr("Delete"), tr("Mark"));
}

eOSState cMenuChannels::Switch(void)
{
  cChannel *ch = Channels.Get(Current());
  if (ch)
     cDevice::PrimaryDevice()->SwitchChannel(ch, true);
  return osEnd;
}

eOSState cMenuChannels::Edit(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  isyslog("editing channel %d", Current() + 1);
  return AddSubMenu(new cMenuEditChannel(Current()));
}

eOSState cMenuChannels::New(void)
{
  if (HasSubMenu())
     return osContinue;
  cChannel *channel = new cChannel(Channels.Get(Current()));
  Channels.Add(channel);
  Channels.ReNumber();
  Add(new cMenuChannelItem(channel->Index()/*XXX*/, channel), true);
  Channels.Save();
  isyslog("channel %d added", channel->number);
  return AddSubMenu(new cMenuEditChannel(Current()));
}

eOSState cMenuChannels::Del(void)
{
  if (Count() > 0) {
     int Index = Current();
     cChannel *channel = Channels.Get(Index);
     int DeletedChannel = channel->number;
     // Check if there is a timer using this channel:
     for (cTimer *ti = Timers.First(); ti; ti = (cTimer *)ti->Next()) {
         if (ti->channel == DeletedChannel) {
            Interface->Error(tr("Channel is being used by a timer!"));
            return osContinue;
            }
         }
     if (Interface->Confirm(tr("Delete channel?"))) {
        // Move and renumber the channels:
        Channels.Del(channel);
        Channels.ReNumber();
        cOsdMenu::Del(Index);
        int i = 0;
        for (cMenuChannelItem *ci = (cMenuChannelItem *)First(); ci; ci = (cMenuChannelItem *)ci->Next())
            ci->SetIndex(i++);
        Channels.Save();
        isyslog("channel %d deleted", DeletedChannel);
        // Fix the timers:
        bool TimersModified = false;
        for (cTimer *ti = Timers.First(); ti; ti = (cTimer *)ti->Next()) {
            int OldChannel = ti->channel;
            if (ti->channel > DeletedChannel)
               ti->channel--;
            if (ti->channel != OldChannel) {
               TimersModified = true;
               isyslog("timer %d: channel changed from %d to %d", ti->Index() + 1, OldChannel, ti->channel);
               }
            }
        if (TimersModified)
           Timers.Save();
        Display();
        }
     }
  return osContinue;
}

void cMenuChannels::Move(int From, int To)
{
  int FromNumber = Channels.Get(From)->number;
  int ToNumber = Channels.Get(To)->number;
  // Move and renumber the channels:
  Channels.Move(From, To);
  Channels.ReNumber();
  cOsdMenu::Move(From, To);
  int i = 0;
  for (cMenuChannelItem *ci = (cMenuChannelItem *)First(); ci; ci = (cMenuChannelItem *)ci->Next())
      ci->SetIndex(i++);
  Channels.Save();
  isyslog("channel %d moved to %d", FromNumber, ToNumber);
  // Fix the timers:
  bool TimersModified = false;
  From++; // user visible channel numbers start with '1'
  To++;
  for (cTimer *ti = Timers.First(); ti; ti = (cTimer *)ti->Next()) {
      int OldChannel = ti->channel;
      if (ti->channel == FromNumber)
         ti->channel = ToNumber;
      else if (ti->channel > FromNumber && ti->channel <= ToNumber)
         ti->channel--;
      else if (ti->channel < FromNumber && ti->channel >= ToNumber)
         ti->channel++;
      if (ti->channel != OldChannel) {
         TimersModified = true;
         isyslog("timer %d: channel changed from %d to %d", ti->Index() + 1, OldChannel, ti->channel);
         }
      }
  if (TimersModified)
     Timers.Save();
  Display();
}

eOSState cMenuChannels::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:     return Switch();
       case kRed:    return Edit();
       case kGreen:  return New();
       case kYellow: return Del();
       case kBlue:   Mark(); break;
       default: break;
       }
     }
  return state;
}

// --- cMenuText -------------------------------------------------------------

class cMenuText : public cOsdMenu {
public:
  cMenuText(const char *Title, const char *Text, eDvbFont Font = fontOsd);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuText::cMenuText(const char *Title, const char *Text, eDvbFont Font)
:cOsdMenu(Title)
{
  Add(new cMenuTextItem(Text, 1, 2, Setup.OSDwidth - 2, MAXOSDITEMS, clrWhite, clrBackground, Font));
}

eOSState cMenuText::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk: return osBack;
       default:  state = osContinue;
       }
     }
  return state;
}

// --- cMenuEditTimer --------------------------------------------------------

class cMenuEditTimer : public cOsdMenu {
private:
  cTimer *timer;
  cTimer data;
  cMenuEditDateItem *firstday;
  void SetFirstDayItem(void);
public:
  cMenuEditTimer(int Index, bool New = false);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuEditTimer::cMenuEditTimer(int Index, bool New)
:cOsdMenu(tr("Edit timer"), 12)
{
  firstday = NULL;
  timer = Timers.Get(Index);
  if (timer) {
     data = *timer;
     if (New)
        data.active = 1;
     Add(new cMenuEditBoolItem(tr("Active"),       &data.active));
     Add(new cMenuEditChanItem(tr("Channel"),      &data.channel));
     Add(new cMenuEditDayItem( tr("Day"),          &data.day));
     Add(new cMenuEditTimeItem(tr("Start"),        &data.start));
     Add(new cMenuEditTimeItem(tr("Stop"),         &data.stop));
//TODO VPS???
     Add(new cMenuEditIntItem( tr("Priority"),     &data.priority, 0, MAXPRIORITY));
     Add(new cMenuEditIntItem( tr("Lifetime"),     &data.lifetime, 0, MAXLIFETIME));
     Add(new cMenuEditStrItem( tr("File"),          data.file, sizeof(data.file), tr(FileNameChars)));
     SetFirstDayItem();
     }
}

void cMenuEditTimer::SetFirstDayItem(void)
{
  if (!firstday && !data.IsSingleEvent()) {
     Add(firstday = new cMenuEditDateItem(tr("First day"), &data.firstday));
     Display();
     }
  else if (firstday && data.IsSingleEvent()) {
     Del(firstday->Index());
     firstday = NULL;
     data.firstday = 0;
     Display();
     }
}

eOSState cMenuEditTimer::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:     if (!*data.file)
                        strcpy(data.file, Channels.GetChannelNameByNumber(data.channel));
                     if (timer && memcmp(timer, &data, sizeof(data)) != 0) {
                        *timer = data;
                        if (timer->active)
                           timer->active = 1; // allows external programs to mark active timers with values > 1 and recognize if the user has modified them
                        Timers.Save();
                        isyslog("timer %d modified (%s)", timer->Index() + 1, timer->active ? "active" : "inactive");
                        }
                     return osBack;
       case kRed:
       case kGreen:
       case kYellow:
       case kBlue:   return osContinue;
       default: break;
       }
     }
  if (Key != kNone)
     SetFirstDayItem();
  return state;
}

// --- cMenuTimerItem --------------------------------------------------------

class cMenuTimerItem : public cOsdItem {
private:
  cTimer *timer;
public:
  cMenuTimerItem(cTimer *Timer);
  virtual bool operator< (const cListObject &ListObject);
  virtual void Set(void);
  cTimer *Timer(void) { return timer; }
  };

cMenuTimerItem::cMenuTimerItem(cTimer *Timer)
{
  timer = Timer;
  Set();
}

bool cMenuTimerItem::operator< (const cListObject &ListObject)
{
  return *timer < *((cMenuTimerItem *)&ListObject)->timer;
}

void cMenuTimerItem::Set(void)
{
  char *buffer = NULL;
  asprintf(&buffer, "%c\t%d\t%s\t%02d:%02d\t%02d:%02d\t%s",
                    !timer->active ? ' ' : timer->firstday ? '!' : timer->recording ? '#' : '>',
                    timer->channel,
                    timer->PrintDay(timer->day),
                    timer->start / 100,
                    timer->start % 100,
                    timer->stop / 100,
                    timer->stop % 100,
                    timer->file);
  SetText(buffer, false);
}

// --- cMenuTimers -----------------------------------------------------------

class cMenuTimers : public cOsdMenu {
private:
  eOSState Edit(void);
  eOSState New(void);
  eOSState Del(void);
  eOSState OnOff(void);
  virtual void Move(int From, int To);
  eOSState Summary(void);
  cTimer *CurrentTimer(void);
public:
  cMenuTimers(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuTimers::cMenuTimers(void)
:cOsdMenu(tr("Timers"), 2, CHNUMWIDTH, 10, 6, 6)
{
  int i = 0;
  cTimer *timer;

  while ((timer = Timers.Get(i)) != NULL) {
        Add(new cMenuTimerItem(timer));
        i++;
        }
  if (Setup.SortTimers)
     Sort();
  SetHelp(tr("Edit"), tr("New"), tr("Delete"), Setup.SortTimers ? tr("On/Off") : tr("Mark"));
}

cTimer *cMenuTimers::CurrentTimer(void)
{
  cMenuTimerItem *item = (cMenuTimerItem *)Get(Current());
  return item ? item->Timer() : NULL;
}

eOSState cMenuTimers::OnOff(void)
{
  cTimer *timer = CurrentTimer();
  if (timer) {
     if (timer->IsSingleEvent())
        timer->active = !timer->active;
     else if (timer->firstday) {
        timer->firstday = 0;
        timer->active = false;
        }
     else if (timer->active)
        timer->Skip();
     else
        timer->active = true;
     timer->Matches(); // refresh start and end time
     RefreshCurrent();
     DisplayCurrent(true);
     if (timer->firstday)
        isyslog("timer %d first day set to %s", timer->Index() + 1, timer->PrintFirstDay());
     else
        isyslog("timer %d %sactivated", timer->Index() + 1, timer->active ? "" : "de");
     Timers.Save();
     }
  return osContinue;
}

eOSState cMenuTimers::Edit(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  isyslog("editing timer %d", CurrentTimer()->Index() + 1);
  return AddSubMenu(new cMenuEditTimer(CurrentTimer()->Index()));
}

eOSState cMenuTimers::New(void)
{
  if (HasSubMenu())
     return osContinue;
  cTimer *timer = new cTimer;
  Timers.Add(timer);
  Add(new cMenuTimerItem(timer), true);
  Timers.Save();
  isyslog("timer %d added", timer->Index() + 1);
  return AddSubMenu(new cMenuEditTimer(timer->Index(), true));
}

eOSState cMenuTimers::Del(void)
{
  // Check if this timer is active:
  cTimer *ti = CurrentTimer();
  if (ti) {
     if (!ti->recording) {
        if (Interface->Confirm(tr("Delete timer?"))) {
           int Index = ti->Index();
           Timers.Del(ti);
           cOsdMenu::Del(Current());
           Timers.Save();
           Display();
           isyslog("timer %d deleted", Index + 1);
           }
        }
     else
        Interface->Error(tr("Timer is recording!"));
     }
  return osContinue;
}

void cMenuTimers::Move(int From, int To)
{
  Timers.Move(From, To);
  cOsdMenu::Move(From, To);
  Timers.Save();
  Display();
  isyslog("timer %d moved to %d", From + 1, To + 1);
}

eOSState cMenuTimers::Summary(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cTimer *ti = CurrentTimer();
  if (ti && ti->summary && *ti->summary)
     return AddSubMenu(new cMenuText(tr("Summary"), ti->summary));
  return Edit(); // convenience for people not using the Summary feature ;-)
}

eOSState cMenuTimers::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:     return Summary();
       case kRed:    return Edit();
       case kGreen:  return New();
       case kYellow: return Del();
       case kBlue:   if (Setup.SortTimers)
                        OnOff();
                     else
                        Mark();
                     break;
       default: break;
       }
     }
  return state;
}

// --- cMenuEvent ------------------------------------------------------------

class cMenuEvent : public cOsdMenu {
private:
  const cEventInfo *eventInfo;
public:
  cMenuEvent(const cEventInfo *EventInfo, bool CanSwitch = false);
  cMenuEvent(bool Now);
  virtual eOSState ProcessKey(eKeys Key);
};

cMenuEvent::cMenuEvent(const cEventInfo *EventInfo, bool CanSwitch)
:cOsdMenu(tr("Event"))
{
  eventInfo = EventInfo;
  if (eventInfo) {
     cChannel *channel = Channels.GetByServiceID(eventInfo->GetServiceID());
     if (channel) {
        char *buffer;
        asprintf(&buffer, "%-17.*s\t%.*s  %s - %s", 17, channel->name, 5, eventInfo->GetDate(), eventInfo->GetTimeString(), eventInfo->GetEndTimeString());
        SetTitle(buffer, false);
        free(buffer);
        int Line = 2;
        cMenuTextItem *item;
        const char *Title = eventInfo->GetTitle();
        const char *Subtitle = eventInfo->GetSubtitle();
        const char *ExtendedDescription = eventInfo->GetExtendedDescription();
        if (!isempty(Title)) {
           Add(item = new cMenuTextItem(Title, 1, Line, Setup.OSDwidth - 2, -1, clrCyan));
           Line += item->Height() + 1;
           }
        if (!isempty(Subtitle)) {
           Add(item = new cMenuTextItem(Subtitle, 1, Line, Setup.OSDwidth - 2, -1, clrYellow));
           Line += item->Height() + 1;
           }
        if (!isempty(ExtendedDescription))
           Add(new cMenuTextItem(ExtendedDescription, 1, Line, Setup.OSDwidth - 2, Height() - Line - 2, clrCyan), true);
        SetHelp(tr("Record"), NULL, NULL, CanSwitch ? tr("Switch") : NULL);
        }
     }
}

eOSState cMenuEvent::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kGreen:
       case kYellow: return osContinue;
       case kOk:     return osBack;
       default: break;
       }
     }
  return state;
}

// --- cMenuWhatsOnItem ------------------------------------------------------

class cMenuWhatsOnItem : public cOsdItem {
public:
  const cEventInfo *eventInfo;
  cMenuWhatsOnItem(const cEventInfo *EventInfo);
};

cMenuWhatsOnItem::cMenuWhatsOnItem(const cEventInfo *EventInfo)
{
  eventInfo = EventInfo;
  char *buffer = NULL;
  cChannel *channel = Channels.GetByNumber(eventInfo->GetChannelNumber());
  asprintf(&buffer, "%d\t%.*s\t%.*s\t%s", eventInfo->GetChannelNumber(), 6, channel ? channel->name : "???", 5, eventInfo->GetTimeString(), eventInfo->GetTitle());
  SetText(buffer, false);
}

// --- cMenuWhatsOn ----------------------------------------------------------

class cMenuWhatsOn : public cOsdMenu {
private:
  eOSState Record(void);
  eOSState Switch(void);
  static int currentChannel;
  static const cEventInfo *scheduleEventInfo;
public:
  cMenuWhatsOn(const cSchedules *Schedules, bool Now, int CurrentChannelNr);
  static int CurrentChannel(void) { return currentChannel; }
  static void SetCurrentChannel(int ChannelNr) { currentChannel = ChannelNr; }
  static const cEventInfo *ScheduleEventInfo(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

int cMenuWhatsOn::currentChannel = 0;
const cEventInfo *cMenuWhatsOn::scheduleEventInfo = NULL;

static int CompareEventChannel(const void *p1, const void *p2)
{
  return (int)( (*(const cEventInfo **)p1)->GetChannelNumber() - (*(const cEventInfo **)p2)->GetChannelNumber());
}

cMenuWhatsOn::cMenuWhatsOn(const cSchedules *Schedules, bool Now, int CurrentChannelNr)
:cOsdMenu(Now ? tr("What's on now?") : tr("What's on next?"), CHNUMWIDTH, 7, 6)
{
  const cSchedule *Schedule = Schedules->First();
  const cEventInfo **pArray = NULL;
  int num = 0;

  while (Schedule) {
        pArray = (const cEventInfo **)realloc(pArray, (num + 1) * sizeof(cEventInfo *));

        pArray[num] = Now ? Schedule->GetPresentEvent() : Schedule->GetFollowingEvent();
        if (pArray[num]) {
           cChannel *channel = Channels.GetByServiceID(pArray[num]->GetServiceID());
           if (channel) {
              pArray[num]->SetChannelNumber(channel->number);
              num++;
              }
           }
        Schedule = (const cSchedule *)Schedules->Next(Schedule);
        }

  qsort(pArray, num, sizeof(cEventInfo *), CompareEventChannel);

  for (int a = 0; a < num; a++)
      Add(new cMenuWhatsOnItem(pArray[a]), pArray[a]->GetChannelNumber() == CurrentChannelNr);

  currentChannel = CurrentChannelNr;
  free(pArray);
  SetHelp(tr("Record"), Now ? tr("Next") : tr("Now"), tr("Button$Schedule"), tr("Switch"));
}

const cEventInfo *cMenuWhatsOn::ScheduleEventInfo(void)
{
  const cEventInfo *ei = scheduleEventInfo;
  scheduleEventInfo = NULL;
  return ei;
}

eOSState cMenuWhatsOn::Switch(void)
{
  cMenuWhatsOnItem *item = (cMenuWhatsOnItem *)Get(Current());
  if (item) {
     cChannel *channel = Channels.GetByServiceID(item->eventInfo->GetServiceID());
     if (channel && cDevice::PrimaryDevice()->SwitchChannel(channel, true))
        return osEnd;
     }
  Interface->Error(tr("Can't switch channel!"));
  return osContinue;
}

eOSState cMenuWhatsOn::Record(void)
{
  cMenuWhatsOnItem *item = (cMenuWhatsOnItem *)Get(Current());
  if (item) {
     cTimer *timer = new cTimer(item->eventInfo);
     cTimer *t = Timers.GetTimer(timer);
     if (!t) {
        Timers.Add(timer);
        Timers.Save();
        isyslog("timer %d added", timer->Index() + 1);
        }
     else {
        delete timer;
        timer = t;
        }
     return AddSubMenu(new cMenuEditTimer(timer->Index(), true));
     }
  return osContinue;
}

eOSState cMenuWhatsOn::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRed:    return Record();
       case kYellow: state = osBack;
                     // continue with kGreen
       case kGreen:  {
                       cMenuWhatsOnItem *mi = (cMenuWhatsOnItem *)Get(Current());
                       if (mi) {
                          scheduleEventInfo = mi->eventInfo;
                          currentChannel = mi->eventInfo->GetChannelNumber();
                          }
                     }
                     break;
       case kBlue:   return Switch();
       case kOk:     if (Count())
                        return AddSubMenu(new cMenuEvent(((cMenuWhatsOnItem *)Get(Current()))->eventInfo, true));
                     break;
       default:      break;
       }
     }
  return state;
}

// --- cMenuScheduleItem -----------------------------------------------------

class cMenuScheduleItem : public cOsdItem {
public:
  const cEventInfo *eventInfo;
  cMenuScheduleItem(const cEventInfo *EventInfo);
};

cMenuScheduleItem::cMenuScheduleItem(const cEventInfo *EventInfo)
{
  eventInfo = EventInfo;
  char *buffer = NULL;
  asprintf(&buffer, "%.*s\t%.*s\t%s", 5, eventInfo->GetDate(), 5, eventInfo->GetTimeString(), eventInfo->GetTitle());
  SetText(buffer, false);
}

// --- cMenuSchedule ---------------------------------------------------------

class cMenuSchedule : public cOsdMenu {
private:
  cMutexLock mutexLock;
  const cSchedules *schedules;
  bool now, next;
  int otherChannel;
  eOSState Record(void);
  eOSState Switch(void);
  void PrepareSchedule(cChannel *Channel);
public:
  cMenuSchedule(void);
  virtual ~cMenuSchedule();
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuSchedule::cMenuSchedule(void)
:cOsdMenu("", 6, 6)
{
  now = next = false;
  otherChannel = 0;
  cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
  if (channel) {
     cMenuWhatsOn::SetCurrentChannel(channel->number);
     schedules = cSIProcessor::Schedules(mutexLock);
     PrepareSchedule(channel);
     SetHelp(tr("Record"), tr("Now"), tr("Next"));
     }
}

cMenuSchedule::~cMenuSchedule()
{
  cMenuWhatsOn::ScheduleEventInfo(); // makes sure any posted data is cleared
}

static int CompareEventTime(const void *p1, const void *p2)
{
  return (int)((*(cEventInfo **)p1)->GetTime() - (*(cEventInfo **)p2)->GetTime());
}

void cMenuSchedule::PrepareSchedule(cChannel *Channel)
{
  Clear();
  char *buffer = NULL;
  asprintf(&buffer, tr("Schedule - %s"), Channel->name);
  SetTitle(buffer);
  free(buffer);
  if (schedules) {
     const cSchedule *Schedule = Channel->pnr ? schedules->GetSchedule(Channel->pnr) : schedules->GetSchedule();
     int num = Schedule->NumEvents();
     const cEventInfo **pArray = MALLOC(const cEventInfo *, num);
     if (pArray) {
        time_t now = time(NULL);
        int numreal = 0;
        for (int a = 0; a < num; a++) {
            const cEventInfo *EventInfo = Schedule->GetEventNumber(a);
            if (EventInfo->GetTime() + EventInfo->GetDuration() > now)
               pArray[numreal++] = EventInfo;
            }

        qsort(pArray, numreal, sizeof(cEventInfo *), CompareEventTime);

        for (int a = 0; a < numreal; a++)
            Add(new cMenuScheduleItem(pArray[a]));
        free(pArray);
        }
     }
}

eOSState cMenuSchedule::Record(void)
{
  cMenuScheduleItem *item = (cMenuScheduleItem *)Get(Current());
  if (item) {
     cTimer *timer = new cTimer(item->eventInfo);
     cTimer *t = Timers.GetTimer(timer);
     if (!t) {
        Timers.Add(timer);
        Timers.Save();
        isyslog("timer %d added", timer->Index() + 1);
        }
     else {
        delete timer;
        timer = t;
        }
     return AddSubMenu(new cMenuEditTimer(timer->Index(), true));
     }
  return osContinue;
}

eOSState cMenuSchedule::Switch(void)
{
  if (otherChannel) {
     if (Channels.SwitchTo(otherChannel))
        return osEnd;
     }
  Interface->Error(tr("Can't switch channel!"));
  return osContinue;
}

eOSState cMenuSchedule::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRed:    return Record();
       case kGreen:  if (schedules) {
                        if (!now && !next) {
                           int ChannelNr = 0;
                           if (Count()) {
                              cChannel *channel = Channels.GetByServiceID(((cMenuScheduleItem *)Get(Current()))->eventInfo->GetServiceID());
                              if (channel)
                                 ChannelNr = channel->number;
                              }
                           now = true;
                           return AddSubMenu(new cMenuWhatsOn(schedules, true, ChannelNr));
                           }
                        now = !now;
                        next = !next;
                        return AddSubMenu(new cMenuWhatsOn(schedules, now, cMenuWhatsOn::CurrentChannel()));
                        }
       case kYellow: if (schedules)
                        return AddSubMenu(new cMenuWhatsOn(schedules, false, cMenuWhatsOn::CurrentChannel()));
                     break;
       case kBlue:   if (Count())
                        return Switch();
                     break;
       case kOk:     if (Count())
                        return AddSubMenu(new cMenuEvent(((cMenuScheduleItem *)Get(Current()))->eventInfo, otherChannel));
                     break;
       default:      break;
       }
     }
  else if (!HasSubMenu()) {
     now = next = false;
     const cEventInfo *ei = cMenuWhatsOn::ScheduleEventInfo();
     if (ei) {
        cChannel *channel = Channels.GetByServiceID(ei->GetServiceID());
        if (channel) {
           PrepareSchedule(channel);
           if (channel->number != cDevice::CurrentChannel()) {
              otherChannel = channel->number;
              SetHelp(tr("Record"), tr("Now"), tr("Next"), tr("Switch"));
              }
           Display();
           }
        }
     }
  return state;
}

// --- cMenuRecordingItem ----------------------------------------------------

class cMenuRecordingItem : public cOsdItem {
private:
  char *fileName;
  char *name;
  int totalEntries, newEntries;
public:
  cMenuRecordingItem(cRecording *Recording, int Level);
  ~cMenuRecordingItem();
  void IncrementCounter(bool New);
  const char *Name(void) { return name; }
  const char *FileName(void) { return fileName; }
  bool IsDirectory(void) { return name != NULL; }
  };

cMenuRecordingItem::cMenuRecordingItem(cRecording *Recording, int Level)
{
  fileName = strdup(Recording->FileName());
  name = NULL;
  totalEntries = newEntries = 0;
  SetText(Recording->Title('\t', true, Level));
  if (*Text() == '\t')
     name = strdup(Text() + 2); // 'Text() + 2' to skip the two '\t'
}

cMenuRecordingItem::~cMenuRecordingItem()
{
  free(fileName);
  free(name);
}

void cMenuRecordingItem::IncrementCounter(bool New)
{
  totalEntries++;
  if (New)
     newEntries++;
  char *buffer = NULL;
  asprintf(&buffer, "%d\t%d\t%s", totalEntries, newEntries, name);
  SetText(buffer, false);
}

// --- cMenuRecordings -------------------------------------------------------

cRecordings cMenuRecordings::Recordings;
int cMenuRecordings::helpKeys = -1;

cMenuRecordings::cMenuRecordings(const char *Base, int Level, bool OpenSubMenus)
:cOsdMenu(Base ? Base : tr("Recordings"), 6, 6)
{
  base = Base ? strdup(Base) : NULL;
  level = Setup.RecordingDirs ? Level : -1;
  if (!Base) {
     Interface->Status(tr("scanning recordings..."));
     Interface->Flush();
     }
  if (Base || Recordings.Load()) {
     const char *LastReplayed = cReplayControl::LastReplayed();
     cMenuRecordingItem *LastItem = NULL;
     char *LastItemText = NULL;
     for (cRecording *recording = Recordings.First(); recording; recording = Recordings.Next(recording)) {
         if (!Base || (strstr(recording->Name(), Base) == recording->Name() && recording->Name()[strlen(Base)] == '~')) {
            cMenuRecordingItem *Item = new cMenuRecordingItem(recording, level);
            if (*Item->Text() && (!LastItem || strcmp(Item->Text(), LastItemText) != 0)) {
               Add(Item);
               LastItem = Item;
               free(LastItemText);
               LastItemText = strdup(LastItem->Text()); // must use a copy because of the counters!
               }
            else
               delete Item;
            if (LastItem) {
               if (LastReplayed && strcmp(LastReplayed, recording->FileName()) == 0)
                  SetCurrent(LastItem);
               if (LastItem->IsDirectory())
                  LastItem->IncrementCounter(recording->IsNew());
               }
            }
         }
     free(LastItemText);
     if (Current() < 0)
        SetCurrent(First());
     else if (OpenSubMenus && Open(true))
        return;
     }
  Display(); // this keeps the higher level menus from showing up briefly when pressing 'Back' during replay
  SetHelpKeys();
}

cMenuRecordings::~cMenuRecordings()
{
  helpKeys = -1;
  free(base);
}

void cMenuRecordings::SetHelpKeys(void)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  int NewHelpKeys = helpKeys;
  if (ri) {
     if (ri->IsDirectory())
        NewHelpKeys = 1;
     else {
        NewHelpKeys = 2;
        cRecording *recording = GetRecording(ri);
        if (recording && recording->Summary())
           NewHelpKeys = 3;
        }
     }
  if (NewHelpKeys != helpKeys) {
     switch (NewHelpKeys) {
       case 0: SetHelp(NULL); break;
       case 1: SetHelp(tr("Open")); break;
       case 2:
       case 3: SetHelp(tr("Play"), tr("Rewind"), tr("Delete"), NewHelpKeys == 3 ? tr("Summary") : NULL);
       }
     helpKeys = NewHelpKeys;
     }
}

cRecording *cMenuRecordings::GetRecording(cMenuRecordingItem *Item)
{
  cRecording *recording = Recordings.GetByName(Item->FileName());
  if (!recording)
     Interface->Error(tr("Error while accessing recording!"));
  return recording;
}

bool cMenuRecordings::Open(bool OpenSubMenus)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && ri->IsDirectory()) {
     const char *t = ri->Name();
     char *buffer = NULL;
     if (base) {
        asprintf(&buffer, "%s~%s", base, t);
        t = buffer;
        }
     AddSubMenu(new cMenuRecordings(t, level + 1, OpenSubMenus));
     free(buffer);
     return true;
     }
  return false;
}

eOSState cMenuRecordings::Play(void)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri) {
     if (ri->IsDirectory())
        Open();
     else {
        cRecording *recording = GetRecording(ri);
        if (recording) {
           cReplayControl::SetRecording(recording->FileName(), recording->Title());
           return osReplay;
           }
        }
     }
  return osContinue;
}

eOSState cMenuRecordings::Rewind(void)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     cDevice::PrimaryDevice()->StopReplay(); // must do this first to be able to rewind the currently replayed recording
     cResumeFile ResumeFile(ri->FileName());
     ResumeFile.Delete();
     return Play();
     }
  return osContinue;
}

eOSState cMenuRecordings::Del(void)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     if (Interface->Confirm(tr("Delete recording?"))) {
        cRecordControl *rc = cRecordControls::GetRecordControl(ri->FileName());
        if (rc) {
           if (Interface->Confirm(tr("Timer still recording - really delete?"))) {
              cTimer *timer = rc->Timer();
              if (timer) {
                 timer->Skip();
                 cRecordControls::Process(time(NULL));
                 Timers.Save();
                 }
              }
           else
              return osContinue;
           }
        cRecording *recording = GetRecording(ri);
        if (recording) {
           if (recording->Delete()) {
              cReplayControl::ClearLastReplayed(ri->FileName());
              cOsdMenu::Del(Current());
              Recordings.Del(recording);
              Display();
              if (!Count())
                 return osBack;
              }
           else
              Interface->Error(tr("Error while deleting recording!"));
           }
        }
     }
  return osContinue;
}

eOSState cMenuRecordings::Summary(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     cRecording *recording = GetRecording(ri);
     if (recording && recording->Summary() && *recording->Summary())
        return AddSubMenu(new cMenuText(tr("Summary"), recording->Summary()));
     }
  return osContinue;
}

eOSState cMenuRecordings::ProcessKey(eKeys Key)
{
  bool HadSubMenu = HasSubMenu();
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:
       case kRed:    return Play();
       case kGreen:  return Rewind();
       case kYellow: return Del();
       case kBlue:   return Summary();
       default: break;
       }
     }
  if (Key == kYellow && HadSubMenu && !HasSubMenu()) {
     // the last recording in a subdirectory was deleted, so let's go back up
     cOsdMenu::Del(Current());
     if (!Count())
        return osBack;
     Display();
     }
  if (!HasSubMenu() && Key != kNone)
     SetHelpKeys();
  return state;
}

// --- cMenuSetupBase --------------------------------------------------------

class cMenuSetupBase : public cMenuSetupPage {
protected:
  cSetup data;
  virtual void Store(void);
public:
  cMenuSetupBase(void);
  };

cMenuSetupBase::cMenuSetupBase(void)
{
  data = Setup;
}

void cMenuSetupBase::Store(void)
{
  Setup = data;
  Setup.Save();
}

// --- cMenuSetupOSD ---------------------------------------------------------

class cMenuSetupOSD : public cMenuSetupBase {
private:
  virtual void Set(void);
public:
  cMenuSetupOSD(void) { Set(); }
  virtual eOSState ProcessKey(eKeys Key);
  };

void cMenuSetupOSD::Set(void)
{
  Clear();
  SetSection(tr("OSD"));
  Add(new cMenuEditStraItem(tr("Setup.OSD$Language"),               &data.OSDLanguage, I18nNumLanguages, I18nLanguages()));
  Add(new cMenuEditIntItem( tr("Setup.OSD$Width"),                  &data.OSDwidth, MINOSDWIDTH, MAXOSDWIDTH));
  Add(new cMenuEditIntItem( tr("Setup.OSD$Height"),                 &data.OSDheight, MINOSDHEIGHT, MAXOSDHEIGHT));
  Add(new cMenuEditIntItem( tr("Setup.OSD$Message time (s)"),       &data.OSDMessageTime, 1, 60));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Channel info position"),  &data.ChannelInfoPos, tr("bottom"), tr("top")));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Info on channel switch"), &data.ShowInfoOnChSwitch));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Scroll pages"),           &data.MenuScrollPage));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Sort timers"),            &data.SortTimers));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Recording directories"),  &data.RecordingDirs));
}

eOSState cMenuSetupOSD::ProcessKey(eKeys Key)
{
  int osdLanguage = data.OSDLanguage;
  eOSState state = cMenuSetupBase::ProcessKey(Key);

  if (data.OSDLanguage != osdLanguage) {
     int OriginalOSDLanguage = Setup.OSDLanguage;
     Setup.OSDLanguage = data.OSDLanguage;
     Set();
     Display();
     Setup.OSDLanguage = OriginalOSDLanguage;
     }
  return state;
}

// --- cMenuSetupEPG ---------------------------------------------------------

class cMenuSetupEPG : public cMenuSetupBase {
public:
  cMenuSetupEPG(void);
  };

cMenuSetupEPG::cMenuSetupEPG(void)
{
  SetSection(tr("EPG"));
  Add(new cMenuEditIntItem( tr("Setup.EPG$EPG scan timeout (h)"),      &data.EPGScanTimeout));
  Add(new cMenuEditIntItem( tr("Setup.EPG$EPG bugfix level"),          &data.EPGBugfixLevel, 0, MAXEPGBUGFIXLEVEL));
  Add(new cMenuEditBoolItem(tr("Setup.EPG$Set system time"),           &data.SetSystemTime));
  Add(new cMenuEditTranItem(tr("Setup.EPG$Use time from transponder"), &data.TimeTransponder));
}

// --- cMenuSetupDVB ---------------------------------------------------------

class cMenuSetupDVB : public cMenuSetupBase {
public:
  cMenuSetupDVB(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuSetupDVB::cMenuSetupDVB(void)
{
  SetSection(tr("DVB"));
  Add(new cMenuEditIntItem( tr("Setup.DVB$Primary DVB interface"), &data.PrimaryDVB, 1, cDevice::NumDevices()));
  Add(new cMenuEditBoolItem(tr("Setup.DVB$Video format"),          &data.VideoFormat, "4:3", "16:9"));
}

eOSState cMenuSetupDVB::ProcessKey(eKeys Key)
{
  int oldPrimaryDVB = Setup.PrimaryDVB;
  eOSState state = cMenuSetupBase::ProcessKey(Key);

  if (state == osBack && Key == kOk) {
     if (Setup.PrimaryDVB != oldPrimaryDVB) {
        state = osSwitchDvb;
        cDevice::PrimaryDevice()->SetVideoFormat(Setup.VideoFormat);
        }
     }
  return state;
}

// --- cMenuSetupLNB ---------------------------------------------------------

class cMenuSetupLNB : public cMenuSetupBase {
public:
  cMenuSetupLNB(void);
  };

cMenuSetupLNB::cMenuSetupLNB(void)
{
  SetSection(tr("LNB"));
  Add(new cMenuEditIntItem( tr("Setup.LNB$SLOF (MHz)"),               &data.LnbSLOF));
  Add(new cMenuEditIntItem( tr("Setup.LNB$Low LNB frequency (MHz)"),  &data.LnbFrequLo));
  Add(new cMenuEditIntItem( tr("Setup.LNB$High LNB frequency (MHz)"), &data.LnbFrequHi));
  Add(new cMenuEditBoolItem(tr("Setup.LNB$Use DiSEqC"),               &data.DiSEqC));
}

// --- cMenuSetupCICAM -------------------------------------------------------

class cMenuSetupCICAM : public cMenuSetupBase {
public:
  cMenuSetupCICAM(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuSetupCICAM::cMenuSetupCICAM(void)
{
  SetSection(tr("CICAM"));
  for (int d = 0; d < cDevice::NumDevices(); d++) {
      for (int i = 0; i < 2; i++) {
          char buffer[32];
          snprintf(buffer, sizeof(buffer), "%s%d %d", tr("Setup.CICAM$CICAM DVB"), d + 1, i + 1);
          Add(new cMenuEditCaItem(buffer, &data.CaCaps[d][i]));
          }
      }
}

eOSState cMenuSetupCICAM::ProcessKey(eKeys Key)
{
  eOSState state = cMenuSetupBase::ProcessKey(Key);

  if (state == osBack && Key == kOk)
     cDevice::SetCaCaps();
  return state;
}

// --- cMenuSetupRecord ------------------------------------------------------

class cMenuSetupRecord : public cMenuSetupBase {
public:
  cMenuSetupRecord(void);
  };

cMenuSetupRecord::cMenuSetupRecord(void)
{
  SetSection(tr("Recording"));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Margin at start (min)"),     &data.MarginStart));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Margin at stop (min)"),      &data.MarginStop));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Primary limit"),             &data.PrimaryLimit, 0, MAXPRIORITY));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Default priority"),          &data.DefaultPriority, 0, MAXPRIORITY));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Default lifetime (d)"),      &data.DefaultLifetime, 0, MAXLIFETIME));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Use episode name"),          &data.UseSubtitle));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Mark instant recording"),    &data.MarkInstantRecord));
  Add(new cMenuEditStrItem( tr("Setup.Recording$Name instant recording"),     data.NameInstantRecord, sizeof(data.NameInstantRecord), tr(FileNameChars)));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Instant rec. time (min)"),   &data.InstantRecordTime, 1, MAXINSTANTRECTIME));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Record Dolby Digital"),      &data.RecordDolbyDigital));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Max. video file size (MB)"), &data.MaxVideoFileSize, MINVIDEOFILESIZE, MAXVIDEOFILESIZE));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Split edited files"),        &data.SplitEditedFiles));
}

// --- cMenuSetupReplay ------------------------------------------------------

class cMenuSetupReplay : public cMenuSetupBase {
public:
  cMenuSetupReplay(void);
  };

cMenuSetupReplay::cMenuSetupReplay(void)
{
  SetSection(tr("Replay"));
  Add(new cMenuEditBoolItem(tr("Setup.Replay$Multi speed mode"), &data.MultiSpeedMode));
  Add(new cMenuEditBoolItem(tr("Setup.Replay$Show replay mode"), &data.ShowReplayMode));
}

// --- cMenuSetupMisc --------------------------------------------------------

class cMenuSetupMisc : public cMenuSetupBase {
public:
  cMenuSetupMisc(void);
  };

cMenuSetupMisc::cMenuSetupMisc(void)
{
  SetSection(tr("Miscellaneous"));
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Min. event timeout (min)"),   &data.MinEventTimeout));
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Min. user inactivity (min)"), &data.MinUserInactivity));
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$SVDRP timeout (s)"),          &data.SVDRPTimeout));
}

// --- cMenuSetupPluginItem --------------------------------------------------

class cMenuSetupPluginItem : public cOsdItem {
private:
  int pluginIndex;
public:
  cMenuSetupPluginItem(const char *Name, int Index);
  int PluginIndex(void) { return pluginIndex; }
  };

cMenuSetupPluginItem::cMenuSetupPluginItem(const char *Name, int Index)
:cOsdItem(Name)
{
  pluginIndex = Index;
}

// --- cMenuSetupPlugins -----------------------------------------------------

class cMenuSetupPlugins : public cMenuSetupBase {
public:
  cMenuSetupPlugins(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuSetupPlugins::cMenuSetupPlugins(void)
{
  SetSection(tr("Plugins"));
  SetHasHotkeys();
  for (int i = 0; ; i++) {
      cPlugin *p = cPluginManager::GetPlugin(i);
      if (p) {
         char *buffer = NULL;
         asprintf(&buffer, "%s (%s) - %s", p->Name(), p->Version(), p->Description());
         Add(new cMenuSetupPluginItem(hk(buffer), i));
         free(buffer);
         }
      else
         break;
      }
}

eOSState cMenuSetupPlugins::ProcessKey(eKeys Key)
{
  eOSState state = HasSubMenu() ? cMenuSetupBase::ProcessKey(Key) : cOsdMenu::ProcessKey(Key);

  if (Key == kOk) {
     if (state == osUnknown) {
        cMenuSetupPluginItem *item = (cMenuSetupPluginItem *)Get(Current());
        if (item) {
           cPlugin *p = cPluginManager::GetPlugin(item->PluginIndex());
           if (p) {
              cMenuSetupPage *menu = p->SetupMenu();
              if (menu) {
                 menu->SetPlugin(p);
                 return AddSubMenu(menu);
                 }
              Interface->Info(tr("This plugin has no setup parameters!"));
              }
           }
        }
     else if (state == osContinue)
        Store();
     }
  return state;
}

// --- cMenuSetup ------------------------------------------------------------

class cMenuSetup : public cOsdMenu {
private:
  virtual void Set(void);
  eOSState Restart(void);
public:
  cMenuSetup(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuSetup::cMenuSetup(void)
:cOsdMenu("")
{
  Set();
}

void cMenuSetup::Set(void)
{
  Clear();
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s - VDR %s", tr("Setup"), VDRVERSION);
  SetTitle(buffer);
  SetHasHotkeys();
  Add(new cOsdItem(hk(tr("OSD")),           osUser1));
  Add(new cOsdItem(hk(tr("EPG")),           osUser2));
  Add(new cOsdItem(hk(tr("DVB")),           osUser3));
  Add(new cOsdItem(hk(tr("LNB")),           osUser4));
  Add(new cOsdItem(hk(tr("CICAM")),         osUser5));
  Add(new cOsdItem(hk(tr("Recording")),     osUser6));
  Add(new cOsdItem(hk(tr("Replay")),        osUser7));
  Add(new cOsdItem(hk(tr("Miscellaneous")), osUser8));
  if (cPluginManager::HasPlugins())
  Add(new cOsdItem(hk(tr("Plugins")),       osUser9));
  Add(new cOsdItem(hk(tr("Restart")),       osUser10));
}

eOSState cMenuSetup::Restart(void)
{
  if (Interface->Confirm(cRecordControls::Active() ? tr("Recording - restart anyway?") : tr("Really restart?"))) {
     cThread::EmergencyExit(true);
     return osEnd;
     }
  return osContinue;
}

eOSState cMenuSetup::ProcessKey(eKeys Key)
{
  int osdLanguage = Setup.OSDLanguage;
  eOSState state = cOsdMenu::ProcessKey(Key);

  switch (state) {
    case osUser1: return AddSubMenu(new cMenuSetupOSD);
    case osUser2: return AddSubMenu(new cMenuSetupEPG);
    case osUser3: return AddSubMenu(new cMenuSetupDVB);
    case osUser4: return AddSubMenu(new cMenuSetupLNB);
    case osUser5: return AddSubMenu(new cMenuSetupCICAM);
    case osUser6: return AddSubMenu(new cMenuSetupRecord);
    case osUser7: return AddSubMenu(new cMenuSetupReplay);
    case osUser8: return AddSubMenu(new cMenuSetupMisc);
    case osUser9: return AddSubMenu(new cMenuSetupPlugins);
    case osUser10: return Restart();
    default: ;
    }
  if (Setup.OSDLanguage != osdLanguage) {
     Set();
     if (!HasSubMenu())
        Display();
     }
  return state;
}

// --- cMenuCommands ---------------------------------------------------------

class cMenuCommands : public cOsdMenu {
private:
  eOSState Execute(void);
public:
  cMenuCommands(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuCommands::cMenuCommands(void)
:cOsdMenu(tr("Commands"))
{
  SetHasHotkeys();
  int i = 0;
  cCommand *command;

  while ((command = Commands.Get(i)) != NULL) {
        Add(new cOsdItem(hk(command->Title())));
        i++;
        }
}

eOSState cMenuCommands::Execute(void)
{
  cCommand *command = Commands.Get(Current());
  if (command) {
     char *buffer = NULL;
     asprintf(&buffer, "%s...", command->Title());
     Interface->Status(buffer);
     Interface->Flush();
     free(buffer);
     const char *Result = command->Execute();
     if (Result)
        return AddSubMenu(new cMenuText(command->Title(), Result, fontFix));
     }
  return osContinue;
}

eOSState cMenuCommands::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:  return Execute();
       default:   break;
       }
     }
  return state;
}

// --- cMenuPluginItem -------------------------------------------------------

class cMenuPluginItem : public cOsdItem {
private:
  int pluginIndex;
public:
  cMenuPluginItem(const char *Name, int Index);
  int PluginIndex(void) { return pluginIndex; }
  };

cMenuPluginItem::cMenuPluginItem(const char *Name, int Index)
:cOsdItem(Name, osPlugin)
{
  pluginIndex = Index;
}

// --- cMenuMain -------------------------------------------------------------

#define STOP_RECORDING tr(" Stop recording ")
#define ON_PRIMARY_INTERFACE tr("on primary interface")

cMenuMain::cMenuMain(bool Replaying, eOSState State)
:cOsdMenu("")
{
  replaying = Replaying;
  Set();

  // Initial submenus:

  switch (State) {
    case osRecordings: AddSubMenu(new cMenuRecordings(NULL, 0, true)); break;
    default: break;
    }
}

void cMenuMain::Set(void)
{
  Clear();
  //SetTitle("VDR"); // this is done below, including disk usage
  SetHasHotkeys();

  // Title with disk usage:

#define MB_PER_MINUTE 25.75 // this is just an estimate!

  char buffer[40];
  int FreeMB;
  int Percent = VideoDiskSpace(&FreeMB);
  int Minutes = int(double(FreeMB) / MB_PER_MINUTE);
  int Hours = Minutes / 60;
  Minutes %= 60;
  snprintf(buffer, sizeof(buffer), "%s  -  %s %d%%  -  %2d:%02d %s", tr("VDR"), tr("Disk"), Percent, Hours, Minutes, tr("free"));
  SetTitle(buffer);

  // Basic menu items:

  Add(new cOsdItem(hk(tr("Schedule")),   osSchedule));
  Add(new cOsdItem(hk(tr("Channels")),   osChannels));
  Add(new cOsdItem(hk(tr("Timers")),     osTimers));
  Add(new cOsdItem(hk(tr("Recordings")), osRecordings));

  // Plugins:

  for (int i = 0; ; i++) {
      cPlugin *p = cPluginManager::GetPlugin(i);
      if (p) {
         const char *item = p->MainMenuEntry();
         if (item)
            Add(new cMenuPluginItem(hk(item), i));
         }
      else
         break;
      }

  // More basic menu items:

  Add(new cOsdItem(hk(tr("Setup")),      osSetup));
  if (Commands.Count())
     Add(new cOsdItem(hk(tr("Commands")),  osCommands));

  // Replay control:

  if (replaying)
     Add(new cOsdItem(tr(" Stop replaying"), osStopReplay));

  // Record control:

  if (cRecordControls::StopPrimary()) {
     char *buffer = NULL;
     asprintf(&buffer, "%s%s", STOP_RECORDING, ON_PRIMARY_INTERFACE);
     Add(new cOsdItem(buffer, osStopRecord));
     free(buffer);
     }

  const char *s = NULL;
  while ((s = cRecordControls::GetInstantId(s)) != NULL) {
        char *buffer = NULL;
        asprintf(&buffer, "%s%s", STOP_RECORDING, s);
        Add(new cOsdItem(buffer, osStopRecord));
        free(buffer);
        }

  // Editing control:

  if (cCutter::Active())
     Add(new cOsdItem(tr(" Cancel editing"), osCancelEdit));

  // Color buttons:

  SetHelp(tr("Record"), /*XXX+ cDevice::PrimaryDevice()->CanToggleAudioTrack() ? tr("Language") :XXX*/ NULL, NULL, replaying ? tr("Button$Stop") : cReplayControl::LastReplayed() ? tr("Resume") : NULL);
  Display();
  lastActivity = time(NULL);
}

eOSState cMenuMain::ProcessKey(eKeys Key)
{
  int osdLanguage = Setup.OSDLanguage;
  eOSState state = cOsdMenu::ProcessKey(Key);

  switch (state) {
    case osSchedule:   return AddSubMenu(new cMenuSchedule);
    case osChannels:   return AddSubMenu(new cMenuChannels);
    case osTimers:     return AddSubMenu(new cMenuTimers);
    case osRecordings: return AddSubMenu(new cMenuRecordings);
    case osSetup:      return AddSubMenu(new cMenuSetup);
    case osCommands:   return AddSubMenu(new cMenuCommands);
    case osStopRecord: if (Interface->Confirm(tr("Stop recording?"))) {
                          cOsdItem *item = Get(Current());
                          if (item) {
                             const char *s = item->Text() + strlen(STOP_RECORDING);
                             if (strcmp(s, ON_PRIMARY_INTERFACE) == 0)
                                cRecordControls::StopPrimary(true);
                             else
                                cRecordControls::Stop(item->Text() + strlen(STOP_RECORDING));
                             return osEnd;
                             }
                          }
                       break;
    case osCancelEdit: if (Interface->Confirm(tr("Cancel editing?"))) {
                          cCutter::Stop();
                          return osEnd;
                          }
                       break;
    case osPlugin:     {
                         cMenuPluginItem *item = (cMenuPluginItem *)Get(Current());
                         if (item) {
                            cPlugin *p = cPluginManager::GetPlugin(item->PluginIndex());
                            if (p) {
                               cOsdMenu *menu = p->MainMenuAction();
                               if (menu)
                                  return AddSubMenu(menu);
                               }
                            }
                         state = osEnd;
                       }
                       break;
    default: switch (Key) {
               case kRed:    if (!HasSubMenu())
                                state = osRecord;
                             break;
               case kGreen:  if (!HasSubMenu()) {
                                /*XXX+
                                if (cDevice::PrimaryDevice()->CanToggleAudioTrack()) {
                                   Interface->Clear();
                                   cDevice::PrimaryDevice()->ToggleAudioTrack();
                                   state = osEnd;
                                   }
                                   XXX*/
                                }
                             break;
               case kBlue:   if (!HasSubMenu())
                                state = replaying ? osStopReplay : osReplay;
                             break;
               default:      break;
               }
    }
  if (Key != kNone) {
     lastActivity = time(NULL);
     if (Setup.OSDLanguage != osdLanguage) {
        Set();
        if (!HasSubMenu())
           Display();
        }
     }
  else if (time(NULL) - lastActivity > MENUTIMEOUT)
     state = osEnd;
  return state;
}

// --- cDisplayChannel -------------------------------------------------------

#define DIRECTCHANNELTIMEOUT 1000 //ms
#define INFOTIMEOUT          5000 //ms

cDisplayChannel::cDisplayChannel(int Number, bool Switched)
:cOsdObject(true)
{
  group = -1;
  withInfo = !Switched || Setup.ShowInfoOnChSwitch;
  int EpgLines = withInfo ? 5 : 1;
  lines = 0;
  oldNumber = number = 0;
  cChannel *channel = Channels.GetByNumber(Number);
  Interface->Open(Setup.OSDwidth, Setup.ChannelInfoPos ? EpgLines : -EpgLines);
  if (channel) {
     DisplayChannel(channel);
     DisplayInfo();
     }
  lastTime = time_ms();
}

cDisplayChannel::cDisplayChannel(eKeys FirstKey)
:cOsdObject(true)
{
  group = -1;
  oldNumber = cDevice::CurrentChannel();
  number = 0;
  lastTime = time_ms();
  int EpgLines = Setup.ShowInfoOnChSwitch ? 5 : 1;
  Interface->Open(Setup.OSDwidth, Setup.ChannelInfoPos ? EpgLines : -EpgLines);
  ProcessKey(FirstKey);
}

cDisplayChannel::~cDisplayChannel()
{
  if (number < 0)
     Interface->DisplayChannelNumber(oldNumber);
  Interface->Close();
}

void cDisplayChannel::DisplayChannel(const cChannel *Channel)
{
  if (Channel && Channel->number > 0)
     Interface->DisplayChannelNumber(Channel->number);
  int BufSize = Width() + 1;
  char buffer[BufSize];
  if (Channel && Channel->number > 0)
     snprintf(buffer, BufSize, "%d%s  %s", Channel->number, number ? "-" : "", Channel->name);
  else
     snprintf(buffer, BufSize, "%s", Channel ? Channel->name : tr("*** Invalid Channel ***"));
  Interface->Fill(0, 0, Setup.OSDwidth, 1, clrBackground);
  Interface->Write(0, 0, buffer);
  const char *date = DayDateTime();
  Interface->Write(-strlen(date), 0, date);
  cStatus::MsgOsdChannel(buffer);
}

void cDisplayChannel::DisplayInfo(void)
{
  if (withInfo) {
     const cEventInfo *Present = NULL, *Following = NULL;
     cMutexLock MutexLock;
     const cSchedules *Schedules = cSIProcessor::Schedules(MutexLock);
     if (Schedules) {
        const cSchedule *Schedule = Schedules->GetSchedule();
        if (Schedule) {
           const char *PresentTitle = NULL, *PresentSubtitle = NULL, *FollowingTitle = NULL, *FollowingSubtitle = NULL;
           int Lines = 0;
           if ((Present = Schedule->GetPresentEvent()) != NULL) {
              PresentTitle = Present->GetTitle();
              if (!isempty(PresentTitle))
                 Lines++;
              PresentSubtitle = Present->GetSubtitle();
              if (!isempty(PresentSubtitle))
                 Lines++;
              }
           if ((Following = Schedule->GetFollowingEvent()) != NULL) {
              FollowingTitle = Following->GetTitle();
              if (!isempty(FollowingTitle))
                 Lines++;
              FollowingSubtitle = Following->GetSubtitle();
              if (!isempty(FollowingSubtitle))
                 Lines++;
              }
           if (Lines > lines) {
              const int t = 6;
              int l = 1;
              Interface->Fill(0, 1, Setup.OSDwidth, Lines, clrBackground);
              if (!isempty(PresentTitle)) {
                 Interface->Write(0, l, Present->GetTimeString(), clrYellow, clrBackground);
                 Interface->Write(t, l, PresentTitle, clrCyan, clrBackground);
                 l++;
                 }
              if (!isempty(PresentSubtitle)) {
                 Interface->Write(t, l, PresentSubtitle, clrCyan, clrBackground);
                 l++;
                 }
              if (!isempty(FollowingTitle)) {
                 Interface->Write(0, l, Following->GetTimeString(), clrYellow, clrBackground);
                 Interface->Write(t, l, FollowingTitle, clrCyan, clrBackground);
                 l++;
                 }
              if (!isempty(FollowingSubtitle)) {
                 Interface->Write(t, l, FollowingSubtitle, clrCyan, clrBackground);
                 }
              Interface->Flush();
              lines = Lines;
              lastTime = time_ms();
              cStatus::MsgOsdProgramme(Present ? Present->GetTime() : 0, PresentTitle, PresentSubtitle, Following ? Following->GetTime() : 0, FollowingTitle, FollowingSubtitle);
              }
           }
        }
     }
}

eOSState cDisplayChannel::ProcessKey(eKeys Key)
{
  switch (Key) {
    case k0:
         if (number == 0) {
            // keep the "Toggle channels" function working
            Interface->PutKey(Key);
            return osEnd;
            }
    case k1 ... k9:
         if (number >= 0) {
            number = number * 10 + Key - k0;
            if (number > 0) {
               cChannel *channel = Channels.GetByNumber(number);
               DisplayChannel(channel);
               lastTime = time_ms();
               if (!channel) {
                  number = -1;
                  lastTime += 1000;
                  }
               }
            }
         break;
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
         withInfo = false;
         if (group < 0) {
            cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
            if (channel)
               group = channel->Index();
            }
         if (group >= 0) {
            int SaveGroup = group;
            if (NORMALKEY(Key) == kRight)
               group = Channels.GetNextGroup(group) ;
            else
               group = Channels.GetPrevGroup(group < 1 ? 1 : group);
            if (group < 0)
               group = SaveGroup;
            cChannel *channel = Channels.Get(group);
            if (channel) {
               Interface->Clear();
               DisplayChannel(channel);
               if (!channel->groupSep)
                  group = -1;
               }
            }
         lastTime = time_ms();
         break;
    case kNone:
         if (number && time_ms() - lastTime > DIRECTCHANNELTIMEOUT) {
            if (number > 0 && !Channels.SwitchTo(number))
               number = -1;
            return osEnd;
            }
         break;
    //TODO
    //XXX case kGreen:  return osEventNow;
    //XXX case kYellow: return osEventNext;
    case kOk:     if (group >= 0)
                     Channels.SwitchTo(Channels.Get(Channels.GetNextNormal(group))->number);
                  return osEnd;
    default:      if (NORMALKEY(Key) == kUp || NORMALKEY(Key) == kDown || (Key & (k_Repeat | k_Release)) == 0) {
                     Interface->PutKey(Key);
                     return osEnd;
                     }
    };
  if (time_ms() - lastTime < INFOTIMEOUT) {
     DisplayInfo();
     return osContinue;
     }
  return osEnd;
}

// --- cVolumeBar ------------------------------------------------------------

class cVolumeBar : public cBitmap {
public:
  cVolumeBar(int Width, int Height, int Current, int Total, const char *Prompt = NULL);
  };

cVolumeBar::cVolumeBar(int Width, int Height, int Current, int Total, const char *Prompt)
:cBitmap(Width, Height, 2)
{
  int l = Prompt ? cBitmap::Width(Prompt) : 0;
  int p = (Width - l) * Current / Total;
  Text(0, 0, Prompt, clrGreen);
  Fill(l, 0, p, Height - 1, clrGreen);
  Fill(l + p, 0, Width - 1, Height - 1, clrWhite);
}

// --- cDisplayVolume --------------------------------------------------------

#define VOLUMETIMEOUT 1000 //ms
#define MUTETIMEOUT   5000 //ms

cDisplayVolume *cDisplayVolume::displayVolume = NULL;

cDisplayVolume::cDisplayVolume(void)
:cOsdObject(true)
{
  displayVolume = this;
  timeout = time_ms() + (cDevice::PrimaryDevice()->IsMute() ? MUTETIMEOUT : VOLUMETIMEOUT);
  Interface->Open(Setup.OSDwidth, -1);
  Show();
}

cDisplayVolume::~cDisplayVolume()
{
  Interface->Close();
  displayVolume = NULL;
}

void cDisplayVolume::Show(void)
{
  cDevice *device = cDevice::PrimaryDevice();
  if (device->IsMute()) {
     Interface->Fill(0, 0, Width(), 1, clrTransparent);
     Interface->Write(0, 0, tr("Mute"), clrGreen);
     }
  else {
     int Current = cDevice::CurrentVolume();
     int Total = MAXVOLUME;
     const char *Prompt = tr("Volume ");
#ifdef DEBUG_OSD
     int l = strlen(Prompt);
     int p = int(double(Width() - l) * Current / Total + 0.5);
     Interface->Write(0, 0, Prompt, clrGreen);
     Interface->Fill(l, 0, p, 1, clrGreen);
     Interface->Fill(l + p, 0, Width() - l - p, 1, clrWhite);
#else
     cVolumeBar VolumeBar(Width() * cOsd::CellWidth(), cOsd::LineHeight(), Current, Total, Prompt);
     Interface->SetBitmap(0, 0, VolumeBar);
#endif
     }
}

cDisplayVolume *cDisplayVolume::Create(void)
{
  if (!displayVolume)
     new cDisplayVolume;
  return displayVolume;
}

void cDisplayVolume::Process(eKeys Key)
{
  if (displayVolume)
     displayVolume->ProcessKey(Key);
}

eOSState cDisplayVolume::ProcessKey(eKeys Key)
{
  switch (Key) {
    case kVolUp|k_Repeat:
    case kVolUp:
    case kVolDn|k_Repeat:
    case kVolDn:
         Show();
         timeout = time_ms() + VOLUMETIMEOUT;
         break;
    case kMute:
         if (cDevice::PrimaryDevice()->IsMute()) {
            Show();
            timeout = time_ms() + MUTETIMEOUT;
            }
         else
            timeout = 0;
         break;
    case kNone: break;
    default: if ((Key & k_Release) == 0) {
                Interface->PutKey(Key);
                return osEnd;
                }
    }
  return time_ms() < timeout ? osContinue : osEnd;
}

// --- cRecordControl --------------------------------------------------------

cRecordControl::cRecordControl(cDevice *Device, cTimer *Timer)
{
  eventInfo = NULL;
  instantId = NULL;
  fileName = NULL;
  recorder = NULL;
  device = Device;
  if (!device) device = cDevice::PrimaryDevice();//XXX
  timer = Timer;
  if (!timer) {
     timer = new cTimer(true);
     Timers.Add(timer);
     Timers.Save();
     asprintf(&instantId, cDevice::NumDevices() > 1 ? "%s - %d" : "%s", Channels.GetChannelNameByNumber(timer->channel), device->CardIndex() + 1);
     }
  timer->SetPending(true);
  timer->SetRecording(true);

  const char *Title = NULL;
  const char *Subtitle = NULL;
  const char *Summary = NULL;
  if (GetEventInfo()) {
     Title = eventInfo->GetTitle();
     Subtitle = eventInfo->GetSubtitle();
     Summary = eventInfo->GetExtendedDescription();
     dsyslog("Title: '%s' Subtitle: '%s'", Title, Subtitle);
     }
  cRecording Recording(timer, Title, Subtitle, Summary);
  fileName = strdup(Recording.FileName());
  cRecordingUserCommand::InvokeCommand(RUC_BEFORERECORDING, fileName);
  cChannel *ch = Channels.GetByNumber(timer->channel);
  recorder = new cRecorder(fileName, ch->ca, timer->priority, ch->vpid, ch->apid1, ch->apid2, ch->dpid1, ch->dpid2);
  if (device->AttachReceiver(recorder)) {
     Recording.WriteSummary();
     cStatus::MsgRecording(device, Recording.Name());
     Interface->DisplayRecording(device->CardIndex(), true);
     }
  else
     DELETENULL(recorder);
}

cRecordControl::~cRecordControl()
{
  Stop(true);
  free(instantId);
  free(fileName);
}

#define INSTANT_REC_EPG_LOOKAHEAD 300 // seconds to look into the EPG data for an instant recording

bool cRecordControl::GetEventInfo(void)
{
  cChannel *channel = Channels.GetByNumber(timer->channel);
  time_t Time = timer->active == taActInst ? timer->StartTime() + INSTANT_REC_EPG_LOOKAHEAD : timer->StartTime() + (timer->StopTime() - timer->StartTime()) / 2;
  for (int seconds = 0; seconds <= MAXWAIT4EPGINFO; seconds++) {
      {
        cMutexLock MutexLock;
        const cSchedules *Schedules = cSIProcessor::Schedules(MutexLock);
        if (Schedules) {
           const cSchedule *Schedule = Schedules->GetSchedule(channel->pnr);
           if (Schedule) {
              eventInfo = Schedule->GetEventAround(Time);
              if (eventInfo) {
                 if (seconds > 0)
                    dsyslog("got EPG info after %d seconds", seconds);
                 return true;
                 }
              }
           }
      }
      if (seconds == 0)
         dsyslog("waiting for EPG info...");
      sleep(1);
      }
  dsyslog("no EPG info available");
  return false;
}

void cRecordControl::Stop(bool KeepInstant)
{
  if (timer) {
     cStatus::MsgRecording(device, NULL);
     DELETENULL(recorder);
     timer->SetRecording(false);
     if ((IsInstant() && !KeepInstant) || (timer->IsSingleEvent() && timer->StopTime() <= time(NULL))) {
        isyslog("deleting timer %d", timer->Index() + 1);
        Timers.Del(timer);
        Timers.Save();
        }
     timer = NULL;
     Interface->DisplayRecording(device->CardIndex(), false);
     cRecordingUserCommand::InvokeCommand(RUC_AFTERRECORDING, fileName);
     }
}

bool cRecordControl::Process(time_t t)
{
  if (!recorder || !timer || !timer->Matches(t))
     return false;
  AssertFreeDiskSpace(timer->priority);
  return true;
}

// --- cRecordControls -------------------------------------------------------

cRecordControl *cRecordControls::RecordControls[MAXRECORDCONTROLS] = { NULL };

bool cRecordControls::Start(cTimer *Timer)
{
  int ch = Timer ? Timer->channel : cDevice::CurrentChannel();
  cChannel *channel = Channels.GetByNumber(ch);

  if (channel) {
     bool NeedsDetachReceivers = false;
     cDevice *device = cDevice::GetDevice(channel, Timer ? Timer->priority : Setup.DefaultPriority, &NeedsDetachReceivers);
     if (device) {
        if (NeedsDetachReceivers)
           Stop(device);
        if (!device->SwitchChannel(channel, false)) {
           cThread::EmergencyExit(true);
           return false;
           }
        for (int i = 0; i < MAXRECORDCONTROLS; i++) {
            if (!RecordControls[i]) {
               RecordControls[i] = new cRecordControl(device, Timer);
               return true;
               }
            }
        }
     else if (!Timer || (Timer->priority >= Setup.PrimaryLimit && !Timer->pending))
        isyslog("no free DVB device to record channel %d!", ch);
     }
  else
     esyslog("ERROR: channel %d not defined!", ch);
  return false;
}

void cRecordControls::Stop(const char *InstantId)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]) {
         const char *id = RecordControls[i]->InstantId();
         if (id && strcmp(id, InstantId) == 0)
            RecordControls[i]->Stop();
         }
      }
}

void cRecordControls::Stop(cDevice *Device)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]) {
         if (RecordControls[i]->Uses(Device)) {
            isyslog("stopping recording on DVB device %d due to higher priority", Device->CardIndex() + 1);
            RecordControls[i]->Stop(true);
            }
         }
      }
}

bool cRecordControls::StopPrimary(bool DoIt)
{
  if (cDevice::PrimaryDevice()->Receiving()) {
     //XXX+ disabled for the moment - might become obsolete with DVB_DRIVER_VERSION >= 2002090101
     cDevice *device = NULL;//XXX cDevice::GetDevice(cDevice::PrimaryDevice()->Ca(), 0);
     if (device) {
        if (DoIt)
           Stop(cDevice::PrimaryDevice());
        return true;
        }
     }
  return false;
}

const char *cRecordControls::GetInstantId(const char *LastInstantId)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]) {
         if (!LastInstantId && RecordControls[i]->InstantId())
            return RecordControls[i]->InstantId();
         if (LastInstantId && LastInstantId == RecordControls[i]->InstantId())
            LastInstantId = NULL;
         }
      }
  return NULL;
}

cRecordControl *cRecordControls::GetRecordControl(const char *FileName)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i] && strcmp(RecordControls[i]->FileName(), FileName) == 0)
         return RecordControls[i];
      }
  return NULL;
}

void cRecordControls::Process(time_t t)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]) {
         if (!RecordControls[i]->Process(t))
            DELETENULL(RecordControls[i]);
         }
      }
}

bool cRecordControls::Active(void)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i])
         return true;
      }
  return false;
}

void cRecordControls::Shutdown(void)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++)
      DELETENULL(RecordControls[i]);
}

// --- cProgressBar ----------------------------------------------------------

class cProgressBar : public cBitmap {
protected:
  int total;
  int Pos(int p) { return p * width / total; }
  void Mark(int x, bool Start, bool Current);
public:
  cProgressBar(int Width, int Height, int Current, int Total, const cMarks &Marks);
  };

cProgressBar::cProgressBar(int Width, int Height, int Current, int Total, const cMarks &Marks)
:cBitmap(Width, Height, 2)
{
  total = Total;
  if (total > 0) {
     int p = Pos(Current);
     Fill(0, 0, p, Height - 1, clrGreen);
     Fill(p + 1, 0, Width - 1, Height - 1, clrWhite);
     bool Start = true;
     for (const cMark *m = Marks.First(); m; m = Marks.Next(m)) {
         int p1 = Pos(m->position);
         if (Start) {
            const cMark *m2 = Marks.Next(m);
            int p2 = Pos(m2 ? m2->position : total);
            int h = Height / 3;
            Fill(p1, h, p2, Height - h, clrRed);
            }
         Mark(p1, Start, m->position == Current);
         Start = !Start;
         }
     }
}

void cProgressBar::Mark(int x, bool Start, bool Current)
{
  Fill(x, 0, x, height - 1, clrBlack);
  const int d = height / (Current ? 3 : 9);
  for (int i = 0; i < d; i++) {
      int h = Start ? i : height - 1 - i;
      Fill(x - d + i, h, x + d - i, h, Current ? clrRed : clrBlack);
      }
}

// --- cReplayControl --------------------------------------------------------

char *cReplayControl::fileName = NULL;
char *cReplayControl::title = NULL;

cReplayControl::cReplayControl(void)
:cDvbPlayerControl(fileName)
{
  visible = modeOnly = shown = displayFrames = false;
  lastCurrent = lastTotal = -1;
  timeoutShow = 0;
  timeSearchActive = false;
  marks.Load(fileName);
  cRecording Recording(fileName);
  cStatus::MsgReplaying(this, Recording.Name());
}

cReplayControl::~cReplayControl()
{
  Hide();
  cStatus::MsgReplaying(this, NULL);
  Stop();
}

void cReplayControl::SetRecording(const char *FileName, const char *Title)
{
  free(fileName);
  free(title);
  fileName = FileName ? strdup(FileName) : NULL;
  title = Title ? strdup(Title) : NULL;
}

const char *cReplayControl::LastReplayed(void)
{
  return fileName;
}

void cReplayControl::ClearLastReplayed(const char *FileName)
{
  if (fileName && FileName && strcmp(fileName, FileName) == 0) {
     free(fileName);
     fileName = NULL;
     }
}

void cReplayControl::Show(int Seconds)
{
  if (modeOnly)
     Hide();
  if (!visible) {
     shown = ShowProgress(true);
     timeoutShow = (shown && Seconds > 0) ? time(NULL) + Seconds : 0;
     }
}

void cReplayControl::Hide(void)
{
  if (visible) {
     Interface->Close();
     needsFastResponse = visible = false;
     modeOnly = false;
     }
}

void cReplayControl::DisplayAtBottom(const char *s)
{
  if (s) {
     int w = cOsd::WidthInCells(s);
     int d = max(Width() - w, 0) / 2;
     if (modeOnly) //XXX remove when displaying replay mode differently
        Interface->Fill(0, -1, Interface->Width(), 1, clrTransparent); //XXX remove when displaying replay mode differently
     Interface->Write(d, -1, s);
     Interface->Flush();
     }
  else
     Interface->Fill(12, 2, Width() - 22, 1, clrBackground);
}

void cReplayControl::ShowMode(void)
{
  if (Setup.ShowReplayMode && !timeSearchActive) {
     bool Play, Forward;
     int Speed;
     if (GetReplayMode(Play, Forward, Speed)) {
        bool NormalPlay = (Play && Speed == -1);

        if (!visible) {
           if (NormalPlay)
              return; // no need to do indicate ">" unless there was a different mode displayed before
           // open small display
           /*XXX change when displaying replay mode differently
           Interface->Open(9, -1);
           Interface->Clear();
           XXX*/
           Interface->Open(0, -1); //XXX remove when displaying replay mode differently
           visible = modeOnly = true;
           }

        if (modeOnly && !timeoutShow && NormalPlay)
           timeoutShow = time(NULL) + MODETIMEOUT;
        const char *Mode;
        if (Speed == -1) Mode = Play    ? "  >  " : " ||  ";
        else if (Play)   Mode = Forward ? " X>> " : " <<X ";
        else             Mode = Forward ? " X|> " : " <|X ";
        char buf[16];
        strn0cpy(buf, Mode, sizeof(buf));
        char *p = strchr(buf, 'X');
        if (p)
           *p = Speed > 0 ? '1' + Speed - 1 : ' ';

        eDvbFont OldFont = Interface->SetFont(fontFix);
        DisplayAtBottom(buf);
        Interface->SetFont(OldFont);
        }
     }
}

bool cReplayControl::ShowProgress(bool Initial)
{
  int Current, Total;

  if (GetIndex(Current, Total) && Total > 0) {
     if (!visible) {
        Interface->Open(Setup.OSDwidth, -3);
        needsFastResponse = visible = true;
        }
     if (Initial) {
        Interface->Clear();
        if (title)
           Interface->Write(0, 0, title);
        lastCurrent = lastTotal = -1;
        }
     if (Total != lastTotal) {
        Interface->Write(-7, 2, IndexToHMSF(Total));
        if (!Initial)
           Interface->Flush();
        }
     if (Current != lastCurrent || Total != lastTotal) {
#ifdef DEBUG_OSD
        int p = Width() * Current / Total;
        Interface->Fill(0, 1, p, 1, clrGreen);
        Interface->Fill(p, 1, Width() - p, 1, clrWhite);
#else
        cProgressBar ProgressBar(Width() * cOsd::CellWidth(), cOsd::LineHeight(), Current, Total, marks);
        Interface->SetBitmap(0, cOsd::LineHeight(), ProgressBar);
        if (!Initial)
           Interface->Flush();
#endif
        Interface->Write(0, 2, IndexToHMSF(Current, displayFrames));
        Interface->Flush();
        lastCurrent = Current;
        }
     lastTotal = Total;
     ShowMode();
     return true;
     }
  return false;
}

void cReplayControl::TimeSearchDisplay(void)
{
  char buf[64];
  strcpy(buf, tr("Jump: "));
  int len = strlen(buf);
  char h10 = '0' + (timeSearchTime >> 24);
  char h1  = '0' + ((timeSearchTime & 0x00FF0000) >> 16);
  char m10 = '0' + ((timeSearchTime & 0x0000FF00) >> 8);
  char m1  = '0' + (timeSearchTime & 0x000000FF);
  char ch10 = timeSearchPos > 3 ? h10 : '-';
  char ch1  = timeSearchPos > 2 ? h1  : '-';
  char cm10 = timeSearchPos > 1 ? m10 : '-';
  char cm1  = timeSearchPos > 0 ? m1  : '-';
  sprintf(buf + len, "%c%c:%c%c", ch10, ch1, cm10, cm1);
  DisplayAtBottom(buf);
}

void cReplayControl::TimeSearchProcess(eKeys Key)
{
#define STAY_SECONDS_OFF_END 10
  int Seconds = (timeSearchTime >> 24) * 36000 + ((timeSearchTime & 0x00FF0000) >> 16) * 3600 + ((timeSearchTime & 0x0000FF00) >> 8) * 600 + (timeSearchTime & 0x000000FF) * 60;
  int Current = (lastCurrent / FRAMESPERSEC);
  int Total = (lastTotal / FRAMESPERSEC);
  switch (Key) {
    case k0 ... k9:
         if (timeSearchPos < 4) {
            timeSearchTime <<= 8;
            timeSearchTime |= Key - k0;
            timeSearchPos++;
            TimeSearchDisplay();
            }
         break;
    case kLeft:
    case kRight: {
         int dir = (Key == kRight ? 1 : -1);
         if (dir > 0)
            Seconds = min(Total - Current - STAY_SECONDS_OFF_END, Seconds);
         SkipSeconds(Seconds * dir);
         timeSearchActive = false;
         }
         break;
    case kUp:
    case kDown:
         Seconds = min(Total - STAY_SECONDS_OFF_END, Seconds);
         Goto(Seconds * FRAMESPERSEC, Key == kDown);
         timeSearchActive = false;
         break;
    default:
         timeSearchActive = false;
         break;
    }

  if (!timeSearchActive) {
     if (timeSearchHide)
        Hide();
     else
        DisplayAtBottom();
     ShowMode();
     }
}

void cReplayControl::TimeSearch(void)
{
  timeSearchTime = timeSearchPos = 0;
  timeSearchHide = false;
  if (modeOnly)
     Hide();
  if (!visible) {
     Show();
     if (visible)
        timeSearchHide = true;
     else
        return;
     }
  timeoutShow = 0;
  TimeSearchDisplay();
  timeSearchActive = true;
}

void cReplayControl::MarkToggle(void)
{
  int Current, Total;
  if (GetIndex(Current, Total, true)) {
     cMark *m = marks.Get(Current);
     lastCurrent = -1; // triggers redisplay
     if (m)
        marks.Del(m);
     else {
        marks.Add(Current);
        Show(2);
        }
     marks.Save();
     }
}

void cReplayControl::MarkJump(bool Forward)
{
  if (marks.Count()) {
     int Current, Total;
     if (GetIndex(Current, Total)) {
        cMark *m = Forward ? marks.GetNext(Current) : marks.GetPrev(Current);
        if (m)
           Goto(m->position, true);
        }
     displayFrames = true;
     }
}

void cReplayControl::MarkMove(bool Forward)
{
  int Current, Total;
  if (GetIndex(Current, Total)) {
     cMark *m = marks.Get(Current);
     if (m) {
        displayFrames = true;
        int p = SkipFrames(Forward ? 1 : -1);
        cMark *m2;
        if (Forward) {
           if ((m2 = marks.Next(m)) != NULL && m2->position <= p)
              return;
           }
        else {
           if ((m2 = marks.Prev(m)) != NULL && m2->position >= p)
              return;
           }
        Goto(m->position = p, true);
        marks.Save();
        }
     }
}

void cReplayControl::EditCut(void)
{
  if (fileName) {
     Hide();
     if (!cCutter::Active()) {
        if (!cCutter::Start(fileName))
           Interface->Error(tr("Can't start editing process!"));
        else
           Interface->Info(tr("Editing process started"));
        }
     else
        Interface->Error(tr("Editing process already active!"));
     ShowMode();
     }
}

void cReplayControl::EditTest(void)
{
  int Current, Total;
  if (GetIndex(Current, Total)) {
     cMark *m = marks.Get(Current);
     if (!m)
        m = marks.GetNext(Current);
     if (m) {
        if ((m->Index() & 0x01) != 0)
           m = marks.Next(m);
        if (m) {
           Goto(m->position - SecondsToFrames(3));
           Play();
           }
        }
     }
}

eOSState cReplayControl::ProcessKey(eKeys Key)
{
  if (!Active())
     return osEnd;
  if (visible) {
     if (timeoutShow && time(NULL) > timeoutShow) {
        Hide();
        ShowMode();
        timeoutShow = 0;
        }
     else if (modeOnly)
        ShowMode();
     else
        shown = ShowProgress(!shown) || shown;
     }
  bool DisplayedFrames = displayFrames;
  displayFrames = false;
  if (timeSearchActive && Key != kNone) {
     TimeSearchProcess(Key);
     return osContinue;
     }
  bool DoShowMode = true;
  switch (Key) {
    // Positioning:
    case kUp:      Play(); break;
    case kDown:    Pause(); break;
    case kLeft|k_Release:
                   if (Setup.MultiSpeedMode) break;
    case kLeft:    Backward(); break;
    case kRight|k_Release:
                   if (Setup.MultiSpeedMode) break;
    case kRight:   Forward(); break;
    case kRed:     TimeSearch(); break;
    case kGreen|k_Repeat:
    case kGreen:   SkipSeconds(-60); break;
    case kYellow|k_Repeat:
    case kYellow:  SkipSeconds( 60); break;
    case kBlue:    Hide();
                   Stop();
                   return osEnd;
    default: {
      DoShowMode = false;
      switch (Key) {
        // Editing:
        //XXX should we do this only when the ProgressDisplay is on???
        case kMarkToggle:      MarkToggle(); break;
        case kMarkJumpBack:    MarkJump(false); break;
        case kMarkJumpForward: MarkJump(true); break;
        case kMarkMoveBack|k_Repeat:
        case kMarkMoveBack:    MarkMove(false); break;
        case kMarkMoveForward|k_Repeat:
        case kMarkMoveForward: MarkMove(true); break;
        case kEditCut:         EditCut(); break;
        case kEditTest:        EditTest(); break;
        default: {
          displayFrames = DisplayedFrames;
          switch (Key) {
            // Menu control:
            case kOk:      if (visible && !modeOnly) {
                              Hide();
                              DoShowMode = true;
                              }
                           else
                              Show();
                           break;
            case kBack:    return osRecordings;
            default:       return osUnknown;
            }
          }
        }
      }
    }
  if (DoShowMode)
     ShowMode();
  if (DisplayedFrames && !displayFrames)
     Interface->Fill(0, 2, 11, 1, clrBackground);
  return osContinue;
}

