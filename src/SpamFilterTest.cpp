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
#include "SpamFilter.hpp"

using namespace std;

int main() {
SpamFilter SF;
char buffer[1024];

   cout << "Test 1: Print uninitialised SpamFilter" << endl;
   SF.printDebug();

   cout << "Test 2: SF.addRule(\"www+.|masala+sysreset+mirc\")" << endl;
   SF.addRule("www+.|masala+sysreset+mirc");
   SF.printDebug();

   SF.addRule("join|masala");
   SF.addRule("streamsale+com|masala");

   cout << "Test 3: SF.addRule(\"http+.|masala+sysreset+mirc\")" << endl;
   SF.addRule("http+.|masala+sysreset+mirc");
   SF.printDebug();

   cout << "Test 4: SF.isLineSpam(\"hello there\")" << endl;
   cout << SF.isLineSpam("hello there") << endl;
   
   cout << "Test 5: SF.isLineSpam(\"msg noone\")" << endl;
   cout << SF.isLineSpam("msg noone") << endl;

   cout << "Test 6: SF.isLineSpam(\"www.yahoo.com\")" << endl;
   cout << SF.isLineSpam("www.yahoo.com") << endl;

   cout << "Test 7: SF.isLineSpam(\"http://www.yahoo.com\")" << endl;
   cout << SF.isLineSpam("http://www.yahoo.com") << endl;

   cout << "Test 8: SF.isLineSpam(\"www.masalaboard.com\")" << endl;
   cout << SF.isLineSpam("www.masalaboard.com") << endl;

   cout << "Test 9: SF.isLineSpam(\"join #masala\")" << endl;
   cout << SF.isLineSpam("join #masala") << endl;

   cout << "Test 10: SF.isLineSpam(\"join#masala\")" << endl;
   cout << SF.isLineSpam("join#masala") << endl;
}
