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

#include "DCCChatClient.hpp"
#include "FilesDetailList.hpp"
#include "LineQueue.hpp"
#include "FServParse.hpp"
#include "Helper.hpp"
#include "Utilities.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

#include <stdio.h>
#include <string.h>

// This Class provides the DCC Chat Client talking interface.
// It provides two interface. One to get the recursive dir listing of server
// The other to get a file from the server.
// All connections are established. We just need to do the work.

#if 0
#ifdef __MINGW32__
MUTEX DCCChatClient::Mutex_DCCChatClientMM = 0;
#else
MUTEX DCCChatClient::Mutex_DCCChatClientMM = 0;
#endif
#endif

// Constructor
DCCChatClient::DCCChatClient() {
   TRACE();
   RetPointer = new char[1024];
   FileCount = 0; // No files added yet.
   DirDepth = 0;  // Depth of directory. 1 => root.

   // Default the FFLC version to 1
   FFLCversion = 1;

#if 0
   // Create the Mutex.
#ifdef __MINGW32__
   if (Mutex_DCCChatClientMM == 0) {
      Mutex_DCCChatClientMM = CreateMutex(NULL, FALSE, NULL);
   }
#else
   if (Mutex_DCCChatClientMM == 0) {
      pthread_mutex_init(&Mutex_DCCChatClientMM, NULL);
   }
#endif
#endif

}

// Destructor
DCCChatClient::~DCCChatClient() {
   TRACE();
   delete [] RetPointer;
}

void DCCChatClient::init(DCC_Container_t *DCC_Container) {

   TRACE();

   // First Thing is to save stuff in the DCC_Container so our private
   // functions have easy access to all the information.
   XGlobal = DCC_Container->XGlobal;
   Connection = DCC_Container->Connection;
   RemoteNick = DCC_Container->RemoteNick;
   RemoteDottedIP = DCC_Container->RemoteDottedIP;
   ClientType = DCC_Container->ClientType;
   CurrentSends = DCC_Container->CurrentSends;
   TotalSends = DCC_Container->TotalSends;
   CurrentQueues = DCC_Container->CurrentQueues;
   TotalQueues = DCC_Container->TotalQueues;
   TriggerName = DCC_Container->TriggerName;
   FileName = DCC_Container->FileName;

   // Also store our nick in MyNick.
   XGlobal->getIRC_Nick(MyNick);
   XGlobal->resetIRC_Nick_Changed();
}

// A chat where messages are transferred to and from the "DCC Chat" window
// Tab in the UI.
bool DCCChatClient::justChat(DCC_Container_t *DCC_Container) {
bool retvalb = true;
char *message;
FilesDetail *FD;

   TRACE();

   // First Thing is to save stuff in the DCC_Container so our private
   // functions have easy access to all the information.
   init(DCC_Container);

   // We should basically receive lines here, and push it to the UI 
   // interface.
   // The UI interface, will basically redirect its text entry to the writeLine
   // This causes two threads working on the same Connection. So, check on
   // mutual exclusion there.

   // First change label in the UI.
   sprintf(RetPointer, "*DCC_CHAT_NICK* %s", RemoteNick);
   XGlobal->IRC_ToUI.putLine(RetPointer);

   // Put in UI that Chat has started.
   sprintf(RetPointer, "DCC-Chat * 09Chatting with %s", RemoteNick);
   XGlobal->IRC_ToUI.putLine(RetPointer);

   do {
      retvalb = recvLine();
      if (retvalb == false) break;

      // we could come here on a timeout as well, on which we quit.
      if (strlen(RetPointer) == 0) {
         retvalb = false;
         break;
      }

      // Pass it to UI
      message = new char[strlen(RetPointer) + strlen(RemoteNick) + 16];
      sprintf(message, "DCC-Chat <%s> %s", RemoteNick, RetPointer);
      XGlobal->IRC_ToUI.putLine(message);
      delete [] message;

      if (strcasecmp(RetPointer, "quit") == 0) retvalb = false;

   } while (retvalb);

   // Inform in the UI that the chat has ended.
   sprintf(RetPointer, "DCC-Chat 04* The Chat has terminated.");
   XGlobal->IRC_ToUI.putLine(RetPointer);

   return(retvalb);
}

// Sole purpose is to get the recursive directory listing.
// That is initialised to DIR_SEP.
bool DCCChatClient::getDirListing(DCC_Container_t *DCC_Container) {
bool retvalb = false;

   TRACE();

   // First Thing is to save stuff in the DCC_Container so our private
   // functions have easy access to all the information.
   init(DCC_Container);

   // As we have connected to server it definitely is accessible.
   // So lets first remove the "Inaccessible Server" entry. It was added
   // by the ToTriggerThr/ToTriggerNowThr along with "TriggerTemplate"
   XGlobal->FilesDB.delFilesDetailNickFile(RemoteNick, "Inaccessible Server");

   if (waitForInitialPrompt() == false) return(retvalb);

   // Call getDirListingMM() if its a MasalaMate Client.
   if (ClientType == IRCNICKCLIENT_MASALAMATE) {

      // Serialise calls to this across all threads. So that each one has
      // a complete listing on a nick by nick basis when exchanging content.
      // No need for serialisation for the moment.
#if 0
      WaitForMutex(Mutex_DCCChatClientMM);
#endif
      retvalb = getDirListingMM();
#if 0
      ReleaseMutex(Mutex_DCCChatClientMM);
#endif

      if (retvalb) {
         sprintf(RetPointer, "QUIT\n");
         sendLine();

         // Update UI that we got the Dir Listing.
         sprintf(RetPointer, "Server 11Trigger: Obtained Extended File Listing from %s", RemoteNick); 
         XGlobal->IRC_ToUI.putLine(RetPointer);
      }
      return(retvalb);
   }

   // call getRecurseDirListing(current dir) once we get the prompt

   // Issue the first primary triggering DIR command.
   strcpy(RetPointer, "DIR\n");
   if (sendLine() == true) {
      recurGetDirListing("\\"); // starting at root and exit.

      // Check if we got anything for this nick, if not add an entry
      // which is dummy, so we dont repeatedly access his trigger.
      // Our caller has deleted all files of nick, and kept only the template.
      if (FileCount == 0) {
         FilesDetail *FD;

         FD = new FilesDetail;
         XGlobal->FilesDB.initFilesDetail(FD);
         FD->FileName = new char[17];
         strcpy(FD->FileName, "No Files Present");
         FD->FileSize = 1;
         FD->Nick = new char[strlen(RemoteNick) + 1];
         strcpy(FD->Nick, RemoteNick);
         FD->TriggerType = FSERVCTCP;
         FD->TriggerName = new char[strlen(TriggerName) + 1];
         // Above can core dump if TriggerName is NULL. It shouldnt be
         strcpy(FD->TriggerName, TriggerName);

         FD->ClientType = ClientType;
         FD->CurrentSends = CurrentSends;
         FD->TotalSends = TotalSends;
         FD->CurrentQueues = CurrentQueues;
         FD->TotalQueues = TotalQueues;

         FD->DownloadState = DOWNLOADSTATE_SERVING;
         // Lets add to the FilesList DB
         XGlobal->FilesDB.addFilesDetail(FD);
      }

      sprintf(RetPointer, "QUIT\n");
      if (sendLine() == true) {
         // Update UI that we got the Dir Listing.
         sprintf(RetPointer, "Server 11Trigger: Obtained File Listing of %s", RemoteNick); 
         XGlobal->IRC_ToUI.putLine(RetPointer);

         // Lets now print out the Statistics.
         XGlobal->lock();
         XGlobal->DirAccess_BytesIn += (double) Connection->BytesReceived;
         XGlobal->DirAccess_BytesOut += (double) Connection->BytesSent;
         sprintf(RetPointer, "Server 09FServ Access: Of %s. Bytes In: %lu Out: %lu. Total Bytes In: %g Out: %g", RemoteNick, Connection->BytesReceived, Connection->BytesSent, XGlobal->DirAccess_BytesIn, XGlobal->DirAccess_BytesOut);
         XGlobal->unlock();
         XGlobal->IRC_ToUI.putLine(RetPointer);
         retvalb = true;
      }
   }
   return(retvalb);
}

// Below is called only by getDirListing() for the recursive dir listing
// extraction. Hence its private.
// Assumption the caller has already CDed to CurDir and sent the DIR command.
// So we extract the file listing, till we reach the prompt.
// Adding to FilesDB all filenames gleaned.
// Adding to DirNames all directory names.
// Once prompt is got, we do a for loop on the DirNames,
// each time issuing a CD and a DIR and calling ourselves.
// CurDir is of form: \ or \Dir1 or \Dir1\Dir2
// Returns immediately if we have hit the limits of: DirDepth = 6.
//  or Total files we have gotten so far > 200
// If depth is 1, we dont check its filesize.
#define DIRLISTING_MAX_FILES 1000
#define DIRLISTING_MAX_DEPTH 4
#define DIRLISTING_MAX_FILESIZE 1048577
void DCCChatClient::recurGetDirListing(char *CurDir) {
LineQueue DirNames;
int str_len;
char NextDir[1024];

   TRACE();

   // Increase the Dir Depth. 1 => Root. (after its ++ed)
   DirDepth++;

   // We glean DirNames and FileNames till "End of list" is received.

   while (recvLine() == true) {
      // Check if we received "End of list" => DIR listing over for cur dir.
      if (strncasecmp(RetPointer, "End of list", 11) == 0) {
         // We hit the end of directory listing.

         // If DIRLISTING_MAX_DEPTH exceeded or DIRLISTING_MAX_FILES exceeded
         if ( (DirDepth >= DIRLISTING_MAX_DEPTH) || (FileCount >= DIRLISTING_MAX_FILES) ) {
            // Now we have to get back one dir level.
            doCD(CurDir, "..");
            DirDepth--;
            return;
         }

         // For each directory name in DirNames, we issue a CD to that
         // directory and issue a DIR. Then call ourselves.
         do {
            if (DirNames.isEmpty() == true) break;

            DirNames.getLine(NextDir);

            // Issue the CD
            if (doCD(CurDir, NextDir) == false) {
               // On error skip this directory.
               DirDepth--;
               DirNames.getLineAndDelete(RetPointer);
               continue;
            }

            // Issue the DIR
            sprintf(RetPointer, "DIR\n");
            if (sendLine() == false) {
               DirDepth--;
               return;
            }

            // We now recurse.
            DirNames.getLineAndDelete(RetPointer);
            if (strlen(CurDir) == 1) {
               sprintf(NextDir, "\\%s", RetPointer);
            }
            else {
               sprintf(NextDir, "%s\\%s", CurDir, RetPointer);
            }
            recurGetDirListing(NextDir);

         } while (true);

         // Now we have to get back one dir level.
         if (doCD(CurDir, "..") == false) {
            DirDepth--;
            return;
         }

         break; // Done with the current directory fully.
      }

      // This is where we extract each line which is a response to a
      // DIR command.
      // The line is either a file information, or a directory,
      // or a line like: "SysReset 2.53." or "MasalaMate blah ...", ".."
      // or the DIR listing start line like: [\Dir1\*.*]
      // RetPointer has line.

      if (strstr(RetPointer, "SysReset")) continue;
      if (strstr(RetPointer, "MasalaMate")) continue;
      if (strstr(RetPointer, "\\*.*")) continue;
      COUT(cout << "DCCChatClient:CurDir: " << CurDir << endl;)

      if (strcmp(RetPointer, "..") == 0) continue;

      // Now what follows is either a Directory or a FileName.
      if (extractFileInformation(CurDir) == true) continue;

      // We are here => its a directory. Add it in DirNames.
      DirNames.putLine(RetPointer);
   }
   DirDepth--;
   return;
}

// recurGetDirListing() calls this.
// RetPointer contains possible FileName line.
// returns true if FileName recognised and processed.
// returns false if it doesnt recognise it as a filename line.
// We can assume its a directory if it returns false.
bool DCCChatClient::extractFileInformation(char *CurDir) {
LineParse LP;
const char *parseptr;
int word_cnt, str_pos;
char KMG;
char *tempstr, *endptr;
long templong;
FilesDetail *FD;

   TRACE();

   // RetPointer contains a possible file information.
   // example: manasaurmanovigyan.avi  499 mb
   // or: lovegames.avi   34.4MB
   // or: lovegames.avi N/A

   LP = RetPointer;
   parseptr = LP.removeNonPrintable();
   LP = (char *) parseptr;
   parseptr = LP.removeConsecutiveDeLimiters();
   LP = (char *) parseptr;
   word_cnt = LP.getWordCount();

   // At least two words should be present for it to be a file information.
   if (word_cnt < 2) return(false);

   parseptr = LP.getWord(word_cnt);

   do {

      if (strcasecmp(parseptr, "N/A") == 0) {
         // So that parseptr hold filename, when it breaks out of here.
         parseptr = LP.getWordRange(1, word_cnt - 1);
         // Set templong to some arbit file size.
         templong = 1289748; // 1.23 MB
         break;
      }

      // The last character should be 'b' or 'B'
      str_pos = strlen(parseptr) - 1;
      if ( (parseptr[str_pos] != 'b') && (parseptr[str_pos] != 'B') ) {
         COUT(cout << "FServ: last char not b or B: " << parseptr << endl;)
         return(false);
      }

      // Its either kb or mb or gb
      KMG = parseptr[str_pos - 1];
      if ( (KMG >= 'A') && (KMG <= 'Z') ) {
         KMG = KMG + ('a' - 'A');
      }

      if ( (KMG != 'k') && (KMG != 'm') && (KMG != 'g') ) {
         COUT(cout << "FServ: KMG not k or m or g: " << KMG << endl;)

         // This could be a file < 1 KB => "sysreset.cnt    679B"
         // We ignore this file, but return true, so that our caller 
         // doesnt treat it like a directory.
         if ( (KMG >= '0') && (KMG <= '9') ) return(true);
   
         return(false);
      }

      if (str_pos > 1) {
         // Implies the size is in the current string
         tempstr = new char[strlen(parseptr) + 1];
         strcpy(tempstr, parseptr);
         tempstr[str_pos - 1] = '\0';
      }
      else {
         parseptr = LP.getWord(word_cnt - 1);
         tempstr = new char[strlen(parseptr) + 1];
         strcpy(tempstr, parseptr);
      }
      templong = strtol(tempstr, &endptr, 10);
      // Dont delete tempstr now, as *endptr points within it.

      if ( (*endptr != '\0') && (*endptr != '.') ) {
         COUT(cout << "FServ: *endptr not zero or .: " << *endptr << endl;)
         delete [] tempstr;
         return(false);
      }
      delete [] tempstr;

      if (KMG == 'k') templong = templong * 1024;
      else if (KMG == 'm') templong = templong * 1024 * 1024;
      else if (KMG == 'g') templong = templong * 1024 * 1024 * 1024;

      if (str_pos > 1) {
         // File size is in form 250mb
         parseptr = LP.getWordRange(1, word_cnt - 1);
      }
      else {
         // File size is in form 250 mb
         parseptr = LP.getWordRange(1, word_cnt - 2);
      }

   } while (false);
   // parseptr now holds the file name.

   // We ignore thumbs.db
   if (strcasecmp(parseptr, "thumbs.db") == 0) {
      return(true);
   }

   // Lets see if we are DirDepth > 1 and FileSize is < 1MB
   // We return if it satisfies the condition.
   // We return true => its not a directory.
   if ( (DirDepth > 1) && (templong <= DIRLISTING_MAX_FILESIZE) ) {
      return(true);
   }

   // Lets add the file detail.
   FD = new FilesDetail;
   XGlobal->FilesDB.initFilesDetail(FD);
   FD->FileName = new char[strlen(parseptr) + 1];
   strcpy(FD->FileName, parseptr);
   if (strlen(CurDir) > 1) {
      FD->DirName = new char[strlen(CurDir)];
      strcpy(FD->DirName, &CurDir[1]); // Saved without the starting DIR_SEP_CHAR
   }
   FD->FileSize = templong;
   FD->Nick = new char[strlen(RemoteNick) + 1];
   strcpy(FD->Nick, RemoteNick);
   FD->TriggerType = FSERVCTCP;

   // This can core dump if TriggerName is NULL. It shouldnt be
   FD->TriggerName = new char[strlen(TriggerName) + 1];

   strcpy(FD->TriggerName, TriggerName);
   FD->ClientType = ClientType;
   FD->CurrentSends = CurrentSends;
   FD->TotalSends = TotalSends;
   FD->CurrentQueues = CurrentQueues;
   FD->TotalQueues = TotalQueues;

   // All Sysreset files are non partial files. 'S'.
   FD->DownloadState = DOWNLOADSTATE_SERVING;

   // Lets add to the FilesList DB
   XGlobal->FilesDB.addFilesDetail(FD);

   // Keep count.
   FileCount++;

   return(true);
}

// Sole purpose is to issue a GET for the FileName from the FServ.
// Assume that FileName contains fully qualified relative path.
// FileName is of the form: file1.avi or DIR1\DIR2\file1.avi
bool DCCChatClient::getFile(DCC_Container_t *DCC_Container) {
bool retvalb = false;
LineParse LP;
const char *parseptr;
int cd_count;
char CurDir[1024];
FilesDetail *FD;
long q_pos;
char client_type;
size_t size_of_partial_file;

   TRACE();

   init(DCC_Container);

   strcpy(CurDir, "\\");
   COUT(cout << "DCCChatClient::getFile: FileName: " << FileName << endl;)

   // Wait for the initial prompt.
   if (waitForInitialPrompt() == false) return(retvalb);

   LP = FileName;
   LP.setDeLimiter('\\'); // Its not DIR_SEP_CHAR. (same on Ux or Wx)
   parseptr = LP.removeConsecutiveDeLimiters();
   LP = (char *) parseptr;
   LP.setDeLimiter('\\');

   cd_count = LP.getWordCount();
   for (int i = 1; i < cd_count; i++) {
      // We CD to the required Directory.
      parseptr = LP.getWord(i);
      if (doCD(CurDir, (char *) parseptr) == false) return(retvalb);

      if (strlen(CurDir) == 1) {
         sprintf(CurDir, "\\%s", parseptr);
      }
      else {
         strcat(CurDir, "\\");
         strcat(CurDir, parseptr);
      }
   }

   // We are at the required directory. We just issue the get now.
   // parseptr contains the bare filename (no dir or dir seperators etc).
   parseptr = LP.getWord(cd_count);

   FD = XGlobal->FServClientInProgress.getFilesDetailListOfNick(RemoteNick);
   client_type = XGlobal->NickList.getNickClient(RemoteNick);

   // Have to get the size of this file if it exists in our Partial Folder.
   // This is required to pass to GETFROM/GETFROMPARTIAL command to be
   // used with MM server.
   XGlobal->lock();
   sprintf(CurDir, "%s%s%s", XGlobal->PartialDir, DIR_SEP, parseptr);
   XGlobal->unlock();

   // On file not existing it will set file size to 0.
   getFileSize(CurDir, &size_of_partial_file);
   if (size_of_partial_file > FILE_RESUME_GAP) {
      // As we will resume from size - FILE_RESUME_GAP
      size_of_partial_file = size_of_partial_file - FILE_RESUME_GAP;
   }
   COUT(cout << "DCCChatClient::getFile File: " << CurDir << " ClientType: " << client_type << " Resume From Size: " << size_of_partial_file << endl;)

   if (FD->DownloadState == DOWNLOADSTATE_PARTIAL) {
      // This is a partial file get MM to MM
      sprintf(RetPointer, "GETFROMPARTIAL %lu %s\n", size_of_partial_file, parseptr);
   }
   else {
      if (client_type == IRCNICKCLIENT_MASALAMATE) {
         sprintf(RetPointer, "GETFROM %lu %s\n", size_of_partial_file, parseptr);
      }
      else {
         sprintf(RetPointer, "GET %s\n", parseptr);
      }
   }

   COUT(cout << RetPointer;)

   if (sendLine() == false) return(false);

   // Update DwnldWaiting
   XGlobal->DwnldWaiting.addFilesDetail(FD);
   FD = NULL; // To prevent reuse.

   // Analyse the line received as response.
   if (analyseGetResponse(&q_pos) == false) return(false);

   // If q_pos is 0 => immediately sending
   if (q_pos == 0) return(true);

   // Issue sends
   sprintf(RetPointer, "SENDS\n", parseptr);
   COUT(cout << RetPointer;)
   if (sendLine() == false) return(false);

   // Analyse the lines received as response.
   if (analyseSendsResponse() == false) {
      strcpy(RetPointer, "QUIT\n");
      retvalb = sendLine();
      return(retvalb);
   }

   // Issue queues
   sprintf(RetPointer, "QUEUES\n", parseptr);
   COUT(cout << RetPointer;)
   if (sendLine() == false) return(false);

   // Analyse the lines received as response.
   retvalb = analyseQueuesResponse();

   // Exit out
   strcpy(RetPointer, "QUIT\n");
   retvalb = sendLine();

   return(retvalb);
}

// Sole purpose is to remove from Q.
// Assume that FileName does NOT contain the relative path in server.
// FileName is of the form: file1.avi
bool DCCChatClient::removeFromQ(DCC_Container_t *DCC_Container) {
bool retvalb = false;
char *tmpcharstr;

   TRACE();

   init(DCC_Container);

   COUT(cout << "DCCChatClient::removeFromQueue" << endl;)

   do {
      // Wait for the initial prompt.
      if (waitForInitialPrompt() == false) break;

      strcpy(RetPointer, "CLR_QUEUES\n");
      if (sendLine() == false) break;

      // We can also get an empty line in the case of Iroffer FServ.
      // So skip over empty lines.
      do {
         // lets get the next line.
         if (recvLine() == false) break;
      } while (RetPointer[0] == '\0');

      // This is the response received.

      COUT(cout << "DCCChatClient::removeFromQ: " << RetPointer << endl;)

      // Lets put that up in UI.
      tmpcharstr = new char[1024];
      sprintf(tmpcharstr, "Server 09Cancel Queue: Clearing %s. %s Responds: %s", FileName, RemoteNick, RetPointer);
      XGlobal->IRC_ToUI.putLine(tmpcharstr);
      delete [] tmpcharstr;

      // Quit.
      strcpy(RetPointer, "QUIT\n");
      retvalb = sendLine();

   } while (false);

   return(retvalb);
}

// CDs to the dir directory.
// return true on success, false otherwise.
// dir can be DIR1, not DIR1\DIR2. It also can be ..
// If cur_dir is root dir it should be \
// cur_dir is of form: \ or \Dir1 or \Dir1\Dir2
// RetPointer is used so caller should not expect it to be undisturbed.
// Return false also if "Invalid folder" is received as response.
bool DCCChatClient::doCD(char *cur_dir, char *to_dir) {
char *tmp_dir;
bool retvalb;

   TRACE();

   // Prepare how the prompt will look.
   if (strcmp(to_dir, "..") == 0) {
   LineParse LP;
   const char *parseptr;
   int word_cnt;

      if (strlen(cur_dir) == 1) {
         // We are already at the base dir. Cant do ..
         // Just flag a success.
         return(true);
      }

      // We are going back one directory.

      LP = cur_dir;
      LP.setDeLimiter('\\');

      // This discards \ at start too.
      parseptr = LP.removeConsecutiveDeLimiters();
      LP = (char *) parseptr;
      LP.setDeLimiter('\\');
      word_cnt = LP.getWordCount();

      if (word_cnt <= 1) {
         // we will get to base dir after this cd ..
         tmp_dir = new char[3];
         strcpy(tmp_dir, "[\\");
      }
      else {
         // Get the correct directory dest dir name.
         parseptr = LP.getWordRange(1, word_cnt - 1);
         tmp_dir = new char[strlen(parseptr) + 3];
         sprintf(tmp_dir, "[\\%s", parseptr);
      }
   }
   else {
      // to_dir is not ..
      tmp_dir = new char[strlen(cur_dir) + strlen(to_dir) + 3];
      if (strlen(cur_dir) == 1) {
         sprintf(tmp_dir, "[\\%s", to_dir);
      }
      else {
         sprintf(tmp_dir, "[%s\\%s", cur_dir, to_dir);
      }
   }

   // Issue the CD
   sprintf(RetPointer, "CD %s\n", to_dir);
   if (sendLine() == false) {
      delete [] tmp_dir;
      return(false);
   }

   COUT(cout << "doCD: cd from: " << cur_dir << " to: " << to_dir << " prompt expected: " << tmp_dir << endl);

   // Wait for prompt.
   while (true) {
      retvalb = recvLine();
      if (retvalb == false) break;
         
      if ( (strncasecmp(RetPointer, tmp_dir, strlen(tmp_dir)) == 0) &&
           (RetPointer[strlen(RetPointer) - 1] == ']') ) {
         // Non Firewall prompt = [\DIR]
         // FireWall prompt = [\DIR\]
         // We hit the prompt. and we are here => retvalb = true.
         break;
      }

      // Check if we received: "Invalid folder"
      if (strncasecmp(RetPointer, "Invalid folder", 1) == 0) {
         retvalb = false;
         break;
      }
   }

   delete [] tmp_dir;
   return(retvalb);
}

// Returns true if the initial File Server prompt = [\] has been received.
// Returns false for a failure
// This is where we grab the FFLC Version number that the server supports
// and set FFLCVersion.
// Here is another place we grab the Sends and Queues information.
// and update FilesDB wiht it.
bool DCCChatClient::waitForInitialPrompt() {
bool retvalb;
FServParse P; // just to remove colors.
const char *parseptr;

   TRACE();
   while (true) {
      retvalb = recvLine();
      if (retvalb == false) break;

      parseptr = P.removeColors(RetPointer);
      strcpy(RetPointer, parseptr);
      
      // Below all retvalb is true, as recvLine() was succesful.
      if (strcmp(RetPointer, "[\\]") == 0) {
         // We got the prompt.
         break;
      }

      // if we receive: ['C' for more, 'S' to stop]
      // we send an S, and move on.
      if (strcasecmp(RetPointer, "['C' for more, 'S' to stop]") == 0) {
         strcpy(RetPointer, "S\n");
         sendLine();
      }

      // If we receive: "Use: commands listed above - FFLC int"
      // then update FFLCversion.
      if (strncasecmp(RetPointer, "Use: commands listed above - FFLC ", 34) == 0) {
         FFLCversion = (int) strtol(&RetPointer[34], NULL, 10);
      }

      // Try to grab the Sends/Queues line.
      // Transfer Status: Sends:[1/2] - Queues:[0/10]
      if (strncasecmp(RetPointer, "Transfer Status:", 16) == 0) {
         LineParse LineP;
         int sends, t_sends, queues, t_queues;
         // Extract the sends line.
         LineP = RetPointer;
         LineP.setDeLimiter('[');
         parseptr = LineP.getWord(2);
         LineP = (char *) parseptr;
         LineP.setDeLimiter(']');
         parseptr = LineP.getWord(1);

         // So parseptr has 1/2
         LineP = (char *) parseptr;
         LineP.setDeLimiter('/');
         parseptr = LineP.getWord(1);
         sends = (int) strtol(parseptr, NULL, 10);
         parseptr = LineP.getWord(2);
         t_sends = (int) strtol(parseptr, NULL, 10);

         // Extract the Queues line.
         LineP = RetPointer;
         LineP.setDeLimiter('[');
         parseptr = LineP.getWord(3);
         LineP = (char *) parseptr;
         LineP.setDeLimiter(']');
         parseptr = LineP.getWord(1);

         // So parseptr has 0/10
         LineP = (char *) parseptr;
         LineP.setDeLimiter('/');
         parseptr = LineP.getWord(1);
         queues = (int) strtol(parseptr, NULL, 10);
         parseptr = LineP.getWord(2);
         t_queues = (int) strtol(parseptr, NULL, 10);

         // We have gotten all the information. Lets update FilesDB.
         // We only update if t_sends and t_queues are not 0.
         // This comes in handy when a manual ctcp trigger is issued and
         // the TriggerTemplate does not have the sends/queues information.
         // Hence for normal cases, we at least dont destroy legit send/
         // queue information from FilesDB
         if ((t_sends != 0) && (t_queues != 0)) {
            XGlobal->FilesDB.updateSendsQueuesOfNick(RemoteNick, sends, t_sends, queues, t_queues);
            // Update the sends/q information in our private , which is 
            // used to populate FilesDB later.
            CurrentSends = sends;
            TotalSends = t_sends;
            CurrentQueues = queues;
            TotalQueues = t_queues;
         }
      }
   }

   return(retvalb);
}

// A SENDS has been issued. Lets take action as per the response.
// Note that as we are collecting SENDS info to populate in DwnldWaiting
// it is highly likely that the entry in DwnldWaiting has moved into
// DwnldsInProgress, as a result of the file starting to download.
bool DCCChatClient::analyseSendsResponse() {
LineParse LineP;
const char *parseptr;
char *tmpcharstr;
FServParse P; // just to remove colors.
long CurrentSendsCount;
long TotalSendsCount;
long i;
char **sends_info;
bool retvalb;
char *SendFileName = NULL, *SendTo = NULL, *SendRate = NULL, *SendETA = NULL;
char *SendFileSize = NULL, *SendPercent = NULL, *SendResend = NULL;
char *SendNumber = NULL;
int word_index, word_cnt;

   TRACE();

   // We can also get an empty line in the case of Iroffer FServ.
   // So skip over empty lines.
   do { 
      // lets get the next line.
      if (recvLine() == false) return(false);
   } while (RetPointer[0] == '\0');

   // The Response is as follows:
   // Total Number Of Sends Currently: 1/1
   // Send 1: fc3-i386-dvd.iso 2.3GB is 0% done at 10711 cps. Eta: 2days 15hrs 57mins 40secs. Sending to: Sur4802.
   // etc.
   // Also if no sends then it replies as follows:
   // Total Number of Sends Currently: 0/1
   // All send slots open

   parseptr = P.removeColors(RetPointer);
   LineP = (char *) parseptr;
   COUT(cout << "DCCChatClient::analyseSendsResponse: " << parseptr << endl;)
   // We are expecting the form: Total Number of Sends Currently: x/y
   if (LineP.isWordsInLine("Total Number of Sends Currently:") == false) {
      return(false);
   }
   if (LineP.getWordCount() != 6) return(false);

   parseptr = LineP.getWord(6);
   LineP = (char *) parseptr;
   LineP.setDeLimiter('/');
   // Extract the number of sends in progress currently.
   parseptr = LineP.getWord(1);
   CurrentSendsCount = strtol(parseptr, NULL, 10);
   if (CurrentSendsCount < 0) return(false);

   parseptr = LineP.getWord(2);
   TotalSendsCount = strtol(parseptr, NULL, 10);
   if (TotalSendsCount < 0) return(false);

   // Note that CurrentSendsCount can be less or equal or more than
   // TotalSendsCount.
   if (recvLine() == false) return(false);

   // Now we expect lines of the form:
   // Send 1: fc3-i386-dvd.iso 2.3GB is 0% done at 10711 cps. Eta: 2days 15hrs 57mins 40secs. Sending to: Sur4802.
   // Send 1: fc3-i386-dvd.iso 2.3GB is 0% done at 10711 cps. Eta: 2days 15hrs 57mins 40secs. Sending to: Sur4802. ReSend: 2.
   // and there will be CurrentSendsCount number of such lines.
   // Also we will receive: Closing Idle connection in ... if detailed
   // sends is blocked in the sysreset server.
   // at least 11 words.
   // Now Eta is variable length, and so is filename.
   // Hence we do a single pass thru the words in line, and note down index
   // of Sending, is, to help us extract them
   sends_info = new char*[CurrentSendsCount + 1];
   retvalb = true;
   i = 0;
   while (i != CurrentSendsCount) {
      int IndexSending = 0, IndexIs = 0;
      i++;

      parseptr = P.removeColors(RetPointer);
      COUT(cout << "DCCChatClient::analyseSendsResponse: " << parseptr << endl;)
      LineP = (char *) parseptr;

      parseptr = LineP.getWord(1);

      word_index = LineP.getWordCount();
      if (strcasecmp(parseptr, "Send") || (word_index < 11) ) {
         i--;
         break;
      }

      // Lets extract the interesting information from the line.
      word_cnt = word_index;
      for (int kk = 1; kk <= word_index; kk++) {
         parseptr = LineP.getWord(kk);
         if (strcasecmp(parseptr, "is") == 0) IndexIs = kk;
         else if (strcasecmp(parseptr, "Sending") == 0) {
            IndexSending = kk;
            break;
         }
      }
      if ( (IndexIs == 0) || (IndexSending == 0) ) {
         i--;
         break;
      }

      // Second word is 2: (send number)
      parseptr = LineP.getWord(2);
      SendNumber = new char[strlen(parseptr) + 1];
      strcpy(SendNumber, parseptr);
      SendNumber[strlen(parseptr) - 1] = '\0';
      
      parseptr = LineP.getWord(word_index - 1);
      // This will be either Resend: or by.
      if (strcasecmp(parseptr, "Resend:") == 0) {
         parseptr = LineP.getWord(word_index);
         SendResend = new char[strlen(parseptr) + 1];
         strcpy(SendResend, parseptr);
         SendResend[strlen(parseptr) - 1] = '\0';
         word_index = word_index - 2; // points to nick.
         parseptr = LineP.getWord(word_index - 1);
      }
      else {
         SendResend = new char[2];
         strcpy(SendResend, "-");
      }
      if (strcasecmp(parseptr, "to:") == 0) {
         parseptr = LineP.getWord(word_index);
         // This will be the nick followed by .
         SendTo = new char[strlen(parseptr) + 1];
         strcpy(SendTo, parseptr);
         // Remove the . at end.
         SendTo[strlen(parseptr) - 1] = '\0';
      }
      else {
         i--;
         delete [] SendNumber;
         delete [] SendResend;
         break;
      }

      // % = IndexIs + 1
      parseptr = LineP.getWord(IndexIs + 1);
      SendPercent = new char[strlen(parseptr) + 1];
      strcpy(SendPercent, parseptr); // Save with the % sign.

      // cps = IndexIs + 4
      parseptr = LineP.getWord(IndexIs + 4);
      SendRate = new char[strlen(parseptr) + 1];
      strcpy(SendRate, parseptr);

      // ETA =  IndexIs + 7 to IndexSending - 1
      parseptr = LineP.getWordRange(IndexIs + 7, IndexSending - 1);
      SendETA = new char[strlen(parseptr) + 1];
      strcpy(SendETA, parseptr);

      // Move word_index to point to filename filesize, seperated by 127
      // Do this last as we loose the orig line in LineP.
      // FileName + FileSize = 3 to IndexIs - 1
      parseptr = LineP.getWordRange(3, IndexIs - 1);
      LineP = (char *) parseptr;
      LineP.setDeLimiter(127);
      parseptr = LineP.getWord(2);
      SendFileSize = new char[strlen(parseptr) + 1];
      strcpy(SendFileSize, parseptr);
      parseptr = LineP.getWord(1);
      SendFileName = new char[strlen(parseptr) + 1];
      strcpy(SendFileName, parseptr);

      COUT(cout << "DCCChatClient::analyseSendsResponse: SendNumber: " << SendNumber << " SendTo: " << SendTo << " FileName: " << SendFileName << " FileSize: " << SendFileSize << " Send %: " << SendPercent << " SendRate: " << SendRate << " SendETA: " << SendETA << " SendResend: " << SendResend << endl;)

      sends_info[i - 1] = new char[strlen(SendNumber) + strlen(SendTo) + strlen(SendFileName) + strlen(SendFileSize) + strlen(SendPercent) + strlen(SendRate) + strlen(SendETA) + 32];
      sprintf(sends_info[i - 1], "%s\t%s\t%s\tS %s\t%s\t%s\t%s\t%s", SendFileName, SendFileSize, SendTo, SendNumber, SendResend, SendRate, SendPercent, SendETA);
      sends_info[i] = NULL;
      COUT(cout << "sends_info[" << i - 1 << "]: " << sends_info[i - 1] << endl;)
      delete [] SendTo;
      delete [] SendFileName;
      delete [] SendFileSize;
      delete [] SendPercent;
      delete [] SendRate;
      delete [] SendETA;
      delete [] SendResend;
      delete [] SendNumber;

      if (i != CurrentSendsCount) {
         if (recvLine() == false) {
            retvalb = false;
            break;
         }
      }
   }

   if (i == 0) {
      delete [] sends_info;
      sends_info = NULL;
   }

   // Add sends_info onto the DwnldWaiting FD.
   // Before we add, lets delete and remove the old information.
   FilesDetail *FD = XGlobal->DwnldWaiting.getFilesDetailListNickFile(RemoteNick, getFileServerFileName(FileName));
   if (FD) {
      XGlobal->DwnldWaiting.updateFilesDetailNickFileData(RemoteNick, getFileServerFileName(FileName), NULL);

      deleteStringArray((char **) FD->Data);

      XGlobal->DwnldWaiting.updateFilesDetailNickFileData(RemoteNick, getFileServerFileName(FileName), sends_info);
      XGlobal->DwnldWaiting.freeFilesDetailList(FD);
   }
   else {
      // free up sends_info, and return false, so that Queues is not issued.
      deleteStringArray(sends_info);
      sends_info = NULL;
      retvalb = false;
   }
   return(retvalb);
}

// A QUEUES has been issued. Lets take action as per the response.
// A previous sends also has been issued, hence, ScanFD->Data might
// already hold some entries.
// Note that as we are collecting QUEUES info to populate in DwnldWaiting
// it is highly likely that the entry in DwnldWaiting has moved into
// DwnldsInProgress, as a result of the file starting to download.
// And in this scenario, ususally the # of Queues info we get will be
// one less than what we received.
bool DCCChatClient::analyseQueuesResponse() {
LineParse LineP;
const char *parseptr;
char *tmpcharstr;
FServParse P; // just to remove colors.
long CurrentQueuesCount;
long TotalQueuesCount;
long i;
char *QueuedBy = NULL, *QueuedFileSize = NULL, *QueuedFileName = NULL;
char **queues_info = NULL, *QueuedResend = NULL, *QueuedNumber = NULL;
bool retvalb;

   TRACE();

   // We can also get an empty line in the case of Iroffer FServ.
   // So skip over empty lines.
   do { 
      // lets get the next line.
      if (recvLine() == false) return(false);
   } while (RetPointer[0] == '\0');

   // The Response is as follows:
   // Total Number Of Queues Currently: 0/2
   // All queue slots open
   // And in the event when it has queues.
   // Total Number Of Queues Currently: 1/2
   // Queue 1: fc3-i386-dvd.iso 2.3GB was queued by Sur4802. ReSend: 1.
   // The ReSend: part will be optional in sysreset. MM needs to add that
   // ReSend part in its queue response.
   // Also FileName and FileSize are one word seperated by seperator 127.
   // Also we will receive: Closing Idle connection in ... if detailed
   // queues is blocked in the sysreset server.

   parseptr = P.removeColors(RetPointer);

   COUT(cout << "DCCChatClient::analyseQueueResponse: " << parseptr << endl;)
   LineP = (char *) parseptr;
   // We are expecting the form: Total Number of Queues Currently: x/y
   if (LineP.isWordsInLine("Total Number of Queues Currently:") == false) {
      return(false);
   }
   if (LineP.getWordCount() != 6) return(false);

   parseptr = LineP.getWord(6);
   LineP = (char *) parseptr;
   LineP.setDeLimiter('/');
   // Extract the number of queues in progress currently.
   parseptr = LineP.getWord(1);
   CurrentQueuesCount = strtol(parseptr, NULL, 10);
   if (CurrentQueuesCount < 0) return(false);

   parseptr = LineP.getWord(2);
   TotalQueuesCount = strtol(parseptr, NULL, 10);
   if (TotalQueuesCount < 0) return(false);

   COUT(cout << "DCCChatClient::analyseQueueResponse: CurrentQueuesCount: " << CurrentQueuesCount << " TotalQueuesCount: " << TotalQueuesCount << endl;)

   // Note that CurrentQueuesCount can be less or equal or more than
   // TotalQueuesCount.
   if (recvLine() == false) return(false);

   // Now we expect lines of the form:
   // Queue 1: fc3-i386-dvd.iso 2.3GB was queued by Sur4802. ReSend: 1.
   // and there will be CurrentSendsCount number of such lines.
   // at least 7 words.
   // FileName is variable length, so we get index of "was" in IndexWas
   queues_info = new char*[CurrentQueuesCount + 1];
   retvalb = true;
   i = 0;
   while (i != CurrentQueuesCount) {
      int IndexWas = 0;
      i++;

      parseptr = P.removeColors(RetPointer);
      COUT(cout << "DCCChatClient::analyseQueuesResponse: " << parseptr << endl;)
      LineP = (char *) parseptr;

      int word_index = LineP.getWordCount();
      parseptr = LineP.getWord(1);
      if (strcasecmp(parseptr, "Queue") || (word_index < 7) ) {
         i--;
         break;
      }

      // Lets extract the interesting information from the line.
      for (int kk = 1; kk <= word_index; kk++) {
         parseptr = LineP.getWord(kk);
         if (strcasecmp(parseptr, "was") == 0) {
            IndexWas = kk;
            break;
         }
      }
      if (IndexWas == 0) {
         i--;
         break;
      }

      // Second word is 2:
      parseptr = LineP.getWord(2);
      QueuedNumber = new char[strlen(parseptr) + 1];
      strcpy(QueuedNumber, parseptr);
      QueuedNumber[strlen(parseptr) - 1] = '\0';
      
      // Second last word.
      parseptr = LineP.getWord(word_index - 1);
      // This will be either Resend: or by.
      if (strcasecmp(parseptr, "Resend:") == 0) {
         parseptr = LineP.getWord(word_index);
         QueuedResend = new char[strlen(parseptr) + 1];
         strcpy(QueuedResend, parseptr);
         QueuedResend[strlen(parseptr) - 1] = '\0';
         word_index = word_index - 2; // points to nick.
         parseptr = LineP.getWord(word_index - 1);
      }
      else {
         QueuedResend = new char[2];
         strcpy(QueuedResend, "-");
      }
      if (strcasecmp(parseptr, "by") == 0) {
         parseptr = LineP.getWord(word_index);
         QueuedBy = new char[strlen(parseptr) + 1];
         strcpy(QueuedBy, parseptr);
         // Remove the . at end.
         QueuedBy[strlen(parseptr) - 1] = '\0';
      }
      else {
         i--;
         delete [] QueuedResend;
         delete [] QueuedNumber;
         break;
      }

      // filename filesize, seperated by 127 - index 3 to IndexWas - 1
      parseptr = LineP.getWordRange(3, IndexWas - 1);
      LineP = (char *) parseptr;
      LineP.setDeLimiter(127);
      parseptr = LineP.getWord(2);
      QueuedFileSize = new char[strlen(parseptr) + 1];
      strcpy(QueuedFileSize, parseptr);
      parseptr = LineP.getWord(1);
      QueuedFileName = new char[strlen(parseptr) + 1];
      strcpy(QueuedFileName, parseptr);

      COUT(cout << "DCCChatClient::analyseQueuesResponse: QueuedNumber: " << QueuedNumber << " ResendCount: " << QueuedResend << " QueuedBy: " << QueuedBy << " QueuedFileSize: " << QueuedFileSize << " QueuedFileName: " << QueuedFileName << endl;)
      queues_info[i - 1] = new char[strlen(QueuedNumber) + strlen(QueuedBy) + strlen(QueuedFileSize) + strlen(QueuedFileName) + 32];
      sprintf(queues_info[i - 1], "%s\t%s\t%s\tQ %s\t%s", QueuedFileName, QueuedFileSize, QueuedBy, QueuedNumber, QueuedResend);
      queues_info[i] = NULL;
      COUT(cout << "queues_info[" << i - 1 << "]: " << queues_info[i - 1] << endl;)

      // Lets free allocations which will be set each line.
      delete [] QueuedNumber;
      delete [] QueuedResend;
      delete [] QueuedBy;
      delete [] QueuedFileSize;
      delete [] QueuedFileName;

      if (i != CurrentQueuesCount) {
         if (recvLine() == false) {
            retvalb = false;
            break;
         }
      }
   }
   if (i == 0) {
      delete [] queues_info;
      queues_info = NULL;
   }

   // Add queues_info onto the DwnldWaiting FD.
   // Take into account that it already has Sends information.
   FilesDetail *FD = XGlobal->DwnldWaiting.getFilesDetailListNickFile(RemoteNick, getFileServerFileName(FileName));
   if (FD) {
      XGlobal->DwnldWaiting.updateFilesDetailNickFileData(RemoteNick, getFileServerFileName(FileName), NULL);
      char **buf = (char **) FD->Data;
      int j = getEntriesInStringArray(buf);
      // So it has a total of i + j entries.
      char **newbuf = new char *[i + j + 1];
      for (int k = 0; k < j; k++) {
         newbuf[k] = buf[k];
      }
      for (int k = 0; k < i; k++) {
         newbuf[j + k] = queues_info[k];
      }
      newbuf[i + j] = NULL;
      XGlobal->DwnldWaiting.updateFilesDetailNickFileData(RemoteNick, getFileServerFileName(FileName), newbuf);
      XGlobal->DwnldWaiting.freeFilesDetailList(FD);
      delete [] queues_info;
      queues_info = NULL;
      delete [] buf;
      retvalb = true;
   }
   else {
      // The FD has already moved to DwnldsInProgress. free up array queues.
      deleteStringArray(queues_info);
      queues_info = NULL;
      retvalb = true;
   }
   return(retvalb);
}

// A Get has been issued. Lets take action as per the response.
// We also change the TAB color appropriately.
bool DCCChatClient::analyseGetResponse(long *q_pos) {
LineParse LineP;
const char *parseptr;
char *tmpcharstr;
FServParse P; // just to remove colors.

   TRACE();

   *q_pos = 0;
   // We can also get an empty line in the case of Iroffer FServ.
   // So skip over empty lines.
   do { 
      // lets get the next line.
      if (recvLine() == false) return(false);
   } while (RetPointer[0] == '\0');

   // This is the response received.
   // If its "Sending you ..." => succes get.
   // If its " You are in Q" => in Q

   parseptr = P.removeColors(RetPointer);

   COUT(cout << "DCCChatClient::analyseGetResponse: " << RetPointer << endl;)
   LineP = (char *) parseptr;
   parseptr = LineP.getWord(1);
   tmpcharstr = new char[1024];

   // Lets put that up in UI.
   sprintf(tmpcharstr, "Server 09Download: Attempted GET %s. %s Responds: %s", FileName, RemoteNick, RetPointer);
   XGlobal->IRC_ToUI.putLine(tmpcharstr);

   if (strcasecmp("Sending", parseptr) == 0) {
      // Response = "Sending File."

      // Do not remove from DwnldWaiting, even if its an immediate send.
      // XGlobal->DwnldWaiting.delFilesDetailNickFile(RemoteNick, getFileServerFileName(FileName));

      // As we are getting actual file. If its a
      // workaround send, then on mirc 6.14
      // we will fail to receive if we quit immediately
      // now. As sock in TIME_WAIT.
      // So we sleep 10 seconds, and proceed further.
      // Not required if we are put in queue.
      sleep(10);
      delete [] tmpcharstr;
      return(true);
   }

   if (strcasecmp("Adding", parseptr) == 0) {
      // If response = "Adding your ... 7th word = 1.
      const char *parseptr2;

      parseptr2 = LineP.getWord(7);
      *q_pos = strtol(parseptr2, NULL, 10);

      XGlobal->DwnldWaiting.updateFilesDetailNickFileQ(RemoteNick, getFileServerFileName(FileName), *q_pos);

      // Instruct UI to change color if applicable.
      XGlobal->IRC_ToUI.putLine("*COLOR* Waiting");

      delete [] tmpcharstr;
      return(true);
   }

   if (strcasecmp("That", parseptr) == 0) {
      // If response = "That file  ... 9th word = 1!
      const char *parseptr2;

      parseptr2 = LineP.getWord(9);
      *q_pos = strtol(parseptr2, NULL, 10);

      if (*q_pos != 0) {
         // Update DwnldWaiting with Q #
         XGlobal->DwnldWaiting.updateFilesDetailNickFileQ(RemoteNick, getFileServerFileName(FileName), *q_pos);
      }

      // Instruct UI to change color if applicable.
      XGlobal->IRC_ToUI.putLine("*COLOR* Waiting");

      delete [] tmpcharstr;
      return(true);
   }

   // We are here. We didnt get any meaningful response.
   // Need to remove it from the DwnldWaiting
   XGlobal->DwnldWaiting.delFilesDetailNickFile(RemoteNick, getFileServerFileName(FileName));

   delete [] tmpcharstr;
   return(false);
}

// DIR listing exchange with a MasalaMate server.
bool DCCChatClient::getDirListingMM() {
Helper H;
bool retvalb;

   TRACE();

   H.init(XGlobal, Connection);
   // Not server calling. FFLCversion is version of the server we are talking
   // to.
   retvalb = H.helperFServStartEndlist(RemoteNick, false, FFLCversion);

   return(retvalb);
}

// Sends Line in RetPointer. returns successfull send or failure.
// Make sure RetPointer contains the string to be sent.
// Here we check to see if we received a DisConnect message.
bool DCCChatClient::sendLine() {
int retval, bytecount;
bool retvalb = true;
char ConnectionMessage;

   TRACE();

   bytecount = strlen(RetPointer);
   retval = Connection->writeData(RetPointer, bytecount);
   if (retval != bytecount) retvalb = false;

   COUT(cout << "DCCChatClient::sendLine: " << RetPointer << endl;)

#if 0
   FD = XGlobal->FServClientInProgress.getFilesDetailOfDottedIP(RemoteDottedIP);
   if (FD && (FD->ConnectionMessage == CONNECTION_MESSAGE_DISCONNECT) ) {
      // We have got a disconnect request cause of our quitting the client.
      COUT(cout << "DCCChatClient::sendLine Got CONNECTION_MESSAGE_DISCONNECT" << endl;)
      Connection->disConnect();
      retvalb = false;
   }
   XGlobal->FServClientInProgress.freeFilesDetailList(FD);
#else

   if (XGlobal->isIRC_QUIT()) retvalb = false;
#endif

   return(retvalb);
}

// Reads a Line into the RetPointer. Returns true on success
// Returns false on failure.
// Here we check to see if we received a DisConnect message.
bool DCCChatClient::recvLine() {
char ConnectionMessage;
bool retvalb;
int retval;

   TRACE();

   retval = Connection->readLine(RetPointer, 1023, DCCCHAT_TIMEOUT);
   if (retval < 0) retvalb = false;
   else retvalb = true;

   COUT( if (retvalb) {cout << "DCCChatClient::recvLine: " << RetPointer << endl;} )

#if 0
   FD = XGlobal->FServClientInProgress.getFilesDetailOfDottedIP(RemoteDottedIP);
   if (FD && (FD->ConnectionMessage == CONNECTION_MESSAGE_DISCONNECT) ) {
      // We have got a disconnect request cause of our quitting the client.
      COUT(cout << "DCCChatClient::recvLine Got CONNECTION_MESSAGE_DISCONNECT" << endl;)
      Connection->disConnect();
      retvalb = false;
   }
   XGlobal->FServClientInProgress.freeFilesDetailList(FD);
#else

   if (XGlobal->isIRC_QUIT()) retvalb = false;
#endif

   return(retvalb);
}
