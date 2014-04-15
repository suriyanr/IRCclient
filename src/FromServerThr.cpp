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
#include "Utilities.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

// Thread which gets lines from the FromServer Q
// Takes actions wrt most of the server lines.
void fromServerThr(XChange *XGlobal) {
IRCLineInterpret LineInterpret;
LineParse Line;
const char *parseptr; // Return ptr from LineParse.
char IRC_Line[1024];
char Response[1024];
IRCChannelList CL;
char Nick[64];
IRCNickLists &NL = XGlobal->NickList;
char *word1 = NULL;
char modeflag = 'u';
int nickindex;
int i;
int nicklistcnt;
THR_EXITCODE thr_exit = 1;
SpamFilter SF; // Spam filter for ACTION/NOTICE/PRIVMSG
Helper H;
TCPConnect T; // Just for a little help.
unsigned long longip;
LineParse LineP, tempLP;
FServParse TriggerParse;
int mm_ads_seen = 0;
time_t cur_time;
char DotIP[20];

   TRACE_INIT_CRASH("FromServerThr");
   TRACE();
   Nick[0] = '\0';

// Get a working Nick
   while (XGlobal->isIRC_Nick_Changed(Nick) == false) {
      sleep(1);
      if (XGlobal->isIRC_QUIT()) break;
   }
   XGlobal->getIRC_Nick(Nick);
   XGlobal->resetIRC_Nick_Changed();

   // Lets set up the SpamFilter.
//   SF.addRule("www+.|masala+sysreset+mirc");
//   SF.addRule("http+.|masala+sysreset+mirc");
//   SF.addRule("join|masala");
//   SF.addRule("streamsale+com|masala");
// Above has been moved to the SpamFilter constructor.

   // Lets initialise the Helper.
   H.init(XGlobal);

   while (true) {
      XGlobal->IRC_FromServer.getLineAndDelete(IRC_Line);
      if (XGlobal->isIRC_QUIT()) break;
      if (XGlobal->isIRC_DisConnected()) continue;

      // spurious lines coming in when run through debugger.
      if (strlen(IRC_Line) < 2) continue;

//COUT(cout << "fromServerThr: Got: " << IRC_Line[0] << IRC_Line[1] << IRC_Line[2] << IRC_Line[3] << IRC_Line[4] << IRC_Line[5] << IRC_Line[6] << " ..." << endl;)
COUT(cout << "fromServerThr: Got: " << IRC_Line << endl;)

      delete [] word1;
      word1 = new char[2 * strlen(IRC_Line) + 1]; // Used with ToUI Q as well
//      COUT(cout << "FromServerThr: Processing: " << IRC_Line << endl;)
      LineInterpret = IRC_Line;
//    We queue the CTCPs (non DCC) in the CTCP Q.
//    We queue the DCC CTCPs in the DCC Q.
//    We take care of PING, ERROR, maintaining the NICK lists (JOIN/PART/KICK)
//      NICK Change.
      switch (LineInterpret.Command) {
         case IC_MODE:
            COUT(LineInterpret.printDebug();)
//          InfoLine contains the modeline string: eg. +v-h a0000 a0000
//          LineInterpret.Channel = Channel that its been set at.
/*
     Update voice, halfop and op list, even if its done for multi nicks 
     in one line.
     Channel modes that can be set are as follows:
     Unless otherwise mentioned, the ones that take an argument, take one
     argument when '-' and '+'
     b = ban, takes an argument.
     c = no colors, no arguments.
     i = invite only, no arguments.
     k = key, takes an argument.
     l = channel user limit, +l takes a parameter, -l doesnt.
     m = channel moderated, no arguments.
     n = no external messages, no arguments.
     p = private, no arguments.
     r = registered, no arguments.
     R = registered nicks only, no arguments.
     s = secret, no arguments.
     t = only ops can set topic, no arguments.

     o = op, takes an argument.
     v = voice, takes an argument.
     h = halfop, takes an argument.

     Hence the mode switches can be like:
     Chanserv set mode +ovv-vv+l op1 voice1 voice2 devoice3 devoice4 50
     and they can come mixed with other channel modes as listed above.
     The ones that always take arguments are b, k, o and v. +l takes an              argument, whereas -l doesnt. All the other characters do not take
     arguments. 
     
     So on getting a mode line, we scan the string, and extract the relevant
     +, - of v, h and o, and update our internal list.
*/
//          Lets Q it to UI correctly.
            if (LineInterpret.From.Type == ILFT_SERVERMESSAGE) {
               sprintf(Response, "%s * 03%s sets mode: %s", LineInterpret.Channel, LineInterpret.From.Server, LineInterpret.InfoLine);
            }
            else {
               sprintf(Response, "%s * 03%s sets mode: %s", LineInterpret.Channel, LineInterpret.From.Nick, LineInterpret.InfoLine);
            }
            XGlobal->IRC_ToUI.putLine(Response);
            
            Line = LineInterpret.InfoLine;
            parseptr = Line.getWord(1); // word1 contains +vvv-v+ooo+l
            strcpy(word1, parseptr);
//COUT(cout << "MODE: " << word1 << endl;)
            i = 0;
            nickindex = 1;
            modeflag = 'u'; // Undefined.
            while (word1[i] != '\0') {
            int nick_mode;
            char *workingnick;

               switch (word1[i]) {
                 case '+':
                    modeflag = '+';
                    break;

                 case '-':
                    modeflag = '-';
                    break;

                 case 'l':
                    if (modeflag == '+') nickindex++;
                    break;

                 case 'k':
                 case 'b':
                    nickindex++;
                    break;

                 case 'v' :
                 case 'h' :
                 case 'o' :
                    nickindex++;
                    parseptr = Line.getWord(nickindex);
                    workingnick = new char[strlen(parseptr) + 1];
                    strcpy(workingnick, parseptr);
                    nick_mode = NL.getNickMode(LineInterpret.Channel, workingnick);
//COUT(cout << "workingnick: " << workingnick << " nickindex: " << nickindex << " orig nick_mode: " << nick_mode << " modeflag: " << modeflag << endl;)
                    if (modeflag == '+') {
                       switch (word1[i]) {
                          case 'v':
                             nick_mode = ADD_VOICE(nick_mode);
                             break;

                          case 'h':
                             nick_mode = ADD_HALFOP(nick_mode);
                             break;

                          case 'o':
                             nick_mode = ADD_OP(nick_mode);
                             // Here we check if we ourselves have become
                             // op in CHANNEL_MM.
                             if ( (strcasecmp(LineInterpret.Channel, CHANNEL_MM) == 0) && (strcasecmp(workingnick, Nick) == 0) ) {
                                // Set key.
                                sprintf(Response, "MODE %s +k %s", CHANNEL_MM, CHANNEL_MM_KEY);
                                XGlobal->IRC_ToServer.putLine(Response);
                                // Set private and secret.
                                sprintf(Response, "MODE %s +ps", CHANNEL_MM);
                                XGlobal->IRC_ToServer.putLine(Response);

                             }
                             break;

                          default:
                             break;
                       }
                    }
                    else if(modeflag == '-') {
                       switch (word1[i]) {
// We Or it with IRCNICKMODE_REGULAR so that retain their REG status.
                          case 'v':
                             nick_mode = REMOVE_VOICE(nick_mode);
//COUT(cout << "nick_mode after -v: " << nick_mode << endl;)
                             break;

                          case 'h':
                             nick_mode = REMOVE_HALFOP(nick_mode);
                             break;

                          case 'o':
                             nick_mode = REMOVE_OP(nick_mode);
                             break;

                          default:
                             break;
                       }
                    }
                    NL.setNickMode(LineInterpret.Channel, workingnick, nick_mode);
//                  Lets notify NickList UI to update correctly.
                    sprintf(Response, "*NICKMOD* %s %s %d", LineInterpret.Channel, workingnick, nick_mode);
                    XGlobal->IRC_ToUI.putLine(Response);

                    delete [] workingnick;
                    break;

                 default:
                    break;
               }
               i++;
            }
//            NL.printDebug();

            break;

         case IC_PING:
            COUT(LineInterpret.printDebug();)
            if (LineInterpret.From.Type == ILFT_COMMANDMESSAGE) {
//             Only respond to a Server PING.
               IRC_Line[1] = 'O';
               XGlobal->IRC_ToServerNow.putLine(IRC_Line);

               // Update PingPongTime accordingly.
               XGlobal->lock();
               XGlobal->PingPongTime = time(NULL);
               XGlobal->unlock();
            }
            joinChannels(XGlobal);
            break;

         case IC_PONG:
//          Server is responding to our PING.
            COUT(LineInterpret.printDebug();)
            // Update PingPongTime accordingly.
            XGlobal->lock();
            XGlobal->PingPongTime = time(NULL);
            XGlobal->unlock();

            joinChannels(XGlobal);
            break;

         case IC_ERROR:
            COUT(LineInterpret.printDebug();)
            //  Lets Q it to UI correctly.
            sprintf(Response, "Server * 04Error: %s", LineInterpret.InfoLine);
            XGlobal->IRC_ToUI.putLine(Response);

            break;

         case IC_SERVER_NOTICE:
            COUT(LineInterpret.printDebug();)
            //  Lets Q it to UI correctly.
            sprintf(Response, "Server * 03SERVER NOTICE: %s", LineInterpret.InfoLine);
            XGlobal->IRC_ToUI.putLine(Response);
            break;

         case IC_JOIN:
//            LineInterpret.printDebug();

            if (strcasecmp(Nick, LineInterpret.From.Nick) == 0) {
//             I joined Channel.
               CL = XGlobal->getIRC_CL();
               CL.setJoined(LineInterpret.Channel);
               XGlobal->resetIRC_CL_Changed();
               XGlobal->putIRC_CL(CL);
               COUT(CL.printDebug();)
               NL.delChannel(LineInterpret.Channel);
               NL.addChannel(LineInterpret.Channel);
               COUT(cout << "Joined Channel: " << LineInterpret.Channel << endl;)

               NL.addNick(LineInterpret.Channel, LineInterpret.From.Nick, IRCNICKMODE_REGULAR);
               // Lets start from a clean slate of FilesDB if we have joined
               // CHANNEL_MAIN
               if (strcasecmp(LineInterpret.Channel, CHANNEL_MAIN) == 0) {
                  cur_time = time(NULL);
                  XGlobal->FilesDB.purge();

                  // Lets set the Ad Time to be Current + FSERV_INITIAL_AD_TIME
                  XGlobal->lock();
                  XGlobal->FServAdTime = cur_time + FSERV_INITIAL_AD_TIME;
                  COUT(cout << "ADSYNC: Current Time: " << cur_time << " FServAdTime: " << XGlobal->FServAdTime << endl);
                  XGlobal->unlock();

                  // Set the mm_ads_seen to 0. We have seen 0 MM ads till now.
                  mm_ads_seen = 0;
               }

//             Lets notify NickList UI to update correctly.
               sprintf(Response, "*NICK_MY* %s %s", LineInterpret.Channel, LineInterpret.From.Nick);
               XGlobal->IRC_ToUI.putLine(Response);

//             Lets Q it to UI correctly.
               sprintf(Response, "%s * 09I, known as 08%s09,01, have joined channel %s", LineInterpret.Channel, LineInterpret.From.Nick, LineInterpret.Channel);
               XGlobal->IRC_ToUI.putLine(Response);

               // We do not issue a !list on joining channel.
               // It just generates useless traffic.
               // if (strcasecmp(LineInterpret.Channel, CHANNEL_MAIN) == 0) {
               //    // Lets issue the !list if we joined CHANNEL_MAIN
               //    sprintf(Response, "PRIVMSG %s :!list", CHANNEL_MAIN);
               //    XGlobal->IRC_ToServer.putLine(Response);
               // }

            }
            else {
               COUT(cout << NL.getNickCount(LineInterpret.Channel) << " is Channel Count in " << LineInterpret.Channel << endl;)
               COUT(cout << LineInterpret.From.Nick << " joined " << LineInterpret.Channel << endl;)
               NL.addNick(LineInterpret.Channel, LineInterpret.From.Nick, IRCNICKMODE_REGULAR);
               COUT(cout << NL.getNickCount(LineInterpret.Channel) << " is Channel Count in " << LineInterpret.Channel << endl;)
//               NL.printDebug();

//             Lets Q it to UI correctly.
               sprintf(Response, "%s * 09%s (%s@%s) has joined %s", LineInterpret.Channel, LineInterpret.From.Nick, LineInterpret.From.UserName, LineInterpret.From.Host, LineInterpret.Channel);
               XGlobal->IRC_ToUI.putLine(Response);

//             Lets notify NickList UI to update correctly.
               sprintf(Response, "*NICKADD* %s !%s", LineInterpret.Channel, LineInterpret.From.Nick);
               XGlobal->IRC_ToUI.putLine(Response);

               // Check if the joined channel is CHANNEL_MM, in which case
               // the joined nick is to be marked as a MASALAMATE_CLIENT
               if (strcasecmp(LineInterpret.Channel, CHANNEL_MM) == 0) {
                  H.markAsMMClient(LineInterpret.From.Nick);

                  // Now do what is necessary if we are possible OP in
                  // CHANNEL_MM
                  H.doOpDutiesIfOpInChannelMM(Nick);
               }
            }
            break;

          case IC_PART:
             COUT(LineInterpret.printDebug();)
//          Lets Q it to UI correctly.
            sprintf(Response, "%s * 03%s (%s@%s) has left %s", LineInterpret.Channel, LineInterpret.From.Nick, LineInterpret.From.UserName, LineInterpret.From.Host, LineInterpret.Channel);
            XGlobal->IRC_ToUI.putLine(Response);

//          Lets notify NickList UI to update correctly.
            sprintf(Response, "*NICKDEL* %s %s", LineInterpret.Channel, LineInterpret.From.Nick);
            XGlobal->IRC_ToUI.putLine(Response);

             if (!strcasecmp(Nick, LineInterpret.From.Nick)) {
                COUT(cout << "Parted Channel: " << LineInterpret.Channel << endl;)
                CL = XGlobal->getIRC_CL();
                CL.setNotJoined(LineInterpret.Channel);
                XGlobal->resetIRC_CL_Changed();
                XGlobal->putIRC_CL(CL);
                COUT(cout << "fromServerThr" << endl;)
                COUT(CL.printDebug();)
                NL.delChannel(LineInterpret.Channel);
//              Need to trash out the NL as I have quit from channel.
             }
             else {
                FilesDetail *FD;
                COUT(cout << NL.getNickCount(LineInterpret.Channel) << " is Channel Count in " << LineInterpret.Channel << endl;)
                COUT(cout << LineInterpret.From.Nick << " parted " << LineInterpret.Channel << endl;)
                NL.delNick(LineInterpret.Channel, LineInterpret.From.Nick);
               COUT(cout << NL.getNickCount(LineInterpret.Channel) << " is Channel Count in " << LineInterpret.Channel << endl;)
//                NL.printDebug();

               // Update the FilesDetailList (abort uploads, queues)
               // Only if PARTED from CHANNEL_MAIN
               if (strcasecmp(LineInterpret.Channel, CHANNEL_MAIN) == 0) {
                  // Remove Entries of this nick from FilesDB
                  XGlobal->FilesDB.delFilesOfNick(LineInterpret.From.Nick);

                  H.removeNickFromAllFDs(LineInterpret.From.Nick);
               }
               else if (strcasecmp(LineInterpret.Channel, CHANNEL_MM) == 0) {
                  H.doOpDutiesIfOpInChannelMM(Nick);
               }
            }
            break;

         case IC_QUIT:
            //LineInterpret.printDebug();

            // Remove Entries of this nick from FilesDB
            XGlobal->FilesDB.delFilesOfNick(LineInterpret.From.Nick);

            {
            int totalc;
            char chanName[32];
            FilesDetail *FD;

                totalc = NL.getChannelCount();
                for (i = 1; i <= totalc; i++) {
                    NL.getChannelName(chanName, i);

                    if (NL.isNickInChannel(chanName, LineInterpret.From.Nick)) {
                       char *quit_mesg;
//                     Have to remove entry of nick from this channel.
//                     Lets Q it to UI correctly.
                       if (strcasestr(LineInterpret.To, CLIENT_NAME) ||
                           strcasestr(LineInterpret.To, NETWORK_NAME) ) {
                          // Put MasalaMate/IRCSuper Quit messages in UI.
                          quit_mesg = LineInterpret.To;
                       }
                       else {
                          quit_mesg = LineInterpret.InfoLine;
                       }
                       sprintf(Response, "%s * 14%s (%s@%s) Quit (%s)", chanName, LineInterpret.From.Nick, LineInterpret.From.UserName, LineInterpret.From.Host, quit_mesg);
                       XGlobal->IRC_ToUI.putLine(Response);

//                     Lets notify NickList UI to update correctly.
                       sprintf(Response, "*NICKDEL* %s %s", chanName, LineInterpret.From.Nick);
                       XGlobal->IRC_ToUI.putLine(Response);

                       COUT(cout << LineInterpret.From.Nick << " quit IRC." << endl;)
                       NL.delNick(chanName, LineInterpret.From.Nick); 
                       COUT(cout << NL.getNickCount(chanName) << " is Channel Count in " << chanName << endl;)
                   }
                }
               // Update the FilesDetailList (abort uploads, queues)
               // If its an actual QUIT and not PING timeouts etc
               // This might vary from network to network.
               // If Infoline has the word Quit in it then its user induced 
               if (strstr(LineInterpret.InfoLine, "Quit") != NULL) {
                  COUT(cout << "IC_QUIT: LineInterpret.InfoLine: " << LineInterpret.InfoLine << endl;)
                  H.removeNickFromAllFDs(LineInterpret.From.Nick);
               }

               // Do the stuff for CHANNEL_MM
               H.doOpDutiesIfOpInChannelMM(Nick);
            }
//             NL.printDebug();
            break;

         case IC_KICK:
            COUT(LineInterpret.printDebug();)

            if (!strcasecmp(Nick, LineInterpret.KickedNick)) {
//             I have been kicked.
               COUT(cout << "Kicked From Channel: " << LineInterpret.Channel << endl;)
               CL = XGlobal->getIRC_CL();
               CL.setNotJoined(LineInterpret.Channel);
               XGlobal->resetIRC_CL_Changed();
               XGlobal->putIRC_CL(CL);
               COUT(cout << "fromServerThr" << endl;)
               COUT(CL.printDebug();)
               NL.delChannel(LineInterpret.Channel);
//             Need to trash out the NL as I have quit from channel.

//             Lets Q it to UI correctly.
               sprintf(Response, "%s * 03You were kicked by %s (%s)", LineInterpret.Channel, LineInterpret.From.Nick, LineInterpret.InfoLine);
               XGlobal->IRC_ToUI.putLine(Response);

//             Lets notify NickList UI to update correctly.
               sprintf(Response, "*NICKCLR* %s %s", LineInterpret.Channel, LineInterpret.KickedNick);
               XGlobal->IRC_ToUI.putLine(Response);

               if (strcasecmp(LineInterpret.Channel, CHANNEL_MAIN) == 0) {
                  // Trigger an Upgrade process, if kicked from main.
                  XGlobal->IRC_ToUI.putLine("*UPGRADE* TRIGGER");
               }

               joinChannels(XGlobal);
            }
            else {
            FilesDetail *FD;
               COUT(cout << NL.getNickCount(LineInterpret.Channel) << " is Channel Count in " << LineInterpret.Channel << endl;)
               COUT(cout << LineInterpret.KickedNick << " was kicked from " << LineInterpret.Channel << endl;)
                NL.delNick(LineInterpret.Channel, LineInterpret.KickedNick);
               COUT(cout << NL.getNickCount(LineInterpret.Channel) << " is Channel Count in " << LineInterpret.Channel << endl;)
//                NL.printDebug();

//             Lets Q it to UI correctly.
               sprintf(Response, "%s * 03%s was kicked by %s (%s)", LineInterpret.Channel, LineInterpret.KickedNick, LineInterpret.From.Nick, LineInterpret.InfoLine);
               XGlobal->IRC_ToUI.putLine(Response);

//             Lets notify NickList UI to update correctly.
               sprintf(Response, "*NICKDEL* %s %s", LineInterpret.Channel, LineInterpret.KickedNick);
               XGlobal->IRC_ToUI.putLine(Response);

               // Update the FilesDetailList (abort uploads, queues)
               // Only if KICKED from CHANNEL_MAIN
               if (strcasecmp(LineInterpret.Channel, CHANNEL_MAIN) == 0) {
                  // Remove Entries of this nick from FilesDB
                  XGlobal->FilesDB.delFilesOfNick(LineInterpret.KickedNick);

                  // Remove from Queues.
                  H.removeNickFromAllFDs(LineInterpret.KickedNick);
               }
               else if (strcasecmp(LineInterpret.Channel, CHANNEL_MM) == 0) {
                  // Do the possible OP duties in CHANNEL_MM
                  H.doOpDutiesIfOpInChannelMM(Nick);
               }
            }
            break;

         case IC_NICKCHANGE:
            char chanName[32];
            int totalc;

//          LineInterpret.From.Nick has org nick, InfoLine has new nick.
            COUT(LineInterpret.printDebug();)

            if (strcasecmp(LineInterpret.From.Nick, Nick) == 0) {
//             Our nick has changed.

               // If we have set a Nick password, we identify here.
               XGlobal->getIRC_Nick(word1);
               XGlobal->getIRC_Password(word1);
               XGlobal->resetIRC_Nick_Changed();
               if (strlen(word1)) {
                  // Now try to identify.
                  sprintf(Response, "PRIVMSG NickServ :IDENTIFY %s", word1);
                  XGlobal->IRC_ToServer.putLine(Response);
               }

               strcpy(Nick, LineInterpret.InfoLine);
               XGlobal->putIRC_Nick(Nick);
               XGlobal->putIRC_ProposedNick(Nick);
COUT(cout << "ThreadMain: IC_NICKCHANGE: From: " << LineInterpret.From.Nick << " To Nick: " << Nick << endl;)
               // Save in config file.
               H.writeIRCConfigFile();

//             Lets notify NickList UI to update correctly.
               NL.getChannelName(chanName, 1); // The below needs a valid one.
               sprintf(Response, "*NICK_MY* %s %s", chanName, Nick);
               XGlobal->IRC_ToUI.putLine(Response);

               // Lets correct MyFilesDB and MyPartialFilesDB
               XGlobal->MyFilesDB.renameNickToNewNick(LineInterpret.From.Nick, Nick);
               XGlobal->MyPartialFilesDB.renameNickToNewNick(LineInterpret.From.Nick, Nick);
            }
            {
                totalc = NL.getChannelCount();
                for (i = 1; i <= totalc; i++) {
                    NL.getChannelName(chanName, i);
                    if (NL.isNickInChannel(chanName, LineInterpret.From.Nick)) {
                       NL.changeNick(chanName, LineInterpret.From.Nick, LineInterpret.InfoLine);
//                     Lets Q it to UI correctly.
                       sprintf(Response, "%s * 03%s is now known as %s", chanName, LineInterpret.From.Nick, LineInterpret.InfoLine);
                       XGlobal->IRC_ToUI.putLine(Response);

//                     Lets notify NickList UI to update correctly.
                       sprintf(Response, "*NICKCHG* %s %s %s", chanName, LineInterpret.From.Nick, LineInterpret.InfoLine);
                       XGlobal->IRC_ToUI.putLine(Response);
                    }
                }
            }

            // Do the possible OP duties in CHANNEL_MM
            H.doOpDutiesIfOpInChannelMM(Nick);

            // Lets propagate this Nick Change in the FilesDetailList
            XGlobal->FilesDB.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->FServClientPending.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->FServClientInProgress.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->DCCAcceptWaiting.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->DwnldWaiting.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->DwnldInProgress.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->SendsInProgress.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->FileServerInProgress.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->QueuesInProgress.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->SmallQueuesInProgress.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->FileServerWaiting.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->DCCSendWaiting.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->DCCChatPending.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->DCCChatInProgress.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->SwarmWaiting.renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            for (i = 0; i < SWARM_MAX_FILES; i++) {
               XGlobal->Swarm[i].renameNickToNewNick(LineInterpret.From.Nick, LineInterpret.InfoLine);
            }
 
//            NL.printDebug();
            break;

         case IC_NICKLIST:
//            LineInterpret.printDebug();
//          Lets get the list of nicks which is in FileNameOrInfoLine
//          Each nick is seperated by a space, and possibly prepended with
//          + or % or @ for voice, halfop, op
            Line = LineInterpret.InfoLine;
COUT(cout << "NICKLIST Line: " << LineInterpret.InfoLine << endl;)
            sprintf(Response, "*NICKADD* %s", LineInterpret.Channel);
            nicklistcnt = Line.getWordCount();
            bool channel_mm;
            if (strcasecmp(LineInterpret.Channel, CHANNEL_MM) == 0) {
               channel_mm = true;
            }
            else {
               channel_mm = false;
            }
            for (i = 1; i <= nicklistcnt; i++) {
               parseptr = Line.getWord(i);
               strcpy(word1, parseptr);
              
               switch (word1[0]) {
                 case '+':
                    NL.addNick(LineInterpret.Channel, &word1[1], IRCNICKMODE_VOICE | IRCNICKMODE_REGULAR);
                    if (channel_mm) {
                       // Mark Nicks in CHANNEL_MM as MM clients.
                       H.markAsMMClient(&word1[1]);
                    }

//                  Lets prepare stuff to be sent to the NickList UI.
                    strcat(Response, " ");
                    strcat(Response, word1);
                    break;

                 case '%':
                    NL.addNick(LineInterpret.Channel, &word1[1], IRCNICKMODE_HALFOP | IRCNICKMODE_REGULAR);
                    if (channel_mm) {
                       // Mark Nicks in CHANNEL_MM as MM clients.
                       H.markAsMMClient(&word1[1]);
                    }
//                  Lets prepare stuff to be sent to the NickList UI.
                    strcat(Response, " ");
                    strcat(Response, word1);
                    break;

                 case '@':
                    NL.addNick(LineInterpret.Channel, &word1[1], IRCNICKMODE_OP | IRCNICKMODE_REGULAR);
//                  Lets prepare stuff to be sent to the NickList UI.
                    strcat(Response, " ");
                    strcat(Response, word1);

                    if (channel_mm) {
                       // Mark Nicks in CHANNEL_MM as MM clients.
                       H.markAsMMClient(&word1[1]);

                       // Handle in case we are op in CHANNEL_MM
                       if (strcasecmp(&word1[1], Nick) == 0) {
                          // Set key.
                          sprintf(word1, "MODE %s +k %s", CHANNEL_MM, CHANNEL_MM_KEY);
                          XGlobal->IRC_ToServer.putLine(word1);
                          // Set private and secret.
                          sprintf(word1, "MODE %s +ps", CHANNEL_MM);
                          XGlobal->IRC_ToServer.putLine(word1);
                       }
                    }
                    break;
 
                 default:
                    NL.addNick(LineInterpret.Channel, word1, IRCNICKMODE_REGULAR);
                    if (channel_mm) {
                       // Mark Nicks in CHANNEL_MM as MM clients.
                       H.markAsMMClient(word1);
                    }
//                  Lets prepare stuff to be sent to the NickList UI.
                    strcat(Response, " !");
                    strcat(Response, word1);
                    break;
               }
            }
//          Lets send the line to the NickList UI.
COUT(cout << "NICKLIST: " << Response << endl;)
            XGlobal->IRC_ToUI.putLine(Response);
//            NL.printDebug();
            break;

         case IC_CTCPNORESEND:
            // We mark the FD from SendsInProgress of From.Nick as noresend.
            XGlobal->SendsInProgress.updateFilesDetailNickNoReSend(LineInterpret.From.Nick, true);
            COUT(cout << "FromServerThr: IC_CTCPNORESEND: Marking " << LineInterpret.From.Nick << " to NORESEND." << endl;)
            break;

         case IC_CTCPPINGREPLY:
            {
            // This is in reply to a ping we have issued.
            // We display in Server UI in red, [FromNick PING reply]: .. secs
            cur_time = time(NULL);
            time_t rcvd_time = strtoul(LineInterpret.InfoLine, NULL, 10);
            sprintf(Response, "Server 04[%s PING reply]: %lu secs", LineInterpret.From.Nick, cur_time - rcvd_time);
            XGlobal->IRC_ToUI.putLine(Response);
            }
            break;

         case IC_CTCPTIMEREPLY:
            // This is in reply to a time we have issued.
            // We display in Server UI in red, [FromNick TIME reply]: ...
            sprintf(Response, "Server 04[%s TIME reply]: %s", LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->IRC_ToUI.putLine(Response);
            break;

         case IC_CTCPVERSIONREPLY:
            // This is in reply to a version we have issued.
            // We display in Server UI in red, [FromNick VERSION reply]: ...
            sprintf(Response, "Server 04[%s VERSION reply]: %s", LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->IRC_ToUI.putLine(Response);
            break;

         case IC_CTCPCLIENTINFOREPLY:
            // This is in reply to a clientinfo we have issued.
            // we display in Server UI in red, [FromNick CLIENTINFO reply]: ...
            sprintf(Response, "Server 04[%s CLIENTINFO reply]: %s", LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->IRC_ToUI.putLine(Response);
            break;

         case IC_DCCALLOWREPLY:
            // This is in reply to a dccallow + or - nick issued.
            // We display in server UI in red, [text]
            sprintf(Response, "Server 04[%s]", LineInterpret.InfoLine);
            XGlobal->IRC_ToUI.putLine(Response);
            break;

         case IC_CTCPUPGRADEREPLY:
            // We allow upgrades only from UPGRADE_OP_NICK
            if (strcasecmp(LineInterpret.From.Nick, UPGRADE_OP_NICK)) break;

            // Looks like of all the ctcps for upgrade we sent, someone has
            // replied. Note his reply and nick, if he is OP in chat, as we
            // will possibly get the file from him.
            if (!IS_OP(XGlobal->NickList.getNickMode(CHANNEL_CHAT, LineInterpret.From.Nick))) {
               break;
            }

            // Here if shas match, with what we have, no upgrade will happen.
            XGlobal->lock();
            if (strcasecmp(LineInterpret.InfoLine, XGlobal->Upgrade_SHA) == 0) {
               XGlobal->unlock();
               XGlobal->IRC_ToUI.putLine("*UPGRADE* NOTREQUIRED");
               break;
            }
            else {
               XGlobal->unlock();
               sprintf(Response, "Server 04[%s UPGRADE reply] - Upgrade Required", LineInterpret.From.Nick);
               XGlobal->IRC_ToUI.putLine(Response);
            }

            XGlobal->lock();
            delete [] XGlobal->Upgrade_SHA;
            delete [] XGlobal->Upgrade_Nick;
            XGlobal->Upgrade_SHA = new char[strlen(LineInterpret.InfoLine) + 1];
            strcpy(XGlobal->Upgrade_SHA, LineInterpret.InfoLine);
            XGlobal->Upgrade_Nick = new char[strlen(LineInterpret.From.Nick) + 1];
            strcpy(XGlobal->Upgrade_Nick, LineInterpret.From.Nick);
            XGlobal->Upgrade_Time = time(NULL) + 1800; // within 30 minutes.
            XGlobal->Upgrade_LongIP = LineInterpret.LongIP;
            XGlobal->unlock();

            break;

         case IC_CTCPUPGRADE:
            // We reply to this only if our UpgradeServerEnable is true.
            // and we are OP in both MAIN and CHAT.
            // and we are able to obtain the SHA of the file mentioned
            // in FileName, being present in UPGRADE_DIR
            XGlobal->lock();
            if (XGlobal->UpgradeServerEnable == false) {
               XGlobal->unlock();
               break;
            }
            XGlobal->unlock();
            if (!IS_OP(XGlobal->NickList.getNickMode(CHANNEL_MAIN, Nick)) ||
                !IS_OP(XGlobal->NickList.getNickMode(CHANNEL_CHAT, Nick))) {
               break;
            }
            if (strcasecmp(UPGRADE_OP_NICK, Nick)) break;

            sprintf(Response, "%s%s%s", UPGRADE_DIR, DIR_SEP, LineInterpret.FileName);
            if (getSHAOfFile(Response, word1)) {
            // We are qualified to reply.
               longip = XGlobal->getIRC_IP(NULL);
               XGlobal->resetIRC_IP_Changed();
               sprintf(Response, "NOTICE %s :\001UPGRADE %s %lu %s\001", LineInterpret.From.Nick, word1, longip, LineInterpret.FileName);
               XGlobal->IRC_ToServer.putLine(Response);

               sprintf(Response, "Server 04[%s UPGRADE]", LineInterpret.From.Nick);
               XGlobal->IRC_ToUI.putLine(Response);
               COUT(cout << "UPGRADE: our sha: " << word1 << " Client sha " << LineInterpret.InfoLine << endl;)

               if (strcasecmp(word1, LineInterpret.InfoLine)) {
                  // If the SHAs dont match, upgrade process.
                  // We now need to put this upgrade request in Q, so
                  // that Timer Thread gets at it every 3 minutes, and dccSends
                  sprintf(Response, "%s %lu %s", LineInterpret.From.Nick, LineInterpret.LongIP, LineInterpret.FileName);
                  XGlobal->IRC_ToUpgrade.putLine(Response);
                  COUT(cout << "UPGRADE: sha not matching - queing in IRC_ToUpgrade. Response:  " << Response << endl);
               }
            }
            break;

         case IC_DCC_CHAT:
         case IC_DCC_SEND:
         case IC_DCC_SWARM:
         case IC_DCC_RESUME:
         case IC_DCC_ACCEPT:
         case IC_CTCPFSERV:
         case IC_CTCPPORTCHECK:
         case IC_USERHOST:
         case IC_CTCPFILESHA1:
         case IC_CTCPFILESHA1REPLY:
            COUT(LineInterpret.printDebug();)
            XGlobal->IRC_DCC.putLine(IRC_Line);
            break;


         case IC_CTCPPROPAGATE:
            // Send it to ToTriggerThr() for processing it.
            // It receives it in form: FromNick PROPAGATION PropagatedNick
            //    Sends TotalSends Queues TotalQueues.

            // Lets see if we have this guys IP already.
            longip = XGlobal->NickList.getNickIP(LineInterpret.From.Nick);
            if (longip == IRCNICKIP_UNKNOWN) {
               sprintf(Response, "%s %s %s", LineInterpret.From.Host, LineInterpret.From.Nick, LineInterpret.InfoLine);
            }
            else {
               T.getDottedIpAddressFromLong(longip, DotIP);
               sprintf(Response, "%s %s %s", DotIP, LineInterpret.From.Nick, LineInterpret.InfoLine);
            }
            XGlobal->IRC_ToTrigger.putLine(Response);
            break;

         case IC_PRIVMSG:
//            COUT(cout << "FromServerThr: " << IRC_Line << endl;)
            if (SF.isLineSpam(LineInterpret.InfoLine)) {
COUT(cout << "IC_PRIVMSG: Spam blocked from " << LineInterpret.From.Nick << " :" << LineInterpret.InfoLine << endl;)
               break;
            }
//          Lets Q it to UI correctly.
            if (LineInterpret.To[0] == '#') {
            tempLP = LineInterpret.InfoLine;
//             This is a channel message.
               if ( (strncasecmp(LineInterpret.From.Nick, "[IM]-", 5) == 0) && 
                    ( tempLP.isWordsInLine("||") || 
                      tempLP.isWordsInLine("open") || 
                      tempLP.isWordsInLine("Firewall")
                    ) && 
                    !(tempLP.isWordsInLine("ctcp")) ) {
                  longip = XGlobal->NickList.getNickIP(LineInterpret.From.Nick);
                  if (longip == IRCNICKIP_UNKNOWN) {
                     sprintf(Response, "%s %s %s", LineInterpret.From.Host, LineInterpret.From.Nick, LineInterpret.InfoLine);
                  }
                  else {
                     T.getDottedIpAddressFromLong(longip, DotIP);
                     sprintf(Response, "%s %s %s", DotIP, LineInterpret.From.Nick, LineInterpret.InfoLine);
                  }
                  H.processXDCCListing(Response);
               }
               else if ( (strcasecmp(LineInterpret.To, CHANNEL_MAIN) == 0) &&
                          tempLP.isWordsInLine("FServ") && 
                          tempLP.isWordsInLine("Active") && 
                          tempLP.isWordsInLine("Trigger") && 
                          tempLP.isWordsInLine("ctcp") && 
                          tempLP.isWordsInLine(LineInterpret.From.Nick) ) {
//                Lets send it to the TriggerQ which will check for possible
//                trigger lines. Delayed triggering.
//                Trigger checks are only done for lines in MAIN.
                  longip = XGlobal->NickList.getNickIP(LineInterpret.From.Nick);
                  if (longip == IRCNICKIP_UNKNOWN) {
                     sprintf(Response, "%s %s %s", LineInterpret.From.Host, LineInterpret.From.Nick, LineInterpret.InfoLine);
                  }
                  else {
                     T.getDottedIpAddressFromLong(longip, DotIP);
                     sprintf(Response, "%s %s %s", DotIP, LineInterpret.From.Nick, LineInterpret.InfoLine);
                  }

                  XGlobal->IRC_ToTrigger.putLine(Response);

                  TriggerParse = Response;
                  // Update Client Type in XGlobal's NickList for Non MM ads.
                  // MM clients are only identified by their presence in
                  // CHANNEL_MM
                  if (TriggerParse.getClientType() != IRCNICKCLIENT_MASALAMATE) {
                     XGlobal->NickList.setNickClient(LineInterpret.From.Nick, TriggerParse.getClientType());
                  }
                  else {
                     // This is a MasalaMate Advertisement.
                     // ADSYNC Algo is here.
                     H.calculateFServAdTime(LineInterpret.From.Nick, Nick);
                  }
               }
               char color_coded_nick[512];
               H.generateColorCodedNick(LineInterpret.To, LineInterpret.From.Nick, color_coded_nick);
               sprintf(Response, "%s %s %s", 
                       LineInterpret.To, 
                       color_coded_nick,
                       LineInterpret.InfoLine);
            }
            else {
               sprintf(Response, "Messages <%s> %s", LineInterpret.From.Nick, LineInterpret.InfoLine);
            }
            XGlobal->IRC_ToUI.putLine(Response);
            break;

        case IC_CONNECT:
           char Password[64];

          COUT( LineInterpret.printDebug();)
//         InfoLine has: Welcome ... ... Stealth!~Stealth@c-67-161-27-122.client.comcast.net
//         So we can get our ip right here.
//         Lets update our Nick as seen by server.
//         Also update the Server Name and the Network Name we are 
//         connected to.
           XGlobal->putIRC_Nick(LineInterpret.To);
           XGlobal->putIRC_ProposedNick(LineInterpret.To);
           XGlobal->putIRC_Server(LineInterpret.From.Server);
           strcpy(Nick, LineInterpret.To);

//         Lets set the mode.
#ifdef IRCSUPER
           sprintf(Response, "MODE %s +i-Rv", LineInterpret.To);
#else
           sprintf(Response, "MODE %s +i-Rx", LineInterpret.To);
#endif
           XGlobal->IRC_ToServer.putLine(Response);

//         Lets see if we can get our IP right here.
//         We parse it nice and update XGlobal with dotted ip and long ip.
           Line = LineInterpret.InfoLine;
           Line.setDeLimiter('@');
           parseptr = Line.getWord(2);
           longip = T.getLongFromHostName((char *) parseptr);
           T.getDottedIpAddressFromLong(longip, word1);
           if (longip != 0) {
              XGlobal->putIRC_IP(longip, word1);
              sprintf(Response, "Server 09Connect: HostName: %s DottedIP: %s LongIP: %lu", parseptr, word1, longip);
              XGlobal->IRC_ToUI.putLine(Response);
           }
           else {
//            Issue a USERHOST to get our ip as seen by server side.
              sprintf(Response, "USERHOST %s", LineInterpret.To);
              XGlobal->IRC_ToServer.putLine(Response);
//            We will get a IC_USERHOST response back which will set our IP.
           }

//         Check if we need to identify to the nick.
           XGlobal->getIRC_Password(Password);
           if (strlen(Password)) {
              sprintf(Response, "PRIVMSG NickServ :identify %s", Password);
              XGlobal->IRC_ToServer.putLine(Response);
           }

//         Attempt to join the channels.
           joinChannels(XGlobal);
           break;

        case IC_NICKINUSE:
           COUT(LineInterpret.printDebug();)
           // We need to generate a new nick as this is already in use.
           // We generate only if Global Nick and Proposed Nick are same.
           char local_nick[64], local_proposednick[64], local_password[64];
           char generate_nick[64];
           XGlobal->getIRC_Nick(local_nick);
           XGlobal->getIRC_ProposedNick(local_proposednick);
           XGlobal->getIRC_Password(local_password);
           XGlobal->resetIRC_Nick_Changed();
           if (XGlobal->isIRC_Nick_Changed(local_proposednick) == false) {
              // Global Nick and Proposed Nick are same
 
              generateNick(generate_nick);
              XGlobal->putIRC_ProposedNick(generate_nick);
              sprintf(Response, "NICK %s", generate_nick);
              XGlobal->IRC_ToServerNow.putLine(Response);

              // Lets put it up in the UI.
              sprintf(Response, "Server 15IRC: Nick in Use. Retrying as %s", generate_nick);
              XGlobal->IRC_ToUI.putLine(Response);

              if (strlen(local_password)) {
                 // There is a password. So lets try to recover.
                 // Now we try to recover the nick.
                 sleep(5); // Hope we get registered by then.
                 sprintf(Response, "PRIVMSG NickServ :GHOST %s %s", local_nick, local_password);
                 XGlobal->IRC_ToServer.putLine(Response);

                 // Now try a nick change.
                 // Ideally we would wait for the GHOST response, and then
                 // change the nick there.
                 sprintf(Response, "NICK %s", local_nick);
                 XGlobal->IRC_ToServer.putLine(Response);

                 // Hope we get registered with this new nick by then.
                 sleep(5);
                 // Not do the MODE change.
#ifdef IRCSUPER
                 sprintf(Response, "MODE %s +i-Rv", local_nick);
#else
                 sprintf(Response, "MODE %s +i-Rx", local_nick);
#endif
                 XGlobal->IRC_ToServer.putLine(Response);
              }
           }
           else {
              sprintf(Response, "Server 15IRC: Nick in Use. Keeping old Nick: %s", Nick);
              XGlobal->IRC_ToUI.putLine(Response);
           }
           break;

        case IC_AWAY:
           // We just discard this.
           break;

        case IC_LIST:
           // InfoLine contains the possible nick whose !list is wanted.
           Response[0] = '\0';
           if (LineInterpret.To[0] == '#') {
              char color_coded_nick[512];
              H.generateColorCodedNick(LineInterpret.To, LineInterpret.From.Nick, color_coded_nick);
              sprintf(Response, "%s %s !list %s", LineInterpret.To, color_coded_nick, LineInterpret.InfoLine);
           }
           if (Response[0] ==  '\0') {
              sprintf(Response, "%s <%s> !list %s", LineInterpret.To, LineInterpret.From.Nick, LineInterpret.InfoLine);
           }
           XGlobal->IRC_ToUI.putLine(Response);

           Line = LineInterpret.InfoLine;
           if (Line.getWordCount() > 1) break;
           parseptr = Line.getWord(1);
           if (strcasecmp(parseptr, Nick) != 0) break;

           // Lets send this guy our Trigger. if he asked for it in CHANNEL_MAIN
           // or as a private message to me
           if ( (strcasecmp(LineInterpret.To, CHANNEL_MAIN) == 0) ||
                (strcasecmp(LineInterpret.To, Nick) == 0) ) {
              H.generateFServAd(IRC_Line);
              sprintf(Response, "NOTICE %s :%s", LineInterpret.From.Nick, IRC_Line);
              H.sendLineToNick(LineInterpret.From.Nick, Response);
           }
           break;

        case IC_FINDSWARM:
           // Infoline contains the @stream text to be searched for.
           Response[0] = '\0';
           if (LineInterpret.To[0] == '#') {
              char color_coded_nick[512];
              H.generateColorCodedNick(LineInterpret.To, LineInterpret.From.Nick, color_coded_nick);
              sprintf(Response, "%s %s @swarm %s", LineInterpret.To, color_coded_nick, LineInterpret.InfoLine);
           }
           if (Response[0] ==  '\0') {
              sprintf(Response, "%s <%s> @swarm %s", LineInterpret.To, LineInterpret.From.Nick, LineInterpret.InfoLine);
           }
           XGlobal->IRC_ToUI.putLine(Response);

           // Respond if this @find was in CHANNEL_MAIN.
           if (strcasecmp(LineInterpret.To, CHANNEL_MAIN) == 0) {
              for (i = 0; i < SWARM_MAX_FILES; i++) {
                 if (XGlobal->Swarm[i].isBeingUsed() && 
                     (XGlobal->Swarm[i].isSwarmServer() || XGlobal->Swarm[i].haveGreatestFileSizeInSwarm()) &&
                     (strcasestr((char *)XGlobal->Swarm[i].getSwarmFileName(), LineInterpret.InfoLine)) ) {
                    // Lets send out the @STREAM Hit.
                    sprintf(IRC_Line, "09,01[Swarm] 07,01FileSize: 08,01%lu 07,01FileName: 08,01%s 09,01- To join the swarm, type: 00,01/swarm %s %s", 
                            XGlobal->Swarm[i].getSwarmFileSize(),
                            XGlobal->Swarm[i].getSwarmFileName(),
                            Nick,
                            XGlobal->Swarm[i].getSwarmFileName());
                    sprintf(Response, "PRIVMSG %s :%s", LineInterpret.From.Nick, IRC_Line);
                    H.sendLineToNick(LineInterpret.From.Nick, Response);
                 }
              }
           }
           break;

        case IC_FIND:
           // InfoLine contains the @find text to be searched for.
           Response[0] = '\0';
           if (LineInterpret.To[0] == '#') {
              char color_coded_nick[512];
              H.generateColorCodedNick(LineInterpret.To, LineInterpret.From.Nick, color_coded_nick);
              sprintf(Response, "%s %s @find %s", LineInterpret.To, color_coded_nick, LineInterpret.InfoLine);
           }
           if (Response[0] ==  '\0') {
              sprintf(Response, "%s <%s> @find %s", LineInterpret.To, LineInterpret.From.Nick, LineInterpret.InfoLine);
           }
           XGlobal->IRC_ToUI.putLine(Response);

           // Respond if this @find was in CHANNEL_MAIN.
           if (strcasecmp(LineInterpret.To, CHANNEL_MAIN) == 0) {
              H.generateFindHit(LineInterpret.InfoLine, IRC_Line);
              if (IRC_Line[0] == '\0') break;

              // Lets send out the @FIND Hit.
              sprintf(Response, "PRIVMSG %s :%s", LineInterpret.From.Nick, IRC_Line);
              H.sendLineToNick(LineInterpret.From.Nick, Response);
           }
           break;

        case IC_NOTICE:
           COUT(LineInterpret.printDebug();)
           // We check for trigger or bot ad and process this too.
           // Just like IC_PRIVMSG
           tempLP = LineInterpret.InfoLine;
           if ( LineInterpret.From.Nick && 
                (strncasecmp(LineInterpret.From.Nick, "[IM]-", 5) == 0) && 
                ( tempLP.isWordsInLine("||") || 
                  tempLP.isWordsInLine("open") ||
                  tempLP.isWordsInLine("Firewall")
                ) && 
                !(tempLP.isWordsInLine("ctcp")) ) {
               longip = XGlobal->NickList.getNickIP(LineInterpret.From.Nick);
               if (longip == IRCNICKIP_UNKNOWN) {
                  sprintf(Response, "%s %s %s", LineInterpret.From.Host, LineInterpret.From.Nick, LineInterpret.InfoLine);
               }
               else {
                  T.getDottedIpAddressFromLong(longip, DotIP);
                  sprintf(Response, "%s %s %s", DotIP, LineInterpret.From.Nick, LineInterpret.InfoLine);
               }
               H.processXDCCListing(Response);
            }
            else if ( LineInterpret.From.Nick && (tempLP.isWordsInLine("FServ") && tempLP.isWordsInLine("Active") && tempLP.isWordsInLine("Trigger") && tempLP.isWordsInLine("ctcp") && tempLP.isWordsInLine(LineInterpret.From.Nick)) ) {
               // Lets send it to the TriggerQ which will check for possible
               // trigger lines. No delays before issuing trigger.
               longip = XGlobal->NickList.getNickIP(LineInterpret.From.Nick);
               if (longip == IRCNICKIP_UNKNOWN) {
                  sprintf(Response, "%s %s %s", LineInterpret.From.Host, LineInterpret.From.Nick, LineInterpret.InfoLine);
               }
               else {
                  T.getDottedIpAddressFromLong(longip, DotIP);
                  sprintf(Response, "%s %s %s", DotIP, LineInterpret.From.Nick, LineInterpret.InfoLine);
               }
               XGlobal->IRC_ToTriggerNow.putLine(Response);
            }
            else if (LineInterpret.From.Nick &&
                    tempLP.isWordsInLine("please") &&
                    tempLP.isWordsInLine("make") &&
                    tempLP.isWordsInLine("sure") &&
                    tempLP.isWordsInLine("your") &&
                    tempLP.isWordsInLine("DCC") &&
                    tempLP.isWordsInLine("Server") &&
                    tempLP.isWordsInLine("is") &&
                    tempLP.isWordsInLine("active") ) {
               // Mark this nick as firewalled.
               XGlobal->NickList.setNickFirewall(LineInterpret.From.Nick, IRCNICKFW_YES);
               COUT(cout << LineInterpret.From.Nick << " marked as IRCNICKFW_YES" << endl;)
            }

            if (SF.isLineSpam(LineInterpret.InfoLine)) {
COUT(cout << "IC_NOTICE: Spam blocked: " << LineInterpret.InfoLine << endl;)
               break;
            }
//          Lets Q it to UI correctly.
            if (LineInterpret.From.Server != NULL) {
               sprintf(Response, "Server %s %s %s", LineInterpret.From.Server, LineInterpret.To, LineInterpret.InfoLine);
               XGlobal->IRC_ToUI.putLine(Response);
            }
            else {
               FilesDetail *FD;
               long q_pos;
//             USER Notices.
//             This is where we see if its a NOTICE from a Nick which
//             is in the DwnldWaiting Structure. In that case we update
//             the UI accordingly.
               FD = XGlobal->DwnldWaiting.getFilesDetailListOfNick(LineInterpret.From.Nick);
               if (FD != NULL) {
                  // Need to update the "Waiting" UI appropriately.
                  // The responses we are looking at are from the XDCCs.
// *** All Slots Full, Added you to the main queue in position 6. To Remove youself at a later time type "/msg [IM]-Art0148 xdcc remove".
// *** All Slots Full, Adding your file to queue slot 1. To Remove youself at a later time type "/msg [IM]-Nikkar xdcc remove".
// You have been queued for 0 hr 6 min, currently in main queue position 6 of 9.  Estimated remaining time is 25 hr 23 min or more.  To remove yourself type "/msg [IM]-Art0148 xdcc remove".
// *** All Slots Full, Denied, You already have that item queued.
// *** Sorry Pack ... => delete from waiting.
// if it has "denied" ... and not "queued" => delete from waiting.
// if it has "Try again later" ... => delete from waiting.
                  LineP = LineInterpret.InfoLine;
                  if (LineP.isWordsInLine("Sorry Pack") || 
                      (LineP.isWordsInLine("denied") && !LineP.isWordsInLine("queued"))||
                      LineP.isWordsInLine("Try again later") ) {
                      XGlobal->DwnldWaiting.delFilesDetailNickFile(FD->Nick, FD->FileName);
                      sprintf(Response, "Server 04Download: Nick %s rejected attempt to get file %s.", FD->Nick, FD->FileName);
                  }
                  else {
                  int q_wordindex = 0;
//                   Lets grab q information.
                     if (LineP.isWordsInLine("You have been queued")) {
                        // q num is word after "position"
                        q_wordindex = LineP.getIndexOfWordInLine("position");
                        if (q_wordindex != 0) {
                           q_wordindex++;
                        }
                     }
                     else if (LineP.isWordsInLine("Added you to the")) {
                        // q num is word after "position"
                        q_wordindex = LineP.getIndexOfWordInLine("position");
                        if (q_wordindex != 0) {
                           q_wordindex++;
                        }
                     }
                     else if (LineP.isWordsInLine("Adding your file to")) {
                        // q num is word after "slot"
                        q_wordindex = LineP.getIndexOfWordInLine("slot");
                        if (q_wordindex != 0) {
                           q_wordindex++;
                        }
                     }
                     if (q_wordindex != 0) {
                        parseptr = LineP.getWord(q_wordindex);
                        q_pos = strtol(parseptr, NULL, 10);
                     }
                     else {
                        q_pos = -1;
                     }
                     if (q_pos >= 0) {
                        XGlobal->DwnldWaiting.updateFilesDetailNickFileQ(FD->Nick, FD->FileName, q_pos);
                        sprintf(Response, "Server 09Download: Nick %s File: %s Queue: %d", FD->Nick, FD->FileName, q_pos);
                        // Do not Remove from DwnldWaiting even if q_pos = 0
                     }
                     else {
                        // This message is not related to send/q info
                        sprintf(Response, "Messages <%s> %s", LineInterpret.From.Nick, LineInterpret.InfoLine);
                     }
                  }
               }
               else {
                  sprintf(Response, "Messages <%s> %s", LineInterpret.From.Nick, LineInterpret.InfoLine);
               }
               XGlobal->IRC_ToUI.putLine(Response);
               XGlobal->DwnldWaiting.freeFilesDetailList(FD);

            }
            break;

        case IC_ACTION:
           COUT(LineInterpret.printDebug();)
            if (SF.isLineSpam(LineInterpret.InfoLine)) {
COUT(cout << "IC_ACTION: Spam blocked: " << LineInterpret.InfoLine << endl;)
               break;
            }
//          Lets Q it to UI correctly.
            sprintf(Response, "%s 06* %s %s", LineInterpret.To, LineInterpret.From.Nick, LineInterpret.InfoLine);
            XGlobal->IRC_ToUI.putLine(Response);
            break;

        case IC_CLIENTINFO:
          {
          char *osversion_str;
          int upnp_state;
          char *upnp_str;
          char *ext_ip_str;
          char *int_ip_str;
          char *fw_str;
          int fw_state;
          long fserv_all_sends, fserv_all_queues, fserv_user_sends;
          long fserv_user_queues;
          size_t fserv_small_filesize, fserv_overall_mincps;
          size_t cap_user_upload, cap_all_upload;
          size_t cap_user_download, cap_all_download;
          
           XGlobal->lock();
           fserv_all_sends = XGlobal->FServSendsOverall;
           fserv_all_queues = XGlobal->FServQueuesOverall;
           fserv_user_sends = XGlobal->FServSendsUser;
           fserv_user_queues = XGlobal->FServQueuesUser;
           fserv_small_filesize = XGlobal->FServSmallFileSize;
           fserv_overall_mincps = XGlobal->OverallMinUploadBPS;
           cap_user_upload = XGlobal->PerTransferMaxUploadBPS;
           cap_all_upload = XGlobal->OverallMaxUploadBPS;
           cap_user_download = XGlobal->PerTransferMaxDownloadBPS;
           cap_all_download = XGlobal->OverallMaxDownloadBPS;
           XGlobal->unlock();

           osversion_str = new char[128];
           upnp_str = new char[64];
           ext_ip_str = new char[32];
           int_ip_str = new char[32];
           fw_str = new char[32];
           // Respond to the Clientinfo. We send the below information:
           // OS version, Win/Unix version, Upnp, internal/externalip,
           // firewall status
           // Lets Q it to UI correctly.
           sprintf(Response, "Server 04[%s CLIENTINFO]", LineInterpret.From.Nick);
           XGlobal->IRC_ToUI.putLine(Response);
           // OS Version String from getOSVersionString()
           // Upnp String from UpnpState
           // Internal IP: from DottedInternalIP
           // External IP: from getIRC_IP()
           // Firewall status: from FireWallState
           getOSVersionString(osversion_str);
           XGlobal->getIRC_IP(ext_ip_str);
           XGlobal->resetIRC_IP_Changed();
           if (strlen(ext_ip_str) == 0) {
              strcpy(ext_ip_str, "UNKNOWN");
           }
           XGlobal->lock();
           fw_state = XGlobal->FireWallState;
           upnp_state = XGlobal->UpnpStatus;
           strcpy(int_ip_str, XGlobal->DottedInternalIP);
           XGlobal->unlock();
           switch (fw_state) {
              case 0:
              case 1:
              case 2:
              case 3:
              case 4:
                 sprintf(fw_str, "%d %c", 100 - fw_state * 20, '%');
                 break;

              case 5:
                 strcpy(fw_str, "NO");
                 break;
           }
           switch (upnp_state) {
              case 0:
                strcpy(upnp_str, "Not Detected");
                break;
 
              case 1:
                strcpy(upnp_str, "Detected");
                break;

              case 2:
                strcpy(upnp_str, "Detected and Port Forwarded");
                break;
           }
           sprintf(Response, "NOTICE %s :\001CLIENTINFO "
                 "11OS: 09%s "
                 "11ROUTER UPNP: 09%s "
                 "11Internal IP: 09%s " 
                 "11External IP: 09%s " 
                 "11FireWalled: 09%s " 
                 "11FileServer: 09S E%lu A%lu | Q E%lu A%lu "
                 "| Sml %lu MinCPS %lu "
                 "11CAPS: 09U E%lu A%lu | D E%lu A%lu"
                 "\001", 
               LineInterpret.From.Nick,
               osversion_str,
               upnp_str,
               int_ip_str,
               ext_ip_str,
               fw_str,
               fserv_user_sends, fserv_all_sends, fserv_user_queues,
               fserv_all_queues, fserv_small_filesize, fserv_overall_mincps,
               cap_user_upload, cap_all_upload, 
               cap_user_download, cap_all_download
              );
               
           H.sendLineToNick(LineInterpret.From.Nick, Response);
           delete [] osversion_str;
           delete [] upnp_str;
           delete [] int_ip_str;
           delete [] ext_ip_str;
           delete [] fw_str;
          }
           break;

        case IC_VERSION:
           sprintf(Response, "Server 04[%s VERSION]", LineInterpret.From.Nick);
           XGlobal->IRC_ToUI.putLine(Response);
           sprintf(Response, "NOTICE %s :\001VERSION %s %s %s\001", LineInterpret.From.Nick, CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING);
           H.sendLineToNick(LineInterpret.From.Nick, Response);
#if 0
           sprintf(Response, "NOTICE %s :\001VERSION Get it from: %s\001", LineInterpret.From.Nick, CLIENT_HTTP_LINK);
           H.sendLineToNick(LineInterpret.From.Nick, Response);
#endif

           break;

        case IC_CTCPPING:
           sprintf(Response, "Server 04[%s PING]", LineInterpret.From.Nick);
           XGlobal->IRC_ToUI.putLine(Response);
           sprintf(Response, "NOTICE %s :\001PING %s\001", LineInterpret.From.Nick, LineInterpret.InfoLine);
           H.sendLineToNick(LineInterpret.From.Nick, Response);
           break;

        case IC_CTCPTIME:
           sprintf(Response, "Server 04[%s TIME]", LineInterpret.From.Nick);
           XGlobal->IRC_ToUI.putLine(Response);
           cur_time = time(NULL);
           strcpy(word1, ctime(&cur_time));
           word1[strlen(word1) - 1] = '\0';
           sprintf(Response, "NOTICE %s :\001TIME %s\001", LineInterpret.From.Nick, word1);
           H.sendLineToNick(LineInterpret.From.Nick, Response);
           break;

        case IC_TOPIC:
           COUT(LineInterpret.printDebug();)
           sprintf(Response, "%s 09* Topic is '00%s09'", LineInterpret.Channel, LineInterpret.InfoLine);
           XGlobal->IRC_ToUI.putLine(Response);
           break;

        case IC_LIST_START:
           sprintf(Response, "Messages * 09Channel Name     | Count | Description");
           XGlobal->IRC_ToUI.putLine(Response);
           break;

        case IC_LIST_ENTRY:
           sprintf(Response, "Messages * 09%-16.16s | %-5d | %s", LineInterpret.Channel, LineInterpret.ListChannelCount, LineInterpret.InfoLine);
           XGlobal->IRC_ToUI.putLine(Response);
           break;

        case IC_LIST_END:
           sprintf(Response, "Messages * 09END CHANNEL LISTING");
           XGlobal->IRC_ToUI.putLine(Response);
           break;

        case IC_BANLIST:
           sprintf(Response, "%s * 08%s", LineInterpret.Channel, LineInterpret.InfoLine);
           XGlobal->IRC_ToUI.putLine(Response);
           break;

        case IC_BANLIST_END:
           sprintf(Response, "%s * 08%s", LineInterpret.Channel, LineInterpret.InfoLine);
           XGlobal->IRC_ToUI.putLine(Response);
           break;

        case IC_NAMES_END:
           // We dont want it printed anywhere.

           // As we got the end of the names list, lets make sure the MM
           // Clients are marked.
           {
           char *mm_nick = new char[128];
           for (int cur_index = 1; ; cur_index++) {
              if (XGlobal->NickList.getNickInChannelAtIndex(CHANNEL_MM, cur_index, mm_nick)) {
                 H.markAsMMClient(mm_nick);
              }
              else {
                 break;
              }
           }

           delete [] mm_nick;
           }
           break;

        case IC_TOPIC_END:
           // We dont want it printed anywhere.
           break;

        case IC_TOPIC_CHANGE:
           COUT(LineInterpret.printDebug();)
           sprintf(Response, "%s 09* %s changes topic to '00%s09'", LineInterpret.Channel, LineInterpret.From.Nick, LineInterpret.InfoLine);
           XGlobal->IRC_ToUI.putLine(Response);
           break;

        case IC_CTCPLAG:
           sprintf(Response, "Server 04[%s LAG]", LineInterpret.From.Nick);
           XGlobal->IRC_ToUI.putLine(Response);
           break;

        default:
           COUT(cout << "FromServerThr: Discarded " << IRC_Line << endl;)
//         Lets Q it to UI correctly.
           if (LineInterpret.From.Type == ILFT_SERVERMESSAGE) {
              sprintf(Response, "Server %s", LineInterpret.InfoLine);
           }
           else {
              sprintf(Response, "Server UNPROCESSED: %s", IRC_Line);
           }
           XGlobal->IRC_ToUI.putLine(Response);
           break;
      }
   }
   delete [] word1;
   COUT(cout << "fromServerThr: Quitting" << endl;)
// We dont call Exit thread as we want to go out of scope => destrcutors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}

void joinChannels(XChange *XGlobal) {
IRCChannelList CL;
time_t CurrentTime = time(NULL);
char Response[256];
bool displayCL = false;
const char *channel_key;

   TRACE();
   CL = XGlobal->getIRC_CL();
   XGlobal->resetIRC_CL_Changed();
   for (int i = 1; i <= CL.getChannelCount(); i++) {
      if ( (CL.isJoined(i) == false) && 
           ((CurrentTime - CL.getJoinAttemptTime(i)) > 60) ) {
         channel_key = CL.getChannelKey(i);
         if (channel_key) {
            sprintf(Response, "JOIN %s %s", CL.getChannel(i), channel_key);
         }
         else {
            sprintf(Response, "JOIN %s", CL.getChannel(i));
         }
         XGlobal->IRC_ToServer.putLine(Response);
         CL.setJoinAttemptTime(i);
         XGlobal->putIRC_CL(CL);
         displayCL = true;
      }
   }
   if (displayCL) { 
      COUT(CL.printDebug();)
   }
}
