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


#include "ThreadMain.hpp"
#include "LineParse.hpp"
#include "IRCLineInterpret.hpp"
#include "IRCChannelList.hpp"
#include "IRCNickLists.hpp"
#include "UI.hpp"
#include "FServParse.hpp"
#include "SpamFilter.hpp"
#include "FilesDetailList.hpp"
#include "Helper.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Thread which updates the options got from user
void fromUIThr(XChange *XGlobal) {
IRCServerList SL;
IRCChannelList CL;
int i;
THR_EXITCODE thr_exit = 1;
Helper H;

   TRACE_INIT_CRASH("FromUIThr");
   TRACE();

   H.init(XGlobal);

//  We set the default information now.
#ifdef IRCSUPER
   SL.addServer("69.64.39.194", 6667);
   SL.addServer("69.64.39.194", 6665);
   SL.addServer("masalairc.redirectme.net", 6667);
   SL.addServer("masalairc.redirectme.net", 6665);
   SL.addServer("iirrcc.dynalias.net", 6667);
   SL.addServer("iirrcc.dynalias.net", 6665);
#else
   SL.addServer("scorpion");
   SL.addServer("irc.criten.net");
#endif
   XGlobal->putIRC_SL(SL);

   CL.addChannel(CHANNEL_MM, CHANNEL_MM_KEY);
   CL.addChannel(CHANNEL_CHAT, NULL);
   CL.addChannel(CHANNEL_MAIN, CHANNEL_MAIN_KEY);
   XGlobal->putIRC_CL(CL);

   H.readConfigFile();

// Lets create the ServingDir, PartialDir, directory
   XGlobal->lock();
#ifdef __MINGW32__
   mkdir(XGlobal->PartialDir);
#else
   mkdir(XGlobal->PartialDir, S_IRWXU);
#endif
   for (i = 0; i < FSERV_MAX_SERVING_DIRECTORIES; i++) {
      if (XGlobal->ServingDir[i]) {
#ifdef __MINGW32__
         mkdir(XGlobal->ServingDir[i]);
#else
         mkdir(XGlobal->ServingDir[i], S_IRWXU);
#endif
      }
      else break;
   }

   // Set up the Trace Dir for crashes.
   char *temp_ptr = new char[strlen(XGlobal->ServingDir[0]) + strlen(DIR_SEP) + 6];
   sprintf(temp_ptr, "%s%sCrash", XGlobal->ServingDir[0], DIR_SEP);
   Tracer.setTraceDir(temp_ptr);
   delete [] temp_ptr;

   XGlobal->unlock();

   fromUI(XGlobal);
   COUT(cout << "fromUIThr: Quitting" << endl;)
// We dont call Exit thread as we want to go out of scope => destrcutors
// of local variables will get called -> To help memory leak detection.
//   ExitThread(thr_exit);
}
