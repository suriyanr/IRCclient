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

#include <string.h>

#include "SpamFilter.hpp"
#include "LineParse.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

// Constructor.
SpamFilter::SpamFilter() {
   TRACE();
   Head = NULL;

// We add the rules here as it is kind of used globally.
// so remove the addRules in fromServerThr and move them here.
   addRule("Do+not+attempt+to+exploit+me.|masala");
   addRule("Server+Notice+dccserver|Trigger");
   addRule("DCC+Fserve+(+)|Server");
   addRule("DCC+Send+(+)|Server");
   addRule("DCC+Chat+(+)|Server");
   addRule("Auto+Away|_%_"); // Dummy _%_
   addRule("Server+Notice:+FileServer+busy|_%_"); // Dummy _%_
//   addRule("www+.|masala+sysreset+mirc");
//   addRule("http+.|masala+sysreset+mirc");
   addRule("server+join+#|masala");
   addRule("streamsale+com|masala");
   addRule("write+$decode(|_%_"); // Dummy _%_
   addRule("Come+watch+webcam+chat+me.mpg|_%_"); // Dummy _%_

//   addRule("www+.|moviez+sysreset+mirc");
//   addRule("http+.|moviez+masala+sysreset+mirc");
//   addRule("join|moviez+masala");
//   addRule("streamsale+com|moviez+masala");
}

// Destructor.
SpamFilter::~SpamFilter() {
SpamRuleHolder *Scan;

   TRACE();
   while (Head != NULL) {
      Scan = Head->Next;
      delete [] Head->SpamRule;
      delete Head;
      Head = Scan;
   }
}

// Guy who checks to see if input line is Spam or not based on the
// exiting rules.
// Rule is in form: word1+word2...|worda1+worda2...
bool SpamFilter::isLineSpam(char *varline) {
LineParse RuleLP, LineLP;
const char *parseptr;
const char *rule;
int index = 1;

// Used to continue on the while loop, on breaking out of for loop
// To try out the next rule set.
bool continue_while;

   TRACE();
   LineLP = varline;
   while (true) {
      rule = getRule(index);
      if (rule == NULL) break;
      continue_while = false;

      RuleLP = (char *) rule;
      RuleLP.setDeLimiter('|');
      parseptr = RuleLP.getWord(1); 
//COUT(cout << "First part of rule: " << parseptr << endl;)
      RuleLP = (char *) parseptr; // Holds the first part of rule.
      RuleLP.setDeLimiter('+');

//    We check if all words in the rule are present in the input line.
      for (int i = 1; i <= RuleLP.getWordCount(); i++) {
         parseptr = RuleLP.getWord(i);
         if (LineLP.isWordsInLine((char *) parseptr) == false) {
//          It doesnt have the words its supposed to have.
//          This rule is OK, check next rule.
            index++;
            continue_while = true;
            break;
         }
      }
      if (continue_while) continue;

//COUT(cout << "Satisfies rule 1" << endl;)
//    We are here cause all the words that should be present in line
//    are present.
//    Now check the second part of the rules.
      RuleLP = (char *) rule;
      RuleLP.setDeLimiter('|');
      parseptr = RuleLP.getWord(2);
//COUT(cout << "Second part of rule: " << parseptr << endl;)
      RuleLP = (char *) parseptr; // Holds the second part of rule.
      RuleLP.setDeLimiter('+');

      for (int i = 1; i <= RuleLP.getWordCount(); i++) {
         parseptr = RuleLP.getWord(i);
         if (LineLP.isWordsInLine((char *) parseptr) == true) {
//COUT(cout << parseptr << " in line, so not spam - moving to next rule" << endl;)
//          It is not spam as per this rule, but need to check the next rule.
            index++;
            continue_while = true;
            break;
         }
      }
      if (continue_while) continue;

      // The line is spam
      return(true);
   }

// Line is Not Spam.
   return(false);
}

// Check if the Rule is present in our list.
bool SpamFilter::isRulePresent(char *rule) {
SpamRuleHolder *Scan;

   TRACE();
   Scan = Head;
   while (Scan != NULL) {
      if (strcasecmp(rule, Scan->SpamRule) == 0) {
         return(true);
      }
      Scan = Scan->Next;
   }
   return(false);
}

// Return Rule at index. NULL if not present at index.
// index is 1 onwards.
const char *SpamFilter::getRule(int index) {
SpamRuleHolder *Scan;
int curindex = 1;

   TRACE();
   if (index < 1) return(NULL);

   Scan = Head;
   while (Scan != NULL) {
      if (curindex == index) {
         return(Scan->SpamRule);
      }
      Scan = Scan->Next;
      curindex++;
   }
   return(NULL);
}

// Add the rule to the list.
void SpamFilter::addRule(char *rule) {
SpamRuleHolder *NewItem;

   TRACE();
   if (isRulePresent(rule)) return;

   NewItem = new SpamRuleHolder;
   NewItem->SpamRule = new char[strlen(rule) + 1];
   strcpy(NewItem->SpamRule, rule);
   NewItem->Next = Head;
   Head = NewItem;
}

// Delete the rule from the list.
void SpamFilter::delRule(char *rule) {
SpamRuleHolder *Scan;
SpamRuleHolder *ScanButOne;

   TRACE();
   Scan = Head;
   ScanButOne = Head;
   while (Scan != NULL) {
      if (strcasecmp(Scan->SpamRule, rule) != 0) {
      // Lets move to the next one.
         ScanButOne = Scan;
         Scan = Scan->Next;
      }
      else {
      // Got hit, lets delete.
         if (Scan == ScanButOne) {
         // 1st Element
            Head = Head->Next;
            delete [] Scan->SpamRule;
            delete Scan;
         }
         else {
         // Not 1st Element. Head is intact.
            ScanButOne->Next = Scan->Next;
            delete [] Scan->SpamRule;
            delete Scan;
         }
         return;
      }
   }
}

void SpamFilter::printDebug() {
SpamRuleHolder *Scan;
int i = 1;

   TRACE();
   Scan = Head;
   while (Scan != NULL) {
      COUT(cout << i << " " << Scan->SpamRule << endl;)
      i++;
      Scan = Scan->Next;
   }
}
