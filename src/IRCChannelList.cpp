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

#include <string>
#include "IRCChannelList.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

// IRCChannelList Constructor class.
IRCChannelList::IRCChannelList() {
   TRACE();
   Head = NULL;
   ChannelCount = 0;
}

// IRCChannelList Destructor class.
IRCChannelList::~IRCChannelList() {
   TRACE();
   freeAll();
}

void IRCChannelList::freeAll() {
IRCChannelChain *Current;

   TRACE();
   while (Head != NULL) {
      Current = Head->Next;
      delete [] Head->Channel;
      delete [] Head->ChannelKey;
      delete Head;
      Head = Current;
   }
   Head = NULL;
   ChannelCount = 0;
}

// IRCChannelList operator ==
bool IRCChannelList::operator==(const IRCChannelList& CL) {
IRCChannelChain *src_cur;
bool retvalb;

   TRACE();

   if (this == &CL) return(true);

   src_cur = CL.Head;
   while (src_cur != NULL) {
      if (isChannel(src_cur->Channel) == false) break;
      src_cur = src_cur->Next;
   }
   if (src_cur == NULL) retvalb = true;
   else retvalb = false;

// Take care of exception when CL.Head is NULL and this->Head is not.
   if ( (CL.Head == NULL) && (this->Head != NULL) ) retvalb = false;

   return(retvalb);
}

// IRCChannelList operator =
// Copies over the channel names and joined status and join time.
IRCChannelList& IRCChannelList::operator=(const IRCChannelList& CL) {
IRCChannelChain *src_cur;

   TRACE();
   if (this == &CL) return(*this);

   freeAll();

// Lets replicate the list here from CL.
   src_cur = CL.Head;
   while (src_cur != NULL) {
      addChannel(src_cur->Channel, src_cur->ChannelKey);
      Head->Joined = src_cur->Joined;
      Head->JoinedTime = src_cur->JoinedTime;
      src_cur = src_cur->Next;
   }
}

void IRCChannelList::delChannel(char *varchannel) {
IRCChannelChain *Current, *Previous;

   TRACE();
   if ( (varchannel == NULL) || (Head == NULL) ) return;

   Current = Head;
   Previous = Head;
   while (Current != NULL) {
      if (strcasecmp(varchannel, Current->Channel) == 0) {
         // This is the entry we want to get rid of.
         if (Previous == Current) {
            // If Previous == Current => Head element. 
            Head = Head->Next;
         }
         else {
            // Some other element other than Head.
            Previous->Next = Current->Next;
         }
         delete [] Current->Channel;
         delete [] Current->ChannelKey;
         delete Current;
         break;
      }
      Previous = Current;
      Current = Current->Next;
   }
}

void IRCChannelList::addChannel(char *varchannel, char *varchannelkey) {
IRCChannelChain *Current;

   TRACE();
   if (varchannel == NULL) return;

// Lets check if the Channel name is already in the list.
   if (isChannel(varchannel) == true) {
      return;
   }

   Current = new IRCChannelChain;
   Current->Next = Head;
   Current->Channel = new char[strlen(varchannel) + 1];
   strcpy(Current->Channel, varchannel);
   if (varchannelkey && (strlen(varchannelkey) > 0) ) {
      Current->ChannelKey = new char[strlen(varchannelkey) + 1];
      strcpy(Current->ChannelKey, varchannelkey);
   }
   else {
      Current->ChannelKey = NULL;
   }
   Current->Joined = false;
   Current->JoinAttemptTime = 0;
   Current->JoinedTime = 0;
   Head = Current;
   ChannelCount++;
   return;
}

// Return the i'th Channel.
char * const IRCChannelList::getChannel(int chanindex) {
char *retstr = NULL;
int i = 0;
IRCChannelChain *Current;

   TRACE();
   Current = Head;
   while (Current != NULL) {
      i++;
      if (i == chanindex) {
         retstr = Current->Channel;
         break;
      }
      Current = Current->Next;
   }
   return(retstr);
}

// Return the i'th Channel Key.
char * const IRCChannelList::getChannelKey(int chanindex) {
char *retstr = NULL;
int i = 0;
IRCChannelChain *Current;

   TRACE();
   Current = Head;
   while (Current != NULL) {
      i++;
      if (i == chanindex) {
         retstr = Current->ChannelKey;
         break;
      }
      Current = Current->Next;
   }
   return(retstr);
}


int IRCChannelList::getChannelCount() {
   TRACE();
   return(ChannelCount);
}

bool IRCChannelList::isChannel(char *varchannel) {
IRCChannelChain *Current;

   TRACE();

   if (varchannel == NULL) {
      return(false);
   }
   Current = getMatch(varchannel);
   if (Current == NULL) {
      return(false);
   }
   return(true);
}

void IRCChannelList::setJoined(char *varchannel) {
IRCChannelChain *Current;

   TRACE();
   Current = getMatch(varchannel);
   if (Current == NULL) {
      COUT(cout << "setJoined " << varchannel << " Failed" << endl;)
      return;
   }
   Current->Joined = true;
   Current->JoinedTime = time(NULL);
}

void IRCChannelList::setJoined(int chanindex) {
IRCChannelChain *Current;

   TRACE();
   Current = getMatch(chanindex);
   if (Current == NULL) {
      return;
   }
   Current->Joined = true;
   Current->JoinedTime = time(NULL);
}

void IRCChannelList::setNotJoined(char *varchannel) {
IRCChannelChain *Current;

   TRACE();
   Current = getMatch(varchannel);
   if (Current == NULL) {
      return;
   }
   Current->Joined = false;
}

void IRCChannelList::setNotJoined(int chanindex) {
IRCChannelChain *Current;

   TRACE();
   Current = getMatch(chanindex);
   if (Current == NULL) {
      return;
   }
   Current->Joined = false;
}

bool IRCChannelList::isJoined(char *varchannel) {
IRCChannelChain *Current;

   TRACE();
   Current = getMatch(varchannel);
   if (Current == NULL) {
      return(false);
   }
   return(Current->Joined);
}

bool IRCChannelList::isJoined(int chanindex) {
IRCChannelChain *Current;

   TRACE();
   Current = getMatch(chanindex);
   if (Current == NULL) {
      return(false);
   }
   return(Current->Joined);
}

time_t IRCChannelList::getJoinAttemptTime(char *varchannel) {
IRCChannelChain *Current;

   TRACE();
   Current = getMatch(varchannel);
   if (Current == NULL) {
      return(0);
   }
   return(Current->JoinAttemptTime);
}

time_t IRCChannelList::getJoinAttemptTime(int chanindex) {
IRCChannelChain *Current;

   TRACE();
   Current = getMatch(chanindex);
   if (Current == NULL) {
      return(0);
   }
   return(Current->JoinAttemptTime);
}

time_t IRCChannelList::getJoinedTime(char *varchannel) {
IRCChannelChain *Current;

   TRACE();
   Current = getMatch(varchannel);
   if (Current == NULL) {
      return(0);
   }
   return(Current->JoinedTime);
}

time_t IRCChannelList::getJoinedTime(int chanindex) {
IRCChannelChain *Current;

   TRACE();
   Current = getMatch(chanindex);
   if (Current == NULL) {
      return(0);
   }
   return(Current->JoinedTime);
}

void IRCChannelList::setJoinAttemptTime(char *varchannel) {
IRCChannelChain *Current;

   TRACE();
   Current = getMatch(varchannel);
   if (Current != NULL) {
      Current->JoinAttemptTime = time(NULL);
   }
}

void IRCChannelList::setJoinAttemptTime(int chanindex) {
IRCChannelChain *Current;

   TRACE();
   Current = getMatch(chanindex);
   if (Current != NULL) {
      Current->JoinAttemptTime = time(NULL);
   }
}

// Need to do a case insensitive channel name comparison.
IRCChannelChain *IRCChannelList::getMatch(char *varchannel) {
IRCChannelChain *Current;

   TRACE();
   if (varchannel == NULL) {
      return(NULL);
   }
   Current = Head;
   while (Current != NULL) {
      if (strcasecmp(Current->Channel, varchannel) == 0) {
         return(Current);
      }
      Current = Current->Next;
   }
   return(NULL);
}

IRCChannelChain *IRCChannelList::getMatch(int chanindex) {
IRCChannelChain *Current;
int i = 1;

   TRACE();
   if (chanindex < 1) {
      return(NULL);
   }
   Current = Head;
   while (Current != NULL) {
      if (i == chanindex) {
         return(Current);
      }
      i++;
      Current = Current->Next;
   }
   return(NULL);
}

void IRCChannelList::printDebug() {
IRCChannelChain *tmp;
int i = 1;

   TRACE();
   tmp = Head;
   while (tmp != NULL) {
      COUT(cout << "Channel: " << i++ << " of " << ChannelCount << " " << tmp->Channel << " Joined: " << tmp->Joined << " Join Attempt: " << tmp->JoinAttemptTime << " Join Time: " << tmp->JoinedTime << " Key: ";)
      if (tmp->ChannelKey) {
         COUT(cout << tmp->ChannelKey << endl;)
      }
      else {
         COUT(cout << "NULL" << endl;)
      }
      tmp = tmp->Next;
   }
   return;
}
