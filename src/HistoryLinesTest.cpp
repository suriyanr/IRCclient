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
#include "HistoryLines.hpp"

using namespace std;

int main() {
HistoryLines IRCLines;
const char *retptr;

   cout << "Test 1: Print empty HistoryLines" << endl;
   IRCLines.printDebug();


   cout << "Test 1.1: empty HistoryLines: getNextLine()" << endl;
   retptr = IRCLines.getNextLine();
   if (retptr) {
      cout << retptr << endl;
   }

   cout << "Test 1.2: empty HistoryLines: getPreviousLine()" << endl;
   retptr = IRCLines.getPreviousLine();
   if (retptr) {
      cout << retptr << endl;
   }

   cout << "Test 2: Add Line 1" << endl;
   IRCLines.addLine("This is line 1");
   IRCLines.printDebug();

   cout << "Test 3: Add Line 2" << endl;
   IRCLines.addLine("This is Line 2");
   IRCLines.printDebug();

   cout << "Test 4: Add Line 3" << endl;
   IRCLines.addLine("This is Line 3, a little longer than the rest.");
   IRCLines.printDebug();

   cout << "Test 5: getNextLine()" << endl;
   cout << IRCLines.getNextLine() << endl;
   IRCLines.printDebug();

   cout << "Test 6: getNextLine()" << endl;
   cout << IRCLines.getNextLine() << endl;
   IRCLines.printDebug();

   cout << "Test 7: getPreviousLine()" << endl;
   cout << IRCLines.getPreviousLine() << endl;
   IRCLines.printDebug();

   cout << "Test 8: getPreviousLine()" << endl;
   cout << IRCLines.getPreviousLine() << endl;
   IRCLines.printDebug();

   cout << "Test 9: getPreviousLine()" << endl;
   cout << IRCLines.getPreviousLine() << endl;
   IRCLines.printDebug();

   cout << "Increasing lines to 11" << endl;
   IRCLines.setLines(11);

   cout << "Test 10: adding 13 lines 4 thru 16" << endl;
   IRCLines.addLine("Line 4");
   IRCLines.addLine("Line 5");
   IRCLines.addLine("Line 6");
   IRCLines.addLine("Line 7");
   IRCLines.addLine("Line 8");
   IRCLines.addLine("Line 9");
   IRCLines.addLine("Line 10");
   IRCLines.addLine("Line 11");
   IRCLines.addLine("Line 12");
   IRCLines.addLine("Line 13");
   IRCLines.addLine("Line 14");
   IRCLines.addLine("Line 15");
   IRCLines.addLine("Line 16");
   IRCLines.printDebug();
}
