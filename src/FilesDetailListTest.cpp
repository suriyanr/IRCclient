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

#include "FilesDetailList.hpp"

int main() {
FilesDetailList FDL;
FilesDetail *FD;

   // getCheckSumString test.
   char chksum[128];
   FDL.getCheckSumString(48, 3, chksum);
   cout << "Test 0: getCheckSumString(48, 3, chksum) " << chksum << endl;
   exit(1);

   cout << "set TimeOut = 30 seconds" << endl;
   FDL.setTimeOut((time_t) 30);
   cout << "Test 1: Print empty FilesDetailList" << endl;
   FDL.printDebug(NULL);

   cout << "Test 2: Add one XDCC FilesDetail" << endl;
   FD = new FilesDetail;
   FD->FileName = new char[10];
   strcpy(FD->FileName, "File 2");
   FD->FileSize = 2024000;
   FD->Nick = new char[10];
   strcpy(FD->Nick, "Nick1");
   FD->TriggerType = XDCC;
   FD->TriggerName = NULL;
   FD->DirName = NULL;
   FD->PackNum = 1;
   FD->Next = NULL;
   FDL.addFilesDetail(FD);
   FDL.printDebug(NULL);
   sleep(2);

   cout << "Test 3: Add one CTCP FilesDetail" << endl;
   FD = new FilesDetail;
   FD->FileName = new char[10];
   strcpy(FD->FileName, "File 1");
   FD->FileSize = 2024000;
   FD->Nick = new char[10];
   strcpy(FD->Nick, "Nick2");
   FD->TriggerType = FSERVCTCP;
   FD->TriggerName = new char[15];
   strcpy(FD->TriggerName, "Trigger1");
   FD->DirName = NULL;
   FD->Next = NULL;
   FDL.addFilesDetail(FD);
   FDL.printDebug(NULL);
   sleep(2);

   cout << "Test 4: Add one FSERV MSG Files Detail" << endl;
   FD = new FilesDetail;
   FD->FileName = new char[10];
   strcpy(FD->FileName, "File 3");
   FD->FileSize = 2044000;
   FD->Nick = new char[10];
   strcpy(FD->Nick, "Nick3");
   FD->TriggerType = FSERVMSG;
   FD->TriggerName = new char[15];
   strcpy(FD->TriggerName, "Trigger2");
   FD->DirName = NULL;
   FD->Next = NULL;
   FDL.addFilesDetail(FD);
   FDL.printDebug(NULL);

   cout << "Test 5: Add one more FSERV MSG Files Detail" << endl;
   FD = new FilesDetail;
   FD->FileName = new char[10];
   strcpy(FD->FileName, "File 2");
   FD->FileSize = 2044000;
   FD->Nick = new char[10];
   strcpy(FD->Nick, "Nick4");
   FD->TriggerType = FSERVMSG;
   FD->TriggerName = new char[15];
   strcpy(FD->TriggerName, "Trigger2");
   FD->DirName = NULL;
   FD->Next = NULL;
   FDL.addFilesDetail(FD);
   FDL.printDebug(NULL);

   sleep(2);
   cout << "Test 6: FDL.searchFilesDetailList(*)" << endl;
   FD = FDL.searchFilesDetailList("*");
   FDL.printDebug(FD);

   sleep(2);
   cout << "Test 7: FDL.removeRepetitionsFilesDetailList(FD)" << endl;
   FD = FDL.removeRepetitionsFilesDetailList(FD);
   FDL.printDebug(FD);
   FDL.freeFilesDetailList(FD);

   sleep(2);
   cout << "Test 8: Add one more FSERV just like 1st one. Time stamp should get updated and also its count" << endl;
   FDL.updateTimeFilesOfNick("Nick1");
   if (FDL.isFilesOfNickPresent("Nick1")) {
      cout << "Files of Nick1 already present" << endl;
   }
   FD = new FilesDetail;
   FD->FileName = new char[10];
   strcpy(FD->FileName, "File 2");
   FD->FileSize = 2024000;
   FD->Nick = new char[10];
   strcpy(FD->Nick, "Nick1");
   FD->TriggerType = XDCC;
   FD->TriggerName = NULL;
   FD->DirName = NULL;
   FD->PackNum = 1;
   FD->Next = NULL;
   FDL.addFilesDetail(FD);
   FDL.printDebug(NULL);

   sleep(2);
   cout << "Test 9: Get getFilesDetailListMatchingFileAndSize(File 2, 2024000)" << endl;
   FD = FDL.getFilesDetailListMatchingFileAndSize("File 2", 2024000);
   FDL.printDebug(FD);
   FDL.freeFilesDetailList(FD);

   sleep(2);
   cout << "SEtting TimeOut to 0 for testing FServPending behavior" << endl;
   FDL.setTimeOut(0);
   cout << "Test 10: Add without FileName - just trigger" << endl;
   FD = new FilesDetail;
   FD->FileName = NULL;
   FD->FileSize = 0;
   FD->Nick = new char[20];
   strcpy(FD->Nick, "NickwithnoFile");
   FD->TriggerType = FSERVCTCP;
   FD->TriggerName = new char[40];
   strcpy(FD->TriggerName, "NickwithnoFile Trigger");
   FD->DirName = NULL;
   FD->Next = NULL;
   FDL.addFilesDetail(FD);
   FDL.printDebug(NULL);

   sleep(2);
   cout << "Test 11: updateTimeFilesOfNick(NickwithnoFile)" << endl;
   FDL.updateTimeFilesOfNick("NickwithnoFile");
   FDL.printDebug(NULL);

   cout << "Test 12: isFilesOfNickPresent(NickwithnoFile)" << endl;
   cout << FDL.isFilesOfNickPresent("NickwithnoFile") << endl;

   cout << "Test 13: isFilesOfNickPresent(NoNickwithnoFile)" << endl;
   cout << FDL.isFilesOfNickPresent("NoNickwithnoFile") << endl;
}
