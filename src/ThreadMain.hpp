/*
 Copyright (C) 2014 Suriyan Ramasami <suriyan.r@gmail.com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef THREADMAINHPP
#define THREADMAINHPP

#include "XChange.hpp"
#include "Compatibility.hpp"

enum TransferTypeE {
   OUTGOING,
   INCOMING
};

typedef struct DCC_Container_t {
   TCPConnect *Connection;
   XChange    *XGlobal;
   char       *RemoteNick;
   char       *RemoteDottedIP;
   unsigned long RemoteLongIP; // Set before spawning DCCChatThr.
   TransferTypeE TransferType;
   size_t     ResumePosition; // Used with INCOMING/OUTGOING.
   size_t     FileSize;       // Used when INCOMING (actual fullsize)
   char       *FileName; // Holds actual relative full PATH

   int        CurrentSends; // These six are filled up by DCCChatThr to 
   int        TotalSends;   // be passed to DCCChatClient.
   int        CurrentQueues;
   int        TotalQueues;
   char       ClientType;
   char       *TriggerName;
} DCC_Container_t;

#define DCC_CONTAINER_INIT(f) memset(f, 0, sizeof(DCC_Container_t))

void toServerThr(XChange *XGlobal);
void toServerNowThr(XChange *XGlobal);
void fromServerThr(XChange *XGlobal);
void DCCThr(XChange *XGlobal);
void DCCChatThr(DCC_Container_t *DCC_Container);
void toUIThr(XChange *XGlobal);
void fromUIThr(XChange *XGlobal);
void toTriggerThr(XChange *XGlobal);
void toTriggerNowThr(XChange *XGlobal);
void DCCServerThr(XChange *XGlobal);
void DwnldInitThr(XChange *XGlobal);
void FileServerThr(DCC_Container_t *DCC_Container);
void TimerThr(XChange *XGlobal);
void TransferThr(DCC_Container_t *DCC_Container);
void UpnpThr(XChange *XGlobal);
void SwarmThr(XChange *XGlobal);

void joinChannels(XChange *XGlobal);
void markNotFireWalled(XChange *XGlobal, char *OurNick);

#define DCCSERVER_TIMEOUT 180
#define FSERV_TIMEOUT 180
#define DCCCHAT_TIMEOUT 180
#define DCCSEND_TIMEOUT 180

// Time delay before a NODELAY ctcp trigger message is sent to access triggers.
// To avoid the Server "708" error message.
// "Message target change too fast"
// Below is not used anymore, we issues a NODELAY ctcp trigger immediately.
// It is still used as the dealy between two double click download attempts
// in DwnldInitThr
#define IRCSERVER_TARGET_CHANGE_DELAY 20

// Time related to Advertisement sync algo. Values are in seconds.
#define FSERV_RELATIVE_AD_TIME 300
#define FSERV_INITIAL_AD_TIME  FSERV_RELATIVE_AD_TIME * 3

// Time related to PING/PONG - 2 minutes
#define PINGPONG_TIMEOUT 120

// Time related to Auto Saving the PARTIALS/WAITING/FSERV Information. 
// 10 minutes
#define PARTIAL_WAITING_FSERV_SAVE_TIME_INTERVAL   600

// Time related to pumping Upgrade sends. 1 minute.
#define PROCESS_UPGRADE_TIME     60

// Sleep time before disconnecting a download - used in DwnldInitThr.
#define CANCEL_DOWNLOAD_TIME     10

#endif
