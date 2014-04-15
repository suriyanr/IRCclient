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

#include "HistoryLines.hpp"

// Constructor.
HistoryLines::HistoryLines() {
  HistoryArray = NULL;
  setLines(HISTORY_LINES);
}

// Destructor.
HistoryLines::~HistoryLines() {
   for (int i = 0; i < Lines; i++) {
      delete [] HistoryArray[i];
   }
   delete [] HistoryArray;
}

// Sets the number of lines.
// Takes care if already lines present, in that case new lines should
// be greater than what is already there. No shrinkage allowed.
void HistoryLines::setLines(int entries) {
int i;
char **tmpArray;

   if (HistoryArray == NULL) {
   // Called from constructor. Just Allocate the Array.
      HistoryArray = new char*[entries];
      for (i = 0; i < entries; i++) {
         HistoryArray[i] = NULL;
      }
      Lines = entries;
//COUT(cout << "HistoryLines Constructor: Lines: " << Lines << endl;)
   }
   else {
      if (entries <= Lines) return;

      // Now we do the transfer into a bigger array.
      tmpArray = HistoryArray;
      HistoryArray = new char*[entries];
      for (i = 0; i < entries; i++) {
         if (i < Lines) {
            HistoryArray[i] = tmpArray[i];
         }
         else {
            HistoryArray[i] = NULL;
         }
      }
      delete [] tmpArray;
//COUT(cout << "HistoryLines Constructor: Lines increased to " << entries << " from " << Lines << endl;)
      Lines = entries;
   }
}

// Return the Next Line.
const char *HistoryLines::getNextLine() {
const char *retstr;
int i;

   retstr = HistoryArray[0];

// Shift them all one up. As line 2 is what should be the next one to
// be returned.
   for (i = 1; i < Lines; i++) {
      HistoryArray[i-1] = HistoryArray[i];
   }
   HistoryArray[Lines - 1] = NULL;
   // This returned string will be the last in array. So search for the
   // last NULL and plug it in there.
   for (i = Lines - 1; i >= 0; i--) {
      if (HistoryArray[i]) break;
   }
   if (HistoryArray[0] == NULL) {
     HistoryArray[0] = (char *) retstr;
   }
   else {
      HistoryArray[i + 1] = (char *) retstr;
   }
   return(retstr);
}

// Return the Previous Line.
const char *HistoryLines::getPreviousLine() {
const char *retstr;
int i;

// The first NON NULL string from the end is the one.
   for (i = Lines - 1; i >= 0; i--) {
      if (HistoryArray[i] != NULL) break;
   }
   if (HistoryArray[0] == NULL) {
      return(NULL);
   }
   retstr = HistoryArray[i];
   HistoryArray[i] = NULL;

// Shift them all one down.
   for (i = Lines - 1; i > 0; i--) {
      HistoryArray[i] = HistoryArray[i - 1];
   }
   HistoryArray[0] = (char *) retstr;
   return(retstr);
}

// add a Line.
void HistoryLines::addLine(char *varline) {

   delete [] HistoryArray[Lines - 1]; // Last one is gone.

   // Shift them all one down.
   for (int i = Lines - 1; i > 0; i--) {
      HistoryArray[i] = HistoryArray[i - 1];
   }

   // Add the line to position 1.
   HistoryArray[0] = new char[strlen(varline) + 1];
   strcpy(HistoryArray[0], varline);
}

void HistoryLines::printDebug() {
   for (int i = 0; i < Lines; i++) {
      if (HistoryArray[i]) {
         COUT(cout << i + 1 << ": " << HistoryArray[i] << endl;)
      }
   }
}
