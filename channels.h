/*
 * channels.h: Channel handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: channels.h 1.22 2004/10/31 12:54:26 kls Exp $
 */

#ifndef __CHANNELS_H
#define __CHANNELS_H

#include "config.h"
#include "sources.h"
#include "thread.h"
#include "tools.h"

#define ISTRANSPONDER(f1, f2)  (abs((f1) - (f2)) < 4) //XXX

#define CHANNELMOD_NONE     0x00
#define CHANNELMOD_ALL      0xFF
#define CHANNELMOD_NAME     0x01
#define CHANNELMOD_PIDS     0x02
#define CHANNELMOD_ID       0x04
#define CHANNELMOD_CA       0x10
#define CHANNELMOD_TRANSP   0x20
#define CHANNELMOD_RETUNE   (CHANNELMOD_PIDS | CHANNELMOD_CA | CHANNELMOD_TRANSP)

#define CHANNELSMOD_NONE    0
#define CHANNELSMOD_AUTO    1
#define CHANNELSMOD_USER    2

#define MAXAPIDS 32
#define MAXCAIDS  8

struct tChannelParameterMap {
  int userValue;
  int driverValue;
  };

//XXX into cChannel???
int MapToUser(int Value, const tChannelParameterMap *Map);
int MapToDriver(int Value, const tChannelParameterMap *Map);
int UserIndex(int Value, const tChannelParameterMap *Map);
int DriverIndex(int Value, const tChannelParameterMap *Map);

extern const tChannelParameterMap InversionValues[];
extern const tChannelParameterMap BandwidthValues[];
extern const tChannelParameterMap CoderateValues[];
extern const tChannelParameterMap ModulationValues[];
extern const tChannelParameterMap TransmissionValues[];
extern const tChannelParameterMap GuardValues[];
extern const tChannelParameterMap HierarchyValues[];

struct tChannelID {
private:
  int source;
  int nid;
  int tid;
  int sid;
  int rid;
public:
  tChannelID(void) { source = nid = tid = sid = rid = 0; }
  tChannelID(int Source, int Nid, int Tid, int Sid, int Rid = 0) { source = Source; nid = Nid; tid = Tid; sid = Sid; rid = Rid; }
  bool operator== (const tChannelID &arg) const;
  bool Valid(void) { return (nid || tid) && sid; } // rid is optional and source may be 0//XXX source may not be 0???
  tChannelID &ClrRid(void) { rid = 0; return *this; }
  tChannelID &ClrPolarization(void);
  static tChannelID FromString(const char *s);
  const char *ToString(void);
  static const tChannelID InvalidID;
  };

class cChannel;

class cLinkChannel : public cListObject {
private:
  cChannel *channel;
public:
  cLinkChannel(cChannel *Channel) { channel = Channel; }
  cChannel *Channel(void) { return channel; }
  };

class cLinkChannels : public cList<cLinkChannel> {
  };

class cChannel : public cListObject {
  friend class cMenuEditChannel;
private:
  static char *buffer;
  static const char *ToText(const cChannel *Channel);
  char *name;
  char *shortName;
  char *provider;
  char *portalName;
  int __BeginData__;
  int frequency; // MHz
  int source;
  int srate;
  int vpid;
  int ppid;
  int apids[MAXAPIDS + 1]; // list is zero-terminated
  char alangs[MAXAPIDS][4];
  int dpids[MAXAPIDS + 1]; // list is zero-terminated
  char dlangs[MAXAPIDS][4];
  int tpid;
  int caids[MAXCAIDS + 1]; // list is zero-terminated
  int nid;
  int tid;
  int sid;
  int rid;
  int number;    // Sequence number assigned on load
  bool groupSep;
  char polarization;
  int inversion;
  int bandwidth;
  int coderateH;
  int coderateL;
  int modulation;
  int transmission;
  int guard;
  int hierarchy;
  int __EndData__;
  int modification;
  cLinkChannels *linkChannels;
  cChannel *refChannel;
  const char *ParametersToString(void) const;
  bool StringToParameters(const char *s);
public:
  cChannel(void);
  cChannel(const cChannel &Channel);
  ~cChannel();
  cChannel& operator= (const cChannel &Channel);
  const char *ToText(void) const;
  bool Parse(const char *s, bool AllowNonUniqueID = false);
  bool Save(FILE *f);
  const char *Name(void) const { return name; }
  const char *ShortName(bool OrName = false) const { return (OrName && isempty(shortName)) ? name : shortName; }
  const char *Provider(void) const { return provider; }
  const char *PortalName(void) const { return portalName; }
  int Frequency(void) const { return frequency; } ///< Returns the actual frequency, as given in 'channels.conf'
  int Transponder(void) const;                    ///< Returns the transponder frequency in MHz, plus the polarization in case of sat
  static int Transponder(int Frequency, char Polarization); ///< builds the transponder from the given Frequency and Polarization
  int Source(void) const { return source; }
  int Srate(void) const { return srate; }
  int Vpid(void) const { return vpid; }
  int Ppid(void) const { return ppid; }
  int Apid1(void) const { return apids[0]; }
  int Apid2(void) const { return apids[1]; }
  int Dpid1(void) const { return dpids[0]; }
  int Dpid2(void) const { return dpids[1]; }
  int Tpid(void) const { return tpid; }
  int Ca(int Index = 0) const { return Index < MAXCAIDS ? caids[Index] : 0; }
  int Nid(void) const { return nid; }
  int Tid(void) const { return tid; }
  int Sid(void) const { return sid; }
  int Rid(void) const { return rid; }
  int Number(void) const { return number; }
  void SetNumber(int Number) { number = Number; }
  bool GroupSep(void) const { return groupSep; }
  char Polarization(void) const { return polarization; }
  int Inversion(void) const { return inversion; }
  int Bandwidth(void) const { return bandwidth; }
  int CoderateH(void) const { return coderateH; }
  int CoderateL(void) const { return coderateL; }
  int Modulation(void) const { return modulation; }
  int Transmission(void) const { return transmission; }
  int Guard(void) const { return guard; }
  int Hierarchy(void) const { return hierarchy; }
  bool IsCable(void) const { return (source & cSource::st_Mask) == cSource::stCable; }
  bool IsSat(void) const { return (source & cSource::st_Mask) == cSource::stSat; }
  bool IsTerr(void) const { return (source & cSource::st_Mask) == cSource::stTerr; }
  tChannelID GetChannelID(void) const;
  int Modification(int Mask = CHANNELMOD_ALL);
  bool SetSatTransponderData(int Source, int Frequency, char Polarization, int Srate, int CoderateH);
  bool SetCableTransponderData(int Source, int Frequency, int Modulation, int Srate, int CoderateH);
  bool SetTerrTransponderData(int Source, int Frequency, int Bandwidth, int Modulation, int Hierarchy, int CodeRateH, int CodeRateL, int Guard, int Transmission);
  void SetId(int Nid, int Tid, int Sid, int Rid = 0);
  void SetName(const char *Name, const char *ShortName, const char *Provider);
  void SetPortalName(const char *PortalName);
  void SetPids(int Vpid, int Ppid, int *Apids, char ALangs[][4], int *Dpids, char DLangs[][4], int Tpid);
  void SetCaIds(const int *CaIds); // list must be zero-terminated
  void SetCaDescriptors(int Level);
  void SetLinkChannels(cLinkChannels *LinkChannels);
  void SetRefChannel(cChannel *RefChannel);
  };

class cChannels : public cRwLock, public cConfig<cChannel> {
private:
  int maxNumber;
  int modified;
  int beingEdited;
public:
  cChannels(void);
  bool Load(const char *FileName, bool AllowComments = false, bool MustExist = false);
  int GetNextGroup(int Idx);   // Get next channel group
  int GetPrevGroup(int Idx);   // Get previous channel group
  int GetNextNormal(int Idx);  // Get next normal channel (not group)
  void ReNumber(void);         // Recalculate 'number' based on channel type
  cChannel *GetByNumber(int Number, int SkipGap = 0);
  cChannel *GetByServiceID(int Source, int Transponder, unsigned short ServiceID);
  cChannel *GetByChannelID(tChannelID ChannelID, bool TryWithoutRid = false, bool TryWithoutPolarization = false);
  int BeingEdited(void) { return beingEdited; }
  void IncBeingEdited(void) { beingEdited++; }
  void DecBeingEdited(void) { beingEdited--; }
  bool HasUniqueChannelID(cChannel *NewChannel, cChannel *OldChannel = NULL);
  bool SwitchTo(int Number);
  int MaxNumber(void) { return maxNumber; }
  void SetModified(bool ByUser = false);
  int Modified(void);
      ///< Returns 0 if no channels have been modified, 1 if an automatic
      ///< modification has been made, and 2 if the user has made a modification.
      ///< Calling this function resets the 'modified' flag to 0.
  cChannel *NewChannel(const cChannel *Transponder, const char *Name, const char *ShortName, const char *Provider, int Nid, int Tid, int Sid, int Rid = 0);
  };

extern cChannels Channels;

const char *ChannelString(const cChannel *Channel, int Number);

#endif //__CHANNELS_H
