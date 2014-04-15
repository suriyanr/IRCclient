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


#include "ThreadMain.hpp"
#include "LineParse.hpp"
#include "IRCLineInterpret.hpp"
#include "IRCChannelList.hpp"
#include "IRCNickLists.hpp"
#include "UI.hpp"
#include "FServParse.hpp"
#include "SpamFilter.hpp"
#include "FilesDetailList.hpp"
#include "Helper.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

// Thread which processes Lines in the UI_ToDwnldInit Q, and tries to set
// up things which could result in a possible download or cancel.
// Word seperator = \001
// First word = GET or CANCELQ or CANCELD or GETPARTIAL
// Second Word = NICK
// Third Word = filename.
// Fourth Word = dirname in case of GET. if dirname = space => no dir.
// GETPARTIAL too doesnt have a dirname.
void DwnldInitThr(XChange *XGlobal) {
THR_EXITCODE thr_exit = 1;
char *DwnldInitLine;
FilesDetail *FD, *ScanFD, *newFD;
LineParse LineP;
const char *parseptr;
size_t FileSize; // == 0 => removing from Q.
char *FileName;
char *DirName = NULL;
char *Nick = NULL; // Used to remove from Q.
Helper H;
bool Get, CancelQ, CancelD, GetPartial;

   TRACE_INIT_CRASH("DwnldInitThr");
   TRACE();

   H.init(XGlobal);
   DwnldInitLine = new char[1024];
   while (true) {

      XGlobal->UI_ToDwnldInit.getLineAndDelete(DwnldInitLine);
      if (XGlobal->isIRC_QUIT()) break;

      // Spurious when in gdb.
      if (strlen(DwnldInitLine) < 2) continue;

      // Even if IRC is disconnected, we should be able to word CANCELD
      // So move this till after CANCELD is taken care of.
      // if (XGlobal->isIRC_DisConnected()) continue;

COUT(cout << "DwnldInitThr: DwnldInitLine: " << DwnldInitLine << endl;)
      // Initialise the flags.
      Get = false;
      CancelQ = false;
      CancelD = false;
      GetPartial = false;

      LineP = DwnldInitLine;
      LineP.setDeLimiter('\001');
      parseptr = LineP.getWord(1);
      if (strcasecmp(parseptr, "GET") == 0) {
         Get = true;
      }
      else if (strcasecmp(parseptr, "GETPARTIAL") == 0) {
         GetPartial = true;
      }
      else if (strcasecmp(parseptr, "CANCELQ") == 0) {
         CancelQ = true;
      }
      else if (strcasecmp(parseptr, "CANCELD") == 0) {
         CancelD = true;
      }

      parseptr = LineP.getWord(2);
      Nick = new char[strlen(parseptr) + 1];
      strcpy(Nick, parseptr);

      parseptr = LineP.getWord(3);
      FileName = new char [strlen(parseptr) + 1];
      strcpy(FileName, parseptr);

      // Process the simplest case of Cancelling the Download.
      if (CancelD) {
         // Sleep the CANCEL_DOWNLOAD_TIME seconds.
         // We sleep here as TabBookWindow has sent the NoReSend CTCP
         // We assume that the NoReSend gets there in this 10 seconds,
         // to prevent the resend of this same file
         COUT(cout << "DwnldInitThr: sleeping 10 seconds before cancel" << endl;)
         sleep(CANCEL_DOWNLOAD_TIME);

         XGlobal->DwnldInProgress.updateFilesDetailNickFileConnectionMessage(Nick, FileName, CONNECTION_MESSAGE_DISCONNECT_DOWNLOAD);
         COUT(cout << "DwnldInitThr: Download disconn Nick: " << Nick << " File: " << FileName << endl;)
         // We are all done !!!!!!!!

         delete [] Nick;
         delete [] FileName;
         continue;
      }

      if (XGlobal->isIRC_DisConnected()) continue;

      if (Get) {
         parseptr = LineP.getWord(4);
         DirName = new char [strlen(parseptr) + 1];
         strcpy(DirName, parseptr);
      }
      else if (GetPartial) {
         DirName = new char[2];
         strcpy(DirName, " ");
      }
      else {
         DirName = NULL;
      }

      // Now lets get the FilesDetails list based on this information.
      // Basically the trigger related information.
      if (CancelQ) {
         // We need to remove  ourselves from this nick's Q
         FD = XGlobal->DwnldWaiting.getFilesDetailListNickFile(Nick, FileName);

         // Now remove the entry from DwnldWaiting
         XGlobal->DwnldWaiting.delFilesDetailNickFile(Nick, FileName);
      }
      else {
         FD = XGlobal->FilesDB.getFilesDetailListNickFileDir(Nick, FileName, DirName);
      }
      delete [] DirName;
      delete [] FileName;
      delete [] Nick;
      COUT(XGlobal->FilesDB.printDebug(FD);)
      ScanFD = FD; // This list has only one entry.
      // Below logic with while was present to try all nicks/triggers for file.
      while (ScanFD) {
         if (Get || GetPartial) {
            FilesDetail *DwnldFD;

            DwnldFD = XGlobal->DwnldInProgress.getFilesDetailListMatchingFileName(ScanFD->FileName);
            if (DwnldFD && DwnldFD->Connection) {
               // If this File is already being downloaded lets not try at all
               COUT(cout << "Not trying " << ScanFD->FileName << " as already being downloaded" << endl;)
               sprintf(DwnldInitLine, "Server 04Download: Not trying File %s from %s as it is already downloading.", ScanFD->FileName, ScanFD->Nick);
               XGlobal->IRC_ToUI.putLine(DwnldInitLine);
               XGlobal->DwnldInProgress.freeFilesDetailList(DwnldFD);

               break;
            }
            if (DwnldFD) {
               // XGlobal->DwnldInProgress.delFilesDetailNickFile(DwnldFD->Nick, DwnldFD->FileName);
               XGlobal->DwnldInProgress.freeFilesDetailList(DwnldFD);
            }
         }
         // Lets try to download it or remove it from Q.

         switch (ScanFD->TriggerType) {
            case XDCC:
              if (CancelQ) {
                 // Lets remove ourselves from the Q
                 sprintf(DwnldInitLine, "PRIVMSG %s :xdcc remove", ScanFD->Nick);
                 H.sendLineToNick(ScanFD->Nick, DwnldInitLine);
                 sprintf(DwnldInitLine, "Server 09Download: Removed ourselves from the Queue of %s", ScanFD->Nick);
                 XGlobal->IRC_ToUI.putLine(DwnldInitLine);
              }
              else {
                 // Add this entry in the DwnldWaiting Structure.
                 newFD = XGlobal->DwnldWaiting.copyFilesDetail(ScanFD);

                 // Remove old entry -> It might be wrong if its an entry
                 // added as part of restart from config file.
                 XGlobal->DwnldWaiting.delFilesDetailNickFile(newFD->Nick, newFD->FileName);

                 XGlobal->DwnldWaiting.addFilesDetail(newFD);
                 // Instruct UI to change color if aplicable.
                 XGlobal->IRC_ToUI.putLine("*COLOR* Waiting");
                 // Issue the Download.
                 sprintf(DwnldInitLine, "PRIVMSG %s :XDCC SEND #%d",
                         ScanFD->Nick, ScanFD->PackNum);
                 H.sendLineToNick(ScanFD->Nick, DwnldInitLine);
                 sprintf(DwnldInitLine, "Server 12Download: Trying File %s of size %ld MB from %s via XDCC pack number %d.", ScanFD->FileName, ScanFD->FileSize/1024/1024, ScanFD->Nick, ScanFD->PackNum);
                 XGlobal->IRC_ToUI.putLine(DwnldInitLine);
              }
              break;

            case FSERVCTCP:
              // Add this entry in the FServClientPending Structure.
              newFD = XGlobal->FServClientPending.copyFilesDetail(ScanFD);
              if (CancelQ) {
                 newFD->FileSize = 0; // To indicate that its a Q removal.
              }
              XGlobal->FServClientPending.addFilesDetail(newFD);
              // Issue the Trigger.
              sprintf(DwnldInitLine, "PRIVMSG %s :\001%s\001", ScanFD->Nick, ScanFD->TriggerName);
              H.sendLineToNick(ScanFD->Nick, DwnldInitLine);
              if (CancelQ) {
                 // Q removal procedings.
                 sprintf(DwnldInitLine, " Server 12Download: Trying to remove File %s from %s's Server.", ScanFD->FileName, ScanFD->Nick);
                 XGlobal->IRC_ToUI.putLine(DwnldInitLine);
              }
              else {
              // The DCC/DCCServer Thr, on getting this chat, should
              // now issue a GET of the file and record the response received.
                 sprintf(DwnldInitLine, "Server 12Download: Trying File %s of size %ld MB from %s via FServ CTCP with Trigger %s.", ScanFD->FileName, ScanFD->FileSize/1024/1024, ScanFD->Nick, ScanFD->TriggerName);
                 XGlobal->IRC_ToUI.putLine(DwnldInitLine);
              }
              break;

            case FSERVMSG:
            default:
              break;
         }

         if (ScanFD != NULL) {
            ScanFD = ScanFD->Next;
            sleep(IRCSERVER_TARGET_CHANGE_DELAY); 
            // Sleep IRCSERVER_TARGET_CHANGE_DELAY seconds between attempts.
         }
      }

      XGlobal->FilesDB.freeFilesDetailList(FD);
   }

   delete [] DwnldInitLine;

   COUT(cout << "DwnldInitThr: Quitting" << endl;)
// We dont call Exit thread as we want to go out of scope => destructors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}

