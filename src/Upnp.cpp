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

#include <stdio.h>
#include <string.h>

#include "TCPConnect.hpp"
#include "LineParse.hpp"
#include "Upnp.hpp"
#include "StackTrace.hpp"
#include "Utilities.hpp"

// Constructor.
Upnp::Upnp() {
   TRACE();

   Connection = NULL;
   ControlURL = NULL;
   ExternalIP = NULL;
   init();
}

void Upnp::init() {

   TRACE();

   State = UPNP_UNKNOWN;
   LocalIP[0] = '\0';
   delete [] ControlURL;
   delete [] ExternalIP;
   delete [] Connection;
   ControlURL = NULL;
   ExternalIP = NULL;
   Connection = NULL;
   ControlIP[0] = '\0';
   ControlPort = 0;
}

// Destructor.
Upnp::~Upnp() {
   TRACE();

   delete [] ControlURL;
   delete [] ExternalIP;
   delete [] Connection;
}

// This discovers the Router, its description URL, and int ControlURL.
// Connection is one of WANIPConnection:1 or WANPPPconnection:1 as of now.
bool Upnp::issueSearch(char *TryConnection) {
char initStrUPNP[512];
TCPConnect TCPupnp;
int retval;
bool retvalb = false;
SOCKET udps;
struct sockaddr_in broadcastDestAddr;
struct sockaddr_in bindServerAddr;
char *LocationURL = NULL;
LineParse LineP;
const char *parseptr;

   TCPupnp.setSetOptions(false);
   // ST = search term.
   // MX: 3 => reply after 3 seconds.
   sprintf(initStrUPNP,
           "M-SEARCH * HTTP/1.1\r\n"
           "Host: 239.255.255.250:1900\r\n"
           "ST: urn:schemas-upnp-org:service:%s\r\n"
           "Man: \"ssdp:discover\"\r\n"
           "MX: 3\r\n"
           "\r\n"
           "\r\n",
           TryConnection);

   State = UPNP_UNKNOWN;

   // BroadCast Destination setup
   memset(&broadcastDestAddr, 0, sizeof(broadcastDestAddr));
   broadcastDestAddr.sin_port = htons(1900);
   broadcastDestAddr.sin_family = AF_INET;
   broadcastDestAddr.sin_addr.s_addr = inet_addr("239.255.255.250");

   // Socket Creation.
   udps = socket(AF_INET, SOCK_DGRAM, 0);
   if (udps < 0) {
      return(retvalb);
   }

   // Bind local server port
   bindServerAddr.sin_family = AF_INET;
   bindServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   bindServerAddr.sin_port = htons(DCCSERVER_PORT);
   retval = bind(udps, (struct sockaddr *) &bindServerAddr, sizeof(bindServerAddr));
   if (retval < 0) {
      closesocket(udps);
      return(retvalb);
   }

   // Lets do the broadcast.
   retval = sendto(udps, initStrUPNP, strlen(initStrUPNP), 0,
                   (struct sockaddr *) &broadcastDestAddr,
                   sizeof(broadcastDestAddr));
   if (retval < 0) {
      closesocket(udps);
      return(retvalb);
   }

   do {
      struct timeval tv;
      struct timeval *tvp = &tv;
      fd_set grp;
 
      tv.tv_usec = 0;
      tv.tv_sec = 15; // we max will wait for 15 seconds for response to come.
      FD_ZERO(&grp);
      FD_SET(udps, &grp);

      int state = select(udps + 1, &grp, NULL, NULL, tvp);
 
      if (state != 1) {
         closesocket(udps);
         State = UPNP_NOTDETECTED;
         return(retvalb);
      }
      sleep(1); // Let all the message come in.
   } while (false);

   // Lets read in the message;
   do {
      char MsgBuf[1024];
      struct sockaddr_in cliAddr;
      int cliLen = sizeof(cliAddr);
      LineParse LineP2;
      int lines;

      retval = recvfrom(udps, MsgBuf, 1022, 0,
                        (struct sockaddr *) &cliAddr, (socklen_t *) &cliLen);
      if (retval < 0) {
         closesocket(udps);
         State = UPNP_NOTDETECTED;
         return(retvalb);
      }

      LineP = MsgBuf;
      parseptr = LineP.replaceString("\r\n", "\n");
      LineP = (char *) parseptr;
      LineP.setDeLimiter('\n');
      // We have LineP with lines ending in \n

      lines = LineP.getWordCount(); // These are the number of lines;
      for (int i = 1; i <= lines; i++) {
         parseptr = LineP.getWord(i); // Line i.
         LineP2 = (char *) parseptr;
         LineP2.setDeLimiter(':');
         parseptr = LineP2.getWord(1);
         if (strcasecmp(parseptr, "Location") == 0) {
            char *endptr;

            // This is the 
            // Location:http://192.168.0.1:80/upnp/service/descrip.xml
            // lets get host, port, path
            // word 3 is //192.168.0.1
            parseptr = LineP2.getWord(3);
            strcpy(ControlIP, &parseptr[2]); // Discarding the // at start.

            parseptr = LineP2.getWordRange(4, 0);
            // This is: 80/upnp/service/descrip.xml
            ControlPort = (int) strtoul(parseptr, &endptr, 10);
            LocationURL = new char[strlen(endptr) + 1];
            strcpy(LocationURL, endptr);
            COUT(cout << "Location URL: " << LocationURL << endl;)

            // We are done with the udp part.
            break;
         }
      }

      closesocket(udps);

      if ( (LocationURL == NULL) || (ControlPort == 0) ||
           (ControlIP[0] == '\0') ) {
         State = UPNP_NOTDETECTED;
         delete [] LocationURL;
         return(retvalb);
      }

   } while (false);

   // We are here => LocationURL, ControlIP, ControlPort is valid.
   State = UPNP_LOCATION_DETECTED;
   COUT(cout << "UPNP:search: ControlIP: " << ControlIP << " ControlPort: " << ControlPort << " LocationURL: " << LocationURL << endl;)

   // Lets issue the GET to the Location URL, and get the Control Information.
   do {
      char Buf[1024];

      sprintf(Buf, "GET %s HTTP/1.1\r\n"
                       "HOST: %s:%d\r\n"
                       "ACCEPT-LANGUAGE: en\r\n\r\n",
                       LocationURL,
                       ControlIP, ControlPort);
      delete [] LocationURL;

      TCPupnp.TCPConnectionMethod.setDirect();
      retvalb = TCPupnp.getConnection(ControlIP, ControlPort);
      if (retvalb == false) {
         State = UPNP_CONTROL_UNDETECTED;
         break;
      }

      retvalb = false;
      retval = TCPupnp.writeData(Buf, strlen(Buf));
      if (retval != strlen(Buf)) {
         State = UPNP_CONTROL_UNDETECTED;
         break;
      }

      bool getcontrol = false;
      // Now we receive the GET and capture the Control information.
      do {

         retval = TCPupnp.readLine(Buf, 1022, 15); // 15 sec timeout
         COUT(cout << "UPNP: TCPupnp.realLine() returned: " << retval << endl;)
         if (retval < 0) break;
         if (retval == 0) continue; // Empty line

         COUT(cout << "UPNP: " << Buf << endl;)
         LineP = Buf;

         if (getcontrol == true) {
            parseptr = LineP.replaceString("\t", " ");
            LineP = (char *) parseptr;
            parseptr = LineP.removeConsecutiveDeLimiters();
            LineP = (char *) parseptr;
            // We got a nice line getting rid of tabs and multiple spaces.

            if (LineP.isWordsInLine("controlURL")) {
               // Got the line we are looking for.
               LineP.setDeLimiter('>');

               // Get the URL
               parseptr = LineP.getWord(2);
               LineP = (char *) parseptr;
               LineP.setDeLimiter('<');
               parseptr = LineP.getWord(1);
               COUT(cout << "control url string: " << parseptr << endl;)
               LineP = (char *) parseptr;

               // Response could be:
               // /upnp/service/WANIPConnection
               // http://192.168.1.1:2468//WANIPConnection
               // So controlURL should just be /WANIPConnection.
               parseptr = LineP.replaceString("//", "/");
               LineP = (char *) parseptr;
               // LineP is one of below.
               // http:/192.168.1.1:2468/WANIPConnection
               // /upnp/service/WANIPConnection
               LineP.setDeLimiter(':');
               parseptr = LineP.getWord(1);
               if (strcasecmp(parseptr, "http") == 0) {
                  char *endptr;

                  // http:/192.168.1.1:2468/WANIPConnection
                  // We possibly get ControlIP and ControlPort again here.
                  // word 2 is /192.168.1.1
                  parseptr = LineP.getWord(2);
                  if (parseptr[0] == '/') {
                     strcpy(ControlIP, &parseptr[1]); // Discarding the /
                  }
                  COUT(cout << "ControlIP: " << ControlIP << endl;)
                  parseptr = LineP.getWordRange(3, 0);
                  // This is: 2468/WANIPConnection
                  int tmpPort = (int) strtoul(parseptr, &endptr, 10);
                  if (tmpPort != 0) {
                     ControlPort = tmpPort;
                  }
                  COUT(cout << "Control port: " << ControlPort << endl;)
                  delete [] ControlURL;
                  ControlURL = new char[strlen(endptr) + 1];
                  strcpy(ControlURL, endptr);
                  COUT(cout << "Control URL: " << ControlURL << endl;)
               }
               else {
                  parseptr = LineP.getWordRange(1, 0);
                  delete [] ControlURL;
                  ControlURL = new char[strlen(parseptr) + 1];
                  strcpy(ControlURL, parseptr);
                  COUT(cout << "ControlURL: " << parseptr << endl;)
               }

               // Lets copy the TryConnection.
               delete [] Connection;
               Connection = new char[strlen(TryConnection) + 1];
               strcpy(Connection, TryConnection);
               State = UPNP_OK;
               getcontrol = false;
            }
         }
         else {
            char tmpbuf[512];
            sprintf(tmpbuf, "urn:schemas-upnp-org:service:%s", TryConnection);
            if (LineP.isWordsInLine(tmpbuf)) {
               getcontrol = true;
            }
         }

      } while (true);

   } while (false);
    
   // In the end lets get our local IP address.
   // Lets get the Local Ip address.
   getInternalDottedIP(LocalIP);

   return(true);
}

bool Upnp::issueGetExternalIP() {
TCPConnect TCPupnp;
bool retvalb = false;
int retval;
char Buf[1024];
char Body[1024];

   TRACE();

   TCPupnp.setSetOptions(false);
   sprintf(Body,
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"\r\n"
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "<s:Body>\r\n"
        "<u:GetExternalIPAddress xmlns:u=\"urn:schemas-upnp-org:service:%s\">\r\n"
        "</u:GetExternalIPAddress>\r\n"
        "</s:Body>\r\n"
        "</s:Envelope>\r\n\r\n",
        Connection);
        


   if (State != UPNP_OK) {
      return(retvalb);
   }

   formPOSTRequest(Buf, Connection, "GetExternalIPAddress", Body);
   do {
       TCPupnp.TCPConnectionMethod.setDirect();
       retvalb = TCPupnp.getConnection(ControlIP, ControlPort);
       if (retvalb == false) break;

       retvalb = false;
       retval = TCPupnp.writeData(Buf, strlen(Buf));     
       if (retval != strlen(Buf)) {
          break;
       }

       // Lets wait for what he sends us back.
       do {
          retval = TCPupnp.readLine(Buf, 1022, 15); // 15 seconds time out.
          if (retval > 0) {

             // If its HTTP/1.1 200 OK then we are good.
             // So we search for " 200 OK " in the line.
             // COUT(cout << Buf << endl;)
             if (retvalb == false) {
                retvalb = isHTTPOK(Buf);
             }
             else {
                LineParse LineP;

                // Lets try to extract
/*
<NewExternalIPAddress>61.203.161.165</NewExternalIPAddress>
<NewExternalIPAddress xmlns:dt="urn:schemas-microsoft-com:datatypes" dt:dt="string">67.169.190.110</NewExternalIPAddress>
*/
                LineP = Buf;
                if (LineP.isWordsInLine("NewExternalIPAddress")) {
                   ExternalIP = extractValueFromXML(Buf, "NewExternalIPAddress");
                }
             }
          }
       } while (retval >= 0);
       // so we drain all the output, and hopefully retvalb = true.

   } while (false);

   if (ExternalIP) return(true);
   else return(false);
}

const char *Upnp::getExternalIP() {

   TRACE();

   if (ExternalIP) {
      return(ExternalIP);
   }
   else {
      return(NULL);
   }
}

const char *Upnp::getPortMappingAtIndex(int PortMapIndex) {
static char *RetBuf = NULL;
TCPConnect TCPupnp;
bool retvalb = false;
int retval;
char Body[1024];
char Buf[1024];
int NewInternalPort = 0, NewExternalPort = 0, NewEnabled = 0;
char *NewInternalClient = NULL;
char *NewPortMappingDescription = NULL;

   TRACE();

   TCPupnp.setSetOptions(false);
   delete [] RetBuf;
   RetBuf = NULL;

   if (State != UPNP_OK) {
      return(RetBuf);
   }

   sprintf(Body,
     "<?xml version=\"1.0\"?>\r\n"
     "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"\r\n"
     "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
     "<s:Body>\r\n"
     "<u:GetGenericPortMappingEntry xmlns:u=\"urn:schemas-upnp-org:service:%s\">\r\n"
     "<NewPortMappingIndex>\r\n"
     "%d\r\n"
     "</NewPortMappingIndex>\r\n"
     "</u:GetGenericPortMappingEntry>\r\n"
     "</s:Body>\r\n"
     "</s:Envelope>\r\n\r\n",
     Connection,
     PortMapIndex);
;

   
   formPOSTRequest(Buf, Connection, "GetGenericPortMappingEntry", Body);
   do {
       TCPupnp.TCPConnectionMethod.setDirect();
       retvalb = TCPupnp.getConnection(ControlIP, ControlPort);
       if (retvalb == false) break;

       retvalb = false;
       retval = TCPupnp.writeData(Buf, strlen(Buf));
       if (retval != strlen(Buf)) {
          break;
       }

       // Lets wait for what he sends us back.
       do {
          retval = TCPupnp.readLine(Buf, 1023, 15); // 15 seconds time out.
          if (retval > 0) {

             // If its HTTP/1.1 200 OK then we are good.
             // COUT(cout << Buf << endl;)
             if (retvalb == false) {
                retvalb = isHTTPOK(Buf);
             }
             else {
                LineParse LineP;

                // Lets try to extract
/*
<NewRemoteHost></NewRemoteHost><NewExternalPort>8124</NewExternalPort><NewProtocol>TCP</NewProtocol><NewInternalPort>8124</NewInternalPort><NewInternalClient>192.168.0.26</NewInternalClient><NewEnabled>0</NewEnabled><NewPortMappingDescription>MasalaMate</Nendex>
*/
/*
<NewRemoteHost xmlns:dt="urn:schemas-microsoft-com:datatypes" dt:dt="string"></NewRemoteHost><NewExternalPort xmlns:dt="urn:schemas-microsoft-com:datatypes" dt:dt="ui2">21</NewExternalPort><NewProtocol xmlns:dt="urn:schemas-microsoft-com:datatypes" dt:dt="string">TCP</NewProtocol><NewInternalPort xmlns:dt="urn:schemas-microsoft-com:datatypes" dt:dt="ui2">21</NewInternalPort><NewInternalClient xmlns:dt="urn:schemas-microsoft-com:datatypes" dt:dt="string">192.168.1.0</NewInternalClient><NewEnabled xmlns:dt="urn:schemas-microsoft-com:datatypes" dt:dt="boolean">0</NewEnabled><NewPortMappingDescription xmlns:dt="urn:schemas-microsoft-com:datatypes" dt:dt="string">FTP</NewPortMappingDescription>
*/
                LineP = Buf;
                if (LineP.isWordsInLine("NewExternalPort")) {
                   char *tmpstr = extractValueFromXML(Buf, "NewExternalPort");
                   if (tmpstr) {
                      COUT(cout << "NewExternalPort: " << tmpstr << endl;)
                      // NewExternalPort = atoi(tmpstr);
                      NewExternalPort = (int) strtol(tmpstr, NULL, 10);
                      delete [] tmpstr;
                   }
                }
                if (LineP.isWordsInLine("NewInternalPort")) {
                   char *tmpstr = extractValueFromXML(Buf, "NewInternalPort");
                   if (tmpstr) {
                      COUT(cout << "NewInternalPort: " << tmpstr << endl;)
                      // NewInternalPort = atoi(tmpstr);
                      NewInternalPort = (int) strtol(tmpstr, NULL, 10);
                      delete [] tmpstr;
                   }
                }
                if (LineP.isWordsInLine("NewInternalClient")) {
                   NewInternalClient = extractValueFromXML(Buf, "NewInternalClient");
                   if (NewInternalClient) {
                      COUT(cout << "NewInternalClient: " << NewInternalClient << endl;)
                   }
                }
                if (LineP.isWordsInLine("NewEnabled")) {
                   char *tmpstr = extractValueFromXML(Buf, "NewEnabled");
                   if (tmpstr) {
                      COUT(cout << "NewEnabled: " << tmpstr << endl;)
                      // NewEnabled = atoi(tmpstr);
                      NewEnabled = (int) strtol(tmpstr, NULL, 10);
                      delete [] tmpstr;
                   }
                }
                if (LineP.isWordsInLine("NewPortMappingDescription")) {
                   NewPortMappingDescription = extractValueFromXML(Buf, "NewPortMappingDescription");
                   if (NewPortMappingDescription) {
                      COUT(cout << "NewPortMappingDescription: " << NewPortMappingDescription << endl;)
                   }
                }
             }
          }
       } while (retval >= 0);
       // so we drain all the output, and hopefully retvalb = true.

   } while (false);

   if ( (NewInternalPort != 0) && (NewExternalPort != 0) ) {
      RetBuf = new char[1024];
      sprintf(RetBuf, "Map[%d] -> Internal Port: %d External Port: %d Enabled?: %d", PortMapIndex, NewInternalPort, NewExternalPort, NewEnabled);
      if (NewInternalClient) {
         sprintf(Buf, " Internal Client: %s", NewInternalClient);
         strcat(RetBuf, Buf);
      }
      if (NewPortMappingDescription) {
         sprintf(Buf, " Description: %s", NewPortMappingDescription);
         strcat(RetBuf, Buf);
      }
   }

   delete [] NewInternalClient;
   delete [] NewPortMappingDescription;
   return(RetBuf);
}

bool Upnp::issueAddPortMapping(int AddPort, char *Description) {
TCPConnect TCPupnp;
bool retvalb = false;
int retval;
char Body[1024];
char Buf[1024];

   TRACE();

   TCPupnp.setSetOptions(false);
   if (State != UPNP_OK) {
      return(retvalb);
   }

   sprintf(Body,
      "<?xml version=\"1.0\"?>\r\n"
      "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"\r\n"
      "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
      "<s:Body>\r\n"
      "<u:AddPortMapping xmlns:u=\"urn:schemas-upnp-org:service:%s\">\r\n"
      "<NewRemoteHost></NewRemoteHost>\r\n"
      "<NewExternalPort>%d</NewExternalPort>\r\n"
      "<NewProtocol>TCP</NewProtocol>\r\n"
      "<NewInternalPort>%d</NewInternalPort>\r\n"
      "<NewInternalClient>%s</NewInternalClient>\r\n"
      "<NewEnabled>1</NewEnabled>\r\n"
      "<NewPortMappingDescription>%s</NewPortMappingDescription>\r\n"
      "<NewLeaseDuration>0</NewLeaseDuration>\r\n"
      "</u:AddPortMapping>\r\n"
      "</s:Body>\r\n"
      "</s:Envelope>\r\n",
      Connection,
      AddPort,
      AddPort,
      LocalIP,
      Description);

   formPOSTRequest(Buf, Connection, "AddPortMapping", Body);

   do {
       TCPupnp.TCPConnectionMethod.setDirect();
       retvalb = TCPupnp.getConnection(ControlIP, ControlPort);
       if (retvalb == false) break;

       retvalb = false;
       retval = TCPupnp.writeData(Buf, strlen(Buf));
       if (retval != strlen(Buf)) {
          break;
       }
       // COUT(cout <<  Buf << endl;)

       // Lets wait for what he sends us back.
       do {
          retval = TCPupnp.readLine(Buf, 1022, 15); // 15 seconds time out.
          if (retval > 0) {

             // COUT(cout << Buf << endl;)
             // If its HTTP/1.1 200 OK then we are good.
             if (retvalb == false) {
                retvalb = isHTTPOK(Buf);
             }
          }
       } while (retval >= 0);
   } while (false);

   return(retvalb);
}

bool Upnp::issueDeletePortMapping(int DelPort) {
TCPConnect TCPupnp;
bool retvalb = false;
int retval;
char Body[1024];
char Buf[1024];

   TRACE();

   TCPupnp.setSetOptions(false);
   if (State != UPNP_OK) {
      return(retvalb);
   }

   sprintf(Body,
      "<?xml version=\"1.0\"?>\r\n"
      "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"\r\n"
      "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
      "<s:Body>\r\n"
      "<u:DeletePortMapping xmlns:u=\"urn:schemas-upnp-org:service:%s\">\r\n"
      "<NewRemoteHost></NewRemoteHost>\r\n"
      "<NewExternalPort>%d</NewExternalPort>\r\n"
      "<NewProtocol>TCP</NewProtocol>\r\n"
      "</u:DeletePortMapping>\r\n"
      "</s:Body>\r\n"
      "</s:Envelope>\r\n",
      Connection,
      DelPort);

   formPOSTRequest(Buf, Connection, "DeletePortMapping", Body);

   do {
       TCPupnp.TCPConnectionMethod.setDirect();
       retvalb = TCPupnp.getConnection(ControlIP, ControlPort);
       if (retvalb == false) break;

       retvalb = false;
       retval = TCPupnp.writeData(Buf, strlen(Buf));
       if (retval != strlen(Buf)) {
          break;
       }
       // COUT(cout << Buf << endl;)

       // Lets wait for what he sends us back.
       do {
          retval = TCPupnp.readLine(Buf, 1022, 15); // 15 seconds time out.
          if (retval > 0) {

             // COUT(cout << Buf << endl;)
             // If its HTTP/1.1 200 OK then we are good.
             if (retvalb == false) {
                retvalb = isHTTPOK(Buf);
             }
          }
       } while (retval >= 0);
   } while (false);

   return(retvalb);
}

UpnpState Upnp::getState() {
   TRACE();

   return(State);
}

// Form the HTTP POST request.
// Request is allocated by caller.
void Upnp::formPOSTRequest(char *Request, char *Interface, char *Command, char *Body) {

   TRACE();

   sprintf(Request,
      "POST %s HTTP/1.1\r\n"
      "HOST: %s:%d\r\n"
      "SOAPACTION: "
      "\"urn:schemas-upnp-org:service:%s#%s\"\r\n"
      "CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
      "CONTENT-LENGTH: %d\r\n\r\n"
      "%s",
      ControlURL,
      ControlIP, ControlPort,
      Interface, Command,
      strlen(Body),
      Body);

   return;
}


// Check for HTTP/1.1 200 OK
bool Upnp::isHTTPOK(char *Buf) {
LineParse LineP;
const char *parseptr;
bool retvalb = false;

   TRACE();

   // If its HTTP/1.1 200 OK then we are good.
   // So we search for " 200 OK " in the line.
   LineP = Buf;
   parseptr = LineP.removeConsecutiveDeLimiters();

   // parseptr == NULL => empty line after removing redundant delimiters.
   if (parseptr == NULL) {
      return(false);
   }

   LineP = (char *) parseptr;
   parseptr = LineP.getWord(1);
   if (strncasecmp(parseptr, "HTTP", 4) == 0) {
      parseptr = LineP.getWord(2);
      if (strcasecmp(parseptr, "200") == 0) {
         parseptr = LineP.getWord(3);
         if (strcasecmp(parseptr, "OK") == 0) {
            retvalb = true;
         }
      }
   }

   return(retvalb);
}


// To extract the value string from xml of a particular element.
// caller needs to free the string returned.
char *Upnp::extractValueFromXML(char *xml, char *element) {
char *retstr = NULL;
LineParse LineP;
const char *parseptr;

   TRACE();

   LineP = xml;
   if (LineP.isWordsInLine(element)) {
      LineP.setDeLimiter('<');
      int numwords = LineP.getWordCount();
      for (int i = 1; i <= numwords; i++) {
         parseptr = LineP.getWord(i);
         int elemlen = strlen(element);
         if (strncasecmp(parseptr, element, elemlen) == 0) {
            LineP = (char *) parseptr;
            LineP.setDeLimiter('>');
            parseptr = LineP.getWord(2);
            retstr = new char[strlen(parseptr) + 1];
            strcpy(retstr, parseptr);
            break;
         }
      }
   }
   return(retstr);
}

void Upnp::printDebug() {

   TRACE();

   COUT(cout << "Upnp::" << endl;)

   COUT(cout << "Local IP: " << LocalIP << " Control IP: " << ControlIP << " Control Port: " << ControlPort << endl;)

   COUT(cout << "Connection: ";)
   if (Connection) {
      COUT(cout << Connection << " Control URL: ";)
   }
   else {
      COUT(cout << "NULL Control URL: ";)
   }

   if (ControlURL) {
      COUT(cout << ControlURL << " External IP: ";)
   }
   else {
      COUT(cout << "NULL External IP: ";)
   }

   if (ExternalIP) {
      COUT(cout << ExternalIP << " State: ";)
   }
   else {
      COUT(cout << "NULL State: ";)
   }

   switch (State) {
     case UPNP_UNKNOWN:
       COUT(cout << "Unknown";)
       break;

     case UPNP_NOTDETECTED:
       COUT(cout << "UPNP Not Detected";)
       break;

     case UPNP_LOCATION_DETECTED:
       COUT(cout << "UPNP Location alone Detected";)
       break;

     case UPNP_CONTROL_UNDETECTED:
       COUT(cout << "UPNP Location Detected but Control UnDetected";)
       break;

     case UPNP_OK:
       COUT(cout << "All OK";)
       break;
 
   }
   COUT(cout << endl;)
}
