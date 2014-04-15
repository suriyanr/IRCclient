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

#ifndef MASALAMATEAPPUI_H
#define MASALAMATEAPPUI_H

#include "FXEmbedApp.h"

class TabBookWindow;

// Main Application class
class MasalaMateAppUI : public FXEmbedApp {
  FXDECLARE(MasalaMateAppUI)
  friend class TabBookWindow;

protected:
   FXFont             *font;
   FXIcon             *wm_icon;
   FXIcon             *clear_icon;
   FXIcon             *portcheck_icon;
   FXIcon             *portcheckme_icon;
   FXIcon             *dccsend_icon;
   FXIcon             *font_icon;
   FXIcon             *help_icon;
   FXIcon             *partialdir_icon;
   FXIcon             *servingdir_icon;
   FXIcon             *options_icon;
   FXIcon             *findnext_icon;
   FXIcon             *findprevious_icon;
   FXIcon             *channeltextcurrent_icon;
   FXIcon             *channeltextnew_icon;

private:
  MasalaMateAppUI(){}

public:
  enum{
    ID_GOT_SIGNAL=FXApp::ID_LAST,
    ID_LAST
    };

   FXIcon             *trayicon_icon;
  long onGotSignal(FXObject*,FXSelector,void*);

  // Construct application object
  MasalaMateAppUI(const FXString& name);

  // Initialize application
  virtual void init(int& argc,char** argv);

  // Exit application
  virtual void exit(FXint code=0);

  // Delete application object
  virtual ~MasalaMateAppUI();
};

#endif

