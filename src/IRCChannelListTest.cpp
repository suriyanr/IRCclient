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
#include "IRCChannelList.hpp"

using namespace std;

int main() {
IRCChannelList ChannelList;

   cout << "Test 1: Print empty channellist" << endl;
   ChannelList.printDebug();

   cout << "Test 2: Add #IndianMasala" << endl;
   ChannelList.addChannel("#IndianMasala", "pass");
   ChannelList.printDebug();

   cout << "Test 3: Add #Masala-Chat" << endl;
   ChannelList.addChannel("#Masala-Chat", NULL);
   ChannelList.printDebug();

   cout << "Test 4: Add #somelongchannelnamesoandso" << endl;
   ChannelList.addChannel("#somelongchannelnamesoandso", "pass");
   ChannelList.printDebug();

   cout << "Test 5: getChannelCount()" << endl;
   cout << ChannelList.getChannelCount() << endl;

   cout << "Test 6: getChannel(1)" << endl;
   cout << ChannelList.getChannel(1) << endl;

   cout << "Test 7: getChannel(2)" << endl;
   cout << ChannelList.getChannel(2) << endl;

   cout << "Test 8: getChannel(3)" << endl;
   cout << ChannelList.getChannel(3) << endl;

   cout << "Test 9: setConnected(getChannel(2))" << endl;
   ChannelList.setJoined(ChannelList.getChannel(2));
   ChannelList.printDebug();
}
