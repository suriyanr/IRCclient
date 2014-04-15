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

// Thread which is fed privmsg FServ lines got in channel.
// It tries to figure out if it is an FServ trigger, and extracts the
// the trigger information and issues the trigger.
// The first word in the line is the Nick. 2nd word on is the possible
// trigger line.
// Note that the XDCC line is updated in the fromServerThr itself.
// This is different from toTriggerThr, in the sense that it doesnt
// delay a random long duration before issuing trigger, but only
// IRCSERVER_TARGET_CHANGE_DELAY. Used to process results from the user
// typing !list or !list <nick> in channel.
// Compulsorily gets the filelist.
void toTriggerNowThr(XChange *XGlobal) {
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

   TRACE_INIT_CRASH("ToTriggerNowThr");
   TRACE();
   H.init(XGlobal);

   while (true) {
      XGlobal->IRC_ToTriggerNow.getLineAndDelete(TriggerLine);
      if (XGlobal->isIRC_QUIT()) break;
      if (XGlobal->isIRC_DisConnected()) continue;

      // First word is Host, Second word is Nick
      // Rest is the possible trigger line.

      if (strlen(TriggerLine) < 2) continue;

      // So we need a TriggerParse class
      TriggerParse = TriggerLine;
      COUT(TriggerParse.printDebug();)
      if (TriggerParse.getTriggerType() != FSERVCTCP) continue;

      Nick = TriggerParse.getTriggerNick();
      Trigger = TriggerParse.getTriggerName();

      RemoteLongIP = TriggerParse.getTriggerLongIP();
      if (RemoteLongIP != IRCNICKIP_UNKNOWN) {
         XGlobal->NickList.setNickIP((char *) Nick, RemoteLongIP);
      }

      COUT(cout << "toTriggerNowThr: " << Nick << " " << Trigger << endl;)

      // Lets us also update the Sends and Qs information with latest
      // that we have gathered now.
      XGlobal->FilesDB.updateSendsQueuesOfNick((char *) Nick,
                         TriggerParse.getCurrentSends(),
                         TriggerParse.getTotalSends(),
                         TriggerParse.getCurrentQueues(),
                         TriggerParse.getTotalQueues());


      // If we are waiting on this nick's trigger to respond or currently
      // are accessing it, no more processing required.
      if ( XGlobal->FServClientPending.isNickPresent(Nick) ||
           XGlobal->FServClientInProgress.isNickPresent(Nick)
         ) {
         sprintf(TriggerLine, "Server 08TriggerNow: Will not issue trigger of %s as we are awaiting a response from a previously issued trigger", Nick);
         XGlobal->IRC_ToUI.putLine(TriggerLine);
         continue;
      }

      // if FServClientPending/InProgress doesnt have this
      // nick & Trigger information. We need to issue the trigger.

      // If this is an MM server we are going to issue the trigger of
      // We do not delete the whole list, but just the entries for
      // "TriggerTemplate" and "Inaccessible Server"
      // This is so that we do not have to get full listing if its
      // the same file list we already have. If we are talking FFLC v2
      if (TriggerParse.getClientType() == IRCNICKCLIENT_MASALAMATE) {
         XGlobal->FilesDB.delFilesDetailNickFile((char *) Nick, "TriggerTemplate");
         XGlobal->FilesDB.delFilesDetailNickFile((char *) Nick, "Inaccessible Server");
         // The above two entries do get added later on below.
      }
      else {
         // Delete the files held by this Nick.
         XGlobal->FilesDB.delFilesOfNick((char *) Nick);
      }

      // Inform in the server TAB about our decision.
      sprintf(TriggerLine, "Server 08TriggerNow: Issuing trigger of %s.", Nick);
      XGlobal->IRC_ToUI.putLine(TriggerLine);

      // We pass on our IP too in case of a MasalaMate trigger.
      if (XGlobal->NickList.getNickClient((char *) Nick) == IRCNICKCLIENT_MASALAMATE) {
         MyIp = XGlobal->getIRC_IP(NULL);
         XGlobal->resetIRC_IP_Changed();
         sprintf(TriggerLine, "PRIVMSG %s :\001%s %lu\001", Nick, Trigger, MyIp);
      }
      else {
         sprintf(TriggerLine, "PRIVMSG %s :\001%s\001", Nick, Trigger);
      }

      COUT(cout << "toTriggerNowThr: Issuing: " << TriggerLine << endl;)

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

      // FromServerThr has update the NickClient type.
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
      FD->TriggerType = FSERVCTCP;
      FD->TriggerName = new char[strlen(Trigger) + 1];
      strcpy(FD->TriggerName, Trigger);

      RemoteDottedIP = TriggerParse.getTriggerDottedIP();
      FD->DottedIP = new char[strlen(RemoteDottedIP) + 1];
      strcpy(FD->DottedIP, RemoteDottedIP);

      XGlobal->FServClientPending.addFilesDetail(FD);
      H.sendLineToNick((char *) Nick, TriggerLine);

   }
   COUT(cout << "toTriggerThrNow: Quitting" << endl;)
// We dont call Exit thread as we want to go out of scope => destrcutors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}

