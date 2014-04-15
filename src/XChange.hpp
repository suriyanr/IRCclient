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

#ifndef CLASS_XCHANGE
#define CLASS_XCHANGE

#ifdef __MINGW32__
#  include <windows.h> 
#else
#  include <pthread.h>
#  include <semaphore.h>
#endif

#include "Compatibility.hpp"

#include "SwarmStream.hpp"
#include "LineParse.hpp"
#include "ConnectionMethod.hpp"
#include "IRCServerList.hpp"
#include "IRCChannelList.hpp"
#include "IRCNickLists.hpp"
#include "LineQueue.hpp"
#include "TCPConnect.hpp"
#include "FilesDetailList.hpp"

#ifndef TRUE
# define TRUE 1
#endif

/*
  This class holds two type of changing objects.
  Both categories are protected by Mutexes.
  Second category has semaphore to avoid race condition and to make
   threads sleep rather than spin loop indefinitely.
  The first category is the Connection Info, Nick, Channel List,
  Server List, Network Name, Connected Server Name, FilesDetailList.
  The consumer and writer of these variables can be many. Hence we
  have an array of ThreadIDs, which tell us who has read the change and
  has not. Though this does not guarantee that all readers get the same
  information. It does guarantee that they get the latest changed information,
  if any.
  The special case is IRC_QUIT, IRC_DisConnected variable. It doesnt have 
  any mutex etc.
  Cause all anyone wants to know is to quit or not, and we just return
  its value.

  The second category are queues which hold lines.
     ToUI Q, DCC Q, FromServer Q, ToServerNow Q, ToServer Q
  Each Q is worked by a seperate thread.
  The consumer of the Q is a single thread. The writer can be many.
  Hence just a mutex is enough to control it.
  Visualise the following threads and their effect on the Q:
  a. Main IRC loop, reads lines from server and takes care of
     (Feeds the ToUI, DCC, Fromserver, Trigger Q)
     1. Queues ToUI related lines in the ToUI Queue. (UI Thread)
     2. Queues DCC related lines in the DCC Queue.
     3. PRIVMSG to Channel messages in the Trigger Queue.
     4. Rest of the messages  + 3. in the FromServer Queue.
  b. DCC Thread. (waits indefinitely on the DCC Q)
     (drains the DCC Q)
     1. Reads DCC Q, and either replies in the ToServerNow Q
        or spawns dedicated DCC Threads for transfer
        or DCC Chat.
        Interacts with FServWaitingNicks Class if CHAT
        Interacts with DwnldWaitingNicks Class if FILE receive.
  c. ToServerNow Thread. (waits indefinitely on the ToServerNow Q)
     (drains the ToServerNow Q)
     1. Sends messages out from the ToServerNow Q immediately.
  d. ToServer Thread. (waits indefinitely on the ToServer Q)
     (drains the ToServer Q)
     1. Sends messages out form the ToServer Q with gap of 2 second.
  e. To UI Thread.
     (drains the ToUI Q)
     1. Reads the ToUI Q and updates UI tabs accordingly.
  f. To Trigger Thread.
     (drains the ToTrigger Q)
     1. Reads the ToTrigger Q and extracts possible Trigger information
        and issues Triggers.
        Extracts possible xdcc information and updates FilesList.
  g. DCCServer Thread.
     This does not work on any queue. Just waits for incoming connects.
     If its from a nick which has been issued a trigger it get the
     dir contents. Interacts with FServWaitingNicks Class if CHAT
     Interacts with DwnldWaitingNicks Class if FILE receive.
  h. DwnldInit Thread. (Download Initiator)
     (drains the ToDwnldInit Q)
     1. Reads the ToDwnldInit Q. This is in the form: size File Name
        ie. First word is a long and 2nd to last is the filename
     2. Queries FilesDB.getFilesDetailListMatchingFileAndSize(...)
        with the size and filename, and gets a list of FilesDetail object.
        For each object in list do {
           if XDCC, update DwnldWaitingNicks Class, issue the GET.
           if FSERV, update FServWaitingNicks Class, issue the trigger.
           sleep 60 seconds.
        }
*/

class XChange {
public:
   XChange();
   ~XChange();

   void setIRC_QUIT();
   bool isIRC_QUIT();

   bool isIRC_DisConnected();

   bool isIRC_CM_Changed(const ConnectionMethod&);
   void resetIRC_CM_Changed();
   const ConnectionMethod& getIRC_CM();
   void putIRC_CM(const ConnectionMethod&);

   bool isIRC_SL_Changed(const IRCServerList&);
   void resetIRC_SL_Changed();
   const IRCServerList& getIRC_SL();
   void putIRC_SL(const IRCServerList&);
  
   bool isIRC_CL_Changed(const IRCChannelList&);
   void resetIRC_CL_Changed();
   const IRCChannelList& getIRC_CL();
   void putIRC_CL(const IRCChannelList&);

   bool isIRC_Nick_Changed(char *varnick);
   void resetIRC_Nick_Changed();
   void getIRC_Nick(char *varnick);
   void putIRC_Nick(char *varnick);
   void getIRC_ProposedNick(char *varnick);
   void putIRC_ProposedNick(char *varnick);
   void getIRC_Password(char *passwd);
   void putIRC_Password(char *passwd);

   bool isIRC_Server_Changed(char *varsvr);
   void resetIRC_Server_Changed();
   void getIRC_Server(char *varserver);
   void getIRC_Network(char *varnetwork);
   void putIRC_Server(char *varserver);

   bool isIRC_IP_Changed(unsigned long varip);
   void resetIRC_IP_Changed();
   unsigned long getIRC_IP(char *dip);
   void putIRC_IP(unsigned long ip, char *dip);

//  IRC TCP Server connection.
   TCPConnect IRC_Conn;
   void putLineIRC_Conn(char *varbuf);

//  The Tab Book UI window.
   void *UI;

// The IRCNickLists Class. It is mutex protected in itself.
// Holds the Nick Lists of the Channels. Written to by FromServerThr.
// Read by FileServer Class, to determine if the nick gets priority
   IRCNickLists NickList;

// Line Queue - to Server Member. Mutex protected with sem.
   LineQueue IRC_ToServer;

// Line Queue - to Server Now Member. Mutex protected with sem.
   LineQueue IRC_ToServerNow;

// Line Queue - from Server Member. Mutex protected with sem.
   LineQueue IRC_FromServer;

// Line Queue - from Server DCC Member. Mutex protected with sem.
   LineQueue IRC_DCC;

// Line Queue - from Server ToUI Member. Mutex protected with sem.
   LineQueue IRC_ToUI;

// Trigger Queue - from Server To Trigger Member. Mutex protected with sem.
   LineQueue IRC_ToTrigger;

// TriggerNow Queue - from Server To TriggerNow Member. Mutex protected with sem
   LineQueue IRC_ToTriggerNow;

// ToDwnldInit Queue - from Search UI To DwnldInit Member. Mutex protected with sem.
   LineQueue UI_ToDwnldInit;

// ToUpnp Queue - To talk to the Upnp module. Mutex protected with sem.
// Pumped by UI or anyone, who wishes to fiddle with upnp.
   LineQueue UI_ToUpnp;

// Upgrade Queue - To send the correct upgrade file. Mutex protected with sem.
// Pumped by FromServerThr, on getting IC_CTCPUPGRADE.
   LineQueue IRC_ToUpgrade;

// The FilesDetailList Class. It is mutex protected in itself.
// Holds the DB of Files and their details.
   FilesDetailList FilesDB;

// The List of Files that I am holding in the Parital Directory.
// It it mutex protected in itself.
   FilesDetailList MyPartialFilesDB;

// The List of Files that I am holding to be served.
// It is mutex protected in itself.
   FilesDetailList MyFilesDB;

// FServ Client Pending Structure. Used only by DCCChatThr and ToTriggerThr
// and DwnldInitThr and DCCChatClient and TabBookWindow
// This is when we are trying to access someone elses File Server.
// We are the client.
// It is mutex protected in itself.
   FilesDetailList FServClientPending;

// FServ In Progress Structure.
// This is when we are talking to someone elses File Server.
// We are the client.
// It is mutex protected in itself.
   FilesDetailList FServClientInProgress;

// DCCChat In Progress Structure.
// This is when we are chatting with comeone. Can possibly be a manually
// issued a CTCP
// It is Mutex protected in itself.
   FilesDetailList DCCChatInProgress;

// DCCChat Pending Structure.
// This is when we are expecting a CHAT connection to connect.
// It is Mutex protected in itseld.
   FilesDetailList DCCChatPending;

// DCC Accept Waiting structure. Populated on receiving a DCC SEND, and sending
// a DCC RESUME. Referred to, to get File Information on receiving a 
// DCC ACCEPT. TimeOut applicable.
   FilesDetailList DCCAcceptWaiting;

// Download Waiting Structure. Its used to update the Download Waiting UI.
// Once we initiate an automatic download attempt, we populate this
// structure with the information.
   FilesDetailList DwnldWaiting;

// Download In Progress stucture. Used by the DwnldInitThr, to check if
// the file to be downloaded is to be further attempted.
// Plus is used by the DownloadThr
   FilesDetailList DwnldInProgress;

// Sends In Progress stucture. Used by the UploadThr
   FilesDetailList SendsInProgress;

// FileServer In Progress structure. Used by the FileServerThr.
// Mainly used to disconnect the File Server session when we are quitting.
   FilesDetailList FileServerInProgress;

// Queues of Files Queued Up to be sent later. Used by the UploadThr.
// Populated by the FileServer class on a GET
   FilesDetailList QueuesInProgress;

// Queues of Files Queued Up to be sent later. Same as above, but used
// for small files -> actual upload bytes will be in small file size category.
   FilesDetailList SmallQueuesInProgress;

// File Server Waiting structure. Used by DCC CHAT to see if
// the incoming connection made is to access our File Server.
   FilesDetailList FileServerWaiting;

// DCC Send Waiting structure. Used by DCC SEND to see if the
// incoming connection made is to send a file.
   FilesDetailList DCCSendWaiting;

// Swarm Incoming Connect Waiting structure. Used by DCCServerThr to see if
// the incoming connection made is for a Swarm Connect.
   FilesDetailList SwarmWaiting;

// Currently allow SWARM_MAX_FILES Swarms to exist.
   SwarmStream Swarm[SWARM_MAX_FILES];

#if 0
// Semaphore used for signaling the UI for updation.
   SEM  UpdateUI_Sem;
#endif

   // Used by FFLC to lock, for serialising FileList
   // FFLC transaction.
   void lockFFLC();
   void unlockFFLC();

   // Used by TabBookWindow/DCCChatThr for DCCChat purposes.
   // So that TabBookWindow doesnt start writing to a Connection which is
   // going to be midway trashed by DCCChatThr, leading in some
   // core dumps.
   void lockDCCChatConnection();
   void unlockDCCChatConnection();

// Other miscellaneus global variables.
// Use the below by caling lock()/unlock()
   void lock();
   void unlock();

// The Bandwidth CAP variables
   size_t PerTransferMaxUploadBPS;
   size_t PerTransferMaxDownloadBPS;
   size_t OverallMaxUploadBPS;
   size_t OverallMaxDownloadBPS;
   size_t OverallMinUploadBPS;

// The last time that a ping/pong was transacted. Note, we update this time
// on receiving a PONG or PING from server, or when we send out a PING
// or PONG.
   time_t PingPongTime;

// The status of UPNP. used in replying the clientinfo ctcp.
// 0 => No router detected.
// 1 => Router detected.
// 2 => Router detected and 8124 port successfully forwarded.
// initialised to 0.
   int UpnpStatus;

// The local IP in dotted form. xxx.xxx.xxx.xxx 
// Initialised in constructor.
   char *DottedInternalIP;

// Below two are for checking on the FFLC algo overhead.
// FFLC = Fast File List Collection algorithm
   double FFLC_BytesIn;
   double FFLC_BytesOut;

// Below two are for checking on the overhead of obtaining a DIR listing
// from SysReset servers.
   double DirAccess_BytesIn;
   double DirAccess_BytesOut;

// The time when we need to put out our FServ advertisement.
   time_t FServAdTime;

// FServ Sends/Queues parameters.
// Note sends always count towards queues.
   long FServQueuesOverall;   // Overall # of queues.      min 10
   long FServQueuesUser;      // Queues allowed per user.  min 1
   long FServSendsOverall;    // Overall # of sends.       min 2
   long FServSendsUser;       // Sends allowed per user.   min 1
   size_t FServSmallFileSize; // Files <= than this are small files.

// The max upload speed achieved.
   double RecordUploadBPS;

// The max download speed achieved.
   double RecordDownloadBPS;

// The total bytes this server has sent out.
   double TotalBytesSent;

// The total bytes this server has received.
   double TotalBytesRcvd;

// The Nick whom we need to portcheck.
   char *PortCheckNick;
// The Window Name where the results of the portcheck go.
   char *PortCheckWindowName;

// The Nick whom we need to CHAT with.
   char *DCCChatNick;

   // To take care of CTCPFILESHA1REPLY.
   // These are filled up in TabBookWindow when user chooses to issue
   // a Check File Integrity.
   char *SHA1_FileName;
   size_t SHA1_FileSize;
   char *SHA1_SHA1;
   time_t SHA1_Time;

   // If we are acting as a Upgrade Server.
   bool UpgradeServerEnable;

   // Probable upgrade from below Nick with corresponding SHA.
   char *Upgrade_SHA;
   char *Upgrade_Nick;
   unsigned long Upgrade_LongIP;
   time_t Upgrade_Time;

// Are we firewalled or not.
// 0 => FIREWALLED, 1 and above = NOT FIREWALLED.
   int FireWallState;

// Our Partial Directory.
   char *PartialDir;

// Our Serving Directory.
   char *ServingDir[4];

// Our Tray Password.
   char *TrayPassword;

// The UI's Font Face.
   char *FontFace;

// The UI's Font Size.
   int FontSize;

private:

// Mutex for access of private members.
   MUTEX XChange_Mutex;

// Mutex for access of the miscellaneous Global variables.
   MUTEX XChange_Misc_Mutex;

   // Mutex for serialising FileList exchange as part of FFLC.
   // Also used to serialise calls to generateMy<Partial>FilesDB()
   MUTEX FFLC_Mutex;

   // Mutex for avoiding DCCChatThr and TabBookWindow stepping over
   // each other over the same Connection for DCC Chat.
   MUTEX DCCChatConnection_Mutex;

// Quitting Flag. We dont mutex this, as a process will set it and
// all trigger a quit.
   bool IRC_QUIT;

// Connection Method member
   ConnectionMethod IRC_CM;

// Server List member
   IRCServerList IRC_SL;

// Channel List member
   IRCChannelList IRC_CL;

// ServerName/Network Name
   char *IRC_Server;
   char *IRC_Network;

// Nick member
   char *IRC_Nick;
   char *IRC_ProposedNick;
   char *IRC_Password;

// IP and DottedIP member
   char *IRC_DottedIP;
   unsigned long IRC_IP;

};


#endif
