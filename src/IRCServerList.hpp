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

#ifndef CLASS_IRCSERVERCHAIN
#define CLASS_IRCSERVERCHAIN

#include "Compatibility.hpp"

/*
  This maintains a linked list of Servers.
  Adding Servers to this list is done only if its not already present.
  You use this class to keep track of Servers, and if connected to that
  server or not.
*/


typedef struct IRCServerChain {
   char *Server;
   unsigned short Port;
   bool Joined;
   struct IRCServerChain *Next;
} IRCServerChain;

class IRCServerList {
public:
   IRCServerList();
   ~IRCServerList();
   IRCServerList& operator=(const IRCServerList&);
   bool operator==(const IRCServerList&);
   void addServer(char* varserver, unsigned short varport = 6667);
   char * const getServer(unsigned short &varport, int serverindex);
   int getServerCount();
   bool isServer(char *varserver, unsigned short varport = 6667);
   void setJoined(char *varserver, unsigned short varport = 6667);
   void setNotJoined(char *varserver, unsigned short varport = 6667);
   bool isJoined(char *varserver, unsigned short varport = 6667);
   void printDebug();

private:
   IRCServerChain *Head;
   int ServerCount;
   IRCServerChain *getMatch(char *varchannel, unsigned short varport = 6667);
   void freeAll();
};


#endif
