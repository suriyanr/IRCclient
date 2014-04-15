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

#include "IRCServerList.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

// IRCServerList Constructor class.
IRCServerList::IRCServerList() {
   TRACE();
   Head = NULL;
   ServerCount = 0;
}

// IRCServerList Destructor class.
IRCServerList::~IRCServerList() {
   TRACE();
   freeAll();
}

// private route to dee memory used by list.
void IRCServerList::freeAll() {
IRCServerChain *Current;

   TRACE();
   while (Head != NULL) {
      Current = Head->Next;
      delete [] Head->Server;
      delete Head;
      Head = Current;
   }
   Head = NULL;
   ServerCount = 0;
}

// == operator.
bool IRCServerList::operator==(const IRCServerList &SL) {
IRCServerChain *src_cur;
bool retvalb;

   TRACE();
   if (this == &SL) return(true);

   src_cur = SL.Head;
   while (src_cur != NULL) {
      if (isServer(src_cur->Server, src_cur->Port) == false) break;
      src_cur = src_cur->Next;
   }
   if (src_cur == NULL) retvalb = true;
   else retvalb = false;

// Take care of exception when SL.Head is NULL and this->Head is not.
   if ( (SL.Head == NULL) && (this->Head != NULL) ) retvalb = false;

   return(retvalb);
}

// = operator.
IRCServerList& IRCServerList::operator=(const IRCServerList &SL) {
IRCServerChain *src_cur;

   TRACE();
   if (this == &SL) return (*this);

   freeAll();

// Lets replicate the list here from SL.
   src_cur = SL.Head;
   while (src_cur != NULL) {
      addServer(src_cur->Server, src_cur->Port);
      src_cur = src_cur->Next;
   }
}

void IRCServerList::addServer(char *varserver, unsigned short port) {
IRCServerChain *Current;

   TRACE();
   if (varserver == NULL) return;

// Lets check if the (Server, Port)  is already in the list.
   if (isServer(varserver, port) == true) {
      return;
   }

   Current = new IRCServerChain;
   Current->Next = Head;
   Current->Server = new char[strlen(varserver) + 1];
   strcpy(Current->Server, varserver);
   Current->Port = port;
   Head = Current;
   ServerCount++;
   return;
}

// Return the i'th server and port.
char * const IRCServerList::getServer(unsigned short &varport, int serindex) {
char *retstr = NULL;
int i = 0;
IRCServerChain *Current;

   TRACE();
   Current = Head;
   while (Current != NULL) {
      i++;
      if (i == serindex) {
         retstr = Current->Server;
         varport = Current->Port;
         break;
      } 
      Current = Current->Next;
   }
   return(retstr);
}

int IRCServerList::getServerCount() {
   TRACE();
   return(ServerCount);
}

bool IRCServerList::isServer(char *varserver, unsigned short varport) {
IRCServerChain *Current;

   TRACE();
   if (varserver == NULL) {
      return(false);
   }
   Current = getMatch(varserver, varport);
   if (Current == NULL) {
      return(false);
   }
   return(true);
}

void IRCServerList::setJoined(char *varserver, unsigned short varport) {
IRCServerChain *Current;

   TRACE();
   Current = getMatch(varserver, varport);
   if (Current == NULL) {
      return;
   }
   Current->Joined = true;
}

void IRCServerList::setNotJoined(char *varserver, unsigned short varport) {
IRCServerChain *Current;

   TRACE();
   Current = getMatch(varserver, varport);
   if (Current == NULL) {
      return;
   }
   Current->Joined = false;
}

bool IRCServerList::isJoined(char *varserver, unsigned short varport) {
IRCServerChain *Current;

   TRACE();
   Current = getMatch(varserver, varport);
   if (Current == NULL) {
      return(false);
   }
   return(Current->Joined);
}

IRCServerChain *IRCServerList::getMatch(char *varserver, unsigned short varport) {
IRCServerChain *Current;

   TRACE();
   if (varserver == NULL) {
      return(NULL);
   }
   Current = Head;
   while (Current != NULL) {
      if ( (strcasecmp(Current->Server, varserver) == 0) && (Current->Port == varport) ) {
         return(Current);
      }
      Current = Current->Next;
   }
   return(NULL);
}

void IRCServerList::printDebug() {
IRCServerChain *tmp;
int i = 1;

   TRACE();
   tmp = Head;
   while (tmp != NULL) {
      COUT(cout << "Server: " << i++ << " of " << ServerCount << " "<< tmp->Server << " Port: " << tmp->Port << endl;)
      tmp = tmp->Next;
   }
   return;
}
