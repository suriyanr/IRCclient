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

#include <fx.h>
#include "MasalaMateAppUI.hpp"

#include "Icons.hpp"

#include <signal.h>

#include "Compatibility.hpp"

// Map
FXDEFMAP(MasalaMateAppUI) MasalaMateAppUIMap[] = {
#if 0
  FXMAPFUNC(SEL_SIGNAL, MasalaMateAppUI::ID_GOT_SIGNAL, MasalaMateAppUI::onGotSignal),
#endif
  0
};

// Object implementation
FXIMPLEMENT(MasalaMateAppUI,FXEmbedApp,MasalaMateAppUIMap,ARRAYNUMBER(MasalaMateAppUIMap))

// Catch the signal.
MasalaMateAppUI::MasalaMateAppUI(const FXString& name):FXEmbedApp(name,FXString::null){

#if 0
  addSignal(SIGINT,this,ID_GOT_SIGNAL);
#ifndef __MINGW32__
  addSignal(SIGQUIT,this,ID_GOT_SIGNAL);
  addSignal(SIGHUP,this,ID_GOT_SIGNAL);
  addSignal(SIGPIPE,this,ID_GOT_SIGNAL);
#endif
#endif

}

// Initialize application
void MasalaMateAppUI::init(int& argc,char** argv){

  FXEmbedApp::init(argc,argv);

// Create all the FXIcons

   wm_icon = new FXGIFIcon(this, resource);

   // Lets get all the other icons.
   clear_icon = new FXGIFIcon(this, clear_gif);
   portcheck_icon = new FXGIFIcon(this, portcheck_gif);
   portcheckme_icon = new FXGIFIcon(this, portcheckme_gif);
   dccsend_icon = new FXGIFIcon(this, dccsend_gif);
   font_icon = new FXGIFIcon(this, font_gif);
   help_icon = new FXGIFIcon(this, help_gif);
   partialdir_icon = new FXGIFIcon(this, partialdir_gif);
   servingdir_icon = new FXGIFIcon(this, servingdir_gif);
   options_icon = new FXGIFIcon(this, options_gif);
   channeltextcurrent_icon = new FXGIFIcon(this, channeltextcurrent_gif);
   channeltextnew_icon = new FXGIFIcon(this, channeltextnew_gif);
   trayicon_icon = new FXICOIcon(this, TrayIcon);
}


// Exit application
void MasalaMateAppUI::exit(FXint code){

  // Writes registry, and quits
  FXEmbedApp::exit(code);
}



// Ignore the signals.
long MasalaMateAppUI::onGotSignal(FXObject*,FXSelector,void*){
   COUT(cout << "onGotSignal" << endl;)
   return(1);
}


// Clean up the mess
MasalaMateAppUI::~MasalaMateAppUI(){

// Free up all the icons.
   delete wm_icon;
   delete clear_icon;
   delete portcheck_icon;
   delete portcheckme_icon;
   delete dccsend_icon;
   delete font_icon;
   delete help_icon;
   delete partialdir_icon;
   delete servingdir_icon;
   delete options_icon;
   delete channeltextcurrent_icon;
   delete channeltextnew_icon;
   delete trayicon_icon;
}

