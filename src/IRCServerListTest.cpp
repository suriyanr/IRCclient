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
#include "IRCServerList.hpp"

using namespace std;

int main() {
IRCServerList ServerList;
unsigned short port;

   cout << "Test 1: Print empty serverlist" << endl;
   ServerList.printDebug();

   cout << "Test 2: Add long.server.name.irc.rc0.net 6667" << endl;
   ServerList.addServer("long.server.name.irc.rc0.net", 6667);
   ServerList.printDebug();

   cout << "Test 3: Add apna.ga.us.rc0.net 7000" << endl;
   ServerList.addServer("apna.ga.us.rc0.net", 7000);
   ServerList.printDebug();

   cout << "Test 4: Add flowerpower.fl.us.rc0.net 6660" << endl;
   ServerList.addServer("flowerpower.fl.us.rc0.net", 6660);
   ServerList.printDebug();

   cout << "Test 5: Add random.net" << endl;
   ServerList.addServer("random.net");
   ServerList.printDebug();

   cout << "Test 6: getServer(1)" << endl;
   cout << ServerList.getServer(port, 1) << " " << port << endl;

   cout << "Test 7: getServer(2)" << endl;
   cout << ServerList.getServer(port, 2) << " " << port << endl;

   cout << "Test 8: getServer(3)" << endl;
   cout << ServerList.getServer(port, 3) << " " << port << endl;

   cout << "Test 9: getServer(4)" << endl;
   cout << ServerList.getServer(port, 4) << " " << port << endl;
   
   cout << "Test 10: Adding same servers with different ports." << endl;
   ServerList.addServer("irc.criten.net", 6660);
   ServerList.addServer("irc.criten.net", 6662);
   ServerList.addServer("irc.criten.net", 6664);
   ServerList.addServer("irc.criten.net", 6666);
   ServerList.addServer("irc.criten.net", 6668);
   ServerList.printDebug();


}
