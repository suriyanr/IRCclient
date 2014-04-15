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

using namespace std;
#include <iostream>
#include <string>

#include "TCPConnect.hpp"


int main() {
TCPConnect Test;
TCPConnect TestC;
bool retbool;
char buffer[1024];
char strinput[1024];
ssize_t retval;

   cout << "Test 1: Setting DIRECT Connect" << endl;
   Test.TCPConnectionMethod.setDirect();
   Test.printDebug();

   cout << "Test 2: Calling getConnection(www.yahoo.com, 80)" << endl;
   retbool = Test.getConnection("www.yahoo.com", 80);
   cout << "Retval: " << retbool << endl;
   Test.printDebug();

   while (true) {
      while (true) {
         cout << "Calling readLine(buffer, 1022, 1)";
         retval = Test.readLine(buffer, 1022, 1);
         cout << " returned: " << retval << endl;
         if (retval < 1) break;
         
         cout << "RECV: " << buffer << endl;
      }
      if (retval == -1) break;
      cout << "INPUT: ";
      cin.getline(strinput, 1022);
      strcat(strinput, "\n");
      cout << "Calling writeData(strinput, strlen(strinput), 10) buffer: " << strinput;
      retval = Test.writeData(strinput, strlen(strinput), 10);
      cout << "Returned: " << retval << endl;
   }
   cout << "END of FTP test." << endl;

   cout << "Test 2a: setHost(www.yahoo.com), setPort(80), getConnection()" << endl;
   Test.setHost("www.yahoo.com");
   Test.setPort(80);
   retbool = Test.getConnection();
   cout << "Retval: " << retbool << endl;
   Test.printDebug();
   Test.disConnect();

   cout << "Test 3: Setting Proxy Connection" << endl;
   Test.TCPConnectionMethod.setProxy((const char *) "www-proxy", 80);
   Test.printDebug();

//   cout << "Test 4: Calling getConnection(www.yahoo.com, 80)" << endl;
//   retbool = Test.getConnection("www.yahoo.com", 80);
   cout << "Test 4: Calling getConnection(205.161.185.10, 80)" << endl;
   retbool = Test.getConnection("205.161.185.10", 80);
   cout << "Retval: " << retbool << endl;
   Test.printDebug();
   if (retbool == true) {
     cout << "Attempt to get yahoo index.html" << endl;
     strcpy(buffer, "GET index.html\n\n");
     cout << "writeData -> GET index.html" << endl;
     if (Test.writeData(buffer, strlen(buffer), 10) == strlen(buffer)) {
        while (true) {
           retval = Test.readLine(buffer, 1020, 1);
           if (retval == -1) break;
           if (retval == 0) break; // Timeout waiting for input.
           cout << "Got line: " << buffer << endl;
        }
     }
     else {
        cout << "writeData failed." << endl;
     }
   }

   cout << "Test 5: Setting BNC Connection" << endl;
   Test.TCPConnectionMethod.setBNC((const char *) "localhost", 61234, (const char *) "mystic");
   Test.printDebug();

   cout << "Test 6: Calling getConnection(irc.rc0.net, 6667)" << endl;
   retbool = Test.getConnection("irc.rc0.net", 6667);
   cout << "Retval: " << retbool << endl;
   Test.printDebug();
   if (retbool == true) {
     cout << "Get response from BNC" << endl;
     while (true) {
        retval = Test.readLine(buffer, 1020, 10);
        if (retval == -1) break;
        if (retval == 0) break; // Timeout waiting for input.
        cout << "Got line: " << buffer << endl;
     }
   }
#ifdef __MINGW32__
   WSACleanup();
#endif
}
