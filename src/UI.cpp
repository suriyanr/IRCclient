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


#ifdef USER_INTERFACE

#include <fx.h>
//#include <FXJPGIcon.h>
#include "FXEmbedApp.h"

#include "UI.hpp"
#include "MasalaMateAppUI.hpp"
#include "TabBookWindow.hpp"
#include "XChange.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

// Called by Thread fromUIThr. This updates the options got from user
void fromUI(XChange *XGlobal) {
TabBookWindow *UI = NULL;
int argc = 1;
char *argv[3] = { "Client", NULL, NULL };
#if 0
FXSplashWindow *Splash;
#endif

   TRACE();

   // Make Application
   MasalaMateAppUI application("MasalaMateAppUI");

   // Make a tool tip
   new FXToolTip(&application,0);

   // Create it.
   application.init(argc,argv);
   application.create();

   // Make the window
   UI = new TabBookWindow(&application, XGlobal);
   UI->create();

#if 0
   // Splash Window.
   splash_icon = new FXGIFIcon(&application, splash_gif);

   Splash = new FXSplashWindow(UI, splash_icon, SPLASH_SHAPED|SPLASH_OWNS_ICON|SPLASH_DESTROY,3000);
   Splash->create();
   Splash->show(PLACEMENT_OWNER);
#endif

   XGlobal->UI = (void *) UI;

   application.run();

#if 0
// If I delete UI here, then many threads core dump, as they expect UI
// to hold some valid value.
// Dont see any memory leak on not deleting UI, too.
// I think as the UI is associated with the application. The application
// explicitly frees it up.
   XGlobal->UI = NULL;
   delete UI;
#endif
// UI has quit. (we dont set IRC_QUIT, as that is initiated from TabBookWindow
   COUT(cout << "fromUI: quitting" << endl;)
}

// Called by Thread toUIThr. This updates the UI with data got from the UI Q.
// We dont update the UI directly, as fox is not thread safe.
// We create a pipe, we write to it on one side.
// the app->addInput() is done on it, waiting for a read.
// We write a char as soon as we get a line. we dont delete it.
// the Fox routine, reads the char, reads and deletes the line, processes it
// and returns. Hence each character we write is equivalent to one
// line to be read and processed.
// In fromUI(), before we create the application, we create a pipe.
// We put the reading end and writing end in XGlobal.
// The TabBookwindow constructor should addInput on the read descriptor.
void toUI(XChange *XGlobal) {
char IRC_Line[1024];
TabBookWindow *UI;
FXApp *App;
bool retvalb;

   TRACE();
//  Lets wait for the UI object to exist.
   while (true) {
      if (XGlobal->isIRC_QUIT()) break;

      if (XGlobal->UI != NULL) break;
      sleep(1);
   }
   UI = (TabBookWindow*) XGlobal->UI;
   App = UI->getApp();
   
   FXMutex &AppMutex = App->mutex();  // Add mutex for calls to UI funcitons.

   while (true) {

      if (XGlobal->isIRC_QUIT()) break;

#ifdef UI_SEM
      COUT(cout << "ToUI: Waiting on Sem" << endl;)
      WaitForSemaphore(XGlobal->UpdateUI_Sem);
      COUT(cout << "ToUI: Dec Sem" << endl;)
      if (XGlobal->isIRC_QUIT()) break;

      while (true) {
         // Loop here till the App is done with the ID_UI_UPDATE

         AppMutex.lock();
         retvalb = App->hasTimeout(UI, TabBookWindow::ID_UI_UPDATE);
         AppMutex.unlock();

         if (retvalb == false) break;
         if (XGlobal->isIRC_QUIT()) break;

         // The last issued timeout is still to be processed.
         COUT(cout << "ToUI: Waiting on previous addTimeout to finish." << endl;)
         msleep(100); // Sleep 100 milliseconds.
      }
#endif

      XGlobal->IRC_ToUI.getLineAndDelete(IRC_Line);
      if (XGlobal->isIRC_QUIT()) break;

      COUT(cout << "ToUI: Waking up UI." << endl;)

      // Technically we need to lock and call updateUI().
      // It works on Linux but hangs on Windows.
#if defined(UI_SEM) || !defined(WIN32)
      AppMutex.lock();
#endif

#ifdef UI_SEM
      App->addTimeout(UI, TabBookWindow::ID_UI_UPDATE, 0, IRC_Line);
#else
      UI->updateUI(IRC_Line);
#endif
#if defined(UI_SEM) || !defined(WIN32)
      AppMutex.unlock();
#endif
      // The Timeout will increase the sem on completion.
   }
}

#if 0
// We dont do anything with UI for a DCC Chat

// Called by Thread DCChatThr to create CHAT GUI Window.
void createDCCUI(XChange *XGlobal, char *windowname) {
TabBookWindow *UI = (TabBookWindow*) XGlobal->UI;

   TRACE();
   if (UI) UI->makeNewChatTabItem(windowname);
}
#endif

#else // The non UI Interface.

#include "XChange.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"


// Called by Thread fromUIThr.
void fromUI(XChange *XGlobal) {

   TRACE();
   while (true) {
      sleep(1);
   }
}

// Called by Thread toUIThr. 
void toUI(XChange *XGlobal) {
char IRC_Line[1024];

   TRACE();
   while (true) {
      XGlobal->IRC_ToUI.getLineAndDelete(IRC_Line);
   }
}

// Called by Thread DCChatThr to create CHAT GUI Window.
void createDCCUI(XChange *XGlobal, char *windowname) {

   TRACE();
}

#endif
