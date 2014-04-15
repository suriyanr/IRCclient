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

#ifdef COUT_OUTPUT
#include <iostream>
using namespace std;
#endif

#include "LineParse.hpp"
#include "Utilities.hpp"
#include "StackTrace.hpp"
#include <string.h>
#include <ctype.h>

#include "Compatibility.hpp"

// Constructor.
LineParse::LineParse(char *line, char delimiter, bool ignorecase) {

   TRACE();
   init(line, delimiter, ignorecase);
}

// Assignment with character string.
// This can be used to assign stuff from its own RetPointer.
// Hence we free RetPointer last.
LineParse &LineParse::operator=(char *line) {
char *saveRetPtr = RetPointer;

   TRACE();
   freeLine();
   init(line, ' ', true);
   if (saveRetPtr) {
      delete [] saveRetPtr;
   }
}

void LineParse::freeAll() {
   TRACE();
   freeLine();
   freeRetPointer();
}

void LineParse::freeLine() {
   TRACE();
   if (Line != NULL) {
      delete [] Line;
      Line = NULL;
   }
}

void LineParse::freeRetPointer() {
   TRACE();
   if (RetPointer != NULL) {
      delete [] RetPointer;
      RetPointer = NULL;
   }
}

// Destructor.
LineParse::~LineParse() {
   TRACE();
   freeAll();
}

// This is a private function, hence called only from constructor.
// So nothing to free.
void LineParse::init(char *line, char delimiter, bool ignorecase) {

   TRACE();
   DeLimiter = delimiter;
   IgnoreCase = ignorecase;
   RetPointer = NULL;

   if (line != NULL) {
      Line = new char[strlen(line) + 1];
      strcpy(Line, line);
   }
   else {
      Line = NULL;
   }
}

// returns pointer to first character of word.
const char *LineParse::getWordStart(int wordindex) {
int i = 1;
const char *scanline = Line;

   TRACE();
   while (true) {
     if (i == wordindex) break;
     scanline = strchr(scanline, DeLimiter);
     if (scanline == NULL) {
//      In case we reached the end of line, we return the pointer to '\0'
        scanline = Line + strlen(Line);
        break;
     }
     else {
        scanline++; // Move past delimiter.
     }
     i++;
   }

   return(scanline);
}

// Returns pointer to last character of word.
const char *LineParse::getWordEnd(int wordindex) {
int i = 0;
const char *scanlinestart;
const char *scanlineend;

   TRACE();
   scanlinestart = getWordStart(wordindex);
   scanlineend = scanlinestart;

   while ( (scanlineend[0] != '\0') && (scanlineend[0] != DeLimiter) ) {
        scanlineend++;
   }
   if (scanlinestart != scanlineend) scanlineend--; // position last char.

   return(scanlineend);
}

// word indexing starts from 1 onwards.
const char *LineParse::getWordRange(int startindex, int endindex) {
int i = 1;
const char *wordstart;
const char *wordend;
int wordlen;

   TRACE();
   if ( (startindex < 1) || (Line == NULL) ) return(NULL);
   if (endindex == 0) endindex = getWordCount();

   freeRetPointer();

   if (endindex < startindex) {
      RetPointer = new char[1];
      RetPointer[0] = '\0';
      return(RetPointer);
   }

   wordstart = getWordStart(startindex);


   wordend = getWordEnd(endindex);
   wordlen = wordend - wordstart + 1;
   RetPointer = new char[wordlen + 1];
   strncpy(RetPointer, wordstart, wordlen);
   RetPointer[wordlen] = '\0';

//   COUT(cout << "Word.getWordRange: " << RetPointer << endl;)
   return(RetPointer);
}

const char *LineParse::getWord(int wordindex) {
   TRACE();
   return(getWordRange(wordindex, wordindex));
}

int LineParse::getWordCount() {
int i = 0;
char *scanline = Line;

   TRACE();
   if (scanline == NULL) return(i);
   i++;

   while (*scanline != '\0') {
     if (*scanline == DeLimiter) i++;
     scanline++;
   }
// If the end was a DeLimiter reduce count by 1.
   scanline--;
   if (*scanline == DeLimiter) i--;

//   COUT(cout << "Word.getWordCount: " << i << endl;)
   return(i);
}

// This sets RetPointer to contain the word range.
// word indexes are 1 and beyond passed as parameters.
bool LineParse::isCharInWordRange(char thechar, int startindex, int endindex) {
int intchar;
char *occur;
bool retval = false;

   TRACE();
   if (Line == NULL) return(retval);

// Lets get RetPointer set properly first.
   if (endindex == 0) {
      endindex = getWordCount();
   }
   getWordRange(startindex, endindex); // updates RetPointer.
   if (RetPointer == NULL) return(retval);

   if (IgnoreCase == false) {
      occur = strchr(RetPointer, thechar);
      if (occur != NULL) {
         retval = true;
      }
      return(retval);
   }

// Now lets do the IgnoreCase = true case.
   intchar = tolower(thechar);
   occur = strchr(RetPointer, intchar);
   if (occur == NULL) {
      intchar = toupper(thechar);
      occur = strchr(RetPointer, intchar);
   }
   if (occur != NULL) {
      retval = true;
   } 

   return(retval);
}

// SideEffect, RetPointer now contains Line.
bool LineParse::isCharInLine(char thechar) {

   TRACE();
   return(isCharInWordRange(thechar, 1, 0));
}

bool LineParse::isEqual(char *line) {
bool retval = false;

   TRACE();
   if ( (Line == NULL) || (line == NULL) ) return(false);

   if (IgnoreCase == true) {
      if (strcasecmp(Line, line) == 0) retval = true;
   }
   else {
      if (strcmp(Line, line) == 0) retval = true;
   }
   return(retval);
}

// Returns the word index of the input word in Line.
// returns 0 if word not present in line.
int LineParse::getIndexOfWordInLine(const char *word) {
int wordcount;
int wordindex;

   TRACE();

   if ( (Line == NULL) || (word == NULL) ) return(0);

   wordcount = getWordCount();
   if (wordcount == 0) return(0);

   for (wordindex = 1; wordindex <= wordcount; wordindex++) {
      getWordRange(wordindex, wordindex);
      // Now RetPointer contains the wordindex'th word.
      if (IgnoreCase == false) {
         if (strcmp(RetPointer, word) == 0) {
            return(wordindex);
         }
      }
      else {
         if (strcasecmp(RetPointer, word) == 0) {
            return(wordindex);
         }
      }
   }

   // Word not hit in line.
   return(0);
}

// This sets RetPointer to contain the word range.
// word indexes are 1 and beyond passed as parameters.
bool LineParse::isWordsInWordRange(char *words, int startindex, int endindex) {
int i;
bool retval = false;
const char *occur;

   TRACE();
   if ( (Line == NULL) || (words == NULL) ) return(retval);

// Lets get RetPointer set properly first.
   if (endindex == 0) {
      endindex = getWordCount();
   }
   getWordRange(startindex, endindex); // RetPointer is set.
   if (RetPointer == NULL) return(retval);

   if (IgnoreCase == false) {
      occur = strstr(RetPointer, words);
      if (occur != NULL) {
         retval = true;
      }
   }
   else {
      occur = strcasestr(RetPointer, words);
      if (occur != NULL) {
         retval = true;
      }
   }

   return(retval);
}

bool LineParse::isWordsInLine(char *words) {

   TRACE();
   return(isWordsInWordRange(words, 1, 0));
}

void LineParse::setDeLimiter(char delimiter) {
   TRACE();
   DeLimiter = delimiter;
}

void LineParse::setIgnoreCase(bool value) {
   TRACE();
   IgnoreCase = value;
}

// Remove non printable characters from the Line.
const char *LineParse::removeNonPrintable() {
int i = 0;
int j = 0;

   TRACE();
   if (Line == NULL) return(NULL);

   freeRetPointer();
   RetPointer = new char[strlen(Line) + 1];
   strcpy(RetPointer, Line);

   while (RetPointer[i] != '\0') {
      if (Line[i] >= ' ') {
         RetPointer[j] = Line[i];
         j++;
      }
      i++;
   }
   RetPointer[j] = '\0';
   return(RetPointer);
}

// Remove more than one consecutive delimiter if present.
// also remove the DeLimiter if present at the end of Line.
// also remove the DeLimiter if present at the start of Line.
// Example: "Two  spaces" becomes: "Two spaces"
// Return NULL only if input is NULL. Return an empty string instead of NULL
// if input line is an empty string too.
const char *LineParse::removeConsecutiveDeLimiters() {
int str_len;
int i, j;

   TRACE();
   if (Line == NULL) return(NULL);

   str_len = strlen(Line);

   freeRetPointer();
   RetPointer = new char[strlen(Line) + 1];

   if (str_len == 0) {
      RetPointer[0] = '\0';
      return(RetPointer);
   }

   j = 0;  // i points at source, j at destination.
   i = 0;
   while (i < (str_len - 1)) {
     if ( (Line[i] == DeLimiter) && (Line[i + 1] == DeLimiter) ) {
     }
     else {
       // This means , (i,i+1) = (ND, ND) or (ND, D) or (D, ND)
       if ( (j == 0) && (Line[i] == DeLimiter) ) {
          // This is the DeLimiter at the start of line.
       }
       else {
          RetPointer[j] = Line[i];
          j++;
       }
     }
     i++;
   }
   // We still have the last character which we need to copy over.
   if (Line[i] != DeLimiter) {
      RetPointer[j] = Line[i];
      j++;
   }
   RetPointer[j] = '\0';
   return(RetPointer);
}

// Replace occurences of string by another string in the Line.
const char *LineParse::replaceString(char *from, char *to) {
int i = 0, j = 0;
int fromlen;
int tolen = 0;
bool match;
int extralen = 1;
char *temp_ptr;

   TRACE();
   if ( (from == NULL) || (Line == NULL) ) return(NULL);

   freeRetPointer();
   fromlen = strlen(from);
   if (to) {
      tolen = strlen(to);
   }
   if (tolen > fromlen) {
     extralen = tolen - fromlen;
   }

// Allocate assuming each char will be replaced.
   RetPointer = new char[strlen(Line) * extralen + 2]; // With +1 there was 
//    memory corruption at last byte.

   while (Line[i] != '\0') {
      switch (IgnoreCase) {
         case true:
            if (strncasecmp(&Line[i], from, fromlen) == 0) match = true;
            else match = false;
            break;

         case false:
            if (strncmp(&Line[i], from, fromlen) == 0) match = true;
            else match = false;
            break;
      }
      if (match == true) {
         strcpy(&RetPointer[j], to);
         i = i + fromlen;
         j = j + tolen;
      }
      else {
         RetPointer[j] = Line[i];
         i++;
         j++;
      }
   }
   RetPointer[j] = '\0';
   temp_ptr = new char [j + 1];
   strcpy(temp_ptr, RetPointer);
   delete [] RetPointer;
   RetPointer = temp_ptr;
   return(RetPointer);
}
