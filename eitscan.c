/*
 * eitscan.c: EIT scanner
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: eitscan.c 1.9 2002/10/19 11:48:02 kls Exp $
 */

#include "eitscan.h"
#include <stdlib.h>
#include "channels.h"
#include "dvbdevice.h"

cEITScanner::cEITScanner(void)
{
  lastScan = lastActivity = time(NULL);
  currentChannel = 0;
  lastChannel = 0;
  numTransponders = 0;
  transponders = NULL;
}

cEITScanner::~cEITScanner()
{
  free(transponders);
}

bool cEITScanner::TransponderScanned(cChannel *Channel)
{
  for (int i = 0; i < numTransponders; i++) {
      if (transponders[i] == Channel->Frequency())
         return true;
      }
  transponders = (int *)realloc(transponders, ++numTransponders * sizeof(int));
  transponders[numTransponders - 1] = Channel->Frequency();
  return false;
}

void cEITScanner::Activity(void)
{
  if (currentChannel) {
     Channels.SwitchTo(currentChannel);
     currentChannel = 0;
     }
  lastActivity = time(NULL);
}

void cEITScanner::Process(void)
{
  if (Setup.EPGScanTimeout && Channels.MaxNumber() > 1) {
     time_t now = time(NULL);
     if (now - lastScan > ScanTimeout && now - lastActivity > ActivityTimeout) {
        for (int i = 0; i < cDevice::NumDevices(); i++) {
            cDevice *Device = cDevice::GetDevice(i);
            if (Device && Device->CardIndex() < MAXDVBDEVICES) {
               if (Device != cDevice::PrimaryDevice() || (cDevice::NumDevices() == 1 && Setup.EPGScanTimeout && now - lastActivity > Setup.EPGScanTimeout * 3600)) {
                  if (!(Device->Receiving() || Device->Replaying())) {
                     int oldCh = lastChannel;
                     int ch = oldCh + 1;
                     while (ch != oldCh) {
                           if (ch > Channels.MaxNumber()) {
                              ch = 1;
                              numTransponders = 0;
                              }
                           cChannel *Channel = Channels.GetByNumber(ch, 1);
                           if (Channel) {
                              if (!Device->ProvidesChannel(Channel))
                                 break;
                              if (Channel->Sid() && !TransponderScanned(Channel)) {
                                 if (Device == cDevice::PrimaryDevice() && !currentChannel)
                                    currentChannel = Device->CurrentChannel();
                                 Device->SwitchChannel(Channel, false);
                                 lastChannel = ch;
                                 break;
                                 }
                              }
                           ch = Channel->Number() + 1;
                           }
                     }
                  }
               }
            else
               lastChannel++; // avoid hangup in case the last channel in the list is not provided by a DVB card
            }
        lastScan = time(NULL);
        }
     }
}
