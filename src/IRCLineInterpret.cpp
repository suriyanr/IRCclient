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

#include <iostream>
using namespace std;


#include "IRCLineInterpret.hpp"
#include "IRCLineInterpretGlobal.hpp"
#include "LineParse.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

/*
  To add a new Command update IRCLineInterpretGlobal.hpp. Then update
  IRCLineInterpret.hpp with its enumerated type. And then take care
  of that Command Type in here.
*/

// Constructor class
IRCLineInterpret::IRCLineInterpret() {
   TRACE();
   init();
}

void IRCLineInterpret::init() {
   TRACE();
   From.Type = ILFT_UNKNOWN;
   From.Nick = NULL;
   From.UserName = NULL;
   From.Host = NULL;
   From.Server = NULL;
   Command = IC_UNKNOWN;
   CommandStr = NULL;
   To = NULL;
   Channel = NULL;
   KickedNick = NULL;
   InfoLine = NULL;
   FileName = NULL;
   LongIP = 0;
   Port = 0;
   FileSize = 0;
   ResumeSize = 0;
}

// Destructor class
IRCLineInterpret::~IRCLineInterpret() {
   TRACE();
   freeAll();
}

void IRCLineInterpret::freeAll() {
   TRACE();
   delete [] From.Nick;
   delete [] From.UserName;
   delete [] From.Host;
   delete [] From.Server;
   delete [] CommandStr;
   delete [] To;
   delete [] Channel;
   delete [] InfoLine;
   delete [] FileName;
   delete [] KickedNick;
}

// Assignment with character string.
IRCLineInterpret &IRCLineInterpret::operator=(char *line) {
LineParse LineP;
char *tmpword;
char *word1;
const char *parseptr; // For return values from LineParse objects.
int command_index;
char *info_line;
int i;

   TRACE();
   freeAll();
   init();
   if (line == NULL) return(*this);

// We have the variables as big as line. So cant overflow.
   tmpword = new char[strlen(line) + 1];
   word1 = new char[strlen(line) + 1];
   info_line = new char[strlen(line) + 1];

   LineP = line;
   parseptr = LineP.getWord(1);
// Lets inspect first character in first word of line.
   if (parseptr[0] == ':') {
      strcpy(info_line, &parseptr[1]);
      LineP = info_line;
//    This is [:PREFIX], hence word1 is one of:
//    -> :flowerpower.fl.us.rc0.net
//    -> :Art-Test-Art
//    -> :Sur4802!~noemail@c-67...comcast.net
      if ( LineP.isCharInLine('@') && LineP.isCharInLine('!') ) {
//       -> :Sur4802!~noemail@c-67...comcast.net
         From.Type = ILFT_USERMESSAGE;
         
         LineP.setDeLimiter('!');
         parseptr = LineP.getWord(1);
         From.Nick = new char[strlen(parseptr) + 1];
         strcpy(From.Nick, parseptr);
         parseptr = LineP.getWord(2);
         strcpy(tmpword, parseptr);
         LineP = tmpword;
         LineP.setDeLimiter('@');
         parseptr = LineP.getWord(1);
         From.UserName = new char[strlen(parseptr) + 1];
         strcpy(From.UserName, parseptr);
         parseptr = LineP.getWord(2);
         From.Host =  new char[strlen(parseptr) + 1];
         strcpy(From.Host, parseptr);
      }
      else if (LineP.isCharInLine('.')) {
//       -> :flowerpower.fl.us.rc0.net
         From.Type = ILFT_SERVERMESSAGE;
         parseptr = LineP.getWord(1);
         From.Server = new char[strlen(parseptr) + 1];
         strcpy(From.Server, parseptr);
      }
      else {
//       -> :Art-Test-Art
         From.Type = ILFT_NICKMESSAGE;
         parseptr = LineP.getWord(1);
         From.Nick = new char[strlen(parseptr) + 1];
         strcpy(From.Nick, parseptr);
      }
      command_index = 2;
   }
   else {
//    No :PREFIX, starts with COMMAND. => PING, PONG, ERROR, NOTICE
      From.Type = ILFT_COMMANDMESSAGE;
      command_index = 1;
   }

// Lets analyse the command.

   LineP = line;
//  Grab the command string = first or second word.
   parseptr = LineP.getWord(command_index);
   CommandStr = new char[strlen(parseptr) + 1];
   strcpy(CommandStr, parseptr);

// Grab the word next to the Command, mostly the To String.
   command_index++;
   parseptr = LineP.getWord(command_index);
   To = new char[strlen(parseptr) + 1];
   strcpy(To, parseptr);

// By Default assume all text after the To string, is the info line.
   command_index++;
   parseptr = LineP.getWordRange(command_index, 0);
   InfoLine = new char[strlen(parseptr) + 1];
// Lets remove the : if present at start of line.
   if (parseptr[0] == ':') {
      strcpy(InfoLine, &parseptr[1]);
   }
   else {
      strcpy(InfoLine, parseptr);
   }

   // We need to differentiate between IC_NOTICE and IC_SERVER_NOTICE
   if ( (From.Type == ILFT_COMMANDMESSAGE) &&
        (strcasecmp(CommandStr, "NOTICE") == 0) ) {
      // This is IC_SERVER_NOTICE, hence change CommandStr to "SERVERNOTICE"
      delete [] CommandStr;
      CommandStr = new char[13];
      strcpy(CommandStr, "SERVERNOTICE");
   }

//  Lets parse the CommandStr in case its a CTCP
   LineP = CommandStr;
   if (strcasecmp(CommandStr, "PRIVMSG") == 0) {
//    Its a PRIVMSG so could be a potential CTCP. Let us take care of that.
//    Also a potential !LIST or @FIND or @SWARM
      LineP = line;
      parseptr = LineP.getWord(4);
      strcpy(tmpword, parseptr);
      LineP = tmpword;

      if (LineP.isWordsInLine(":!LIST")) {
         // Lets check if it has exactly one more word next to it.
         LineP = line;
         if (LineP.getWordCount() == 5) {
            delete [] CommandStr;
            CommandStr = new char[5];
            strcpy(CommandStr, "LIST");
         }
         LineP = CommandStr; // LineP properly initialised.
      }
      else if (LineP.isWordsInLine(":@FIND")) {
         // Lets check if it has at least one more word next to it.
         LineP = line;
         if (LineP.getWordCount() >= 5) {
            delete [] CommandStr;
            CommandStr = new char[5];
            strcpy(CommandStr, "FIND");
         }
         LineP = CommandStr; // LineP properly initialised.
      }
      else if (LineP.isWordsInLine(":@SWARM")) {
         // Lets check if it has at least one more word next to it.
         LineP = line;
         if (LineP.getWordCount() >= 5) {
            delete [] CommandStr;
            CommandStr = new char[11];
            strcpy(CommandStr, "FINDSWARM");
         }
         LineP = CommandStr; // LineP properly initialised.
      }
      else if (LineP.isWordsInLine(":\001DCC")) {
//       Its a CTCP DCC
         LineP = line;
         parseptr = LineP.getWord(5);
         delete [] CommandStr;
         CommandStr = new char[strlen(parseptr) + 1];
         strcpy(CommandStr, parseptr);
         LineP = CommandStr;
//       We set 5th word as the CommandStr.
      }
      else if (tmpword[1] == '\001') { // Cant use parseptr here.
//       Its a non DCC CTCP.
         delete [] CommandStr;
         CommandStr = new char[strlen(tmpword) - 1];
         strcpy(CommandStr, &tmpword[2]);
         if (CommandStr[strlen(CommandStr) - 1] == '\001') {
//          Remove the \001 at the end of the CTCP if present.
            CommandStr[strlen(CommandStr) - 1] = '\0';
         }
//       HACK to change PING to CTCPPING to avoid confusion.
         if (strcasecmp(CommandStr, "PING") == 0) {
            delete [] CommandStr;
            CommandStr = new char[9];
            strcpy(CommandStr, "CTCPPING");
         }
         else if (strcasecmp(CommandStr, "MASALA") == 0) {
//       This is the FSERV CTCP. Trigger is -> Masala of <Nick> longip
            delete [] CommandStr;
            CommandStr = new char[10];
            strcpy(CommandStr, "CTCPFSERV");
         }
         LineP = CommandStr;
      }
      else {
//       This is a non CTCP PRIVMSG.
         LineP = CommandStr;
      }
   }
   else if (strcasecmp(CommandStr, "NOTICE") == 0) {
      // Its a NOTICE so could be a potential CTCP PING reply. Let us take 
      // care of that. or a CTCP VERSION reply, or a CTCP TIME reply.
      // Check for Infoline containing: \001PING timestamp\001 (for ping)
      // Check for Infoline containing: \001VERSION ... \001 (for version)
      // Check for Infoline containing: \001TIME ... \001 (for time)
      // Check for Infoline containing: \001CLIENTINFO ...\001 (for clientinfo)
      // Check for Infoline containing: \001FILESHA1 ... \001 (for filesha1)
      // Check for Infoline containing: \001UPGRADE .. \001 (for upgrade)
      if (strncasecmp(InfoLine, "\001PING ", 6) == 0) {
         // This is a reply to a ping we have issued.
         delete [] CommandStr;
         CommandStr = new char[11];
         strcpy(CommandStr, "NOTICEPING");
         // Make InfoLine have the timestamp alone.
         if (strlen(InfoLine) > 7) {
            strcpy(tmpword, &InfoLine[6]);
            if (tmpword[strlen(tmpword) - 1] == '\001') {
               // Remove the \001 at the end.
               tmpword[strlen(tmpword) - 1] = '\0';
               strcpy(InfoLine, tmpword);
            }
         }
         LineP = CommandStr;
      }
      else if (strncasecmp(InfoLine, "\001VERSION ", 9) == 0) {
         // This is a reply to a version we have issued.
         delete [] CommandStr;
         CommandStr = new char[14];
         strcpy(CommandStr, "NOTICEVERSION");
         // Make InfoLine have the version information alone.
         if (strlen(InfoLine) > 10) {
            strcpy(tmpword, &InfoLine[9]);
            if (tmpword[strlen(tmpword) - 1] == '\001') {
               // Remove the \001 at the end.
               tmpword[strlen(tmpword) - 1] = '\0';
               strcpy(InfoLine, tmpword);
            }
         }
         LineP = CommandStr;
      }
      else if (strncasecmp(InfoLine, "\001TIME ", 6) == 0) {
         // This is a reply to a time we have issued.
         delete [] CommandStr;
         CommandStr = new char[11];
         strcpy(CommandStr, "NOTICETIME");
         // Make InfoLine have the time information alone.
         if (strlen(InfoLine) > 7) {
            strcpy(tmpword, &InfoLine[6]);
            if (tmpword[strlen(tmpword) - 1] == '\001') {
               // Remove the \001 at the end.
               tmpword[strlen(tmpword) - 1] = '\0';
               strcpy(InfoLine, tmpword);
            }
         }
         LineP = CommandStr;
      }
      else if (strncasecmp(InfoLine, "\001CLIENTINFO ", 12) == 0) {
         // This is a reply to a clientinfo we have issued.
         delete [] CommandStr;
         CommandStr = new char[17];
         strcpy(CommandStr, "NOTICECLIENTINFO");
         // Make InfoLine have the time information alone.
         if (strlen(InfoLine) > 13) {
            strcpy(tmpword, &InfoLine[12]);
            if (tmpword[strlen(tmpword) - 1] == '\001') {
               // Remove the \001 at the end.
               tmpword[strlen(tmpword) - 1] = '\0';
               strcpy(InfoLine, tmpword);
            }
         }
         LineP = CommandStr;
      }
      else if (strncasecmp(InfoLine, "\001FILESHA1 ", 10) == 0) {
         // This is a reply to a filesha1 we have issued.
         delete [] CommandStr;
         CommandStr = new char[15];
         strcpy(CommandStr, "NOTICEFILESHA1");
         // InfoLine contains: FILESHA1 file_size file_sha1_str file_name
         // We fill up FileSize, FileName and SHA (in InfoLine)
         if (InfoLine[strlen(InfoLine) - 1] == '\001') {
            InfoLine[strlen(InfoLine) - 1] = '\0';
         }
         LineP = InfoLine;
         parseptr = LineP.getWord(2);
         FileSize = strtoul(parseptr, NULL, 10);
         parseptr = LineP.getWord(3);
         delete [] InfoLine;
         InfoLine = new char[strlen(parseptr) + 1];
         strcpy(InfoLine, parseptr);
         parseptr = LineP.getWordRange(4, 0);
         FileName = new char[strlen(parseptr) + 1];
         strcpy(FileName, parseptr);

         LineP = CommandStr;
      }
      else if (strncasecmp(InfoLine, "\001UPGRADE ", 9) == 0) {
         // This is a reply to a upgrade we have issued.
         delete [] CommandStr;
         CommandStr = new char[14];
         strcpy(CommandStr, "NOTICEUPGRADE");
         // InfoLine contains: UPGRADE sha longip filename
         // We fill up FileName and SHA (in InfoLine)
         if (InfoLine[strlen(InfoLine) - 1] == '\001') {
            InfoLine[strlen(InfoLine) - 1] = '\0';
         }
         LineP = InfoLine;
         parseptr = LineP.getWordRange(4, 0);
         FileName = new char[strlen(parseptr) + 1];
         strcpy(FileName, parseptr);

         parseptr = LineP.getWord(3);
         LongIP = strtoul(parseptr, NULL, 10);

         parseptr = LineP.getWord(2);
         delete [] InfoLine;
         InfoLine = new char[strlen(parseptr) + 1];
         strcpy(InfoLine, parseptr);
         // SHA in InfoLine.
 
         LineP = CommandStr;
      }
   }

// Here all CTCP/DCC's are taken care of.
// full_line is init'ed with CommandStr, which contains Command without
//  any preceding \001's or :'s

   i = 0;
   Command = IC_UNKNOWN;
   while (IRCCommandTable[i].CommandStr != NULL) {
   int numwrds;

      if (LineP.isEqual(IRCCommandTable[i].CommandStr)) {
//       Got an IRC Command hit.
         Command = IRCCommandTable[i].Type;

//       Lets get in the other values if its a DCC chat or transfer.
         LineP = line;
         switch (Command) {

           case IC_MODE:
              Channel = new char[strlen(To) + 1];
              strcpy(Channel, To);
              break;

           case IC_PRIVMSG:
//            InfoLine has the full line.
//            Nothing to be done.
              break;

           case IC_LIST:
//            Make InfoLine have the next word after !LIST
              parseptr = LineP.getWord(5);
              delete [] InfoLine;
              InfoLine = new char[strlen(parseptr) + 1];
              strcpy(InfoLine, parseptr);
              break;

           case IC_FIND:
//            Make InfoLine have the rest of the words after @FIND
              parseptr = LineP.getWordRange(5, 0);
              delete [] InfoLine;
              InfoLine = new char[strlen(parseptr) + 1];
              strcpy(InfoLine, parseptr);
              break;

           case IC_FINDSWARM:
              // Make InfoLine have the rest of the words after @STREAM
              parseptr = LineP.getWordRange(5, 0);
              delete [] InfoLine;
              InfoLine = new char[strlen(parseptr) + 1];
              strcpy(InfoLine, parseptr);
              break;

           case IC_CTCPPING:
              parseptr = LineP.getWord(5);
              delete [] InfoLine;
              InfoLine = new char[strlen(parseptr) + 1];
              strcpy(InfoLine, parseptr); // The PING timestamp in InfoLine
              if (InfoLine[strlen(InfoLine) - 1] == '\001') {
                 InfoLine[strlen(InfoLine) - 1] = '\0';
              }
              break;

           case IC_CTCPFILESHA1:
             // InfoLine contains: FILESHA1 file_size file_sha1_str file_name
             // We fill up FileSize, FileName and SHA (in InfoLine)
             if (InfoLine[strlen(InfoLine) - 1] == '\001') {
                InfoLine[strlen(InfoLine) - 1] = '\0';
             }
             LineP = InfoLine;
             parseptr = LineP.getWord(2);
             FileSize = strtoul(parseptr, NULL, 10);
             parseptr = LineP.getWord(3);
             delete [] InfoLine;
             InfoLine = new char[strlen(parseptr) + 1];
             strcpy(InfoLine, parseptr);
             parseptr = LineP.getWordRange(4, 0);
             FileName = new char[strlen(parseptr) + 1];
             strcpy(FileName, parseptr);
             break;

          case IC_CTCPUPGRADE:
             // InfoLine contains: UPGRADE sha longip filename
             // We fill up FileName, LongIP and SHA (in InfoLine)
             if (InfoLine[strlen(InfoLine) - 1] == '\001') {
                InfoLine[strlen(InfoLine) - 1] = '\0';
             }
             LineP = InfoLine;
             parseptr = LineP.getWordRange(4, 0);
             FileName = new char[strlen(parseptr) + 1];
             strcpy(FileName, parseptr);

             parseptr = LineP.getWord(3);
             LongIP = strtoul(parseptr, NULL, 10);

             parseptr = LineP.getWord(2);
             delete [] InfoLine;
             InfoLine = new char[strlen(parseptr) + 1];
             strcpy(InfoLine, parseptr);
             break;

           case IC_CTCPNORESEND:
              break;

           case IC_CTCPLAG:
              break;

           case IC_CTCPPORTCHECK:
              Port = 0;
              parseptr = LineP.getWord(5);
              if (strlen(parseptr)) {
                 // Port = atoi(parseptr);
                 Port = (unsigned short) strtol(parseptr, NULL, 10);
              }
              if (Port == 0) {
                 Port = DCCSERVER_PORT;
              }
              break;

           case IC_ACTION:
//            Get rid of the "ACTION" at start of the InfoLine.
//              COUT(cout << "ACTION: InfoLine: " << InfoLine << endl;)
              strcpy(tmpword, &InfoLine[7]);
              strcpy(InfoLine, tmpword);
              break;

           case IC_CTCPFSERV:
              // We get the longip present in word 7
              parseptr = LineP.getWord(7);
              if (parseptr && strlen(parseptr)) {
                 LongIP = strtoul(parseptr, NULL, 10);
              }
              else {
                 LongIP = 0;
              }
              break;

           case IC_DCC_SEND:
           case IC_DCC_SWARM:
           case IC_DCC_RESUME:
           case IC_DCC_ACCEPT:
           case IC_DCC_CHAT:
              numwrds = LineP.getWordCount();
              numwrds = numwrds - 2;
              if ( (Command == IC_DCC_SEND) ||
                   (Command == IC_DCC_SWARM) ) {
                 numwrds--;
              }
              parseptr = LineP.getWordRange(6, numwrds);
              // FileName is possibly "name with space"
              FileName = new char[strlen(parseptr) + 1];
              if (parseptr[0] == '\"') {
                 strcpy(FileName, &parseptr[1]);
              }
              else {
                 strcpy(FileName, parseptr);
              }
              if (FileName[strlen(FileName) -1] == '\"') {
                 FileName[strlen(FileName) -1] = '\0';
              }
//            Got the FileName part (without quotes). In case of CHAT its chat
              if (strlen(FileName) == 0) {
                 // We just got an empty filename, lets add, default file.ext
                 delete [] FileName;
                 FileName = new char[9];
                 strcpy(FileName, "file.ext");
              }
              break;

           case IC_JOIN:
              parseptr = LineP.getWord(3); 
              // word1 has :#channel (if others join) or #channel (if I join)
              Channel = new char[strlen(parseptr) + 1];
              if (parseptr[0] == '#') {
                 strcpy(Channel, parseptr);
              }
              else {
                 strcpy(Channel, &parseptr[1]);
              }
              break;

           case IC_PART:
              parseptr = LineP.getWord(3); // word1 has #channel
              Channel = new char[strlen(parseptr) + 1];
              strcpy(Channel, parseptr);
              break;

           case IC_KICK:
              parseptr = LineP.getWord(3); // word 3 is #channel
              Channel = new char[strlen(parseptr) + 1];
              strcpy(Channel, parseptr);
              parseptr = LineP.getWord(4); // word 4 is The kicked nick.
              KickedNick = new char[strlen(parseptr) + 1];
              strcpy(KickedNick, parseptr);
              delete [] InfoLine;
              LineP.setDeLimiter(':');
              parseptr = LineP.getWord(3);
              InfoLine = new char[strlen(parseptr) + 1];
              strcpy(InfoLine, parseptr);
              LineP.setDeLimiter(' ');
              break;

           case IC_QUIT:
           case IC_NICKCHANGE: // new nick is in InfoLine. old in From.Nick
             LineP.setDeLimiter(':');
             parseptr = LineP.getWord(3);
             delete [] InfoLine;
             InfoLine = new char[strlen(parseptr) + 1];
             strcpy(InfoLine, parseptr);

             if (Command == IC_QUIT) {
                // Copy over Word 4 and beyond if it exists as the 
                // Quit message in To
                parseptr = LineP.getWordRange(4, 0);
                if ( (parseptr) && strlen(parseptr) ) {
                   delete [] To;
                   To = new char[strlen(parseptr)];
                   strcpy(To, &parseptr[1]); // Skip past space.
                }
             }
             LineP.setDeLimiter(' ');
             break;

           case IC_ERROR: // Lets get the Proper Message.
             LineP.setDeLimiter('(');
             parseptr = LineP.getWord(2);
             delete [] InfoLine;
             InfoLine = new char[strlen(parseptr) + 1];
             strcpy(InfoLine, parseptr);
             InfoLine[strlen(InfoLine) - 1] = '\0'; // The ending ')'
//           InfoLine has: "Quit:  MasalaMate v0.2 Alpha"
             LineP.setDeLimiter(' ');
             break;

           case IC_NICKLIST:
              parseptr = LineP.getWord(5);
              Channel = new char[strlen(parseptr) + 1];
              strcpy(Channel, parseptr); // Channel has channel name.
              parseptr = LineP.getWordRange(6, 0);

              delete [] InfoLine;
              InfoLine = new char[strlen(parseptr)];
              strcpy(InfoLine, &parseptr[1]);
              // InfoLine has the nick list.
COUT(cout << "IRCLineInterpret NICKLIST: " << InfoLine << endl;)
              break;

           case IC_TOPIC:
//            Channel Name is word 4
              parseptr = LineP.getWord(4);
              Channel = new char[strlen(parseptr) + 1];
              strcpy(Channel, parseptr); // Channel has channel name.

//            Topic text is 3rd and beyond part of line with delimiter as :
              LineP.setDeLimiter(':');
              parseptr = LineP.getWordRange(3, 0);

              delete [] InfoLine;
              InfoLine = new char[strlen(parseptr) + 1];
              strcpy(InfoLine, parseptr);
              LineP.setDeLimiter(' ');
              break;

           case IC_TOPIC_END:
              // Channel Name is word 4
              parseptr = LineP.getWord(4);
              Channel = new char[strlen(parseptr) + 1];
              strcpy(Channel, parseptr); // Channel has channel name.

              // Topic last changed by and Change Time is rest of line.
              parseptr = LineP.getWordRange(5, 0);
              delete [] InfoLine;
              InfoLine = new char[strlen(parseptr) + 1];
              strcpy(InfoLine, parseptr);
              break;

           case IC_TOPIC_CHANGE:
              Channel = new char[strlen(To) + 1];
              strcpy(Channel, To);
              break;

           case IC_NAMES_END:
              break;

           case IC_LIST_START:
              break;

           case IC_LIST_END:
              break;

           case IC_BANLIST:
              // 4th word is Channel Name.
              parseptr = LineP.getWord(4);
              Channel = new char[strlen(parseptr) + 1];
              strcpy(Channel, parseptr);

              // Word 5 to the end is-> banstring bannedby Time
              parseptr = LineP.getWordRange(5, 0);
              delete [] InfoLine;
              InfoLine = new char[strlen(parseptr) + 1];
              strcpy(InfoLine, parseptr);
              break;

           case IC_BANLIST_END:
              // 4th word is Channel Name.
              parseptr = LineP.getWord(4);
              Channel = new char[strlen(parseptr) + 1];
              strcpy(Channel, parseptr);

              // Word 5 to the end is-> :End of Channel Ban List
              parseptr = LineP.getWordRange(5, 0);
              delete [] InfoLine;
              InfoLine = new char[strlen(parseptr)];
              strcpy(InfoLine, &parseptr[1]);
              break;

           case IC_LIST_ENTRY:
              // Channel Name in LIST ENTRY is word 4.
              parseptr = LineP.getWord(4);
              Channel = new char[strlen(parseptr) + 1];
              strcpy(Channel, parseptr);

              // Channel Strength in LIST ENTRY is word 5.
              parseptr = LineP.getWord(5);
              ListChannelCount = strtoul(parseptr, NULL, 10);

              // word 6 to end is the Channel description.
              parseptr = LineP.getWordRange(6, 0);
              delete [] InfoLine;
              InfoLine = new char[41];
              strncpy(InfoLine, parseptr, 40);
              InfoLine[40] = '\0';
              break;

           default:
              break;
         }
         switch (Command) {

           case IC_DCC_CHAT:
           case IC_DCC_SEND:
           case IC_DCC_SWARM:
              numwrds = LineP.getWordCount();
              numwrds = numwrds - 1;
              if ( (Command == IC_DCC_SEND) ||
                   (Command == IC_DCC_SWARM) ) {
                 // last word is the FileSize.
                 parseptr = LineP.getWord(numwrds + 1);
                
                 strcpy(tmpword, parseptr);
                 if (tmpword[strlen(tmpword) - 1] == '\001') {
                    tmpword[strlen(tmpword) - 1] = '\0';
                 }

                 FileSize = strtoul(tmpword, NULL, 10);
                 // Got the File Size for SEND.

                 // patch numwrds to be applicable for both cases.
                 numwrds--;
              }

              parseptr = LineP.getWord(numwrds);
              LongIP = strtoul(parseptr, NULL, 10);
              // Got the IP address for SEND/CHAT
              parseptr = LineP.getWord(numwrds + 1);
              strcpy(tmpword, parseptr);
              if (tmpword[strlen(tmpword) - 1] == '\001') {
                 tmpword[strlen(tmpword) - 1] = '\0';
              }
              // Port = atoi(tmpword);
              Port = (unsigned short) strtol(tmpword, NULL, 10);
              // Got the Port for SEND/CHAT
COUT(cout << "DCC: " << line << endl;)
              break;

           case IC_DCC_RESUME:
           case IC_DCC_ACCEPT:
              numwrds = LineP.getWordCount();
              numwrds = numwrds - 1;

              parseptr = LineP.getWord(numwrds);
              // Port = atoi(parseptr);
              Port = (unsigned short) strtol(parseptr, NULL, 10);
              // Got the Port for RESUME/ACCEPT

              parseptr = LineP.getWord(numwrds + 1);
              strcpy(tmpword, parseptr);
              if (tmpword[strlen(tmpword) - 1] == '\001') {
                 tmpword[strlen(tmpword) - 1] = '\0';
              }
              ResumeSize = strtoul(tmpword, NULL, 10);
              // Got the Resume Size for RESUME/ACCEPT.
              break;

           default:
              break;
         }
         break; // break out of the while loop.
      }
      i++;
   }

// Check if Command is IC_UNKNOWN. it might be IC_NUMERIC.
   if (Command == IC_UNKNOWN) {
      if (strspn(CommandStr, "0123456789") == strlen(CommandStr)) {
         Command = IC_NUMERIC;
      }
   }
   delete [] word1;
   delete [] tmpword;
   delete [] info_line;
}

void IRCLineInterpret::printDebug() {
   TRACE();
   COUT(cout << "Interpret: ";)
   switch (From.Type) {
     case ILFT_NICKMESSAGE:
        COUT(cout << "From Type: NICKMESSAGE Nick: " << From.Nick;)
        break;

     case ILFT_USERMESSAGE:
        COUT(cout << "From Type: USERMESSAGE Nick: " << From.Nick << " UserName: " << From.UserName << " Host: " << From.Host;)
        break;

     case ILFT_SERVERMESSAGE:
        COUT(cout << "From Type: SERVERMESSAGE Server: " << From.Server;)
        break;
 
     case ILFT_COMMANDMESSAGE:
        COUT(cout << "From Type: COMMAND";)
        break;

     case ILFT_UNKNOWN:
        COUT(cout << "From Type: UNKNOWN";)
        break;
   }

   switch (Command) {
     case IC_SERVER_NOTICE:
        COUT(cout << " Command: SERVER_NOTICE Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_ERROR:
        COUT(cout << " Command: ERROR Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_CONNECT:
        COUT(cout << " Command: CONNECT Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_AWAY:
        COUT(cout << " Command: AWAY Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_USERHOST:
        COUT(cout << " Command: USERHOST Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_NOTICE:
        COUT(cout << " Command: NOTICE Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_PRIVMSG:
        COUT(cout << " Command: PRIVMSG Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_LIST:
        COUT(cout << " Command: LIST Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_FIND:
        COUT(cout << " Command: FIND Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_PING:
        COUT(cout << " Command: PING Str: " << CommandStr << " To: " << To << endl;)
        break;

     case IC_PONG:
        COUT(cout << " Command: PONG Str: " << CommandStr << " To: " << To << endl;)
        break;

     case IC_JOIN:
        COUT(cout << " Command: JOIN Str: " << CommandStr << " To: " << To << " Channel: " << Channel << endl;)
        break;

     case IC_PART:
        COUT(cout << " Command: PART Str: " << CommandStr << " To: " << To << " Channel: " << Channel << endl;)
        break;

     case IC_KICK:
        COUT(cout << " Command: KICK Str: " << CommandStr << " To: " << To << " Channel: " << Channel << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_QUIT:
        COUT(cout << " Command: QUIT Str: " << CommandStr << " To: " << To << " Quitting Nick: " << From.Nick << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_NICKCHANGE:
        COUT(cout << " Command: NICKCHANGE Str: " << CommandStr << " To: " << To << " Old Nick: " << From.Nick << " New Nick: " << InfoLine << endl;)
        break;

     case IC_MODE:
        COUT(cout << " Command: MODE Str: " << CommandStr << " To: " << To << " Channel: " << Channel << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_NICKLIST:
        COUT(cout << " Command: NICKLIST Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_CLIENTINFO:
        COUT(cout << " Command: CLIENTINFO Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_CTCPCLIENTINFOREPLY:
        COUT(cout << " Command: CLIENTINFOREPLY Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_VERSION:
        COUT(cout << " Command: VERSION Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_ACTION:
        COUT(cout << " Command: ACTION Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

    case IC_CTCPFSERV:
        COUT(cout << " Command: CTCPFSERV Str: " << CommandStr << " To: " << To << " LongIP: " << LongIP << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_CTCPPING:
        COUT(cout << " Command: CTCPPING Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_CTCPTIME:
        COUT(cout << " Command: CTCPTIME Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;


     case IC_CTCPNORESEND:
        COUT(cout << " Command: IC_CTCPNORESEND Str: " << CommandStr << " To: " << To << endl;)
        break;

     case IC_DCC_CHAT:
        COUT(cout << " Command: DCC CHAT Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << " IP: " << LongIP << " Port " << Port << endl;)
        break;

     case IC_DCC_SEND:
        COUT(cout << " Command: DCC SEND Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << " File: " << FileName << " IP: " << LongIP << " Port " << Port << endl;)
        break;

     case IC_DCC_SWARM:
        COUT(cout << " Command: DCC SWARM Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << " File: " << FileName << " IP: " << LongIP << " Port " << Port << " FileSize: " << FileSize << endl;)
        break;

     case IC_DCC_RESUME:
        COUT(cout << " Command: DCC RESUME Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << " File: " << FileName << " Port " << Port << endl;)
        break;

     case IC_DCC_ACCEPT:
        COUT(cout << " Command: DCC ACCEPT Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << " File: " << FileName << " Port " << Port << endl;)
        break;

     case IC_TOPIC:
        COUT(cout << " Command: IC_TOPIC Str: " << CommandStr << " Channel: " << Channel << " Topic is: " << InfoLine << endl;)
        break;

     case IC_TOPIC_END:
        COUT(cout << " Command: IC_TOPIC_END Str: " << CommandStr << " Channel: " << Channel << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_TOPIC_CHANGE:
        COUT(cout << " Command: IC_TOPIC_CHANGE Str: " << CommandStr << " Channel: " << Channel << " Changed by: " << From.Nick << " Topic is: " << InfoLine << endl;)
        break;

     case IC_NAMES_END:
        COUT(cout << " Command: IC_NAMES_END Str: " << CommandStr << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_LIST_START:
        COUT(cout << " Command: IC_LIST_START Str: " << CommandStr << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_LIST_END:
        COUT(cout << " Command: IC_LIST_END Str: " << CommandStr << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_LIST_ENTRY:
        COUT(cout << " Command: IC_LIST_ENTRY Str: " << CommandStr << " Channel: " << Channel << " Channel Count: " << ListChannelCount << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_NICKINUSE:
        COUT(cout << " Command: IC_NICKINUSE Str: " << CommandStr << " Nick: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_NUMERIC:
        COUT(cout << " Command: NUMERIC Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_CTCPPORTCHECK:
        COUT(cout << " Command: IC_CTCPPORTCHECK Str: " << CommandStr << " To: " << To << " Port: " << Port << endl;)
        break;

     case IC_CTCPPINGREPLY:
        COUT(cout << " Command: IC_CTCPPINGREPLY Str: " << CommandStr << " To: " << To << " TimeStamp: " << InfoLine << endl;)
        break;

     case IC_CTCPTIMEREPLY:
        COUT(cout << " Command: IC_CTCPTIMEREPLY Str: " << CommandStr << " To: " << To << " Time: " << InfoLine << endl;)
        break;

     case IC_CTCPVERSIONREPLY:
        COUT(cout << " Command: IC_CTCPVERSIONREPLY Str: " << CommandStr << " To: " << To << " Version: " << InfoLine << endl;)
        break;

     case IC_DCCALLOWREPLY:
        COUT(cout << " Command: IC_DCCALLOWREPLY Str: " << CommandStr << " To: " << To << " Response: " << InfoLine << endl;)
        break;

     case IC_CTCPPROPAGATE:
        COUT(cout << " Command: IC_CTCPPROPAGATE Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

     case IC_CTCPFILESHA1:
        COUT(cout << " Command: IC_CTCPFILESHA1 Str: " << CommandStr << " To: " << To << " FileName: " << FileName << " FileSize: " << FileSize << " SHA: " << InfoLine << endl;)
        break;

     case IC_CTCPFILESHA1REPLY:
        COUT(cout << " Command: IC_CTCPFILESHA1REPLY Str: " << CommandStr << " To: " << To << " FileName: " << FileName << " FileSize: " << FileSize << " SHA: " << InfoLine << endl;)
        break;

     case IC_CTCPUPGRADE:
        COUT(cout << " Command: IC_CTCPUPGRADE Str: " << CommandStr << " To: " << To << " FileName: " << FileName << " LongIP: " << LongIP << " SHA: " << InfoLine << endl;)
        break;

     case IC_CTCPUPGRADEREPLY:
        COUT(cout << " Command: IC_CTCPUPGRADEREPLY Str: " << CommandStr << " To: " << To << " FileName: " << FileName << " LongIP: " << LongIP << " SHA: " << InfoLine << endl;)
        break;

     case IC_UNKNOWN:
        COUT(cout << " Command: UNKNOWN Str: " << CommandStr << " To: " << To << " InfoLine: " << InfoLine << endl;)
        break;

   }
}
