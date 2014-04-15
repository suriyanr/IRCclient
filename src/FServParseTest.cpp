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
using namespace std;

#include "FServParse.hpp"

int main() {
FServParse FP;
char buffer[1024];

   cout << "Test 1: Print uninitialised FServParse" << endl;
   FP.printDebug();

   cout << "Test 1a: FServParse = T30Linux [IM]-Art0001 ** 1 pack **  3 of 3 slots open" << endl;
   FP = "T30Linux [IM]-Art0001 ** 1 pack **  3 of 3 slots open";
   FP.printDebug();

   cout << "Test 2: FServParse = 192.168.1.100 [IM]-Art0001 ** 2 packs **  3 of 3 slots open, Record: 46.7KB/s" << endl;
   FP = "192.168.1.100 [IM]-Art0001 ** 2 packs **  3 of 3 slots open, Record: 46.7KB/s";
   FP.printDebug();

   cout << "Test 3: FServParse = T30Linux [IM]-Art0002 ** 2 packs **  0 of 3 slots open, Queue: 3/10, Record: 14.6KB/s" << endl;
   FP = "T30Linux [IM]-Art0002 ** 2 packs **  0 of 3 slots open, Queue: 3/10, Record: 14.6KB/s";
   FP.printDebug();

   cout << "Test 4: FServParse = 192.168.1.100 [IM]-Art0551 [FServ Active] - Trigger:[/ctcp [IM]-Art0551 More Masala] - Users:[ 0/5 ] - Sends:[ 0/3 ] - Queues:[ 0/10 ] - Record CPS:[ 119.7KB/s ] - Bytes Sent:[ 356.43 GB ] - Upload Speed:[ 0.0KB/s ] - Server:[ Open To All ] - FireWall Workaround Port:[ OFF ] - Message:[[IM]-Art0551 Message] - Iroffer FServ" << endl;
   FP = "192.168.1.100 [IM]-Art0551 [FServ Active] - Trigger:[/ctcp [IM]-Art0551 More Masala] - Users:[ 0/5 ] - Sends:[ 0/3 ] - Queues:[ 0/10 ] - Record CPS:[ 119.7KB/s ] - Bytes Sent:[ 356.43 GB ] - Upload Speed:[ 0.0KB/s ] - Server:[ Open To All ] - FireWall Workaround Port:[ OFF ] - Message:[[IM]-Art0551 Message] - Iroffer FServ";
   FP.printDebug();

   cout << "Test 5: FServParse = T30Linux [IM]-Art01 #1 343x [535M] IV-HardCoreCompilation.avi || Har" << endl;
   FP = "T30Linux [IM]-Art01 #1 343x [535M] IV-HardCoreCompilation.avi || Har";
   FP.printDebug();

   cout << "Test 6: FServParse = 192.168.1.100 [IM]-Art02 #1 23x [ 24M] File || Desc..." << endl;
   FP = "192.168.1.100 [IM]-Art02 #1 23x [ 24M] File || Desc...";
   FP.printDebug();

   cout << "Test 7: FServParse = T30Linux [IM]-Art03 #1 23x [1.1M] File || Desc..." << endl;
   FP = "T30Linux [IM]-Art03 #1 23x [1.1M] File || Desc...";
   FP.printDebug();

   cout << "Test 8: FServParse = 192.168.1.100 tommylee [Fserve Active] - Trigger:[/ctcp tommylee Imas whatever] - Users:[0/5] - Sends:[1/1] - Queues:[2/5] - Record CPS:[34.6kB/s by aglp2k] - Bytes Sent:[92.33GB] - Files Sent:[185] - Resends:[341] - Accesses:[5459] - Upload Speed:[10.1kB/s] - Download Speed:[14.8kB/s] - Current Bandwidth:[24.9kB/s] - SysReset 2.53" << endl;
   FP = "192.168.1.100 tommylee [Fserve Active] - Trigger:[/ctcp tommylee Imas whatever] - Users:[0/5] - Sends:[1/1] - Queues:[2/5] - Record CPS:[34.6kB/s by aglp2k] - Bytes Sent:[92.33GB] - Files Sent:[185] - Resends:[341] - Accesses:[5459] - Upload Speed:[10.1kB/s] - Download Speed:[14.8kB/s] - Current Bandwidth:[24.9kB/s] - SysReset 2.53";
   FP.printDebug();

   cout << "Test 9: FServParse = \"T30Linux [IM]-Art0451 \0030,1\002#1 \002 118x [433M] Bangla-Vol4.avi || Bangla - Vol 4\"" << endl;
   FP = "T30Linux [IM]-Art0451 \0030,1\002#1 \002 118x [433M] Bangla-Vol4.avi || Bangla - Vol 4";

   FP.printDebug();

   cout << "Test 10: FServParse = \"192.168.1.100 Nick #2 0x [148M] [TMD]Saw.(SG18).Proper.TS.(2of2).avi\"" << endl;
   FP = "192.168.1.100 Nick #2 0x [148M] [TMD]Saw.(SG18).Proper.TS.(2of2).avi";
   FP.printDebug();

   cout << "Test 11: FServParse = \"T30Linux Nick ** Server Open To All, Firewall Workaround Port OFF **\"" << endl;
   FP = "T30Linux Nick ** Server Open To All, Firewall Workaround Port OFF **";
   FP.printDebug();

   cout << "Test 12: FServParse = \"192.168.1.100 Nick ** Server Voice Only, Firewall Workaround Port OFF **\"" << endl;
   FP = "192.168.1.100 Nick ** Server Voice Only, Firewall Workaround Port OFF **";
   FP.printDebug();

   cout << "Test 13: FServParse = \"T30Linux Nick ** Server Open To All, Firewall Workaround Port 8124 **\"" << endl;
   FP = "T30Linux Nick ** Server Voice Only, Firewall Workaround Port 8124 **";
   FP.printDebug();

   cout << "Test 14: FServParse = \"192.168.1.100 Nick ** Server Voice Only, Firewall Workaround Port 8124 **\"" << endl;
   FP = "192.168.1.100 Nick ** Server Voice Only, Firewall Workaround Port 8124 **";
   FP.printDebug();

   cout << "Test 15: FServParse = \"\"" << endl;
   FP = "";
   FP.printDebug();

   cout << "Test 16: FServParse = 192.168.1.100 [IM]-Art0467 ** 6 packs ** 0 of 3 slots open, Queue: 1/10, Record: 458.7KB/s" << endl;
   FP = "192.168.1.100 [IM]-Art0467 ** 6 packs ** 0 of 3 slots open, Queue: 1/10, Record: 458.7KB/s";
   FP.printDebug();

   cout << "Test 17: FServParse = T30Linux FromNick PROPAGATION PropagatedNick 1 2 4 10" << endl;
   FP = "T30Linux FromNick PROPAGATION PropagatedNick 1 2 4 10";
   FP.printDebug();
}
