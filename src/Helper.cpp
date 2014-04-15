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

#ifdef COUT_OUTPUT
#include <iostream>
using namespace std;
#endif

#include <fx.h>
#include "Helper.hpp"
#include "FServParse.hpp"
#include "StackTrace.hpp"
#include "Utilities.hpp"

#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

#include "SHA1File.hpp"

#include "Compatibility.hpp"

// A Helper Class which has miscellaneous functions
Helper::Helper() {
   TRACE();
   XGlobal = NULL;
   Connection = NULL;
   RetPointer = NULL;
   MyNick = NULL;
}

Helper::~Helper() {
   TRACE();
   delete [] RetPointer;
   delete [] MyNick;
}

void Helper::init(XChange *X) {
   TRACE();
   XGlobal = X;
}

// Called only by DCCChatClient/ FileServer to handle the MM file exchange
// protocol
void Helper::init(XChange *X, TCPConnect *T) {
   TRACE();

   XGlobal = X;
   Connection = T;

   RetPointer = new char[1024];
   MyNick = new char[64];

   // Lets get MyNick too as we need it in this context.
   XGlobal->getIRC_Nick(MyNick);
   XGlobal->resetIRC_Nick_Changed();
}
 
// Returns two values.
int Helper::getSendsAndTotalUploadSpeed(float *UploadSpeed) {
FilesDetail *FD, *ScanFD;
int TotalSends;
time_t CurrentTime = time(NULL);

   TRACE();

   *UploadSpeed = 0.0;
   if (XGlobal == NULL) {
      COUT(cout << "Helper Used without being initialised." << endl;)
      return(0);
   }

   // Lets get the Sends and the Total Upload Speed.
   FD = XGlobal->SendsInProgress.searchFilesDetailList("*");
   ScanFD = FD;
   TotalSends = 0;
   while (ScanFD) {
      *UploadSpeed = *UploadSpeed + (float) ScanFD->UploadBps;
      TotalSends++;
      ScanFD = ScanFD->Next;
   }

   // Lets free the FD List we had obtained.
   XGlobal->SendsInProgress.freeFilesDetailList(FD);

   // DCCSendWaiting also accounts for the Sends.
   TotalSends += XGlobal->DCCSendWaiting.getCount(NULL);
   return(TotalSends);
}

// Returns two values.
int Helper::getGetsAndTotalDownloadSpeed(float *DownloadSpeed) {
FilesDetail *FD, *ScanFD;
int TotalGets;
time_t CurrentTime = time(NULL);

   TRACE();
   *DownloadSpeed = 0.0;

   if (XGlobal == NULL) {
      COUT(cout << "Helper Used without being initialised." << endl;)
      return(0);
   }

   // Get the Gets and the Total Download Speeds.
   FD = XGlobal->DwnldInProgress.searchFilesDetailList("*");
   ScanFD = FD;
   TotalGets = 0;
   while (ScanFD) {
      *DownloadSpeed = *DownloadSpeed + (float) ScanFD->DownloadBps;
      TotalGets++;
      ScanFD = ScanFD->Next;
   }

   // Lets free the FD List we had obtained.
   XGlobal->DwnldInProgress.freeFilesDetailList(FD);
   return(TotalGets);
}

int Helper::getTotalQueues() {
int TotalQueues;

   TRACE();
   // Get the Send Queues.
   TotalQueues = XGlobal->QueuesInProgress.getCount(NULL) +
                 XGlobal->SmallQueuesInProgress.getCount(NULL);
   return(TotalQueues);
}

// Generate the File Server Ad in buffer.
void Helper::generateFServAd(char *buffer) {
char *Response;
int TotalSends;
int TotalGets;
int TotalQueues;
int MyMaxSends;
int MyMaxQueues;
float UploadSpeed;
float DownloadSpeed;
double TSbytes;
char TSbytesUnits[3];
double TRbytes;
char TRbytesUnits[3];
double RUploadBPS;
double RDownloadBPS;
char Nick[64];
char FireWalledString[24];
int FireWall;
int NumberOfSwarms;

   TRACE();

   TotalSends = getSendsAndTotalUploadSpeed(&UploadSpeed);
   TotalGets = getGetsAndTotalDownloadSpeed(&DownloadSpeed);
   TotalQueues = getTotalQueues();

   // Lets get our nick.
   XGlobal->getIRC_Nick(Nick);
   XGlobal->resetIRC_Nick_Changed();

   // Lets get TotalBytesSent pretty.
   strcpy(TSbytesUnits, " B");
   strcpy(TRbytesUnits, " B");

   XGlobal->lock();
   TSbytes = XGlobal->TotalBytesSent;
   TRbytes = XGlobal->TotalBytesRcvd;
   RUploadBPS = XGlobal->RecordUploadBPS;
   RDownloadBPS = XGlobal->RecordDownloadBPS;
   FireWall = XGlobal->FireWallState;
   MyMaxSends = XGlobal->FServSendsOverall;
   MyMaxQueues = XGlobal->FServQueuesOverall * 2;
   XGlobal->unlock();

   // Get Numer of Swarms, and also get their Upload/Download Speed to be
   // added to UploadSpeed/DownloadSpeed.
   NumberOfSwarms = 0;
   for (int i = 0; i < SWARM_MAX_FILES; i++) {
   size_t tempUp, tempDown;
      if (XGlobal->Swarm[i].isBeingUsed()) {
         NumberOfSwarms++;
         XGlobal->Swarm[i].ConnectedNodes.getCurrentSpeeds(&tempUp, &tempDown);
         UploadSpeed = UploadSpeed + (float) tempUp;
         DownloadSpeed = DownloadSpeed + (float) tempDown;
      }
   }

   if (TSbytes < 1024.0);
   else if (TSbytes < 1048576.0) {
      TSbytesUnits[0] = 'K';
      TSbytes = TSbytes / 1024.0;
   }
   else if (TSbytes < 1073741824.0) {
      TSbytesUnits[0] = 'M';
      TSbytes = TSbytes / 1048576.0;
   }
   else if (TSbytes < 1099511627776.0) {
      TSbytesUnits[0] = 'G';
      TSbytes = TSbytes / 1073741824.0;
   }
   else {
      TSbytesUnits[0] = 'T';
      TSbytes = TSbytes / 1099511627776.0;
   }

   // Lets get TotalBytesRcvd pretty.
   if (TRbytes < 1024.0);
   else if (TRbytes < 1048576.0) {
      TRbytesUnits[0] = 'K';
      TRbytes = TRbytes / 1024.0;
   }
   else if (TRbytes < 1073741824.0) {
      TRbytesUnits[0] = 'M';
      TRbytes = TRbytes / 1048576.0;
   }
   else if (TRbytes < 1099511627776.0) {
      TRbytesUnits[0] = 'G';
      TRbytes = TRbytes / 1073741824.0;
   }
   else {
      TRbytesUnits[0] = 'T';
      TRbytes = TRbytes / 1099511627776.0;
   }

   switch (FireWall) {
      case 0:
        strcpy(FireWalledString, "\00304,01100 %\00307,01");
        break;

      case 1:
      case 2:
      case 3:
      case 4:
        sprintf(FireWalledString, "\00308,01%d %c\00307,01", 100 - FireWall*20, '%');
        break;

      case 5:
        strcpy(FireWalledString, "\00309,01NO\00307,01");
        break;
   }

   sprintf(buffer, "\00307,01[\00309,01Fserve Active\00307,01] - "
                   "Trigger:[\00309,01/ctcp %s Masala of %s\00307,01] - "
                   "Sends:[\00309,01%d/%d\00307,01] - "
                   "Queues:[\00309,01%d/%d\00307,01] - "
                   "Record BPS Up|Down:[\00309,01%.2fkB/s | %.2fkB/s\00307,01] - "
                   "Bytes Sent|Rcvd:[\00309,01%.2f%s | %.2f%s\00307,01] - "
                   "Speed Up|Down:[\00309,01%.2fkB/s | %.2fkB/s\00307,01] - "
                   "Swarms:[\00309,01%d\00307,01] - "
                   "FireWalled:[%s] - "
                   "%s %s %s", 
                   Nick, Nick, 
                   TotalSends, MyMaxSends, 
                   TotalQueues, MyMaxQueues, 
                   RUploadBPS/1024.0, 
                   RDownloadBPS/1024.0,
                   TSbytes, TSbytesUnits,
                   TRbytes, TRbytesUnits,
                   UploadSpeed/1024.0, DownloadSpeed/1024.0, 
                   NumberOfSwarms,
                   FireWalledString,
                   CLIENT_NAME_FULL, VERSION_STRING, DATE_STRING);
}

// Generates Text response to a @find search string sent in search
// returns buffer of 0 length if no hit.
void Helper::generateFindHit(char *search, char *buffer) {
FilesDetail *FD;
char Nick[64];
int HitCount;
char s_filesize[32];

   TRACE();
   buffer[0] = '\0';
   if (search == NULL) return;
   if (strlen(search) < 4) return;
   FD = XGlobal->MyFilesDB.searchFilesDetailList(search);
   if (FD == NULL) return;

   HitCount = XGlobal->MyFilesDB.getCount(FD);
   // Lets get our nick.
   XGlobal->getIRC_Nick(Nick);
   XGlobal->resetIRC_Nick_Changed();

   convertFileSizeToString(FD->FileSize, s_filesize);
   // We will just list the first hit.
   sprintf(buffer, "\00307,01Found [\00309,01%d\00307,01] Files. "
                   "Trigger:[\00309,01/ctcp %s Masala Of %s\00307,01] "
                   "- File:[\00309,01%s\00307,01] - Size ["
                   "\00309,01%s\00307,01]",
                   HitCount, Nick, Nick,
                   FD->FileName, s_filesize);

   // Lets free the FD List we had obtained.
   XGlobal->MyFilesDB.freeFilesDetailList(FD);
}

// Reads the Config file on startup. Fills up Nick, Connection, Queue Info
// FServ Info, Font Info, DwnldInProgress, DwnldWaiting in XGlobal
void Helper::readConfigFile() {
FXSettings S;
ConnectionMethod CM;
IRCChannelList CL;
const char *value;
const char *host, *user, *password, *vhost;
int port;
const char empty[] = "";
int fontsize;
size_t MyMaxSmallFileSize;

   TRACE();

   S.parseFile(CONFIG_FILE, true);

   // Lets get the Nick.
   // Both Nick and ProposedNick are set as same, on start up.
   value = S.readStringEntry(CONFIG_IRC_SECTION, CONFIG_IRC_NICK);
   if ( (value == NULL) || (strlen(value) == 0) ) {
   char *storage1;
      // Lets generate the Nick
      storage1 = new char [16];
      generateNick(storage1);

      // Lets update it in XChange.
      XGlobal->putIRC_Nick(storage1);
      XGlobal->putIRC_ProposedNick(storage1);
      delete [] storage1;

      // Lets update the config file too.
      writeIRCConfigFile();
   }
   else {
      // Lets update it in XChange.
      XGlobal->putIRC_Nick((char *) value);
      XGlobal->putIRC_ProposedNick((char *) value);
   }
   // Lets get the Nick Password.
   value = S.readStringEntry(CONFIG_IRC_SECTION, CONFIG_IRC_NICK_PASSWORD);
   if (value != NULL) {
      // Lets update it in XChange.
      XGlobal->putIRC_Password((char *) value);
   }
   // Lets get the Tray Password.
   value = S.readStringEntry(CONFIG_IRC_SECTION, CONFIG_IRC_TRAY_PASSWORD);
   if (value != NULL) {
      XGlobal->lock();
      delete [] XGlobal->TrayPassword;
      XGlobal->TrayPassword = new char[strlen(value) + 1];
      strcpy(XGlobal->TrayPassword, value);
      XGlobal->unlock();
   }

   // Lets work on the Connection Method.
   value = S.readStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_HOW);
   host = S.readStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_HOST);
   port = S.readUnsignedEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_PORT);
   user = S.readStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_USER);
   password = S.readStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_PASSWORD);
   vhost = S.readStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_VHOST);
   if (port == 0) port = 8080;
   if (host == NULL) host = empty;
   if (user == NULL) user = empty;
   if (password == NULL) password = empty;
   if (vhost == NULL) vhost = empty;

   if (value && strcasecmp(value, "BNC") == 0) {
      // need host, port, user, password, vhost.
      CM.setBNC(host, port, user, password, vhost);
   }
   else if (value && strcasecmp(value, "WINGATE") == 0) {
      // need host, port.
      CM.setWingate(host, port);
   }
   else if (value && strcasecmp(value, "SOCKS4") == 0) {
      // need host, port, user.
      CM.setSocks4(host, port, user);
   }
   else if (value && strcasecmp(value, "SOCKS5") == 0) {
      // need host, port, user, password.
      CM.setSocks5(host, port, user, password);
   }
   else if (value && strcasecmp(value, "PROXY") == 0) {
      // need host, port, user, password.
      CM.setProxy(host, port, user, password);
   }
   else {
      // Default it to a DIRECT Connection.
      CM.setDirect();
   }

   // Lets update it in XChange.
   XGlobal->putIRC_CM(CM);

   if (value == NULL) {
      // Lets update the config file too.
      writeConnectionConfigFile();
   }

   // Now lets read in the additional Channel values.
   // FromUIThr, has already set the 3 Channels before calling this function
   CL = XGlobal->getIRC_CL();
   XGlobal->resetIRC_CL_Changed();

   char *Channel = new char[strlen(CONFIG_IRC_CHANNEL) + 4];
   for (int i = 0; ; i++) {
      LineParse LineP;
      const char *parseptr;
      char *tempstr;

      sprintf(Channel, "%s%d", CONFIG_IRC_CHANNEL, i);
      value = S.readStringEntry(CONFIG_IRC_CHANNEL_SECTION, Channel);
      if ( (value == NULL) || (strlen(value) == 0) ) break;

      LineP = (char *) value;
      parseptr = LineP.getWord(1);
      tempstr = new char[strlen(parseptr) + 1];
      strcpy(tempstr, parseptr);

      parseptr = LineP.getWord(2); // possible key.
      CL.addChannel(tempstr, (char *) parseptr);
      delete [] tempstr;
   }
   delete [] Channel;
   // Lets put the CL we have obtained.
   XGlobal->putIRC_CL(CL);

   // Now lets read in the CAP variables.
   XGlobal->lock();
   XGlobal->OverallMaxUploadBPS = S.readUnsignedEntry(CONFIG_CAP_SECTION, CONFIG_CAP_UPLOAD_OVERALL_MAX);
   XGlobal->OverallMinUploadBPS = S.readUnsignedEntry(CONFIG_CAP_SECTION, CONFIG_CAP_UPLOAD_OVERALL_MIN);
   XGlobal->PerTransferMaxUploadBPS = S.readUnsignedEntry(CONFIG_CAP_SECTION, CONFIG_CAP_UPLOAD_EACH_MAX);
   XGlobal->OverallMaxDownloadBPS = S.readUnsignedEntry(CONFIG_CAP_SECTION, CONFIG_CAP_DOWNLOAD_OVERALL_MAX);
   XGlobal->PerTransferMaxDownloadBPS = S.readUnsignedEntry(CONFIG_CAP_SECTION, CONFIG_CAP_DOWNLOAD_EACH_MAX);
   // Now we sanitize the values.
   if ( (XGlobal->OverallMaxUploadBPS != 0) && (XGlobal->OverallMaxUploadBPS < XGlobal->PerTransferMaxUploadBPS) ) {
      XGlobal->PerTransferMaxUploadBPS = XGlobal->OverallMaxUploadBPS;
   }
   if ( (XGlobal->OverallMaxDownloadBPS != 0) && (XGlobal->OverallMaxDownloadBPS < XGlobal->PerTransferMaxDownloadBPS) ) {
      XGlobal->PerTransferMaxDownloadBPS = XGlobal->OverallMaxDownloadBPS;
   }
   XGlobal->unlock();

   // Now lets read in the FServ* variables.
   XGlobal->lock();
   XGlobal->FServQueuesOverall = S.readUnsignedEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_QUEUES_OVERALL);
   XGlobal->FServQueuesUser = S.readUnsignedEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_QUEUES_USER);
   XGlobal->FServSendsOverall = S.readUnsignedEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_SENDS_OVERALL);
   XGlobal->FServSendsUser = S.readUnsignedEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_SENDS_USER);
   XGlobal->FServSmallFileSize = S.readUnsignedEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_SMALL_FILE_SIZE);
   COUT(cout << "FServ values: Queues: Overall: " << XGlobal->FServQueuesOverall <<  " QueuesUser: " << XGlobal->FServQueuesUser << " Sends: Overall: " << XGlobal->FServSendsOverall << " SendsUser: " << XGlobal->FServSendsUser << " SmallFileSize: " << XGlobal->FServSmallFileSize << endl;)
   // Now we sanitize the values.
   // Absolute values sanity check.
   if (XGlobal->FServQueuesOverall < FSERV_MIN_QUEUES_OVERALL) {
      XGlobal->FServQueuesOverall = FSERV_MIN_QUEUES_OVERALL;
   }
   if (XGlobal->FServQueuesUser < FSERV_MIN_QUEUES_USER) {
      XGlobal->FServQueuesUser = FSERV_MIN_QUEUES_USER;
   }
   if (XGlobal->FServSendsOverall < FSERV_MIN_SENDS_OVERALL) {
      XGlobal->FServSendsOverall = FSERV_MIN_SENDS_OVERALL;
   }
   if (XGlobal->FServSendsUser < FSERV_MIN_SENDS_USER) {
      XGlobal->FServSendsUser = FSERV_MIN_SENDS_USER;
   }
   // Relative values sanity check.
   // overallsends > usersends, else usersends = overallsends - 1.
   // userq >= usersends, else userq = usersends.
   // overallq > userq, else userq = overallq - 1.
   if (XGlobal->FServSendsOverall <= XGlobal->FServSendsUser) {
      XGlobal->FServSendsUser = XGlobal->FServSendsOverall - 1;
   }
   if (XGlobal->FServQueuesUser < XGlobal->FServSendsUser) {
      XGlobal->FServQueuesUser = XGlobal->FServSendsUser;
   }
   if (XGlobal->FServQueuesOverall <= XGlobal->FServQueuesUser) {
      XGlobal->FServQueuesUser = XGlobal->FServQueuesOverall - 1;
   }

   if ( (XGlobal->FServSmallFileSize <= 0) || 
        (XGlobal->FServSmallFileSize > FSERV_SMALL_FILE_MAX_SIZE) ) {
      XGlobal->FServSmallFileSize = FSERV_SMALL_FILE_DEFAULT_SIZE;
   }
   MyMaxSmallFileSize = XGlobal->FServSmallFileSize;
   XGlobal->unlock();

   // Now lets read in the RecordDownloadCPS, RecordUploadCPS
   XGlobal->lock();
   XGlobal->RecordDownloadBPS = S.readRealEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_RECORD_DOWNLD_BPS);
   XGlobal->RecordUploadBPS = S.readRealEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_RECORD_UPLOAD_BPS);

   // The Record Upload/Download should be reset on restart.
   XGlobal->RecordDownloadBPS = 0.0;
   XGlobal->RecordUploadBPS = 0.0;

   XGlobal->unlock();
   // Now that Records are initialised, lets sanitize the caps.
   // This call inside lock XGlobal again.
   updateRecordDownloadAndAdjustUploadCaps(0, true);

   XGlobal->lock();
   // Now lets read in the TotalBytesRcvd, TotalBytesSent
   XGlobal->TotalBytesRcvd = S.readRealEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_TOTAL_BYTES_RCVD);
   XGlobal->TotalBytesSent = S.readRealEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_TOTAL_BYTES_SENT);

   // Now lets read in the PartialDir, ServingDir
   DIR *dir;
   value = S.readStringEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_PARTIAL_DIR);
   dir = NULL;
   if (value) dir = opendir(value);
   delete [] XGlobal->PartialDir;
   if (dir == NULL) {
      XGlobal->PartialDir = new char[8];
      strcpy(XGlobal->PartialDir, "Partial");
   }
   else {
      XGlobal->PartialDir = new char[strlen(value) + 1];
      strcpy(XGlobal->PartialDir, value);
      closedir(dir);
   }
   // Now lets read in the Serving Directories.
   char *ServingDirStr = new char[64];
   for (int i = 0; i < FSERV_MAX_SERVING_DIRECTORIES; i++) {
      sprintf(ServingDirStr, "%s%d", CONFIG_FSERV_SERVING_DIR, i);
      value = S.readStringEntry(CONFIG_FSERV_SECTION, ServingDirStr);
      dir = NULL;
      if (value) dir = opendir(value);
      delete [] XGlobal->ServingDir[i];
      XGlobal->ServingDir[i] = NULL;
      if (dir) {
         XGlobal->ServingDir[i] = new char[strlen(value) + 1];
         strcpy(XGlobal->ServingDir[i], value);
         closedir(dir);
      }
      else if (i == 0) {
         // ServingDir[0] should always exist.
         XGlobal->ServingDir[i] = new char[9];
         strcpy(XGlobal->ServingDir[i], "Serving");
      }
      // break on the first NULL entry, to avoid discontinuity in index.
      // We break so late, as index 0 is compulsory.
      if (value == NULL) break;
   }
   delete [] ServingDirStr;

   XGlobal->unlock();

   // Populate MyFilesDB.
   generateMyFilesDB();

   // Populate MyPartialFilesDB
   generateMyPartialFilesDB();

   // Lets now populate the Queues.
   // Should be populated only after MyFilesDB has a valid list.
   char *Queue = new char[strlen(CONFIG_FSERV_QUEUE) + 4];
   int smallqindex = 1;
   int bigqindex = 1;
   for (int i = 1; i <= FSERV_MAX_QUEUES_OVERALL; i++) {
   FilesDetail *FD;
   LineParse LineP;
   const char *parseptr;
   char store_nick[64], store_ip[32];
   size_t resume_pos;

      sprintf(Queue, "%s%d", CONFIG_FSERV_QUEUE, i);
      value = S.readStringEntry(CONFIG_FSERV_SECTION, Queue);
      if (value == NULL) continue; // Just loop throught the gaps.

      // Need to add this entry in QueuesInProgress.
      // Entry is of form: Nick DottedIP File Name ... avi
      LineP = (char *) value;
      parseptr = LineP.getWord(1);
      strcpy(store_nick, parseptr);
      parseptr = LineP.getWord(2);
      strcpy(store_ip, parseptr);
      parseptr = LineP.getWord(3);
      resume_pos = strtoul(parseptr, NULL, 10);
      parseptr = LineP.getWordRange(4, 0);

      // Now we are ready to populate FD and add to QueuesInProgress
      FD = XGlobal->MyFilesDB.getFilesDetailListMatchingFileName((char *) parseptr);
      if (FD == NULL) {
         FD = XGlobal->MyPartialFilesDB.getFilesDetailListMatchingFileName((char *) parseptr);
      }
      if (FD == NULL) continue;

      delete [] FD->Nick;
      FD->Nick = new char[strlen(store_nick) + 1];
      strcpy(FD->Nick, store_nick);
      delete [] FD->DottedIP;
      FD->DottedIP = new char[strlen(store_ip) + 1];
      strcpy(FD->DottedIP, store_ip);
      FD->FileResumePosition = resume_pos;
      // We now queue this guy in the correct Q.
      if ( (FD->FileSize <= MyMaxSmallFileSize) ||
           (FD->FileSize <= (resume_pos + MyMaxSmallFileSize)) ) {
         XGlobal->SmallQueuesInProgress.addFilesDetailAtIndex(FD, smallqindex);
         smallqindex++;
         COUT(XGlobal->SmallQueuesInProgress.printDebug(NULL));
      }
      else {
         XGlobal->QueuesInProgress.addFilesDetailAtIndex(FD, bigqindex);
         bigqindex++;
         COUT(XGlobal->QueuesInProgress.printDebug(NULL));
      }
   }
   delete [] Queue;

   // Read the FONT section.
   value = S.readStringEntry(CONFIG_FONT_SECTION, CONFIG_FONT_FACE);
   XGlobal->lock();
   delete [] XGlobal->FontFace;
   if (value) {
      XGlobal->FontFace = new char[strlen(value) + 1];
      strcpy(XGlobal->FontFace, value);
   }
   else {
#ifdef __MINGW32__
      XGlobal->FontFace = new char[9];
      strcpy(XGlobal->FontFace, "Fixedsys");
#else
      XGlobal->FontFace = new char[13];
      strcpy(XGlobal->FontFace, "fixed [misc]");
#endif
   }
   XGlobal->unlock();

   fontsize = S.readUnsignedEntry(CONFIG_FONT_SECTION, CONFIG_FONT_SIZE);
   if (fontsize == 0) {
      fontsize = 105;
   }
   XGlobal->lock();
   XGlobal->FontSize = fontsize;
   XGlobal->unlock();

   // Read in the PARTIAL section.
   char *Partial = new char[strlen(CONFIG_PARTIAL_PARTIAL) + 4];
   for (int i = 1; i <= CONFIG_PARTIAL_MAXPARTIALS; i++) {
   FilesDetail *FD;
   LineParse LineP;
   const char *parseptr;

      sprintf(Partial, "%s%d", CONFIG_PARTIAL_PARTIAL, i);
      value = S.readStringEntry(CONFIG_PARTIAL_SECTION, Partial);
      if (value == NULL) break;

      // Need to add this entry in DwnldInProgress
      LineP = (char *) value;
      LineP.setDeLimiter('*');
      // fields are: 
      // FileName, File Size, Cur Size, Nick, Dir Name
      // So lets get the FD ready.
      FD = new FilesDetail;
      XGlobal->FilesDB.initFilesDetail(FD);

      parseptr = LineP.getWord(1);
      FD->FileName = new char[strlen(parseptr) + 1];
      strcpy(FD->FileName, parseptr);

      parseptr = LineP.getWord(2);
      FD->FileSize = strtoul(parseptr, NULL, 10);

      parseptr = LineP.getWord(3);
      FD->FileResumePosition = strtoul(parseptr, NULL, 10);

      parseptr = LineP.getWord(4);
      FD->Nick = new char[strlen(parseptr) + 1];
      strcpy(FD->Nick, parseptr);

      parseptr = LineP.getWord(5);
      // Space => NULL.
      if (strcasecmp(parseptr, " ")) {
         FD->DirName = new char[strlen(parseptr) + 1];
         strcpy(FD->DirName, parseptr);
      }

      // Mark it as a PARTIAL Download.
      FD->DownloadState = DOWNLOADSTATE_PARTIAL;

      // Now we are ready to add FD to DwnldInProgress
      XGlobal->DwnldInProgress.addFilesDetailAtIndex(FD, i);
      FD = NULL;
   }
   delete [] Partial;

   // Read in the WAITING section.
   char *Waiting = new char[strlen(CONFIG_WAITING_WAITING) + 4];
   for (int i = 1; i <= CONFIG_WAITING_MAXWAITINGS; i++) {
   FilesDetail *FD;
   LineParse LineP;
   const char *parseptr;

      sprintf(Waiting, "%s%d", CONFIG_WAITING_WAITING, i);
      value = S.readStringEntry(CONFIG_WAITING_SECTION, Waiting);
      if (value == NULL) break;

      // Need to add this entry in DwnldWaiting
      LineP = (char *) value;
      LineP.setDeLimiter('*');
      // fields are:
      // FileName, File Size, Nick, Dir Name
      // So lets get the FD ready.
      FD = new FilesDetail;
      XGlobal->FilesDB.initFilesDetail(FD);

      parseptr = LineP.getWord(1);
      FD->FileName = new char[strlen(parseptr) + 1];
      strcpy(FD->FileName, parseptr);

      parseptr = LineP.getWord(2);
      FD->FileSize = strtoul(parseptr, NULL, 10);

      parseptr = LineP.getWord(3);
      FD->Nick = new char[strlen(parseptr) + 1];
      strcpy(FD->Nick, parseptr);

      parseptr = LineP.getWord(4);
      // Space => NULL.
      if (strcasecmp(parseptr, " ")) {
         FD->DirName = new char[strlen(parseptr) + 1];
         strcpy(FD->DirName, parseptr);
      }

      // Now we are ready to add FD to DwnldInProgress
      XGlobal->DwnldWaiting.addFilesDetailAtIndex(FD, i);
      FD = NULL;
   }
   delete [] Waiting;
}

// Below is called only by generateMyFilesDB for the recursive dir listing
// extraction. Hence its private.
void Helper::recurGenerateMyFilesDB(char *nick, char *basedir, char *dir, int ServingDirIndex) {
DIR *d, *tempd;
struct dirent *fdir;
char dirname[PATH_MAX];
char filename[2*PATH_MAX + 1];
FilesDetail *FD;
struct stat s;

   TRACE();

   d = opendir(dir);
   if (!d) return;

   while (fdir = readdir(d)) {
      // We skip over the . and the .. entries.
      if (strcmp(".", fdir->d_name) == 0) continue;
      if (strcmp("..",fdir->d_name) == 0) continue;

      // Generate the new directory name.
      // Potential overflow of tempstr, if exceeding MAXPATHLEN.
      sprintf(dirname, "%s"DIR_SEP"%s", dir, fdir->d_name);
      tempd = opendir(dirname);
      if (tempd) {
         // We recurse this directory.
         closedir(tempd);
         recurGenerateMyFilesDB(nick, basedir, dirname, ServingDirIndex);
      }
      else {
         // This is a file, so we note it down.

         // First lets check if we have hit the max files that we can store.
         if (CountFilesInMyFilesDB >= MAX_FSERV_FILENAMES) {
            closedir(d);
            return;
         }

         // Skip files starting with "."
         if (fdir->d_name[0] == '.') continue;

         // Skip the Thumbs.db file.
         if (strcasecmp("Thumbs.db", fdir->d_name) == 0) continue;

         // Skip the files which have .exe in them
         if (strcasestr(fdir->d_name, ".exe")) continue;

         CountFilesInMyFilesDB++;

         sprintf(filename, "%s"DIR_SEP"%s", dir, fdir->d_name);
         stat(filename, &s);
         // Skip 0 byte files.
         if (s.st_size == 0) continue;

         FD = new FilesDetail;
         XGlobal->MyFilesDB.initFilesDetail(FD);
         FD->Nick = new char[strlen(nick) + 1];
         strcpy(FD->Nick, nick);

         // We keep filenames without ServingDir
         FD->FileName = new char[strlen(fdir->d_name) + 1];
         strcpy(FD->FileName, fdir->d_name);

         // Get DirName relative to Serving Dir.
         // Note that dir includes the Serving Dir.
         if (strlen(dir) > strlen(basedir)) {
            FD->DirName = new char[strlen(dir) + 1 - strlen(basedir)];
            strcpy(FD->DirName, &dir[strlen(basedir)]);
#ifndef __MINGW32__
            // This is where we convert the DIR_SEP in DirName to FS_DIR_SEP
            convertToFSDIRSEP(FD->DirName);
#endif
         }
         // Else its NULL neway.

         // Note down the ServingDirIndex for this file.
         FD->ServingDirIndex = ServingDirIndex;

         FD->FileSize = s.st_size;
         FD->DownloadState = DOWNLOADSTATE_SERVING; // Full file and not partial.
         XGlobal->MyFilesDB.addFilesDetail(FD);

         // Do not generate the meta file for MasalaMate*
         if (strncasecmp("MasalaMate", fdir->d_name, 10) == 0) continue;

         // Lets check and or generate the meta files.
         // S.initFile(dir, fdir->d_name);
      }
   }
   closedir(d);
}

// This call to be serialised as many threads might call it and run
// over each other.
// Using the FFLC lock is a bad idea, as then it almost never gets a lock.
// We can have a seperate lock for just MyFiles and MyPartialFiles, which 
// can be used by FFLC when only transferring MyFilesDB and MyPartialFilesDB
// Now that we have remove lockFFLC from most of FFLC except when adding
// the collected FileDB of a particular nick, we should use it here.
void Helper::generateMyPartialFilesDB() {
char Nick[64];
char *PartialDir;
DIR *d, *tempd;
struct dirent *fdir;
char filename[2*PATH_MAX + 1];
FilesDetail *FD;
struct stat s;

   // Serialise
   XGlobal->lockFFLC();
   COUT(cout << "Helper::generateMyPartialFilesDB: lockFFLC()" << endl;)

   TRACE();

   // Lets get our nick.
   XGlobal->getIRC_Nick(Nick);
   XGlobal->resetIRC_Nick_Changed();

   // Lets purge what we are already holding.
   XGlobal->MyPartialFilesDB.purge();
   XGlobal->lock();
   PartialDir = new char[strlen(XGlobal->PartialDir) + 1];
   strcpy(PartialDir, XGlobal->PartialDir);
   XGlobal->unlock();

   d = opendir(PartialDir);
   if (!d) {
      COUT(cout << "Helper::generateMyPartialFilesDB: unlockFFLC()" << endl;)
      XGlobal->unlockFFLC();
      delete [] PartialDir;
      return;
   }

   while (fdir = readdir(d)) {
      // We skip over the . and the .. entries.
      if (strcmp(".", fdir->d_name) == 0) continue;
      if (strcmp("..",fdir->d_name) == 0) continue;

      // Skip files starting with "."
      if (fdir->d_name[0] == '.') continue;

      // Skip the Thumbs.db file.
      if (strcasecmp("Thumbs.db", fdir->d_name) == 0) continue;

      sprintf(filename, "%s"DIR_SEP"%s", PartialDir, fdir->d_name);
      tempd = opendir(filename);
      if (tempd) {
         // Skip the directories.
         closedir(tempd);
         continue;
      }

      // Skip the Thumbs.db file.
      if (strcasecmp("Thumbs.db", fdir->d_name) == 0) continue;

      // Skip the files with .exe in them.
      if (strcasestr(fdir->d_name, ".exe")) continue;

      // All Files to be noted. If File size = 0, make it 1.
      stat(filename, &s);
      if (s.st_size == 0) {
         s.st_size = 1;
      }

      FD = new FilesDetail;
      XGlobal->MyPartialFilesDB.initFilesDetail(FD);
      FD->Nick = new char[strlen(Nick) + 1];
      strcpy(FD->Nick, Nick);

      FD->FileSize = s.st_size;

      // We keep filenames without PartialDir
      FD->FileName = new char[strlen(fdir->d_name) + 1];
      FD->DownloadState = DOWNLOADSTATE_PARTIAL; // Marked as a partial file.
      strcpy(FD->FileName, fdir->d_name);

      XGlobal->MyPartialFilesDB.addFilesDetail(FD);
      COUT(cout << "generateMyPartialFilesDB: " << FD->FileName << endl;)
   }

   delete [] PartialDir;
   closedir(d);

   COUT(XGlobal->MyPartialFilesDB.printDebug(NULL);)

   COUT(cout << "Helper::generateMyPartialFilesDB: unlockFFLC()" << endl;)
   XGlobal->unlockFFLC();
}

// This call to be serialised as many threads might call it and run
// over each other.
// Using the FFLC lock is a bad idea, as then it almost never gets a lock.
// We can have a seperate lock for just MyFiles and MyPartialFiles, which 
// can be used by FFLC when only transferring MyFilesDB and MyPartialFilesDB
// Now that we have removed lockFFLC from most of FFLC except when adding
// the FD, after accumulation, we should use it now.
void Helper::generateMyFilesDB() {
char Nick[64];
char *ServDir;

   // Serialise.
   XGlobal->lockFFLC();
   COUT(cout << "Helper::generateMyFilesDB: lockFFLC()" << endl;)

   TRACE();

   // Lets get our nick.
   XGlobal->getIRC_Nick(Nick);
   XGlobal->resetIRC_Nick_Changed();

   // Lets purge what we are already holding.
   XGlobal->MyFilesDB.purge();
   // update the count to the purge.
   CountFilesInMyFilesDB = 0;

   for (int i = 0; i < FSERV_MAX_SERVING_DIRECTORIES; i++) {
      XGlobal->lock();
      if (XGlobal->ServingDir[i] == NULL) {
         XGlobal->unlock();
         break;
      }
      ServDir = new char[strlen(XGlobal->ServingDir[i]) + 1];
      strcpy(ServDir, XGlobal->ServingDir[i]);
      XGlobal->unlock();

      recurGenerateMyFilesDB(Nick, ServDir, ServDir, i);
      delete [] ServDir;
   }

   COUT(XGlobal->MyFilesDB.printDebug(NULL);)

   COUT(cout << "Helper::generateMyFilesDB: unlockFFLC()" << endl;)
   XGlobal->unlockFFLC();
}

void Helper::writeIRCConfigFile() {
FXSettings S;
char Nick[64];
char Password[64];
char *TP;

   TRACE();

   S.parseFile(CONFIG_FILE, true);

   // Lets get our Nick
   XGlobal->getIRC_Nick(Nick);
   XGlobal->getIRC_Password(Password);
   XGlobal->resetIRC_Nick_Changed();
   S.writeStringEntry(CONFIG_IRC_SECTION, CONFIG_IRC_NICK, Nick);
   S.writeStringEntry(CONFIG_IRC_SECTION, CONFIG_IRC_NICK_PASSWORD, Password);

   XGlobal->lock();
   TP = XGlobal->TrayPassword;
   if (TP == NULL) {
      TP = "";
   }
   S.writeStringEntry(CONFIG_IRC_SECTION, CONFIG_IRC_TRAY_PASSWORD, TP);
   XGlobal->unlock();

   S.unparseFile(CONFIG_FILE);
}

void Helper::writeFontConfigFile() {
FXSettings S;

   TRACE();

   S.parseFile(CONFIG_FILE, true);

   XGlobal->lock();
   S.writeStringEntry(CONFIG_FONT_SECTION, CONFIG_FONT_FACE, XGlobal->FontFace);
   S.writeUnsignedEntry(CONFIG_FONT_SECTION, CONFIG_FONT_SIZE, XGlobal->FontSize);
   XGlobal->unlock();

   S.unparseFile(CONFIG_FILE);
}

// Updates Config file with the FServ* sends/queues information.
void Helper::writeFServParamsConfig() {
FXSettings S;

   TRACE();

   S.parseFile(CONFIG_FILE, true);

   XGlobal->lock();

   S.writeUnsignedEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_SENDS_OVERALL, XGlobal->FServSendsOverall);
   S.writeUnsignedEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_QUEUES_OVERALL, XGlobal->FServQueuesOverall);
   S.writeUnsignedEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_SENDS_USER, XGlobal->FServSendsUser);
   S.writeUnsignedEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_QUEUES_USER, XGlobal->FServQueuesUser);
   S.writeUnsignedEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_SMALL_FILE_SIZE, XGlobal->FServSmallFileSize);

   XGlobal->unlock();

   S.unparseFile(CONFIG_FILE);
}

// Updates Config file with the CAP Bandwidth information.
void Helper::writeCapConfig() {
FXSettings S;

   TRACE();

   S.parseFile(CONFIG_FILE, true);

   XGlobal->lock();
   S.writeUnsignedEntry(CONFIG_CAP_SECTION, CONFIG_CAP_UPLOAD_OVERALL_MAX, XGlobal->OverallMaxUploadBPS);
   S.writeUnsignedEntry(CONFIG_CAP_SECTION, CONFIG_CAP_UPLOAD_OVERALL_MIN, XGlobal->OverallMinUploadBPS);
   S.writeUnsignedEntry(CONFIG_CAP_SECTION, CONFIG_CAP_UPLOAD_EACH_MAX, XGlobal->PerTransferMaxUploadBPS);
   S.writeUnsignedEntry(CONFIG_CAP_SECTION, CONFIG_CAP_DOWNLOAD_OVERALL_MAX, XGlobal->OverallMaxDownloadBPS);
   S.writeUnsignedEntry(CONFIG_CAP_SECTION, CONFIG_CAP_DOWNLOAD_EACH_MAX, XGlobal->PerTransferMaxDownloadBPS);
   XGlobal->unlock();

   S.unparseFile(CONFIG_FILE);
}


void Helper::writeConnectionConfigFile() {
FXSettings S;
ConnectionMethod CM;
const char *value;
char *empty = "";
unsigned int port;

   TRACE();

   S.parseFile(CONFIG_FILE, true);

   // Lets get the Connection Method
   CM = XGlobal->getIRC_CM();
   XGlobal->resetIRC_CM_Changed();

   switch (CM.howto()) {
     case CM_BNC:
       S.writeStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_HOW, "BNC");
       break;

     case CM_WINGATE:
       S.writeStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_HOW, "WINGATE");
       break;

     case CM_SOCKS4:
       S.writeStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_HOW, "SOCKS4");
       break;

     case CM_SOCKS5:
       S.writeStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_HOW, "SOCKS5");
       break;

     case CM_PROXY:
       S.writeStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_HOW, "PROXY");
       break;

     case CM_DIRECT:
     default:
       S.writeStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_HOW, "DIRECT");
       break;
   }
   value = CM.getHost();
   if (value == NULL) value = empty;
   S.writeStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_HOST, value);
   port = CM.getPort();
   if (port == 0) port = 8080;
   S.writeUnsignedEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_PORT, port);
   value = CM.getUser();
   if (value == NULL) value = empty;
   S.writeStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_USER, value);
   value = CM.getPassword();
   if (value == NULL) value = empty;
   S.writeStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_PASSWORD, value);
   value = CM.getVhost();
   if (value == NULL) value = empty;
   S.writeStringEntry(CONFIG_CONNECTION_SECTION, CONFIG_CONNECTION_VHOST, value);

   S.unparseFile(CONFIG_FILE);
}

void Helper::writeIRCChannelConfig() {
FXSettings S;
char *Channel = new char[strlen(CONFIG_IRC_CHANNEL) + 4];
IRCChannelList CL;
int ChannelCount, i, cur_index;
const char *channel_name;
const char *channel_key;
char *buffer = new char[256];

   TRACE();

   S.parseFile(CONFIG_FILE, true);

   // Delete the whole CONFIG_IRC_CHANNEL_SECTION first.
   S.deleteSection(CONFIG_IRC_CHANNEL_SECTION);

   // Get the Channel List first.
   CL = XGlobal->getIRC_CL();
   XGlobal->resetIRC_CL_Changed();
   ChannelCount = CL.getChannelCount();

   cur_index = 0;
   for (i = 1; i <= ChannelCount; i++) {
      channel_name = CL.getChannel(i);
      if (strcasecmp(channel_name, CHANNEL_MAIN) == 0) continue;
      if (strcasecmp(channel_name, CHANNEL_CHAT) == 0) continue;
      if (strcasecmp(channel_name, CHANNEL_MM) == 0) continue;

      // These ones need to be written down.
      sprintf(Channel, "%s%d", CONFIG_IRC_CHANNEL, cur_index);
      cur_index++;

      channel_key = CL.getChannelKey(i);
      if (channel_key && strlen(channel_key) ) {
         sprintf(buffer, "%s %s", channel_name, channel_key);
      }
      else {
         sprintf(buffer, "%s", channel_name);
      }
      S.writeStringEntry(CONFIG_IRC_CHANNEL_SECTION, Channel, buffer);
   }
   delete [] Channel;
   delete [] buffer;

   S.unparseFile(CONFIG_FILE);
}

void Helper::writeWaitingConfig() {
FXSettings S;
char *Waiting = new char[strlen(CONFIG_WAITING_WAITING) + 4];
FilesDetail *FD, *ScanFD;
char *buffer;
int buffer_len;
int i;
char *DirName;
char EmptyDir[2];

   TRACE();

   S.parseFile(CONFIG_FILE, true);

   // Delete the whole CONFIG_WAITING_SECTION first.
   S.deleteSection(CONFIG_WAITING_SECTION);

   // Lets now populate the Waitings from DwnldWaiting
   i = 0;

   FD = XGlobal->DwnldWaiting.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD) {
      i++;
      sprintf(Waiting, "%s%d", CONFIG_WAITING_WAITING, i);
      buffer_len = strlen(ScanFD->Nick) + strlen(ScanFD->FileName) + 32;
      if (ScanFD->DirName) {
         buffer_len += strlen(ScanFD->DirName);
         DirName = ScanFD->DirName;
      }
      else {
         strcpy(EmptyDir, " ");
         DirName = EmptyDir;
      }
      buffer = new char[buffer_len];

      sprintf(buffer, "%s*%lu*%s*%s", ScanFD->FileName, ScanFD->FileSize, ScanFD->Nick, DirName);
      S.writeStringEntry(CONFIG_WAITING_SECTION, Waiting, buffer);
      delete [] buffer;
      ScanFD = ScanFD->Next;
   }
   XGlobal->DwnldWaiting.freeFilesDetailList(FD);

   delete [] Waiting;

   S.unparseFile(CONFIG_FILE);
}

void Helper::writePartialConfig() {
FXSettings S;
char *Partial = new char[strlen(CONFIG_PARTIAL_PARTIAL) + 4];
FilesDetail *FD, *ScanFD;
char *buffer;
int buffer_len;
int i;
char *DirName;
char EmptyDir[2];
size_t cur_size;

   TRACE();

   S.parseFile(CONFIG_FILE, true);

   // Delete the whole CONFIG_PARTIAL_SECTION first.
   S.deleteSection(CONFIG_PARTIAL_SECTION);

   // Lets now populate the Partial/InProgress Downloads.
   i = 0;

   FD = XGlobal->DwnldInProgress.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD) {
      if ( (ScanFD->DownloadState == DOWNLOADSTATE_SERVING) && (ScanFD->Connection == NULL) ) {
         // This is a complete download, skip over this one.
         // We can have entries as 'S' for current downloads in progress.
         ScanFD = ScanFD->Next;
         continue;
      }
      i++;
      sprintf(Partial, "%s%d", CONFIG_PARTIAL_PARTIAL, i);
      buffer_len = strlen(ScanFD->Nick) + strlen(ScanFD->FileName) + 32;
      if (ScanFD->DirName) {
         buffer_len += strlen(ScanFD->DirName);
         DirName = ScanFD->DirName;
      }
      else {
         strcpy(EmptyDir, " ");
         DirName = EmptyDir;
      }
      buffer = new char[buffer_len];

      cur_size = ScanFD->FileResumePosition + ScanFD->BytesReceived;
 
      sprintf(buffer, "%s*%lu*%lu*%s*%s", ScanFD->FileName, ScanFD->FileSize, cur_size, ScanFD->Nick, DirName);
      S.writeStringEntry(CONFIG_PARTIAL_SECTION, Partial, buffer);
      delete [] buffer;
      ScanFD = ScanFD->Next;
   }
   XGlobal->DwnldInProgress.freeFilesDetailList(FD);

   delete [] Partial;

   S.unparseFile(CONFIG_FILE);
}

void Helper::writeFServeConfig() {
FXSettings S;
char *Queue = new char[strlen(CONFIG_FSERV_QUEUE) + 4];
FilesDetail *FD, *ScanFD;
char *buffer;
int i;

   TRACE();

   S.parseFile(CONFIG_FILE, true);

   // Delete the whole CONFIG_FSERV_SECTION first.
   S.deleteSection(CONFIG_FSERV_SECTION);

   // Lets now populate the Sends
   i = 0;

   FD = XGlobal->SendsInProgress.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD) {
      i++;
      sprintf(Queue, "%s%d", CONFIG_FSERV_QUEUE, i);
      buffer = new char[strlen(ScanFD->Nick) + strlen(ScanFD->DottedIP) + strlen(ScanFD->FileName) + 20];
      sprintf(buffer, "%s %s %lu %s", ScanFD->Nick, ScanFD->DottedIP, ScanFD->FileResumePosition + ScanFD->BytesReceived, ScanFD->FileName);
      S.writeStringEntry(CONFIG_FSERV_SECTION, Queue, buffer);
      delete [] buffer;
      ScanFD = ScanFD->Next;
   }
   XGlobal->SendsInProgress.freeFilesDetailList(FD);

   // Lets now populate the DCCSendWaiting.
   FD = XGlobal->DCCSendWaiting.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD) {
      i++;
      sprintf(Queue, "%s%d", CONFIG_FSERV_QUEUE, i);
      buffer = new char[strlen(ScanFD->Nick) + strlen(ScanFD->DottedIP) + strlen(ScanFD->FileName) + 20];
      sprintf(buffer, "%s %s %lu %s", ScanFD->Nick, ScanFD->DottedIP, ScanFD->FileResumePosition, ScanFD->FileName);
      S.writeStringEntry(CONFIG_FSERV_SECTION, Queue, buffer);
      delete [] buffer;
      ScanFD = ScanFD->Next;
   }
   XGlobal->DCCSendWaiting.freeFilesDetailList(FD);

   // Lets now populate the Queues.
   FD = XGlobal->QueuesInProgress.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD) {
      i++;
      sprintf(Queue, "%s%d", CONFIG_FSERV_QUEUE, i);
      buffer = new char[strlen(ScanFD->Nick) + strlen(ScanFD->DottedIP) + strlen(ScanFD->FileName) + 20];
      sprintf(buffer, "%s %s %lu %s", ScanFD->Nick, ScanFD->DottedIP, ScanFD->FileResumePosition, ScanFD->FileName);
      S.writeStringEntry(CONFIG_FSERV_SECTION, Queue, buffer);
      delete [] buffer;
      ScanFD = ScanFD->Next;
   }
   XGlobal->QueuesInProgress.freeFilesDetailList(FD);

   // Lets now populate the SmallQueues.
   FD = XGlobal->SmallQueuesInProgress.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD) {
      i++;
      sprintf(Queue, "%s%d", CONFIG_FSERV_QUEUE, i);
      buffer = new char[strlen(ScanFD->Nick) + strlen(ScanFD->DottedIP) + strlen(ScanFD->FileName) + 20];
      sprintf(buffer, "%s %s %lu %s", ScanFD->Nick, ScanFD->DottedIP, ScanFD->FileResumePosition, ScanFD->FileName);
      S.writeStringEntry(CONFIG_FSERV_SECTION, Queue, buffer);
      delete [] buffer;
      ScanFD = ScanFD->Next;
   }
   XGlobal->SmallQueuesInProgress.freeFilesDetailList(FD);

   delete [] Queue;

   // Lets write out RecordBPS and TotalBytesSent/Received.
   XGlobal->lock();
   S.writeRealEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_RECORD_DOWNLD_BPS, XGlobal->RecordDownloadBPS);
   S.writeRealEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_RECORD_UPLOAD_BPS, XGlobal->RecordUploadBPS);
   S.writeRealEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_TOTAL_BYTES_SENT, XGlobal->TotalBytesSent);
   S.writeRealEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_TOTAL_BYTES_RCVD, XGlobal->TotalBytesRcvd);
   S.writeStringEntry(CONFIG_FSERV_SECTION, CONFIG_FSERV_PARTIAL_DIR, XGlobal->PartialDir);
   buffer = new char[64];
   for (i = 0; i < FSERV_MAX_SERVING_DIRECTORIES; i++) {
      if (XGlobal->ServingDir[i] == NULL) break;
      sprintf(buffer, "%s%d", CONFIG_FSERV_SERVING_DIR, i);
      S.writeStringEntry(CONFIG_FSERV_SECTION, buffer, XGlobal->ServingDir[i]);
   }
   delete [] buffer;
  
   XGlobal->unlock();

   S.unparseFile(CONFIG_FILE);

   // Now write in the FServ Params data.
   writeFServParamsConfig();
}

// Moves a file from the PartialDir to ServingDir[0], if normal is true.
// Usually called after a successful download is complete.
// We only move if the destination file doesnt exist.
// if normal = false, this is a move from PartialDir to run position (upgrade)
bool Helper::moveFile(char *filename, bool normal) {
int fd1, fd2;
char *mybuf = NULL;
char *from_file, *to_file;
size_t filesize;
size_t current_index, delta;
struct stat mystat;
bool retvalb = false;

   TRACE();

   XGlobal->lock();
   from_file = new char[strlen(filename) + strlen(XGlobal->PartialDir) + strlen(DIR_SEP) + 1];
   sprintf(from_file, "%s%s%s", XGlobal->PartialDir, DIR_SEP, filename);
   XGlobal->unlock();

   if (normal) {
      XGlobal->lock();
      to_file = new char[strlen(filename) + strlen(XGlobal->ServingDir[0]) + strlen(DIR_SEP) + 1];
      sprintf(to_file, "%s%s%s", XGlobal->ServingDir[0], DIR_SEP, filename);
      XGlobal->unlock();
   }
   else {
      char *rename_file_from, *rename_file_to;

      // First  we delete the .bak file.
      // Second we rename the running executable.
#ifdef __MINGW32__
      rename_file_from = new char[strlen(PROGRAM_NAME_WINDOWS) + 1];
      rename_file_to = new char[strlen(PROGRAM_NAME_WINDOWS) + 5];
      strcpy(rename_file_from, PROGRAM_NAME_WINDOWS);
      sprintf(rename_file_to, "%s.bak", PROGRAM_NAME_WINDOWS);
#else
      rename_file_from = new char[strlen(PROGRAM_NAME_NON_WINDOWS) + 1];
      rename_file_to = new char[strlen(PROGRAM_NAME_NON_WINDOWS) + 5];
      strcpy(rename_file_from, PROGRAM_NAME_NON_WINDOWS);
      sprintf(rename_file_to, "%s.bak", PROGRAM_NAME_NON_WINDOWS);
#endif
      unlink(rename_file_to);

      if (rename(rename_file_from, rename_file_to) != 0) {
         delete [] rename_file_from;
         delete [] rename_file_to;
         delete [] from_file;
         return(false);
      }
      delete [] rename_file_from;
      delete [] rename_file_to;

      // Move to base folder. (Upgrade) (FileName is also changed to run file)
#ifdef __MINGW32__
      to_file = new char[strlen(PROGRAM_NAME_WINDOWS) + 1];
      strcpy(to_file, PROGRAM_NAME_WINDOWS);
#else
      to_file = new char[strlen(PROGRAM_NAME_NON_WINDOWS) + 1];
      strcpy(to_file, PROGRAM_NAME_NON_WINDOWS);
#endif
   }

#ifdef __MINGW32__
   // Try to use the easier OS MoveFile call.
   retvalb = MoveFile(from_file, to_file);
   delete [] to_file;
   delete [] from_file;
   return(retvalb);

#endif

   fd2 = open(to_file, O_RDONLY | O_BINARY);
   COUT(cout << "open: " << to_file << " returned: " << fd2 << endl;)

#ifndef __MINGW32__
   if (fd2 == -1) {
      // Destination file doesnt exist. Lets try the link/unlnk method
      // to move the file.
      // Change its mode to exe if upgrade.
      if (normal == false) {
         // This is an upgrade, change mode for executable in non windows port
         chmod(from_file, S_IRWXU | S_IRWXG);
      }

      if (link(from_file, to_file) != -1) {

         if (normal == false) {
            // This is an upgrade, change mode for executable, again
            // in non windows port
            chmod(to_file, S_IRWXU | S_IRWXG);
         }

         // Successfully linked. Just delete from_file.
         // Dont care if it fails to unlink.
         unlink(from_file);
         delete [] to_file;
         delete [] from_file;
         COUT(cout << "Helper::moveFile: Moved file using link method" << endl;)
         return(true);
      }
   }
#endif

   fd1 = open(from_file, O_RDONLY | O_BINARY);
   COUT(cout << "open: " << from_file << " returned: " << fd1 << endl;)

   if ( (fd2 != -1) || (fd1 == -1) ) {
      if (fd2 != -1) {
         close(fd2);
         COUT(cout << "close: " << fd2 << endl;)
      }
      delete [] to_file;
      delete [] from_file;
      return(retvalb);
   }

   if (fstat(fd1, &mystat) == -1) {
      close(fd1);
      COUT(cout << "close: " << fd1 << endl;)
      delete [] to_file;
      delete [] from_file;
      return(retvalb);
   }
   filesize = mystat.st_size;

   // We do an exclusive open on create here, as, if the file already exists
   // we dont want to do anything
   fd2 = open(to_file, O_CREAT | O_WRONLY | O_EXCL | O_BINARY, CREAT_PERMISSIONS);
   COUT(cout << "open: " << to_file << " returned: " << fd2 << endl;)

   if (fd2 == -1) {
      close(fd1);
      COUT(cout << "close: " << fd1 << endl;)
      delete [] from_file;
      delete [] to_file;
      return(retvalb);
   }

// we have fd1 and fd2 open and ready. filesize is set.

   current_index = 0;
   mybuf = new char[65536];
   retvalb = true;
   while (current_index < filesize) {
      delta = read(fd1, mybuf, 65536);
      if (delta < 0) {
         COUT(cout << "Helper::moveFile: Error in reading source file." << endl;)
         retvalb = false;
         break;
      }
      current_index += delta;
      if (write(fd2, mybuf, delta) != delta) {
         COUT(cout << "Helper::moveFile: Error in writing to file." << endl;)
         retvalb = false;
         break;
      }
   }
   delete [] mybuf;
   close(fd1);
   COUT(cout << "close: " << fd1 << endl;)
   close(fd2);
   COUT(cout << "close: " << fd2 << endl;)
   if (retvalb == false) {
      delete [] to_file;
      delete [] from_file;
      return(retvalb);
   }

   // File has been copied over sucessfully.
   unlink(from_file);
   delete [] from_file;

#ifndef __MINGW32__
   if (normal == false) {
      // This is an upgrade, change mode for executable, again
      // in non windows port
      chmod(to_file, S_IRWXU | S_IRWXG);
   }
#endif

   delete [] to_file;
   return(retvalb);
}

void Helper::sendLineToNick(char *nick, char *Line) {
IRCNickLists &NL = XGlobal->NickList;
int i, chan_count;
char ChannelName[128];

   TRACE();

   if ( (nick == NULL) || (Line == NULL) ) return;

   if ( (strlen(nick) == 0) || (strlen(Line) == 0) ) return;

   // Lets go thru the channels and see if he is any of our known channels.
   chan_count = NL.getChannelCount();
   for (i = 1; i <= chan_count; i++) {
      NL.getChannelName(ChannelName, i);
      if (NL.isNickInChannel(ChannelName, nick)) {
         XGlobal->IRC_ToServer.putLine(Line);
         COUT(cout << "Helper::sendLineToNick-> " << Line << endl;)
         break;
      }
   }
}

// Below is related to dcc sending a file.
// Sends the file as per FD which is in QueuesInProgress
// Returns the Thread handle.
// The File in Q, will have its DirName and FileName set.
// The variations are:
// if DownloadState == DOWNLOADSTATE_PARTIAL, base dir = PartialDir, 
//   and ignore DirName.
//   correct the file size, and we should be MyPartialFilesDB
//   Check if File exists, if not, inform user and regenerate our list.
// if DownloadState == DOWNLOADSTATE_SERVING, base dir = Serving Dir, 
//   and consider DirName and
//   FileName to get full filename, and we should be in MyFilesDB.
//   Check if File exists, if not, inform user and regenerate our list.
// if FD->ManualSend = MANUALSEND_DCCSEND we dont have to be serving the 
//   file, and DirName
//   contains fully qualified base dir. (these come in with DownloadState S)
// if FD->ManualSend = MANUALSEND_FILEPUSH these are S or P files, which 
//   are pushed.
// We do not free FD in the call. Our caller should free it.
// SmallQ true => SmallQueueInProgress, else QueueInProgress.
THR_HANDLE Helper::dccSend(FilesDetail *FD, bool SmallQ) {
TCPConnect *DCCSend;
bool retvalb;
THR_HANDLE TransferThrH = 0;
char Nick[64];
char Response[1024];
int retval;
char *fullq_filename = NULL;

   TRACE();

   if (XGlobal == NULL) {
      COUT(cout << "Helper Used without being initialised." << endl;)
      // Crash here.
      char *p = NULL; *p = '\0'; // Crash
      return(TransferThrH);
   }

   do {
      if (FD == NULL) break;

#ifndef __MINGW32__
      // The DirName is possibly in FS_DIR_SEP_CHAR. Convert it to DIR_SEP_CHAR
      if (FD->DirName) {
         convertToDIRSEP(FD->DirName);
      }
#endif

      retvalb = false;
      if (SmallQ) {
         XGlobal->SmallQueuesInProgress.delFilesDetailNickFile(FD->Nick, FD->FileName);
      }
      else {
         XGlobal->QueuesInProgress.delFilesDetailNickFile(FD->Nick, FD->FileName);
         
      }

      // If the guy we are sending to is not in CHANNEL_MAIN
      // do not send, unless first 10 characters are MasalaMate
      // Dont want to not send an Upgrade !!
      // and unless its a manual send. We allow Manual Sends to Nick wether
      // in channel or not.
      if ( (FD->ManualSend != MANUALSEND_DCCSEND) &&
           (XGlobal->NickList.isNickInChannel(CHANNEL_MAIN, FD->Nick) == false)  &&
           strncasecmp(FD->FileName, CLIENT_NAME, strlen(CLIENT_NAME))
         ) {
         // Refuse to send.
         sprintf(Response, "Server 04,01Upload: %s not in Main Channel: %s. Not sending file \"%s\"", FD->Nick, CHANNEL_MAIN, FD->FileName);
         XGlobal->IRC_ToUI.putLine(Response);

         COUT(cout << "Not in CHANNEL_MAIN and FileName not having Client Name - outgoing transfer rejected to " << FD->Nick << endl;)

         // we are done, break out. Do not free FD.
         // XGlobal->QueuesInProgress.freeFilesDetailList(FD);
         break;
      }

      // Correct file size in FD.
      fullq_filename = getFilenameWithPathFromFD(FD);
      retvalb = getFileSize(fullq_filename, &FD->FileSize);
      COUT(cout << "Helper::dccSend: File: " << fullq_filename << " Size: " << FD->FileSize << endl;)

      if (retvalb == false) {
         // If File doesnt exist, we inform the user that we arent serving
         // the file, and to please attempt to update our file list again.
         // If file didnt exist, we refresh our internal list.
         generateMyFilesDB();
         generateMyPartialFilesDB();

         sprintf(Response, "PRIVMSG %s :I am not currently serving the requested file: %s. Please update my serving list again.", FD->Nick, FD->FileName);
         XGlobal->IRC_ToServer.putLine(Response);

         // we are done, break out.
         break;
      }

      // Lets make a copy of FD to be used to put in DCCSendWaiting
      {
      FilesDetail *CopyFD = XGlobal->DCCSendWaiting.copyFilesDetail(FD);

         // Lets add this FD in DCCSendWaiting
         DCCSend = new TCPConnect;
         CopyFD->Connection = DCCSend;

         XGlobal->DCCSendWaiting.addFilesDetail(CopyFD);
         CopyFD = NULL;
         // Now we cant use CopyFD as its gone in Q, forcing to NULL so we
         // dont use it.
      }

      // We first try to connect directly to that nicks DCCSERVER_PORT
//    sprintf(Response, "NOTICE %s :I shall try Sending to DCCServer at port %d first. If that fails I shall try the DCC way.", CopyFD->Nick, DCCSERVER_PORT);
//    XGlobal->putLineIRC_ToServer(Response);
      DCCSend->TCPConnectionMethod = XGlobal->getIRC_CM();
      XGlobal->resetIRC_CM_Changed();
      COUT(cout << "TimerThr: ";)
      COUT(DCCSend->printDebug();)
      // Now lets see if we connect.
      retvalb = DCCSend->getConnection(FD->DottedIP, DCCSERVER_PORT);
      if (retvalb) {
         LineParse LineP;
         const char *parseptr;
         DCC_Container_t *DCC_Container;

         retvalb = false;
         // Lets get our nick.
         XGlobal->getIRC_Nick(Nick);
         XGlobal->resetIRC_Nick_Changed();

         sprintf(Response, "120 %s %lu %s\n", Nick, FD->FileSize, FD->FileName);
         DCCSend->writeData(Response, strlen(Response), DCCSERVER_TIMEOUT);
         retval = DCCSend->readLine(Response, sizeof(Response) - 1, DCCSERVER_TIMEOUT);
         LineP = Response;
         parseptr = LineP.getWord(1);
         if ( (retval > 0) && (strcmp("121", parseptr) == 0) ) {
            // Mark this nick we connected to as not firewalled.
            XGlobal->NickList.setNickFirewall(FD->Nick, IRCNICKFW_NO);
            COUT(cout << "Nick: " << FD->Nick << " marked as IRCNICKFW_NO" << endl;)

            retvalb = true;
            DCC_Container = new DCC_Container_t;
            DCC_CONTAINER_INIT(DCC_Container);
            DCC_Container->Connection = DCCSend;
            DCC_Container->XGlobal = XGlobal;
            DCC_Container->RemoteNick = new char[strlen(FD->Nick) + 1];
            strcpy(DCC_Container->RemoteNick, FD->Nick);
            DCC_Container->RemoteDottedIP = new char[strlen(FD->DottedIP) + 1];
            strcpy(DCC_Container->RemoteDottedIP, FD->DottedIP);
            parseptr = LineP.getWord(3);
            DCC_Container->ResumePosition = strtoul(parseptr, NULL, 10);
            DCC_Container->FileName = fullq_filename;
            DCC_Container->FileSize = FD->FileSize;
            fullq_filename = NULL;

            // Lets update the Resume Position in DCCSendWaiting
            XGlobal->DCCSendWaiting.updateFilesDetailNickFileResume(FD->Nick, FD->FileName, DCC_Container->ResumePosition);
#ifdef __MINGW32__
            THREADID tempTID;
            TransferThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) TransferThr, DCC_Container, 0, &tempTID);
#else
            {
               pthread_attr_t thread_attr;
               pthread_attr_init(&thread_attr);
               pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
               pthread_create(&TransferThrH, &thread_attr, (void * (*)(void *)) TransferThr, DCC_Container);
            }
#endif
         }
         else if ( (retval > 0) && (strcmp("151", parseptr) == 0) ) {
            // It was rejected. Do NOT try the DCC SEND.
            retvalb = true;
            // Lets delete the TCPConnect created previously.
            delete DCCSend;
         } 
         else {
            // TRY the DCC SEND method.
            retvalb = false;
         }
      }
      if (retvalb == false) {
         unsigned long MyIp;

         MyIp = XGlobal->getIRC_IP(NULL);
         XGlobal->resetIRC_IP_Changed();

//       If successful with workaround in previous if(), we should
//       set retvalb to true, so that we dont come here.
//       We need to do send a DCC Chat for the client to connect to
//       us and access the File Server.

         // Lets make the Connection FD in the DCCSendWaiting as NULL.
         // as it has an erroneous value set previously.
         XGlobal->DCCSendWaiting.updateFilesDetailNickFileConnection(FD->Nick, FD->FileName, NULL);

         // Lets delete the TCPConnect created previously.
         delete DCCSend;

//       sprintf(Response, "NOTICE %s :DCCServer send failed. Trying to DCC Send", FD->Nick);
//       XGlobal->putLineIRC_ToServer(Response);

//       Lets issue a DCC SEND to the remote nick with our DCCServer port.
//       We use same port DCCSERVER_PORT for everything.
//       Hence we need to get a nice dotted ip for recognising an
//       incoming connection as a DCC GET
         sprintf(Response, "PRIVMSG %s :\001DCC SEND \"%s\" %lu %d %lu\001", FD->Nick, FD->FileName, MyIp, DCCSERVER_PORT, FD->FileSize);
         XGlobal->IRC_ToServer.putLine(Response);
//       The above will get us a connection or a DCC RESUME.
      }
   } while (false);

   delete [] fullq_filename;
   return(TransferThrH);
}

// Below is called by the helperFServ function
bool Helper::fservRecvMessage() {
bool retvalb;
int retval;

   TRACE();

   retval = Connection->readLine(RetPointer, 1023, DCCCHAT_TIMEOUT);
   if (XGlobal->isIRC_QUIT()) {
      retval = -1;
   }

   if (retval <= 0) retvalb = false;
   else retvalb = true;

#if 0
   COUT(cout << "Helper::fservRecvMessage: " << RetPointer << endl);
#endif
   return(retvalb);
}

// Sends Message in RetPointer. returns succesfull send or failure.
// Make sure RetPointer contains the string to be sent.
bool Helper::fservSendMessage() {
int retval, bytecount;
bool retvalb = true;

   TRACE();

   bytecount = strlen(RetPointer);
   retval = Connection->writeData(RetPointer, bytecount);
   if (retval != bytecount) retvalb = false;

#if 0
   COUT(cout << "Helper::fservSendMessage: " << RetPointer << endl);
#endif
   return(retvalb);
}

// Common function used by both FileServer and DCCChatClient
// Responds to a NICKLIST command.
// We reply as follows for version 3:
// nick_1 0 ip sends tsend qs tqs fw ChkSumS ChkSumP
// nick_2 update_count ip sends tsends qs tqs fw ChkSumS ChkSumP
// ...
// nick_n update_count ip sends tsends qs tqs fw ChkSumS ChkSumP
// Note ip is in hex. First entry is our own entry.
// ChkSumS = ChkSum done on Serving Files.
// ChkSumP = Chksum done on Partial Files.
bool Helper::helperFServNicklist(int FFLCversion) {
bool retvalb = false;
int cur_index;
char *cmp_nick = new char[64];
FilesDetail *FD;
char *ChkSumS = new char[64];
char *ChkSumP = new char[64];
long servingfileslen;
long servingfilescnt;
long partialfileslen;
long partialfilescnt;
unsigned long ip;
char fw_state;
int MyMaxSends;
int MyMaxQueues;


   TRACE();

   XGlobal->lock();
   MyMaxSends = XGlobal->FServSendsOverall;
   MyMaxQueues = XGlobal->FServQueuesOverall;
   XGlobal->unlock();

   do {
      strcpy(RetPointer, "ERROR: FFLC Version != 3");
      if (FFLCversion != 3) break;

      fw_state = XGlobal->NickList.getNickFirewall(MyNick);
      ip = XGlobal->getIRC_IP(NULL);
      XGlobal->resetIRC_IP_Changed();

      // Consider checksums of both MyFilesDB and MyPartialFilesDB.
      XGlobal->lockFFLC();
      XGlobal->MyFilesDB.getCheckSums(MyNick, &servingfileslen, &servingfilescnt, &partialfileslen, &partialfilescnt);
      XGlobal->MyFilesDB.getCheckSumString(servingfileslen, servingfilescnt, ChkSumS);
      XGlobal->MyPartialFilesDB.getCheckSums(MyNick, &servingfileslen, &servingfilescnt, &partialfileslen, &partialfilescnt);
      XGlobal->MyPartialFilesDB.getCheckSumString(partialfileslen, partialfilescnt, ChkSumP);
      XGlobal->unlockFFLC();

      sprintf(RetPointer, "%s 0 %x %d %d %d %d %c %s %s\n", 
                           MyNick, 
                           ip, 
                           XGlobal->SendsInProgress.getCount(NULL),
                           MyMaxSends,
                           XGlobal->QueuesInProgress.getCount(NULL),
                           MyMaxQueues,
                           fw_state,
                           ChkSumS,
                           ChkSumP);
      if (fservSendMessage() == false) break;

      // Now we iterate through the NickList in XGlobal for channel
      // CHANNEL_MAIN, and check if we have any of that nick's files in
      // FilesDB. If we do we send out the name with the update count.
      for (int cur_index = 1; ; cur_index++) {
         if (XGlobal->NickList.getNickInChannelAtIndex(CHANNEL_MAIN, cur_index, cmp_nick)) {
            // If FilesDB has "Inaccessible Server" entry for this nick
            // We dont present it.
            if (XGlobal->FilesDB.isPresentMatchingNickFileName(cmp_nick, "Inaccessible Server") == 0) {
               // Now check if nick in cmp_nick exists in FilesDB.
               FD = XGlobal->FilesDB.getFilesDetailListNickFile(cmp_nick, "TriggerTemplate");
               // To optimise, we send this nick only if its UpdateCount is
               // <= FILES_DETAIL_LIST_UPDATE_ROLLOVER - 2
               // We also send the UpdateCount as one more that what we have
               // so that UpdateCounts are propagated differently.
               // helping to distribute the time when a MM will access a 
               // trigger. With Propagation algo in place its not required.
               if (FD) {
                  // We have this guys file.
                  ip = XGlobal->NickList.getNickIP(FD->Nick);
                  XGlobal->lockFFLC();
                  XGlobal->FilesDB.getCheckSums(FD->Nick, &servingfileslen, &servingfilescnt, &partialfileslen, &partialfilescnt);
                  XGlobal->FilesDB.getCheckSumString(servingfileslen, servingfilescnt, ChkSumS);
                  XGlobal->FilesDB.getCheckSumString(partialfileslen, partialfilescnt, ChkSumP);
                  XGlobal->unlockFFLC();
                  fw_state = XGlobal->NickList.getNickFirewall(FD->Nick);
                  sprintf(RetPointer, "%s %d %x %d %d %d %d %c %s %s\n", 
                                FD->Nick, 
                                FD->UpdateCount,
                                ip,
                                XGlobal->SendsInProgress.getCount(NULL),
                                MyMaxSends,
                                XGlobal->QueuesInProgress.getCount(NULL),
                                MyMaxQueues,
                                fw_state,
                                ChkSumS,
                                ChkSumP);
                  XGlobal->FilesDB.freeFilesDetailList(FD);
                  if (fservSendMessage() == false) break;
               }
            }
         }
         else break; // No more nicks in channel.
      }

      // We are done. Send out the suffix
      strcpy(RetPointer, "nicklist end\n");
      if (fservSendMessage() == false) break;

      retvalb = true;
   } while (false);

   delete [] cmp_nick;
   delete [] ChkSumS;
   delete [] ChkSumP;
   return(retvalb);
}

// Common function used by both FileServer and DCCChatClient
// Responds to a FILELIST<S or P> command.
// dstate is S or P.
// We reply as follos:
// TriggerTemplate ...
// fileinfo_1
// ...
// fileinfo_n
// filelist end
// TriggerTemplate is of the form:
// TriggerTemplate*TriggerType*ClientType*TriggerName*CurrentSends*TotalSends*
//    CurrentQueues*TotalQueues
// Now fileinfo_x is of the below form
//   FileSize*DirName*FileName (CTCP)
//     or
//   FileSize*PackNum*FileName (XDCC)
bool Helper::helperFServFilelist(char *varnick, char dstate) {
bool retvalb = false;
FilesDetail *FD, *ScanFD;
char *dir_name;
char client_type = IRCNICKCLIENT_UNKNOWN;
char trig_type = ' ';
int MyMaxSends;
int MyMaxQueues;

   TRACE();

   XGlobal->lock();
   MyMaxSends = XGlobal->FServSendsOverall;
   MyMaxQueues = XGlobal->FServQueuesOverall;
   XGlobal->unlock();
   do {
      // Lets get the FD which holds all files of the Nick mentioned.
      // If varnick is our nick then we need to send him info from MyFilesDB
      if (strcasecmp(varnick, MyNick) == 0) {
          // First send out our TriggerTemplate

          client_type = IRCNICKCLIENT_MASALAMATE;
          trig_type = 'C';
          sprintf(RetPointer, "TriggerTemplate*C*%c*Masala of %s*%d*%d*%d*%d\n",
                  IRCNICKCLIENT_MASALAMATE,
                  MyNick,
                  XGlobal->SendsInProgress.getCount(NULL),
                  MyMaxSends,
                  XGlobal->QueuesInProgress.getCount(NULL),
                  MyMaxQueues);
          if (fservSendMessage() == false) break;

          // if dstate == S, we need to send list from MyFilesDB
          // if dstate == P, we need to send list from MyPartialFilesDB
           
          if (dstate == 'P') {
             FD = XGlobal->MyPartialFilesDB.getFilesOfNickByDownloadState(MyNick, dstate);
          }
          else {
             FD = XGlobal->MyFilesDB.getFilesOfNickByDownloadState(MyNick, dstate);
          }
          if (FD == NULL) {
             // If no Files
             // We are not serving any files. So just send a dummy 
             // "No Files Present" entry in FD. (fill up FileName
             // and FileSize.
             FD = new FilesDetail;
             XGlobal->MyFilesDB.initFilesDetail(FD);
             FD->FileName = new char[17];
             strcpy(FD->FileName, "No Files Present");
             FD->FileSize = 1;
             FD->ClientType = IRCNICKCLIENT_MASALAMATE;
          }
      }
      else {
          FD = XGlobal->FilesDB.getFilesDetailListNickFile(varnick, "TriggerTemplate");
          if (FD) {
             char *trig_name;
             if (FD->TriggerType == FSERVCTCP) trig_type = 'C';
             else if (FD->TriggerType == XDCC) trig_type = 'X';
             else trig_type = ' ';

             if (FD->TriggerName) trig_name = FD->TriggerName;
             else trig_name = " ";
             sprintf(RetPointer, "TriggerTemplate*%c*%c*%s*%d*%d*%d*%d\n",
                  trig_type,
                  FD->ClientType,
                  trig_name,
                  FD->CurrentSends,
                  FD->TotalSends,
                  FD->CurrentQueues,
                  FD->TotalQueues);

             XGlobal->FilesDB.freeFilesDetailList(FD);
             if (fservSendMessage() == false) break;
          }
          else if (XGlobal->FilesDB.isFilesOfNickPresent(varnick) &&
                (XGlobal->FilesDB.isPresentMatchingNickFileName(varnick, "Inaccessible Server") == 0) ) {
               // ie, I have an entry other than "TemplateTrigger" and
               // I do not have an entry for "Inaccessible Server"

             // Only send TriggerTemplate if we have at least one file.
             sprintf(RetPointer, "TriggerTemplate*%c* * * * * * \n",
                  IRCNICKCLIENT_UNKNOWN);
             if (fservSendMessage() == false) break;
          }

          FD = XGlobal->FilesDB.getFilesOfNickByDownloadState(varnick, dstate);
      }

      ScanFD = FD;
      while (ScanFD) {

          // Skip over "TriggerTemplate" as we are done with it already.
          if (strcasecmp(ScanFD->FileName, "TriggerTemplate") == 0) {
             ScanFD = ScanFD->Next;
             continue;
          }
          // Skip over "Inaccessible Server" as we wouldnt be propagating
          // this entry.
          if (strcasecmp(ScanFD->FileName, "Inaccessible Server") == 0) {
             ScanFD = ScanFD->Next;
             continue;
          }

          if (ScanFD->DirName) {
             dir_name = ScanFD->DirName;
          }
          else dir_name = " ";

          if (client_type != IRCNICKCLIENT_MASALAMATE) {
             client_type = ScanFD->ClientType;
          }
          if (trig_type == 'X') {
             sprintf(RetPointer, "%lu*%d*%s\n",
                     ScanFD->FileSize, ScanFD->PackNum, ScanFD->FileName);
          }
          else {
             sprintf(RetPointer, "%lu*%s*%s\n",
                     ScanFD->FileSize, dir_name, ScanFD->FileName);
          }

          if (fservSendMessage() == false) break;

          ScanFD = ScanFD->Next;
      }
      XGlobal->FilesDB.freeFilesDetailList(FD);

      // We are done. Send out the suffix
      strcpy(RetPointer, "filelist end\n");
      if (fservSendMessage() == false) break;

      retvalb = true;

   } while (false);

   return(retvalb);
}

// Common function used by both FileServer and DCCChatClient
// Server: Handles the Endlist command issued by client. So server, now
//   tries to get client's information. Server, will exit after this call.
// Client: As soon as it gets the prompt from File Server, it starts this
//   conversation. Client, will issue an "Endlist" before exiting this call.
// We always issue the nicklist first, and keep comparing the update counts
// that he has against our update counts.
// If his are smaller we note that nick in LineQueue, and issue filelist
// against it. Also note down the nicks whose listing we dont have.
// The param server == true => server, false => client.
// if server is false, we are client, use FFLCversion as the FFLC version
// of the server we are talking to, or the client we are talking to
bool Helper::helperFServStartEndlist(char *RemoteNick, bool server, int FFLCversion) {
bool retvalb = false;
LineQueue UpdateNicks;
LineParse LineP;
const char *charp;
int update_count;
FilesDetail *HeadFD = NULL, *FD = NULL, *TemplateFD = NULL;
char *tmp_str = NULL;

   TRACE();

   // The full nicklist/filelist transaction of all nicks is locked as of now.
//   XGlobal->lockFFLC();
//   COUT(cout << "FFLC: lock for nicklist/filelist for transaction with " << RemoteNick << endl;)

   do {
      // Let us first issue the nicklist command.
      sprintf(RetPointer, "nicklist %d\n", FFLCversion);
      if (fservSendMessage() == false) break;

      // Now we should receive the nicklist response as follows:
      // nick updatecount ip sends tsends qs tqs fw ChkSumS ChkSumP
      // nicklist end
      // So lets loop till we get it all.
      while (true) {

         if (fservRecvMessage() == false) break;

         // Take appropriate action.
         if (strcasecmp(RetPointer, "NICKLIST END") == 0) {
            // Got the end of the listing. Lets break
            break;
         }
         else {
           unsigned long nick_ip;
           char fw_state;
            // If we dont have info on nick then add it in
            // Version 3 ->
            // If we have info on that nick but update count is smaller than
            // what we have, then do the below:
            // update the ip we have for that nick.
            // generated CHKSum for the files of Nick that we hold.
            // if SHAs match, then just update the UpdateCount on the Files
            // that we hold for Nick.
            // if CHKSums dont match then we add it in.

            LineP = RetPointer;

            // proceed if server is FFLC version 3
            if (FFLCversion != 3) break;
            if (LineP.getWordCount() != 10) break;

            charp = LineP.getWord(1); // Nick

            // Save nick for later use.
            tmp_str = new char[strlen(charp) + 1];
            strcpy(tmp_str, charp);

            // Note update_count now itself, so we have sane update_count
            // if we dont have this nicks file information
            charp = LineP.getWord(2);
            update_count = strtoul(charp, NULL, 10);

            if (XGlobal->FilesDB.isFilesOfNickPresent(tmp_str) &&
                (XGlobal->FilesDB.isPresentMatchingNickFileName(tmp_str, "Inaccessible Server") == 0) ) {
               // ie, I have an entry other than "TemplateTrigger" and
               // I do not have an entry for "Inaccessible Server"
               // The Inaccessible server is removed by DCCChatClient.
               FD = XGlobal->FilesDB.getFilesDetailListNickFile(tmp_str, "TriggerTemplate");
               // We have this nicks info. Check on the update_count
               // We already have noted down the update_count.

               // FD could possibly be NULL above.
               // We always try to update the files held by the server we
               // are accesing => update_count = 0.
               if ( (FD == NULL) || (FD->UpdateCount > update_count)  || (update_count == 0) ) {
                  if (FFLCversion == 3) {
                     char ChkSumS[64], ChkSumP[64];
                     char ChkSumRcvS[64], ChkSumRcvP[64];
                     long servingfileslen, servingfilescnt;
                     long partialfileslen, partialfilescnt;

                     // Lets update the IP we have received. ip in hex.
                     charp = LineP.getWord(3);
                     nick_ip = strtoul(charp, NULL, 16);
                     // We update here only if we dont have that ip info.
                     if ( (nick_ip != 0) && (XGlobal->NickList.getNickIP(tmp_str) == IRCNICKIP_UNKNOWN) ) {
                        XGlobal->NickList.setNickIP(tmp_str, nick_ip);
                        COUT(cout << "FFLC: ip of " << tmp_str << " updated to: " << nick_ip << endl;)
                     }
                     charp = LineP.getWord(8);
                     fw_state = *charp;

                     COUT(cout << "FFLC: Firewall received: " << fw_state << endl;)

                     if (fw_state != IRCNICKFW_UNKNOWN) {
                        // If we dont have info on this nicks's firewall
                        // lets update with what he has.
                        if (XGlobal->NickList.getNickFirewall(tmp_str) == IRCNICKFW_UNKNOWN) {
                           XGlobal->NickList.setNickFirewall(tmp_str, fw_state);
                           COUT(cout << "Nick: " << tmp_str << " marked as " << fw_state << endl;)
                        }
                     }

                     // Now lets see if we need to really get the serving/
                     // partials files of nick, or just set the UpdateCount.
                     charp = LineP.getWord(9); // Serving ChkSum
                     strncpy(ChkSumRcvS, charp, sizeof(ChkSumRcvS) - 1);
                     charp = LineP.getWord(10); // Partial ChkSum
                     strncpy(ChkSumRcvP, charp, sizeof(ChkSumRcvP) - 1);
                     XGlobal->FilesDB.getCheckSums(tmp_str, &servingfileslen, &servingfilescnt, &partialfileslen, &partialfilescnt);
                     XGlobal->FilesDB.getCheckSumString(servingfileslen, servingfilescnt, ChkSumS);
                     XGlobal->FilesDB.getCheckSumString(partialfileslen, partialfilescnt, ChkSumP);
                     COUT(cout << "FFLC: SFiles: chksum rcvd: " << ChkSumRcvS << " chksum calc: " << ChkSumS << endl;)
                     COUT(cout << "FFLC: PFiles: chksum rcvd: " << ChkSumRcvP << " chksum calc: " << ChkSumP << endl;)

                     bool alldeleted = false;
                     if ( strcasecmp(ChkSumS, ChkSumRcvS) &&
                          strcasecmp(ChkSumP, ChkSumRcvP) ) {
                        // Both checksums are different. Delete entire list.
                        // We dont delete anything yet.
                        // XGlobal->FilesDB.delFilesOfNick(tmp_str);
                        alldeleted = true;
                        COUT(cout << "FFLC: S and P ChkSums differ" << endl;)
                     }
                     else {
                        // One of them is same, so update count on all
                     int sends, tsends, qs, tqs;

                        // Same File List. just update UpdateCount, and stuff
                        // Lets update the send/qs information.
                        charp = LineP.getWord(4);
                        sends = (int) strtol(charp, NULL, 10);
                        charp = LineP.getWord(5);
                        tsends = (int) strtol(charp, NULL, 10);
                        charp = LineP.getWord(6);
                        qs = (int) strtol(charp, NULL, 10);
                        charp = LineP.getWord(7);
                        tqs = (int) strtol(charp, NULL, 10);

                        XGlobal->FilesDB.updateSendsQueuesOfNick(tmp_str, sends, tsends, qs, tqs);

                        XGlobal->FilesDB.updateUpdateCountOfNick(tmp_str, update_count);
                        COUT(cout << "FFLC: P or S or both have same File List. Just update UpdateCount" << endl;)
                     }
                     // Now check if serving or partial or both need to be
                     // filelist updated.

                     if (strcasecmp(ChkSumS, ChkSumRcvS)) {
                        // We need to update the Serving filelist on this nick.
                        // First delete its entries.
                        if (alldeleted == false) {
                           // to save on iterating the list, the flag is used.
                           // We dont delete anything yet.
                           // XGlobal->FilesDB.delFilesOfNickByDownloadState(tmp_str, DOWNLOADSTATE_SERVING);
                        }
                        sprintf(RetPointer, "%s %d %c", tmp_str, update_count, DOWNLOADSTATE_SERVING);
                        UpdateNicks.putLine(RetPointer);
                        COUT(cout << "FFLC: Adding: " << RetPointer << endl;)
                     }

                     if (strcasecmp(ChkSumP, ChkSumRcvP)) {
                        // We need to update the Partial filelist on this nick.
                        // First delete its entries.
                        if (alldeleted == false) {
                           // to save on iterating the list, the flag is used.
                           // We dont delete anything yet.
                           // XGlobal->FilesDB.delFilesOfNickByDownloadState(tmp_str, DOWNLOADSTATE_PARTIAL);
                        }
                        sprintf(RetPointer, "%s %d %c", tmp_str, update_count, DOWNLOADSTATE_PARTIAL);
                        UpdateNicks.putLine(RetPointer);
                        COUT(cout << "FFLC: Adding: " << RetPointer << endl;)
                     }

                  }
               }
               XGlobal->FilesDB.freeFilesDetailList(FD);
            }
            else if (strcasecmp(tmp_str, MyNick)) {
               // We dont have this nicks info. Lets note that down.
               // If the nick is NOT our nick, then only note it down
               // First delete its entry if any.
               // Dont delete anything yet.
               // XGlobal->FilesDB.delFilesOfNick(tmp_str);

               // Need to get both its Serving and Partial list.
               sprintf(RetPointer, "%s %d %c", tmp_str, update_count, DOWNLOADSTATE_SERVING);
               UpdateNicks.putLine(RetPointer);
               COUT(cout << "FFLC: Adding: " << RetPointer << endl;)

               sprintf(RetPointer, "%s %d %c", tmp_str, update_count, DOWNLOADSTATE_PARTIAL);
               UpdateNicks.putLine(RetPointer);
               COUT(cout << "FFLC: Adding: " << RetPointer << endl;)

               // Note below work on LineP. RetPointer is destroyed !!!

               if (FFLCversion == 3) {
                  // Lets update the IP we have received. ip in hex.
                  charp = LineP.getWord(3);
                  nick_ip = strtoul(charp, NULL, 16);
                  // We update here only if we dont have that ip info.
                  if ( (nick_ip != 0) && (XGlobal->NickList.getNickIP(tmp_str) == IRCNICKIP_UNKNOWN) ) {
                     XGlobal->NickList.setNickIP(tmp_str, nick_ip);
                     COUT(cout << "FFLC: ip of " << tmp_str << " updated to:" << nick_ip << endl;)
                  }
                  charp = LineP.getWord(8);
                  fw_state = *charp;

                  COUT(cout << "FFLC: Firewall received: " << fw_state << endl;)

                  if (fw_state != IRCNICKFW_UNKNOWN) {
                     // If we dont have info on this nicks's firewall
                     // lets update with what he has.
                     if (XGlobal->NickList.getNickFirewall(tmp_str) == IRCNICKFW_UNKNOWN) {
                        XGlobal->NickList.setNickFirewall(tmp_str, fw_state);
                        COUT(cout << "Nick: " << tmp_str << " marked as " << fw_state << endl;)
                     }
                  }
               }
            }
            delete [] tmp_str;
         }
      }
      tmp_str = NULL;

      // We have finished the nicklist part of the exchange.
      // We have noted the nicks we are interested in with their update_count
      // in UpdateNicks followed by a one character S or P, which stands for
      // Serving Dir Files or Partial Dir Files.
      // Note that we havent deleted any entries from FilesDB.
      bool UpdateFilesDB = false;
      char SorP;
      TemplateFD = NULL;
      FD = NULL;
      HeadFD = NULL;
      do {
         // If its empty, we have nothing to process.
         if (UpdateNicks.isEmpty()) break;

         // Lets get the nicks out one by one.
         UpdateNicks.getLineAndDelete(RetPointer);

         // First word is nick. second word is the update_count.
         // Third word is S or P.
         LineP = RetPointer;
         charp = LineP.getWord(2); // update count
         update_count = strtoul(charp, NULL, 10);
         charp = LineP.getWord(3);
         SorP = *charp; // Either S or P.
         charp = LineP.getWord(1); // Nick

         tmp_str = new char[strlen(charp) + 1];
         strcpy(tmp_str, charp); // Save nick for later.

         // Lets issue filelists <nick> or filelistp <nick>
         sprintf(RetPointer, "filelist%c %s\n", SorP, tmp_str);
         if (fservSendMessage() == false) break;

         // Now we read in what he has to say. First is the TriggerTemplate.
         if (fservRecvMessage() == false) break;

         // TriggerTemplate*TriggerType*ClientType*TriggerName*CurrentSends*
         //   TotalSends*CurrentQueues*TotalQueues

         LineP = RetPointer;
         LineP.setDeLimiter('*');
         charp = LineP.getWord(1);
         if (strcasecmp(charp, "TriggerTemplate")) break;

         // We add the "TriggerTemplate"
         FD = new FilesDetail;
         XGlobal->FilesDB.initFilesDetail(FD);
         charp = LineP.getWord(2);
         FD->TriggerName = NULL;
         if (*charp == 'C') {
            FD->TriggerType = FSERVCTCP;
            charp = LineP.getWord(4);
            FD->TriggerName = new char[strlen(charp) + 1];
            strcpy(FD->TriggerName, charp);
         }
         else if (*charp == 'X') {
            FD->TriggerType = XDCC;
         }
         else if (*charp == ' ') {
            FD->TriggerType = FSERVINVALID;
         }
         FD->Nick = tmp_str;
         tmp_str = NULL;
         FD->FileName = new char[16];
         strcpy(FD->FileName, "TriggerTemplate");
         FD->UpdateCount = update_count;

         charp = LineP.getWord(3);

         // Update the Client Type only if its not IRCNICKCLIENT_MASALAMATE
         // and not IRCNICKCLIENT_UNKNOWN
         // We do not need anyones help in knowing the other MM clients
         // They are just the nicks in channel CHANNEL_MAIN_MM
         FD->ClientType = *charp;
         if ( (*charp != IRCNICKCLIENT_MASALAMATE) &&
              (*charp != IRCNICKCLIENT_UNKNOWN) ) {

            // Update Client Type in XGlobal's NickList too
            XGlobal->NickList.setNickClient(FD->Nick, FD->ClientType);
         }
         charp = LineP.getWord(5);
         FD->CurrentSends = strtoul(charp, NULL, 10);
         charp = LineP.getWord(6);
         FD->TotalSends = strtoul(charp, NULL, 10);
         charp = LineP.getWord(7);
         FD->CurrentQueues = strtoul(charp, NULL, 10);
         charp = LineP.getWord(8);
         FD->TotalQueues = strtoul(charp, NULL, 10);

         TemplateFD = FD;
         FD = NULL;
         HeadFD = TemplateFD;
         // HeadFD will contain all the FDs created as a link.
         // Just delete it on failure.
         // Or add it to FilesDB on success.
         // Note that it also contains the TemplateFD.

         // Lets have a working copy of the TriggerTemplate
         // TemplateFD = XGlobal->FilesDB.copyFilesDetail(FD);

         // Delete the copy already existing if any.
         // Nothing to delete yet.
         // XGlobal->FilesDB.delFilesDetailNickFile(FD->Nick, FD->FileName);

         // Nothing to add yet.
         // XGlobal->FilesDB.addFilesDetail(FD);
         // FD = NULL;
         // With above we are done with the "TriggerTemplate"

         // Now what follows are all FileSize*DirName*FileName
         // till we hit "filelist end"

         do {
            if (fservRecvMessage() == false) break;

            // Check if its "filelist end"
            if (strcasecmp(RetPointer, "filelist end") == 0) {
               // This was a successful transaction.
               UpdateFilesDB = true;
               break;
            }

            // Process all the lines: FileSize*DirName*FileName
            // or like: FileSize*PackNum*FileName
            FD = XGlobal->FilesDB.copyFilesDetail(TemplateFD);
            // In case the copy from template had a DirName.
            delete [] FD->DirName;
            FD->DirName = NULL;
            // The copy from template has a FileName
            delete [] FD->FileName;
            FD->FileName = NULL;

            LineP = RetPointer;
            LineP.setDeLimiter('*');
            charp = LineP.getWord(1);
            FD->FileSize = strtoul(charp, NULL, 10);
            charp = LineP.getWord(2);
            FD->DownloadState = SorP;
            if (FD->TriggerType == XDCC) {
               // Its the pack number.
               FD->PackNum = strtol(charp, NULL, 10);
            }
            else {
               if (strcmp(charp, " ") != 0) {
                  FD->DirName = new char[strlen(charp) + 1];
                  strcpy(FD->DirName, charp);
               }
            }
            charp = LineP.getWord(3);
            FD->FileName = new char[strlen(charp) + 1];
            strcpy(FD->FileName, charp);

            // We dont add to FilesDB yet.
            // XGlobal->FilesDB.addFilesDetail(FD);

            // Append to HeadFD.
            FD->Next = HeadFD;
            HeadFD = FD;
            FD = NULL;

         } while (true);

         // Check if the filelist was a successful transaction.
         if (UpdateFilesDB) {
            // We first delete the TriggerTemplate in FilesDB.
            // TemplateFD is non NULL as UpdateFilesDB is true.
            XGlobal->lockFFLC();
            COUT(cout << "Helper:: FFLC adding nick " << TemplateFD->Nick << "SorP: " << SorP << " lockFFLC()" << endl;)
            XGlobal->FilesDB.delFilesDetailNickFile(TemplateFD->Nick, TemplateFD->FileName);
            // Now depending on SorP delete the proper entries of nick
            // in FilesDB.
            XGlobal->FilesDB.delFilesOfNickByDownloadState(TemplateFD->Nick, SorP);
            // Now add the list we have amassed.
            XGlobal->FilesDB.addFilesDetail(HeadFD);
            COUT(cout << "Helper:: FFLC adding nick unlockFFLC()" << endl;)
            XGlobal->unlockFFLC();
         }
         else {
            // Unsuccessful transaction. Delete whatever we created.
            XGlobal->FilesDB.freeFilesDetailList(HeadFD);
         }
         HeadFD = NULL;
         FD = NULL;
         TemplateFD = NULL;

      } while (true);

      // We are done. lets say so.
      strcpy(RetPointer, "endlist\n");
      if (fservSendMessage() == false) break;
      retvalb = true;

   } while (false);

   delete [] tmp_str; // In case we break on error.
   XGlobal->FilesDB.freeFilesDetailList(TemplateFD); // In case we break on error.

   // If server we are all done.
   if (server) {
//      COUT(cout << "FFLC: unlock if server" << endl;)
//      XGlobal->unlockFFLC();

      // update the statistics and output in UI.
      helperFFLCStatistics(RemoteNick, FFLCversion);
      return(retvalb);
   }

   // If Client, we wait for the server to ask us questions.
   // Now its servers turn to ask us questions and we reply.
   // This is simlar to the FileServer::run loop, but handles only
   // NICKLIST, FILELIST and ENDLIST commands.
   // Server knows what version we support as client already (detected by
   // how we issued the "nicklist version" command.

   time_t CurrentTime;
   time_t TimeOut;
   int retval;

   while (retvalb) {
      CurrentTime = time(NULL);
      TimeOut = CurrentTime - Connection->Born;
      TimeOut = FSERV_TIMEOUT - TimeOut;
      if (TimeOut <= 0) break;

      retval = Connection->readLine(RetPointer, 1023, TimeOut);
      if (XGlobal->isIRC_QUIT()) {
         retval = -1;
      }

      if (retval <= 0) {
         // FileServ timeout expired or error.
         break;
      }
      // COUT(cout << "Helper:fserv read message from server: " << RetPointer << endl;)

      LineP = RetPointer;
      charp = LineP.getWord(1);

      if ( !strcasecmp(charp, "NICKLIST") ) {
      int fflc_version;
         charp = LineP.getWord(2);
         fflc_version = (int) strtol(charp, NULL, 10);

         retvalb = helperFServNicklist(fflc_version);
      }
      else if ( !strncasecmp(charp, "FILELIST", 8) ) {
         // Handles both the filelists and filelistp command.
         char SorP = 'S';
         if ( (charp[8] == 'P') ||
              (charp[8] == 'p') ) {
            SorP = 'P';
         }
         if (LineP.getWordCount() == 2) {
            charp = LineP.getWord(2); // Nicks dont have space
            tmp_str = new char[strlen(charp) + 1];
            strcpy(tmp_str, charp);
            retvalb = helperFServFilelist(tmp_str, SorP);
            delete [] tmp_str;
         }
         else retvalb = false;
      }
      else if ( !strcasecmp(charp, "ENDLIST") ) {
         retvalb = true;
         break;
      }
   }

//   COUT(cout << "FFLC: unlock" << endl;)
//   XGlobal->unlockFFLC();

   // update the statistics and output in UI. (Client case)
   helperFFLCStatistics(RemoteNick, FFLCversion);

   return(retvalb);
}

// Collect statistics of the FFLC = Fast File List Collection algorithm
// and display in UI.
void Helper::helperFFLCStatistics(char *RemoteNick, int FFLCversion) {

   TRACE();

   if (Connection == NULL) return;

   XGlobal->lock();
   XGlobal->FFLC_BytesIn += (double) Connection->BytesReceived;
   XGlobal->FFLC_BytesOut += (double) Connection->BytesSent;
   sprintf(RetPointer, "Server 09FFLC Version %d: Transaction with %s. Bytes In: %lu Out: %lu. Total Bytes In: %g Out: %g", FFLCversion, RemoteNick, Connection->BytesReceived, Connection->BytesSent, XGlobal->FFLC_BytesIn, XGlobal->FFLC_BytesOut);
   XGlobal->unlock();
   XGlobal->IRC_ToUI.putLine(RetPointer);
}

// Used by TimerThr to process IRC_ToUpgrade Queue if any need be.
// If present we put an entry in QueuesInProgress at index 1, and mark
// it as a Manual send, so that it gets sent.
// Caller ensures that we are connected to IRC server and are joined in
// CHANNEL_MAIN.
// Always use SmallQueuesInProgress for the Upgrade file!
void Helper::processUpgrade() {
char DotIP[20];
TCPConnect *T;
unsigned long ip;
FilesDetail *FD;
LineParse LineP;
const char *parseptr;
IRCChannelList CL;
char *Response;

   TRACE();

   if (XGlobal->IRC_ToUpgrade.isEmpty()) return;

   // We got some upgrade to do.
   Response = new char[1024];

   XGlobal->IRC_ToUpgrade.getLineAndDelete(Response);
   // Process this line.
   COUT(cout << "processUpgrade: " << Response << endl;)

   // Response is of format: Nick longip FileName
   FD = new FilesDetail;
   XGlobal->SmallQueuesInProgress.initFilesDetail(FD);

   LineP = Response;
   parseptr = LineP.getWord(1);

   FD->Nick = new char[strlen(parseptr) + 1];
   strcpy(FD->Nick, parseptr);

   FD->DirName = new char[strlen(UPGRADE_DIR) + 1];
   strcpy(FD->DirName, UPGRADE_DIR);

   parseptr = LineP.getWord(2);
   ip = strtoul(parseptr, NULL, 10);
   T->getDottedIpAddressFromLong(ip, DotIP);
   FD->DottedIP = new char[strlen(DotIP) + 1];
   strcpy(FD->DottedIP, DotIP);

   FD->ManualSend = MANUALSEND_DCCSEND;
   FD->DownloadState = DOWNLOADSTATE_SERVING;

   parseptr = LineP.getWordRange(3, 0);
   FD->FileName = new char[strlen(parseptr) + 1];
   strcpy(FD->FileName, parseptr);

   // Lets fill up the file size.
   sprintf(Response, "%s%s%s", FD->DirName, DIR_SEP, FD->FileName);
   getFileSize(Response, &FD->FileSize);

   XGlobal->SmallQueuesInProgress.addFilesDetailAtIndex(FD, 1);
   FD = NULL;
   delete [] Response;
   COUT(cout << "processUpgrade: Returning from processUpgrade" << endl;)
}

// Used to get fully qualified filename with PATH given an FD.
// Generates it considering DownloadState, ManualSend. Used before spawning
// TransferThr.
// Caller should free the pointer returned.
char *Helper::getFilenameWithPathFromFD(FilesDetail *FD) {
char *fullq_filename = NULL;
int DirIndex;

   TRACE();

   if (FD == NULL) return(fullq_filename);

   if (FD->DownloadState == DOWNLOADSTATE_PARTIAL) {
      // if DownloadState = DOWNLOADSTATE_PARTIAL (MyPartialsFileDB)
      XGlobal->lock();
      fullq_filename = new char[strlen(XGlobal->PartialDir) + strlen(FD->FileName) + 2];
      sprintf(fullq_filename, "%s%s%s", XGlobal->PartialDir, DIR_SEP, FD->FileName);
      XGlobal->unlock();

   }
   else if (FD->DownloadState == DOWNLOADSTATE_SERVING) {
      // if DownloadState = DOWNLOADSTATE_SERVING (MYFilesDB or dcc Sends or Upgrades)
      if (FD->ManualSend != MANUALSEND_DCCSEND) {
         DirIndex = FD->ServingDirIndex;
         XGlobal->lock();
         if (FD->DirName) {
            fullq_filename = new char[strlen(XGlobal->ServingDir[DirIndex]) + strlen(FD->DirName) + strlen(FD->FileName) + 3];
            sprintf(fullq_filename, "%s%s%s%s%s", XGlobal->ServingDir[DirIndex], DIR_SEP, FD->DirName, DIR_SEP, FD->FileName);
         }
         else {
            fullq_filename = new char[strlen(XGlobal->ServingDir[DirIndex]) + strlen(FD->FileName) + 2];
            sprintf(fullq_filename, "%s%s%s", XGlobal->ServingDir[DirIndex], DIR_SEP, FD->FileName);
         }
         XGlobal->unlock();
      }
      else {
         // if ManualSend == MANUALSEND_DCCSEND (DCC Send or Upgrade)
         if (FD->DirName) {
            fullq_filename = new char[strlen(FD->DirName) + strlen(FD->FileName) + 2];
            sprintf(fullq_filename, "%s%s%s", FD->DirName, DIR_SEP, FD->FileName);
         }
         else {
            fullq_filename = new char[strlen(FD->FileName) + 1];
            strcpy(fullq_filename, FD->FileName);
         }
      }
   }
   return(fullq_filename);
}

// Returns a suitable FD from QueuesInProgress which is good for a Send.
// This can then be passes to dccSend()
// This should be as intelligent as possible.
// algo as of now ->
// 1. Try to get a Queue of Nick who has the least number of sends currently.
// The returned FD should be freed by the caller.
FilesDetail *Helper::getSuitableQueueForSend(bool SmallQ) {
FilesDetail *FD, *ScanFD;
char *NickWithLowestSends = NULL;
char *FileOfNick = NULL;
int SendsOfNick;

   TRACE();

   if (SmallQ) {
      FD = XGlobal->SmallQueuesInProgress.searchFilesDetailList("*");
   }
   else {
      FD = XGlobal->QueuesInProgress.searchFilesDetailList("*");
   }
   ScanFD = FD;
   while (ScanFD) {
      if (NickWithLowestSends == NULL) {
         // Initialise the nick and its sends.
         NickWithLowestSends = new char[strlen(ScanFD->Nick) + 1];
         strcpy(NickWithLowestSends, ScanFD->Nick);
         // Lets find out its Sends in progress.
         SendsOfNick = XGlobal->SendsInProgress.getFileCountOfNick(NULL, ScanFD->Nick) + XGlobal->DCCSendWaiting.getFileCountOfNick(NULL, ScanFD->Nick);
         // Lets store its FileName.
         FileOfNick = new char[strlen(ScanFD->FileName) + 1];
         strcpy(FileOfNick, ScanFD->FileName);
         COUT(cout << "getSuitableQueueForSend: Init: Nick: " << NickWithLowestSends << " File: " << FileOfNick << " Sends: " << SendsOfNick << endl;)
      }
      else {
         // Get sends of the current ScanFD->Nick, and see if its less
         // than the sends of NickWithLowestSends
         int tmpSendsOfNick = XGlobal->SendsInProgress.getFileCountOfNick(NULL, ScanFD->Nick) + XGlobal->DCCSendWaiting.getFileCountOfNick(NULL, ScanFD->Nick);
         if (tmpSendsOfNick < SendsOfNick) {
            delete [] NickWithLowestSends;
            delete [] FileOfNick;
            SendsOfNick = tmpSendsOfNick;
            NickWithLowestSends = new char[strlen(ScanFD->Nick) + 1];
            strcpy(NickWithLowestSends, ScanFD->Nick);
            FileOfNick = new char[strlen(ScanFD->FileName) + 1];
            strcpy(FileOfNick, ScanFD->FileName);
            COUT(cout << "getSuitableQueueForSend: New: Nick: " << NickWithLowestSends << " File: " << FileOfNick << " Sends: " << SendsOfNick << endl;)
         }
      }
      ScanFD = ScanFD->Next;
   }
   // First free FD.
   XGlobal->QueuesInProgress.freeFilesDetailList(FD);
   FD = NULL;

   // We have NickWithLowestSends, FileOfNick populated, or both NULL
   if (NickWithLowestSends) {
      // Obtain the FD given Nick and FileName
      if (SmallQ) {
         FD = XGlobal->SmallQueuesInProgress.getFilesDetailListNickFile(NickWithLowestSends, FileOfNick);
      }
      else {
         FD = XGlobal->QueuesInProgress.getFilesDetailListNickFile(NickWithLowestSends, FileOfNick);
      }
      delete [] NickWithLowestSends;
      delete [] FileOfNick;
      COUT(cout << "getSuitableQueueForSend: Chosen: Nick: " << FD->Nick << " File: " << FD->FileName << endl;)
   }
   return(FD);
}

// Imbalanced, means, all sends are occupied, and big sends
// are more than allowed ceil(TotalSends/2) and a small Q has an entry
// or small sends are more than allowed floor(TotalSends/2) and
// a big Q has an entry. In such a case, we get the suitable Send FD to
// be stopped and send it a message to disconnect its send
// It returns -1 => no preference.          SEND_FROM_ANY_QUEUE
// It returns  0 => get a send from SmallQ. SEND_FROM_SMALL_QUEUE
// It returns  1 => get a send from BigQ.   SEND_FROM_BIG_QUEUE
// Even if there is no imbalance, the return value of this function dictates
// which Queue to pick a send from. Hence our return value should be proper,
// as its used by TimerThr to pick the Big Q or Small Q for a send.
// When it comes time to actually cancelling a send. We should do it only
// once in IMBALANCE_ALGORITHM_TIME
int Helper::stopImbalancedSends() {
size_t MySmallFileSize;
int MyOverallSends;
int CorrectOverallSends;
FilesDetail *FD, *ScanFD;
FilesDetail *DiscFD, *BigDiscFD, *SmallDiscFD;
int CurrentBigSends;
int CurrentSmallSends;
int CorrectCurrentBigSends;
int CorrectCurrentSmallSends;
int CurrentBigQueues;
int CurrentSmallQueues;
bool SmallSendImbalance;
int retval;
static time_t LastImbalanceDisconnectTime = 0;

   TRACE();

   XGlobal->lock();
   MySmallFileSize = XGlobal->FServSmallFileSize;
   MyOverallSends = XGlobal->FServSendsOverall;
   XGlobal->unlock();

   do {
      CurrentBigQueues = XGlobal->QueuesInProgress.getCount(NULL);
      CurrentSmallQueues = XGlobal->SmallQueuesInProgress.getCount(NULL);
      if ( (CurrentBigQueues == 0) &&
           (CurrentSmallQueues == 0) ) {
         // No Queues present, so nothing to do.
         retval = SEND_FROM_ANY_QUEUE;
         break;
      }
      // We are here => Either Big Q or Small Q has element.

      // We do have to scan even if one of the queues has entries.
      // This is the case when sends are all hogged by Big only or
      // Small only sends, and the other type is Queued.
      // => all big sends, small file Queued, or
      //    all small sends, big file Queued.

      // Sends may or may not be full.
      // We definitely have at least one Big or at least one Small element
      // in Q.

      // lets get the Current Big Sends/Small Sends count, Overall Sends
      FD = XGlobal->SendsInProgress.searchFilesDetailList("*");
      ScanFD = FD;
      CurrentBigSends = 0;
      CurrentSmallSends = 0;
      while (ScanFD) {
         if ( (ScanFD->ManualSend != MANUALSEND_DCCSEND) && (ScanFD->ManualSend != MANUALSEND_FILEPUSH) ) {
            // Exclude direct DCC Sends and File Pushes from imbalance algo.
            if ( (ScanFD->FileSize <= MySmallFileSize) ||
                 (ScanFD->FileSize <= (ScanFD->FileResumePosition + ScanFD->BytesSent + MySmallFileSize)) ) {
               CurrentSmallSends++;
            }
            else CurrentBigSends++;
         }
         ScanFD = ScanFD->Next;
      }
      XGlobal->SendsInProgress.freeFilesDetailList(FD);
      FD = NULL;

      FD = XGlobal->DCCSendWaiting.searchFilesDetailList("*");
      ScanFD = FD;
      while (ScanFD) {
         if ( (ScanFD->ManualSend != MANUALSEND_DCCSEND) && (ScanFD->ManualSend != MANUALSEND_FILEPUSH) ) {
            // Exclude direct DCC Sends and File Pushes from imbalance algo.
            if ( (ScanFD->FileSize <= MySmallFileSize) ||
                 (ScanFD->FileSize <= (ScanFD->FileResumePosition + MySmallFileSize)) ) {
               CurrentSmallSends++;
            }
            else CurrentBigSends++;
         }
         ScanFD = ScanFD->Next;
      }
      XGlobal->DCCSendWaiting.freeFilesDetailList(FD);
      FD = NULL;

      // Now we get the Correct values and totals for the imbalance algo.
      // lets get the Correct Big Sends/Small Sends count.
      CorrectOverallSends = CurrentSmallSends + CurrentBigSends;
      if (CorrectOverallSends < MyOverallSends) {
         // We havent reached full capacity yet.
         CorrectOverallSends = MyOverallSends;
      }
      CorrectCurrentSmallSends = CorrectOverallSends / 2;
      CorrectCurrentBigSends = CorrectOverallSends - CorrectCurrentSmallSends;

      if ( (CurrentBigSends <= CorrectCurrentBigSends) &&
           (CurrentSmallSends <= CorrectCurrentSmallSends) ) {
         // The Current Sends are within the Correct values, so nothing to do
         // So set retval to the most apt Q to pick from.
         COUT(cout << "Helper::stopImbalancedSends -> Sends within correct values CurrentBigSends: " << CurrentBigSends << " CorrectCurrentBigSends: " << CorrectCurrentBigSends << " CurrentSmallSends: " << CurrentSmallSends << " CorrectCurrentSmallSends: " << CorrectCurrentSmallSends;)

         if ( (CurrentBigQueues != 0) &&
              (CurrentSmallQueues == 0) ) {
            // Only Big Q is occupied
            retval = SEND_FROM_BIG_QUEUE;
            COUT(cout << " SEND_FROM_BIG_QUEUE" << endl;)
            break;
         }

         if ( (CurrentSmallQueues != 0) &&
              (CurrentBigQueues == 0) ) {
            // Only Small Q is occupied.
            retval = SEND_FROM_SMALL_QUEUE;
            COUT(cout << " SEND_FROM_SMALL_QUEUE" << endl;)
            break;
         }

         // Both Qs have elements.
         if (CurrentSmallSends < CorrectCurrentSmallSends) {
            retval = SEND_FROM_SMALL_QUEUE;
            COUT(cout << " SEND_FROM_SMALL_QUEUE" << endl;)
            break;
         }
         if (CurrentBigSends < CorrectCurrentBigSends) {
            retval = SEND_FROM_BIG_QUEUE;
            COUT(cout << " SEND_FROM_BIG_QUEUE" << endl;)
            break; 
         }

         // We are here -> CurrentSmallSends == CorrectCurrentSmallSends.
         // and CurrentBigSends == CorrectCurrentBigSends.
         // In this scenario we return the Q which is the correct Q if
         // an additional send slot is opened up, as would happen in the
         // case when overallmin cps is set.
         if ( ((CorrectOverallSends + 1) % 2) == 0) {
            // Number of Sends will be Even => send slot for small will be open.
            retval = SEND_FROM_SMALL_QUEUE;
            COUT(cout << " SEND_FROM_SMALL_QUEUE" << endl;)
         }
         else {
            // Number of Sends will be Odd => send slot for big Q will be open
            retval = SEND_FROM_BIG_QUEUE;
            COUT(cout << " SEND_FROM_BIG_QUEUE" << endl;)
         }
         break;
      }

      // We are here => an imbalance exists.
      COUT(cout << "Helper::stopImbalancedSends -> Imbalance detected CurrentBigSends: " << CurrentBigSends << " CorrectCurrentBigSends: " << CorrectCurrentBigSends << " CurrentSmallSends: " << CurrentSmallSends << " CorrectCurrentSmallSends: " << CorrectCurrentSmallSends << " CurrentSmallQueues: " << CurrentSmallQueues << " CurrentBigQueues: " << CurrentBigQueues;)
      if (CurrentSmallSends > CorrectCurrentSmallSends) {
         SmallSendImbalance = true;
         COUT(cout << " Small Send imbalance detected." << endl;)
      }
      else {
         SmallSendImbalance = false;
         COUT(cout << " Big Send imbalance detected." << endl;)
      }

      // If there is a BigSend Imbalance but no small Q exists or
      // If there is a SmallSend imbalance but no big Q exists
      // this is an imbalance, but no send needs to be canceled.
      if ( ((SmallSendImbalance == true) && (CurrentBigQueues == 0)) ||
           ((SmallSendImbalance == false) && (CurrentSmallQueues == 0))
         ) {
         COUT(cout << "Helper::stopImbalancedSends -> Sends imbalanced but OK. CurrentBigSends: " << CurrentBigSends << " CorrectCurrentBigSends: " << CorrectCurrentBigSends << " CurrentSmallSends: " << CurrentSmallSends << " CorrectCurrentSmallSends: " << CorrectCurrentSmallSends;)

         if (SmallSendImbalance) {
            // Only Small Q is occupied
            retval = SEND_FROM_SMALL_QUEUE;
            COUT(cout << " SEND_FROM_SMALL_QUEUE" << endl;)
            break;
         }
         else {
            // Only Big Q is occupied
            retval = SEND_FROM_BIG_QUEUE;
            COUT(cout << " SEND_FROM_BIG_QUEUE" << endl;)
            break;
         }
      }       

      // Boolean SmallSendImbalance has the correct imbalance set.
      // Thinking more on this, its OK to have a SmallSendImbalance as
      // it will correct itself as soon the small send is over.
      // Thinking some more, we should disconnect the small send as well
      // as some other small send will indeed get over soon to resume it.

      // So we go through SendsInProgress looking for the most apt one
      // to disconnect.
      FD = XGlobal->SendsInProgress.searchFilesDetailList("*");
      ScanFD = FD;
      SmallDiscFD = NULL;
      BigDiscFD = NULL;

      // We do the actual diconnection only once in IMBALANCE_ALGORITHM_TIME
      time_t CurrentTime = time(NULL);
      if (LastImbalanceDisconnectTime + IMBALANCE_ALGORITHM_TIME < CurrentTime) {
         while (ScanFD) {
            if ( (ScanFD->ManualSend != MANUALSEND_DCCSEND) && (ScanFD->ManualSend != MANUALSEND_FILEPUSH) ) {
               // Exclude direct DCC Sends and File Pushes from imbalance algo.
               if ( (ScanFD->FileSize <= MySmallFileSize) ||
                    (ScanFD->FileSize <= (ScanFD->FileResumePosition + ScanFD->BytesSent + MySmallFileSize)) ) {
                  // This is a small file send.
                  // We note the last entry as the entry to be disconnected, as that
                  // was the most recent one started.
                  SmallDiscFD = ScanFD;
               }
               else {
                  // This is a big file send.
                  // We note the last entry as the entry to be disconnected, as that
                  // was the most recent one started.
                  BigDiscFD = ScanFD;
               }
            }
            ScanFD = ScanFD->Next;
         }
      }

      if (SmallSendImbalance) {
         DiscFD = SmallDiscFD;
         retval = SEND_FROM_BIG_QUEUE;
      }
      else {
         DiscFD = BigDiscFD;
         retval = SEND_FROM_SMALL_QUEUE;
      }

      if (DiscFD) {
         // We need to disconnect this send.
         LastImbalanceDisconnectTime = CurrentTime;
         XGlobal->SendsInProgress.updateFilesDetailNickFileConnectionMessage(DiscFD->Nick, DiscFD->FileName, CONNECTION_MESSAGE_DISCONNECT_REQUEUE_NOINCRETRY);
            COUT(cout << "Helper::stopImbalancedSends -> Stopping Send Nick: " << DiscFD->Nick << " File: " << DiscFD->FileName << " FileSize: " << DiscFD->FileSize << " Resume: " << DiscFD->FileResumePosition << " BytesSend: " << DiscFD->BytesSent << endl;)
      }

      // Now free up FD.
      XGlobal->SendsInProgress.freeFilesDetailList(FD);
      FD = NULL;

   } while (false);

   return(retval);
}

// Gets the sum of the UploadBPS's from the SendsInProgress.
// Used by TimerThr, to maintain OverallMinBPS.
size_t Helper::getCurrentOverallUploadBPSFromSends() {
FilesDetail *FD, *ScanFD;
size_t overallbps = 0;

   TRACE();

   FD = XGlobal->SendsInProgress.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD) {
      overallbps = overallbps + ScanFD->UploadBps;
      ScanFD = ScanFD->Next;
   }

   XGlobal->SendsInProgress.freeFilesDetailList(FD);
   return(overallbps);
}

// Used by FromServerThr when a nick is kicked/part/quit from CHANNEL_MAIN
void Helper::removeNickFromAllFDs(char *nick) {
char *message = new char[1024];
FilesDetail *FD;

   TRACE();

   // Remove Entries of this nick from FilesDB
   XGlobal->FilesDB.delFilesOfNick(nick);

   // Disconnect all sends of this nick.
   if (XGlobal->SendsInProgress.updateFilesDetailAllNickConnectionMessage(nick, CONNECTION_MESSAGE_DISCONNECT_NOREQUEUE)) {
      COUT(cout << "Helper::removeNickFromAllFDs: SendsInProgress -> Message sent to disconnect: " << nick << endl;)
      sprintf(message, "NOTICE %s :Send Aborted. You must stay in channel.",
                        nick);
      sendLineToNick(nick, message);
   }
   // Remove this nick's entries from DCCSendWaiting.
   FD = XGlobal->DCCSendWaiting.getFilesDetailListOfNick(nick);
   if (FD) {
      XGlobal->DCCSendWaiting.delFilesOfNick(nick);
      COUT(cout << "Helper::removeNickFromAllFDs: DCCSendWaiting: removed: " << nick << endl;)
      XGlobal->DCCSendWaiting.freeFilesDetailList(FD);
      sprintf(message, "NOTICE %s :Send Aborted. You must stay in channel.", nick);
      sendLineToNick(nick, message);
   }

   // Remove this nick's entries from the Big Queues.
   FD = XGlobal->QueuesInProgress.getFilesDetailListOfNick(nick);
   if (FD) {
      XGlobal->QueuesInProgress.delFilesOfNick(nick);
      COUT(cout << "Helper::removeNickFromAllFDs: QueuesInProgress: removed: " << nick << endl;)
      XGlobal->QueuesInProgress.freeFilesDetailList(FD);
      sprintf(message, "NOTICE %s :Removed from Queue. You must stay in channel.", nick);
      sendLineToNick(nick, message);
   }

   // Remove this nicks entries from the Small Queues.
   FD = XGlobal->SmallQueuesInProgress.getFilesDetailListOfNick(nick);
   if (FD) {
      XGlobal->SmallQueuesInProgress.delFilesOfNick(nick);
      COUT(cout << "Helper::removeNickFromAllFDs: SmallQueuesInProgress: removed: " << nick << endl;)
      XGlobal->SmallQueuesInProgress.freeFilesDetailList(FD);
      sprintf(message, "NOTICE %s :Removed from Queue. You must stay in channel.", nick);
      sendLineToNick(nick, message);
   }

   // Disconnect all of this nick's File Server access.
   if (XGlobal->FileServerInProgress.updateFilesDetailAllNickConnectionMessage(nick, CONNECTION_MESSAGE_DISCONNECT_FSERV)) {
      COUT(cout << "Helper::removeNickFromAllFDs: FileServerInProgress: message sent to disconnect " << nick << endl;)
      sprintf(message, "NOTICE %s :Removed from FileServer. You must stay in channel.", nick);
      sendLineToNick(nick, message);
   }

   delete [] message;
}

// Below is related to update the RecordDownloadBPS, and as a side effect
// move the Upload CAPS around if they are set.
// We update the record, if record is higer than what we have currently.
// If adjust is false, cap is sanitized only if a new record is being set.
// If adjust is true, cap is sanitized even if a new record is not set.
// used to sanitize entry input by user by /cap command.
void Helper::updateRecordDownloadAndAdjustUploadCaps(size_t record, bool adjust) {
size_t new_record = 0;
size_t per_user_upload;
size_t max_upload;
size_t overall_mincps;
long overall_sends;

   TRACE();

   XGlobal->lock();
   if (XGlobal->RecordDownloadBPS < (double) record) {
      XGlobal->RecordDownloadBPS = (double) record;
      new_record = record;
   }
   per_user_upload = XGlobal->PerTransferMaxUploadBPS;
   max_upload = XGlobal->OverallMaxUploadBPS;
   overall_sends = XGlobal->FServSendsOverall;
   overall_mincps = XGlobal->OverallMinUploadBPS;
   if (adjust) {
      new_record = (size_t) XGlobal->RecordDownloadBPS;
   }
   XGlobal->unlock();

   // if a new_record is set in the Download, sanitize the upload caps.
   // Things to sanitize =>
   // 1. if set, overalluploadmaxcps = new_record/4 or higher.
   // 2. if overalluploadmaxcps is set, overallmincps = overalluploadmaxcps -10
   // 3. if set, per user upload = overalluploadmaxcps/# of sends or higher.

   do {
      if (new_record == 0) break;

      if ( (max_upload != 0) && (max_upload < (new_record / 4)) ) {
         // max upload cap is set -> sanitize it.
         max_upload = new_record / 4;
         XGlobal->lock();
         XGlobal->OverallMaxUploadBPS = max_upload;
         XGlobal->unlock();
      }

      if (max_upload != 0) {
         // max upload is set -> kick in overallmincps.
         overall_mincps = max_upload - 10000;
         if (overall_mincps < 0) overall_mincps = 0;
         XGlobal->lock();
         XGlobal->OverallMinUploadBPS = overall_mincps;
         XGlobal->unlock();
      }

      if ( (per_user_upload != 0) && (per_user_upload < (new_record / overall_sends)) ) {
         // per user upload cap is set -> sanitize it.
         per_user_upload = new_record / overall_sends;

         // This just implies that there is an implicit overall cap of
         // per_user_upload * overall_sends. Hence we should kick in
         // overallmincps.
         overall_mincps = (new_record / 4) - 10000;
         if (overall_mincps <= 0) overall_mincps = 0;
         
         XGlobal->lock();
         XGlobal->PerTransferMaxUploadBPS = per_user_upload;
         XGlobal->OverallMinUploadBPS = overall_mincps;
         XGlobal->unlock();
      }

   } while (false);

   // Just make sure overallmincps is at least 10000 bytes less than
   // max_upload. This is fall back when new_record is 0.
   // but we get called. Part of sanity checking.
   if ( (overall_mincps != 0) && (max_upload != 0) &&
        (max_upload < (overall_mincps + 10000)) ) {
      // overall_mincps is set, max_upload is set
      // and overall_mincps is very close to max_upload.
      overall_mincps = max_upload - 10000;
      if (overall_mincps < 0) overall_mincps = 0;
      XGlobal->lock();
      XGlobal->OverallMinUploadBPS = overall_mincps;
      XGlobal->unlock();
   }
}

// Used by TimerThr()
// Converts SEND_FROM_SMALL_QUEUE, SEND_FROM_BIG_QUEUE to boolean SmallQ
bool Helper::convertQueueIndicatorToSmallQueueBoolean(int q_indicator) {
static bool SmallQ = false;

   TRACE();

   switch (q_indicator) {
      case SEND_FROM_SMALL_QUEUE:
      SmallQ = true;
      break;

      case SEND_FROM_BIG_QUEUE:
      SmallQ = false;
      break;

      case SEND_FROM_ANY_QUEUE:
      default:
      // We toggle it each call, so that send picks up a send from a
      // random Q to send from.
      SmallQ = !SmallQ;
      break;
   }
   return(SmallQ);
}

// Check if there are open sends and if so push a send out from the
// Q dictated by SmallQ.
// Assumes we are connected to server and joined in CHANNEL_MAIN.
void Helper::checkAndNormalSendFromQueue(bool SmallQ, time_t ChannelJoinTime) {
long MyMaxSends;
int retval;
FilesDetail *FD;
static time_t NormalRetryTime = 0;
time_t CurrentTime;

   TRACE();

   // If nothing in Q, nothing to do.
   if ( (XGlobal->QueuesInProgress.getCount(NULL) + 
         XGlobal->SmallQueuesInProgress.getCount(NULL)) == 0) return;

   XGlobal->lock();
   MyMaxSends = XGlobal->FServSendsOverall;
   XGlobal->unlock();

   // All Send Slots occupied, nothing to do.
   if ( (XGlobal->DCCSendWaiting.getCount(NULL) +
         XGlobal->SendsInProgress.getCount(NULL)) >= MyMaxSends) return;

   // Sends are underfed, so lets get the most suitable Queued Item out.
   FD = getSuitableQueueForSend(SmallQ);
   do {
      if (FD == NULL) break;

      // Check if its atleast 60 seconds since we joined channel.
      // So that we got the full nick list. Dont want the deserving
      // nick to be disqualified with a not in channel message.
      CurrentTime = time(NULL);
      if (CurrentTime < (ChannelJoinTime + 60)) {
         NormalRetryTime = 0;
         break;
      }

      // Check if we need to delay the send if its RetryCount != 0
      if (FD->RetryCount == 0) {
         // First time initiated send.
         dccSend(FD, SmallQ);
         NormalRetryTime = 0;
         break;
      }

      // This is a retry send.
      // So we are coming here first time, in which case
      // we set RetryTime.
      if (NormalRetryTime == 0) {
         NormalRetryTime = CurrentTime + 60; // wait 1 minute.
         COUT(cout << "TimerThr: NormalRetryTime set: " << NormalRetryTime << endl;)
         break;
      }

      // We have already set RetryTime, so we check if
      // we are ready to send now.
      if (CurrentTime < NormalRetryTime) break;

      COUT(cout << "TimerThr: NormalRetryTime exceeded CurrentTime, ready to send. NormalRetryTime: " << NormalRetryTime << " CurrentTime: " << CurrentTime << endl;)
      NormalRetryTime = 0;
      dccSend(FD, SmallQ);

   } while (false);

   if (FD) {
      XGlobal->QueuesInProgress.freeFilesDetailList(FD);
   }
}

void Helper::checkForOverallMinCPSAndSendFromQueue(bool SmallQ, time_t ChannelJoinTime) {
time_t CurrentTime;
size_t min_cap, bps;
static time_t LastTimeWhenItWasAboveMinCAP = 0;
FilesDetail *FD;

   TRACE();

   do {
      // Check if its atleast 60 seconds since we joined channel.
      // So that we got the full nick list. Dont want the deserving
      // nick to be disqualified with a not in channel message.
      CurrentTime = time(NULL);
      if (CurrentTime < (ChannelJoinTime + 60)) {
         LastTimeWhenItWasAboveMinCAP = CurrentTime;
         break;
      }
 
      // If OverallMinUploadBPS is not set, we dont need to do anything.
      XGlobal->lock();
      min_cap = XGlobal->OverallMinUploadBPS;
      XGlobal->unlock();
      if (min_cap == 0) {
         LastTimeWhenItWasAboveMinCAP = CurrentTime;
         break;
      }

      // See if we are maintaining at least the OverallMinUploadBPS
      bps = getCurrentOverallUploadBPSFromSends();
      if (bps >= min_cap) {
         LastTimeWhenItWasAboveMinCAP = CurrentTime;
         // COUT(cout << "TimerThr: MINUPLOADCPS. resetting LastTimeWhenItWasAboveMinCAP to CurrentTime = " << CurrentTime << " as bps receieved at that time is " << bps << endl;)
         break;
      }

      // We are here => min_cap is in effect.
      #define MIN_UPLOAD_CPS_TIMEPERIOD_FOR_SEND    300
      if (CurrentTime < (LastTimeWhenItWasAboveMinCAP + MIN_UPLOAD_CPS_TIMEPERIOD_FOR_SEND)) {
         // Though we can push one out the TIMEPERIOD is not yet up.
         // COUT(cout << "TimerThr: MINUPLOADCPS. time diff since it maintained min cps: " << CurrentTime - LastTimeWhenItWasAboveMinCAP << " so not yet pushing one send." << endl;)
         break;
      }

      // We can push one out.
      FD = getSuitableQueueForSend(SmallQ);
      if (FD) {
         COUT(cout << "TimerThr: MINUPLOADCPS. time diff since it maintained min cps: " << CurrentTime - LastTimeWhenItWasAboveMinCAP << " so pushing one send." << endl;)
         LastTimeWhenItWasAboveMinCAP = CurrentTime;
         dccSend(FD, SmallQ);
         XGlobal->QueuesInProgress.freeFilesDetailList(FD);
         FD = NULL;
      }

   } while (false);
}

void Helper::checkAndManualSendFromQueue(time_t ChannelJoinTime) {
FilesDetail *FD = NULL;
static time_t ManualRetryTime = 0;
time_t CurrentTime;
bool SmallQueue = false;

   TRACE();

   do {
      // Only attempt if its been at least 60 seconds since we joined
      // Channel.
      CurrentTime = time(NULL);
      if (CurrentTime < (ChannelJoinTime + 60)) {
         ManualRetryTime = 0;
         break;
      }

      FD = XGlobal->QueuesInProgress.getFilesDetailAtIndex(1);

      if ( (FD == NULL) ||
           ((FD->ManualSend != MANUALSEND_DCCSEND) && (FD->ManualSend != MANUALSEND_FILEPUSH)) ) {
         // If no FD or FD but not a Manual Push or a DCC Send, check
         // the SmallQ.
         if (FD) {
            XGlobal->QueuesInProgress.freeFilesDetailList(FD);
            FD = NULL;
         }
         FD = XGlobal->SmallQueuesInProgress.getFilesDetailAtIndex(1);
         if ( (FD == NULL) ||
              ((FD->ManualSend != MANUALSEND_DCCSEND) && (FD->ManualSend != MANUALSEND_FILEPUSH)) ) {
            break;
         }
         SmallQueue = true;
      }
      // We are here, we got a valid FD which is Manual Push or DCC Send.

      // Check if we need to delay the send if its RetryCount != 0
      if (FD->RetryCount == 0) {
         // First time initiated send.
         dccSend(FD, SmallQueue);
         ManualRetryTime = 0;
         break;
      }

      // This is a retry send.
      // So we are coming here first time, in which case
      // we set RetryTime.
      if (ManualRetryTime == 0) {
         ManualRetryTime = CurrentTime + 60; // wait 1 minute.
         COUT(cout << "TimerThr: ManualRetryTime set: " << ManualRetryTime << endl;)
         break;
      }

      // We have already set RetryTime, so we check if
      // we are ready to send now.
      if (CurrentTime < ManualRetryTime) break;

      COUT(cout << "TimerThr: ManualRetryTime exceeded CurrentTime, ready to send. ManualRetryTime: " << ManualRetryTime << " CurrentTime: " << CurrentTime << endl;)
      ManualRetryTime = 0;
      dccSend(FD, SmallQueue);

   } while (false);

   if (FD) {
      XGlobal->QueuesInProgress.freeFilesDetailList(FD);
   }
}

// Used by FromServerThr() to handle IC_PRIVMSG and IC_NOTICE of XDCC
// File Listings.
void Helper::processXDCCListing(char *xdcc_line) {
FServParse TriggerParse;
FilesDetail *FD, *NoFileFD, *TrigFD;
const char *tmpstr;

   TRACE();

   TriggerParse = xdcc_line;

   if (TriggerParse.getTriggerType() == XDCC) {
      // If xdcc line, update FilesList with informaion.
      FD = new FilesDetail;
      XGlobal->FilesDB.initFilesDetail(FD);
      FD->TriggerType = XDCC;
      FD->FileSize = TriggerParse.getFileSize();
      FD->PackNum = TriggerParse.getPackNumber();
      tmpstr = TriggerParse.getTriggerNick();
      FD->Nick = new char[strlen(tmpstr) + 1];
      strcpy(FD->Nick, tmpstr);
      tmpstr = TriggerParse.getFileName();
      FD->FileName = new char[strlen(tmpstr) + 1];
      strcpy(FD->FileName, tmpstr);
      FD->DownloadState = DOWNLOADSTATE_SERVING;

      // Now lets fill in the info regarding its sends and qs.
      TrigFD = XGlobal->FilesDB.getFilesDetailListNickFile(FD->Nick, "TriggerTemplate");
      if (TrigFD) {
         FD->ClientType = TrigFD->ClientType;
         FD->CurrentSends = TrigFD->CurrentSends;
         FD->TotalSends = TrigFD->TotalSends;
         FD->CurrentQueues = TrigFD->CurrentQueues;
         FD->TotalQueues = TrigFD->TotalQueues;
         XGlobal->FilesDB.freeFilesDetailList(TrigFD);
      }

      // Lets delete the "No Files Present" entry.
      XGlobal->FilesDB.delFilesDetailNickFile(FD->Nick, "No Files Present");

      XGlobal->FilesDB.addFilesDetail(FD);
   }
   else if (TriggerParse.getTriggerType() == SENDS_QS_LINE) {
      // This is a XDCC send/q information line.
      FD = new FilesDetail;
      XGlobal->FilesDB.initFilesDetail(FD);
      FD->TriggerType = XDCC;
      tmpstr = TriggerParse.getTriggerNick();
      FD->Nick = new char[strlen(tmpstr) + 1];
      strcpy(FD->Nick, tmpstr);
      FD->FileName = new char[16];
      strcpy(FD->FileName, "TriggerTemplate");

      // First delete the previous info line if present.
      XGlobal->FilesDB.delFilesDetailNickFile(FD->Nick, FD->FileName);

      FD->ClientType = IRCNICKCLIENT_IROFFER; // Iroffer
      // Update the NickList too with this information
      XGlobal->NickList.setNickClient(FD->Nick, FD->ClientType);

      FD->CurrentSends = TriggerParse.getCurrentSends();
      FD->TotalSends = TriggerParse.getTotalSends();
      FD->CurrentQueues = TriggerParse.getCurrentQueues();
      FD->TotalQueues = TriggerParse.getTotalQueues();

      // Copy to modify and add later for No Files Present.
      NoFileFD = XGlobal->FilesDB.copyFilesDetail(FD);
      XGlobal->FilesDB.addFilesDetail(FD);
      FD = NULL;

      // Lets add in the "No Files Present" entry with State 'S'
      // Will be removed when a file entry is added.
      delete [] NoFileFD->FileName;
      NoFileFD->FileName = new char[17];
      strcpy(NoFileFD->FileName, "No Files Present");
      NoFileFD->DownloadState = DOWNLOADSTATE_SERVING;
      NoFileFD->FileSize = 1;
      XGlobal->FilesDB.delFilesDetailNickFile(NoFileFD->Nick, NoFileFD->FileName);
      XGlobal->FilesDB.addFilesDetail(NoFileFD);
      // This entry will be first deleted once we get a file.
   }
   else if (TriggerParse.getTriggerType() == IROFFER_FIREWALL_LINE) {
      tmpstr = TriggerParse.getTriggerNick();

      if (TriggerParse.isIrofferFirewalled()) {
        
         XGlobal->NickList.setNickFirewall((char *) tmpstr, IRCNICKFW_YES);
         COUT(cout << "Nick: " << tmpstr << " marked as IRCNICKFW_YES" << endl;)
      }
      else {
         XGlobal->NickList.setNickFirewall((char *) tmpstr, IRCNICKFW_NO);
         COUT(cout << "Nick: " << tmpstr << " marked as IRCNICKFW_NO" << endl;)
      }
   }
} 

// Used by FromServerThr() to mark nick as an MM client.
// Can be done FromServerThr, as now its a single Line.
void Helper::markAsMMClient(char *nick) {
IRCNickLists &NL = XGlobal->NickList;

   TRACE();
   NL.setNickClient(nick, IRCNICKCLIENT_MASALAMATE);
}

// Used by FromServerThr() to handle IC_MODE, IC_JOIN, IC_NICKLIST
// events in CHANNEL_MM
// Check we are OP in CHANNEL_MM.
// Make 1st and last nick of CHANNEL_MM as op. And then
// if we are not the first or the last, we deop ourselves.
// our_nick passed is our own nick.
void Helper::doOpDutiesIfOpInChannelMM(char *our_nick) {
char temp_nick[128];
char line_buf[256];
bool deop = true;
IRCNickLists &NL = XGlobal->NickList;
unsigned int nick_mode;
int nick_count;

   TRACE();

   nick_mode = NL.getNickMode(CHANNEL_MM, our_nick);
   do {
      if (IS_OP(nick_mode) == false) break;

      // So I am OP in CHANNEL_MM. Make sure of the below:
      // Make the first and last in list as OPs.
      nick_count = NL.getNickCount(CHANNEL_MM);
      NL.getNickInChannelAtIndex(CHANNEL_MM, 1, temp_nick);
      nick_mode = NL.getNickMode(CHANNEL_MM, temp_nick);
      if (IS_OP(nick_mode)) {
         if (strcasecmp(our_nick, temp_nick) == 0) deop = false;
      }
      else {
         sprintf(line_buf, "MODE %s +o %s", CHANNEL_MM, temp_nick);
         XGlobal->IRC_ToServer.putLine(line_buf);
      }

      // Now for the last one.
      NL.getNickInChannelAtIndex(CHANNEL_MM, nick_count, temp_nick);
      nick_mode = NL.getNickMode(CHANNEL_MM, temp_nick);
      if (IS_OP(nick_mode)) {
         if (strcasecmp(our_nick, temp_nick) == 0) deop = false;
      }
      else {
         sprintf(line_buf, "MODE %s +o %s", CHANNEL_MM, temp_nick);
         XGlobal->IRC_ToServer.putLine(line_buf);
      }

      // Now we deop ourselves if deop is true.
      if (deop) {
         sprintf(line_buf, "MODE %s -o %s", CHANNEL_MM, our_nick);
         XGlobal->IRC_ToServer.putLine(line_buf);
      }
   } while (false);

}

// Used by FromServerThr() to calculate the Next Ad Time
// ADSYNC algo.
// ad_nick is the Advertisement of the MM Nick spotted in CHANNEL_MAIN
// our_nick is our current Nick Name.
// Nicks in CHANNEL_MM are not marked as MM clients.
// Hence always use CHANNEL_MM for calculations.
void Helper::calculateFServAdTime(char *ad_nick, char *our_nick) {
int x, N, m;
time_t cur_time, new_ad_time;
IRCNickLists &NL = XGlobal->NickList;

   TRACE();

   x = NL.getMMNickIndex(CHANNEL_MAIN, ad_nick);
   N = NL.getMMNickCount(CHANNEL_MAIN);
   m = NL.getMMNickIndex(CHANNEL_MAIN, our_nick);
   cur_time = time(NULL);
   new_ad_time = 0;

   if (x > 0) {
      // Assume its an MM ad.
      COUT(cout << "ADSYNC: MM Ad spotted: x: " << x << " m: " << m << " N: " << N << endl;)

      // Lets calculate our next Ad trigger delay time.
      if (m > 0) {
         // set the new Ad time for us.
         if (m < x) {
            new_ad_time = cur_time + (N - x + m) * FSERV_RELATIVE_AD_TIME;
         }
         else if (m > x) {
            new_ad_time = cur_time + (m - x) * FSERV_RELATIVE_AD_TIME;
         }
         else {
            // Shouldnt come here at all. We dont see our own ad
            COUT(cout << "ADSYNC: !!!! m == x !!!! - should never happen" << endl;)
            new_ad_time = cur_time + FSERV_INITIAL_AD_TIME;
         }
      }
   }

   if (new_ad_time != 0) {
      XGlobal->lock();
      XGlobal->FServAdTime = new_ad_time;
      XGlobal->unlock();
      COUT(cout << "ADSYNC: MyNick: " << our_nick << " AdNick: " << ad_nick << " N: " << N << " m: " << m << " x: " << x << " cur_time: " << cur_time << " new_ad_time: " << new_ad_time << " I will ad next in: " << new_ad_time - cur_time << " seconds" << endl;)
   }
}

// Used by TabBookWindow, to select the channel where !list should
// be done for the given nick.
// apt_channel is assumed to be allocated by caller and has enough space.
// issued_window = Window Name where the !list <nick> was typed.
void Helper::getAptChannelForNickToIssueList(char *nick, char *issued_window, char *apt_channel) {
IRCNickLists &NL = XGlobal->NickList;
int i, chan_count;

   TRACE();

   apt_channel[0] = '\0';

   if ( (nick == NULL) || (apt_channel == NULL) || (issued_window == NULL) ) return;

   if ( (strlen(nick) == 0) || (strlen(issued_window) == 0) ) return;

   // If we issued in a Channel Window, then most apt channel is that same
   // channel if the nick is present in it, unless if its "#Masala-Chat"
   if ( (issued_window[0] == '#') &&
        strcasecmp(issued_window, CHANNEL_CHAT)
      ) {
      if (NL.isNickInChannel(issued_window, nick)) {
         strcpy(apt_channel, issued_window);
         return;
      }
   }

   if (NL.isNickInChannel(CHANNEL_MAIN, nick)) {
      // If nick is present in CHANNEL_MAIN, that is the most apt
      // channel.
      strcpy(apt_channel, CHANNEL_MAIN);
      return;
   }

   // Lets go thru the channels and see if he is any of our known channels.
   chan_count = NL.getChannelCount();
   for (i = 1; i <= chan_count; i++) {
      NL.getChannelName(apt_channel, i);
      // Skip CHANNEL_MAIN (already checked) and CHANNEL_CHAT
      if (strcasecmp(apt_channel, CHANNEL_MAIN) == 0) continue;
      if (strcasecmp(apt_channel, CHANNEL_CHAT) == 0) continue;
      
      if (NL.isNickInChannel(apt_channel, nick)) {
         // This is the apt channel for this nick for listing.
         return;
      }
   }
}

// Get the SwarmIndex of Filename being swarmed.
// Returns -1 if doesnt match and Swarm.
int Helper::getSwarmIndexGivenFileName(const char *FileName) {
int SwarmIndex = -1;

   TRACE();

   for (int i = 0; i < SWARM_MAX_FILES; i++) {
      if (XGlobal->Swarm[i].isFileBeingSwarmed(FileName) == false) continue;
      SwarmIndex = i;
      break;
   }

   return(SwarmIndex);
}

// Called by handshake routines on successful HS, to go ahead and
// exchange the NL List/Message.
// if true returned -> successful exchange.
// if false then some problem.
// Note that it populates ToBeTried from the NL obtained.
// Note that SwarmIndex should not be -1.
bool Helper::nicklistWriteReadSwarmConnection(int SwarmIndex, char *SwarmNick, TCPConnect *Connection) {
bool retvalb;
char NLBuffer[8192];
size_t retval;
LineParse LineP;
const char *parseptr;
char MyNick[64];
unsigned long MyIP;

   TRACE();

   COUT(cout << "Entering:: Helper::nicklistWriteReadSwarmConnection" << endl;)

   retvalb = XGlobal->Swarm[SwarmIndex].ConnectedNodes.generateStringNL(NLBuffer, sizeof(NLBuffer) - 1);

   if (retvalb) {
      // Send it over the wire.
      retval = Connection->writeData(NLBuffer, strlen(NLBuffer), SWARM_CONNECTION_TIMEOUT);
      if (retval != strlen(NLBuffer)) return(false);
   }

   retvalb = XGlobal->Swarm[SwarmIndex].YetToTryNodes.generateStringNL(NLBuffer, sizeof(NLBuffer) - 1);

   if (retvalb) {
      // Send it over the wire.
      retval = Connection->writeData(NLBuffer, strlen(NLBuffer), SWARM_CONNECTION_TIMEOUT);
      if (retval != strlen(NLBuffer)) return(false);
   }

   // Send the End of NL transaction.
   retval = Connection->writeData("NL ND\n", 6, SWARM_CONNECTION_TIMEOUT);
   if (retval != 6) return(false);

   COUT(cout << "Finished sending NL ND" << endl;)

   // We have successfully sent our NL list out.
   MyIP = XGlobal->getIRC_IP(NULL);
   XGlobal->resetIRC_IP_Changed();
   XGlobal->getIRC_Nick(MyNick);
   XGlobal->resetIRC_Nick_Changed();

   int loop_counter = 0;
   retvalb = false;
   do {
      // Now we try to read what NL he is going to send us back.
      // Loop till we get a "NL ND", which terminates the NL exchange.
      // Also dont loop more than 5 loops.

      retval = Connection->readLine(NLBuffer, sizeof(NLBuffer) - 1, SWARM_CONNECTION_TIMEOUT);
      if (retval <= 0) break;

      COUT(cout << "readLine: " << NLBuffer << endl;)

      if (strcasecmp("NL ND", NLBuffer) == 0) {
         // Good exit.
         retvalb = true;
         break;
      }

      // Break this line up and add to the ToBeTried Nodes.
      LineP = NLBuffer;
      int TotalWords = LineP.getWordCount();

      // TotalWords has to be >= 3 and cannot be even.
      if ( (TotalWords < 3) || ((TotalWords % 2) == 0) ) break;

      int CurrentWord = 2;
      do {
         unsigned long new_ip;

         if (CurrentWord > TotalWords) break;

         parseptr = LineP.getWord(CurrentWord + 1);
         new_ip = strtoul(parseptr, NULL, 16);
         parseptr = LineP.getWord(CurrentWord);

         // First check if this is not us itself.
         if ( (strcasecmp(parseptr, MyNick) == 0) ||
              (MyIP == new_ip) ) {
            CurrentWord += 2;
            continue;
         }

         // now check if this parseptr, new_ip pair is not already in
         // Connected or ToBeTried or TriedAndFailed.
         if ( (XGlobal->Swarm[SwarmIndex].ConnectedNodes.isSwarmNodePresent((char *) parseptr, new_ip)) ||
              (XGlobal->Swarm[SwarmIndex].YetToTryNodes.isSwarmNodePresent((char *) parseptr, new_ip)) ||
              (XGlobal->Swarm[SwarmIndex].TriedAndFailedNodes.isSwarmNodePresent((char *) parseptr, new_ip)) ) {
            CurrentWord += 2;
            continue;
         }

         // Ok, we are here, so safe to add it in YetToTryNodes
         XGlobal->Swarm[SwarmIndex].YetToTryNodes.addToSwarmNodeNickIPState((char *) parseptr, new_ip, SWARM_NODE_NOT_TRIED);
         CurrentWord += 2;

         COUT(cout << "Added: " << parseptr << " with ip: " << new_ip << endl;)

      } while (true);

      loop_counter++;
      if (loop_counter >= 5) break;
   } while (true);

   COUT(cout << "Exiting:: Helper::nicklistWriteReadSwarmConnection. retvalb: " << retvalb << endl;)
   return(retvalb);
}

// Used by SwarmThr/DCCServerThr/DCCThr to do swarm HandShake.
// Does the HS and if successful calls nicklistReadWriteSwarmConnection
// Return true if successful Handshake. (Connection object in SwarmNodeList)
// false otherwise.
// On failure, caller should disconnect and delete Connection.
//  it adds failure cases in TriedButFailed.
// Note that SwarmIndex should not be -1.
bool Helper::handshakeWriteReadSwarmConnection(int SwarmIndex, char *SwarmNick, TCPConnect *Connection) {
size_t SwarmNickFileSize;
size_t SwarmFileSize;
char SwarmNickFileSHA[41];
bool retvalb = false;
size_t retval;
int track_node_state;
char HSBuffer[512];
LineParse LineP;
const char *parseptr;

   TRACE();
   if ( (SwarmNick == NULL) ||
        (strlen(SwarmNick) == 0) ||
        (Connection == NULL) ||
        (Connection->state() != TCP_ESTABLISHED) ) {
      if (Connection) {
         delete Connection;
      }
      return(retvalb);
   }
   COUT(cout << "Helper::handshakeWriteReadSwarmConnection" << endl;)

   do {
      // First write out our HS message.
      XGlobal->Swarm[SwarmIndex].generateStringHS(HSBuffer);
      retval = Connection->writeData(HSBuffer, strlen(HSBuffer), SWARM_CONNECTION_TIMEOUT);
      track_node_state = SWARM_NODE_WRITE_FAILED;
      if (retval != strlen(HSBuffer)) break;

      // Now read the reply. It can be an "AC NO" (straight reject)
      // or an HS message.
      retval = Connection->readLine(HSBuffer, sizeof(HSBuffer) - 1, SWARM_CONNECTION_TIMEOUT);
      track_node_state = SWARM_NODE_READ_FAILED;
      if (retval <= 0) break;

      COUT(cout << "Received: " << HSBuffer << endl;);

      track_node_state = SWARM_NODE_ACNO;
      if (strcasecmp(HSBuffer, "AC NO") == 0) break;

      // It has to be an HS message. So lets parse it.
      // HS FileSize SHA FileName
      track_node_state = SWARM_NODE_HS_MISFORMED;
      LineP = HSBuffer;
      if (LineP.getWordCount() < 4) break;

      parseptr = LineP.getWord(1);
      if (strcasecmp(parseptr, "HS")) break;

      parseptr = LineP.getWord(2);
      SwarmNickFileSize = strtoul(parseptr, NULL, 16);

      // Get the FileName.
      parseptr = LineP.getWordRange(4, 0);
      if (XGlobal->Swarm[SwarmIndex].isFileBeingSwarmed(parseptr) == false) {
         // FileName not matching ?
         track_node_state = SWARM_NODE_FILENAME_MISMATCH;
         break;
      }

      track_node_state = SWARM_NODE_SHA_LENGTH_MISMATCH;
      parseptr = LineP.getWord(3);
      if (strlen(parseptr) != 40) break;

      strcpy(SwarmNickFileSHA, parseptr);

      SwarmFileSize = XGlobal->Swarm[SwarmIndex].getSwarmFileSize();
      if (SwarmNickFileSize > SwarmFileSize) {
         // He has more of the file, so we send "AC YS" as we accept
         // the connection.
         strcpy(HSBuffer, "AC YS\n");
         retval = Connection->writeData(HSBuffer, strlen(HSBuffer), SWARM_CONNECTION_TIMEOUT);
         COUT(cout << "Sent: " << HSBuffer;)
         if (retval != strlen(HSBuffer)) {
            track_node_state = SWARM_NODE_WRITE_FAILED;
            break;
         }
         track_node_state = SWARM_NODE_HS_SUCCESS;
      }
      else {
         // Validate the SHA and notify him of our AC YS/NO
         if (XGlobal->Swarm[SwarmIndex].isMatchingSHA(SwarmNickFileSize, SwarmNickFileSHA)) {
            track_node_state = SWARM_NODE_HS_SUCCESS;
            strcpy(HSBuffer, "AC YS\n");
         }
         else {
            track_node_state = SWARM_NODE_SHA_MISMATCH;
            strcpy(HSBuffer, "AC NO\n");
         }

         retval = Connection->writeData(HSBuffer, strlen(HSBuffer), SWARM_CONNECTION_TIMEOUT);
         COUT(cout << "Sent: " << HSBuffer;)
         if (retval != strlen(HSBuffer)) {
            track_node_state  = SWARM_NODE_WRITE_FAILED;
            break;
         }
      }

      // Now we read his response of an AC YS/NO.
      retval = Connection->readLine(HSBuffer, sizeof(HSBuffer) - 1, SWARM_CONNECTION_TIMEOUT);
      if (retval <= 0) {
         track_node_state = SWARM_NODE_READ_FAILED;
         break;
      }

      COUT(cout << "Received: " << HSBuffer << endl;);

      if (strcasecmp(HSBuffer, "AC YS") == 0) {
         // track_node_state is already set above to SWARM_NODE_HS_SUCCESS
         // We shoudl not force it again here to SWARM_NODE_HS_SUCCESS as
         // we come here even with track_node_state = SWARM_NODE_SHA_MISMATCH
         // Hence track_node_state should not be touched here.
         // track_node_state = SWARM_NODE_HS_SUCCESS;
      }
      else {
         // Anything else assume a reject.
         track_node_state = SWARM_NODE_ACNO;
      }

   } while (false);

   retvalb = false;

   // if track_node_state == SWARM_NODE_HS_SUCCESS, add to Connected Nodes.
   if (track_node_state == SWARM_NODE_HS_SUCCESS) {
      // Do the NL (Node List Exchange of this Swarm)
      retvalb = nicklistWriteReadSwarmConnection(SwarmIndex, SwarmNick, Connection);
      if (retvalb) {
         track_node_state = SWARM_NODE_NL_SUCCESS;
      }

      // Lets check if this guy is already present in our Connected Nodes.
      // In that case we disconnect him now.
      // We do the check this late, as we can allow a handhsake and let
      // get client list, which he can further use to connect to other
      // nodes.
      if (XGlobal->Swarm[SwarmIndex].ConnectedNodes.isSwarmNodePresent(NULL, Connection->getRemoteIP())) {
         // Its already connected. So return false, so that caller will
         // disconnect this connection.
         retvalb = false;
      }
      else {
         // Add to ConnectedNodes.
         // Mark the Connection to be monitored for the caps.
         Connection->monitorForUploadCap(true);
         Connection->monitorForDwnldCap(true);
         XGlobal->Swarm[SwarmIndex].ConnectedNodes.addToSwarmNodeNickSizeStateConnection(SwarmNick, SwarmNickFileSize, track_node_state, Connection);
         retvalb = true;
     }
   }
   else {
      // Failed, Add to TriedAndFailedNodes
      XGlobal->Swarm[SwarmIndex].TriedAndFailedNodes.addToSwarmNodeNickIPState(SwarmNick, Connection->getRemoteIP(), track_node_state);
   }

   return(retvalb);
}

// Used to generated the <<mode>Nick> string.
// For ease of color coding the < ... > section in text.
void Helper::generateColorCodedNick(const char *Channel, const char *Nick, char ColorCodedNick[]) {
char char_mode[2];
char mode_color_start[10];
char *mode_color_end = "00,01";

   TRACE();

   char_mode[1] = '\0';
   char_mode[0] = XGlobal->NickList.getNickCharMode((char *) Channel, (char *) Nick);
   if (*char_mode != ' ') {
      if (*char_mode == '@') {
         // RED
         strcpy(mode_color_start, "04,01");
      }
      else if (*char_mode == '%') {
         // ORANGE
         strcpy(mode_color_start, "07,01");
      }
      else {
         strcpy(mode_color_start, "00,01");
      }
   }
   else {
      char_mode[0] = '\0';
      strcpy(mode_color_start, "00,01");
   }

   sprintf(ColorCodedNick,
            "<%s%s%s%s>",
            mode_color_start,
            char_mode,
            Nick,
            mode_color_end);
   COUT(cout << "Helper::generateColorCodedNick: " << ColorCodedNick << endl;)
}

