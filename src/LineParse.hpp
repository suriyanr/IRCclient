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

#ifndef CLASS_LINEPARSE
#define CLASS_LINEPARSE

#include <stdlib.h>
#include "Compatibility.hpp"


// This Class is for getting words out of lines,
// given a delimiter to use -> default delimiter is space.

class LineParse {
public:
   LineParse(char *line = NULL, char delimiter = ' ', bool ignorecase = true);
   ~LineParse();
   LineParse& operator=(char *); // Equating it to a character string.

   int  getWordCount();
   const char *getWord(int index = 1);
// If endindex == 0 => till end of line.
   const char *getWordRange(int startindex = 1, int endindex = 0);

   int getIndexOfWordInLine(const char *word);

   bool isCharInLine(char thechar = ' ');
   bool isCharInWordRange(char thechar = ' ', int startindex = 1, int endindex = 0);
   bool isWordsInLine(char *words);
   bool isWordsInWordRange(char *words, int startindex = 1, int endindex = 0);
   bool isEqual(char *line);

   void setDeLimiter(char delimiter = ' ');
   void setIgnoreCase(bool ignore=true);

   const char *removeNonPrintable();
   const char *removeConsecutiveDeLimiters();
   const char *replaceString(char *from, char *to);

private:
   void init(char *line, char delimiter = ' ', bool ignorecase = true); 
   void freeAll();
   void freeLine();
   void freeRetPointer();
   const char *getWordStart(int wordindex = 1);
   const char *getWordEnd(int wordindex = 1);


   char *Line;
   char *RetPointer;
   bool IgnoreCase; // default is true
   char DeLimiter;  // default is ' '
};

#endif
