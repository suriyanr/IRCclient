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
#include "StackTrace.hpp"

#include "Compatibility.hpp"
#include <unistd.h>

// Thread which gets lines from the ToServerNow Q, and writes to Server.
void toServerNowThr(XChange *XGlobal) {
char IRC_Line[1024];
THR_EXITCODE thr_exit = 1;

   TRACE_INIT_CRASH("ToServerNowThr");
   TRACE();
   while (true) {
      XGlobal->IRC_ToServerNow.getLineAndDelete(IRC_Line);
      if (XGlobal->isIRC_QUIT()) break;
      if (XGlobal->isIRC_DisConnected()) continue;

      COUT(cout << "toServerNowThr: " << IRC_Line << endl;)
      if (strlen(IRC_Line) > 1) {
         XGlobal->putLineIRC_Conn(IRC_Line);
      }
   }
   COUT(cout << "toServerNowThr: Quitting" << endl;)
// We dont call Exit thread as we want to go out of scope => destrcutors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}

