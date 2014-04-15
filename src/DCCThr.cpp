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
#include "SHA1.hpp"
#include "Helper.hpp"
#include "Utilities.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

// Thread which gets lines from the DCC Q, and processes it.
void DCCThr(XChange *XGlobal) {
char IRC_Line[1024];
IRCLineInterpret LineInterpret;
TCPConnect *DCCChat, *FServChat, *DCCAccept, *SwarmConnect;
DCC_Container_t *DCC_Container;
THR_EXITCODE thr_exit = 1;
THR_HANDLE DCCChatThrH = 0, FileServerThrH = 0, TransferThrH = 0;
bool retvalb;
int retval;
char MyNick[64];
LineParse LineP;
const char *parseptr;
char *tmpbuf;
CSHA1 SHA;
FilesDetail *FD;
char Response[1024];
Helper H;
TCPConnect T; // Just for a little help.
THREADID tempTID;
size_t my_resume_position;
char DotIp[20]; // cant be more than 3 + 1 + 3 + 1 + 3 + 1 + 3
int SwarmIndex;
bool isSwarmed;

   TRACE_INIT_CRASH("DCCThr");
   TRACE();
   H.init(XGlobal);
   while (true) {
      XGlobal->IRC_DCC.getLineAndDelete(IRC_Line);
      if (XGlobal->isIRC_QUIT()) break;
      if (XGlobal->isIRC_DisConnected()) continue;

      LineInterpret = IRC_Line;
      switch (LineInterpret.Command) {

        case IC_CTCPFILESHA1REPLY:
           // Compare FileName, FileSize, SHA1 and update Server UI.
           // If FileSize = 0 and SHA1 = 0, => error string present in FileName
           XGlobal->lock();
           if ( (XGlobal->SHA1_FileName) &&
                (XGlobal->SHA1_SHA1) &&
                (strcasecmp(LineInterpret.FileName, XGlobal->SHA1_FileName) == 0) &&
                (LineInterpret.FileSize == XGlobal->SHA1_FileSize) ) {
              // Reply matches what we are waiting for.
              // Compare SHAs
              if (strcasecmp(XGlobal->SHA1_SHA1, LineInterpret.InfoLine) == 0) {
                 sprintf(Response, "Server 04[%s FILECHECK REPLY]: \"%s\" can be resumed from me", LineInterpret.From.Nick, LineInterpret.FileName);
              }
              else {
                 sprintf(Response, "Server 04[%s FILECHECK REPLY]: \"%s\" CANNOT be resumed from me", LineInterpret.From.Nick, LineInterpret.FileName);
              }
              // Delete the Global as we are done with this.
              delete [] XGlobal->SHA1_FileName;
              XGlobal->SHA1_FileName = NULL;
              delete [] XGlobal->SHA1_SHA1;
              XGlobal->SHA1_SHA1 = NULL;
           }
           else {
              sprintf(Response, "Server 04[%s FILECHECK REPLY]: %s", LineInterpret.From.Nick, LineInterpret.FileName);
           }
           XGlobal->unlock();
           XGlobal->IRC_ToUI.putLine(Response);
           break;

        case IC_CTCPFILESHA1:
           // Reply back to his request with a IC_CTCPFILESHA1REPLY
           // Same thing that we did in the UI to generate IC_CTCPFILESHA1
           // Here we will search for the file in MyFilesDB. If not found
           // there, we use MyPartialFilesDB.
           FD = XGlobal->MyFilesDB.getFilesDetailListMatchingFileName(LineInterpret.FileName);
           if (FD == NULL) {
              // Lets try MyPartialFilesDB.
              FD = XGlobal->MyPartialFilesDB.getFilesDetailListMatchingFileName(LineInterpret.FileName);
              if (FD == NULL) {
                 sprintf(Response, "Server 04[%s FILECHECK]: File: %s FileSize: %lu - We do not have this file.", LineInterpret.From.Nick, LineInterpret.FileName, LineInterpret.FileSize);
                 XGlobal->IRC_ToUI.putLine(Response);
                 sprintf(Response, "NOTICE %s :\001FILESHA1 0 0 I do not have the file requested: %s\001", LineInterpret.From.Nick, LineInterpret.FileName);
                 XGlobal->IRC_ToServer.putLine(Response);
                 break;
              }
              else {
                 // File in MyPartialFilesDB. Prepare filename in Response
                 XGlobal->lock();
                 sprintf(Response, "%s%s%s", XGlobal->PartialDir, DIR_SEP, LineInterpret.FileName);
                 XGlobal->unlock();
              }
           }
           else {
              // File in MyFilesDB. Prepare filename in Response
              XGlobal->lock();
              if (FD->DirName) {
                 sprintf(Response, "%s%s%s%s%s", XGlobal->ServingDir[FD->ServingDirIndex], DIR_SEP, FD->DirName, DIR_SEP, LineInterpret.FileName);
              }
              else {
                 sprintf(Response, "%s%s%s", XGlobal->ServingDir[FD->ServingDirIndex], DIR_SEP, LineInterpret.FileName);
              }
              XGlobal->unlock();
           }

           XGlobal->MyFilesDB.freeFilesDetailList(FD);
           FD = NULL;

           // we are here => Response has full file path.
           tmpbuf = new char[FILE_RESUME_GAP];
           if (getFileResumeChunk(Response, LineInterpret.FileSize, tmpbuf) == false) {
              sprintf(Response, "Server 04[%s FILECHECK]: File: %s FileSize: %lu - Error Reading Chunk.", LineInterpret.From.Nick, LineInterpret.FileName, LineInterpret.FileSize);
              XGlobal->IRC_ToUI.putLine(Response);
              sprintf(Response, "NOTICE %s :\001FILESHA1 0 0 Error reading chunk: %s\001", LineInterpret.From.Nick, LineInterpret.FileName);
              XGlobal->IRC_ToServer.putLine(Response);
              delete [] tmpbuf;
              break;
           }

           SHA.Reset();
           SHA.Update((UINT_8 *) tmpbuf, (UINT_32) FILE_RESUME_GAP);
           SHA.Final();
           SHA.ReportHash(tmpbuf);

           sprintf(Response, "Server 04[%s FILECHECK]: File: %s FileSize: %lu.", LineInterpret.From.Nick, LineInterpret.FileName, LineInterpret.FileSize);
           XGlobal->IRC_ToUI.putLine(Response);
           sprintf(Response, "NOTICE %s :\001FILESHA1 %lu %s %s\001",
                    LineInterpret.From.Nick,
                    LineInterpret.FileSize,
                    tmpbuf,
                    LineInterpret.FileName);
           XGlobal->IRC_ToServer.putLine(Response);
           delete [] tmpbuf;

           break;

        case IC_USERHOST:
           char windowname[32];
           unsigned long longip, myip;
           char *correct_nick;
           COUT(cout << "IC_USERHOST: IRCLine: " << IRC_Line << endl;)
           COUT(LineInterpret.printDebug();)
//      InfoLine contains: Sur4802=+~khamand@c-67-161-27-122.client.comcast.net
//      batata_wada*=-~email@dsl092-046-112.blt1.dsl.speakeasy.net (ircop)
//         We parse it nice and update XGlobal with dotted ip and long ip
//         if its our nick, and update NickList with long ip.

           LineP = LineInterpret.InfoLine;
           LineP.setDeLimiter('@');
           parseptr = LineP.getWord(2);
           longip = T.getLongFromHostName((char *) parseptr);
           T.getDottedIpAddressFromLong(longip, DotIp);

           // Lets get our nick.
           XGlobal->getIRC_Nick(MyNick);
           XGlobal->resetIRC_Nick_Changed();
           LineP.setDeLimiter('=');
           parseptr = LineP.getWord(1);
           correct_nick = new char[strlen(parseptr) + 1];
           strcpy(correct_nick, parseptr);
           if (correct_nick[strlen(correct_nick) - 1] == '*') {
              // Remove the * at end of ircop nick.
              correct_nick[strlen(correct_nick) - 1] = '\0';
           }
           // correct_nick contains corrected nick.

           if (strcasecmp(correct_nick, MyNick) == 0) {
              // Its my own Nick
              XGlobal->putIRC_IP(longip, DotIp);
              sprintf(Response, "Server 09USERHOST: DottedIP: %s LongIP: %lu", DotIp, longip);
              XGlobal->IRC_ToUI.putLine(Response);
           }

           // Update the XGlobal->NickList
           XGlobal->NickList.setNickIP(correct_nick, longip);

           // For testing.
           if (longip != 0) {
              sprintf(Response, "Server 09USERHOST: Nick: %s DottedIP: %s LongIP: %lu", correct_nick, DotIp, longip);
           } 
           else {
              sprintf(Response, "Server 04USERHOST: Nick: %s DottedIP: %s LongIP: %lu", correct_nick, DotIp, longip);
           }
           XGlobal->IRC_ToUI.putLine(Response);

           // If the USERHOST response is of Nick which is same as in
           // XChange->DCCChatnick, then we need to try to Chat with this nick.
           retval = 1;
           XGlobal->lock();
           if (XGlobal->DCCChatNick) {
              retval = strcasecmp(correct_nick, XGlobal->DCCChatNick);
           }
           XGlobal->unlock();

           if (retval == 0) {
              FilesDetail *FD;

              XGlobal->lock();
              delete [] XGlobal->DCCChatNick;
              XGlobal->DCCChatNick = NULL;
              XGlobal->unlock();

              // Check if a DCCChat is already in progress. (allow only 1)
              if (XGlobal->DCCChatInProgress.getCount(NULL) != 0) {
                 sprintf(Response, "Server * 04DCC-CHAT: A CHAT is already in progress.");
                 XGlobal->IRC_ToUI.putLine(Response);
                 delete [] correct_nick;
                 break;
              }

              if (longip != 0) {
                 // Try to connect to his DCCServer as a CHAT.
                 TCPConnect *DCCChat = new TCPConnect;
                 DCCChat->TCPConnectionMethod = XGlobal->getIRC_CM();
                 XGlobal->resetIRC_CM_Changed();
                 retvalb = DCCChat->getConnection(DotIp, DCCSERVER_PORT);
                 Response[0] = '\0';
                 if (retvalb) {
                    // Connection was successful
                    sprintf(Response, "100 %s\n", MyNick);
                    DCCChat->writeData(Response, strlen(Response), DCCSERVER_TIMEOUT);
                    retval = DCCChat->readLine(Response, sizeof(Response) - 1, DCCSERVER_TIMEOUT);
                    LineP = Response;
                    parseptr = LineP.getWord(1);
                    if (retval && strcasecmp(parseptr, "101") == 0) {
                       // Now also check if the nick matches. Else he has some
                       // other client listening at that port.
                       parseptr = LineP.getWord(2);
                       if (strcasecmp(parseptr, correct_nick) == 0) {
                          // We have established a CHAT.
                          // Now we pass the buck on to the DCC Chat Thr
                          // It will add it in DCCChatInProgress etc.
                          COUT(cout << "DCCChatThr: IC_DCC_CHAT (DCC)" << endl;)
                          // Here we spawn a thread to take care of this DCC Chat.
                          // We pass it the ESTABLISED connection
                          // The created thread should free this structure before exiting.
                          DCC_Container = new DCC_Container_t;
                          DCC_CONTAINER_INIT(DCC_Container);
                          DCC_Container->RemoteNick = new char [strlen(parseptr) + 1];
                          strcpy(DCC_Container->RemoteNick, parseptr);
                          DCC_Container->Connection = DCCChat;
                          DCC_Container->XGlobal = XGlobal;

                          // Have to set DCC_Container->RemoteLongIP.
                          DCC_Container->RemoteDottedIP = new char[strlen(DotIp) + 1];
                          strcpy(DCC_Container->RemoteDottedIP, DotIp);
                          DCC_Container->RemoteLongIP = longip;

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
                          delete [] correct_nick;
                          break;
                       }
                    }
                 }
                 DCCChat->disConnect();
                 delete DCCChat;
              }
              else {
                 // Couldnt get ip of Remote Nick, no point issuing a DCC
                 // CHAT as when it connects, we wont be able to identify it.
                 delete [] correct_nick;
                 break;
              }

              // We are here => couldnt connect to his DCCServer for CHAT.
              // Try the DCC CHAT method.

              // IP should already be set.
              myip = XGlobal->getIRC_IP(NULL);
              XGlobal->resetIRC_IP_Changed();
              if (myip == 0) {
                 delete [] correct_nick;
                 break;
              }

              // We need to add this entry in DCCChatPending
              FD = new FilesDetail;
              XGlobal->DCCChatPending.initFilesDetail(FD);
              FD->Nick = new char[strlen(correct_nick) + 1];
              strcpy(FD->Nick, correct_nick);
              FD->DottedIP = new char[strlen(DotIp) + 1];
              strcpy(FD->DottedIP, DotIp);
              XGlobal->DCCChatPending.addFilesDetail(FD);
              FD = NULL;

              sprintf(IRC_Line, "PRIVMSG %s :\001DCC CHAT chat %lu %d\001", correct_nick, myip, DCCSERVER_PORT);
              H.sendLineToNick(correct_nick, IRC_Line);
              delete [] correct_nick;
              break;
           }
   
           // If the USERHOST response is of Nick which is same as in
           // XChange->PortCheckNick, then we need to connect to it and
           // check its port DCCSERVER_PORT.
           // Update results in window index PortCheckWindowIndex
           retval = 1;
           XGlobal->lock();
           if (XGlobal->PortCheckNick) {
              retval = strcasecmp(correct_nick, XGlobal->PortCheckNick);
              strcpy(windowname, XGlobal->PortCheckWindowName);
           }
           XGlobal->unlock();

           if (retval == 0) {
              XGlobal->lock();
              delete [] XGlobal->PortCheckNick;
              XGlobal->PortCheckNick = NULL;
              XGlobal->unlock();

              if (longip == 0) {
                 // Give Error message in UI.
                 sprintf(Response, "%s * 04PORTCHECK: Could not get valid IP of %s. Possibly mode not set, or cannot resolve. USERHOST response -> %s.", windowname, correct_nick, LineInterpret.InfoLine);
                 XGlobal->IRC_ToUI.putLine(Response);
                 delete [] correct_nick;
                 break;
              }

              T.TCPConnectionMethod = XGlobal->getIRC_CM();
              XGlobal->resetIRC_CM_Changed();
              retvalb = T.getConnection(DotIp, DCCSERVER_PORT);
              Response[0] = '\0';
              if (retvalb) {
                 // Connection was successful => DCCSERVER_PORT open.
                 // Now check if its a DCCServer listening.
                 sprintf(Response, "100 %s\n", MyNick);
                 T.writeData(Response, strlen(Response), DCCSERVER_TIMEOUT);
                 retval = T.readLine(Response, sizeof(Response) - 1, DCCSERVER_TIMEOUT);
                 LineP = Response;
                 parseptr = LineP.getWord(1);
                 if (retval && strcasecmp(parseptr, "101") == 0) {
                    // Now also check if the nick matches. Else he has some
                    // other client listening at that port.
                    parseptr = LineP.getWord(2);
                    if (strcasecmp(parseptr, correct_nick) == 0) {
                       sprintf(Response, "%s * 09PORTCHECK: Port %d of %s is open for incoming traffic => NOT Firewalled. He can receive files from all users.", windowname, DCCSERVER_PORT, correct_nick);
                    }
                    else {
                       sprintf(Response, "%s * 04PORTCHECK: Port %d of %s is open for incoming traffic and DCCServer spotted, but its a different client using the nick %s => NOT Firewalled. He can erratically receive files from all users.", windowname, DCCSERVER_PORT, correct_nick, parseptr);
                    }
                 }
                 else {
                    sprintf(Response, "%s * 04PORTCHECK: Port %d of %s is open for incoming traffic, but DCCServer not spotted. => He cannot receive files from Firewalled users.", windowname, DCCSERVER_PORT, correct_nick);
                 }
              }
              if (Response[0] == '\0') {
                 // Connection unsuccesful => Port closed for incoming.
                 sprintf(Response, "%s * 04PORTCHECK: Port %d of %s is NOT open for incoming traffic => Firewalled. He cannot receive files from Firewalled users.", windowname, DCCSERVER_PORT, correct_nick);
              }
              XGlobal->IRC_ToUI.putLine(Response);
              T.disConnect();
           }
           delete [] correct_nick;

           break;

        case IC_CTCPPORTCHECK:
           T.TCPConnectionMethod = XGlobal->getIRC_CM();
           XGlobal->resetIRC_CM_Changed();

           // Lets get our nick.
           XGlobal->getIRC_Nick(MyNick);
           XGlobal->resetIRC_Nick_Changed();

           COUT(cout << "IC_CTCPPORTCHECK: Attempting connection to Host: " << LineInterpret.From.Host << " Port: " << LineInterpret.Port << endl;)

           retvalb = T.getConnection(LineInterpret.From.Host, LineInterpret.Port);
           Response[0] = '\0';
           if (retvalb) {
              // Connection was successful => DCCSERVER_PORT open.
              // Now check if its a DCCServer listening.
              sprintf(Response, "100 %s\n", MyNick);
              T.writeData(Response, strlen(Response), DCCSERVER_TIMEOUT);
              retval = T.readLine(Response, sizeof(Response) - 1, DCCSERVER_TIMEOUT);
              LineP = Response;
              parseptr = LineP.getWord(1);
              if (retval && strcasecmp(parseptr, "101") == 0) {
                 parseptr = LineP.getWord(2);
                 if (strcasecmp(parseptr, LineInterpret.From.Nick) == 0) {
                    sprintf(Response, "NOTICE %s :09Your port %d is open for incoming traffic => NOT Firewalled. You can receive files from all users.", LineInterpret.From.Nick, LineInterpret.Port);
                 }
                 else {
                    sprintf(Response, "NOTICE %s :04Your Port %d is open for incoming traffic and DCCServer spotted, but its a different client using the nick %s => NOT Firewalled. You will erratically receive files from all users.", LineInterpret.From.Nick, LineInterpret.Port, parseptr);
                 }
              }
              else {
                 sprintf(Response, "NOTICE %s :04Your port %d is open for incoming traffic, but DCCServer not spotted. => You cannot receive files from Firewalled users.", LineInterpret.From.Nick, LineInterpret.Port);
              }
           }
           if (Response[0] == '\0') {
              // Connection unsuccesful => Port closed for incoming.
              sprintf(Response, "NOTICE %s :04Your port %d is NOT open for incoming traffic => Firewalled. You cannot receive files from Firewalled users like you.", LineInterpret.From.Nick, LineInterpret.Port);
           }
           H.sendLineToNick(LineInterpret.From.Nick, Response);
           T.disConnect();
           break;

        case IC_DCC_CHAT:
           COUT(cout << "DCCThr: IC_DCC_CHAT " << IRC_Line << endl;)

           // Lets first check out its IP and see if we need to proceed.
           DCCChat->getDottedIpAddressFromLong(LineInterpret.LongIP, DotIp);
COUT(cout << "DCCThr: IC_DCC_CHAT: RemoteIP: " << LineInterpret.LongIP << " DotIp: " << DotIp << endl;)

           // Here we first check if that DotIp is already in
           // FServClientInProgress. (It should be in FServClientPending for
           // FServ access, and shouldnt be in FServClientPending for Chat
           // If it is we just ignore.
           FD = XGlobal->FServClientInProgress.getFilesDetailListOfDottedIP(DotIp);
           if (FD != NULL) {
              XGlobal->FServClientInProgress.freeFilesDetailList(FD);
              FD = NULL;
              break;
           }
           // Lets check again using Nick this time. (sysreset servers with
           // host masked can escape above.
           FD = XGlobal->FServClientInProgress.getFilesDetailListOfNick(LineInterpret.From.Nick);
           if (FD != NULL) {
              XGlobal->FServClientInProgress.freeFilesDetailList(FD);
              FD = NULL;
              break;
           }

           // Get the DCC_Container fields all set.
           DCC_Container = new DCC_Container_t;
           DCC_CONTAINER_INIT(DCC_Container);
           DCC_Container->XGlobal = XGlobal;
           DCC_Container->RemoteNick = new char[strlen(LineInterpret.From.Nick) + 1];
           strcpy(DCC_Container->RemoteNick, LineInterpret.From.Nick);
           DCC_Container->RemoteLongIP = LineInterpret.LongIP;
           DCC_Container->RemoteDottedIP = new char[strlen(DotIp) + 1];
           strcpy(DCC_Container->RemoteDottedIP, DotIp);

//         Here we spawn a thread to take care of this DCC Chat.
//         We pass it a copy of a TCPConnect structure.
// Allocate DCCChat as the thread created will use this structure.
// The created thread should free this structure before exiting.
           DCCChat = new TCPConnect;
           DCCChat->TCPConnectionMethod = XGlobal->getIRC_CM();
           XGlobal->resetIRC_CM_Changed();
           DCCChat->setLongIP(LineInterpret.LongIP);
           DCCChat->setPort(LineInterpret.Port);

           // Put in the DCCChat in the Container.
           DCC_Container->Connection = DCCChat;

           COUT(cout << "DCCThr: ";)
           COUT(DCCChat->printDebug();)

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
           break;

        case IC_CTCPFSERV:         
           COUT(cout << "DCCThr: IC_CTCPFSERV " << IRC_Line << endl;)
//         Someone has sent us a CTCP for the FServ.
//         Lets first add it to the FileServerWaiting structure.
           unsigned long Ip, RemoteIp;

           // Lets first check if this nick is in CHANNEL_MAIN, if not reject.
           if (XGlobal->NickList.isNickInChannel(CHANNEL_MAIN, LineInterpret.From.Nick) == false) {
              COUT(cout << "Not in CHANNEL_MAIN - FileServer rejected of " << LineInterpret.From.Nick << endl;)
              break;
           }

//         IP should already be set.
           Ip = XGlobal->getIRC_IP(NULL);
           XGlobal->resetIRC_IP_Changed();
           if (Ip == 0) {
              break;
           }

           // Get Remote Nicks' DottedIP
           // Do we already have this guys IP ?
           RemoteIp = XGlobal->NickList.getNickIP(LineInterpret.From.Nick);
           if (RemoteIp == 0) {
              RemoteIp = FServChat->getLongFromHostName(LineInterpret.From.Host);
              if (RemoteIp != 0) {
                 // Lets save the IP in the NickList.
                 XGlobal->NickList.setNickIP(LineInterpret.From.Nick, RemoteIp);
              }
              else {
                 // Use the one as supplied in the CTCP request if present.
                 RemoteIp = LineInterpret.LongIP;
                 if (RemoteIp == 0) {
                    // We have tried everything to get its IP - no luck.
                    sprintf(Response, "NOTICE %s :Please unmask your host by typing: /mode %s -v", LineInterpret.From.Nick, LineInterpret.From.Nick);
                    H.sendLineToNick(LineInterpret.From.Nick, Response);
                    break;
                 }
              }
           }
           FServChat->getDottedIpAddressFromLong(RemoteIp, DotIp);
COUT(cout << "IC_CTCPFSERV: RemoteIP: " << RemoteIp << " DotIp: " << DotIp << endl;)

           // Here we first check if that DotIp is already in
           // FileServerWaiting or FileServerInProgress
           // If it is we just ignore.
           FD = XGlobal->FileServerWaiting.getFilesDetailListOfDottedIP(DotIp);
           if (FD != NULL) {
              XGlobal->FileServerWaiting.freeFilesDetailList(FD);
              break;
           }
           FD = XGlobal->FileServerInProgress.getFilesDetailListOfDottedIP(DotIp);
           if (FD != NULL) {
              XGlobal->FileServerInProgress.freeFilesDetailList(FD);
              break;
           }

           FD = new FilesDetail;
           XGlobal->FileServerWaiting.initFilesDetail(FD);
           FD->DottedIP = new char[strlen(DotIp) + 1];
           strcpy(FD->DottedIP, DotIp);
           FD->Nick = new char[strlen(LineInterpret.From.Nick) + 1];
           strcpy(FD->Nick, LineInterpret.From.Nick);
           FD->TriggerType = FSERVCTCP;
           FServChat = new TCPConnect;
           COUT(cout << "DCCThr:: FServChat: " << FServChat << endl;)
           FD->Connection = FServChat;

           XGlobal->FileServerWaiting.addFilesDetail(FD);
           // Do not use FD as we have sent it to FileServerWaiting.
           // If used further down, it might have been already freed.
           // Setting FD to NULL so that we dont use it by mistake.
           FD = NULL;

//         We first try to connect directly to that nicks DCCSERVER_PORT
//           sprintf(Response, "NOTICE %s :I shall try FServ'ing to DCCServer at port %d first. If that fails, I shall try the DCC way.", LineInterpret.From.Nick, DCCSERVER_PORT);
//         H.sendLineToNick(LineInterpret.From.Nick, Response);
           FServChat->TCPConnectionMethod = XGlobal->getIRC_CM();
           XGlobal->resetIRC_CM_Changed();
           FServChat->setHost(LineInterpret.From.Host);
           FServChat->setPort(DCCSERVER_PORT);
           COUT(cout << "DCCThr: ";)
           COUT(FServChat->printDebug();)
//         Now lets see if we connect.
           retvalb = FServChat->getConnection();
           if (retvalb) {
              retvalb = false;
              // Lets get our nick.
              XGlobal->getIRC_Nick(MyNick);
              XGlobal->resetIRC_Nick_Changed();

              sprintf(IRC_Line, "100 %s\n", MyNick);
              FServChat->writeData(IRC_Line, strlen(IRC_Line), DCCSERVER_TIMEOUT);
              retval = FServChat->readLine(IRC_Line, sizeof(IRC_Line) - 1, DCCSERVER_TIMEOUT);
              LineP = IRC_Line;
              parseptr = LineP.getWord(1);
              if (retval && strcmp("101", parseptr) == 0) {
                 // Mark this nick we connected to as not firewalled.
                 XGlobal->NickList.setNickFirewall(LineInterpret.From.Nick, IRCNICKFW_NO);
                 COUT(cout << "Nick: " << LineInterpret.From.Nick << " marked as IRCNICKFW_NO" << endl;)

                 retvalb = true;
                 DCC_Container = new DCC_Container_t;
                 DCC_CONTAINER_INIT(DCC_Container);
                 DCC_Container->Connection = FServChat;
                 DCC_Container->XGlobal = XGlobal;
                 DCC_Container->RemoteNick = new char[strlen(LineInterpret.From.Nick) + 1];
                 strcpy(DCC_Container->RemoteNick, LineInterpret.From.Nick);
                 DCC_Container->RemoteDottedIP = new char[strlen(DotIp) + 1];
                 strcpy(DCC_Container->RemoteDottedIP, DotIp);
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
              }
              else if ( (retval > 0) && (strcmp("151", parseptr) == 0) ) {
                 // It was rejected. Do NOT try the DCC CHAT
                 retvalb = true;
                // Lets delete the TCPConnect created previously.
                delete FServChat;
              }
              else {
                // TRY the DCC SEND method.
                retvalb = false;
              }
           }
           if (retvalb == false) {
//            If successful with workaround in previous if(), we should
//            set retvalb to true, so that we dont come here.
//            We need to do send a DCC Chat for the client to connect to
//            us and access the File Server.


              // Update the FD in FileServerWaiting, with the NULL Connection
              XGlobal->FileServerWaiting.updateFilesDetailNickFileConnection(LineInterpret.From.Nick, NULL, NULL);
              // Lets delete the TCPConnect created previosuly.
              delete FServChat;

//              sprintf(Response, "NOTICE %s :DCCServer FServ failed. Trying to DCC FServ", LineInterpret.From.Nick);
//            H.sendLineToNick(LineInterpret.From.Nick, Response);

//            Lets issue a DCC Chat to the remote nick with our DCCServer port.
//            We use same port DCCSERVER_PORT for everything.
//            Hence we need to get a nice dotted ip for recognising an
//            incoming connection as a DCC CHAT for an FSERV
              sprintf(IRC_Line, "PRIVMSG %s :\001DCC CHAT chat %lu %d\001", LineInterpret.From.Nick, Ip, DCCSERVER_PORT);
              H.sendLineToNick(LineInterpret.From.Nick, IRC_Line);
           }
           break;

        case IC_DCC_SWARM:
           // Got a DCC SWARM to try to connect to his end.
           COUT(cout << "DCCThr: IC_DCC_SWARM " << IRC_Line << endl;)

           // Lets get the SwarmIndex.
           SwarmIndex = H.getSwarmIndexGivenFileName(LineInterpret.FileName);
           if (SwarmIndex == -1) break;

           // We are here SwarmIndex is set.
           SwarmConnect = new TCPConnect;
           SwarmConnect->TCPConnectionMethod = XGlobal->getIRC_CM();
           XGlobal->resetIRC_CM_Changed();

           // Now lets see if we connect.
           SwarmConnect->setLongIP(LineInterpret.LongIP);
           SwarmConnect->setPort(LineInterpret.Port);

           // No buffer enalrgement for SWARM connections.
           SwarmConnect->setSetOptions(false);

           retvalb = SwarmConnect->getConnection();
           if (retvalb == true) {
              // We successfully connected to this guy.
              // We need to Handshake
              // This routine takes care of Connection.
              retvalb = H.handshakeWriteReadSwarmConnection(SwarmIndex, LineInterpret.From.Nick, SwarmConnect);
              if (retvalb == false) {
                 SwarmConnect->disConnect();
                 delete SwarmConnect;
              }
           }
           else {
              delete SwarmConnect;
           }
           break;

        case IC_DCC_SEND:
           // All of them are forced to a RESUME. We will get an ACCEPT.
           COUT(cout << "DCCThr: IC_DCC_SEND " << IRC_Line << endl;)

           // Here if it exists in our Partial folder, then we make
           // it take the same filename (case insensitive) as it exists.
           // ie, abc.avi and Abc.avi are same file.
           parseptr = LineInterpret.FileName;
           FD =  XGlobal->MyPartialFilesDB.getFilesDetailListMatchingFileName((char *) parseptr);
           if (FD) {
              COUT(cout << "DCCThr: Changing filename from: " << parseptr << " to: " << FD->FileName << endl;)

              parseptr = FD->FileName;
           }

           XGlobal->lock();
           sprintf(IRC_Line, "%s%s%s", XGlobal->PartialDir, DIR_SEP, parseptr);
           XGlobal->unlock();

           if (FD) {
              XGlobal->MyPartialFilesDB.freeFilesDetailList(FD);
              FD = NULL;
           }

           // Do not accept exe files which are sent.
           if (strcasestr(LineInterpret.FileName, ".exe")) {
              // Send a NOTICE informing the nick.
              sprintf(Response, "PRIVMSG %s :I do not receive EXE files: \"%s\" - Rejecting your send.", 
                      LineInterpret.From.Nick, LineInterpret.FileName);
              H.sendLineToNick(LineInterpret.From.Nick, Response);

              // Put this up in the UI too.
              sprintf(Response, "Server 04DOWNLOAD: Rejected attempt from %s to send us file: \"%s\", as its an EXE file.",
              LineInterpret.From.Nick, LineInterpret.FileName);
              XGlobal->IRC_ToUI.putLine(Response);

              break;
           }

           // We accept a send if its not in MyFilesDB => we already have it.
           // and its NOT currently downloading.
           FD = XGlobal->MyFilesDB.getFilesDetailListMatchingFileName(LineInterpret.FileName);
           if (FD) {
              XGlobal->MyFilesDB.freeFilesDetailList(FD);
              // Send a NOTICE informing the nick.
              sprintf(Response, "PRIVMSG %s :I already have the file \"%s\" in my Serving Folder - Rejecting your send.", 
                      LineInterpret.From.Nick, LineInterpret.FileName);
              H.sendLineToNick(LineInterpret.From.Nick, Response);

              // Put this up in the UI too.
              sprintf(Response, "Server 04DOWNLOAD: Rejected attempt from %s to send us file: \"%s\", as we already have it in our Serving Folder.",
              LineInterpret.From.Nick, LineInterpret.FileName);
              XGlobal->IRC_ToUI.putLine(Response);

              break;
           }

           // Check if its already in DwnldInProgress and is Downloading.
           FD = XGlobal->DwnldInProgress.getFilesDetailListMatchingFileName(LineInterpret.FileName);
           if (FD && FD->Connection) {
              XGlobal->DwnldInProgress.freeFilesDetailList(FD);
              sprintf(Response, "PRIVMSG %s :I am currently downloading the file \"%s\" - Rejecting your send.",
                      LineInterpret.From.Nick, LineInterpret.FileName);
              H.sendLineToNick(LineInterpret.From.Nick, Response);

              // Put this up in the UI too.
              sprintf(Response, "Server 04DOWNLOAD: Rejected attempt from %s to send us file: \"%s\", as we are already downloading it.",
              LineInterpret.From.Nick, LineInterpret.FileName);
              XGlobal->IRC_ToUI.putLine(Response);
              break;
           }
           if (FD) {
              XGlobal->DwnldInProgress.freeFilesDetailList(FD);
           }

           // Check if this file is in swarm.
           isSwarmed = false;
           for (int i = 0; i < SWARM_MAX_FILES; i++) {
              if (XGlobal->Swarm[i].isFileBeingSwarmed(parseptr) == false) continue;
              // This file is in swarm. Reject this attempt.
              isSwarmed = true;
              sprintf(Response, "PRIVMSG %s :I am currently swarming the file \"%s\" - Rejecting your send.",
                      LineInterpret.From.Nick, LineInterpret.FileName);
              H.sendLineToNick(LineInterpret.From.Nick, Response);

              // Put this up in the UI too.
              sprintf(Response, "Server 04DOWNLOAD: Rejected attempt from %s to send us file: \"%s\", as we are already swarming it.",
              LineInterpret.From.Nick, LineInterpret.FileName);
              XGlobal->IRC_ToUI.putLine(Response);
              break;
           }
           if (isSwarmed) break;

           // Lets create an FD and dump it in DCCAcceptWaiting
           // First lets check if its already there.
           // DCCAcceptWaiting hold FileNames with full path.
           if (XGlobal->DCCAcceptWaiting.isPresentMatchingNickFileName(LineInterpret.From.Nick, IRC_Line)) {
              // already in the list, so lets just do nothing.
              break;
           }

           // Verify if FileSize is greater than what our resume size is.
           my_resume_position = getResumePosition(IRC_Line);
           if (LineInterpret.FileSize < my_resume_position) {
              // We have more file than the server. Reject.
              sprintf(Response, "PRIVMSG %s :I have more of the file \"%s\" than you do - Rejecting your send.",
                  LineInterpret.From.Nick, LineInterpret.FileName);
              H.sendLineToNick(LineInterpret.From.Nick, Response);

              // Put this up in the UI too.
              sprintf(Response, "Server 04DOWNLOAD: Rejected attempt from %s to send us file: \"%s\", as we have more of the file than him.",
                  LineInterpret.From.Nick, LineInterpret.FileName);
              XGlobal->IRC_ToUI.putLine(Response);

              break;
           }

           FD = new FilesDetail;
           XGlobal->DCCAcceptWaiting.initFilesDetail(FD);
           FD->Nick = new char[strlen(LineInterpret.From.Nick) + 1];
           strcpy(FD->Nick, LineInterpret.From.Nick);
           FD->TriggerType = FSERVCTCP;
           FD->FileName = new char[strlen(IRC_Line) + 1];
           strcpy(FD->FileName, IRC_Line);
           FD->FileSize = LineInterpret.FileSize;
           FD->Port = LineInterpret.Port;
           DCCAccept->getDottedIpAddressFromLong(LineInterpret.LongIP, Response);
           FD->DottedIP = new char[strlen(Response) + 1];
           strcpy(FD->DottedIP, Response);

           XGlobal->DCCAcceptWaiting.addFilesDetail(FD);

           sprintf(Response, "PRIVMSG %s :\001DCC RESUME \"%s\" %d %lu\001",
                      LineInterpret.From.Nick, LineInterpret.FileName,
                      LineInterpret.Port, my_resume_position);
           H.sendLineToNick(LineInterpret.From.Nick, Response);
           break;

        case IC_DCC_RESUME:
           COUT(cout << "DCCThr: IC_DCC_RESUME " << IRC_Line << endl;)
//         This is most probably a DCC RESUME of a DCC SEND we initiated
//         through the TimerThr

//         Lets first validate it.
           FD = XGlobal->DCCSendWaiting.getFilesDetailListOfNick(LineInterpret.From.Nick);
           do {
              if (FD == NULL) break;

//            We have FD which has all the details. Lets do some more checking.
              if ( (LineInterpret.FileName == NULL) || 
                   (FD->FileName == NULL)) {
                 break;
              }

#if 0
              if (strcasecmp(LineInterpret.FileName, FD->FileName) &&
                  strcasecmp(LineInterpret.FileName, "file.ext")
                 ) {
                 // FileName doesnt match for the resume
                 sprintf(IRC_Line, "NOTICE %s :Do not have such a file: \"%s\" waiting to be Resumed!", LineInterpret.From.Nick, LineInterpret.FileName);
                 H.sendLineToNick(LineInterpret.From.Nick, IRC_Line);
                 COUT(cout << "DCC RESUME: LineInterpret.FileName: " << LineInterpret.FileName << " FD->FileName: " << FD->FileName << endl;)
                 break;
              }
#endif

              if (LineInterpret.Port != DCCSERVER_PORT) {
                 // Port number doesnt match.
                 break;
              }
              if ( (LineInterpret.ResumeSize < 0) || 
                   (LineInterpret.ResumeSize > FD->FileSize) ) {
                 sprintf(IRC_Line, "NOTICE %s :Bad Resume size of File.", LineInterpret.From.Nick);
                 H.sendLineToNick(LineInterpret.From.Nick, IRC_Line);
                 break;
              }

              // We are here means all looks good. so lets accept the resume
              // We need to update the DCCSendWaiting with the correct Resume
              XGlobal->DCCSendWaiting.updateFilesDetailNickFileResume(FD->Nick, FD->FileName, LineInterpret.ResumeSize);
              COUT(cout << "DCCthr: DCCSendWaiting ";)
              COUT(XGlobal->DCCSendWaiting.printDebug(NULL);)

              sprintf(IRC_Line, "PRIVMSG %s :\001DCC ACCEPT \"%s\" %d %lu\001",
                      LineInterpret.From.Nick, LineInterpret.FileName,
                      LineInterpret.Port, LineInterpret.ResumeSize);
              H.sendLineToNick(LineInterpret.From.Nick, IRC_Line);
           } while (false);

           // Lets free up the FD we have gotten earlier.
           XGlobal->DCCSendWaiting.freeFilesDetailList(FD);
           break;

        case IC_DCC_ACCEPT:
//         This is most probably a DCC ACCEPT of a DCC RESUME we initiated
//         above.
           COUT(cout << "DCCThr: IC_DCC_ACCEPT " << IRC_Line << endl;)

           // Lets grab its FD from DwnldWaiting
           FD = XGlobal->DCCAcceptWaiting.getFilesDetailListOfNick(LineInterpret.From.Nick);
           if (FD == NULL) break; // Dont know about this one.

           // lets delete it from DCCAcceptWaiting
           XGlobal->DCCAcceptWaiting.delFilesOfNick(LineInterpret.From.Nick);

           // Lets try to connect first.
           DCCAccept = new TCPConnect;
           DCCAccept->TCPConnectionMethod = XGlobal->getIRC_CM();
           XGlobal->resetIRC_CM_Changed();
           DCCAccept->setHost(FD->DottedIP);
           DCCAccept->setPort(FD->Port); // how about LineInterprets Port ?
           COUT(cout << "DCCThr: ";)
           COUT(DCCAccept->printDebug();)
//         Now lets see if we connect.
           retvalb = DCCAccept->getConnection();
           if (retvalb) {
              DCC_Container = new DCC_Container_t;
              DCC_CONTAINER_INIT(DCC_Container);
              DCC_Container->Connection = DCCAccept;
              DCC_Container->XGlobal = XGlobal;
              DCC_Container->RemoteNick = new char[strlen(FD->Nick) + 1];
              strcpy(DCC_Container->RemoteNick, FD->Nick);
              DCC_Container->TransferType = INCOMING;
              DCC_Container->FileSize = FD->FileSize;
              DCC_Container->FileName = new char[strlen(FD->FileName) + 1];
              strcpy(DCC_Container->FileName, FD->FileName);
              DCC_Container->ResumePosition = getResumePosition(DCC_Container->FileName);
              DCC_Container->RemoteDottedIP = new char[strlen(FD->DottedIP) + 1];
              strcpy(DCC_Container->RemoteDottedIP, FD->DottedIP);
              XGlobal->DCCAcceptWaiting.freeFilesDetailList(FD);
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
           }
           else {
               // Free up FD, DCCAccept, DCC_Container
               XGlobal->DCCAcceptWaiting.freeFilesDetailList(FD);
               delete DCCAccept; 
           }
           break;

        default:
           break;
      }
   }

   COUT(cout << "DCCThr Quitting." << endl;)
// We dont call Exit thread as we want to go out of scope => destrcutors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}

