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

// to debug the Propagation algo - says stuff in main channel.
// #define DEBUG


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

// Thread which is fed privmsg FServ lines got in channel.
// It tries to figure out if it is an FServ trigger, and extracts the
// the trigger information and issues the trigger.
// The first word in the line is the Nick. 2nd word on is the possible
// trigger line. All triggers are issued after a random delay.
// Note that the XDCC line is updated in the fromServerThr itself.
void toTriggerThr(XChange *XGlobal) {
THR_EXITCODE thr_exit = 1;
char TriggerLine[1024];
FServParse TriggerParse;
const char *Nick;
const char *Trigger;
FilesDetail *FD;
const char *tmpstr;
Helper H;
unsigned long MyIp;
const char *RemoteDottedIP;
unsigned long RemoteLongIP;
char MyNick[64];
TriggerTypeE TheTriggerType;
const char *pro_nick;

   TRACE_INIT_CRASH("ToTriggerThr");
   TRACE();
   H.init(XGlobal);

   while (true) {
      XGlobal->IRC_ToTrigger.getLineAndDelete(TriggerLine);
      if (XGlobal->isIRC_QUIT()) break;
      if (XGlobal->isIRC_DisConnected()) continue;

      // First word is Host, Second word is Nick
      // Rest is the possible trigger line, or PROPAGATION line.

      if (strlen(TriggerLine) < 2) continue;

      // So we need a TriggerParse class
      TriggerParse = TriggerLine;

      COUT(TriggerParse.printDebug();)
      TheTriggerType = TriggerParse.getTriggerType();
      if ( (TheTriggerType != FSERVCTCP) &&
           (TheTriggerType != PROPAGATIONCTCP)
         ) {
         // We only process FSERVCTCP's and PROPAGATIONCTCP's
         continue;
      }

      bool to_propagate = false;

      Nick = TriggerParse.getTriggerNick();
      Trigger = TriggerParse.getTriggerName();

      RemoteLongIP = TriggerParse.getTriggerLongIP();
      if (RemoteLongIP != IRCNICKIP_UNKNOWN) {
         XGlobal->NickList.setNickIP((char *) Nick, RemoteLongIP);
      }

      // Lets tell FilesDB to update all files held by nick. So we can
      // avoid accessing his trigger as much as possible.
      // only if not PROPAGATIONCTCP
      if (TheTriggerType != PROPAGATIONCTCP) {
         XGlobal->FilesDB.updateTimeFilesOfNick((char *) Nick);
      }
      else {
         // Check the update count of the nick's entry in FilesDB
         // and set to_propagate if applicable.
         pro_nick = TriggerParse.getPropagatedNick();

         FD = XGlobal->FilesDB.getFilesDetailListNickFile((char *) pro_nick, "TriggerTemplate");
         // If UpdateCount <= 1, we just recently got the list, possibly
         // cause of the propagation. Hence do not propagate this time.
         if ( (FD == NULL) || (FD->UpdateCount > 1) ) {
            // Getting lot of Propagation requests.
            if (FD) {
               COUT(cout << "PROPAGATION: to_propagate = true FD: ";)
               COUT(XGlobal->FilesDB.printDebug(FD);)
            }
            to_propagate = true;
         }
         if (FD) {
            XGlobal->FilesDB.freeFilesDetailList(FD);
            FD = NULL;
         }
         COUT(cout << "ToTriggerThr: PROPAGATIONCTCP: Propagated Nick: " << pro_nick << " needs to be propagated." << endl;)
      }

      // Let us also update the Sends and Qs information with latest
      // that we have gathered now.
      XGlobal->FilesDB.updateSendsQueuesOfNick((char *) Nick,
                         TriggerParse.getCurrentSends(),
                         TriggerParse.getTotalSends(),
                         TriggerParse.getCurrentQueues(),
                         TriggerParse.getTotalQueues());

      COUT(TriggerParse.printDebug();)
      if ( (TheTriggerType == PROPAGATIONCTCP) &&
           (to_propagate == false)
         ) {
         // Nothing to do as we have decided not to propagate.
         continue;
      }
      // For Type FSERVCTCP
      // - If FilesDB doesnt have info on this nick and
      // - If FServClientPending doesnt have this nick and
      // - If FServClientInProgress doesnt have this nick.
      // for Type = PROPAGATIONCTCP
      // - If FServClientPending doesnt have this nick and
      // - If FServClientInProgress doesnt have this nick.

      // So first check if a FServ access has been initiated or in progress.
      if ( XGlobal->FServClientPending.isNickPresent(Nick) ||
           XGlobal->FServClientInProgress.isNickPresent(Nick)
         ) {
         // We already have an attempt in progress.
         continue;
      }

      if ( (TheTriggerType == FSERVCTCP) &&
           (XGlobal->FilesDB.isFilesOfNickPresent((char *) Nick))
         ) {
         // In case of FSERVCTCP if we already have the file list
         // we do not attempt the trigger.
         continue;
      }

      XGlobal->getIRC_Nick(MyNick);
      XGlobal->resetIRC_Nick_Changed();

      // With new Propagation Algo in place, no delay.

      // if a Propagation CTCP, then check if pro_nick is in channel.
      if ( (TheTriggerType == PROPAGATIONCTCP) &&
           (XGlobal->NickList.isNickInChannel(CHANNEL_MAIN, (char *) pro_nick) == false)
         ) {
         COUT(cout << "toTriggerThr: Not Issuing PROPAGATIONCTCP as Nick " << pro_nick << " is not in Channel anymore: " << TriggerLine << endl;)
         continue;
      }

      // If a Propagation CTCP, then check if a trigger has already been issued
      // and is in FServClientInProgress. (FServClientPending doesnt count, as
      // we might fail to make a connection)
      if ( (TheTriggerType == PROPAGATIONCTCP) &&
           XGlobal->FServClientInProgress.isPresentMatchingPropagatedNick(pro_nick)
         ) {
         COUT(cout << "toTriggerThr: Not Issuing PROPAGATIONCTCP as a FServClient access is already in progress to get listing of " << pro_nick << endl;)
         continue;
      }

      // We are here, we can possibly issue the trigger of FSERVCTCP types
      // if propagation algorithm allows us to.

      // Propagation Algorithm.
      int NFCount = XGlobal->NickList.getNFMMcount(CHANNEL_MAIN);
      int FWCount = XGlobal->NickList.getFWMMcount(CHANNEL_MAIN);
      int MyNFIndex = XGlobal->NickList.getNickNFMMindex(CHANNEL_MAIN, MyNick);
      // Catch the trigger if I am NF and first or last guy in list
      // or NFcount == 0 (no trigger catchers in our group)
      // So I do not catch trigger if I am not first in list
      // and I am not last in list
      // and there is some trigger catcher in our group.
      // We always catch a PROPAGATION Trigger.
      if (TheTriggerType != PROPAGATIONCTCP) {
         if ( (MyNFIndex != 1) && 
              (MyNFIndex != NFCount) &&
              (NFCount != 0)
            ) {
            COUT(cout << "ToTriggerThr: Not issuing trigger of: " << Nick << " MyNFIndex: " << MyNFIndex << " NFCount: " << NFCount << " FWCount: " << FWCount << endl;)
#ifdef DEBUG
            sprintf(TriggerLine, "PRIVMSG %s :Not issuing trigger of %s - MyNFIndex: %d NFCount: %d FWCount: %d", CHANNEL_MAIN, Nick, MyNFIndex, NFCount, FWCount);
            XGlobal->IRC_ToServer.putLine(TriggerLine);
#endif
            continue;
         }

         COUT(cout << "ToTriggerThr: Issuing trigger of: " << Nick << " MyNFIndex: " << MyNFIndex << " NFCount: " << NFCount << " FWCount: " << FWCount << endl;)
#ifdef DEBUG
         sprintf(TriggerLine, "PRIVMSG %s :Issuing trigger of %s - MyNFIndex: %d NFCount: %d FWCount: %d", CHANNEL_MAIN, Nick, MyNFIndex, NFCount, FWCount);
         XGlobal->IRC_ToServer.putLine(TriggerLine);
#endif
      }
      else {
         // This is the Propagation Trigger, which needs to be propagated.
         // Issue only if TriggerParse.getPropagatedNick() is not us.
         if (strcasecmp(MyNick, pro_nick) == 0) {
#ifdef DEBUG
            sprintf(TriggerLine, "PRIVMSG %s :PROPAGATIONCTCP - Not Issuing trigger of %s (this is our nick) - MyNFIndex: %d NFCount: %d FWCount: %d For ProNick: %s", CHANNEL_MAIN, Nick, MyNFIndex, NFCount, FWCount, pro_nick);
            XGlobal->IRC_ToServer.putLine(TriggerLine);
#endif
            COUT(cout << "ToTriggerThr: PROPAGATIONCTCP: Not Issuing trigger of: " << pro_nick << " as its same as out Nick " << Nick << endl;)
            continue;
         }
#ifdef DEBUG
         sprintf(TriggerLine, "PRIVMSG %s :PROPAGATIONCTCP - Issuing trigger of %s - MyNFIndex: %d NFCount: %d FWCount: %d For ProNick: %s", CHANNEL_MAIN, Nick, MyNFIndex, NFCount, FWCount, pro_nick);
         XGlobal->IRC_ToServer.putLine(TriggerLine);
#endif
         COUT(cout << "ToTriggerThr: PROPAGATIONCTCP: Issuing trigger of: " << Nick << " for propagation of " << pro_nick << " MyNFIndex: " << MyNFIndex << " NFCount: " << NFCount << " FWCount: " << FWCount << endl;)
      }

      // We are here, => Issue the damn trigger!!
      // We pass on our IP too in case of a MasalaMate trigger.
      if (XGlobal->NickList.getNickClient((char *) Nick) == IRCNICKCLIENT_MASALAMATE) {
         MyIp = XGlobal->getIRC_IP(NULL);
         XGlobal->resetIRC_IP_Changed();
         sprintf(TriggerLine, "PRIVMSG %s :\001%s %lu\001", Nick, Trigger, MyIp);
      }
      else {
         sprintf(TriggerLine, "PRIVMSG %s :\001%s\001", Nick, Trigger);
      }
      COUT(cout << "toTriggerThr: Issuing: " << TriggerLine << endl;)

      // We add the "TriggerTemplate"
      FD = new FilesDetail;
      XGlobal->FilesDB.initFilesDetail(FD);
      FD->TriggerType = FSERVCTCP;
      tmpstr = TriggerParse.getTriggerNick();
      FD->Nick = new char[strlen(tmpstr) + 1];
      strcpy(FD->Nick, tmpstr);

      FD->FileName = new char[16];
      strcpy(FD->FileName, "TriggerTemplate");
      FD->TriggerName = new char[strlen(Trigger) + 1];
      strcpy(FD->TriggerName, Trigger);

      // First delete the previous "TriggerTemplate" if present.
      XGlobal->FilesDB.delFilesDetailNickFile(FD->Nick, FD->FileName);

      FD->ClientType = XGlobal->NickList.getNickClient(FD->Nick);
      FD->CurrentSends = TriggerParse.getCurrentSends();
      FD->TotalSends = TriggerParse.getTotalSends();
      FD->CurrentQueues = TriggerParse.getCurrentQueues();
      FD->TotalQueues = TriggerParse.getTotalQueues();

      // Lets make a copy to be used for "Inaccessible Server"
      FilesDetail *InacFD = XGlobal->FilesDB.copyFilesDetail(FD);
      delete [] InacFD->FileName;
      InacFD->FileName = new char[20];
      strcpy(InacFD->FileName, "Inaccessible Server");

      // Add both to FilesDB. -> 2 entries to start with.
      // Note that DCCChatClient will first thing remove the
      // "Inaccessible Server" entry.
      XGlobal->FilesDB.addFilesDetail(FD);

      XGlobal->FilesDB.addFilesDetail(InacFD);

      // Add it in FServClientPending as a pending trigger.
      FD = new FilesDetail;
      XGlobal->FServClientPending.initFilesDetail(FD);
      FD->Nick = new char[strlen(Nick) + 1];
      strcpy(FD->Nick, Nick);
      if (TheTriggerType == PROPAGATIONCTCP) {
         // Note down PropagatedNick.
         FD->PropagatedNick = new char[strlen(pro_nick) + 1];
         strcpy(FD->PropagatedNick, pro_nick);
      }
      else {
         // Note down Nick as the Propagating Nick for FSERVCTCP
         FD->PropagatedNick = new char[strlen(FD->Nick) + 1];
         strcpy(FD->PropagatedNick, FD->Nick);
      }
      FD->TriggerType = FSERVCTCP;
      FD->TriggerName = new char[strlen(Trigger) + 1];
      strcpy(FD->TriggerName, Trigger);

      RemoteDottedIP = TriggerParse.getTriggerDottedIP();
      FD->DottedIP = new char[strlen(RemoteDottedIP) + 1];
      strcpy(FD->DottedIP, RemoteDottedIP);
 
      XGlobal->FServClientPending.addFilesDetail(FD);
      H.sendLineToNick((char *) Nick, TriggerLine);

      // Update UI.
      if (TheTriggerType == PROPAGATIONCTCP) {
         sprintf(TriggerLine, "Server 08Trigger: Propagation Algorithm: Issuing trigger of %s for getting file list of %s", Nick, pro_nick);
      }
      else {
         sprintf(TriggerLine, "Server 08Trigger: Issuing trigger of %s", Nick);
      }
      XGlobal->IRC_ToUI.putLine(TriggerLine);
   }

   COUT(cout << "toTriggerThr: Quitting" << endl;)
// We dont call Exit thread as we want to go out of scope => destrcutors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}

