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

#include "ThreadMain.hpp"
#include "LineParse.hpp"
#include "IRCLineInterpret.hpp"
#include "IRCChannelList.hpp"
#include "IRCNickLists.hpp"
#include "UI.hpp"
#include "FServParse.hpp"
#include "SpamFilter.hpp"
#include "FilesDetailList.hpp"
#include "FileServer.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"
#include <unistd.h>

// Thread which is the File Server
void FileServerThr(DCC_Container_t *DCC_Container) {
THR_EXITCODE thr_exit = 1;
TCPConnect *FileServerTCP = DCC_Container->Connection;
XChange *XGlobal = DCC_Container->XGlobal;
char *DottedIP = DCC_Container->RemoteDottedIP;
FilesDetail *FD;
FileServer FS;
char *nicktrack = NULL;

   TRACE_INIT_NOCRASH();
   TRACE();

   // We are already connected via FileServer;
   // Just function as a File Server command processor.
   COUT(cout << "Entered: FileServerThr" << endl;)
   COUT(FileServerTCP->printDebug();)

   // Lets get the FD
   COUT(cout << "FileServerThr:FileServerWaiting: ";)
   COUT(XGlobal->FileServerWaiting.printDebug(NULL);)
   FD = XGlobal->FileServerWaiting.getFilesDetailListOfDottedIP(DottedIP);
   do {
      if (FD == NULL) break;
      COUT(cout << "FileServerThr:FileServerWaiting: ";)
      COUT(XGlobal->FileServerWaiting.printDebug(FD);)

      nicktrack = new char[strlen(FD->Nick) + 1];
      strcpy(nicktrack, FD->Nick);
      // Lets remove this entry from FileServerWaiting
      XGlobal->FileServerWaiting.delFilesOfNick(nicktrack);

      // And add it to the FileServerInProgress.
      XGlobal->FileServerInProgress.addFilesDetail(FD);
      FD = NULL;

      COUT(cout << "FileServerThr:FileServerInProgress: ";)
      COUT(XGlobal->FileServerInProgress.printDebug(NULL);)

      // Check if this nick is in CHANNEL_MAIN
      if (XGlobal->NickList.isNickInChannel(CHANNEL_MAIN, nicktrack) == false) {
         COUT(cout << "Not in CHANNEL_MAIN - FileServer rejected of " << nicktrack << endl;)
         break;
      }

      // We have the FD information. Lets give him the FileServer Interface.
      FS.run(DCC_Container);
   } while (false);

   // Now we delete it from the FileServerInProgress.
   // nicktrack might no longer be its valid nick, as during FS.run
   // it could have changed its nick.
   FD = XGlobal->FileServerInProgress.getFilesDetailListOfDottedIP(DottedIP);
   if (FD) {
      XGlobal->FileServerInProgress.delFilesOfNick(FD->Nick);
      XGlobal->FileServerInProgress.freeFilesDetailList(FD);
      FD = NULL;
   }

   delete [] nicktrack;
   delete FileServerTCP;
   delete [] DCC_Container->RemoteNick;
   delete [] DottedIP;
   delete DCC_Container;

   COUT(cout << "FileServerThr: Quitting" << endl;)
// We dont call Exit thread as we want to go out of scope => destructors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}

