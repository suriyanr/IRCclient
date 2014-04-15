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

#include "Compatibility.hpp"

#include "ThreadMain.hpp"
#include "Helper.hpp"
#include "StackTrace.hpp"

#include <stdio.h>
#include <string.h>

// Thread which handles the Swarm.
// For starters currently we will make it try to get new Connections
// looking into YetToTryNodes.
// This Thread does Three JOBS.
// 1 - Try to make new Connections for the swarm not exceeding the limit.
//   - Try this once every second.
// 2 - Read incoming message lines from each of the connections and update
//   - the Connection state.
// 3 - Send messages out depending on the connection state.
// 4 - Send messages out for what we now want - DR requests, if we dont
//     have more than some limit pending.
void SwarmThr(XChange *XGlobal) {
THR_EXITCODE thr_exit = 1;
Helper H;
bool retvalb;
int retval;
char Message[512];
char MyNick[64];
int i, SwarmIndex;
bool do_i_sleep;
time_t LastCAPSyncTime = 0, CurrentTime;
time_t LastNewConnectionTime = 0;
time_t Last5SecondTimeChore = 0;

   TRACE_INIT_NOCRASH();
   TRACE();

   H.init(XGlobal);
   while (true) {
      if (XGlobal->isIRC_QUIT()) break;

      do {
         CurrentTime = time(NULL);

         // Once in 5 seconds we update the Record/Bytes Sent/Received.
         if (CurrentTime > (Last5SecondTimeChore + 5)) {
            Last5SecondTimeChore = CurrentTime;
            for (i = 0; i < SWARM_MAX_FILES; i++) {
               size_t b_s = XGlobal->Swarm[i].ConnectedNodes.getBytesSent();
               size_t b_r = XGlobal->Swarm[i].ConnectedNodes.getBytesReceived();
               size_t r_d = 0;
               if (XGlobal->Swarm[i].ConnectedNodes.isDownloadComplete()) {
                  r_d = XGlobal->Swarm[i].ConnectedNodes.getDownloadCompletedSpeed();
               }
               XGlobal->lock();
               XGlobal->TotalBytesSent += b_s;
               XGlobal->TotalBytesRcvd += b_r;
               if (XGlobal->RecordDownloadBPS < r_d) {
                  XGlobal->RecordDownloadBPS = r_d;
               }
               XGlobal->unlock();
            }
         }

         // Once in 2 seconds we do this new connection attempts.
         bool try_new_connections;
         if (CurrentTime > LastNewConnectionTime + 2) {
            try_new_connections = true;
            LastNewConnectionTime = CurrentTime;
         }
         else {
            try_new_connections = false;
         }

         // Lets see if we need to try new connections.
         for (i = 0; i < SWARM_MAX_FILES; i++) {

            if (try_new_connections == false) break;

            SwarmIndex = i;

            // Check if we have reached SWARM_MAX_CONNECTED_NODES for
            // this swarm. We do not try if we have reached this.
            if (XGlobal->Swarm[i].ConnectedNodes.getCount() >= SWARM_MAX_CONNECTED_NODES) continue;

            SwarmNode *SN = XGlobal->Swarm[i].YetToTryNodes.getFirstSwarmNode();
            if (SN == NULL) continue;

            // Sanitize IP if possible.
            if (SN->NodeIP == IRCNICKIP_UNKNOWN) {
               SN->NodeIP = XGlobal->NickList.getNickIP(SN->Nick);
            }

            // If IP is unknown and State is SWARM_NODE_USERHOST_ISSUED
            // we dont try this Node at all and dump it in TriedAndFailedNodes
            // Mark its state as such
            if ( (SN->NodeIP == IRCNICKIP_UNKNOWN) &&
                 (SN->NodeState == SWARM_NODE_USERHOST_ISSUED) ) {
               SN->NodeState = SWARM_NODE_IP_UNKNOWN;
               XGlobal->Swarm[i].TriedAndFailedNodes.addToSwarmNode(SN);
               SN = NULL;
               continue;
            }

            if (SN->NodeIP == IRCNICKIP_UNKNOWN) {
               // Issue a USERHOST so we get IP
               sprintf(Message, "USERHOST %s", SN->Nick);
               XGlobal->IRC_ToServerNow.putLine(Message);

               // Mark the NodeState as such.
               SN->NodeState = SWARM_NODE_USERHOST_ISSUED;

               // Put the SN back in YetToTryNodes
               XGlobal->Swarm[i].YetToTryNodes.addToSwarmNode(SN);
               SN = NULL;

               // As we have issued USERHOST, lets delay the next time
               // we will enter the new connection route by another 20 seconds
               LastNewConnectionTime = CurrentTime + 20;
               continue;
            }

            // Lets try to get a connection to this SwarmNode.
            COUT(cout << "SwarmThr: Trying to connect directly to Nick: " << SN->Nick << " IP: " << SN->NodeIP << endl;)
            TCPConnect *Connection = new TCPConnect;
            Connection->TCPConnectionMethod = XGlobal->getIRC_CM();
            XGlobal->resetIRC_CM_Changed();
            // Now lets see if we connect.
            Connection->setLongIP(SN->NodeIP);
            Connection->setPort(DCCSERVER_PORT);

            // No buffer enlargement for SWARM connections.
            Connection->setSetOptions(false);

            retvalb = Connection->getConnection();
            if (retvalb) {
               // Send DCCServer SWARM protocol -> 140 Nick FileName
               XGlobal->getIRC_Nick(MyNick);
               XGlobal->resetIRC_Nick_Changed();

               sprintf(Message, "140 %s %s\n", MyNick, XGlobal->Swarm[SwarmIndex].getSwarmFileName());
               retval = Connection->writeData(Message, strlen(Message), SWARM_CONNECTION_TIMEOUT);
               if (retval != strlen(Message)) {
                  Connection->disConnect();
                  retvalb = false;
               }
            }

            if (retvalb) {
               // We successfully connected to this guy.

               // We need to Handshake
               retvalb = H.handshakeWriteReadSwarmConnection(SwarmIndex, SN->Nick, Connection);
            }

            if (retvalb == false) {
               // Handshake failed, lets assume failure was cause it was
               // not due to a mismatched SHA. So try to send a DCC SWARM.
               
               FilesDetail *FD = new FilesDetail;

               // SwarmWaiting needs Nick, DottedIP and FileName.
               XGlobal->SwarmWaiting.initFilesDetail(FD);
               FD->Nick = new char[strlen(SN->Nick) + 1];
               strcpy(FD->Nick, SN->Nick);
               char DotIP[64];
               Connection->getDottedIpAddressFromLong(SN->NodeIP, DotIP);
               FD->DottedIP = new char[strlen(DotIP) + 1];
               strcpy(FD->DottedIP, DotIP);

               FD->FileName = new char[strlen(XGlobal->Swarm[SwarmIndex].getSwarmFileName()) + 1];
               strcpy(FD->FileName, XGlobal->Swarm[SwarmIndex].getSwarmFileName());

               // Add it to SwarmWaiting.
               XGlobal->SwarmWaiting.addFilesDetail(FD);
               FD = NULL;

               delete Connection;

               // We need to send out a DCC SWARM so he tries from his side.
               // We just have this message as close to the DCC SEND message.
               // so its easy to parse.
               // So we add this in FD in SwarmWaiting, so we know when
               // the guy connects.
               unsigned long MyIp = XGlobal->getIRC_IP(NULL);
               XGlobal->resetIRC_IP_Changed();

               sprintf(Message, "PRIVMSG %s :\001DCC SWARM \"%s\" %lu %d %lu\001", 
                                 SN->Nick, 
                                 XGlobal->Swarm[i].getSwarmFileName(),
                                 MyIp,
                                 DCCSERVER_PORT,
                                 XGlobal->Swarm[i].getSwarmFileSize());
               XGlobal->IRC_ToServer.putLine(Message);
            }

            // Delete SN.
            XGlobal->Swarm[i].YetToTryNodes.freeSwarmNodeList(SN);

            break; // break out of the for loop.
         }

         do_i_sleep = true;
         // Second JOB - Read Messages from the Connections and update state.
         for (i = 0; i < SWARM_MAX_FILES; i++) {
            SwarmIndex = i;
            retvalb = XGlobal->Swarm[i].readMessagesAndUpdateState();
            if (retvalb) do_i_sleep = false;

            // Here we need to check if this Swarm has met with a critical
            // error which means we destroy this swarm.
            int ErrorCode = XGlobal->Swarm[i].getErrorCode();
            if (ErrorCode != SWARM_NO_ERROR) {
               // This is where we update the UI with the error message,
               // so user knows what happened.
               char ErrorString[512];
               XGlobal->Swarm[i].getErrorCodeString(ErrorString);

               // Let the UI know.
               sprintf(Message, "Server 04,01Swarm: %s", ErrorString);
               XGlobal->IRC_ToUI.putLine(Message);

               // Need to quit this swarm.
               XGlobal->Swarm[i].quitSwarm();
            }
         }

         // Third JOB - Write Messages to the Connections and update state.
         for (i = 0; i < SWARM_MAX_FILES; i++) {
            SwarmIndex = i;
            retvalb = XGlobal->Swarm[i].writeMessagesAndUpdateState();
            if (retvalb) do_i_sleep = false;
         }

         // Fourth JOB - Check for Dead Nodes.
         for (i = 0; i < SWARM_MAX_FILES; i++) {
            SwarmIndex = i;
            XGlobal->Swarm[i].checkForDeadNodes();
         }

         // Fifth JOB - Check for EndGame to get the last piece.
         for (i = 0; i < SWARM_MAX_FILES; i++) {
            SwarmIndex = i;
            retvalb = XGlobal->Swarm[i].checkForEndGame();
            if (retvalb) do_i_sleep = false;
         }

      } while (false);

      // Sync CAP values from Global.
      // Actually syncs once in 5 seconds.
      CurrentTime = time(NULL);
      if (CurrentTime > (LastCAPSyncTime + 5) ) {
         LastCAPSyncTime = CurrentTime;
         for (i = 0; i < SWARM_MAX_FILES; i++) {
            SwarmIndex = i;
            XGlobal->lock();
            size_t PerTransferMaxUploadBPS = XGlobal->PerTransferMaxUploadBPS;
            size_t OverallMaxUploadBPS = XGlobal->OverallMaxUploadBPS;
            size_t PerTransferMaxDownloadBPS = XGlobal->PerTransferMaxDownloadBPS;
            size_t OverallMaxDownloadBPS = XGlobal->OverallMaxDownloadBPS;
            XGlobal->unlock();

            retvalb = XGlobal->Swarm[i].updateCAPFromGlobal(PerTransferMaxUploadBPS, OverallMaxUploadBPS, PerTransferMaxDownloadBPS, OverallMaxDownloadBPS);
         }
      }

      msleep(25);
#if 0
      if (do_i_sleep == true) {
         //COUT(cout << "SwarmThr: do_i_sleep is true - sleeping 100 milli secs." << endl;)
         msleep(100);
      }
#endif
   }

   COUT(cout << "SwarmThr: Quitting" << endl;)
// We dont call Exit thread as we want to go out of scope => destuctors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}
