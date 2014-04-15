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

#ifndef CLASS_IRCLINEINTERPRET
#define CLASS_IRCLINEINTERPRET

#include "Compatibility.hpp"

// Enumerate the IRC Line From type.
typedef enum {
   ILFT_UNKNOWN,

   ILFT_SERVERMESSAGE,
// examples:
//    :flowerpower.fl.us.rc0.net NOTICE AUTH :Checking Ident
//    :flowerpower.fl.us.rc0.net 001 Art-Test-Art :Welcome to the network

   ILFT_USERMESSAGE,
//    :Sur4802!~noemail@c-67...comcast.net PRIVMSG Art-Test-Art :Hello
//    :Sur4802!~noemail@c-67...comcast.net NOTICE Art-Test-Art :DCC Chat (67..)
//    :Sur4802!~noemail@c-67...comcast.net DCC CHAT chat 134631802 8125

   ILFT_NICKMESSAGE,
//    :Art-Test-Art MODE Art-Test-Art :+i

   ILFT_COMMANDMESSAGE
//     ERROR :Closing Link: blah blah. No FROM fields.
//     NOTICE Stealth :*** Your host ... <- this getting new in IRCSuper.
//     This is different from IC_NOTICE, this is IC_SERVER_NOTICE

} IRCLineFromTypeE;

typedef struct IRCLineFrom {
// If Type = ILFT_NICKMESSAGE only Nick Filed is valid.
// If Type = ILFT_USERMESSAGE Nick, UserName and HostOrServer are valid.
//           and HostOrServer is the Host.
// If Type = ILFT_SERVERMESSAGE only HostOrServer is valid
//           and HostOrServer is the Server.
// If Type = ILFT_UNKNOWN none of the fields are valid.
   IRCLineFromTypeE Type;

   char *Nick;
   char *UserName;
   char *Host;
   char *Server;
} IRCLineFrom;

typedef enum {
   IC_UNKNOWN,

// :flowerpower.fl.us.rc0.net 001 Art-Test-Art :Welcome to the network
   IC_CONNECT,

// :flowerpower.fl.us.rc0.net 433 Art-Test-Art :Nickname is already in use
   IC_NICKINUSE,

// :flowerpower.fl.us.rc0.net 301 Art-Test-Art :Auto Away: 9 mins of no action
   IC_AWAY,

// :Insanity.mo.us.ircSuper.net 302 Sur4802 :Sur4802=+~khamand@c-67-161-27-122.client.comcast.net
   IC_USERHOST,

// ERROR :Closing Link: Art-Test-Art[c-67...comcast.net] (Ping timeout)
// ERROR :Closing Link: Art-CzSIrb[c-67-161-27-12...net] (Quit:  MasalaMate v0.2 Alpha)
   IC_ERROR,

// NOTICE Stealth :*** Your host ... This is a Server notice, and not IC_NOTICE
   IC_SERVER_NOTICE,

// :flowerpower.fl.us.rc0.net NOTICE AUTH :Checking ident
// Note that CTCP PING reply is a NOTICE, but we classify it further as
// IC_CTCPPINGREPLY. Same with IC_CTCPVERSIONREPLY
   IC_NOTICE,
   IC_CTCPPINGREPLY,
   IC_CTCPTIMEREPLY,
   IC_CTCPVERSIONREPLY,
   IC_CTCPCLIENTINFOREPLY,
   IC_CTCPFILESHA1REPLY,
   IC_CTCPUPGRADEREPLY,

// :Sur4802!~noemail@c67-...comcast.net PRIVMSG Art-Test-Art :Hello
// Note that CTCP's come in as PRIVMSG, but we classify them further
// as IC_CLIENTINFO etc and hence it doesnt fall in this category.
   IC_PRIVMSG,

// Detects PRIVMSG which comes in as @FIND.
   IC_FIND,

// Detects PRIVMSG which comes in as @SWARM.
   IC_FINDSWARM,

// Detect PRIVMSG which comes in as !LIST
   IC_LIST,

// :flowerpower.fl.us.rc0.net 001 Art-Test-Art :Welcome to the RC0 network.
   IC_NUMERIC,

// PING :flowerpower.fl.us.rc0.net
   IC_PING,

// PONG :whatever.criten.net
   IC_PONG,

// :Sur4802!~noemail@c67-...comcast.net JOIN :#Masala-Chat
   IC_JOIN,

// :Sur4802!~noemail@c67-...comcast.net PART :#Masala-Chat
   IC_PART,

// :a0000!~a0000@inet...oracle.com KICK #masala-chat cyprusrodt :test
   IC_KICK,

// :flowerpower.fl.us.rc0.net 353 Art-Test-Art = #masala-chat :nick1 nick2
// %nick3 +Nick4 @nick5
   IC_NICKLIST,

// :Sur4802!~khamand@c-67-161-27-122.client.comcast.net TOPIC #Masala :test
// User Sur4802 has changed topic of #Masala to "test"
   IC_TOPIC_CHANGE,

// :flowerpower.fl.us.rc0.net 332 Art-Test-Art #IndianMasala :This is the topic text.
   IC_TOPIC,

// :Matrix.mo.us.ircsuper.net 321 Sur4802 Channel User: Name
   IC_LIST_START,

// :Matrix.mo.us.ircsuper.net 322 Sur4802 #whatever 21: Junk description
   IC_LIST_ENTRY,

// :Matrix.mo.us.ircsuper.net 323 Sur4802 :End of /LIST
   IC_LIST_END,

// :Matrix.mo.us.ircsuper.net 366 Sur4802 :End of /NAMES
   IC_NAMES_END,

// :Matrix.mo.us.ircsuper.net 333 Sur4802 #tamil-music Jolo 1125676737
   IC_TOPIC_END,

// :Matrix.mo.us.ircsuper.net 367 Sur4802 #tamil-music *^*!*@*
//    KhelBaccha!KhelBaccha@netblock-72-25-106-15.dslextreme.com 1128047791
   IC_BANLIST,

// :Matrix.mo.us.ircsuper.net 368 Sur4802 #tamil-music :End of Channel Ban List
   IC_BANLIST_END,

// :flowerpower.fl.us.rc0.net 617 Art-Test-Art :information (dccallow response)
   IC_DCCALLOWREPLY,

// :Art-Test-Art MODE Art-Test-Art :+i
   IC_MODE,

// QUIT: :crazyacrazy!~WinNT@202.9.163.3 QUIT :Ping timeout
   IC_QUIT,

// :Guest95222!~lucky_leo@c68.119.25.215.stc.mn.charter.com NICK :lucky
   IC_NICKCHANGE,

// Below are PRIVMSG's but as they are CTCPs we further finely classify them.
   IC_CTCPPROPAGATE,
   IC_CTCPFILESHA1,
   IC_CTCPPORTCHECK,
   IC_CTCPNORESEND,
   IC_CTCPFSERV,
   IC_CTCPPING,
   IC_CTCPTIME,
   IC_CLIENTINFO,
   IC_CTCPUPGRADE,
   IC_CTCPLAG,
   IC_ACTION,
   IC_VERSION,
   IC_DCC_CHAT,
   IC_DCC_SEND,
   IC_DCC_SWARM,
   IC_DCC_RESUME,
   IC_DCC_ACCEPT

} IRCCommandE;

// Below is used as a table to get a quick translation.
typedef struct IRCCommand {
   IRCCommandE Type;
   char	*CommandStr;
} IRCCommand;


// The way to use it is to call interpretLine().
// and then access the structures.
class IRCLineInterpret {
public:
   IRCLineInterpret();
   ~IRCLineInterpret();

   IRCLineInterpret& operator = (char *); // Equating it to a char string.

   IRCLineFrom From;
   IRCCommandE Command;
   char *CommandStr; //example: "NOTICE"
   char *To;  // 2nd word of Command. can be AUTH too, usually our nick.
   char *Channel; // For commands which have channel info, eg. JOIN/NICKlist
   char *KickedNick; // Nick which has been kicked.
   char *InfoLine; // For message part or NICK list or Kick reason etc.
   char *FileName; // For DCC File Transfer
   unsigned long FileSize; // For DCC File Transfer.
   unsigned long ResumeSize; // For DCC File Transfer.
   unsigned long LongIP; // for DCC File transfer.
   unsigned short Port; // for DCC File transfer. or CTCP PORTCHECK
   unsigned long ListChannelCount; // For a /list response.
   void printDebug();

private:
   void init();
   void freeAll();
   void updateTriggerInfo(char *);
};


#endif
