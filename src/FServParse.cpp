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

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "FServParse.hpp"
#include "TCPConnect.hpp"
#include "StackTrace.hpp"

// Constructor
FServParse::FServParse() {
   TRACE();
   init();
}

// Destructor
FServParse::~FServParse() {
   TRACE();
   freeAll();
}

// freeAll()
void FServParse::freeAll() {
   TRACE();
   delete [] TriggerName;
   delete [] TriggerDottedIP;
   delete [] TriggerNick;
   delete [] FileName;
   delete [] PropagatedNick;
   delete [] Stripped;
   delete [] savedHostName;
}

// init()
void FServParse::init() {
   TRACE();
   Type = FSERVINVALID;
   TriggerName = NULL;
   TriggerNick = NULL;
   TriggerDottedIP = NULL;
   TriggerLongIP = 0;
   PropagatedNick = NULL;
   FileName = NULL;
   savedHostName = NULL;
   FileSize = 0;
   PackNumber = 0;
   ClientType = IRCNICKCLIENT_UNKNOWN;
   CurrentSends = 0;
   TotalSends = 0;
   CurrentQueues = 0;
   TotalQueues = 0;

   Stripped = NULL;
}

// Assignment with character string.
FServParse &FServParse::operator=(char *line) {
   TRACE();
   freeAll();
   init();
   parse(line);
}

TriggerTypeE FServParse::getTriggerType() {
   TRACE();
   return(Type);
}

bool FServParse::isIrofferFirewalled() {

   TRACE();

   return(IrofferFirewalled);
}

const char *FServParse::getTriggerName() {

   TRACE();
   return(TriggerName);
}

const char *FServParse::getPropagatedNick() {
   TRACE();

   return(PropagatedNick);
}

const char *FServParse::getTriggerNick() {

   TRACE();
   return(TriggerNick);
}

const char *FServParse::getTriggerDottedIP() {

   TRACE();
   return(TriggerDottedIP);
}

unsigned long FServParse::getTriggerLongIP() {

   TRACE();
   return(TriggerLongIP);
}

const char *FServParse::getFileName() {

   TRACE();
   return(FileName);
}

size_t FServParse::getFileSize() {

   TRACE();
   return(FileSize);
}

long FServParse::getPackNumber() {

   TRACE();
   return(PackNumber);
}

int FServParse::getCurrentSends() {

   TRACE();
   return(CurrentSends);
}

int FServParse::getTotalSends() {

   TRACE();
   return(TotalSends);
}

int FServParse::getCurrentQueues() {

   TRACE();
   return(CurrentQueues);
}

int FServParse::getTotalQueues() {

   TRACE();
   return(TotalQueues);
}

void FServParse::populateIP() {
TCPConnect *T;
char DotIP[20]; // max 3 + 1 + 3 + 1 + 3 + 1 + 3 + 1 + '\0

   TRACE();

   TriggerLongIP = T->getLongFromHostName(savedHostName);
   T->getDottedIpAddressFromLong(TriggerLongIP, DotIP);
   delete [] TriggerDottedIP;
   TriggerDottedIP = new char[strlen(DotIP) + 1];
   strcpy(TriggerDottedIP, DotIP);
}

// The guy which does all the parsing work.
void FServParse::parse(char *line) {
LineParse LP;
const char *parseptr;
char *newline;
char *tmpword;
const char *strip;
long templong;
int tempi;
int wordindex;
char KMG;
char *endptr;

   TRACE();
   if (line == NULL) return;
   LP = line;

   // Lets get the IP from the first word = host
   parseptr = LP.getWord(1);
   delete [] savedHostName;
   savedHostName = new char[strlen(parseptr) + 1];
   strcpy(savedHostName, parseptr);

   // Lets get the Nick from the second word.
   parseptr = LP.getWord(2);
   delete [] TriggerNick;
   TriggerNick = new char[strlen(parseptr) + 1];
   strcpy(TriggerNick, parseptr);

// Removed nick which is 1st word, host = second word
// Its better as, if nick starts with number
// it might be misinterpreted as color code.

   parseptr = LP.getWordRange(3, 0);
   newline = new char[strlen(parseptr) + 1];
   strcpy(newline, parseptr);
   strip = removeColors(newline);
   LP = (char *) strip;
   parseptr = LP.removeConsecutiveDeLimiters();
   strcpy(newline, parseptr);
   // newline is not disturbed through the do loops.
   // Allocate *tmpword for intermediate memory.
   // delete newline if proper trigger parse before returning
//COUT(cout << "FServParse: " << newline << endl;)

// Check if its a PROPAGATION line.
// PROPAGATION PropagatedNick Sends TotSends Qs TotQs
   do {
      LP = newline;
      parseptr = LP.getWord(1);
      if (strcasecmp(parseptr, "PROPAGATION") != 0) {
         break;
      }

      if (LP.getWordCount() != 6) break;

      // OK this is a propagation CTCP. Lets fill it up.
      // A Propagation CTCP is same as a FSERV CTCP, but with additionally
      // PropagatedNick filled up.
      Type = PROPAGATIONCTCP;
      ClientType = IRCNICKCLIENT_MASALAMATE;

      // 2nd word is the PropagatedNick
      parseptr = LP.getWord(2);
      PropagatedNick = new char[strlen(parseptr) + 1];
      strcpy(PropagatedNick, parseptr);

      // 3rd word is sends.
      parseptr = LP.getWord(3);
      CurrentSends = (int) strtoul(parseptr, NULL, 10);

      //  4th word is total sends.
      parseptr = LP.getWord(4);
      TotalSends = (int) strtoul(parseptr, NULL, 10);

      // 5th word is queues.
      parseptr = LP.getWord(5);
      CurrentQueues = (int) strtoul(parseptr, NULL, 10);

      // 6th word is queues.
      parseptr = LP.getWord(6);
      TotalQueues = (int) strtoul(parseptr, NULL, 10);

      // Fill up the usual MasalaMate trigger name.
      TriggerName = new char[strlen(TriggerNick) + 11]; 
      sprintf(TriggerName, "Masala of %s", TriggerNick);

      populateIP();
      delete [] newline;
      return;

   } while (false);
  
// Check if its an Iroffer Sends line.
// ** 5 packs **  0 of 1 slot open, Queue: 4/10, Record: 39.9KB/s
// ** 1 pack **  3 of 3 slots open
// ** 2 packs **  3 of 3 slots open, Record: 46.7KB/s
// ** 2 packs **  0 of 3 slots open, Queue: 3/10, Record: 14.6KB/s
   do {
      LP = newline;
      parseptr = LP.getWord(1);
      if (strcasecmp(parseptr, "**") != 0) {
         break;
      }

      parseptr = LP.getWord(3);
      if (strncasecmp(parseptr, "pack", 4) != 0) {
         break;
      }

      parseptr = LP.getWord(4);
      if (strcasecmp(parseptr, "**") != 0) {
         break;
      }

      parseptr = LP.getWord(6);
      if (strcasecmp(parseptr, "of") != 0) {
         break;
      }

      // slot or slots
      parseptr = LP.getWord(8);
      if (strncasecmp(parseptr, "slot", 4) != 0) {
         break;
      }

      // open or open,
      parseptr = LP.getWord(9);
      if (strncasecmp(parseptr, "open", 4) != 0) {
         break;
      }

      Type = SENDS_QS_LINE;
      ClientType = IRCNICKCLIENT_IROFFER;

      // 7th word = Total Sends
      parseptr = LP.getWord(7);
      TotalSends = (int) strtol(parseptr, NULL, 10);

      // 5th word = Sends Open.
      parseptr = LP.getWord(5);
      CurrentSends = TotalSends - (int) strtol(parseptr, NULL, 10);

      // 10th word = Queue: => we can get q information.
      parseptr = LP.getWord(10);
      if (strcasecmp(parseptr, "Queue:") == 0) {
         // LP is destroyed henceforth.

         parseptr = LP.getWord(11); // "3/10,"
         LP = (char *) parseptr;
         LP.setDeLimiter('/');
         parseptr = LP.getWord(1);
         CurrentQueues = (int) strtol(parseptr, NULL, 10);
         parseptr = LP.getWord(2);
         TotalQueues = (int) strtol(parseptr, NULL, 10);
      }
      if (TotalQueues == 0) TotalQueues = 10; // It has free queues anyway.

      delete [] newline;
      return;

   } while (false);

// Check if its an Iroffer Firewall line.
// ** Server Open To All, Firewall Workaround Port OFF **
// ** Server Open To All, Firewall Workaround Port 8124 **
// The "Open to All" could be "Voice Only" or "Op Only" etc.
   do {
      LP = newline;
      parseptr = LP.getWord(1);
      if (strcasecmp(parseptr, "**") != 0) {
         break;
      }

      parseptr = LP.getWord(2);
      if (strcasecmp(parseptr, "Server") != 0) {
         break;
      }

      tempi = LP.getWordCount();
      parseptr = LP.getWord(tempi);
      if (strcasecmp(parseptr, "**") != 0) {
         break;
      }

      tempi = tempi - 4;
      parseptr = LP.getWord(tempi);
      if (strcasecmp(parseptr, "Firewall") != 0) {
         break;
      }

      tempi++;
      parseptr = LP.getWord(tempi);
      if (strcasecmp(parseptr, "Workaround") != 0) {
         break;
      }

      tempi++;
      parseptr = LP.getWord(tempi);
      if (strcasecmp(parseptr, "Port") != 0) {
         break;
      }

      tempi++;
      // This is what we really want.
      Type = IROFFER_FIREWALL_LINE;
      ClientType = IRCNICKCLIENT_IROFFER;

      parseptr = LP.getWord(tempi);
      if (strcasecmp(parseptr, "OFF") == 0) {
         IrofferFirewalled = false;
      }
      else {
         IrofferFirewalled = true;
      }

      delete [] newline;
      return;

   } while (false);


// Check if its an XDCC offer line.
// Prereq = Nick first five characters = [IM]-
// #1 343x [535M] IV-HardCoreCompilation.avi || Har.....
// #1 23x [ 24M] File || Desc...
// #1 23x [1.1M] File || Desc...
// First Word:
//   First char = #
//   Rest = number (pack numner)
// Second Word:
//   Last Character = x
//   Rest = number
// Third Word:
//   First Character = [
// If strlen of Third Word == 1 => Fourth word is second part
//   Last Character = ]
//   Rest = size of file with K/M/G suffix
// Fourth|Fifth Word:
//   FileName (pack filename)
// Fifth|Sixth Word:
//   ||
   do {
      LP = newline;
      parseptr = LP.getWord(1);
      if (parseptr[0] != '#') {
         break;
      }
//      PackNumber = (long) atoi(&parseptr[1]);
      PackNumber = strtol(&parseptr[1], &endptr, 10);
      if (*endptr != '\0') {
         break;
      }
      // End of First Word

      parseptr = LP.getWord(2);
      tmpword = new char[strlen(parseptr) + 1];
      strcpy(tmpword, parseptr);
      if (tmpword[strlen(tmpword) - 1] != 'x') {
         delete [] tmpword;
         break;
      }
      tmpword[strlen(tmpword) - 1] = '\0';
//      templong = (long) atoi(tmpword);
      templong = strtol(tmpword, &endptr, 10);
      if (*endptr != '\0') {
         break;
      }
      delete [] tmpword; // Delete after endptr is accessed.
      // End of Second Word.

      parseptr = LP.getWord(3);
      if (parseptr[0] != '[') {
         break;
      }
      if (parseptr[1] == '\0') {
         // Case where 4th word is remainder of size information.
         parseptr = LP.getWord(4);
         tmpword = new char[strlen(parseptr) + 1];
         strcpy(tmpword, parseptr);
         wordindex = 6;
      }
      else {
         tmpword = new char[strlen(parseptr)];
         strcpy(tmpword, &parseptr[1]);
         wordindex = 5;
      }
      if (parseptr[strlen(parseptr) - 1] != ']') {
         delete [] tmpword;
         break;
      }

      tmpword[strlen(tmpword) - 1] = '\0';
//      templong = (long) atoi(tmpword);
      templong = strtol(tmpword, &endptr, 10);
      KMG = *endptr;
      if (KMG == '.') KMG = (endptr)[2];
      if ( (KMG != 'K') && (KMG != 'M') && (KMG != 'G') ) {
         break;
      }

//      tempi = 0;
//      while ( (tmpword[tempi] != 'K') && (tmpword[tempi] != 'M') && (tmpword[tempi] != 'G') && (tmpword[tempi] != '\0') ) tempi++;
//      KMG = tmpword[tempi];
//      if (KMG == '\0') break;

      if (KMG == 'K') FileSize = templong * 1024;
      else if (KMG == 'M') FileSize = templong * 1024 * 1024;
      else if (KMG == 'G') FileSize = templong * 1024 * 1024 * 1024;
      delete [] tmpword;
      // End of Third Word and/or Fourth Word.

      parseptr = LP.getWord(wordindex); // wordindex = 5 or 6 appropriately

// We dont do this check as other iroffers dont have this.
//      if (strcasecmp(parseptr, "||") != 0) {
//         break;
//      }
      // End of Fifth or Sixth Word
   
      parseptr = LP.getWord(wordindex - 1);
      FileName = new char[strlen(parseptr) + 1];
      strcpy(FileName, parseptr);
      // End of Fourth or Fifth Word

      Type = XDCC;
      ClientType = IRCNICKCLIENT_IROFFER;

      delete [] newline;
      return;

   } while (false);

// Check if its an FServ trigger.
// [Fserve Active] - Trigger:[/ctcp tommylee Imas whatever] - Users:[0/5] - Sends:[1/1] - Queues:[2/5] - Record CPS:[34.6kB/s by aglp2k] - Bytes Sent:[92.33GB] - Files Sent:[185] - Resends:[341] - Accesses:[5459] - Upload Speed:[10.1kB/s] - Download Speed:[14.8kB/s] - Current Bandwidth:[24.9kB/s] - SysReset 2.53
// First Word:
//   first Five chars = [Fserv
// Second Word:
//   Active]
// Third Word:
//   -
// Fourth Word:
//   first 9 chars = Trigger:[
// Fifth Word:
//   should be equal to Nick
// Remove fifth word (takes care of nick having [ or ])
// Trigger extraction ->
//   set DeLimiter = ]
//   getWord(2)
//   init Line with above string.
//   set DeLimiter = [
//   trigger = getWord(2)
//   init Line with above string.
//   getWord(1) == /ctcp, Set trigger type: FSERVCTCP
//   getWord(2-) == TriggerName
// Sends Extraction ->
//   replace "- Sends:[" with "\001"
//   set DeLimiter = '\001'
//   getWord(2)
//   init Line with above string.
//   set DeLimiter = /
//   getWord(1) = CurrentSends
//   getWord(2) = TotalSends
// Queues Extraction ->
//   replace "- Queues:[" with "\001"
//   set DeLimiter = '\001'
//   getWord(2)
//   init Line with above string.
//   set DeLimiter = /
//   getWord(1) = CurrentSends
//   getWord(2) = TotalSends

   do {
      LP = newline;
      parseptr = LP.getWord(1);
      if (strncasecmp(parseptr, "[FServ", 6) != 0) {
         break;
      }
      // End of First Word.

      parseptr = LP.getWord(2);
      if (strcasecmp(parseptr, "Active]") != 0) {
         break;
      }
      // End of Second Word.

      parseptr = LP.getWord(3);
      if (strcasecmp(parseptr, "-") != 0) {
         break;
      }
      // End of Third Word.

      parseptr = LP.getWord(4);
      if (strncasecmp(parseptr, "Trigger:[", 9) != 0) {
         break;
      }
      // End of Fourth Word.

      parseptr = LP.getWord(5);
      if (strcasecmp(parseptr, TriggerNick) != 0) {
         break;
      }
      // End of Fifth Word.

      // Remove Fifth word.
      tmpword = new char [strlen(newline) + 1];
      parseptr = LP.getWordRange(1, 4);
      strcpy(tmpword, parseptr);
      strcat(tmpword, " ");
      parseptr = LP.getWordRange(6, 0);
      strcat(tmpword, parseptr);
      LP = tmpword;
      // tmpword has the line without 5th word = Trigger Nick

      LP.setDeLimiter(']');
      parseptr = LP.getWord(2);
      strcpy(tmpword, parseptr);
      LP = tmpword;
      LP.setDeLimiter('[');
      parseptr = LP.getWord(2);
      strcpy(tmpword, parseptr);
      LP = tmpword;
      delete [] tmpword;
      parseptr = LP.getWord(1);
      if (strcasecmp(parseptr, "/ctcp") != 0) {
         break;
      }
      parseptr = LP.getWordRange(2, 0);
      TriggerName = new char[strlen(parseptr) + 1];
      strcpy(TriggerName, parseptr);
      
      Type = FSERVCTCP;

      // Get the client.
      LP = newline;
      // Set Default client as IRCNICKCLIENT_SYSRESET
      // Cause some users have long ads, which chops out the SysReset word
      ClientType = IRCNICKCLIENT_SYSRESET;

      if (LP.isWordsInLine("SysReset")) {
         ClientType = IRCNICKCLIENT_SYSRESET;
      }
      else if (LP.isWordsInLine("MasalaMate")) {
         ClientType = IRCNICKCLIENT_MASALAMATE;
      }
      else if (LP.isWordsInLine("Iroffer FServ")) {
         ClientType = IRCNICKCLIENT_SYSRESET;
      }

      // Sends extraction.
      //   replace "- Sends:[" with "\001"
      //   set DeLimiter = '\001'
      //   getWord(2)
      //   init Line with above string.
      //   set DeLimiter = /
      //   getWord(1) = CurrentSends
      //   getWord(2) = TotalSends
      LP = newline;
      parseptr = LP.replaceString("- Sends:[", "\001");
      LP = (char *) parseptr;
      LP.setDeLimiter('\001');
      parseptr = LP.getWord(2);
      LP = (char *) parseptr;
      LP.setDeLimiter('/');
      parseptr = LP.getWord(1);
      CurrentSends = (int) strtol(parseptr, NULL, 10);
      parseptr = LP.getWord(2);
      TotalSends = (int) strtol(parseptr, NULL, 10);

      // Queues Extraction ->
      //   replace "- Queues:[" with "\001"
      //   set DeLimiter = '\001'
      //   getWord(2)
      //   init Line with above string.
      //   set DeLimiter = /
      //   getWord(1) = CurrentSends
      //   getWord(2) = TotalSends
      LP = newline;
      parseptr = LP.replaceString("- Queues:[", "\001");
      LP = (char *) parseptr;
      LP.setDeLimiter('\001');
      parseptr = LP.getWord(2);
      LP = (char *) parseptr;
      LP.setDeLimiter('/');
      parseptr = LP.getWord(1);
      CurrentQueues = (int) strtol(parseptr, NULL, 10);
      parseptr = LP.getWord(2);
      TotalQueues = (int) strtol(parseptr, NULL, 10);

      populateIP();
      delete [] newline;
      return;

   } while (false);
   delete [] newline;
}

// Strip all color codes from line and return in stripped.
// Also make consecutive spaces as 1 space.
const char *FServParse::removeColors(char *line) {
LineParse LineP;
const char *parseptr; // Return pointer from LineP
int wordcount;
int i, j;

   TRACE();
   LineP = line;
// Turn all Ctrl O's as Color delimiters with white fg color. (Ctrl-C)
   delete [] Stripped;
   parseptr = LineP.replaceString("", "00,01");
// Turn all Ctrl B's as Color delimiters with white fg color.
   LineP = (char *) parseptr;
   parseptr = LineP.replaceString("", "00,01");
// Turn all Ctrl R's as Color delimiters with white fg color.
   LineP = (char *) parseptr;
   parseptr = LineP.replaceString("", "00,01");
// Turn all Ctrl U's as Color delimiters with white fg color. (Ctrl-_)
   LineP = (char *) parseptr;
   parseptr = LineP.replaceString("", "00,01");

   // Stripped needs one more extra charater as we concatenate the string.
   Stripped = new char [strlen(parseptr) + 2];
   strcpy(Stripped, parseptr);
   LineP = Stripped;
   LineP.setDeLimiter('');
   wordcount = LineP.getWordCount();
   Stripped[0] = '\0';
   for (i = 1; i <= wordcount; i++) {
      parseptr = LineP.getWord(i);
      j = 0;

      // isdigit has crashed - looks like for no reason, might have to use
      // our own implementation.
      if (isdigit(parseptr[j])) {
         j++;
         if (isdigit(parseptr[j])) {
            // two digit fg color code.
            j++;
         }
         if (parseptr[j] == ',') {
            j++;
            if (isdigit(parseptr[j])) {
               j++;
               if (isdigit(parseptr[j])) {
                  // two digit bg color code.
                  j++;
               }
            }
         }
      }
      strcat(Stripped, &parseptr[j]);
   }

   // Remove non printables
   LineP = Stripped;
   parseptr = LineP.removeNonPrintable();
   delete [] Stripped;

   // One more extra space for the code for removing consecutive spaces.
   // It appends an extra space at end which is later removed.
   Stripped = new char[strlen(parseptr) + 2];
   strcpy(Stripped, parseptr);

   // Now remove consecutive DeLimiter spaces if any.
   LineP = Stripped;
   parseptr = LineP.removeConsecutiveDeLimiters();
   strcpy(Stripped, parseptr);
   return((const char *) Stripped);
}

char FServParse::getClientType() {
   TRACE();

   return(ClientType);
}

void FServParse::printDebug() {
char *ip_string;

   TRACE();

   if (TriggerDottedIP == NULL) {
      ip_string = "NULL";
   }
   else {
      ip_string = TriggerDottedIP;
   }

   switch (Type) {
      case FSERVINVALID:
        COUT(cout << "Invalid FServ Parse Line" << endl;)
        break;

      case XDCC:
        COUT(cout << "XDCC: Nick: " << TriggerNick << " Long IP: " << TriggerLongIP << " Dotted IP: " << ip_string << " File: " << FileName << " FileSize: " << FileSize << " Pack: " << PackNumber << endl;)
        break;

      case FSERVCTCP:
        COUT(cout << "FSERVCTCP: Nick: " << TriggerNick << " Long IP: " << TriggerLongIP << " Dotted IP: " << ip_string << " Trigger: " << TriggerName << " Client Type: " << ClientType << " Current Sends: " << CurrentSends << " Total Sends: " << TotalSends << " Current Queues: " << CurrentQueues << " Total Queues: " << TotalQueues << endl;)
        break;

      case PROPAGATIONCTCP:
        COUT(cout << "PROPAGATIONCTCP: Nick: " << TriggerNick << " Propagated Nick: " << PropagatedNick << " Long IP: " << TriggerLongIP << " Dotted IP: " << ip_string << " Trigger: " << TriggerName << " Client Type: " << ClientType << " Current Sends: " << CurrentSends << " Total Sends: " << TotalSends << " Current Queues: " << CurrentQueues << " Total Queues: " << TotalQueues << endl;)
        break;

      case FSERVMSG:
        COUT(cout << "FSERVMSG: Stripped: " << Stripped;)
        break;

      case SENDS_QS_LINE:
        COUT(cout << "XDCC: Nick: " << TriggerNick << " Long IP: " << TriggerLongIP << " Dotted IP: " << ip_string << " Client Type: " << ClientType <<" Current Sends: " << CurrentSends << " Total Sends: " << TotalSends << " Current Queues: " << CurrentQueues << " Total Queues: " << TotalQueues << endl;)
        break;

      case IROFFER_FIREWALL_LINE:
        COUT(cout << "IROFFER FIREWALL LINE: Nick: " << TriggerNick << " Long IP: " << TriggerLongIP << " Dotted IP: " << ip_string << " Firewalled: ";)
        if (IrofferFirewalled) {
           COUT(cout << "YES" << endl;)
        }
        else {
           COUT(cout << "NO" << endl;)
        }
        break;
   }
}

