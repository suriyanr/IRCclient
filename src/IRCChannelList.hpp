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

#ifndef CLASS_IRCCHANNELCHAIN
#define CLASS_IRCCHANNELCHAIN

#include "Compatibility.hpp"
#include <time.h>


/*
  This maintains a linked list of Channel names.
  Adding channel names to this list is done only if its not already present.
  You use this class to keep track of channel names, and if joined in
  that channel or not.
*/
typedef struct IRCChannelChain {
   char *Channel;
   char *ChannelKey;
   bool Joined;
   time_t JoinAttemptTime;
   time_t JoinedTime;
   struct IRCChannelChain *Next;
} IRCChannelChain;

class IRCChannelList {
public:
   IRCChannelList();
   ~IRCChannelList();
   IRCChannelList& operator=(const IRCChannelList&);
   bool operator==(const IRCChannelList&);
   void addChannel(char* varchannel, char *varchannelkey);
   void delChannel(char *varchannel);
   char * const getChannel(int chanindex);
   char * const getChannelKey(int chanindex);
   int getChannelCount();
   bool isChannel(char *varchannel);
   void setJoined(char *varchannel);
   void setJoined(int chanindex);
   void setNotJoined(char *varchannel);
   void setNotJoined(int chanindex);
   bool isJoined(char *varchannel);
   bool isJoined(int chanindex);
   time_t getJoinAttemptTime(char *varchannel);
   time_t getJoinAttemptTime(int chanindex);
   void setJoinAttemptTime(char *varchannel);
   void setJoinAttemptTime(int chanindex);
   time_t getJoinedTime(char *varchannel);
   time_t getJoinedTime(int chanindex);
   void printDebug();

private:
   IRCChannelChain *Head;
   int ChannelCount;
   IRCChannelChain *getMatch(char *varchannel);
   IRCChannelChain *getMatch(int chanindex);
   void freeAll();
};


#endif
