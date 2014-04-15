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
#include "TCPConnect.hpp"
#include "Compatibility.hpp"


int main() {
TCPConnect Test;
TCPConnect TestC;
bool retbool;
char buffer[1024];
char outb[1024];
unsigned long ip;

#if 0
   cout << "Test a: isValid test" << endl;
   if (Test.isValid() == true) {
      cout << "Pass" << endl;
   }
   else {
      cout << "Failed" << endl;
   }
   exit(0);
#endif

   cout << "Test 0: Helper tests" << endl;
   ip = Test.getLongFromHostName("ramasami.is-a-geek.org");
   cout << "calling getLongFromHostName(ramasami.is-a-geek.org): " << ip << endl;
   Test.getDottedIpAddressFromLong(ip, buffer);
   cout << "calling getDottedIpAddressFromLong(ip, buffer): " << buffer << endl;
   ip = Test.getLongFromHostName("67.161.27.122");
   cout << "calling getLongFromHostName(67.161.27.122): " << ip << endl;
   Test.getDottedIpAddressFromLong(ip, buffer);
   cout << "calling getDottedIpAddressFromLong(ip, buffer): " << buffer << endl;
   ip = Test.getLongFromHostName("IRC2456.is-a-geek.org");
   cout << "calling getLongFromHostName(IRC2456.is-a-geek.org): " << ip << endl;
   Test.getDottedIpAddressFromLong(ip, buffer);
   cout << "calling getDottedIpAddressFromLong(ip, buffer): " << buffer << endl;
   ip = Test.getLongFromHostName("4.7.0.62");
   cout << "calling getLongFromHostName(4.7.0.62): " << ip << endl;
   Test.getDottedIpAddressFromLong(ip, buffer);
   cout << "calling getDottedIpAddressFromLong(ip, buffer): " << buffer << endl;

   cout << "Test 1: Calling serverInit" << endl;
   retbool = Test.serverInit(8080);
   cout << "Retval: " << retbool << endl;
   Test.printDebug();

   cout << "Test 2: Calling getConnection(2 seconds)" << endl;
   retbool = Test.getConnection(2);
   cout << "Retval: " << retbool << endl;
   Test.printDebug();

   cout << "Test 3: Calling getConnection(TIMEOUT_INF)" << endl;
   retbool = Test.getConnection(TIMEOUT_INF);
   cout << "Connection Established - Retval: " << retbool << endl;
   Test.printDebug();

   cout << "Test 4: Equating TestC = Test" << endl;
   TestC = Test;
   cout << "Printing Test:" << endl;
   Test.printDebug();
   cout << "Printing TestC:" << endl;
   TestC.printDebug();

   cout << "Test 5: readLine with timeout = 2 seconds. do not type." << endl;
   TestC.readLine(buffer, sizeof(buffer), 2);

   while (true) {
      cout << "Test 6: readLine with timeout = 60 seconds. do type." << endl;
      TestC.readLine(buffer, sizeof(buffer), 60);
      cout << "Received: "<< buffer << endl;

      strcpy(outb, "ECHO: ");
      strcat(outb, buffer);
      strcat(outb, "\n");
      cout << "Test 7: writeData with timeout = 10 seconds. " << outb;
      TestC.writeData(outb, strlen(outb), 10);
      cout << "Printing TestC:" << endl;
      TestC.printDebug();
      if (strcmp(buffer, "EXIT") == 0) break;
   }

   sleep(1);

#ifdef __MINGW32__
   WSACleanup();
#endif
}
