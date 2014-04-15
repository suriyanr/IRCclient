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

#include "IRCNickLists.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

// IRCNickList Constructor class.
IRCNickLists::IRCNickLists() {
   TRACE();
   Head = NULL;
   ChannelCount = 0;

#ifdef __MINGW32__
   IRCNickListsMutex = CreateMutex(NULL, FALSE, NULL);
#else
   pthread_mutex_init(&IRCNickListsMutex, NULL);
#endif
}

// IRCChannelList Destructor class.
IRCNickLists::~IRCNickLists() {
   TRACE();
   freeAll();

   DestroyMutex(IRCNickListsMutex);
// Possible core dump if someone is using that data and we happily
// go out of scope.
}

// Private Function - Lock should be owned by caller.
void IRCNickLists::freeChannel(IRCNickChainExt *CurrentExt) {
IRCNickChain *Current;
IRCNickChain *TmpCurrent;
IRCNickChainExt *TmpCurrentExt;

   TRACE();
   if (CurrentExt == NULL) return;

   if (CurrentExt->Channel) {
      delete [] CurrentExt->Channel;
      CurrentExt->Channel = NULL;
   }

   CurrentExt->NickCount = 0;
   if (CurrentExt->NickListHead != NULL) {

      Current = CurrentExt->NickListHead;
      while (Current != NULL) {
         TmpCurrent = Current->Next;
         delete [] Current->Nick;
         delete Current;
         Current = TmpCurrent;
      }
      CurrentExt->NickListHead = NULL;
   }

// Now to delete CurrentExt in the IRCNickChainExt list.
   if (CurrentExt == Head) {
//    Very first element.
      delete [] CurrentExt->Channel;
      Head = Head->Next;
      ChannelCount--;
      delete CurrentExt;
      return;
   }

   TmpCurrentExt = Head;
   while (TmpCurrentExt->Next != NULL) {
      if (TmpCurrentExt->Next == CurrentExt) {
//       Got a hit.
         TmpCurrentExt->Next = CurrentExt->Next;
         delete [] CurrentExt->Channel;
         delete CurrentExt;
         ChannelCount--;
         break;
      }
      TmpCurrentExt = TmpCurrentExt->Next;
   }
}

void IRCNickLists::freeAll() {
IRCNickChainExt *CurrentExt;

   TRACE();
   WaitForMutex(IRCNickListsMutex);
   while (Head != NULL) {
      CurrentExt = Head->Next;
      freeChannel(Head);
      Head = CurrentExt;
   }

   Head = NULL;
   ChannelCount = 0;
   ReleaseMutex(IRCNickListsMutex);
}

void IRCNickLists::addChannel(char *varchannel) {
IRCNickChainExt *CurrentExt;

   TRACE();
   if (varchannel == NULL) return;

   WaitForMutex(IRCNickListsMutex);

   CurrentExt = getChannelMatch(varchannel);
   if (CurrentExt != NULL) {
      ReleaseMutex(IRCNickListsMutex);
      return;
   }

   CurrentExt = new IRCNickChainExt;
   CurrentExt->Channel = new char[strlen(varchannel) + 1];
   strcpy(CurrentExt->Channel, varchannel);
   CurrentExt->NickCount = 0;
   CurrentExt->NickListHead = NULL;
   CurrentExt->Next = Head;
   Head = CurrentExt;
   ChannelCount++;

   ReleaseMutex(IRCNickListsMutex);
}

// Private function - Lock is held by caller.
IRCNickChainExt *IRCNickLists::getChannelMatch(char *varchannel) {
IRCNickChainExt *CurrentExt;

   TRACE();
   if (varchannel == NULL) return(NULL);

   CurrentExt = Head;
   while (CurrentExt != NULL) {
      if (strcasecmp(CurrentExt->Channel, varchannel) == 0) {
         break;
      }
      CurrentExt = CurrentExt->Next;
   }

   return(CurrentExt);
}

void IRCNickLists::delChannel(char *varchannel) {
IRCNickChainExt *CurrentExt;

   TRACE();
   if (varchannel == NULL) return;

   WaitForMutex(IRCNickListsMutex);

   CurrentExt = getChannelMatch(varchannel);
   if (CurrentExt != NULL) {
      freeChannel(CurrentExt);
   }

   ReleaseMutex(IRCNickListsMutex);
}

void IRCNickLists::getChannelName(char *varchannel, int chanindex) {
IRCNickChainExt *CurrentExt;
int i = 1;

   TRACE();
   varchannel[0] = '\0';

   WaitForMutex(IRCNickListsMutex);

   CurrentExt = Head;
   while (CurrentExt != NULL) {
      if (i == chanindex) {
         strcpy(varchannel, CurrentExt->Channel);
         break;
      }
      i++;
      CurrentExt = CurrentExt->Next;
   }

   ReleaseMutex(IRCNickListsMutex);
}

// IRCNickList operator =
// Copies over the Nick Names List.
// Current only copies the Channels and empty nick lists.
IRCNickLists& IRCNickLists::operator=(const IRCNickLists& NL) {
IRCNickChainExt *src_cur;

   TRACE();
   if (this == &NL) return(*this);

   freeAll();

// Lets replicate the list here from NL.
// No Mutex protection here as its complicated - chance of a hang.
   src_cur = NL.Head;
   while (src_cur != NULL) {
      addChannel(src_cur->Channel);
      src_cur = src_cur->Next;
   }
}

void IRCNickLists::delNick(char *varchannel, char *varnick) {
IRCNickChainExt *CurrentExt;
IRCNickChain *Current, *TmpCurrent;

   TRACE();
   if ( (Head == NULL) || (varnick == NULL) || (varchannel == NULL) ) return;

   WaitForMutex(IRCNickListsMutex);

   CurrentExt = getChannelMatch(varchannel);
   if (CurrentExt == NULL) {
       ReleaseMutex(IRCNickListsMutex);
       return;
   }

   Current = CurrentExt->NickListHead;

// Lets see if its the very first element.
   if ( !strcasecmp(Current->Nick, varnick) ) {
      CurrentExt->NickListHead = Current->Next;
      delete [] Current->Nick;
      delete Current;
      CurrentExt->NickCount--;
      ReleaseMutex(IRCNickListsMutex);
      return;
   }

   while (Current->Next != NULL) {
      if ( !strcasecmp(Current->Next->Nick, varnick) ) {
//       Got a hit.
         TmpCurrent = Current->Next;
         delete [] TmpCurrent->Nick;
         Current->Next = TmpCurrent->Next;
         delete TmpCurrent;
         CurrentExt->NickCount--;
         break;
      }
      Current = Current->Next;
   }
   ReleaseMutex(IRCNickListsMutex);
}

void IRCNickLists::setNickFirewall(char *varnick, char fw) {
IRCNickChainExt *ChainHead;
IRCNickChain *Current;

   TRACE();
   if (varnick == NULL) {
      return;
   }

   WaitForMutex(IRCNickListsMutex);

   ChainHead = Head;
   while (ChainHead) {
      Current = getNickMatch(ChainHead->Channel, varnick);
      if (Current != NULL) {
         Current->NickFirewall = fw;
      }
      ChainHead = ChainHead->Next;
   }

   ReleaseMutex(IRCNickListsMutex);
}

char IRCNickLists::getNickFirewall(char *varnick) {
IRCNickChainExt *ChainHead;
IRCNickChain *Current;
char retval = IRCNICKFW_UNKNOWN;

   TRACE();
   if (varnick == NULL) {
      return(retval);
   }

   WaitForMutex(IRCNickListsMutex);

   ChainHead = Head;
   while (ChainHead) {
      Current = getNickMatch(ChainHead->Channel, varnick);
      if ((Current != NULL) && (Current->NickFirewall != retval)) {
         retval = Current->NickFirewall;
         break;
      }
      ChainHead = ChainHead->Next;
   }

   ReleaseMutex(IRCNickListsMutex);

   return(retval);
}

void IRCNickLists::setNickClient(char *varnick, char client) {
IRCNickChainExt *ChainHead;
IRCNickChain *Current;

   TRACE();
   if (varnick == NULL) {
      return;
   }

   WaitForMutex(IRCNickListsMutex);

   ChainHead = Head;
   while (ChainHead) {
      Current = getNickMatch(ChainHead->Channel, varnick);
      if (Current != NULL) {
         Current->NickClient = client;
      }
      ChainHead = ChainHead->Next;
   }

   ReleaseMutex(IRCNickListsMutex);
}

char IRCNickLists::getNickClient(char *varnick) {
IRCNickChainExt *ChainHead;
IRCNickChain *Current;
char retval = IRCNICKCLIENT_UNKNOWN;

   TRACE();
   if (varnick == NULL) {
      return(retval);
   }

   WaitForMutex(IRCNickListsMutex);

   ChainHead = Head;
   while (ChainHead) {
      Current = getNickMatch(ChainHead->Channel, varnick);
      if ((Current != NULL) && (Current->NickClient != retval)) {
         retval = Current->NickClient;
         break;
      }
      ChainHead = ChainHead->Next;
   }

   ReleaseMutex(IRCNickListsMutex);

   return(retval);
}

// varnick is allocated by callee.
// Returns the nick at nickindex in varchannel from the list of MM clients
// who are with FW state != IRCNICKFW_NO
// returns true on success, false on failure.
// nickindex varies from 1 to FWMMCount
bool IRCNickLists::getNickInChannelAtIndexFWMM(char *varchannel, int nickindex, char *varnick) {
IRCNickChainExt *CurrentExt;
IRCNickChain *Current;
int cur_index;
bool retvalb = false;
bool found = false;

   TRACE();
   if ( (varchannel == NULL) || (varnick == NULL) ) {
      return(false);
   }

   varnick[0] = '\0';
   if (nickindex <= 0) return(false);

   WaitForMutex(IRCNickListsMutex);

   do {

      CurrentExt = getChannelMatch(varchannel);
      if (CurrentExt == NULL) break;
      if (CurrentExt->NickCount < nickindex) break;

      // We are here, so all values are correct. Just pick the
      // correct nick.
      Current = CurrentExt->NickListHead;

      cur_index = 0;
      while (Current != NULL) {
         if ( (Current->NickClient == IRCNICKCLIENT_MASALAMATE) &&
              (Current->NickFirewall != IRCNICKFW_NO) ) {
            cur_index++;
            if (cur_index == nickindex) {
               // We have spotted the nick we have been looking for.
               found = true;
               break;
            }
         }
         Current = Current->Next;
      }

      // This shouldnt happen either.
      if (Current == NULL) break;

      if (found) {
         strcpy(varnick, Current->Nick);
         retvalb = true;
      }

   } while (false);

   ReleaseMutex(IRCNickListsMutex);

   return(retvalb);
}

// varnick is allocated by callee.
// Returns the nick at nickindex in varchannel from the list of MM clients
// who are with FW state = IRCNICKFW_NO
// returns true on success, false on failure.
// nickindex varies from 1 to NFMMCount
bool IRCNickLists::getNickInChannelAtIndexNFMM(char *varchannel, int nickindex, char *varnick) {
IRCNickChainExt *CurrentExt;
IRCNickChain *Current;
int cur_index;
bool retvalb = false;
bool found = false;

   TRACE();
   if ( (varchannel == NULL) || (varnick == NULL) ) {
      return(false);
   }

   varnick[0] = '\0';
   if (nickindex <= 0) return(false);

   WaitForMutex(IRCNickListsMutex);

   do {

      CurrentExt = getChannelMatch(varchannel);
      if (CurrentExt == NULL) break;
      if (CurrentExt->NickCount < nickindex) break;

      // We are here, so all values are correct. Just pick the
      // correct nick.
      Current = CurrentExt->NickListHead;

      cur_index = 0;
      while (Current != NULL) {
         if ( (Current->NickClient == IRCNICKCLIENT_MASALAMATE) &&
              (Current->NickFirewall == IRCNICKFW_NO) ) {
            cur_index++;
            if (cur_index == nickindex) {
               // We have spotted the nick we have been looking for.
               found = true;
               break;
            }
         }
         Current = Current->Next;
      }

      // This shouldnt happen either.
      if (Current == NULL) break;

      if (found) {
         strcpy(varnick, Current->Nick);
         retvalb = true;
      }

   } while (false);

   ReleaseMutex(IRCNickListsMutex);

   return(retvalb);
}


// varnick is allocated by callee.
// Returns the nick at nickindex in varchannel.
// returns true on success, false on failure.
// nickindex varies from 1 to NickCount
bool IRCNickLists::getNickInChannelAtIndex(char *varchannel, int nickindex, char *varnick) {
IRCNickChainExt *CurrentExt;
IRCNickChain *Current;
int cur_index;
bool retvalb = false;
bool found = false;

   TRACE();
   if ( (varchannel == NULL) || (varnick == NULL) ) {
      return(false);
   }

   varnick[0] = '\0';
   if (nickindex <= 0) return(false);

   WaitForMutex(IRCNickListsMutex);

   do {
  
      CurrentExt = getChannelMatch(varchannel);
      if (CurrentExt == NULL) break;
      if (CurrentExt->NickCount < nickindex) break;

      // We are here, so all values are correct. Just pick the
      // correct nick.
      Current = CurrentExt->NickListHead;

      cur_index = 0;
      while (Current != NULL) {
         cur_index++;
         if (cur_index == nickindex) {
            // We have spotted the nick we have been looking for.
            found = true;
            break;
         }
         Current = Current->Next;
      }

      // This shouldnt happen either.
      if (Current == NULL) break;

      if (found) {
         strcpy(varnick, Current->Nick);
         retvalb = true;
      }

   } while (false);

   ReleaseMutex(IRCNickListsMutex);

   return(retvalb);
}


void IRCNickLists::setNickIP(char *varnick, unsigned long ip) {
IRCNickChainExt *ChainHead;
IRCNickChain *Current;

   TRACE();
   if (varnick == NULL) {
      return;
   }

   WaitForMutex(IRCNickListsMutex);

   ChainHead = Head;
   while (ChainHead) {
      Current = getNickMatch(ChainHead->Channel, varnick);
      if (Current != NULL) {
         Current->NickIP = ip;
      }
      ChainHead = ChainHead->Next;
   }

   ReleaseMutex(IRCNickListsMutex);
}

unsigned long IRCNickLists::getNickIP(char *varnick) {
IRCNickChain *Current;
IRCNickChainExt *ChainHead;
unsigned long ip = IRCNICKIP_UNKNOWN;

   TRACE();
   if (varnick == NULL) {
      return(ip);
   }

   WaitForMutex(IRCNickListsMutex);

   ChainHead = Head;
   while (ChainHead) {
      Current = getNickMatch(ChainHead->Channel, varnick);
      if ((Current != NULL) && (Current->NickIP != ip)) {
         ip = Current->NickIP;
         break;
      }
      ChainHead = ChainHead->Next;
   }

   ReleaseMutex(IRCNickListsMutex);

   return(ip);
}

// We maintain this nick in ascending order. As its useful for:
// Ad synchronization.
void IRCNickLists::addNick(char *varchannel, char *varnick, unsigned int varmode) {
IRCNickChainExt *CurrentExt;
IRCNickChain *Current, *preCurrent, *TmpLink;


   TRACE();
   if ( (varchannel == NULL) || (varnick == NULL) ) return;

   WaitForMutex(IRCNickListsMutex);

   // Lets check if the nick is already in the list.
   Current = getNickMatch(varchannel, varnick);
   if (Current != NULL) {
      // If Nick is already present, update its mode.
      Current->NickMode = varmode;
      ReleaseMutex(IRCNickListsMutex);
      return;
   }

   CurrentExt = getChannelMatch(varchannel);

   // Add it in the Head. And try to bubble it in the right place.

   Current = new IRCNickChain;
   Current->Next = CurrentExt->NickListHead;
   Current->Nick = new char[strlen(varnick) + 1];
   strcpy(Current->Nick, varnick);
   Current->NickMode = varmode;
   Current->NickClient = IRCNICKCLIENT_UNKNOWN;
   Current->NickIP = IRCNICKIP_UNKNOWN;
   Current->NickFirewall = IRCNICKFW_UNKNOWN;
   CurrentExt->NickListHead = Current;
   CurrentExt->NickCount++;

   // Lets now bubble our guy up.
   preCurrent = NULL;

   while (Current->Next != NULL) {
      // Check if the Nicks need to swap places.
      if (strcasecmp(Current->Nick, Current->Next->Nick) > 0) {
         // CurrentNick is bigger than the Next Nick = swap.
         if (preCurrent == NULL) {
            // Change in the Head of List.
            CurrentExt->NickListHead = Current->Next;
            TmpLink = Current->Next->Next;
            Current->Next->Next = Current;
            Current->Next = TmpLink;
            preCurrent = CurrentExt->NickListHead;
         }
         else {
               TmpLink = Current->Next->Next;
               Current->Next->Next = Current;
               preCurrent->Next = Current->Next;
               Current->Next = TmpLink;
               preCurrent = preCurrent->Next;
         }
         continue;
      } else break; // It is situated properly; we are done.
   }

   ReleaseMutex(IRCNickListsMutex);

   // As we just added this nick. Lets set its IP, Firewall, Client
   // if its known already from some other channel.
   char fw = getNickFirewall(varnick);
   setNickFirewall(varnick, fw);
   char client = getNickClient(varnick);
   setNickClient(varnick, client);
   unsigned long ip = getNickIP(varnick);
   setNickIP(varnick, ip);

   return;
}

int IRCNickLists::getNickCount(char *varchannel) {
IRCNickChainExt *CurrentExt;
int count;

   TRACE();
   if (varchannel == NULL) return(0);

   WaitForMutex(IRCNickListsMutex);

   CurrentExt = getChannelMatch(varchannel);
   
   if (CurrentExt == NULL) {
      count = 0;
   }
   else {
      count = CurrentExt->NickCount;
   }

   ReleaseMutex(IRCNickListsMutex);

   return(count);
}

bool IRCNickLists::isNickInChannel(char *varchannel, char *varnick) {
IRCNickChain *Current;

   TRACE();

   if ( (varchannel == NULL) || (varnick == NULL) ) {
      return(false);
   }

   WaitForMutex(IRCNickListsMutex);
   Current = getNickMatch(varchannel, varnick);
   if (Current == NULL) {
      ReleaseMutex(IRCNickListsMutex);
      return(false);
   }

   ReleaseMutex(IRCNickListsMutex);
   return(true);
}

// Private Function, Lock is held by caller.
IRCNickChain *IRCNickLists::getNickMatch(char *varchannel, char *varnick) {
IRCNickChainExt *CurrentExt;
IRCNickChain *Current;

   TRACE();
   if ( (varchannel == NULL) || (varnick == NULL) ) {
      return(NULL);
   }

   CurrentExt = getChannelMatch(varchannel);
   if (CurrentExt == NULL) {
      return(NULL);
   }

   Current = CurrentExt->NickListHead;
   while (Current != NULL) {
      if (strcasecmp(Current->Nick, varnick) == 0) {
         break;
      }
      Current = Current->Next;
   }
   return(Current);
}

// Returns the Firewalled/Unknown/Maybe MM index of the given nick in the 
// given channel.
// Returns 0 if channel doesnt exist or nick is not in channel or is not
// an MM nick
// else index is from 1 to FWMMCount.
int IRCNickLists::getNickFWMMindex(char *varchannel, char *varnick) {
IRCNickChainExt *CurrentExt;
IRCNickChain *Current;
int index;
bool found = false;

   TRACE();
   if ( (varchannel == NULL) || (varnick == NULL) ) {
      return(0);
   }

   WaitForMutex(IRCNickListsMutex);

   CurrentExt = getChannelMatch(varchannel);
   if (CurrentExt == NULL) {
      ReleaseMutex(IRCNickListsMutex);
      return(0);
   }

   Current = CurrentExt->NickListHead;
   index = 0;
   while (Current != NULL) {
      if ( (Current->NickClient == IRCNICKCLIENT_MASALAMATE) &&
           (Current->NickFirewall != IRCNICKFW_NO) ) {
         index++;
         if (strcasecmp(Current->Nick, varnick) == 0) {
            // We have spotted the nick we have been looking for.
            found = true;
            break;
         }
      }
      Current = Current->Next;
   }

   ReleaseMutex(IRCNickListsMutex);

   if (found) {
      return(index);
   }
   else {
      return(0);
   }
}

// Returns the Not Firewalled MM index of the given nick in the given
// channel.
// Returns 0 if channel doesnt exist or nick is not in channel or is not
// an MM nick
// else index is from 1 to NFMMCount.
int IRCNickLists::getNickNFMMindex(char *varchannel, char *varnick) {
IRCNickChainExt *CurrentExt;
IRCNickChain *Current;
int index;
bool found = false;

   TRACE();
   if ( (varchannel == NULL) || (varnick == NULL) ) {
      return(0);
   }

   WaitForMutex(IRCNickListsMutex);

   CurrentExt = getChannelMatch(varchannel);
   if (CurrentExt == NULL) {
      ReleaseMutex(IRCNickListsMutex);
      return(0);
   }

   Current = CurrentExt->NickListHead;
   index = 0;
   while (Current != NULL) {
      if ( (Current->NickClient == IRCNICKCLIENT_MASALAMATE) &&
           (Current->NickFirewall == IRCNICKFW_NO) ) {
         index++;
         if (strcasecmp(Current->Nick, varnick) == 0) {
            // We have spotted the nick we have been looking for.
            found = true;
            break;
         }
      }
      Current = Current->Next;
   }

   ReleaseMutex(IRCNickListsMutex);

   if (found) {
      return(index);
   }
   else {
      return(0);
   }
}

// Returns the MM index of the given nick in the given channel.
// Returns 0 if channel doesnt exist or nick is not in channel or is not
// an MM nick.
// else index is from 1 to MMNickCount.
int IRCNickLists::getMMNickIndex(char *varchannel, char *varnick) {
IRCNickChainExt *CurrentExt;
IRCNickChain *Current;
int index;
bool found = false;

   TRACE();
   if ( (varchannel == NULL) || (varnick == NULL) ) {
      return(0);
   }

   WaitForMutex(IRCNickListsMutex);

   CurrentExt = getChannelMatch(varchannel);
   if (CurrentExt == NULL) {
      ReleaseMutex(IRCNickListsMutex);
      return(0);
   }

   Current = CurrentExt->NickListHead;
   index = 0;
   while (Current != NULL) {
      if (Current->NickClient == IRCNICKCLIENT_MASALAMATE) {
         index++;
         if (strcasecmp(Current->Nick, varnick) == 0) {
            // We have spotted the nick we have been looking for.
            found = true;
            break;
         }
      }
      Current = Current->Next;
   }

   ReleaseMutex(IRCNickListsMutex);

   if (found) {
      return(index);
   }
   else {
      return(0);
   }
}

// Returns the number of MM clients whose firewall state is NOT
// IRCNICKFW_NO, ie all the others, Unknown/Firewalled/Maybe
// 0 is returned if none or no such channel.
int IRCNickLists::getFWMMcount(char *varchannel) {
IRCNickChainExt *CurrentExt;
IRCNickChain *Current;
int count;

   TRACE();
   if (varchannel == NULL) {
      return(0);
   }

   WaitForMutex(IRCNickListsMutex);
   CurrentExt = getChannelMatch(varchannel);
   if (CurrentExt == NULL) {
      ReleaseMutex(IRCNickListsMutex);
      return(0);
   }

   Current = CurrentExt->NickListHead;
   count = 0;
   while (Current != NULL) {
      if ( (Current->NickClient == IRCNICKCLIENT_MASALAMATE) &&
           (Current->NickFirewall != IRCNICKFW_NO) ) {
         count++;
      }
      Current = Current->Next;
   }

   ReleaseMutex(IRCNickListsMutex);

   return(count);
}

// Returns the number of MM clients whose firewall state is
// IRCNICKFW_NO => non firewalled.
// 0 is returned if none or no such channel.
int IRCNickLists::getNFMMcount(char *varchannel) {
IRCNickChainExt *CurrentExt;
IRCNickChain *Current;
int count;

   TRACE();
   if (varchannel == NULL) {
      return(0);
   }

   WaitForMutex(IRCNickListsMutex);
   CurrentExt = getChannelMatch(varchannel);
   if (CurrentExt == NULL) {
      ReleaseMutex(IRCNickListsMutex);
      return(0);
   }

   Current = CurrentExt->NickListHead;
   count = 0;
   while (Current != NULL) {
      if ( (Current->NickClient == IRCNICKCLIENT_MASALAMATE) &&
           (Current->NickFirewall == IRCNICKFW_NO) ) {
         count++;
      }
      Current = Current->Next;
   }

   ReleaseMutex(IRCNickListsMutex);

   return(count);
}

// Returns the number of MM clients including ourself in the channel.
// 0 is returned if no such channel.
int IRCNickLists::getMMNickCount(char *varchannel) {
IRCNickChainExt *CurrentExt;
IRCNickChain *Current;
int count;

   TRACE();
   if (varchannel == NULL) {
      return(0);
   }

   WaitForMutex(IRCNickListsMutex);
   CurrentExt = getChannelMatch(varchannel);
   if (CurrentExt == NULL) {
      ReleaseMutex(IRCNickListsMutex);
      return(0);
   }

   Current = CurrentExt->NickListHead;
   count = 0;
   while (Current != NULL) {
      if (Current->NickClient == IRCNICKCLIENT_MASALAMATE) {
         count++;
      }
      Current = Current->Next;
   }

   ReleaseMutex(IRCNickListsMutex);

   return(count);
}


// Returns the character which represents the mode of nick.
// Example, @ for op, % for hop, + for voice, space for normal user.
// Used by FromServerThr, to tag nick with the mode character for UI.
char IRCNickLists::getNickCharMode(char *varchannel, char *varnick) {
IRCNickChain *Current;
unsigned int mode;
char charmode;

   TRACE();
   if ( (varchannel == NULL) || (varnick == NULL) ) {
      return(' ');
   }

   WaitForMutex(IRCNickListsMutex);
   Current = getNickMatch(varchannel, varnick);
   if (Current == NULL) {
      mode = 0;
   }
   else {
      mode = Current->NickMode;
   }
   ReleaseMutex(IRCNickListsMutex);

   if (IS_OWNER(mode) || IS_SOP(mode) || IS_OP(mode) ) {
      charmode = '@';
   }
   else if (IS_HALFOP(mode)) {
      charmode = '%';
   }
   else if (IS_VOICE(mode)) {
      charmode = '+';
   }
   else {
      charmode = ' ';
   }
   return(charmode);
}

unsigned int IRCNickLists::getNickMode(char *varchannel, char *varnick) {
IRCNickChain *Current;
unsigned int mode;

   TRACE();
   if ( (varchannel == NULL) || (varnick == NULL) ) {
      return(0);
   }

   WaitForMutex(IRCNickListsMutex);
   Current = getNickMatch(varchannel, varnick);
   if (Current == NULL) {
      mode = 0;
   }
   else {
      mode = Current->NickMode;
   }
   ReleaseMutex(IRCNickListsMutex);
   return(mode);
}

void IRCNickLists::setNickMode(char *varchannel, char *varnick, unsigned int varmode) {
IRCNickChain *Current;

   TRACE();
   if ( (varchannel == NULL) || (varnick == NULL) ) {
      return; 
   }

   WaitForMutex(IRCNickListsMutex);

   Current = getNickMatch(varchannel, varnick);

   if (Current != NULL) {
      Current->NickMode = varmode;
   }
   ReleaseMutex(IRCNickListsMutex);
}

void IRCNickLists::changeNick(char *varchannel, char *orgnick, char *newnick) {
IRCNickChain *Current;

   TRACE();
   if ( (varchannel == NULL) || (orgnick == NULL) || (newnick == NULL) ) return;

   WaitForMutex(IRCNickListsMutex);
   Current = getNickMatch(varchannel, orgnick);
   if (Current != NULL) {
   unsigned int SaveNickMode;
   char SaveNickClient;
   unsigned long SaveNickIP;
   char SaveNickFirewall;
      // We got the entry. Lets save it and del and add. To maintain the
      // sorting order.
      // Things to save are: NickMode, NickClient, NickIP, NickFirewall
      SaveNickMode = Current->NickMode;
      SaveNickClient = Current->NickClient;
      SaveNickIP = Current->NickIP;
      SaveNickFirewall = Current->NickFirewall;

      ReleaseMutex(IRCNickListsMutex);

      // Delete the nick now.
      delNick(varchannel, orgnick);

      // Lets add the nick with new name now.
      addNick(varchannel, newnick, SaveNickMode);

      // Now lets set its other attributes.
      WaitForMutex(IRCNickListsMutex);
      Current = getNickMatch(varchannel, newnick);
      if (Current != NULL) {
         // We should always comes here. unless nick quit after changing name
         Current->NickClient = SaveNickClient;
         Current->NickIP = SaveNickIP;
         Current->NickFirewall = SaveNickFirewall;
      }
   }
   ReleaseMutex(IRCNickListsMutex);
}

int IRCNickLists::getChannelCount() {
   TRACE();
   return(ChannelCount);
}

void IRCNickLists::printDebug() {
int i = 1;
int totalc;
char C_Name[128];

   TRACE();
   totalc = getChannelCount();

   for (i = 1; i <= totalc; i++) {
      COUT(cout << "Channel " << i << " of " << totalc << " ";)
      getChannelName(C_Name, i);
      COUT(cout << C_Name << endl;)

      printDebugNicks(C_Name);
   }
}

void IRCNickLists::printDebugNicks(char *varchannel) {
int i = 1;
int totaln;
IRCNickChainExt *CurrentExt;
IRCNickChain *Current;

   TRACE();
   if (varchannel == NULL) return;

   totaln = getNickCount(varchannel);

   WaitForMutex(IRCNickListsMutex);

   CurrentExt = getChannelMatch(varchannel);
   if (CurrentExt == NULL) {
      ReleaseMutex(IRCNickListsMutex);
      return;
   }

   Current = CurrentExt->NickListHead;
   while (Current != NULL) {
      COUT(cout << "Nick: " << i++ << " of " << totaln << " " << Current->Nick << " Mode: ";)

      if (IS_REGULAR(Current->NickMode)) COUT(cout << "REG ";)
      if (IS_VOICE(Current->NickMode)) COUT(cout << "VOICE ";)
      if (IS_HALFOP(Current->NickMode)) COUT(cout << "HALFOP ";)
      if (IS_OP(Current->NickMode)) COUT(cout << "OP ";)

      switch (Current->NickClient) {
        case IRCNICKCLIENT_UNKNOWN:
           COUT(cout << " Client: Unknown";)
           break;

        case IRCNICKCLIENT_MASALAMATE:
           COUT(cout << " Client: MasalaMate";)  
           break;

        case IRCNICKCLIENT_SYSRESET:
           COUT(cout << " Client: SysReset";)
           break;

        case IRCNICKCLIENT_IROFFER:
           COUT(cout << " Client: Iroffer";)
           break;
      }

      switch (Current->NickFirewall) {
        case IRCNICKFW_UNKNOWN:
           COUT(cout << " FW: Unknown";)
           break;

        case IRCNICKFW_MAYBE:
           COUT(cout << " FW: Maybe";)
           break;

        case IRCNICKFW_NO:
           COUT(cout << " FW: NO";)
           break;

        case IRCNICKFW_YES:
           COUT(cout << " FW: YES";)
           break;
      }

      switch (Current->NickIP) {
        case IRCNICKIP_UNKNOWN:
           COUT(cout << " IP: Unknown";)
           break;

        default:
           COUT(cout << " IP: " << Current->NickIP;)
           break;
      }

      COUT(cout << endl;)
      Current = Current->Next;
   }

   ReleaseMutex(IRCNickListsMutex);

   return;
}
