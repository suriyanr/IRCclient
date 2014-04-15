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

#ifndef CLASS_SPAMFILTER
#define CLASS_SPAMFILTER

#include "Compatibility.hpp"

// We have a different syntax for a Rule regular expression.
// It consists of two rules seperated by the character '|'
// A rule is a list of words seperated by the character '+'
// The + takes the place of "AND" in the first rule
// The + takes the place of "OR" in the second rule
// example: word1+word2+word3 => line having word1 and word2 and word3
// '|' takes the place of -> but not.
// Example: word1+word2|word3+word4
// => line having word1 and word2 (but not having) word3 or word4.
// Simple isnt it ?

class SpamFilter {
public:
   SpamFilter();
   ~SpamFilter();
   void addRule(char *rule);
   void delRule(char *rule);
   const char *getRule(int index);
   bool isRulePresent(char *rule);
   bool isLineSpam(char *varline);
   void printDebug();

private:
   typedef struct SpamRuleHolder {
      char *SpamRule;
      struct SpamRuleHolder *Next;
   } SpamRuleHolder;
   SpamRuleHolder *Head;
};


#endif
