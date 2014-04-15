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

#include <iostream>
#include "Upnp.hpp"

using namespace std;

int main() {
Upnp UPNP;
bool retvalb;
const char *retbuf;

  retvalb = UPNP.issueSearch("WANIPConnection:1");
  UPNP.printDebug();
  if (retvalb == false) {
     retvalb = UPNP.issueSearch("WANPPPConnection:1");
     UPNP.printDebug();
  }

  if (UPNP.getState() == UPNP_OK) {
     UPNP.issueGetExternalIP();
     retbuf = UPNP.getExternalIP();
     cout << "External IP: " << retbuf << endl;
     UPNP.printDebug();

     cout << "Trying to map port 2525" << endl;
     retvalb = UPNP.issueAddPortMapping(2525, "Testing");
     if (retvalb) {
        cout << "Successfully added port mapping for port 2525" << endl;
     }
     else {
        cout << "Adding Port mapping for port 2525 failed." << endl;
     }

     cout << "Listing all the Port Mappings." << endl;
     for (int i = 0; i < 100; i++) {
        retbuf = UPNP.getPortMappingAtIndex(i);
        if (retbuf) {
           cout << retbuf << endl;
        }
        else {
           break;
        }
     }

     cout << "Trying to unmap port 2525" << endl;
     retvalb = UPNP.issueDeletePortMapping(2525);
     if (retvalb) {
        cout << "Successfully deleted port mapping for port 2525" << endl;
     }
     else {
        cout << "Deleting Port mapping for port 2525 failed." << endl;
     }
  }
}
