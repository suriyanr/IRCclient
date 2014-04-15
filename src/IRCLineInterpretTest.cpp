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
#include "IRCLineInterpret.hpp"

using namespace std;

int main() {
IRCLineInterpret Line;
char word[128];

  // This is for /list output testing.
  cout << "init: -> :Matrix.mo.us.ircsuper.net 321 Sur4802 Channel User: Name" << endl;
  Line = ":Matrix.mo.us.ircsuper.net 321 Sur4802 Channel User: Name";
  Line.printDebug();

  cout << "init: -> :Matrix.mo.us.ircsuper.net 323 Sur4802 :End of /LIST" << endl;
  Line = ":Matrix.mo.us.ircsuper.net 323 Sur4802 :End of /LIST";
  Line.printDebug();

  cout << "init: -> :Matrix.mo.us.ircsuper.net 322 Sur4802 #whatever 21: Junk description" << endl;
  Line = ":Matrix.mo.us.ircsuper.net 322 Sur4802 #whatever 21: Junk description";
  Line.printDebug();

  // This is for CTCP FSERV test with IP.
  cout << "init: ->:Ice!~Aishwarya@c-67-161-27-122.client.comcast.net PRIVMSG censtitsT :\001MASALA of censtitsT 12345678\001" << endl;
  Line = ":Ice!~Aishwarya@c-67-161-27-122.client.comcast.net PRIVMSG censtitsT :\001MASALA of censtitsT 12345678\001";
  Line.printDebug();

   // This is for /ctcp nick NORESEND
   cout << "init: -> :IRC!IRC@flowerpower.fl.us.rc0.net PRIVMSG Art-Test-Art :\001NORESEND\001" << endl;
   Line = ":IRC!IRC@flowerpower.fl.us.rc0.net PRIVMSG Art-Test-Art :\001NORESEND\001";
   Line.printDebug();

   // This is for /ctcp nick PORTCHECK 8124
   cout << "init: -> :IRC!IRC@flowerpower.fl.us.rc0.net PRIVMSG Art-Test-Art :\001PORTCHECK 8124\001" << endl;
   Line = ":IRC!IRC@flowerpower.fl.us.rc0.net PRIVMSG Art-Test-Art :\001PORTCHECK 8124\001";
   Line.printDebug();

   // This is for Nick in use.
   cout << "init: -> :flowerpower.fl.us.rc0.net 433 Art-Test-Art :Nickname is already in use" << endl;
   Line = ":flowerpower.fl.us.rc0.net 433 Art-Test-Art :Nickname is already in use";
   Line.printDebug();

   // This is for the topic change.
   cout << "init: -> :Sur4802!~khamand@c-67-161-27-122.client.comcast.net TOPIC #Masala :test" << endl;
   Line = ":Sur4802!~khamand@c-67-161-27-122.client.comcast.net TOPIC #Masala :test";
   Line.printDebug();

   // this is for the @FIND
   cout << "init: -> :IRC!IRC@flowerpower.fl.us.rc0.net PRIVMSG Art-Test-Art :@FIND avi" << endl;
   Line = ":IRC!IRC@flowerpower.fl.us.rc0.net PRIVMSG Art-Test-Art :@FIND avi";
   Line.printDebug();

   // this is for the !LIST
   cout << "init: -> :IRC!IRC@flowerpower.fl.us.rc0.net PRIVMSG Art-Test-Art :!LIST nick" << endl;
   Line = ":IRC!IRC@flowerpower.fl.us.rc0.net PRIVMSG Art-Test-Art :!LIST nick";
   Line.printDebug();

   // this is for the !LIST

   // This is when we join channel
   cout << "init: -> :sundal1!sundal@192.168.0.21 JOIN #Masala-Chat" << endl;
   Line = ":sundal1!sundal@192.168.0.21 JOIN #Masala-Chat";
   Line.printDebug();

   // This is when others join channel
   cout << "init: -> :OrStlEth!~OrStlEth@192.168.0.26 JOIN :#IndianMasala" << endl;
   Line = ":OrStlEth!~OrStlEth@192.168.0.26 JOIN :#IndianMasala";
   Line.printDebug();

   cout << "init: -> ERROR :Closing Link: Art-Test-Art[c-67...comcast.net] (Ping Timeout)" << endl;
   Line = "ERROR :Closing Link: Art-Test-Art[c-67...comcast.net] (Ping Timeout)";
   Line.printDebug();

   cout << "init: -> :flowerpoer.fl.us.rc0.net NOTICE AUTH :Checking Ident" << endl;
   Line = ":flowerpoer.fl.us.rc0.net NOTICE AUTH :Checking Ident";
   Line.printDebug();

   cout << "init: -> :flowerpower.fl.us.rc0.net 001 Art-Test-Art :Welcome to RC0" << endl;
   Line = ":flowerpower.fl.us.rc0.net 001 Art-Test-Art :Welcome to RC0";
   Line.printDebug();

   cout << "init: -> :Art-Test-Art MODE Art-Test-Art :+i" << endl;
   Line = ":Art-Test-Art MODE Art-Test-Art :+i";
   Line.printDebug();

   cout << "init: -> PING :flowerpower.fl.us.rc0.net" << endl;
   Line = "PING :flowerpower.fl.us.rc0.net";
   Line.printDebug();

   cout << "init: -> :Sur4802!~noemail@c-67...comcast.net PRIVMSG Art-Test-Art : Hello" << endl;
   Line = ":Sur4802!~noemail@c-67...comcast.net PRIVMSG Art-Test-Art : Hello";
   Line.printDebug();

   cout << "init: -> :IRC!IRC@flowerpower.fl.us.rc0.net PRIVMSG Art-Test-Art :\001VERSION" << endl;
   Line = ":IRC!IRC@flowerpower.fl.us.rc0.net PRIVMSG Art-Test-Art :\001VERSION\001";
   Line.printDebug();

   cout << "init: -> :Sur4802!~noemail@c67-...comcast.net JOIN :#Masala-Chat" << endl;
   Line = ":Sur4802!~noemail@c67-...comcast.net JOIN :#Masala-Chat";
   Line.printDebug();

  cout << "init: -> :[IM]-Art0009!~Artist@whatever.com PRIVMSG #IndianMasala :#1 523x [24M] Thisthat.avi || Example test" << endl;
  Line = ":[IM]-Art0009!~Artist@whatever.com PRIVMSG #IndianMasala :#1 523x [24M] Thisthat.avi || Example test";
  Line.printDebug();

  cout << "init: ->:Ice!~Aishwarya@c-67-161-27-122.client.comcast.net PRIVMSG censtitsT :\001MASALA of censtitsT\001" << endl;
  Line = ":Ice!~Aishwarya@c-67-161-27-122.client.comcast.net PRIVMSG censtitsT :\001MASALA of censtitsT\001";
  Line.printDebug();

  cout << "init: ->:Ice!~Aishwarya@c-67-161-27-122.client.comcast.net NOTICE Stealth :\001PING 12345678\001" << endl;
  Line = ":Ice!~Aishwarya@c-67-161-27-122.client.comcast.net NOTICE Stealth :\001PING 12345678\001";
  Line.printDebug();

  cout << "init: ->:Sur4802!~khamand@c-67-169-190-238.hsd1.ca.comcast.net NOTICE Sur4802 :\001VERSION mIRC v6.16\001" << endl;
  Line = ":Sur4802!~khamand@c-67-169-190-238.hsd1.ca.comcast.net NOTICE Sur4802 :\001VERSION mIRC v6.16\001";
  Line.printDebug();

  cout << "init: -> 617 mynick :nick has been added to your DCC allow list" << endl;
  Line = "617 mynick :nick has been added to your DCC allow list";
  Line.printDebug();

  cout << "init: ->:Ice!~Aishwarya@c-67-161-27-122.client.comcast.net PRIVMSG Stealth :\001TIME\001" << endl;
  Line = ":Ice!~Aishwarya@c-67-161-27-122.client.comcast.net PRIVMSG Stealth :\001TIME\001";
  Line.printDebug();

  cout << "init: ->NOTICE Stealth :*** Your host is Matrix.mo.us.ircsuper.net, running version solid-ircd-3.4(07)dev" << endl;
  Line = "NOTICE Stealth :*** Your host is Matrix.mo.us.ircsuper.net, running version solid-ircd-3.4(07)dev";
  Line.printDebug();

  cout << "init: ->QUIT: :crazyacrazy!~WinNT@202.9.163.3 QUIT :Ping timeout" << endl;
  Line = ":crazyacrazy!~WinNT@202.9.163.3 QUIT :Ping timeout";
  Line.printDebug();

  cout << "init: ->:Ice!~Aishwarya@c-67-161-27-122.client.comcast.net PRIVMSG Stealth :\001FILESHA1 12345678 01234567890123456790 Some FileName .avi\001" << endl;
  Line = ":Ice!~Aishwarya@c-67-161-27-122.client.comcast.net PRIVMSG Stealth :\001FILESHA1 12345678 01234567890123456790 Some FileName .avi\001";
  Line.printDebug();

  cout << "init: ->:Ice!~Aishwarya@c-67-161-27-122.client.comcast.net PRIVMSG Stealth :\001FILESHA1REPLY 12345678 01234567890123456790 Some FileName .avi\001" << endl;
  Line = ":Ice!~Aishwarya@c-67-161-27-122.client.comcast.net NOTICE Stealth :\001FILESHA1 12345678 01234567890123456790 Some FileName .avi\001";
  Line.printDebug();

  cout << "init: ->:Ice!~Aishwarya@c-67-161-27-122.client.comcast.net PRIVMSG Stealth :\001UPGRADE asbcdef1234 12345678 MasalaMate.exe\001" << endl;
  Line = ":Ice!~Aishwarya@c-67-161-27-122.client.comcast.net PRIVMSG Stealth :\001UPGRADE asbcdef1234 12345678 MasalaMate.exe\001";
  Line.printDebug();

  cout << "init: ->:Ice!~Aishwarya@c-67-161-27-122.client.comcast.net NOTICE Stealth :\001UPGRADE asbcdef1234 12345678 MasalaMate.exe\001" << endl;
  Line = ":Ice!~Aishwarya@c-67-161-27-122.client.comcast.net NOTICE Stealth :\001UPGRADE asbcdef1234 12345678 MasalaMate.exe\001";
  Line.printDebug();

  cout << "init: ->:Stealth!Stealth@c-67-169-190-238.hsd1.ca.comcast.net QUIT :Quit: Be Right Back... - MasalaMate Win May 17th 2005 Beta" << endl;
  Line = ":Stealth!Stealth@c-67-169-190-238.hsd1.ca.comcast.net QUIT :Quit: Be Right Back... - MasalaMate Win May 17th 2005 Beta - Get it from http://ssss";
  Line.printDebug();

  cout << "init: ->:Stealth!Stealth@c-67-169-190-238.hsd1.ca.comcast.net PRIVMSG Ice :\001PROPAGATION TheNick 1 2 0 10\001" << endl;
  Line = ":Stealth!Stealth@c-67-169-190-238.hsd1.ca.comcast.net PRIVMSG Ice :\001PROPAGATION TheNick 1 2 0 10\001";
  Line.printDebug();

  cout << "init: ->:Stealth!Stealth@c-67-169-190-238.hsd1.ca.comcast.net PRIVMSG Ice :\001DCC SWARM \"VgKaviMasala-Vol1.avi\" 12345678 8124 58000000\001";
  Line = ":Stealth!Stealth@c-67-169-190-238.hsd1.ca.comcast.net PRIVMSG Ice :\001DCC SWARM \"VgKaviMasala-Vol1.avi\" 12345678 8124 58000000\001";
  Line.printDebug();

}
