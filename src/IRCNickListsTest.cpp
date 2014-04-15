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
#include "IRCNickLists.hpp"

using namespace std;

int main() {
IRCNickLists NickList;
unsigned int nickmode;
char got_nick[64];

   cout << "Test 1: add Channel #Masala-Chat" << endl;
   NickList.addChannel("#Masala-Chat");
   NickList.printDebug();

   cout << "Test 2: Add #Masala-Chat Sur4802 Regular" << endl;
   NickList.addNick("#Masala-Chat", "Sur4802", IRCNICKMODE_REGULAR);
   NickList.printDebug();

   cout << "Test 3: Add #Masala-Chat Nick a0000 Op, Chick Voice" << endl;
   NickList.addNick("#Masala-Chat", "a0000", IRCNICKMODE_OP);
   NickList.addNick("#Masala-Chat", "Chick", IRCNICKMODE_VOICE);
   NickList.printDebug();

   cout << "Test 4: Set Nick Mode Sur4802 Voice" << endl;
   NickList.setNickMode("#Masala-Chat", "Sur4802", IRCNICKMODE_VOICE);
   NickList.printDebug();

   cout << "Test 5: getNickCount()" << endl;
   cout << NickList.getNickCount("#Masala-Chat") << endl;

   cout << "Test 6: getNickMode(Sur4802)" << endl;
   cout << NickList.getNickMode("#Masala-Chat", "Sur4802") << endl;

   cout << "Test 7: isNickInChannel(#Masala-Chat, a0000)" << endl;
   cout << NickList.isNickInChannel("#Masala-Chat", "a0000") << endl;

   cout << "Test 8: delNick(a0000)" << endl;
   NickList.delNick("#Masala-Chat", "a0000");
   NickList.printDebug();

   cout << "Test 9: isNickInChannel(#Masala-Chat, a0000)" << endl;
   cout << NickList.isNickInChannel("#Masala-Chat", "a0000") << endl;

   cout << "Test 10: addChannel #IndianMasala" << endl;
   NickList.addChannel("#IndianMasala");
   NickList.printDebug();

   cout << "Deleting channel #Masala-Chat" << endl;
   NickList.delChannel("#Masala-Chat");
   NickList.printDebug();

   cout << "Test 11: addNick #IndianMasala I-Sur4802, I-a0000" << endl;
   NickList.addNick("#IndianMasala", "I-Sur4802", IRCNICKMODE_REGULAR);
   NickList.addNick("#IndianMasala", "I-a0000", IRCNICKMODE_REGULAR);
   NickList.addNick("#IndianMasala", "I-a0001", IRCNICKMODE_REGULAR);
   NickList.addNick("#IndianMasala", "I-a0002", IRCNICKMODE_REGULAR);
   NickList.addNick("#IndianMasala", "I-a0003", IRCNICKMODE_REGULAR);
   NickList.addNick("#IndianMasala", "I-a0004", IRCNICKMODE_REGULAR);
   NickList.addNick("#IndianMasala", "I-a0005", IRCNICKMODE_REGULAR);
   NickList.addNick("#IndianMasala", "I-a0006", IRCNICKMODE_REGULAR);
   NickList.addNick("#IndianMasala", "I-a0007", IRCNICKMODE_REGULAR);
   NickList.addNick("#IndianMasala", "I-a0008", IRCNICKMODE_REGULAR);
   NickList.addNick("#IndianMasala", "I-a0009", IRCNICKMODE_REGULAR);
   NickList.addNick("#IndianMasala", "I-a0010", IRCNICKMODE_REGULAR);
   NickList.printDebug();

   cout << "Test 12: getNickMode(#IndianMasala, I-a0000)" << endl;
   cout << "         setNickMode(#IndianMasala, I-a0000, IRCNICKMODE_VOICE)" << endl;
   nickmode = NickList.getNickMode("#IndianMasala", "I-a0000");
   nickmode = nickmode | IRCNICKMODE_VOICE;
   NickList.setNickMode("#IndianMasala", "I-a0000", nickmode);
   NickList.setNickClient("I-a0000", IRCNICKCLIENT_MASALAMATE);
   NickList.setNickIP("I-a0000", 0xFFFFFFFF);
   NickList.printDebug();

   cout << "Test 13: setting I-a0000 as MM client NF" << endl;
   cout << "         setting I-a0004 as MM client FW" << endl;
   cout << "         setting I-a0010 as MM client UN" << endl;
   cout << "         setting I-a0008 as MM client MB" << endl;
   NickList.setNickClient("I-a0000", IRCNICKCLIENT_MASALAMATE);
   NickList.setNickFirewall("I-a0000", IRCNICKFW_NO);
   NickList.setNickClient("I-a0004", IRCNICKCLIENT_MASALAMATE);
   NickList.setNickFirewall("I-a0004", IRCNICKFW_YES);
   NickList.setNickClient("I-a0010", IRCNICKCLIENT_MASALAMATE);
   NickList.setNickFirewall("I-a0010", IRCNICKFW_UNKNOWN);
   NickList.setNickClient("I-a0008", IRCNICKCLIENT_MASALAMATE);
   NickList.setNickFirewall("I-a0008", IRCNICKFW_MAYBE);
   cout << "getMMNickCount() should be: 4 got: " << NickList.getMMNickCount("#IndianMasala") << endl;
   cout << "getMMNickIndex() for I-a0004 should be: 2 got " << NickList.getMMNickIndex("#IndianMasala", "I-a0004") << endl;
   cout << "getFWMMcount() should be: 3 got " << NickList.getFWMMcount("#IndianMasala") << endl;
   cout << "getNFMMcount() should be: 1 got " << NickList.getNFMMcount("#IndianMasala") << endl;
   cout << "getNickFWMMindex(I-a0002) should be: 0 got " << NickList.getNickFWMMindex("#IndianMasala", "I-a0002") << endl;
   cout << "getNickFWMMindex(I-a0008) should be: 2 got " << NickList.getNickFWMMindex("#IndianMasala", "I-a0008") << endl;
   cout << "getNickNFMMindex(I-a0000) should be: 1 got " << NickList.getNickNFMMindex("#IndianMasala", "I-a0000") << endl;
   cout << "getNickInChannelAtIndexNFMM(1) should be: I-a0000 got ";
   NickList.getNickInChannelAtIndexNFMM("#IndianMasala", 1, got_nick);
   cout << got_nick << endl;
   cout << "getNickInChannelAtIndexNFMM(2) should be empty got ";
   NickList.getNickInChannelAtIndexNFMM("#IndianMasala", 2, got_nick);
   cout << got_nick << endl;
   cout << "getNickInChannelAtIndexFWMM(1) should be: I-a0004 got ";
   NickList.getNickInChannelAtIndexFWMM("#IndianMasala", 1, got_nick);
   cout << got_nick << endl;
   cout << "getNickInChannelAtIndexFWMM(0) should be: empty got ";
   NickList.getNickInChannelAtIndexFWMM("#IndianMasala", 0, got_nick);
   cout << got_nick << endl;
   cout << "getNickInChannelAtIndexFWMM(3) should be: I-a0010 got ";
   NickList.getNickInChannelAtIndexFWMM("#IndianMasala", 3, got_nick);
   cout << got_nick << endl;

   cout << "getNickInChannelAtIndexFWMM(4) should be: empty got ";
   NickList.getNickInChannelAtIndexFWMM("#IndianMasala", 4, got_nick);
   cout << got_nick << endl;


   
}
