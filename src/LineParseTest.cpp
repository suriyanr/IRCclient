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
#include "LineParse.hpp"

using namespace std;

int main() {
const char *word;
LineParse *LINE_p;

   cout << "init: -> ERROR Z: Closing Link: Art-Test-Art[c-67...comcast.net] (Ping Timeout)" << endl;
   LINE_p = new LineParse("ERROR Z: Closing Link: Art-Test-Art[c-67...comcast.net] (Ping Timeout)");

   cout << "Test 1: Print Number of Words: " << LINE_p->getWordCount() << endl;

   cout << "Test 2: set Delimiter = : and print number of words: ";
   LINE_p->setDeLimiter(':');
   cout << LINE_p->getWordCount() << endl;

   cout << "Test 3: set Delimiter = E and print number of words: ";
   LINE_p->setDeLimiter('E');
   cout << LINE_p->getWordCount() << endl;

   cout << "Test 4: getWord(1): ";
   word = LINE_p->getWord(1);
   cout << word << endl;

   cout << "Test 5: getWord(2): ";
   word = LINE_p->getWord(2);
   cout << word << endl;

   cout << "Test 6: isCharInLINE('Z'): " << LINE_p->isCharInLine('Z') << endl;

   cout << "Test 7: isCharInLINE('z'): " << LINE_p->isCharInLine('z') << endl;

   cout << "Test 8: set case sensitive." << endl;
   LINE_p->setIgnoreCase(false);

   cout << "Test 9: isCharInLINE('Z'): " << LINE_p->isCharInLine('Z') << endl;

   cout << "Test 10: isCharInLINE('z'): " << LINE_p->isCharInLine('z') << endl;
   delete LINE_p;

   LineParse LINE("ERROR Z: Closing Link: Art-Test-Art[c-67...comcast.net] (Ping Timeout)");
   cout << "Test 11: setDelimiter = space" << endl;
   LINE.setDeLimiter(' ');

   cout << "Test 12: getWord(1): ";
   word = LINE.getWord(1);
   cout << word;
   cout << " getWord(2): ";
   word = LINE.getWord(2);
   cout << word;
   cout << " getWord(3): ";
   word = LINE.getWord(3);
   cout << word;
   cout << " getWord(4): ";
   word = LINE.getWord(4);
   cout << word << endl;

   LINE.setIgnoreCase(false);
   cout << "Test 13: Case Sensitive: isWordsInLine(\"error\"): " << LINE.isWordsInLine("error") << endl;

   LINE.setIgnoreCase(true);
   cout << "Test 14: Ignore Case: isWordsInLine(\"error\"): " << LINE.isWordsInLine("error") << endl;

    cout << "init: -> Chickmagalur buzzlightyear NFN vijaylair %bholaladka @Fire @aagantuk @cyprusrodt MsbA testing592 Shakeela schizosaint @c crazyboy lucky @a0000 king1234 @kazaawizkid replace_this_with_your_nick gunturboy bk sghas Ajman zzz seanz25 msi @Sur4802 geeks_hyd %bml @Samour SirFunk KamiKazi ZombieBombie @cowboys" << endl;
   LINE_p = new LineParse("Chickmagalur buzzlightyear NFN vijaylair %bholaladka @Fire @aagantuk @cyprusrodt MsbA testing592 Shakeela schizosaint @c crazyboy lucky @a0000 king1234 @kazaawizkid replace_this_with_your_nick gunturboy bk sghas Ajman zzz seanz25 msi @Sur4802 geeks_hyd %bml @Samour SirFunk KamiKazi ZombieBombie @cowboys");
   cout << LINE_p->getWordCount() << endl;
   for (int i = 1; i <= LINE_p->getWordCount(); i++) {
      word = LINE_p->getWord(i);
      cout << i << " " << word << endl;
   }
   delete LINE_p;

   cout << "init with =: Chumma time pass testcase" << endl;
   LINE = "Chumma time pass testcase";
   cout << "wordcount: " << LINE.getWordCount() << endl;
   cout << "word1: " << LINE.getWord(1) << endl;
   cout << "word3: " << LINE.getWord(3) << endl;
   cout << "word4: " << LINE.getWord(4) << endl;

   cout << "Replace test: replaceString(time, replaced)" << endl;
   cout << LINE.replaceString("time", "replaced") << endl;

   cout << "init with =: <Nickserv>      " << endl;
   LINE = "<Nickserv>      ";
   cout << "word1: " << LINE.getWord(1) << endl;

   cout << "init with =:Sen_kum!Sen_kum@me-augustacuda1cable6b-119.agstme.adelphia.net QUIT :Quit: " << endl;
   LINE = ":Sen_kum!Sen_kum@me-augustacuda1cable6b-119.agstme.adelphia.net QUIT :Quit: ";
   cout << "getWordRange(4,0): " << LINE.getWordRange(4, 0) << endl;
   cout << "isWordsInLine(net Qui): " << LINE.isWordsInLine("net qui") << endl;

   cout << "init with 12[06Fserve Active12] - Trigger:[06/ctcp david !full movies12] - Users:[060/512] - Sends:[062/212] - Queues:[060/1012] - Record CPS:[060B/s by None12] - Bytes Sent:[060B12] - Upload Speed:[069.7kB/s12] - Download Speed:[060B/s12] - SysReset 2.53" << endl;
   LINE = "12[06Fserve Active12] - Trigger:[06/ctcp david !full movies12] - Users:[060/512] - Sends:[062/212] - Queues:[060/1012] - Record CPS:[060B/s by None12] - Bytes Sent:[060B12] - Upload Speed:[069.7kB/s12] - Download Speed:[060B/s12] - SysReset 2.53";
   cout << "isWordsInLine(FServe Active): " << LINE.isWordsInLine("FServe Active") << endl;

   cout << "Test removeConsecutiveDeLimiters" << endl;
   cout << "Test 1: LINE = \"Normal Line with 5 words\"" << endl;
   LINE = "Normal Line with 5 words";
   cout << LINE.removeConsecutiveDeLimiters() << endl;
   cout << "Test 2: LINE = \"Normal  Line with 5  words\"" << endl;
   LINE = "Normal  Line with 5  words";
   cout << LINE.removeConsecutiveDeLimiters() << endl;
   cout << "Test 3: LINE = \" Normal  Line with 5  words\"" << endl;
   LINE = " Normal  Line with 5  words";
   cout << LINE.removeConsecutiveDeLimiters() << endl;
   cout << "Test 4: LINE = \"  Normal  Line with 5  words\"" << endl;
   LINE = "  Normal  Line with 5  words";
   cout << LINE.removeConsecutiveDeLimiters() << endl;
   cout << "Test 5: LINE = \"Normal  Line with 5  words \"" << endl;
   LINE = "Normal  Line with 5  words ";
   cout << LINE.removeConsecutiveDeLimiters() << endl;
   cout << "Test 6: LINE = \"Normal  Line with 5  words  \"" << endl;
   LINE = "Normal  Line with 5  words  ";
   cout << LINE.removeConsecutiveDeLimiters() << endl;
   cout << "Test 7: LINE = \" Normal  Line with 5  words  \"" << endl;
   LINE = " Normal  Line with 5  words  ";
   cout << LINE.removeConsecutiveDeLimiters() << endl;
   cout << "Test 8: LINE = \"  Normal  Line with 5  words  \"" << endl;
   LINE = "  Normal  Line with 5  words  ";
   cout << LINE.removeConsecutiveDeLimiters() << endl;

   cout << "Test 9: LINE = \"\"" << endl;
   LINE = "";
   cout << LINE.removeConsecutiveDeLimiters() << endl;
}
