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

#ifndef CLASS_IRCNICKLISTS
#define CLASS_IRCNICKLISTS

#ifdef __MINGW32__
#  include <windows.h>
#else
#  include <pthread.h>
#  include <semaphore.h>
#endif


#include "Compatibility.hpp"

/*
  This maintains a linked list of Nick names in ascending order.
  Adding Nicks to this list is done only if its not already present.
  You use this class to keep track of Nicks.
*/

// defines for NickIP
#define IRCNICKIP_UNKNOWN 0

// defines for NickFirewall. Start with IRCNICKFW_MAYBE
#define IRCNICKFW_YES     'F'
#define IRCNICKFW_NO      'N'
#define IRCNICKFW_MAYBE   'M'
#define IRCNICKFW_UNKNOWN 'U'

typedef struct IRCNickChain {
   char *Nick;
   unsigned int NickMode;
   char NickClient; // If possible we note the Client he uses.
   char NickFirewall; // If possible we note down his Firewall state.
   unsigned long NickIP;    // If possible we note down the IP.
   struct IRCNickChain *Next;
} IRCNickChain;

typedef struct IRCNickChainExt {
  char *Channel;
  int NickCount;
  IRCNickChain *NickListHead;
  struct IRCNickChainExt *Next;
} IRCNickChainExt;

#define IRCNICKMODE_REGULAR 0x01
#define IRCNICKMODE_VOICE   0x02
#define IRCNICKMODE_HALFOP  0x04
#define IRCNICKMODE_OP      0x08
#define IRCNICKMODE_SOP     0x10
#define IRCNICKMODE_OWNER   0x20

// Helpful Macros.
#define IS_OWNER(mode)       ((mode) & IRCNICKMODE_OWNER)
#define IS_SOP(mode)         ((mode) & IRCNICKMODE_SOP)
#define IS_OP(mode)          ((mode) & IRCNICKMODE_OP)
#define IS_HALFOP(mode)      ((mode) & IRCNICKMODE_HALFOP)
#define IS_VOICE(mode)       ((mode) & IRCNICKMODE_VOICE)
#define IS_REGULAR(mode)     ((mode) & IRCNICKMODE_REGULAR)

#define IS_MORE_THAN_REGULAR(mode)         ((mode) & (~IRCNICKMODE_REGULAR))

#define ADD_OWNER(mode)      ((mode) | IRCNICKMODE_OWNER | IRCNICKMODE_REGULAR)
#define ADD_SOP(mode)        ((mode) | IRCNICKMODE_SOP | IRCNICKMODE_REGULAR)
#define ADD_OP(mode)         ((mode) | IRCNICKMODE_OP | IRCNICKMODE_REGULAR)
#define ADD_HALFOP(mode)     ((mode) | IRCNICKMODE_HALFOP | IRCNICKMODE_REGULAR)
#define ADD_VOICE(mode)      ((mode) | IRCNICKMODE_VOICE | IRCNICKMODE_REGULAR)
#define ADD_REGULAR(mode)    ((mode) | IRCNICKMODE_REGULAR)

#define REMOVE_OWNER(mode)   (((mode) & (~IRCNICKMODE_OWNER)) | IRCNICKMODE_REGULAR)
#define REMOVE_SOP(mode)     (((mode) & (~IRCNICKMODE_SOP)) | IRCNICKMODE_REGULAR)
#define REMOVE_OP(mode)      (((mode) & (~IRCNICKMODE_OP)) | IRCNICKMODE_REGULAR)
#define REMOVE_HALFOP(mode)  (((mode) & (~IRCNICKMODE_HALFOP)) | IRCNICKMODE_REGULAR)
#define REMOVE_VOICE(mode)   (((mode) & (~IRCNICKMODE_VOICE)) | IRCNICKMODE_REGULAR)

class IRCNickLists {
public:
   IRCNickLists();
   ~IRCNickLists();
   IRCNickLists& operator=(const IRCNickLists&);
   void addNick(char *varchannel, char* varnick, unsigned int varmode);
   void delNick(char *varchannel, char* varnick);
   void addChannel(char *varchannel);
   void delChannel(char *varchannel);
   void getChannelName(char *varchannel, int chanindex);
   bool isChannel(char *varchannel);
   int  getChannelCount();
   unsigned int getNickMode(char *varchannel, char *varnick);
   char getNickCharMode(char *varchannel, char *varnick);
   void setNickMode(char *varchannel, char *varnick, unsigned int varmode);

   // Below do not depend on which channel Nick is on.
   void setNickClient(char *varnick, char client);
   void setNickIP(char *varnick, unsigned long ip);
   void setNickFirewall(char *varnick, char fw);
   char getNickFirewall(char *varnick);
   char getNickClient(char *varnick);
   unsigned long getNickIP(char *varnick);

   void changeNick(char *varchannel, char *orgnick, char *newnick);
   bool getNickInChannelAtIndex(char *varchannel, int nickindex, char *varnick);
   int getNickCount(char *varchannel);
   bool isNickInChannel(char *varchannel, char *varnick);

   // Below 2 for Ad sync algo
   int getMMNickCount(char *varchannel);
   int getMMNickIndex(char *varchannel, char *varnick);

   // Below for Propagation Algo.
   int getFWMMcount(char *varchannel);
   int getNFMMcount(char *varchannel);
   int getNickFWMMindex(char *varchannel, char *varnick);
   int getNickNFMMindex(char *varcahnnel, char *varnick);
   bool getNickInChannelAtIndexFWMM(char *varchannel, int nickindex, char *varnick);
   bool getNickInChannelAtIndexNFMM(char *varchannel, int nickindex, char *varnick);

   void printDebug();

private:
   IRCNickChainExt *Head;
   int ChannelCount;
   IRCNickChain *getNickMatch(char *varchannel, char *varnick);
   IRCNickChainExt *getChannelMatch(char *varchannel);
   void freeChannel(IRCNickChainExt *CurrentExt);
   void printDebugNicks(char *varchannel);
   void freeAll();

// A mutex to serialise its access as it will exist in XChange Class
// and a seperate mutex wont be required to control it there.
   MUTEX IRCNickListsMutex;
};


#endif
