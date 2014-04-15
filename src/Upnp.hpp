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

#ifndef CLASS_UPNP
#define CLASS_UPNP

#include "Compatibility.hpp"

#define UPNP_TIMEOUT 30

typedef enum {
   UPNP_UNKNOWN,
   UPNP_NOTDETECTED,
   UPNP_LOCATION_DETECTED,
   UPNP_CONTROL_UNDETECTED,
   UPNP_OK
} UpnpState;

class Upnp {
public:
   Upnp();
   ~Upnp();

   // Returns the State we are in.
   UpnpState getState();

   // This does the search. true => success
   // Connection is one of WANIPConnection:1 or WANPPPconnection:1 as of now.
   bool issueSearch(char *Connection);
 
   // Issues request for getting external IP. true => success
   bool issueGetExternalIP();

   // gets our external IP. // Returns NULL if we dont have it.
   const char *getExternalIP();

   // gets specific port Mapping.
   const char *getPortMappingAtIndex(int);

   // Issues request for adding a port mapping.
   bool issueAddPortMapping(int port, char *description);

   // Issues a request for deleting a port mapping.
   bool issueDeletePortMapping(int port);

   void printDebug();

private:
   void init(); // Common initialiser.

   // To extract the value string from xml of a particular element.
   // caller needs to free the string returned.
   char *extractValueFromXML(char *xml, char *element);

   // Check for HTTP/1.1 200 OK
   bool isHTTPOK(char *buf);

   // Form the HTTP POST request.
   // Req is allocated by caller.
   void formPOSTRequest(char *Req, char *Interface, char *Command, char *Body);

   UpnpState State;

   char *Connection;
   char *ExternalIP;
   char LocalIP[20];

   char *ControlURL;
   char ControlIP[20];
   int  ControlPort;
};


#endif
