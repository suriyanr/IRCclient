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

#ifndef CLASS_CONNECTIONMETHOD
#define CLASS_CONNECTIONMETHOD



#include "Compatibility.hpp"

// Enumerate the how of the connection.
typedef enum {
   CM_DIRECT = 1,
   CM_PROXY,
   CM_BNC,
   CM_SOCKS4,
   CM_SOCKS5,
   CM_WINGATE,
   CM_UNKNOWN
} ConnectionHowE;

// Contains all the relevant information to make a connection.
// If direct then none of the info is required.
// If bnc, host, port, user, password, vhost is used.
// If wingate, host, port, user, password is used.
// If socks4, host, port, user, password is used.
// If socks5, host, port, user, password is used.
// If proxy, host, port, user, password is used.
class ConnectionMethod {
public:
   ConnectionMethod();
   ~ConnectionMethod();
   ConnectionMethod& operator=(const ConnectionMethod&);
   bool operator==(const ConnectionMethod&);
   void setDirect();
   void setBNC(const char *, unsigned short, const char *varuser, const char *varpassword=NULL, const char *varvhost = NULL);
   void setWingate(const char *, unsigned short);
   void setSocks4(const char *, unsigned short, const char *varuser=NULL);
   void setSocks5(const char *, unsigned short, const char *varuser=NULL, const char *varpassword=NULL);
   void setProxy(const char *, unsigned short, const char *varuser=NULL, const char *varpassword=NULL);
   ConnectionHowE howto();
   bool valid();
   char* const getHost();
   unsigned short getPort();
   char* const getUser();
   char* const getPassword();
   char* const getVhost();
   void printDebug();

private:
   ConnectionHowE how;
   char           *host;
   unsigned short port;
   char           *user; // For bnc this is same as IRC Nick.
   char           *password;
   char           *vhost; // Used only with bnc ?
   void           freeAll();
};


#endif
