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

#include <iostream>
using namespace std;

#include "XChange.hpp"
#include "StackTrace.hpp"
#include "Utilities.hpp"

#include "Compatibility.hpp"

// Constructor.
XChange::XChange() {

   TRACE();

   UI = NULL;
   IRC_QUIT = false;

#ifdef __MINGW32__
   XChange_Mutex = CreateMutex(NULL, FALSE, NULL);
   XChange_Misc_Mutex = CreateMutex(NULL, FALSE, NULL);
   FFLC_Mutex = CreateMutex(NULL, FALSE, NULL);
   DCCChatConnection_Mutex = CreateMutex(NULL, FALSE, NULL);

#if 0
   // initial and max count of this UI semaphore is 1.
   UpdateUI_Sem = CreateSemaphore(NULL, 1, 1, NULL);
#endif
#else
   pthread_mutex_init(&XChange_Mutex, NULL);
   pthread_mutex_init(&XChange_Misc_Mutex, NULL);
   pthread_mutex_init(&FFLC_Mutex, NULL);
   pthread_mutex_init(&DCCChatConnection_Mutex, NULL);

#if 0
   // initial and max count of this UI semaphore is 1.
#ifdef USE_NAMED_SEMAPHORE
   char sem_name[64];
   sprintf(sem_name, "%s%d", ONLY_ONE_SEM_NAME, UseNamedSemaphore);
   UseNamedSemaphore++;
   UpdateUI_Sem = sem_open(sem_name, O_CREAT, S_IRWXU, 0);
   if (UpdateUI_Sem == NULL) {
      COUT(cout << "CRITICAL: sem_open returned NULL" << endl;)
   }
#else
   int sem_ret = sem_init(&UpdateUI_Sem, 0, 1);
   if (sem_ret == -1) {
      COUT(cout << "CRITICAL: sem_init returned -1" << endl;)
   }
#endif // USE_NAMED_SEMAPHORE
#endif
#endif // #if 0

   IRC_Nick = NULL;
   IRC_ProposedNick = NULL;
   IRC_Password = NULL;
   IRC_Server = NULL;
   IRC_Network = NULL;
   IRC_IP = 0;
   IRC_DottedIP = NULL;
   PortCheckNick = NULL;
   PortCheckWindowName = NULL;
   DCCChatNick = NULL;
   SHA1_FileName = NULL;
   SHA1_SHA1 = NULL;
   SHA1_Time = 0;
   UpnpStatus = 0;
   FireWallState = 0;
   UpgradeServerEnable = false;
   Upgrade_SHA = NULL;
   Upgrade_Nick = NULL;
   Upgrade_LongIP = 0;
   Upgrade_Time = 0;
   PartialDir = NULL;
   for (int i = 0; i < FSERV_MAX_SERVING_DIRECTORIES; i++) {
      ServingDir[i] = NULL;
   }
   TrayPassword = NULL;
   FontFace = NULL;

   // Set our internal IP.
   DottedInternalIP = new char[20];
   getInternalDottedIP(DottedInternalIP);

   // Initialize the CAP variables to 0 => no cap.
   PerTransferMaxUploadBPS = 0;
   PerTransferMaxDownloadBPS = 0;
   OverallMaxUploadBPS = 0;
   OverallMinUploadBPS = 0;
   OverallMaxDownloadBPS = 0;

   // Initialise the ping/pong time to current time.
   PingPongTime = time(NULL);

   // shouldnt set special options on this socket.
   IRC_Conn.setSetOptions(false);

   FServAdTime = 0; // No advertisement till we set it.

   // File Server Sends/Queues values.
   FServQueuesOverall = FSERV_MIN_QUEUES_OVERALL;
   FServQueuesUser = FSERV_MIN_QUEUES_USER;
   FServSendsOverall = FSERV_MIN_SENDS_OVERALL;
   FServSendsUser = FSERV_MIN_SENDS_USER;

   // FServ Max FileSize below which its labelled a small file.
   FServSmallFileSize = FSERV_SMALL_FILE_DEFAULT_SIZE;

   FFLC_BytesIn = 0.0;
   FFLC_BytesOut = 0.0;
   DirAccess_BytesIn = 0.0;
   DirAccess_BytesOut = 0.0;

   // FilesDB.setTimeOut(3600);            // 1 hour
   FilesDB.setTimeOut(0);            // No timeout.
   FilesDB.setSort(FILELIST_NO_SORT);

   MyFilesDB.setTimeOut(0);             // No timeout
   MyFilesDB.setSort(FILELIST_SORT_BY_DIR_FILENAME);

   MyPartialFilesDB.setTimeOut(0);      // No timeout
   MyPartialFilesDB.setSort(FILELIST_NO_SORT);

   FServClientPending.setTimeOut(180);  // 3 minutes
   FServClientPending.setSort(FILELIST_NO_SORT);

   FServClientInProgress.setTimeOut(0); // No timeout
   FServClientInProgress.setSort(FILELIST_NO_SORT);

   DCCChatPending.setTimeOut(180); // 3 minutes.
   DCCChatPending.setSort(FILELIST_NO_SORT);

   DCCChatInProgress.setTimeOut(0); // No timeout
   DCCChatInProgress.setSort(FILELIST_NO_SORT);

   DCCAcceptWaiting.setTimeOut(180);    // 3 minutes
   DCCAcceptWaiting.setSort(FILELIST_NO_SORT);

   DwnldWaiting.setTimeOut(0);          // No timeout
   DwnldWaiting.setSort(FILELIST_NO_SORT);

   DwnldInProgress.setTimeOut(0);       // No timeout
   DwnldInProgress.setSort(FILELIST_NO_SORT);

   SendsInProgress.setTimeOut(0);       // No timeout
   SendsInProgress.setSort(FILELIST_NO_SORT);

   QueuesInProgress.setTimeOut(0);      // No timeout
   QueuesInProgress.setSort(FILELIST_NO_SORT);

   SmallQueuesInProgress.setTimeOut(0); // No timeout
   SmallQueuesInProgress.setSort(FILELIST_NO_SORT);

   FileServerInProgress.setTimeOut(0);  // No timeout
   FileServerInProgress.setSort(FILELIST_NO_SORT);

   FileServerWaiting.setTimeOut(180);   // 3 minutes
   FileServerWaiting.setSort(FILELIST_NO_SORT);

   DCCSendWaiting.setTimeOut(180);      // 3 minutes
   DCCSendWaiting.setSort(FILELIST_NO_SORT);

   SwarmWaiting.setTimeOut(180);        // 3 minutes
   SwarmWaiting.setSort(FILELIST_NO_SORT);

}

// Destructor.
XChange::~XChange() {

   TRACE();
   delete [] IRC_Nick;
   delete [] IRC_ProposedNick;
   delete [] IRC_Password;
   delete [] IRC_Network;
   delete [] IRC_Server;
   delete [] IRC_DottedIP;
   delete [] PortCheckNick;
   delete [] PortCheckWindowName;
   delete [] DCCChatNick;
   delete [] PartialDir;
   for (int i = 0; i < FSERV_MAX_SERVING_DIRECTORIES; i++) {
      delete [] ServingDir[i];
   }
   delete [] TrayPassword;
   delete [] FontFace;
   delete [] DottedInternalIP;
   delete [] SHA1_FileName;
   delete [] SHA1_SHA1;
   delete [] Upgrade_SHA;
   delete [] Upgrade_Nick;
   DestroyMutex(XChange_Mutex);
   DestroyMutex(XChange_Misc_Mutex);
   DestroyMutex(FFLC_Mutex);
#if 0
   DestroySemaphore(UpdateUI_Sem);
#endif
}

void XChange::setIRC_QUIT() {
   TRACE();
   IRC_QUIT = true;
}

bool XChange::isIRC_QUIT() {
   TRACE();
   return(IRC_QUIT);
}

bool XChange::isIRC_DisConnected() {
bool retvalb;

   TRACE();

   if (IRC_Conn.state() == TCP_ESTABLISHED) {
      retvalb = false;
   }
   else {
      retvalb = true;
   }

   return(retvalb);
}


bool XChange::isIRC_CM_Changed(const ConnectionMethod &ChkCM) {
bool retvalb;

   TRACE();

   WaitForMutex(XChange_Mutex);

   if (IRC_CM == ChkCM) retvalb = false;
   else retvalb = true;

   ReleaseMutex(XChange_Mutex);

   return(retvalb);
}

void XChange::resetIRC_CM_Changed() {

   TRACE();

   ReleaseMutex(XChange_Mutex);
}

const ConnectionMethod& XChange::getIRC_CM() {
   TRACE();
   WaitForMutex(XChange_Mutex);
   return(IRC_CM);
}

void XChange::putIRC_CM(const ConnectionMethod& varCM) {

   TRACE();
   WaitForMutex(XChange_Mutex);

   IRC_CM = varCM;

   COUT(cout << "putIRC_CM: CM Changed" << endl;)
   COUT(IRC_CM.printDebug();)

   ReleaseMutex(XChange_Mutex);
}

bool XChange::isIRC_SL_Changed(const IRCServerList &ChkSL) {
bool retvalb;

   TRACE();
   WaitForMutex(XChange_Mutex);

   if (IRC_SL == ChkSL) retvalb = false;
   else retvalb = true;

   ReleaseMutex(XChange_Mutex);

   return(retvalb);
}

void XChange::resetIRC_SL_Changed() {

   TRACE();
   ReleaseMutex(XChange_Mutex);
}

const IRCServerList& XChange::getIRC_SL() {
   TRACE();
   WaitForMutex(XChange_Mutex);
   return(IRC_SL);
}

void XChange::putIRC_SL(const IRCServerList& varSL) {
int i;

   TRACE();
   WaitForMutex(XChange_Mutex);

   IRC_SL = varSL;

   COUT(cout << "putIRC_SL: SL Changed" << endl;)
   COUT(IRC_SL.printDebug();)

   ReleaseMutex(XChange_Mutex);
}

bool XChange::isIRC_CL_Changed(const IRCChannelList& ChkCL) {
bool retvalb;

   TRACE();
   WaitForMutex(XChange_Mutex);

   if (IRC_CL == ChkCL) retvalb = false;
   else retvalb = true;

   ReleaseMutex(XChange_Mutex);
   return(retvalb);
}

void XChange::resetIRC_CL_Changed() {

   TRACE();
   ReleaseMutex(XChange_Mutex);
}

const IRCChannelList& XChange::getIRC_CL() {
   TRACE();
   WaitForMutex(XChange_Mutex);
   return(IRC_CL);
}

void XChange::putIRC_CL(const IRCChannelList& varCL) {
int i;

   TRACE();
   WaitForMutex(XChange_Mutex);

   IRC_CL = varCL;

   COUT(cout << "putIRC_CL: CL Changed" << endl;)
   COUT(IRC_CL.printDebug();)

   ReleaseMutex(XChange_Mutex);
}

bool XChange::isIRC_Server_Changed(char *varsrv) {
bool retvalb = false;

   TRACE();

   WaitForMutex(XChange_Mutex);

   do {
      if (IRC_Server == NULL) break;
      if (strcasecmp(IRC_Server, varsrv)) retvalb = true;
   } while (false);

   ReleaseMutex(XChange_Mutex);

   return(retvalb);
}

void XChange::resetIRC_Server_Changed() {

   TRACE();

   ReleaseMutex(XChange_Mutex);
}

void XChange::getIRC_Server(char *varserver) {

   TRACE();
   WaitForMutex(XChange_Mutex);

// Hope that varserver is a valid pointer and has enough space to 
// to occupy IRC_Server. Dont check, as its better to crash and fix.
   strcpy(varserver, IRC_Server);
}

void XChange::getIRC_Network(char *varnetwork) {
// We dont hold Mutex here. We assume its done when getting IRC_Server.
// So first: getIRC_Server() and then getIRC_Network() and then
// resetIRC_Server_Changed()
// Hope that varnetwork is a valid pointer and has enough space to
// to occupy IRC_Network. Dont check, as its better to crash and fix.
   TRACE();

   strcpy(varnetwork, IRC_Network);
}

void XChange::putIRC_Server(char *varserver) {
LineParse tmpline;
int wordindex;
const char *tmp_netname;

   TRACE();

   WaitForMutex(XChange_Mutex);

   delete [] IRC_Server;
   IRC_Server = new char[strlen(varserver) + 1];
   strcpy(IRC_Server, varserver);

   tmpline = IRC_Server;
   tmpline.setDeLimiter('.');
   delete [] IRC_Network;
   wordindex = tmpline.getWordCount() - 1;
   tmp_netname = tmpline.getWord(wordindex);
   IRC_Network = new char[strlen(tmp_netname) + 1];
   strcpy(IRC_Network, tmp_netname);
   COUT(cout << "putIRC_Server: " << IRC_Server << " Network: " << IRC_Network << endl;)

   ReleaseMutex(XChange_Mutex);
}

bool XChange::isIRC_IP_Changed(unsigned long ip) {
bool retvalb = false;

   TRACE();
   WaitForMutex(XChange_Mutex);

   if (ip != IRC_IP) retvalb = true;

   ReleaseMutex(XChange_Mutex);

   return(retvalb);
}

void XChange::resetIRC_IP_Changed() {

   TRACE();
   ReleaseMutex(XChange_Mutex);
}

// If dip is NULL, no need to fill that up.
unsigned long XChange::getIRC_IP(char *dip) {
   TRACE();
   WaitForMutex(XChange_Mutex);
   if (dip) {
      dip[0] = '\0';
      if (IRC_DottedIP) {
         strcpy(dip, IRC_DottedIP);
      }
   }

   return(IRC_IP);
}

void XChange::putIRC_IP(unsigned long ip, char *dip) {
int i;

   TRACE();
   WaitForMutex(XChange_Mutex);

   delete [] IRC_DottedIP;
   IRC_DottedIP = new char[strlen(dip) + 1];
   strcpy(IRC_DottedIP, dip);
   IRC_IP = ip;

   COUT(cout << "putIRC_IP: IP Changed. New IP: " << IRC_IP << " " << IRC_DottedIP << endl;)

   ReleaseMutex(XChange_Mutex);
}

bool XChange::isIRC_Nick_Changed(char *varnick) {
bool retvalb = false;

   TRACE();
   WaitForMutex(XChange_Mutex);
   do {
      if (IRC_Nick == NULL) break;
      if (strcasecmp(IRC_Nick, varnick)) retvalb = true;
   } while (false);

   ReleaseMutex(XChange_Mutex);

   return(retvalb);
}

void XChange::resetIRC_Nick_Changed() {

   TRACE();
   ReleaseMutex(XChange_Mutex);
}

void XChange::getIRC_Nick(char *varnick) {
   TRACE();
   WaitForMutex(XChange_Mutex);
// Hope that varnick is a valid pointer and has enough space to
// to occupy IRC_Nick. Dont check, as its better to crash and fix.
   varnick[0] = '\0';
   if (IRC_Nick) {
      strcpy(varnick, IRC_Nick);
   }
}

// We assume getIRC_Nick() is called first => Mutex is owned.
void XChange::getIRC_ProposedNick(char *varnick) {
   TRACE();
// Hope that varnick is a valid pointer and has enough space to
// to occupy IRC_ProposedNick. Dont check, as its better to crash and fix.
   varnick[0] = '\0';
   if (IRC_ProposedNick) {
      strcpy(varnick, IRC_ProposedNick);
   }
}

// We assume getIRC_Nick() is called first => Mutex is owned.
void XChange::getIRC_Password(char *passwd) {
   TRACE();
// Hope that passwd is a valid pointer and has enough space to
// to occupy IRC_Password. Dont check, as its better to crash and fix.
   passwd[0] = '\0';
   if (IRC_Password) {
      strcpy(passwd, IRC_Password);
   }
}

void XChange::putIRC_Password(char *passwd) {

   TRACE();
   WaitForMutex(XChange_Mutex);

   delete [] IRC_Password;
   IRC_Password = new char[strlen(passwd) + 1];
   strcpy(IRC_Password, passwd);

   COUT(cout << "putIRC_Password: Password Changed. New Password: " << IRC_Password << endl;)

   ReleaseMutex(XChange_Mutex);
}

void XChange::putIRC_Nick(char *varnick) {

   TRACE();
   WaitForMutex(XChange_Mutex);

   delete [] IRC_Nick;
   IRC_Nick = new char[strlen(varnick) + 1];
   strcpy(IRC_Nick, varnick);

   COUT(cout << "putIRC_Nick: Nick Changed. New Nick: " << IRC_Nick << endl;)

   ReleaseMutex(XChange_Mutex);
}

void XChange::putIRC_ProposedNick(char *varnick) {

   TRACE();
   WaitForMutex(XChange_Mutex);

   delete [] IRC_ProposedNick;
   IRC_ProposedNick = new char[strlen(varnick) + 1];
   strcpy(IRC_ProposedNick, varnick);

   COUT(cout << "putIRC_ProposedNick: Proposed Nick: " << IRC_ProposedNick << endl;)

   ReleaseMutex(XChange_Mutex);
}

// Write a line to server socket, and and append a new line to it.
void XChange::putLineIRC_Conn(char *varbuf) {

   TRACE();
   WaitForMutex(XChange_Mutex);

   IRC_Conn.writeData(varbuf, strlen(varbuf));
   IRC_Conn.writeData("\n", 1);

   ReleaseMutex(XChange_Mutex);
}

// Lock before accessing the miscellaneous global variables.
void XChange::lock() {
   TRACE();
   WaitForMutex(XChange_Misc_Mutex);
}

// Un Lock after accessing/using the miscellaneous global variables.
void XChange::unlock() {
   TRACE();
   ReleaseMutex(XChange_Misc_Mutex);
}

// Used by FFLC to lock, for serialising FileList
// FFLC transaction.
void XChange::lockFFLC() {
  TRACE();
  WaitForMutex(FFLC_Mutex);
}

// Used by FFLC to unlock, for serialising FileList
// FFLC transaction.
void XChange::unlockFFLC() {
   TRACE();
   ReleaseMutex(FFLC_Mutex);
}

// Used by DCC Chat purposes to lock.
void XChange::lockDCCChatConnection() {
   TRACE();
   WaitForMutex(DCCChatConnection_Mutex);
}

// Used bt DCC Chat purposes to unlock.
void XChange::unlockDCCChatConnection() {
   TRACE();
   ReleaseMutex(DCCChatConnection_Mutex);
}

