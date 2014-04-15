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

// Temp debug for Propagation algo.
#define DEBUG

#include "DCCChatClient.hpp"
#include "Helper.hpp"
#include "FilesDetailList.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

static void freeDCCChatThrContainer(DCC_Container_t *DCC_Container) {
   delete DCC_Container->Connection;
   delete [] DCC_Container->RemoteNick;
   delete [] DCC_Container->FileName;
   delete [] DCC_Container->TriggerName;
   delete [] DCC_Container->RemoteDottedIP;
   delete DCC_Container;
}

// Thread which does DCC Chat.
// The TCPConnect structure has all values set, so a connection can be
// initiated. If its from a workaround then the state is already established,
// and hence no need of attempting an outgoing connect.
void DCCChatThr(DCC_Container_t *DCC_Container) {
bool retvalb = true;;
TCPConnect *DCCChat = DCC_Container->Connection;
XChange *XGlobal = DCC_Container->XGlobal;
char *DottedIP = DCC_Container->RemoteDottedIP;
char *RemoteNick = DCC_Container->RemoteNick;
DCCChatClient DC;
THR_EXITCODE thr_exit = 1;
FilesDetail *SendFD, *FServFD, *CopyFServFD;
char MyNick[64];
bool chat_success = false;
bool mydccserver = false;
Helper H;

   TRACE_INIT_NOCRASH();
   TRACE();

   H.init(XGlobal);

   if (DCCChat->state() != TCP_ESTABLISHED) {
   //  If its already established its a DCCServer Chat connect.
      retvalb = DCCChat->getConnection();
   }
   else {
      mydccserver = true;
   }

   COUT(cout << "Entered DCCChatThr: FServPending: ";)
   COUT(DCCChat->printDebug();)
   COUT(XGlobal->FServClientPending.printDebug(NULL);)

   // Lets get the FD.
   FServFD = XGlobal->FServClientPending.getFilesDetailListOfDottedIP(DottedIP);

   // Note that for a sysreset server who has his host masked, we cant know
   // his ip to be put in FServClientPending, when we issue trigger.
   // Hence if we get NULL above, we should fall back to using the RemoteNick
   if (FServFD == NULL) {
      FServFD = XGlobal->FServClientPending.getFilesDetailListOfNick(RemoteNick);
   }

   // Hope FServFD contains only 1 item.
   // If more, the first one is what we go by.

   // If a successful connection we should move it from FServClientPending
   // onto FServClientInProgress, with Connection piggy backed.
   if (mydccserver || retvalb) {
      if (FServFD) {
         CopyFServFD = XGlobal->FServClientPending.copyFilesDetail(FServFD);
         CopyFServFD->Connection = DCCChat;
         XGlobal->FServClientPending.delFilesOfNick(CopyFServFD->Nick);
         XGlobal->FServClientInProgress.addFilesDetail(CopyFServFD);
         CopyFServFD = NULL;
      }
   }

   if ( (FServFD == NULL) && (retvalb) ) {
      COUT(cout << "DCCChatThr: Connection Success - Pure Time Pass Chatting connection" << endl;)
      // Check if DCCChatInProgress already has a Chat.
      if (XGlobal->DCCChatInProgress.getCount(NULL) >= 1) {
         char message[512];
         // Cant accept this new Chat. Just Quit.
         // Send a NOTICE informing the nick.
         sprintf(message, "NOTICE %s : A current Chat is in progress. Rejecting your Chat.", RemoteNick);
         H.sendLineToNick(RemoteNick, message);

         // Also put it up in the UI.
         sprintf(message, "Server CHAT: Rejecting Chat of %s, as we already have a Chat in progress.", RemoteNick);
         XGlobal->IRC_ToUI.putLine(message);
      }
      else {
         // Add an FD for this CHAT into DCCChatInProgress
         FilesDetail *FD = new FilesDetail;
         XGlobal->DCCChatInProgress.initFilesDetail(FD);
         FD->Nick = new char[strlen(RemoteNick) + 1];
         strcpy(FD->Nick, RemoteNick);
         FD->DottedIP = new char[strlen(DottedIP) + 1];
         strcpy(FD->DottedIP, DottedIP);
         FD->Connection = DCCChat;

         XGlobal->DCCChatInProgress.addFilesDetail(FD);
         FD = NULL;

         DC.justChat(DCC_Container); // ignore retval bool for now.

         // Remove the entry as we are done chatting.
         // Make sure we can acquire the lock and delete the entry.
         // so that TabBookWindow is mutually excluded.
         XGlobal->lockDCCChatConnection();
         XGlobal->DCCChatInProgress.delFilesDetailDottedIP(DottedIP);
         XGlobal->unlockDCCChatConnection();
      }
   }
   else if (FServFD && retvalb) {
      COUT(cout << "DCCChatThr: Connection success. - for DIR or GET or unQ" << endl;)

      // Update the XGlobal->NickList
      XGlobal->NickList.setNickIP(DCC_Container->RemoteNick, DCC_Container->RemoteLongIP);

      if (mydccserver) {
         // Mark as not firewalled.
         XGlobal->getIRC_Nick(MyNick);
         XGlobal->resetIRC_Nick_Changed();
         markNotFireWalled(XGlobal, MyNick);
      }

      // Lets call the correct function, for DIR/GET/unQ
      if (FServFD->FileName) {
         // This is either a GET or an unQ request.
         if (FServFD->FileSize == 0) {
            // This is an unQ request.
            DCC_Container->FileName = new char[strlen(FServFD->FileName) + 1];
            strcpy(DCC_Container->FileName, FServFD->FileName);
            chat_success = DC.removeFromQ(DCC_Container);
         }
         else {
            // This is a GET request.
            if (FServFD->DirName) {
               DCC_Container->FileName = new char[strlen(FServFD->DirName) + strlen(FServFD->FileName) + 2];
               sprintf(DCC_Container->FileName, "%s"FS_DIR_SEP"%s", FServFD->DirName, FServFD->FileName);
            }
            else {
               DCC_Container->FileName = new char[strlen(FServFD->FileName) + 1];
               strcpy(DCC_Container->FileName, FServFD->FileName);
            }
            chat_success = DC.getFile(DCC_Container);
         }
      }
      else {
         // We are going to issue a DIR.
         SendFD = XGlobal->FilesDB.getFilesDetailListNickFile(FServFD->Nick, "TriggerTemplate");
         if (SendFD) {
            // Lets fill up the Container with send/q information
            // so that the DCCChatClient class can do reduced work.
            DCC_Container->CurrentSends = SendFD->CurrentSends;
            DCC_Container->TotalSends = SendFD->TotalSends;
            DCC_Container->CurrentQueues = SendFD->CurrentQueues;
            DCC_Container->TotalQueues = SendFD->TotalQueues;
            DCC_Container->ClientType = SendFD->ClientType;
         }
         // TriggerName should not be NULL.
         DCC_Container->TriggerName = new char[strlen(FServFD->TriggerName) + 1];
         strcpy(DCC_Container->TriggerName, FServFD->TriggerName);

         // Hope FServFD contains only 1 item.
         // If more, the first one is what we go by.
         COUT(XGlobal->FServClientInProgress.printDebug(FServFD);)

         // Purge all entries of this Nick in FilesDB except "TriggerTemplate"
         // ToTriggerThr/ToTriggerNowThr have already purged FilesDB of Nick

         // This call will add the "No Files Present" FilesDB, if no
         // files are being served.
         chat_success = DC.getDirListing(DCC_Container);

         XGlobal->FilesDB.freeFilesDetailList(SendFD);
         SendFD = NULL; // To discourage reuse.

         // If FServFD is marked with PropagatingNick. We need to propagate.
         // Issue ctcp propagate to some guys, under certain conditions.
         // If we are not NF then do nothing
         do {
            if (FServFD->PropagatedNick == NULL) break;

            // Propagate only if we got some FileListing of the Propagated
            // Nick.
            if (XGlobal->FilesDB.isFilesOfNickPresent(FServFD->PropagatedNick) == false) break;

            // Before we start the propagation, make sure that the UpdateCount
            // of the Propagated Nick is 0, else we transfer the propagation
            // with a non zero update count and can form an endless loop, when
            // operating with older clients.
            XGlobal->FilesDB.updateUpdateCountOfNick(FServFD->PropagatedNick, 0);

            XGlobal->getIRC_Nick(MyNick);
            XGlobal->resetIRC_Nick_Changed();
            int MyNFIndex = XGlobal->NickList.getNickNFMMindex(CHANNEL_MAIN, MyNick);
            if (MyNFIndex == 0) break;

            int NFCount = XGlobal->NickList.getNFMMcount(CHANNEL_MAIN);
            int SecondMyNFIndex = NFCount - MyNFIndex + 1;

            // Send Propagation CTCP to 2 of my NF children (if any)
            // and to 2 of my Secondary tree Children (if any)
            // and to 2 of my FW children (if any)
            char *Response = new char[512];
            char *MyChild = new char[64];
            int MySends, MyQueues, MyMaxSends, MyMaxQueues;
            MySends = XGlobal->SendsInProgress.getCount(NULL);
            MyQueues = XGlobal->QueuesInProgress.getCount(NULL) +
                       XGlobal->SmallQueuesInProgress.getCount(NULL);
            XGlobal->lock();
            MyMaxSends = XGlobal->FServSendsOverall;
            MyMaxQueues = XGlobal->FServQueuesOverall * 2;
            XGlobal->unlock();

            // Print debug the NF and FW tree.
            {
            int i;
            COUT(
               i = 1;
               cout << "PROPAGATION TREE NF:";
               while (XGlobal->NickList.getNickInChannelAtIndexNFMM(CHANNEL_MAIN, i, MyChild)) {
                  cout << " [" << i << "]:" << MyChild;
                  i++;
               }
               cout << endl;
            )
            COUT(
               i = 1;
               cout << "PROPAGATION TREE FW:";
               while (XGlobal->NickList.getNickInChannelAtIndexFWMM(CHANNEL_MAIN, i, MyChild)) {
                  cout << " [" << i << "]:" << MyChild;
                  i++;
               }
               cout << endl;
            )
            }

            if (XGlobal->NickList.getNickInChannelAtIndexNFMM(CHANNEL_MAIN, 2 * MyNFIndex, MyChild)) {
               // First NF child. send propagation ctcp
               if (strcasecmp(MyChild, FServFD->PropagatedNick)) {
                  sprintf(Response, "PRIVMSG %s :\001PROPAGATION %s %d %d %d %d\001",
                     MyChild, FServFD->PropagatedNick,
                     MySends, MyMaxSends, MyQueues, MyMaxQueues);
                  XGlobal->IRC_ToServer.putLine(Response);
#if 0
                  sprintf(Response, "PRIVMSG %s :PROPAGATION FirstNFChild: %s for %s with %d %d %d %d", CHANNEL_MAIN, MyChild, FServFD->PropagatedNick, MySends, MAX_SENDS, MyQueues, MAX_QUEUES);
                  XGlobal->IRC_ToServer.putLine(Response);
#endif
                  COUT(cout << "DCCChatThr: PROPAGATION: NF Child1: " << Response << endl;)
               }

               if (XGlobal->NickList.getNickInChannelAtIndexNFMM(CHANNEL_MAIN, 2 * MyNFIndex + 1, MyChild)) {
                  if (strcasecmp(MyChild, FServFD->PropagatedNick)) {
                     // Second NF child. send propagation ctcp
                     sprintf(Response, "PRIVMSG %s :\001PROPAGATION %s %d %d %d %d\001",
                        MyChild, FServFD->PropagatedNick,
                        MySends, MyMaxSends, MyQueues, MyMaxQueues);
                     XGlobal->IRC_ToServer.putLine(Response);
#if 0
                     sprintf(Response, "PRIVMSG %s :PROPAGATION SecondNFChild: %s for %s with %d %d %d %d", CHANNEL_MAIN, MyChild, FServFD->PropagatedNick, MySends, MAX_SENDS, MyQueues, MAX_QUEUES);
                     XGlobal->IRC_ToServer.putLine(Response);
#endif
                     COUT(cout << "DCCChatThr: PROPAGATION: NF Child2: " << Response << endl;)
                  }
               }
            }

            if (XGlobal->NickList.getNickInChannelAtIndexNFMM(CHANNEL_MAIN, NFCount - (SecondMyNFIndex * 2) + 1, MyChild)) {
               if (strcasecmp(MyChild, FServFD->PropagatedNick)) {
                  // First Secondary Tree NF child. send propagation ctcp
                  sprintf(Response, "PRIVMSG %s :\001PROPAGATION %s %d %d %d %d\001",
                     MyChild, FServFD->PropagatedNick,
                     MySends, MyMaxSends, MyQueues, MyMaxQueues);
                  XGlobal->IRC_ToServer.putLine(Response);
#if 0
                  sprintf(Response, "PRIVMSG %s :PROPAGATION Secondary Tree FirstNFChild: %s for %s with %d %d %d %d", CHANNEL_MAIN, MyChild, FServFD->PropagatedNick, MySends, MAX_SENDS, MyQueues, MAX_QUEUES);
                  XGlobal->IRC_ToServer.putLine(Response);
#endif
                  COUT(cout << "DCCChatThr: PROPAGATION: Secondary Tree NF Child1: " << Response << endl;)
               }
               if (XGlobal->NickList.getNickInChannelAtIndexNFMM(CHANNEL_MAIN, NFCount - (SecondMyNFIndex * 2 + 1) + 1, MyChild)) {
                  if (strcasecmp(MyChild, FServFD->PropagatedNick)) {
                     // Second Secondary Tree NF child. send propagation ctcp
                     sprintf(Response, "PRIVMSG %s :\001PROPAGATION %s %d %d %d %d\001",
                        MyChild, FServFD->PropagatedNick,
                        MySends, MyMaxSends, MyQueues, MyMaxQueues);
                     XGlobal->IRC_ToServer.putLine(Response);
#if 0
                     sprintf(Response, "PRIVMSG %s :PROPAGATION Secondary Tree SecondNFChild: %s for %s with %d %d %d %d", CHANNEL_MAIN, MyChild, FServFD->PropagatedNick, MySends, MAX_SENDS, MyQueues, MAX_QUEUES);
                     XGlobal->IRC_ToServer.putLine(Response);
#endif
                     COUT(cout << "DCCChatThr: PROPAGATION: Secondary Tree NF Child2: " << Response << endl;)
                  }
               }
            }

            if (XGlobal->NickList.getNickInChannelAtIndexFWMM(CHANNEL_MAIN, 2 * MyNFIndex - 1, MyChild)) {
               if (strcasecmp(MyChild, FServFD->PropagatedNick)) {
                  // First FW child. send propagation ctcp
                  sprintf(Response, "PRIVMSG %s :\001PROPAGATION %s %d %d %d %d\001",
                     MyChild, FServFD->PropagatedNick,
                     MySends, MyMaxSends, MyQueues, MyMaxQueues);
                  XGlobal->IRC_ToServer.putLine(Response);
#if 0
                  sprintf(Response, "PRIVMSG %s :PROPAGATION FirstFWChild: %s for %s with %d %d %d %d", CHANNEL_MAIN, MyChild, FServFD->PropagatedNick, MySends, MAX_SENDS, MyQueues, MAX_QUEUES);
                  XGlobal->IRC_ToServer.putLine(Response);
#endif
                  COUT(cout << "DCCChatThr: PROPAGATION: FW Child1: " << Response << endl;)
               }
               if (XGlobal->NickList.getNickInChannelAtIndexFWMM(CHANNEL_MAIN, 2 * MyNFIndex, MyChild)) {
                  if (strcasecmp(MyChild, FServFD->PropagatedNick)) {
                     // Second FW child. send propagation ctcp
                     sprintf(Response, "PRIVMSG %s :\001PROPAGATION %s %d %d %d %d\001",
                        MyChild, FServFD->PropagatedNick,
                        MySends, MyMaxSends, MyQueues, MyMaxQueues);
                     XGlobal->IRC_ToServer.putLine(Response);
#if 0
                     sprintf(Response, "PRIVMSG %s :PROPAGATION SecondFWChild: %s for %s with %d %d %d %d", CHANNEL_MAIN, MyChild, FServFD->PropagatedNick, MySends, MAX_SENDS, MyQueues, MAX_QUEUES);
                     XGlobal->IRC_ToServer.putLine(Response);
#endif
                     COUT(cout << "DCCChatThr: PROPAGATION: NF Child2: " << Response << endl;)
                  }
               }
            }

            delete [] MyChild;
            delete [] Response;

         } while (false);

      }
   }
   else if (FServFD && (retvalb == false) ) {
      // This is the case where we are not able to connect to his FileServer.
      // Check if we got anything for this nick, if not add an entry
      // which is dummy, so we dont repeatedly access his trigger.
      FilesDetail *tempFD = XGlobal->FServClientPending.getFilesDetailListOfDottedIP(DottedIP);
      if ( (tempFD == NULL) || (XGlobal->FilesDB.isFilesOfNickPresent(tempFD->Nick) == false) ) {
     FilesDetail *FD;

         FD = new FilesDetail;
         XGlobal->FilesDB.initFilesDetail(FD);
         FD->FileName = new char[20];
         strcpy(FD->FileName, "Inaccessible Server");
         FD->FileSize = 1;
         FD->Nick = new char[strlen(DCC_Container->RemoteNick) + 1];
         strcpy(FD->Nick, DCC_Container->RemoteNick);
         FD->TriggerType = FSERVCTCP;
         FD->TriggerName = new char[strlen(FServFD->TriggerName) + 1];
         // Above can core dump if TriggerName is NULL. It shouldnt be
         strcpy(FD->TriggerName, FServFD->TriggerName);

         // Lets add to the FilesList DB
         XGlobal->FilesDB.addFilesDetail(FD);
      }
      XGlobal->FServClientPending.freeFilesDetailList(tempFD);

      COUT(cout << "DCCChatThr: Connection Failed." << endl;)
   }
   else {
      // This is the case when FServFD is NULL and retvalb is false
      // That is the connection didnt make it. Dont care case.
      // This is when a firewalled user, issues a DCC Chat on our nick.
   }

   // Lets get the possibly changed nick again.
   FilesDetail *tempFD = XGlobal->FServClientInProgress.getFilesDetailListOfDottedIP(DottedIP);
   if (tempFD == NULL) {
      tempFD = XGlobal->FServClientPending.getFilesDetailListOfDottedIP(DottedIP);
   }
 
   if (chat_success && (mydccserver == false) ) {
      // Mark this nick we connected to as not firewalled.
      if (tempFD) {
         XGlobal->NickList.setNickFirewall(tempFD->Nick, IRCNICKFW_NO);
         COUT(cout << "Nick: " << tempFD->Nick << " marked as IRCNICKFW_NO" << endl;)
      }
      else {
         XGlobal->NickList.setNickFirewall(DCC_Container->RemoteNick, IRCNICKFW_NO);
         COUT(cout << "Nick: " << DCC_Container->RemoteNick << " marked as IRCNICKFW_NO" << endl;)
      }
   }

   // Do not use FServFD entries here as we can come here even if
   // FServFD is NULL.
   if (tempFD) {
      XGlobal->FServClientPending.delFilesOfNick(tempFD->Nick);
      XGlobal->FServClientInProgress.delFilesOfNick(tempFD->Nick);
   }
   else {
      XGlobal->FServClientPending.delFilesOfNick(DCC_Container->RemoteNick);
      XGlobal->FServClientInProgress.delFilesOfNick(DCC_Container->RemoteNick);
   }
   
   COUT(cout << "Exiting DCCChatThr: FServClientPending: ";)
   COUT(XGlobal->FServClientPending.printDebug(NULL);)
   COUT(cout << "Exiting DCCChatThr: FServClientInProgress: ";)
   COUT(XGlobal->FServClientInProgress.printDebug(NULL);)

   XGlobal->FServClientInProgress.freeFilesDetailList(FServFD);
   XGlobal->FServClientInProgress.freeFilesDetailList(tempFD);

   freeDCCChatThrContainer(DCC_Container);

//   XGlobal->FilesDB.printDebug(NULL);
// We dont call Exit thread as we want to go out of scope => destructors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}

