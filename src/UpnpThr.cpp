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


#include "ThreadMain.hpp"
#include "LineParse.hpp"
#include "UI.hpp"
#include "Helper.hpp"
#include "Upnp.hpp"
#include "StackTrace.hpp"

#include <stdio.h>
#include <string.h>

#include "Compatibility.hpp"

// Thread which is the Upnp caretaker.
// Does the below:
void UpnpThr(XChange *XGlobal) {
THR_EXITCODE thr_exit = 1;
char Response[1024];
char Message[1024];
Upnp UpnpObject;
LineParse LineP;
const char *parseptr;
Upnp UPNP;
bool retvalb;
bool initPortFwd = false;
bool timerThrInitiatedSearch = false;

   TRACE_INIT_NOCRASH();
   TRACE();

   // We give ourselves a SEARCH command.
   XGlobal->UI_ToUpnp.putLine("Server SEARCH");

   while (true) {
      XGlobal->UI_ToUpnp.getLineAndDelete(Message);
      // First Word is the Window Name, where responses need to go.
      if (XGlobal->isIRC_QUIT()) break;

      LineP = Message;
      parseptr = LineP.getWord(2);

      // This is the timerThr initiated periodic search.
      if (strcasecmp(parseptr, "SEARCH_IF_NOT_OK") == 0) {
         if (UPNP.getState() != UPNP_OK) {
            timerThrInitiatedSearch = true;
            initPortFwd = false; // So that 8124 is added on router detect.
            XGlobal->UI_ToUpnp.putLine("Server SEARCH");
         }
         continue;
      }

      if (strcasecmp(parseptr, "SEARCH") == 0) {
         // We have to issue a M-SEARCH
         // This returns only after the SEARCH completes or timeouts.
         retvalb = UPNP.issueSearch("WANIPConnection:1");
         if (retvalb == false) {
            retvalb = UPNP.issueSearch("WANPPPConnection:1");
         }
         // Here we update UI with results.
         if (retvalb) {

            // If this is the first search, we have initiated it.
            // So lets add the DCC_SERVER_PORT port forwarding.
            if (initPortFwd == false) {
               initPortFwd = true;
               sprintf(Response, "Server ADD_PORT_MAPPING %d MasalaMate", DCCSERVER_PORT);
               XGlobal->UI_ToUpnp.putLine(Response);
            }

            // Successful search, lets give the details.
            parseptr = LineP.getWord(1);
            sprintf(Response, "%s 09UPNP: Router detected.", parseptr);
            timerThrInitiatedSearch = false;
            XGlobal->lock();
            XGlobal->UpnpStatus = 1;
            XGlobal->unlock();
         }
         else {
            parseptr = LineP.getWord(1);
            sprintf(Response, "%s 04UPNP: UPNP Enabled Router not found", parseptr);
            XGlobal->lock();
            XGlobal->UpnpStatus = 0;
            XGlobal->unlock();
         }
         if (timerThrInitiatedSearch == false) {
            XGlobal->IRC_ToUI.putLine(Response);
         }
         else {
            timerThrInitiatedSearch = false;
         }
         COUT(UPNP.printDebug();)
         continue;
      }

      // For rest of commands, the status should be UPNP_OK
      if (UPNP.getState() != UPNP_OK) {
         parseptr = LineP.getWord(1);
         sprintf(Response, "%s 04UPNP: UPNP Enabled Router not found. Possibly issue a /upnp search first.", parseptr);
         XGlobal->IRC_ToUI.putLine(Response);
         continue;
      }

      if (strcasecmp(parseptr, "GET_EXTERNAL_IP") == 0) {
         const char *ip;

         UPNP.issueGetExternalIP();
         ip = UPNP.getExternalIP();
         parseptr = LineP.getWord(1);
         // Here we update UI with results.
         if (ip) {
            // Successful, lets give the details.
            sprintf(Response, "%s 09UPNP: External IP -> %s", parseptr, ip);
         }
         else {
            sprintf(Response, "%s 04UPNP: Couldnt get External IP", parseptr);
         }
         XGlobal->IRC_ToUI.putLine(Response);
         continue;
      }

      if (strcasecmp(parseptr, "GET_CURRENT_PORT_MAPPINGS") == 0) {
         const char *retbuf;
         parseptr = LineP.getWord(1);
         // We have to get the port mappings.
         for (int i = 0; i < 100; i++) {
            retbuf = UPNP.getPortMappingAtIndex(i);
            if (retbuf) {
            // Successful, lets give the details.
               sprintf(Response, "%s 09 UPNP: %s", parseptr, retbuf);
               XGlobal->IRC_ToUI.putLine(Response);
            }  
            else {
               if (i == 0) {
                  sprintf(Response, "%s 04UPNP: Could not get the Current Port Mappings.", parseptr);
                  XGlobal->IRC_ToUI.putLine(Response);
               }
               break;
            }
         }
         continue;
      }

      if (strcasecmp(parseptr, "ADD_PORT_MAPPING") == 0) {
         // Add the port in 3rd word.
         parseptr = LineP.getWord(3);
         // int addport = atoi(parseptr);
         int addport = (int) strtol(parseptr, NULL, 10);
         if ( (addport <= 0) && (addport >= 65536) ) {
            parseptr = LineP.getWord(1);
            sprintf(Response, "%s 04UPNP: The Port to be forwarded should be between 1 and 65535.", parseptr);
         }
         else {
            parseptr = LineP.getWordRange(4, 0);
            retvalb = UPNP.issueAddPortMapping(addport, (char *) parseptr);
            COUT(cout << "UPNP.issueAddPortMapping: " << addport << " " << parseptr << endl;)
            parseptr = LineP.getWord(1);
            if (retvalb) {
               // Successful, lets give the details.
               sprintf(Response, "%s 09UPNP: Added Port Mapping for Port %d.", parseptr, addport);
               XGlobal->lock();
               XGlobal->UpnpStatus = 2;
               XGlobal->unlock();
            }
            else {
               sprintf(Response, "%s 04UPNP: Port Mapping for Port %d failed.", parseptr);
            }
         }
         XGlobal->IRC_ToUI.putLine(Response);
         continue;
      }

      if (strcasecmp(parseptr, "DEL_PORT_MAPPING") == 0) {
         // Delete the port in 3rd word.
         parseptr = LineP.getWord(3);
         // int delport = atoi(parseptr);
         int delport = (int) strtol(parseptr, NULL, 10);
         if ( (delport <= 0) && (delport >= 65536) ) {
            parseptr = LineP.getWord(1);
            sprintf(Response, "%s 04UPNP: The Port forwarding entry to be deleted should be between 1 and 65535.", parseptr);
         }
         else {
            retvalb = UPNP.issueDeletePortMapping(delport);
            parseptr = LineP.getWord(1);
            if (retvalb) {
               // Successful, lets give the details.
               sprintf(Response, "%s 09UPNP: Successfully deleted the Port Mapping for Port %d.", parseptr, delport);
            }
            else {
               sprintf(Response, "%s 04UPNP: Couldnt delete the Port Mapping for Port %d.", parseptr, delport);
            }
         }
         XGlobal->IRC_ToUI.putLine(Response);
         continue;
      }
   }

   // We delete the port mapping we had created.
   UPNP.issueDeletePortMapping(DCCSERVER_PORT);

   COUT(cout << "UpnpThr: Quitting" << endl;)
// We dont call Exit thread as we want to go out of scope => destrcutors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}
