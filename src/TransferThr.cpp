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
#include "Transfer.hpp"
#include "Helper.hpp"
#include "Utilities.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

// Thread which does a Transfer (Upload/Download)
void TransferThr(DCC_Container_t *DCC_Container) {
THR_EXITCODE thr_exit = 1;
TCPConnect *TransferTCP = DCC_Container->Connection;
XChange *XGlobal = DCC_Container->XGlobal;
char *DottedIP = DCC_Container->RemoteDottedIP;
char *RemoteNick = DCC_Container->RemoteNick;
// The FileName that we get here is already prefixed with the directory.
char *FileName = DCC_Container->FileName;
char *tmp_str;
char *saveDirName;
TriggerTypeE saveTriggerType;
char saveDownloadState;
FilesDetail *FD;
Transfer T;
Helper H;
char *Response;
int retval;
size_t TransferBPS;

   TRACE_INIT_NOCRASH();
   TRACE();

   H.init(XGlobal);

   Response = new char[1024];

   COUT(cout << "TransferThr register Id: " << GetCurrentThreadId() << endl;)

   // We are already connected via TransferTCP
   COUT(cout << "Entered: TransferThr" << endl;)
   COUT(TransferTCP->printDebug();)

   // Lets get the appropriate FD
   // and move them to the respective InProgress Structures.
   switch (DCC_Container->TransferType) {
     case OUTGOING:
        FD = XGlobal->DCCSendWaiting.getFilesDetailListOfNick(RemoteNick);
        XGlobal->DCCSendWaiting.delFilesOfNick(RemoteNick);

        if (FD != NULL) {
           // We print the FD
           COUT(XGlobal->SendsInProgress.printDebug(FD);)

           COUT(cout << "TransferThr: preT.run: RemoteNick: " << FD->Nick << " FileName: " << FD->FileName << " Dotted IP: " << DottedIP << endl;)

           // Add the entry to end of list.
           // Should be last entry, cause when we save in config file,
           // an earlier send will have a smaller send number than a later
           // one, and hence on restart, smaller one will start for sure.
           XGlobal->SendsInProgress.addFilesDetailAtIndex(FD, 0);
           FD = NULL;

           // We have the FD information. Lets get this upload going.
           retval = T.run(DCC_Container);

           // Lets delete the FD from the appropriate Qs.
           tmp_str = getFileName(FileName);

           // Note that RemoteNick might be invalid, as it could have changed
           // names since T.run above.
           XGlobal->SendsInProgress.delFilesDetailFileNameDottedIP(tmp_str, DottedIP);

           COUT(cout << "TransferThr: postT.run: RemoteNick: " << RemoteNick << " FileName: " << FileName << " tmp_str: " << tmp_str << endl;)

           // Lets add it to TotalBytesSent
           XGlobal->lock();
           XGlobal->TotalBytesSent += TransferTCP->BytesSent;
           XGlobal->unlock();
           // Lets update the config file appropriately.
           // We dont update it here as when we quit, the sends are cancelled
           // Its updated only in TabBookwindow destructor and periodically
           // in TimerThr.
        }
        break;

     case INCOMING:
        // FileName contains Partial Dir.
        tmp_str = getFileName(FileName);
        FD = XGlobal->DwnldInProgress.getFilesDetailListMatchingFileName(tmp_str);
        if (FD && FD->Connection) {
           // We already have a download of same file => reject this one.
           XGlobal->DwnldInProgress.freeFilesDetailList(FD);
           FD = NULL;
           break;
        }

        // Remove it from DwnldInProgress if present.
        if (FD) {
           XGlobal->DwnldInProgress.delFilesDetailNickFile(FD->Nick, FD->FileName);

           XGlobal->DwnldInProgress.freeFilesDetailList(FD);
           FD = NULL;
        }


        // Lets make sure the FileSize and Resume Position values are sane.
        if ( (DCC_Container->FileSize > FILE_RESUME_GAP) &&
             ((DCC_Container->FileSize - FILE_RESUME_GAP) < DCC_Container->ResumePosition) ) {
           // Invalid resume => reject this one.
           break;
        }

        // Lets remove it if its present in DwnldWaiting
        // Do get the DirName from DwnldWaiting, as we will need it on
        // a reget on failed download, for right click in downloads tab
        // We should also get the TriggerType, as its used to cancel.
        // Also get the DownloadState to check if its a get of a Partial file.
        // FileName is prefixed with PartialDir
        // Delete FD->Data if present in DwnldWaiting FD.

        FD = XGlobal->DwnldWaiting.getFilesDetailListNickFile(RemoteNick, tmp_str);
        saveDirName = NULL;
        saveTriggerType = FSERVINVALID;
        // Assume its a Full file to start with, as Manual sends wont have
        // the FD in DwnldWaiting.
        saveDownloadState = DOWNLOADSTATE_SERVING;
            
        if (FD) {
           saveTriggerType = FD->TriggerType;
           saveDownloadState = FD->DownloadState;
           if (FD->DirName) {
              saveDirName = new char[strlen(FD->DirName) + 1];
              strcpy(saveDirName, FD->DirName);
           }
           if (FD->Data) {
              char **str_arr = (char **) FD->Data;
              int str_arr_index = 0;
              while (str_arr[str_arr_index]) {
                 delete [] str_arr[str_arr_index];
                 str_arr_index++;
              }
              delete [] str_arr;
           }
           XGlobal->DwnldWaiting.delFilesDetailNickFile(RemoteNick, tmp_str);
           XGlobal->DwnldWaiting.freeFilesDetailList(FD);
           FD = NULL;
        }

        // Lets add a new FD
        FD = new FilesDetail;
        XGlobal->FileServerWaiting.initFilesDetail(FD);
        FD->Nick = new char[strlen(RemoteNick) + 1];
        strcpy(FD->Nick, RemoteNick);
        FD->FileName = new char[strlen(tmp_str) + 1];
        strcpy(FD->FileName, tmp_str);
        FD->DirName = saveDirName;
        FD->DottedIP = new char[strlen(DottedIP) + 1];
        strcpy(FD->DottedIP, DottedIP);
        FD->FileSize = DCC_Container->FileSize;
        FD->TriggerType = saveTriggerType;
        FD->DownloadState = saveDownloadState;
        FD->FileResumePosition = DCC_Container->ResumePosition;
        FD->Connection = TransferTCP;
        // Delete an older one if present before adding.
        XGlobal->DwnldInProgress.delFilesDetailNickFile(FD->Nick, FD->FileName);
        
        XGlobal->DwnldInProgress.addFilesDetail(FD);
        // Instruct UI to update color, if applicable.
        XGlobal->IRC_ToUI.putLine("*COLOR* Downloads");
        FD = NULL; // cant use it now.

        // We have the FD information. Lets get this download going.
        retval = T.run(DCC_Container);

        // Lets update the FD from the DwnldInProgress Q.
        if (retval != TRANSFER_DOWNLOAD_FAILURE) {
           // Note that RemoteNick might be invalid, as it could have changed
           // names since T.run above.
           FD = XGlobal->DwnldInProgress.getFilesDetailFileNameDottedIP(tmp_str, DottedIP);

           // Its weird but sometimes, DwnldInProgress doesnt have the info
           // of RemoteNick/tmp_str which we just added above.
           // Well that was cause RemoteNick has changed his nick.
           // That is why we are using FileName/DottedIP to spot the FD.
           if (FD) {
              // Delete it for now.
              XGlobal->DwnldInProgress.delFilesDetailNickFile(FD->Nick, tmp_str);

              // Update FD according to how we want it seen in UI.
              FD->Connection = NULL;
              if (retval == TRANSFER_DOWNLOAD_SUCCESS) FD->DownloadState = DOWNLOADSTATE_SERVING;
              if (retval == TRANSFER_DOWNLOAD_PARTIAL) FD->DownloadState = DOWNLOADSTATE_PARTIAL;
              if (retval == TRANSFER_DOWNLOAD_ROLLBACK_ERROR) FD->DownloadState = DOWNLOADSTATE_PARTIAL;

              // If saveDownloadState was DOWNLOADSTATE_PARTIAL, then it 
              // remains as a DOWNLOADSTATE_PARTIAL
              // As its still a complete download of a Partial file.
              // and shouldnt be moved to serving folder.
              if (saveDownloadState == DOWNLOADSTATE_PARTIAL) {
                 FD->DownloadState = DOWNLOADSTATE_PARTIAL;
              }

              // Lets move the file to the Serving Dir if complete => 'S'
              if (FD->DownloadState == DOWNLOADSTATE_SERVING) {
                 // If its the Upgrade File, we need to do some checks
                 // and move elsewhere.
                 bool retvalb, normalmove = true;
                 char sha[41];
                 do {
                    if (strcasecmp(FD->FileName, UPGRADE_PROGRAM_NAME)) {
                       break;
                    }

                    // Check if the Nick we got it from is OP in Chat.
                    if (!IS_OP(XGlobal->NickList.getNickMode(CHANNEL_CHAT, FD->Nick))) {
                       break;
                    }

                    // Check if its same nick as Upgrade_Nick.
                    XGlobal->lock();
                    if ((XGlobal->Upgrade_Nick == NULL) || strcasecmp(XGlobal->Upgrade_Nick, FD->Nick)) {
                       XGlobal->unlock();
                       break;
                    }
                    XGlobal->unlock();

                    // Check if Upgrade_Time is > current time.
                    XGlobal->lock();
                    if (XGlobal->Upgrade_Time <= time(NULL)) {
                       XGlobal->unlock();
                       break;
                    }
                    XGlobal->unlock();

                    // Check the SHA of this file with what we have.
                    getSHAOfFile(FileName, sha);
                    XGlobal->lock();
                    if ((XGlobal->Upgrade_SHA == NULL) || strcasecmp(sha, XGlobal->Upgrade_SHA)) {
                       XGlobal->unlock();
                       break;
                    }
                    XGlobal->unlock();

                    // All is OK, can be moved as an Upgrade.
                    // 2nd parameter to moveFile = false.
                    normalmove = false;

                    // Remove the Entries in XGlobal.
                    XGlobal->lock();
                    delete [] XGlobal->Upgrade_Nick;
                    XGlobal->Upgrade_Nick = NULL;
                    delete [] XGlobal->Upgrade_SHA;
                    XGlobal->Upgrade_SHA = NULL;
                    XGlobal->Upgrade_Time = 0;
                    XGlobal->unlock();
                 } while (false);
                 TransferBPS = TransferTCP->getAvgDownloadBps();
                 sprintf(Response, "Server 09Download: \"%s\" from %s complete @%d Bps", FD->FileName, FD->Nick, TransferBPS);
                 XGlobal->IRC_ToUI.putLine(Response);
                 // Update RecordDownloadBPS and sanitize caps
                 if (TransferTCP->BytesReceived > MIN_BYTES_TRANSFERRED_FOR_RECORD) {
                    H.updateRecordDownloadAndAdjustUploadCaps(TransferBPS, false);
                 }

                 retvalb = H.moveFile(FD->FileName, normalmove);
                 if (retvalb == false) {
                    COUT(cout << "Error in moving downloaded file to serving folder." << endl;)
                    sprintf(Response, "Server 04Download: \"%s\" failed to be moved.", FD->FileName);
                    XGlobal->IRC_ToUI.putLine(Response);
                 }
                 else { 
                    // Succesful move.
                    if (normalmove) {
                       H.generateMyFilesDB();
                    }
                    else {
                       // Tell that the upgrade was a succes.
                       XGlobal->IRC_ToUI.putLine("*UPGRADE* DONE");
                    }
                 }
              }
              else {
                 // In case its Partial, lets update MyPartialFilesDB.
                 H.generateMyPartialFilesDB();
                 if (retval == TRANSFER_DOWNLOAD_SUCCESS) {
                 // Put again in UI only if Partial file was downloaded fully.
                    TransferBPS = TransferTCP->getAvgDownloadBps();
                    sprintf(Response, "Server 09Download: Partial file \"%s\" from %s complete @%d Bps", FD->FileName, FD->Nick, TransferBPS);
                    XGlobal->IRC_ToUI.putLine(Response);
                    // Update RecordDownloadBPS
                    if (TransferTCP->BytesReceived > MIN_BYTES_TRANSFERRED_FOR_RECORD) {
                       H.updateRecordDownloadAndAdjustUploadCaps(TransferBPS, false);
                    }
                 }
              }
          
              // Update its BytesReceived, for sane listing in TAB
              FD->BytesReceived = TransferTCP->BytesReceived;

              // Add it the way we want it.
              XGlobal->DwnldInProgress.addFilesDetail(FD);
           }

        } else {
           // Delete it in case of TRANSFER_DOWNLOAD_FAILURE
           // Cant use RemoteNick as it could have changed.
           XGlobal->DwnldInProgress.delFilesDetailFileNameDottedIP(tmp_str,DottedIP);

           // Delete the file if its size is 0. FileName contains full path..
           size_t file_size;
           bool file_exists = getFileSize(FileName, &file_size);
           if (file_exists && (file_size == 0) ) {
              delFile(FileName);
              // lets update MyPartialFilesDB, to remove this 0 entry.
              H.generateMyPartialFilesDB();
           }
        }
        // Instruct UI to update color, if applicable.
        XGlobal->IRC_ToUI.putLine("*COLOR* Downloads");

        // Lets add it to TotalBytesRcvd
        XGlobal->lock();
        XGlobal->TotalBytesRcvd += TransferTCP->BytesReceived;
        XGlobal->unlock();

        break;

     default:
        COUT(cout << "TransferThr: Unknown Transfer Type. Shouldnt happen." << endl;)
        break;
   }

   delete TransferTCP;
   delete [] RemoteNick;
   delete [] DottedIP;
   delete [] FileName;
   delete DCC_Container;

   delete [] Response;

   COUT(cout << "TransferThr: Quitting" << endl;)
// We dont call Exit thread as we want to go out of scope => destrcutors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}

