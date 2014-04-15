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

#include <sys/types.h>
#include <dirent.h>

#include "FileServer.hpp"
#include "Helper.hpp"
#include "Utilities.hpp"
#include "StackTrace.hpp"
#include "SHA1File.hpp"

#include "Compatibility.hpp"

// Define what version of FFLC we are at currently.
#define FFLC_VERSION 3

// This Class provides the File server talking interface.
// It provides the accessor with the Files present in MyFilesDB

FileServer::FileServer() {
   TRACE();
   RetPointer = new char[1024];
   CurrentDir = new char[1024];;
   CurrentDir[0] = '\0';
}

FileServer::~FileServer() {
   TRACE();
   delete [] RetPointer;
   delete [] CurrentDir;
}

void FileServer::run(DCC_Container_t *DCC_Container) {
int retval;
bool retvalb = true;
time_t CurrentTime;
time_t TimeOut;
LineParse LineP;
const char *parseptr;
char *tempstr;
bool normal_dir_access = false;

   TRACE();

   // First Thing is to save stuff in the DCC_Container so our private
   // functions have easy access to all the information.
   XGlobal = DCC_Container->XGlobal;
   Connection = DCC_Container->Connection;
   COUT(cout << "FileServer: Connection: " << Connection << endl;)
   COUT(Connection->printDebug();)
   RemoteNick = DCC_Container->RemoteNick;
   RemoteDottedIP = DCC_Container->RemoteDottedIP;
   RemoteNickMode = XGlobal->NickList.getNickMode(CHANNEL_MAIN, RemoteNick);

   // Lets try to get our nick.
   XGlobal->getIRC_Nick(MyNick);
   XGlobal->resetIRC_Nick_Changed();

   // Update MyMaxSends, MyMaxQueus, MyMaxSendsToEachUser, MyMaxQueuesToEachUser
   XGlobal->lock();
   MyMaxSends = XGlobal->FServSendsOverall;
   MyMaxQueues = XGlobal->FServQueuesOverall;
   MyMaxSendsToEachUser = XGlobal->FServSendsUser;
   MyMaxQueuesToEachUser = XGlobal->FServQueuesUser;
   MyMaxOverallCPS = XGlobal->OverallMaxUploadBPS;
   MyMaxSmallFileSize = XGlobal->FServSmallFileSize;
   XGlobal->unlock();

   // Now present the user with a welcome message.
   if (welcomeMessage() == false) return;

   while (retvalb) {
      CurrentTime = time(NULL);
      TimeOut = CurrentTime - Connection->Born;
      TimeOut = FSERV_TIMEOUT - TimeOut;
      if (TimeOut <= 0) break;

      retval = Connection->readLine(RetPointer, 1023, TimeOut);
      if (XGlobal->isIRC_QUIT()) {
         retval = -1;
      }

      if (retval <= 0) break;

      LineP = RetPointer;
      parseptr = LineP.getWord(1);
      if ( !strcasecmp(parseptr, "DIR") || 
           !strcasecmp(parseptr, "LS") ) {
         normal_dir_access = true;
         retvalb = fservDir();
      }
      else if ( !strcasecmp(parseptr, "CD") ) {
         parseptr = LineP.getWordRange(2, 0);
         tempstr = new char[strlen(parseptr) + 1];
         strcpy(tempstr, parseptr);
         retvalb = fservCD(tempstr);
         delete [] tempstr;
      }
      else if ( !strcasecmp(parseptr, "PWD") ) {
         retvalb = fservPWD();
      }
      else if ( !strcasecmp(parseptr, "GET") ) {
         parseptr = LineP.getWordRange(2, 0);
         tempstr = new char[strlen(parseptr) + 1];
         strcpy(tempstr, parseptr);
         retvalb = fservGet(tempstr, false, 0);
         delete [] tempstr;
      }
      else if ( !strcasecmp(parseptr, "GETPARTIAL") ) {
         parseptr = LineP.getWordRange(2, 0);
         tempstr = new char[strlen(parseptr) + 1];
         strcpy(tempstr, parseptr);
         retvalb = fservGet(tempstr, true, 0);
         delete [] tempstr;
      }
      else if ( !strcasecmp(parseptr, "GETFROM") ) {
         parseptr = LineP.getWord(2);
         size_t resume_pos = strtoul(parseptr, NULL, 10);
         parseptr = LineP.getWordRange(3, 0);
         tempstr = new char[strlen(parseptr) + 1];
         strcpy(tempstr, parseptr);
         retvalb = fservGet(tempstr, false, resume_pos);
         delete [] tempstr;
      }
      else if ( !strcasecmp(parseptr, "GETFROMPARTIAL") ) {
         parseptr = LineP.getWord(2);
         size_t resume_pos = strtoul(parseptr, NULL, 10);
         parseptr = LineP.getWordRange(3, 0);
         tempstr = new char[strlen(parseptr) + 1];
         strcpy(tempstr, parseptr);
         retvalb = fservGet(tempstr, true, resume_pos);
         delete [] tempstr;
      }
      else if ( !strcasecmp(parseptr, "SENDS") ) {
         retvalb = fservSends();
      }
      else if ( !strcasecmp(parseptr, "QUEUES") ) {
         retvalb = fservQueues();
      }
      else if ( !strcasecmp(parseptr, "CLR_QUEUES") ) {
         retvalb = fservClearQueues(0);
      }
      else if ( !strcasecmp(parseptr, "CLR_QUEUE") ) {
         parseptr = LineP.getWord(2);
         long q_num = strtoul(parseptr, NULL, 10);
         if (q_num <= 0) q_num = 1;
         retvalb = fservClearQueues(q_num);
      }
      else if ( !strcasecmp(parseptr, "NICKLIST") ) {
         parseptr = LineP.getWord(2);
         FFLCversion = (int) strtol(parseptr, NULL, 10);
         retvalb = fservNicklist();
      }
      else if ( !strncasecmp(parseptr, "FILELIST", 8) ) {
         // Handles both the filelists and filelistp command.
         char SorP = DOWNLOADSTATE_SERVING;
         if ( (parseptr[8] == 'P') ||
              (parseptr[8] == 'p') ) {
            SorP = DOWNLOADSTATE_PARTIAL;
         }
         if (LineP.getWordCount() == 2) {
            parseptr = LineP.getWord(2); // Nicks dont have space
            tempstr = new char[strlen(parseptr) + 1];
            strcpy(tempstr, parseptr);
            retvalb = fservFilelist(tempstr, SorP);
            delete [] tempstr;
         }
         else retvalb = true;
      }
      else if ( !strcasecmp(parseptr, "ENDLIST") ) {
         retvalb = fservEndlist();
      }
      else if ( !strcasecmp(parseptr, "METAINFO") ) {
         parseptr = LineP.getWordRange(2, 0);
         tempstr = new char[strlen(parseptr) + 1];
         strcpy(tempstr, parseptr);
         retvalb = fservMetaInfo(tempstr);
         delete [] tempstr;
      }
      else if ( !strcasecmp(parseptr, "QUIT") ||
                !strcasecmp(parseptr, "EXIT") ) {
         fservExit();
         retvalb = false; // Forced false to QUIT.
      }
      else if ( !strcasecmp(parseptr, "HELP") ) {
         retvalb = welcomeMessage();
      }
      else {
         retvalb = fservUnknown();
      }
   }

   // Print out the statistics if it was a non FFLC dir access.
   if (normal_dir_access) {
      // Lets now print out the Statistics.
      XGlobal->lock();
      XGlobal->DirAccess_BytesIn += (double) Connection->BytesReceived;
      XGlobal->DirAccess_BytesOut += (double) Connection->BytesSent;
      sprintf(RetPointer, "Server 09FServ Access: By %s. Bytes In: %lu Out: %lu. Total Bytes In: %g Out: %g", RemoteNick, Connection->BytesReceived, Connection->BytesSent, XGlobal->DirAccess_BytesIn, XGlobal->DirAccess_BytesOut);
      XGlobal->unlock();
      XGlobal->IRC_ToUI.putLine(RetPointer);
   }
}

// Returns true on sending the Welcome message successfully.
bool FileServer::welcomeMessage() {
char prioritystr[64];
char maxcpsstr[64];
char instasendstr[64];
char fs_str[32];
bool retvalb;

   TRACE();
   updateSendsAndQueues();
   if (IS_MORE_THAN_REGULAR(RemoteNickMode)) {
      sprintf(prioritystr, "You have priority queuing access.\n");
   }
   else {
      prioritystr[0] = '\0';
   }

   if (MyMaxOverallCPS == 0) {
      maxcpsstr[0] = '\0';
   }
   else {
      sprintf(maxcpsstr, "Maximum CPS is: %lu\n", MyMaxOverallCPS);
   }

   convertFileSizeToString(MyMaxSmallFileSize, fs_str);
   sprintf(instasendstr, "Instant Send is at: %s\n", fs_str);

   // Dont list the "filelist", "nicklist <nick>", "metainfo" commands.
   // as they are for MM alone.
   sprintf(RetPointer,
           "Welcome to %s's File Server\n"
           "Commands: cd, clr_queue, clr_queues, dir, exit, get, help, ls, queues, quit, sends\n"
           "Transfer Status: Sends:[%d/%d] - Queues:[%d/%d]\n"
           "%s"
           "This server will auto-close after %d seconds.\n"
           "%s" 
           "%s"
           "%s - %s - %s\n"
           "Use: commands listed above - FFLC %d\n"
           "[\\]\n",
           MyNick,
           TotalSends, MyMaxSends,
           TotalQueues, MyMaxQueues * 2,
           prioritystr,
           FSERV_TIMEOUT,
           instasendstr,
           maxcpsstr,
           CLIENT_NAME_FULL, VERSION_STRING, DATE_STRING,
           FFLC_VERSION);
   retvalb = sendMessage();
   return(retvalb);
}

void FileServer::updateSendsAndQueues() {

   TRACE();

   TotalSends = XGlobal->SendsInProgress.getCount(NULL) + 
           XGlobal->DCCSendWaiting.getCount(NULL);
   BigQueues = XGlobal->QueuesInProgress.getCount(NULL);
   SmallQueues = XGlobal->SmallQueuesInProgress.getCount(NULL);
   TotalQueues = BigQueues + SmallQueues;
}

// Responds to a DIR or LS command.
// CurrentDir holds the Current Directory that we are in.
// It has FS_DIR_SEP as the Dir seperator.
bool FileServer::fservDir() {
bool retvalb = true;
FilesDetail *FD, *ScanFD;
char NullDirStr[1];
char *ScanDirStr;
char LastPrintedDir[MAX_PATH];

   TRACE();

   NullDirStr[0] = '\0';
   ScanDirStr = NullDirStr;
   LastPrintedDir[0] = '\0';

   // Lets get the list of Files we are serving.
   FD = XGlobal->MyFilesDB.searchDirFilesDetailList(CurrentDir);
   ScanFD = FD;
   sprintf(RetPointer, "%s - %s - %s\n[%s\\*.*]\n",
                    CLIENT_NAME_FULL, VERSION_STRING, DATE_STRING, CurrentDir);
   retvalb = sendMessage();
   
   while (ScanFD && retvalb) {
   char SizeStr[16];
   float fsize;

      if (XGlobal->isIRC_QUIT()) break;
         

      if (ScanFD->DirName == NULL) {
         ScanDirStr = NullDirStr;
      }
      else {
         ScanDirStr = ScanFD->DirName;
      }

      // If the DirName matches the CurrentDir exactly, print its file info.
      if (strcasecmp(CurrentDir, ScanDirStr) == 0) {
         fsize = ScanFD->FileSize;
         if (ScanFD->FileSize < 1024 * 1024) {
            sprintf(SizeStr, "%.2f kb", fsize/1024.0);
         }
         else {
            sprintf(SizeStr, "%.2f mb", fsize/(1024.0*1024.0));
         }
      
         sprintf(RetPointer, "%s %s\n", ScanFD->FileName, SizeStr);
         retvalb = sendMessage();
      }
      else {
         int str_index;
         // This is where we extract the DIR part of DirName.
         // So if CurrentDir is NULL, and DirName is "\CVS", implies
         //  we print "CVS" in FServ list.
         // We print that only once. So we remember the last printed DIR
         // name and not repeat if printed in previous loop.
         // Also, if DirName is "\CVS\someDIR", the DIR to be printed is
         // "CVS".
         // Lets get the correct directory name in ScanDirStr.
         // It starts at strlen(CurrentDir) and goes till we hit FS_DIR_SEP_CHAR
         // or end of string.
COUT(cout << "CurrentDir: " << CurrentDir << " ScanDirStr: " << ScanDirStr << endl;)

         str_index = strlen(CurrentDir);

         // Starting from where CurrentDir ends, we scan ScanDirStr for next
         // Directory name. Note we come here cause CurrentDir is not
         // ScanDirStr. => there definitely is a \ which we skip.
         for (str_index = strlen(CurrentDir) + 1; ScanDirStr[str_index] != '\0'; str_index++) {
            if (ScanDirStr[str_index] == FS_DIR_SEP_CHAR) break;
         }
         // Below is length of the possible directory name.
         // starting from index = strlen(CurrentDir) + 1
         // str_index = str_index - strlen(CurrentDir) - 1;
         str_index = str_index - strlen(CurrentDir);
         COUT(cout << "LastPrintedDir: " << LastPrintedDir << " ScanDirStr[strlen(CurrentDir)]: " << &ScanDirStr[strlen(CurrentDir)] << " str_index: " << str_index << endl;)
         if ( (LastPrintedDir[0] == '\0') || 
            strncasecmp(LastPrintedDir, &ScanDirStr[strlen(CurrentDir)], str_index)) {
COUT(cout << "LastPrintedDir: " << LastPrintedDir << " ScanDirStr: " << ScanDirStr << " str_index: " << str_index << endl;)
            strncpy(LastPrintedDir, &ScanDirStr[strlen(CurrentDir)], str_index + 1);
            LastPrintedDir[str_index + 1] = '\0';
            if (LastPrintedDir[str_index] == FS_DIR_SEP_CHAR) {
               LastPrintedDir[str_index] = '\0';
            }
            // Print with no DIR_SEP_CHAR at start.
            sprintf(RetPointer, "%s\n", &LastPrintedDir[1]);
            retvalb = sendMessage();
         }
      }

      ScanFD = ScanFD->Next;
   }

   sprintf(RetPointer, "End of List\n");
   retvalb = sendMessage();

   XGlobal->MyFilesDB.freeFilesDetailList(FD);
   return(retvalb);
}

// Responds to a GET command.
// If fromPartialDir == true, => get from the Partial Dir and not from
// the Serving Dir.
// resume_pos = requested resume from that position, used to decide on which
// Q it will make it in -> small or big.
bool FileServer::fservGet(const char *filename, bool fromPartialDir, size_t resume_pos) {
bool retvalb = true;
int retval;
int qnum;;
FilesDetail *FD;
Helper H;
int SendsOfUser, QueuesOfUser;
bool BoolSmallQueue = false;

   TRACE();

   H.init(XGlobal);

// Lets first check if we are serving that file.
   if (fromPartialDir) {
      // This command is allowed only if the RemoteNick is an MM client.
      // Add in the check here later.
      retvalb = XGlobal->MyPartialFilesDB.isPresentMatchingFileName(filename);
      if (retvalb == false) {
         // Check in Serving Dir, if not present in Partial.
         retvalb = XGlobal->MyFilesDB.isPresentMatchingFileName(filename);
      }
   }
   else {
      retvalb = XGlobal->MyFilesDB.isPresentMatchingFileName(filename);
   }

   if (retvalb == false) {
      sprintf(RetPointer, "Invalid file name, please use the form: "
                          "< get filename.ext >\n"
                          "This server does NOT handle wildcards ( *.* )\n");
      retvalb = sendMessage();
      return(retvalb);
   }

// So we are serving the file.

// Check if this Nick/FileName is already queued.
   retval = XGlobal->QueuesInProgress.isPresentMatchingNickFileName(RemoteNick, filename) + XGlobal->SmallQueuesInProgress.isPresentMatchingNickFileName(RemoteNick, filename);
   if (retval != 0) {
//    He already has file queued at position retval.
      sprintf(RetPointer, "That file has already been queued in slot %d!\n",
              retval);
      retvalb = sendMessage();
      return(retvalb);
   }

// Check if this Nick/FileName is already being sent.
   if ( XGlobal->SendsInProgress.isPresentMatchingNickFileName(RemoteNick, filename) ||
        XGlobal->DCCSendWaiting.isPresentMatchingNickFileName(RemoteNick, filename) ) {
//    This file is already being sent to this nick.
      sprintf(RetPointer, "That file is already sending!\n");
      retvalb = sendMessage();
      return(retvalb);
   }

   // update Sends/Queues as we will be using those values.
   updateSendsAndQueues();

// Check if this Nick is already holding a Send or a Queue.
   SendsOfUser = XGlobal->SendsInProgress.getFileCountOfNick(NULL, RemoteNick) +
            XGlobal->DCCSendWaiting.getFileCountOfNick(NULL, RemoteNick);
   QueuesOfUser = XGlobal->QueuesInProgress.getFileCountOfNick(NULL, RemoteNick) + XGlobal->SmallQueuesInProgress.getFileCountOfNick(NULL, RemoteNick);

   // If SendsOfUser + QueuesOfUser >= MyMaxQueuesToEachUser
   // then not sending or queueing.
   // Sends count towards Queues, that is why !!
   if ( (SendsOfUser + QueuesOfUser) >= MyMaxQueuesToEachUser) {
      sprintf(RetPointer, "All your Sends and Queues are full!!\n");
      retvalb = sendMessage();
      return(retvalb);
   }

   // Check if we this file is being Swarmed.
   for (int s_index = 0; s_index < SWARM_MAX_FILES; s_index++) {
     if (XGlobal->Swarm[s_index].isFileBeingSwarmed(filename)) {
        sprintf(RetPointer,
                  "This File is being swarmed. To join the swarm,"
                  " type: /swarm %s %s\n",
                  MyNick,
                  XGlobal->Swarm[s_index].getSwarmFileName());
        retvalb = sendMessage();
        return(retvalb);
     }
   }

   // We will always put him in Queue. Even if immediate send is possible,

   // If current holding Queues is >= MyMaxQueues cant q him
   if (TotalQueues >= (MyMaxQueues * 2)) {
      sprintf(RetPointer, "All Send Slots and Queue Slots are full. "
              "Please try again later, Paji\n");
      retvalb = sendMessage();
      return(retvalb);
   }

// Now we are ready to populate FD and add to QueuesInProgress or
// SmallQueuesInProgress
   if (fromPartialDir) {
      FD = XGlobal->MyPartialFilesDB.getFilesDetailListMatchingFileName((char *) filename);
      if (FD == NULL) {
         // Check in Serving Dir, if not present in Partial.
         FD = XGlobal->MyFilesDB.getFilesDetailListMatchingFileName((char *) filename);
      }
   }
   else {
      FD = XGlobal->MyFilesDB.getFilesDetailListMatchingFileName((char *) filename);
   }

   // Again make sure that FD is valid.
   if (FD == NULL) {
      sprintf(RetPointer, "Invalid file name, please use the form: "
                          "< get filename.ext >\n"
                          "This server does NOT handle wildcards ( *.* )\n");
      retvalb = sendMessage();
      return(retvalb);
   }

   // Decide which Queue.
   // and Check again against the Correct BIG/SMALL Q.
   if ( (FD->FileSize <= MyMaxSmallFileSize) ||
        (FD->FileSize <= (resume_pos + MyMaxSmallFileSize)) ) {
      BoolSmallQueue = true;
      retval = SmallQueues;
   }
   else {
      BoolSmallQueue = false;
      retval = BigQueues;
   }

   if (retval >= MyMaxQueues) {
      // This one has exceeded its Q capacity. Cant Q.
      // Free up the FD.
      XGlobal->MyFilesDB.freeFilesDetailList(FD);
      FD = NULL;

      if (BoolSmallQueue) {
         sprintf(RetPointer, 
           "All Send Slots and Queue Slots for Small Files (<= %lu) are full. "
           "Please try again later, Paji\n", MyMaxSmallFileSize);
      }
      else {
         sprintf(RetPointer, 
           "All Send Slots and Queue Slots for Big Files (> %lu) are full. "
           "Please try again later, Paji\n", MyMaxSmallFileSize);
      }
      retvalb = sendMessage();
      return(retvalb);
   }

   delete [] FD->Nick;
   // This FD should have only one item and not a list.
   FD->Nick = new char[strlen(RemoteNick) + 1];
   strcpy(FD->Nick, RemoteNick);
   delete [] FD->DottedIP;
   FD->DottedIP = new char[strlen(RemoteDottedIP) + 1];
   strcpy(FD->DottedIP, RemoteDottedIP);
   FD->FileResumePosition = resume_pos; // will be verified when send starts.

   // We now queue this guy in the correct place, taking his mode in
   // RemoteNickMode.
   if (strncasecmp("MasalaMate.", FD->FileName, 11) == 0) {
      // if File is MasalaMate.* send immediately - like manual file push
      FD->ManualSend = MANUALSEND_FILEPUSH;
      // Add to correct Queue depending on the filesize.
      if (BoolSmallQueue) {
         XGlobal->SmallQueuesInProgress.addFilesDetailAtIndex(FD, 1);
      }
      else {
         XGlobal->QueuesInProgress.addFilesDetailAtIndex(FD, 1);
      }
      qnum = 1;
   }
   else if (IS_MORE_THAN_REGULAR(RemoteNickMode)) {
      FilesDetail *TempFD, *ScanFD;
      int count, index;
      unsigned int nickmode;
      // Lets go through the whole list and insert him in the correct place.
      // First find which Q this guy will belong in.
      if (BoolSmallQueue) {
         TempFD = XGlobal->SmallQueuesInProgress.searchFilesDetailList("*", &count);
      }
      else {
         TempFD = XGlobal->QueuesInProgress.searchFilesDetailList("*", &count);
      }
      ScanFD = TempFD;
      index = 1;
      while (ScanFD) {
         if (XGlobal->NickList.isNickInChannel(CHANNEL_MAIN, ScanFD->Nick) == false) {
            // We skip nicks which are not in channel, as we wont know
            // their nick mode. Put ourselves after them.
            ScanFD = ScanFD->Next;
            index++;
            continue;
         }

         if (ScanFD->RetryCount > 0) {
            // We skip nicks which have been put in Q cause of a send failure.
            // These could be regular nicks, and ahead of a priority nick, as
            // they are coming in from the send q to the Q q.
            ScanFD = ScanFD->Next;
            index++;
            continue;
         }

         nickmode = XGlobal->NickList.getNickMode(CHANNEL_MAIN, ScanFD->Nick);
         // higher the nick mode more the power.
         // Hence we skip till we get someone who has less power than him.
         if (nickmode >= RemoteNickMode ) {
            // This nick already in Q is also a nick with greater or same
            // priority. Hence we keep skipping till we get there.
            ScanFD = ScanFD->Next;
            index++;
            continue;
         }

         // We are here ! got the slot where we can insert.
         break;
      }
      // We insert at this index.
      if (BoolSmallQueue) {
         XGlobal->SmallQueuesInProgress.addFilesDetailAtIndex(FD, index);
      }
      else {
         XGlobal->QueuesInProgress.addFilesDetailAtIndex(FD, index);
      }
      qnum = index;

      XGlobal->QueuesInProgress.freeFilesDetailList(TempFD);
   }
   else {
      // Append to Q.
      if (BoolSmallQueue) {
         XGlobal->SmallQueuesInProgress.addFilesDetailAtIndex(FD, 0);
         qnum = SmallQueues + 1;
      }
      else {
         XGlobal->QueuesInProgress.addFilesDetailAtIndex(FD, 0);
         qnum = BigQueues + 1;
      }
   }

   // Give the message in the case when he is queued. which is always.
   sprintf(RetPointer, "Adding your file to queue slot %d. "
           "The file will send when the next send slot is open.\n",
           qnum);
   sendMessage();

   COUT(cout << "FileServer: QueuesInProgress: ";)
   COUT(XGlobal->QueuesInProgress.printDebug(NULL);)

   // Lets write the FServe Sends/Queues in config file so its recorded.
   H.writeFServeConfig();

// The TimerThr will wake up every second, and check if it needs
// to initiate a send from QueuesInProgress
   return(true);
}

// clear Queue at index <q_index> that the caller holds.
// If q_index == 0 => clear all his queues.
bool FileServer::fservClearQueues(int q_index) {
bool retvalb = true;
FilesDetail *FD, *ScanFD;
int cur_index;
bool did_clear = false;

   TRACE();

   // Get the whole Queues List.
   FD = XGlobal->QueuesInProgress.searchFilesDetailList("*");

   ScanFD = FD;
   cur_index = 0;
   while (ScanFD) {
      cur_index++;
      if ( (cur_index == q_index) || (q_index == 0) ) {
         // We delete this entry if Nick matches that of RemoteNick.
         if (strcasecmp(ScanFD->Nick, RemoteNick) == 0) {
            XGlobal->QueuesInProgress.delFilesDetailNickFile(ScanFD->Nick, ScanFD->FileName);
            did_clear = true;
            // Print it out.
            sprintf(RetPointer, "Removing %s from slot %d.\n",  ScanFD->FileName, cur_index);
            retvalb = sendMessage();
            if (retvalb == false) break;
         }
         if (q_index != 0) break;
      }
      ScanFD = ScanFD->Next;
   }
   // Free up FD
   XGlobal->QueuesInProgress.freeFilesDetailList(FD);

   // We are done with the Big Queue. Now for the Small Queue.

   // Get the whole Small Queues List.
   FD = XGlobal->SmallQueuesInProgress.searchFilesDetailList("*");

   ScanFD = FD;
   cur_index = 0;
   while (ScanFD) {
      cur_index++;
      if ( (cur_index == q_index) || (q_index == 0) ) {
         // We delete this entry if Nick matches that of RemoteNick.
         if (strcasecmp(ScanFD->Nick, RemoteNick) == 0) {
            XGlobal->SmallQueuesInProgress.delFilesDetailNickFile(ScanFD->Nick, ScanFD->FileName);
            did_clear = true;
            // Print it out.
            sprintf(RetPointer, "Removing %s from slot %d.\n",  ScanFD->FileName, cur_index);
            retvalb = sendMessage();
            if (retvalb == false) break;
         }
         if (q_index != 0) break;
      }
      ScanFD = ScanFD->Next;
   }

   // Free up FD
   XGlobal->SmallQueuesInProgress.freeFilesDetailList(FD);

   if (did_clear == false) {
      sprintf(RetPointer, "I can't remove queues that don't exist!\n");
      retvalb = sendMessage();
   }

   return(retvalb);
}

// To get the metainfo file
// Returns true only on Error.
bool FileServer::fservMetaInfo(char *fname) {
bool retvalb = true;
int retval;
int bytecount;
SHA1File S;
char *ServDir;

   TRACE();

   // ideally we should search in MyFilesDB for this fname.
   for (int i = 0; i < FSERV_MAX_SERVING_DIRECTORIES; i++) {
      XGlobal->lock();
      ServDir = new char[strlen(XGlobal->ServingDir[i]) + 1];
      strcpy(ServDir, XGlobal->ServingDir[i]);
      XGlobal->unlock();
      retvalb = S.initFile(ServDir, fname);
      delete [] ServDir;
      if (retvalb) break;
   }

   if (retvalb == true) {
      // We have the metafile info in S. Just send it over the wire.

      do {
         S.generatePreambleLines(RetPointer, fname);
         bytecount = strlen(RetPointer);
         retval = Connection->writeData(RetPointer, bytecount);
         if (retval != bytecount) {
            retvalb = false;
            break;
         }
         const char *shas = S.getSHA1MetaInfoRaw();
         if (shas == NULL) {
            retvalb = false;
            break;
         }
         // Lets send the bytes out 1K at a time.
         #define METAINFO_TRANSFER_SIZE 1024
         size_t full_size = S.getSHA1MetaInfoRawSize();
         size_t cur_index;
         size_t send_piece;
         do {
            if (full_size < METAINFO_TRANSFER_SIZE) {
               // Last piece.
               send_piece = full_size;
            }
            else {
               send_piece = METAINFO_TRANSFER_SIZE;
            }
            retval = Connection->writeData(&shas[cur_index], send_piece);
            if (retval != send_piece) {
               full_size = 0;
               retvalb = false;
               break;
            }
            cur_index += retval;
            full_size -= retval;
         } while (full_size > 0);
         
      } while (false);
   }

   if (retvalb == false) {
      sprintf(RetPointer, "Error:\n");
      bytecount = strlen(RetPointer);
      retval = Connection->writeData(RetPointer, bytecount);
      retvalb = true;
   }
   else {
      retvalb = false; // Exit out.
   }
   return(retvalb);
}

// Responds to a EXIT/QUIT command.
bool FileServer::fservExit() {
bool retvalb = true;

   TRACE();

   sprintf(RetPointer, "Bye Bye Miss GoodNight, Kal Phir Milenge ...\n");
   retvalb = sendMessage();

   return(retvalb);
}

// Responds to a UNKNOWN command.
bool FileServer::fservUnknown() {
bool retvalb = true;

   TRACE();

   sprintf(RetPointer, "Unrecognised Command.\n");
   retvalb = sendMessage();

   return(retvalb);
}


// Responds to a SENDS command.
// Total Number Of Sends Currently: 1/2
// Send 1: FileName 4MB is 0% done at 0 cps. Eta: 0 secs. Sending to: Sur4802.
//   Resend: 1.
bool FileServer::fservSends() {
FilesDetail *ScanFD, *FD;
float f_file_delta_remain;
float f_file_size;
float f_file_done;
float f_progress_percent;
float f_speed;
float f_time_left;
time_t t_time_left;
char  s_time_left[64];
bool retvalb = true;
int i;
char sep127 = 127;

   TRACE();

   updateSendsAndQueues();

   sprintf(RetPointer, "Total Number Of Sends Currently: %d/%d\n", TotalSends, MyMaxSends);
   retvalb = sendMessage();

   FD = XGlobal->SendsInProgress.searchFilesDetailList("*");
   ScanFD = FD;
   i = 1;
   while (ScanFD && retvalb) {
      // This loop code is copied from TabBookWindow::updateFileServer()

      if (XGlobal->isIRC_QUIT())  break;

      f_speed = ScanFD->UploadBps;
      f_file_done = ScanFD->FileResumePosition + ScanFD->BytesSent;
      f_file_size = ScanFD->FileSize;
      if (f_file_size < f_file_done) f_file_size = f_file_done;
      f_file_delta_remain = f_file_size - f_file_done;
      f_progress_percent = (f_file_done / f_file_size) * 100.0;
      f_time_left = f_file_delta_remain / f_speed;
      t_time_left = (time_t) f_time_left;
      if (f_speed < 100.0) {
         strcpy(s_time_left, "UNKNOWN");
      }
      else {
         convertTimeToString(t_time_left, s_time_left);
      }

      sprintf(RetPointer,
              "Send %d: %s%c%liMB is %05.2f%c done at %li cps. Eta: %s. Sending to: %s. Resend: %d.\n",
              i, ScanFD->FileName, sep127, (long) (f_file_size/1024/1024),
              f_progress_percent, '%', (long) f_speed, s_time_left,
              ScanFD->Nick, ScanFD->RetryCount);
      retvalb = sendMessage();

      i++;

      ScanFD = ScanFD->Next;
   }

   XGlobal->SendsInProgress.freeFilesDetailList(FD);

   FD = XGlobal->DCCSendWaiting.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD && retvalb) {
      if (XGlobal->isIRC_QUIT()) break;

      f_speed = ScanFD->UploadBps;
      f_file_done = ScanFD->FileResumePosition + ScanFD->BytesSent;
      f_file_size = ScanFD->FileSize;
      if (f_file_size < f_file_done) f_file_size = f_file_done;
      f_file_delta_remain = f_file_size - f_file_done;
      f_progress_percent = (f_file_done / f_file_size) * 100.0;
      f_time_left = f_file_delta_remain / f_speed;
      t_time_left = (time_t) f_time_left;
      if (f_speed < 100.0) {
         strcpy(s_time_left, "UNKNOWN");
      }
      else {
         convertTimeToString(t_time_left, s_time_left);
      }

      sprintf(RetPointer, "Send %d: %s%c%liMB is %05.2f%c done at %li cps. Eta: %s. Sending to: %s. Resend: %d.\n",
              i, ScanFD->FileName, sep127, (long) (ScanFD->FileSize/1024/1024),
              f_progress_percent, '%', (long) f_speed, s_time_left,
              ScanFD->Nick, ScanFD->RetryCount);

      retvalb = sendMessage();

      i++;

      ScanFD = ScanFD->Next;
   }

   XGlobal->DCCSendWaiting.freeFilesDetailList(FD);

   if (i == 1) {
      // No Send lines printed out.
      sprintf(RetPointer, "All send slots open\n");
      retvalb = sendMessage();
   }

   return(retvalb);
}

// Responds to a QUEUES command.
bool FileServer::fservQueues() {
bool retvalb = true;
FilesDetail *FD, *ScanFD;
int i, QueueDisplayed;
char s_filesize[32];
char sep127 = 127;

   TRACE();

   updateSendsAndQueues();

   sprintf(RetPointer, "Total Number Of Queues Currently: %d/%d\n", TotalQueues, MyMaxQueues * 2);
   retvalb = sendMessage();

   FD = XGlobal->QueuesInProgress.searchFilesDetailList("*");
   ScanFD = FD;
   i = 1;
   QueueDisplayed = 0;
   while (ScanFD && retvalb) {

      convertFileSizeToString(ScanFD->FileSize, s_filesize);
      sprintf(RetPointer, "Queue %dBig. %s%c%s was queued by %s. Resend: %d.\n", i, ScanFD->FileName, sep127, s_filesize, ScanFD->Nick, ScanFD->RetryCount);
      retvalb = sendMessage();
      QueueDisplayed++;
      i++;

      ScanFD = ScanFD->Next;
   }
   XGlobal->QueuesInProgress.freeFilesDetailList(FD);

   FD = XGlobal->SmallQueuesInProgress.searchFilesDetailList("*");
   ScanFD = FD;
   i = 1;
   while (ScanFD && retvalb) {

      convertFileSizeToString(ScanFD->FileSize, s_filesize);
      sprintf(RetPointer, "Queue %dSml. %s%c%s was queued by %s. Resend: %d.\n", i, ScanFD->FileName, sep127, s_filesize, ScanFD->Nick, ScanFD->RetryCount);
      retvalb = sendMessage();
      QueueDisplayed++;
      i++;

      ScanFD = ScanFD->Next;
   }
   XGlobal->SmallQueuesInProgress.freeFilesDetailList(FD);

   if (QueueDisplayed == 0) {
      sprintf(RetPointer, "All queue slots open\n");
      retvalb = sendMessage();
      return(retvalb);
   }

   // If we have not displayed TotalQueues amount of Entries, we are not
   // displaying as many entries as promised. Hence some dummy entries.
   while (QueueDisplayed < TotalQueues) {
      sprintf(RetPointer, "Queue %dSml. Dummy%c1MB was queued by Dummy. Resend: 0.\n", i, sep127);
      retvalb = sendMessage();
      i++;
      QueueDisplayed++;
   }

   return(retvalb);
}

// Responds to a CD command.
// Update our current directory, CurrentDir.
bool FileServer::fservCD(char *dirname) {
bool retvalb = true;
LineParse LineP;
const char *parseptr;
int char_index;
DIR *d;

   TRACE();

   if ( (strlen(dirname) + strlen(CurrentDir)) >= MAX_PATH) {
      strcpy(RetPointer, "Directory Name too Long. Try correctly again!\n");
      retvalb = sendMessage();
      return(retvalb);
   }

   // Lets handle the "CD .." command.
   if (strcmp(dirname, "..") == 0) {
      if (CurrentDir[0] == '\0') {
        // Cant CD .. beyond this.
        strcpy(RetPointer, "Already at BASE of serving directory.\n");
        retvalb = sendMessage();
        return(retvalb);
      }

      // Do the requested CD ..
      for (char_index = strlen(CurrentDir) - 1; char_index > 0; char_index--) {
         if (CurrentDir[char_index] == FS_DIR_SEP_CHAR) {
           break;
	 }
      }
      CurrentDir[char_index] = '\0';
      if (CurrentDir[0] == '\0') {
         strcpy(RetPointer, "[\\]\n");
      }
      else {
         sprintf(RetPointer, "[%s]\n", CurrentDir);
      }
      retvalb = sendMessage();
      return(retvalb);
   }

   // Lets reject useless dirname's. Which contain UNIX/WIN DIR_SEP_CHAR in it.
   if (strchr(dirname, WIN_DIR_SEP_CHAR) || strchr(dirname, UNIX_DIR_SEP_CHAR) ) {
      sprintf(RetPointer, "Directory Name cannot have '%c' or '%c'. Try again correctly!\n", WIN_DIR_SEP_CHAR, UNIX_DIR_SEP_CHAR);
      retvalb = sendMessage();
      return(retvalb);
   }

   // Lets validate the dirname.
   // We need to loop through XGlobal->ServingDir[]
   d = NULL;
   for (int DirIndex = 0; DirIndex < FSERV_MAX_SERVING_DIRECTORIES; DirIndex++) {
      XGlobal->lock();
      sprintf(RetPointer, "%s%c%s%c%s", XGlobal->ServingDir[DirIndex], FS_DIR_SEP_CHAR, CurrentDir, DIR_SEP_CHAR, dirname);
      XGlobal->unlock();

      // Convert RetPointer PATH with correct DIR_SEP_CHAR
      for (int i = 0; ;i++) {
         if (RetPointer[i] == '\0') break;
         if (RetPointer[i] == FS_DIR_SEP_CHAR) {
            RetPointer[i] = DIR_SEP_CHAR;
         }
      }

      d = opendir(RetPointer);
      if (d) break;
   }
   if (d == NULL) {
      // CD into invalid directory.
      strcpy(RetPointer, "CD into invalid directory. Please try again correctly!\n");
      retvalb = sendMessage();
      return(retvalb);
   }
   else {
      closedir(d);
   }

   // Its a valid directory.
   strcat(CurrentDir, FS_DIR_SEP);
   strcat(CurrentDir, dirname);

   sprintf(RetPointer, "%s - %s - %s\n[%s]\n",
                    CLIENT_NAME_FULL, VERSION_STRING, DATE_STRING, CurrentDir);
   retvalb = sendMessage();
   return(retvalb);
}

// Responds to a PWD command.
bool FileServer::fservPWD() {
bool retvalb = true;

   TRACE();

   sprintf(RetPointer, "%s - %s - %s\n[%s]\n",
                    CLIENT_NAME_FULL, VERSION_STRING, DATE_STRING, CurrentDir);
   retvalb = sendMessage();
   return(retvalb);
}

// Takes care of the "FILELIST<S or P> <nick>" command
// dstate = S or P.
bool FileServer::fservFilelist(char *varnick, char dstate) {
Helper H;
bool retvalb;

   TRACE();
//   XGlobal->lockFFLC();
//   COUT(cout << "FFLC lock: fservFilelist for nick " << varnick << " dstate: " << dstate << endl;)

   H.init(XGlobal, Connection);

   retvalb = H.helperFServFilelist(varnick, dstate);

//   COUT(cout << "FFLC unlock: fservFilelist for nick " << varnick << " dstate " << dstate << endl;)
//   XGlobal->unlockFFLC();

   return(retvalb);
}


// Takes care of the "NICKLIST" command.
bool FileServer::fservNicklist() {
bool retvalb;
Helper H;

   TRACE();

//   XGlobal->lockFFLC();
//   COUT(cout << "FFLC lock: fservNicklist" << endl;)

   H.init(XGlobal, Connection);
   retvalb = H.helperFServNicklist(FFLCversion);

//   COUT(cout << "FFLC unlock: fservNicklist" << endl;)
//   XGlobal->unlockFFLC();

   return(retvalb);
}

// Handles the "ENDLIST" command.
bool FileServer::fservEndlist() {
bool retvalb;
Helper H;

   TRACE();

   H.init(XGlobal, Connection);
   // Server calling. Now server needs to issue the nicklist, but same version
   // as that of client.
   retvalb = H.helperFServStartEndlist(RemoteNick, true, FFLCversion);

   return(retvalb);
}

// Sends Message in RetPointer. returns succesfull send or failure.
// Make sure RetPointer contains the string to be sent.
bool FileServer::sendMessage() {
int retval, bytecount;
bool retvalb = true;

   TRACE();

   bytecount = strlen(RetPointer);
   retval = Connection->writeData(RetPointer, bytecount);
   if (retval != bytecount) retvalb = false;
   return(retvalb);
}

bool FileServer::recvMessage() {
bool retvalb;
int retval;

   TRACE();

   retval = Connection->readLine(RetPointer, 1023, DCCCHAT_TIMEOUT);
   if (retval <= 0) retvalb = false;
   else retvalb = true;

   // COUT(cout << "FileServer::recvMessage: " << RetPointer << endl);
   return(retvalb);
}

