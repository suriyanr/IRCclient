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
#include "ConnectionMethod.hpp"

using namespace std;

int main() {
ConnectionMethod Test;
char *FAIL="FAIL";
char *PASS="PASS";
ConnectionHowE method;

// Check to see if its valid.
   cout << "Test 1: Invalid check, should return 0: Returns: " << Test.valid();
   if (Test.valid() == false) {
       cout << " PASS" << endl;
   }
   else {
       cout << " FAIL" << endl;
   }

// set ConnectionMethod to direct.
   Test.setDirect();
   cout << "Test 2: Valid check, should return 1: Returns: " << Test.valid();
   if (Test.valid() == true) {
       cout << " PASS" << endl;
   }
   else {
       cout << " FAIL" << endl;
   }

// check if the connection method is direct.
   method = CM_DIRECT;
   cout << "Test 3: Method Direct Check, should return " << method << ": Returns: " << Test.howto();
   if (Test.howto() == method) {
       cout << " PASS" << endl;
   }   
   else {
       cout << " FAIL" << endl;
   }

// check if the connection method is proxy.
   method = CM_PROXY;
   Test.setProxy((const char *) "www-proxy", 80, (const char *)"proxy-user", (const char *)"proxy-password");
   cout << "Test 4: Method Proxy Check, should return " << method << ": Returns: " << Test.howto();
   if (Test.howto() == method) {
       cout << " PASS" << endl;
   }
   else {
       cout << " FAIL" << endl;
   }
// check to see if its valid.
   cout << "Test 5: Valid proxy check, should return " << true << ": Returns: " << Test.valid();
   if (Test.valid() == true) {
       cout << " PASS" << endl;
   }
   else {
       cout << " FAIL" << endl;
   }
// print out the proxy information.
   Test.printDebug();

   method = CM_BNC;
   Test.setBNC((const char *) "bnc-host", 1234, (const char *) "bnc-pass", (const char *) "bnc-vhost");
   cout << "Test 6: Method BNC Check, should return " << method << ": Returns: " << Test.howto();
   if (Test.howto() == method) {
       cout << " PASS" << endl;
   }
   else {
       cout << " FAIL" << endl;
   }
// check to see if its valid.
   cout << "Test 7: Valid proxy check, should return " << true << ": Returns: " << Test.valid();
   if (Test.valid() == true) {
       cout << " PASS" << endl;
   }
   else {
       cout << " FAIL" << endl;
   }
   
// print out the BNC information.
   Test.printDebug();

   method = CM_SOCKS4;
   Test.setSocks4((const char *) "socks4-host", 1081, (const char *) "socks4-user");
   cout << "Test 8: Method socks4 Check, should return " << method << ": Returns: " << Test.howto();
   if (Test.howto() == method) {
       cout << " PASS" << endl;
   }
   else {
       cout << " FAIL" << endl;
   }
// check to see if its valid.
   cout << "Test 9: Valid socks4 check, should return " << true << ": Returns: " << Test.valid();
   if (Test.valid() == true) {
       cout << " PASS" << endl;
   }
   else {
       cout << " FAIL" << endl;
   }

// print out the Socks4 information.
   Test.printDebug();

   method = CM_SOCKS5;
   Test.setSocks5((const char *) "socks5-host", 1082, (const char *) "socks5-user", (const char *) "socks5-pass");
   cout << "Test 10: Method socks5 Check, should return " << method << ": Returns: " << Test.howto();
   if (Test.howto() == method) {
       cout << " PASS" << endl;
   }
   else {
       cout << " FAIL" << endl;
   }
// check to see if its valid.
   cout << "Test 11: Valid socks5 check, should return " << true << ": Returns: " << Test.valid();
   if (Test.valid() == true) {
       cout << " PASS" << endl;
   }
   else {
       cout << " FAIL" << endl;
   }

// print out the Socks5 information.
   Test.printDebug();

   method = CM_WINGATE;
   Test.setWingate((const char *) "wingate-host", 1083);
   cout << "Test 12: Method wingate Check, should return " << method << ": Returns: " << Test.howto();
   if (Test.howto() == method) {
       cout << " PASS" << endl;
   }
   else {
       cout << " FAIL" << endl;
   }
// check to see if its valid.
   cout << "Test 13: Valid wingate check, should return " << true << ": Returns: " << Test.valid();
   if (Test.valid() == true) {
       cout << " PASS" << endl;
   }
   else {
       cout << " FAIL" << endl;
   }

// print out the wingate information.
   Test.printDebug();
}

