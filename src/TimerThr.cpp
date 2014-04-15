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

// Thread which is the Timer
// Does the below:
// 1. Advertisement generation
// 2. Generates the list of files we are serving every hour.
// 3. Goes thru the QueuesInProgress every second to initiate sends
//    when appropriate.
// 4. PING/PONG with server every 90 seconds = PINGPONG_TIMEOUT
// 5. Partial/Waiting/FServ Information in config file every 10 minutes.
//    = PARTIAL_WAITING_FSERV_SAVE_TIME_INTERVAL
// 6. Process IRC_ToUpgrade Queue, once every PROCESS_UPGRADE_TIME seconds.
//    = every 3 minutes.
void TimerThr(XChange *XGlobal) {
THR_EXITCODE thr_exit = 1;
FilesDetail *FD, *ScanFD;
time_t NextAdTime, LastFileGenTime = 0;
#if 0
time_t LastRecordUpdateTime = 0;
#endif
time_t LastUpnpTime = 0, LastPartialWaitingSaveTime;
time_t LastProcessUpgradeTime = 0;
time_t CurrentTime;
time_t ChannelJoinTime;
time_t NextPingPongCheckTime;
time_t LastTimeWhenItWasAboveMinCAP;
char Response[1024];
int MyMaxSends;
int TotalSends;
int TotalGets;
int TotalQueues;
float DownloadSpeed;
float UploadSpeed;
char Nick[64];
char ServerName[128];
int retval;
bool SmallQ;
THR_HANDLE TransferThrH = 0;
IRCChannelList CL;
Helper H;

   TRACE_INIT_CRASH("TimerThr");
   TRACE();
   Nick[0] = '\0';

   H.init(XGlobal);

// Lets wait till a working nick is available.
   while (XGlobal->isIRC_Nick_Changed(Nick) == false) {
      sleep(1);
      if (XGlobal->isIRC_QUIT()) break;
   }

   LastFileGenTime = time(NULL); // FromUIThr, has generated file list.
   LastUpnpTime = LastFileGenTime;
   LastPartialWaitingSaveTime = LastUpnpTime + PARTIAL_WAITING_FSERV_SAVE_TIME_INTERVAL;
   LastTimeWhenItWasAboveMinCAP = LastFileGenTime;

   while (true) {
      if (XGlobal->isIRC_QUIT()) break;
      CurrentTime = time(NULL);

      XGlobal->lock();
      NextAdTime = XGlobal->FServAdTime;
      NextPingPongCheckTime = XGlobal->PingPongTime + PINGPONG_TIMEOUT;
      XGlobal->unlock();

      // Start of Section ->
      // - Need not be connected to Server, nor joined in CHANNEL

      // Do we need to save the Partial/Waiting/FServ Info in Config file ?
      if (CurrentTime > LastPartialWaitingSaveTime) {
         LastPartialWaitingSaveTime = CurrentTime + PARTIAL_WAITING_FSERV_SAVE_TIME_INTERVAL;

         H.writePartialConfig();
         H.writeWaitingConfig();
         H.writeFServeConfig();
      }

      // Do we need to generate the list of files we are serving ?
      if ( (CurrentTime - LastFileGenTime) > 3600) {
         COUT(cout << "TimerThr: Calling generate MyFilesDB" << endl;)
         LastFileGenTime = CurrentTime;
         H.generateMyFilesDB();
         H.generateMyPartialFilesDB();
         COUT(cout << "TimerThr: Returning from generate MyFilesDB" << endl;)
      }

      // Do we need to issue a SEARCH_IF_NOT_OK to the UpnpThr ?
      // Once every 2 minutes.
      if ( (CurrentTime - LastUpnpTime) > (2 * 60) ) {
         LastUpnpTime = CurrentTime;
         XGlobal->UI_ToUpnp.putLine("Server SEARCH_IF_NOT_OK");
      }

      // - Need not be connected to Server, nor joined in CHANNEL
      // <- END of Section


      // Start of Section ->
      // - Need to be connected to Server.

      do {
         // First meet the pre-req, else break out.
         if (XGlobal->isIRC_DisConnected()) break;

         // Do we need to send out a PING ?
         if (CurrentTime < NextPingPongCheckTime) break;

         if (XGlobal->isIRC_Server_Changed(ServerName)) {
            XGlobal->getIRC_Server(ServerName);
            XGlobal->resetIRC_Server_Changed();
         }

         // Server Name should be set when we come here.
         sprintf(Response, "PING :%s", ServerName);
         XGlobal->IRC_ToServerNow.putLine(Response);

         XGlobal->lock();
         XGlobal->PingPongTime = CurrentTime;
         XGlobal->unlock();

         COUT(cout << "TimerThr -> " << Response << endl;)
      } while (false);

      // - Need to be connected to Server.
      // <- END of Section


      // Start of Section ->
      // - Need to be connected to Server and joined in CHANNEL_MAIN

      do {
         // First meet the pre-req, else break out.
         if (XGlobal->isIRC_DisConnected()) {
            // Drain XGlobal->IRC_ToUpgrade if not connected.
            if (XGlobal->IRC_ToUpgrade.isEmpty() == false) {
               XGlobal->IRC_ToUpgrade.getLineAndDelete(Response);
            }
            break;
         }

         CL = XGlobal->getIRC_CL();
         XGlobal->resetIRC_CL_Changed();
         if (CL.isJoined(CHANNEL_MAIN) == false) {
            // Drain XGlobal->IRC_ToUpgrade if not connected.
            if (XGlobal->IRC_ToUpgrade.isEmpty() == false) {
               XGlobal->IRC_ToUpgrade.getLineAndDelete(Response);
            }
            break;
         }

         ChannelJoinTime = CL.getJoinedTime(CHANNEL_MAIN);

         // Do we need to send out a trigger ?
         if ( (NextAdTime != 0) && (CurrentTime > NextAdTime) ) {
            XGlobal->lock();
            XGlobal->FServAdTime = CurrentTime + FSERV_INITIAL_AD_TIME;
            XGlobal->unlock();

            char *buffer = new char[1024];
            H.generateFServAd(buffer);
            sprintf(Response, "PRIVMSG %s :%s", CHANNEL_MAIN, buffer);
            XGlobal->IRC_ToServer.putLine(Response);

            // Lets get our Nick
            XGlobal->getIRC_Nick(Nick);
            XGlobal->resetIRC_Nick_Changed();

            char color_coded_nick[512];
            H.generateColorCodedNick(CHANNEL_MAIN, Nick, color_coded_nick);
            sprintf(Response, "%s %s %s", CHANNEL_MAIN, color_coded_nick, buffer);
            XGlobal->IRC_ToUI.putLine(Response);
            delete [] buffer;
         }
         // End of Trigger generation.

         // Do we need to process anything from IRC_ToUpgrade Queue.
         // Once every PROCESS_UPGRADE_TIME seconds.
         // This should be done if we are connected to server and joined in
         // channel main.
         if ( (CurrentTime - LastProcessUpgradeTime) > PROCESS_UPGRADE_TIME) {
            LastProcessUpgradeTime = CurrentTime;
            COUT(cout << "TimerThr: Calling processUpgrade" << endl;)
            H.processUpgrade();
         }
         // End of Process Upgrade.

         // We check all the sends, and see if its imbalanced.
         // Imbalanced, means, all sends are occupied, and big sends
         // are more than allowed ceil(TotalSends/2) and a small Q has an entry
         // or small sends are more than allowed floor(TotalSends/2) and
         // a big Q has an entry. In such a case, we get the suitable Send FD to
         // be stopped.
         int q_indicator = H.stopImbalancedSends();

         // Lets set SmallQ depending on q_indicator
         SmallQ = H.convertQueueIndicatorToSmallQueueBoolean(q_indicator);

         // We check and send from QueuesInProgress
         H.checkAndNormalSendFromQueue(SmallQ, ChannelJoinTime);

         // Monitor to see if we are consistently below OverallMinUploadBPS
         // that implies we need to push a send out.
         H.checkForOverallMinCPSAndSendFromQueue(SmallQ, ChannelJoinTime);

         // Check to see if any of the Queues, has a Manual Send or a 
         // File Push. It needs to be sent.
         H.checkAndManualSendFromQueue(ChannelJoinTime);

      } while (false);
      // - Need to be connected to Server and joined in CHANNEL_MAIN
      // <- End of Section

      sleep(5); // Sleep five seconds
   }

   COUT(cout << "TimerThr: Quitting" << endl;)
// We dont call Exit thread as we want to go out of scope => destrcutors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}
