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

#include <string.h>

#include "StackTrace.hpp"
#include "ConnectionMethod.hpp"

// Constructor.
ConnectionMethod::ConnectionMethod() {
   TRACE();
   how = CM_UNKNOWN;
   host = NULL;
   port = 0;
   user = NULL;
   password = NULL;
   vhost = NULL;
}

// Destructor
ConnectionMethod::~ConnectionMethod() {
   TRACE();
   freeAll();
}

// == operator
bool ConnectionMethod::operator==(const ConnectionMethod &ChkCM) {
bool retvalb = false;
char *str1, *str2;

   TRACE();

   if (this == &ChkCM) return(true);

   do {
      if (ChkCM.how != this->how) break;

      if (ChkCM.port != this->port) break;

      str1 = ChkCM.host;
      str2 = this->host;
      if (str1 && str2 && strcasecmp(str1, str2)) break;
      if ( (str1 == NULL) && str2) break;
      if ( (str2 == NULL) && str1) break;
      // We come here if both are NULL, or both are non null, but same str.

      str1 = ChkCM.user;
      str2 = this->user;
      if (str1 && str2 && strcasecmp(str1, str2)) break;
      if ( (str1 == NULL) && str2) break;
      if ( (str2 == NULL) && str1) break;
      // We come here if both are NULL, or both are non null, but same str.

      str1 = ChkCM.password;
      str2 = this->password;
      if (str1 && str2 && strcasecmp(str1, str2)) break;
      if ( (str1 == NULL) && str2) break;
      // We come here if both are NULL, or both are non null, but same str.

      str1 = ChkCM.vhost;
      str2 = this->vhost;
      if (str1 && str2 && strcasecmp(str1, str2)) break;
      if ( (str1 == NULL) && str2) break;
      if ( (str2 == NULL) && str1) break;
      // We come here if both are NULL, or both are non null, but same str.

      // Everything is the same.
      retvalb = true;

   } while (false);

   return(retvalb);
}

// = operator
ConnectionMethod& ConnectionMethod::operator=(const ConnectionMethod &CM) {

   TRACE();
   if (this == &CM) return(*this);

   freeAll();

   how = CM.how;
   port = CM.port;
   if (CM.host != NULL) {
      host = new char [strlen(CM.host) + 1];
      strcpy(host, CM.host);
   }
   if (CM.user != NULL) {
      user = new char [strlen(CM.user) + 1];
      strcpy(user, CM.user);
   }
   if (CM.password != NULL) {
      password = new char [strlen(CM.password) + 1];
      strcpy(password, CM.password);
   }
   if (CM.vhost != NULL) {
      vhost = new char [strlen(CM.vhost) + 1];
      strcpy(vhost, CM.vhost);
   }
}

void ConnectionMethod::freeAll() {
   TRACE();
   delete [] host;
   host = NULL;
   delete [] user;
   user = NULL;
   delete [] password;
   password = NULL;
   delete [] vhost;
   vhost = NULL;
   port = 0;
}

void ConnectionMethod::setDirect() {
   TRACE();
   freeAll();
   how = CM_DIRECT;
}

void ConnectionMethod::setBNC(const char *bnchost, unsigned short bncport, const char *bncuser, const char *bncpassword, const char *bncvhost) {
   TRACE();
   freeAll();
   how = CM_BNC;
   if (bnchost != NULL) {
      host = new char[strlen(bnchost) + 1];
      strcpy(host, bnchost);
   }
   port = bncport;
   if (bncuser != NULL) {
      user = new char[strlen(bncuser) + 1];
      strcpy(user, bncuser);
   }
   if (bncpassword != NULL) {
      password = new char[strlen(bncpassword) + 1];
      strcpy(password, bncpassword);
   }
   if (bncvhost != NULL) {
      vhost = new char[strlen(bncvhost) + 1];
      strcpy(vhost, bncvhost);
   }
}

void ConnectionMethod::setWingate(const char *wingatehost, unsigned short wingateport) {
   TRACE();
   freeAll();
   how = CM_WINGATE;
   port = wingateport;
   if (wingatehost != NULL) {
      host = new char[strlen(wingatehost) + 1];
      strcpy(host, wingatehost);
   }
}


void ConnectionMethod::setSocks4(const char *socks4host, unsigned short socks4port, const char *socks4user) {
   TRACE();
   freeAll();
   how = CM_SOCKS4;
   port = socks4port;
   if (socks4host != NULL) {
      host = new char[strlen(socks4host) + 1];
      strcpy(host, socks4host);
   }
   if (socks4user != NULL) {
      user = new char[strlen(socks4user) + 1];
      strcpy(user, socks4user);
   }
}

void ConnectionMethod::setSocks5(const char *socks5host, unsigned short socks5port, const char *socks5user, const char *socks5password) {
   TRACE();
   freeAll();
   how = CM_SOCKS5;
   port = socks5port;
   if (socks5host != NULL) {
      host = new char[strlen(socks5host) + 1];
      strcpy(host, socks5host);
   }
   if (socks5user != NULL) {
      user = new char[strlen(socks5user) + 1];
      strcpy(user, socks5user);
   }
   if (socks5password != NULL) {
      password = new char[strlen(socks5password) + 1];
      strcpy(password, socks5password);
   }
}

void ConnectionMethod::setProxy(const char *proxyhost, unsigned short proxyport, const char *proxyuser, const char *proxypassword) {
   TRACE();
   freeAll();
   if ( (proxyhost != NULL) && strlen(proxyhost) ) {
      host = new char[strlen(proxyhost) + 1];
      strcpy(host, proxyhost);
   }
   else return; // Just return if host is invalid
   how = CM_PROXY;
   port = proxyport;
   if ( (proxyuser != NULL) && strlen(proxyuser) ) {
      user = new char[strlen(proxyuser) + 1];
      strcpy(user, proxyuser);
   }
   if ( (proxypassword != NULL) && strlen(proxypassword) ) {
      password = new char[strlen(proxypassword) + 1];
      strcpy(password, proxypassword);
   }
}

ConnectionHowE ConnectionMethod::howto() {
   TRACE();
   return(how);
}

bool ConnectionMethod::valid() {
   TRACE();
   switch (how) {
      case CM_DIRECT:
         return(true);
         break;

      case CM_BNC:
      case CM_WINGATE:
      case CM_SOCKS4:
      case CM_SOCKS5:
      case CM_PROXY:
         if ( (host != NULL) && (port != 0) ) return(true);
         break;
   }
   return(false);
}

char* const ConnectionMethod::getHost() {
   TRACE();
   return(host);
}

unsigned short ConnectionMethod::getPort() {
   TRACE();
   return(port);
}

char* const ConnectionMethod::getUser() {
   TRACE();
   return(user);
}

char* const ConnectionMethod::getPassword() {
   TRACE();
   return(password);
}

char* const ConnectionMethod::getVhost() {
   TRACE();
   return(vhost);
}

void ConnectionMethod::printDebug() {
   TRACE();
   switch (how) {
      case CM_DIRECT:
         COUT(cout << "METHOD: DIRECT ";)
         break;

      case CM_BNC:
         COUT(cout << "METHOD: BNC ";)
         break;

      case CM_WINGATE:
         COUT(cout << "METHOD: WINGATE ";)
         break;

      case CM_SOCKS4:
         COUT(cout << "METHOD: SOCKS4 ";)
         break;

      case CM_SOCKS5:
         COUT(cout << "METHOD: SOCKS5 ";)
         break;

      case CM_PROXY:
         COUT(cout << "METHOD: PROXY ";)
         break;

      case CM_UNKNOWN:
         COUT(cout << "METHOD: UNKNOWN ";)
         break;
   }
   if (host != NULL) {
      COUT(cout << "HOST: " << host << " ";)
   }
   if (port != 0) {
      COUT(cout << "PORT: " << port << " ";)
   }
   if (user != NULL) {
      COUT(cout << "USER: " << user << " ";)
   }
   if (password != NULL) {
      COUT(cout << "PASSWORD: " << password << " ";)
   }
   if (vhost != NULL) {
      COUT(cout << "VHOST: " << vhost << " ";)
   }
   COUT(cout << endl;)
}
