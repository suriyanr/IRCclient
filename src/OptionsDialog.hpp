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

#ifndef OPTIONSDIALOGHPP
#define OPTIONSDIALOGHPP

#include <fx.h>

#include "Compatibility.hpp"

// Dialog Window
class OptionsDialog : public FXDialogBox {
   FXDECLARE(OptionsDialog)

protected:

   FXVerticalFrame *contents;

   FXHorizontalFrame *Line1;
   FXLabel *NickLabel;
   FXTextField *NickInput;
   FXLabel *NickPassLabel;
   FXTextField *NickPassInput;

   FXVerticalFrame *ConnectionLine;

   FXHorizontalFrame *Line2;
   FXHorizontalFrame *Line3;
   FXHorizontalFrame *Line4;

   FXLabel *ConnectionLabel;
   FXListBox *ConnectionListBox;
   FXButton *ReconnectButton;

   FXLabel *HostLabel;
   FXTextField *HostInput;
   FXLabel *PortLabel;
   FXTextField *PortInput;

   FXLabel *UserLabel;
   FXTextField *UserInput;
   FXLabel *UserPassLabel;
   FXTextField *UserPassInput;

   FXHorizontalFrame *ButtonLine;
   FXButton *Cancel;
   FXButton *Accept;

   FXObject *Parent;

   FXint ConnectionIndex;

private:
   OptionsDialog() {}

public:

   enum {
      ID_TEXTINPUT=FXDialogBox::ID_LAST,
      ID_DROPBOXSELECTION
   };

   // This is called by TabBookWindow just before dialog->show is called
   // so that it can have valid data in the options fields.
   void setOptions();

   long onOptionsEntry(FXObject *, FXSelector, void *);
   long onDropBoxSelection(FXObject *, FXSelector, void *);
   OptionsDialog(FXWindow *owner, FXFont *font);
   virtual ~OptionsDialog();
};


#endif
