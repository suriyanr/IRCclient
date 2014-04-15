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
#include "Utilities.hpp"
#include "FilesDetailList.hpp"
#include "StackTrace.hpp"
#include "Helper.hpp"

#include "Compatibility.hpp"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DCCCHAT_TIMEOUT 180


// Thread which is the DCCServer Listener.
// The DCCServer listener doubles as other incoming connections as well.
// Like a) leecher types trigger, we issue a DCC CHAT at incoming 8124.
// Like b) leecher issues a get, we issue a DCC SEND at incoming 8124.
void DCCServerThr(XChange *XGlobal) {
THR_EXITCODE thr_exit = 1;
TCPConnect DCCServer, *DCCServerEst;
bool retvalb;
ssize_t retval;
LineParse LP;
const char *parseptr; // Return values from LineParse.
char Buffer[256];
char Response[512];
char Nick[64];
unsigned long Ip;
char DotIp[20];
FilesDetail *FD;
DCC_Container_t *DCC_Container;
THR_HANDLE DCCChatThrH = 0;
THR_HANDLE FileServerThrH = 0;
THR_HANDLE TransferThrH = 0;
Helper H;
char *tmpstr;
int SwarmIndex;
THREADID tempTID;

   TRACE_INIT_CRASH("DCCServerThr");
   TRACE();

   H.init(XGlobal);

   Nick[0] = '\0';

   // Set up the listener.
   while (DCCServer.serverInit(DCCSERVER_PORT) == false) {
      if (XGlobal->isIRC_QUIT()) break; // If we need to quit
   // Failed to listen as server. Keep trying every 5 minutes.
      sprintf(Response, "Server 04DCCServer: Failed to listen at port %d ... Will try again in 5 minutes.", DCCSERVER_PORT);
      COUT(cout << Response << endl;);
      XGlobal->IRC_ToUI.putLine(Response);
      long r = 5 * 60;
      long rr = 0;
      while (rr < r) {
         sleep(1);
         if (XGlobal->isIRC_QUIT()) break; // If we need to quit
         rr++;
      }
   }

   if (XGlobal->isIRC_QUIT() == false) {
      sprintf(Response, "Server 09DCCServer: listening at port %d", DCCSERVER_PORT);
      XGlobal->IRC_ToUI.putLine(Response);

      // Lets wait till a working nick is available.
      while (XGlobal->isIRC_Nick_Changed(Nick) == false) {
         sleep(1);
         if (XGlobal->isIRC_QUIT()) break;
      }
   }

   while (true) {
      // Timeout is 1 second so that we can quit when Quitting UI.
      retvalb = DCCServer.getConnection(1);
      if (XGlobal->isIRC_QUIT()) break;

      if (retvalb) {
         // We got a Connection.
         DCCServerEst = new TCPConnect;
         *DCCServerEst = DCCServer;

         // Lets get our nick.
         XGlobal->getIRC_Nick(Nick);
         XGlobal->resetIRC_Nick_Changed();

         COUT(cout << "DCCServerThr: Got an incoming connect: My Nick: " << Nick << endl;)
         // Lets first check if its a DCC CHAT for FServ
         Ip = DCCServerEst->getRemoteIP();
         DCCServerEst->getDottedIpAddressFromLong(Ip, DotIp);
         // The only way to check if its valid is by the remote guy's ip.
         COUT(cout << "DCCServer: IP: " << Ip << " DotIP: " << DotIp << endl;)
         FD = XGlobal->FileServerWaiting.getFilesDetailListOfDottedIP(DotIp);
         if (FD != NULL) {
            // Mark as not firewalled.
            markNotFireWalled(XGlobal, Nick);

            // This is an allowed connect and we have the Nick's details
            // in FD.

            // Lets launch the FileServerThr to do the rest of the chatting.
            COUT(DCCServerEst->printDebug();)
            DCC_Container = new DCC_Container_t;
            DCC_CONTAINER_INIT(DCC_Container);
            DCC_Container->Connection = DCCServerEst;
            DCC_Container->XGlobal = XGlobal;
            DCC_Container->RemoteNick = new char[strlen(FD->Nick) + 1];
            strcpy(DCC_Container->RemoteNick, FD->Nick);
            DCC_Container->RemoteDottedIP = new char[strlen(DotIp) + 1];
            strcpy(DCC_Container->RemoteDottedIP, DotIp);
            DCC_Container->RemoteLongIP = Ip;

            // Lets update FileServerWaiting with the correct Connect.
            XGlobal->FileServerWaiting.updateFilesDetailNickFileConnection(FD->Nick, NULL, DCCServerEst);
#ifdef __MINGW32__
            FileServerThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) FileServerThr, DCC_Container, 0, &tempTID);
#else
            {
               pthread_attr_t thread_attr;
               pthread_attr_init(&thread_attr);
               pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
               pthread_create(&FileServerThrH, &thread_attr, (void * (*)(void *)) FileServerThr, DCC_Container);
            }
#endif

            // Free up the FD.
            XGlobal->FileServerWaiting.freeFilesDetailList(FD);

            continue;
         }

         // Lets now check if we are DCC SENDing a file.
         FD = XGlobal->DCCSendWaiting.getFilesDetailListOfDottedIP(DotIp);
         if (FD != NULL) {
            // Mark as not firewalled.
            markNotFireWalled(XGlobal, Nick);

            // This is an allowed connect and we have the Nick's details
            // in FD.

            // Lets update DCCSendWaiting with the correct Connection.
            XGlobal->DCCSendWaiting.updateFilesDetailNickFileConnection(FD->Nick, FD->FileName, DCCServerEst);

            COUT(cout << "DCCServerThr: DCCSendWaiting: ";)
            COUT(XGlobal->DCCSendWaiting.printDebug(NULL);)
            // Lets launch the TransferThr to do the actual send.
            COUT(DCCServerEst->printDebug();)
            DCC_Container = new DCC_Container_t;
            DCC_CONTAINER_INIT(DCC_Container);
            DCC_Container->Connection = DCCServerEst;
            DCC_Container->XGlobal = XGlobal;
            DCC_Container->RemoteNick = new char[strlen(FD->Nick) + 1];
            strcpy(DCC_Container->RemoteNick, FD->Nick);
            DCC_Container->RemoteDottedIP = new char[strlen(DotIp) + 1];
            strcpy(DCC_Container->RemoteDottedIP, DotIp);
            DCC_Container->RemoteLongIP = Ip;
            DCC_Container->TransferType = OUTGOING;
            DCC_Container->ResumePosition = FD->FileResumePosition;
            DCC_Container->FileSize = FD->FileSize;

            // correct File Name Generation - same as in Helper::dccSend
            DCC_Container->FileName = H.getFilenameWithPathFromFD(FD);
            if (DCC_Container->FileName) {

#ifdef __MINGW32__
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

            // Free up the FD.
            XGlobal->DCCSendWaiting.freeFilesDetailList(FD);

            continue;
         }

         // Lets now check if we were waiting for a CHAT with this ip.
         FD = XGlobal->DCCChatPending.getFilesDetailListOfDottedIP(DotIp);
         if (FD != NULL) {
            // Mark as not firewalled.
            markNotFireWalled(XGlobal, Nick);

            // This is an allowed connect and we have the Nick's details
            // in FD.

            // Lets remove this entry from DCCChatPending
            XGlobal->DCCChatPending.delFilesDetailDottedIP(DotIp);

            // Check if do not have an already ongoing Chat InProgress.
            if (XGlobal->DCCChatInProgress.getCount(NULL) != 0) {
               // Free up the FD.
               XGlobal->DCCChatPending.freeFilesDetailList(FD);
               // DisConnect it.
               DCCServerEst->disConnect();
               delete DCCServerEst;
               continue;
            }

            // Now we pass the buck on to the DCC Chat Thr
            // It will add it in DCCChatInProgress etc.
            COUT(cout << "DCCServerThr: IC_DCC_CHAT (DCC)" << endl;)
//          Here we spawn a thread to take care of this DCC Chat.
//          We pass it the ESTABLISED connection
//          The created thread should free this structure before exiting.
            DCC_Container = new DCC_Container_t;
            DCC_CONTAINER_INIT(DCC_Container);
            DCC_Container->RemoteNick = new char [strlen(FD->Nick) + 1];
            strcpy(DCC_Container->RemoteNick, FD->Nick);
            DCC_Container->Connection = DCCServerEst;
            DCC_Container->XGlobal = XGlobal;

//          Have to set DCC_Container->RemoteLongIP.
            DCC_Container->RemoteDottedIP = new char[strlen(DotIp) + 1];
            strcpy(DCC_Container->RemoteDottedIP, DotIp);
            DCC_Container->RemoteLongIP = Ip;
            
#ifdef __MINGW32__
            DCCChatThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) DCCChatThr, DCC_Container, 0, &tempTID);
#else
            {
               pthread_attr_t thread_attr;
               pthread_attr_init(&thread_attr);
               pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
               pthread_create(&DCCChatThrH, &thread_attr, (void * (*)(void *)) DCCChatThr, DCC_Container);
            }
#endif

            // Free up the FD.
            XGlobal->DCCChatPending.freeFilesDetailList(FD);
            continue;
         }

         // Lets now check if we were waiting for a SWARM connect with this ip.
         FD = XGlobal->SwarmWaiting.getFilesDetailListOfDottedIP(DotIp);
         if (FD != NULL) {
            // Mark as not firewalled.
            markNotFireWalled(XGlobal, Nick);

            // Get SwarmIndex using FD->FileName.
            SwarmIndex = H.getSwarmIndexGivenFileName(FD->FileName);

            // We need to delete this entry now.
            XGlobal->SwarmWaiting.delFilesDetailDottedIP(FD->DottedIP);

            if (SwarmIndex == -1) {
               // Send "AC NO\n"
               DCCServerEst->writeData("AC NO\n", 6, DCCSERVER_TIMEOUT);

               retvalb = false;
            }
            else {
               retvalb = H.handshakeWriteReadSwarmConnection(SwarmIndex, FD->Nick, DCCServerEst);
            }

            if (retvalb == false) {
               // Need to disco and delete DCCServerEst.
               DCCServerEst->disConnect();
               delete DCCServerEst;
               DCCServerEst = NULL;
            }

            // Free up the FD.
            XGlobal->SwarmWaiting.freeFilesDetailList(FD);
            continue;
         }

         // Now its all actual DCCServer stuff below.
         retval = DCCServerEst->readLine(Buffer, sizeof(Buffer) - 1, DCCSERVER_TIMEOUT);
         COUT(cout << "DCCServerThr: Received line length: " << retval << " :" << Buffer << endl;)
         LP = Buffer;
         parseptr = LP.getWord(1);
         if (retval && strcmp("100", parseptr) == 0) {
            // Mark as not firewalled.
            // This is done in DCCChatThr() as its more decisive there
            // using mydccserver variable.

            parseptr = LP.getWord(2);
            
            // DCCServer CHAT protocol. Accept it.
            snprintf(Response, sizeof(Buffer) - 1, "101 %s\n", Nick);
            DCCServerEst->writeData(Response, strlen(Response), DCCSERVER_TIMEOUT);
            // Now we pass the buck on to the DCC Chat Thr
            COUT(cout << "DCCServerThr: IC_DCC_CHAT" << endl;)
//          Here we spawn a thread to take care of this DCC Chat.
//          We pass it the ESTABLISED connection
//          The created thread should free this structure before exiting.
            DCC_Container = new DCC_Container_t;
            DCC_CONTAINER_INIT(DCC_Container);
            DCC_Container->RemoteNick = new char [strlen(parseptr) + 1];
            strcpy(DCC_Container->RemoteNick, parseptr);
            DCC_Container->Connection = DCCServerEst;
            DCC_Container->XGlobal = XGlobal;

//          Have to set DCC_Container->RemoteLongIP.
            DCC_Container->RemoteDottedIP = new char[strlen(DotIp) + 1];
            strcpy(DCC_Container->RemoteDottedIP, DotIp);
            DCC_Container->RemoteLongIP = Ip;
            
#ifdef __MINGW32__
            DCCChatThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) DCCChatThr, DCC_Container, 0, &tempTID);
#else
            {
               pthread_attr_t thread_attr;
               pthread_attr_init(&thread_attr);
               pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
               pthread_create(&DCCChatThrH, &thread_attr, (void * (*)(void *)) DCCChatThr, DCC_Container);
            }
#endif
         }
         else if (retval && strcmp("120", parseptr) == 0) {
         size_t server_file_size;
         size_t my_resume_position;
         char *full_file_name = NULL;
            // Mark as not firewalled.
            markNotFireWalled(XGlobal, Nick);

            // Someone is sending us a file.
            parseptr = LP.getWord(2); // Remote Nick
            tmpstr = new char[strlen(parseptr) + 1];
            strcpy(tmpstr, parseptr);
            parseptr = LP.getWordRange(4, 0);  // File Name

            do {

               // Reject attempts on receiving an exe file.
               if (strcasestr((char *) parseptr, ".exe")) {
                  // Send a NOTICE informing the nick.
                  sprintf(Response, "PRIVMSG %s :I do not receive EXE files: \"%s\" - Rejecting your send.", tmpstr, parseptr);
                  H.sendLineToNick(tmpstr, Response);

                  // Put this up in the UI too.
                  sprintf(Response, "Server 04DOWNLOAD: Rejected attempt from %s to send us file: \"%s\", as its an EXE file",
                          tmpstr, parseptr);
                  XGlobal->IRC_ToUI.putLine(Response);

                  delete [] tmpstr;

                  sprintf(Response, "151 %s\n", Nick);
                  DCCServerEst->writeData(Response, strlen(Response), DCCSERVER_TIMEOUT);
                  DCCServerEst->disConnect();
                  delete DCCServerEst;

                  break;
               }

               // We accept a send if its not in MyFilesDB => we already have it.
               // and its NOT currently downloading.
               FD = XGlobal->MyFilesDB.getFilesDetailListMatchingFileName( (char *) parseptr);
               if (FD) {
                  XGlobal->MyFilesDB.freeFilesDetailList(FD);
                  // Send a NOTICE informing the nick.
                  sprintf(Response, "PRIVMSG %s :I already have the file \"%s\" in my Serving Folder - Rejecting your send.", tmpstr, parseptr);
                  H.sendLineToNick(tmpstr, Response);

                  // Put this up in the UI too.
                  sprintf(Response, "Server 04DOWNLOAD: Rejected attempt from %s to send us file: \"%s\", as we already have it in our Serving Folder.",
                          tmpstr, parseptr);
                  XGlobal->IRC_ToUI.putLine(Response);

                  delete [] tmpstr;

                  sprintf(Response, "151 %s\n", Nick);
                  DCCServerEst->writeData(Response, strlen(Response), DCCSERVER_TIMEOUT);
                  DCCServerEst->disConnect();
                  delete DCCServerEst;

                  break;
               }

               // Check if its already in DwnldInProgress and is Downloading.
               FD = XGlobal->DwnldInProgress.getFilesDetailListMatchingFileName((char *) parseptr);
               if (FD && FD->Connection) {
                  XGlobal->DwnldInProgress.freeFilesDetailList(FD);
                  sprintf(Response, "PRIVMSG %s :I am currently downloading the file \"%s\" - Rejecting your send.", tmpstr, parseptr);
                  H.sendLineToNick(tmpstr, Response);

                  // Put this up in the UI too.
                  sprintf(Response, "Server 04DOWNLOAD: Rejected attempt from %s to send us file: \"%s\", as we are already downloading it.",
                  tmpstr, parseptr);
                  XGlobal->IRC_ToUI.putLine(Response);

                  sprintf(Response, "151 %s\n", Nick);
                  DCCServerEst->writeData(Response, strlen(Response), DCCSERVER_TIMEOUT);
                  DCCServerEst->disConnect();
                  delete DCCServerEst;

                  delete [] tmpstr;
                  break;
               }
               if (FD) {
                  XGlobal->DwnldInProgress.freeFilesDetailList(FD);
                  FD = NULL;
               }

               // Check if the file is in swarm.
               bool isSwarmed = false;
               for (int i = 0; i < SWARM_MAX_FILES; i++) {
                  if (XGlobal->Swarm[i].isFileBeingSwarmed(parseptr) == false) continue;
                  // This file is in swarm. Reject this attempt.
                  isSwarmed = true;
                  sprintf(Response, "PRIVMSG %s :I am currently swarming the file \"%s\" - Rejecting your send.", tmpstr, parseptr);
                  H.sendLineToNick(tmpstr, Response);

                  // Put this up in the UI too.
                  sprintf(Response, "Server 04DOWNLOAD: Rejected attempt from %s to send us file: \"%s\", as we are already swarming it.",
                  tmpstr, parseptr);
                  XGlobal->IRC_ToUI.putLine(Response);

                  sprintf(Response, "151 %s\n", Nick);
                  DCCServerEst->writeData(Response, strlen(Response), DCCSERVER_TIMEOUT);
                  DCCServerEst->disConnect();
                  delete DCCServerEst;

                  delete [] tmpstr;
                  break;
               }
               if (isSwarmed) break;

               // Verify if FileSize is greater than what our resume size is.
               // 3rd word is file size of file name in server.
               parseptr = LP.getWord(3);
               server_file_size = strtoul(parseptr, NULL, 10);

               // Get the Full pathed File Name.
               parseptr = LP.getWordRange(4, 0);

               // Here if it exists in our Partial folder, then we make
               // it take the same filename (case insensitive) as it exists.
               // ie, abc.avi and Abc.avi are same file.
               FD = XGlobal->MyPartialFilesDB.getFilesDetailListMatchingFileName((char *) parseptr);
               if (FD) {
                  COUT(cout << "DCCServerThr: Changing filename from: " << parseptr << " to: " << FD->FileName << endl;)

                  parseptr = FD->FileName;
               }
               XGlobal->lock();
               full_file_name = new char[strlen(parseptr) + strlen(DIR_SEP) + strlen(XGlobal->PartialDir) + 1];
               sprintf(full_file_name, "%s%s%s", XGlobal->PartialDir, DIR_SEP, parseptr);
               XGlobal->unlock();
               my_resume_position = getResumePosition(full_file_name);

               if (server_file_size < my_resume_position) {
                  // Note parseptr, has the filename
                  // We have more file than the server. Reject.
                  sprintf(Response, "PRIVMSG %s :I have more of the file \"%s\" than you do - Rejecting your send.", tmpstr, parseptr);
                  H.sendLineToNick(tmpstr, Response);

                  // Put this up in the UI too.
                  sprintf(Response, "Server 04DOWNLOAD: Rejected attempt from %s to send us file: \"%s\", as we have more of the file than him.",
                  tmpstr, parseptr);
                  XGlobal->IRC_ToUI.putLine(Response);

                  sprintf(Response, "151 %s\n", Nick);
                  DCCServerEst->writeData(Response, strlen(Response), DCCSERVER_TIMEOUT);
                  DCCServerEst->disConnect();
                  delete DCCServerEst;

                  delete [] full_file_name;
                  delete [] tmpstr;
                  if (FD) {
                     XGlobal->MyPartialFilesDB.freeFilesDetailList(FD);
                     FD = NULL;
                  }
                  break;
               }
               if (FD) {
                  XGlobal->MyPartialFilesDB.freeFilesDetailList(FD);
                  FD = NULL;
               }

               // All checks done, OK to receive.

               DCC_Container = new DCC_Container_t;
               DCC_CONTAINER_INIT(DCC_Container);
               DCC_Container->Connection = DCCServerEst;
               DCC_Container->XGlobal = XGlobal;

               parseptr = LP.getWord(2);
               DCC_Container->RemoteNick = tmpstr;
               DCC_Container->RemoteDottedIP = new char[strlen(DotIp) + 1];
               strcpy(DCC_Container->RemoteDottedIP, DotIp);
               DCC_Container->RemoteLongIP = Ip;
               DCC_Container->TransferType = INCOMING;

               DCC_Container->FileSize = server_file_size;

               DCC_Container->FileName = full_file_name;
               full_file_name = NULL;
               DCC_Container->ResumePosition = my_resume_position;
               // Lets tell them where we want it resumed from.
               sprintf(Response, "121 %s %lu\n", Nick, DCC_Container->ResumePosition);
               DCCServerEst->writeData(Response, strlen(Response), DCCSERVER_TIMEOUT);
               // Rest of the process is same for DCC or DCCServer, and
               // TransferThr will handle it.
#ifdef __MINGW32__
               TransferThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) TransferThr, DCC_Container, 0, &tempTID);
#else
               {
                  pthread_attr_t thread_attr;
                  pthread_attr_init(&thread_attr);
                  pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
                  pthread_create(&TransferThrH, &thread_attr, (void * (*)(void *)) TransferThr, DCC_Container);
               }
#endif

            } while (false);
         }
         else if (retval && strcmp("140", parseptr) == 0) {
            // This is a swarm Connection.
            // Mark as not firewalled.
            markNotFireWalled(XGlobal, Nick);

            // Get FileName to get SwarmIndex.
            parseptr = LP.getWordRange(3, 0);
            SwarmIndex = H.getSwarmIndexGivenFileName(parseptr);

            if (SwarmIndex == -1) {
               // Send "AC NO\n"
               DCCServerEst->writeData("AC NO\n", 6, DCCSERVER_TIMEOUT);
               retvalb = false;
            }
            else {
               // Get Nick.
               parseptr = LP.getWord(2);
               retvalb = H.handshakeWriteReadSwarmConnection(SwarmIndex, (char *) parseptr, DCCServerEst);
            }

            if (retvalb == false) {
               // Need to disco and delete DCCServerEst.
               DCCServerEst->disConnect();
               delete DCCServerEst;
               DCCServerEst = NULL;
            }
         }
         else {
            sprintf(Response, "151 %s\n", Nick);
            DCCServerEst->writeData(Response, strlen(Response), DCCSERVER_TIMEOUT);
            DCCServerEst->disConnect();
            delete DCCServerEst;
         }

      }
   }

   COUT(cout << "DCCServerThr: Quitting" << endl;)
// We dont call Exit thread as we want to go out of scope => destrcutors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}

// Mark as not firewalled.
void markNotFireWalled(XChange *XGlobal, char *OurNick) {
// -1 => Maybe firewalled.
// 0 => firewalled, 5 = not firewalled surely.
// This 5 step shift is currently in sync with the 
// TabBookWindow::updateToNotFirewalled(). It decreases red by 51 and increases
// red by 51 on each call. so it gets fully green on the 5th call.
// We manipulate the variable XGlobal->FireWallState, sync with FireWalled.
int FireWalled;

   TRACE();

   XGlobal->lock();
   if (XGlobal->FireWallState < 5) {
      XGlobal->FireWallState++;
   }
   FireWalled = XGlobal->FireWallState;
   XGlobal->unlock();
   if (FireWalled < 5) {
      COUT(cout << "DCCServerThr: FireWall count: " << FireWalled << endl;)
      XGlobal->IRC_ToUI.putLine("*NOTFIREWALLED*");
      // For practical cases we are not firewalled, hence mark ourselves
      // as not firewalled in NickList => this helps in FFLC
      XGlobal->NickList.setNickFirewall(OurNick, IRCNICKFW_NO);
      COUT(cout << "Firewall: marking myself: " << OurNick << " as IRCNICKFW_NO " << endl;)
   }
}
