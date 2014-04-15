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

#include "OptionsDialog.hpp"
#include "TabBookWindow.hpp"
#include "StackTrace.hpp"
#include "Helper.hpp"

#include "Compatibility.hpp"

// Map
FXDEFMAP(OptionsDialog) OptionsDialogMap[]={
   FXMAPFUNC(SEL_COMMAND, OptionsDialog::ID_TEXTINPUT, OptionsDialog::onOptionsEntry),
   FXMAPFUNC(SEL_COMMAND, OptionsDialog::ID_DROPBOXSELECTION, OptionsDialog::onDropBoxSelection),
};

// DialogOptions implementation
FXIMPLEMENT(OptionsDialog, FXDialogBox, OptionsDialogMap, ARRAYNUMBER(OptionsDialogMap))

// Construct a dialog box
OptionsDialog::OptionsDialog(FXWindow* owner, FXFont *font): FXDialogBox(owner,CLIENT_NAME_FULL " :File->Options", DECOR_ALL, 30, 30, 600, 240) {

   TRACE();
   Parent = owner;
   contents = new FXVerticalFrame(this, FRAME_THICK|FRAME_RAISED|LAYOUT_FILL_X|LAYOUT_FILL_Y);

   Line1 = new FXHorizontalFrame(contents, FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL_X|LAYOUT_TOP);
   NickLabel =  new FXLabel(Line1, " Nick Name:      ");
   NickInput = new FXTextField(Line1, 0, NULL, 0, JUSTIFY_LEFT|FRAME_SUNKEN|FRAME_THICK|LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
   NickPassLabel =  new FXLabel(Line1, " Pass Word:      ");
   NickPassInput = new FXTextField(Line1, 0, NULL, 0, JUSTIFY_LEFT|FRAME_SUNKEN|FRAME_THICK|LAYOUT_SIDE_TOP|LAYOUT_FILL_X);

   ConnectionLine = new FXVerticalFrame(contents, FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_TOP);

   Line2 = new FXHorizontalFrame(ConnectionLine, FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL_X|LAYOUT_TOP);
   ConnectionLabel = new FXLabel(Line2, " Connection Type:    ");
   ConnectionListBox = new FXListBox(Line2, this, ID_DROPBOXSELECTION, JUSTIFY_LEFT|FRAME_SUNKEN|FRAME_THICK|LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
   ReconnectButton = new FXButton(Line2," Re&Connect ",NULL,this,ID_TEXTINPUT,BUTTON_DEFAULT|BUTTON_INITIAL|FRAME_RAISED|FRAME_THICK|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
   ConnectionListBox->appendItem("Direct");
   ConnectionListBox->appendItem("BNC");
   ConnectionListBox->appendItem("Socks4");
   ConnectionListBox->appendItem("Socks5");
   ConnectionListBox->appendItem("Proxy");
   ConnectionListBox->setNumVisible(5);

   Line3 = new FXHorizontalFrame(ConnectionLine, FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL_X|LAYOUT_TOP);
   HostLabel = new FXLabel(Line3,     " Connection Host: ");
   HostInput = new FXTextField(Line3, 0, NULL, 0, JUSTIFY_LEFT|FRAME_SUNKEN|FRAME_THICK|LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
   HostInput->setEditable(FALSE);
   PortLabel = new FXLabel(Line3,     " Connection Port: ");
   PortInput = new FXTextField(Line3, 0, NULL, 0, TEXTFIELD_INTEGER|JUSTIFY_LEFT|FRAME_SUNKEN|FRAME_THICK|LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
   PortInput->setEditable(FALSE);

   Line4 = new FXHorizontalFrame(ConnectionLine, FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL_X|LAYOUT_TOP);
   UserLabel =     new FXLabel(Line4, " Connection User: ");
   UserInput = new FXTextField(Line4, 0, NULL, 0, JUSTIFY_LEFT|FRAME_SUNKEN|FRAME_THICK|LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
   UserInput->setEditable(FALSE);
   UserPassLabel = new FXLabel(Line4, " Connection Pass: ");
   UserPassInput = new FXTextField(Line4, 0, NULL, 0, JUSTIFY_LEFT|FRAME_SUNKEN|FRAME_THICK|LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
   UserPassInput->setEditable(FALSE);

   ButtonLine = new FXHorizontalFrame(contents, FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL_X|LAYOUT_TOP);
   Accept = new FXButton(ButtonLine," &Accept ",NULL,this,ID_TEXTINPUT,BUTTON_DEFAULT|BUTTON_INITIAL|FRAME_RAISED|FRAME_THICK|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
   Cancel = new FXButton(ButtonLine," &Cancel ",NULL,this,ID_CANCEL,BUTTON_DEFAULT|FRAME_RAISED|LAYOUT_LEFT|LAYOUT_CENTER_Y);

   ConnectionIndex = 0; // Direct.
}

//Destructor
OptionsDialog::~OptionsDialog() {
   TRACE();
#if 0
   delete Cancel;
   delete Accept;
   delete ButtonLine;

   delete ConnectionListBox;
   delete ReconnectButton;
   delete NickPassInput;
   delete UserPassLabel;
   delete NickInput;
   delete NickLabel;

   delete PortInput;
   delete PortLabel;
   delete HostInput;
   delete HostLabel;

   delete UserPassInput;
   delete UserPassLabel;
   delete UserInput;
   delete UserLabel;

   delete Line4;
   delete Line3;
   delete Line2;
   delete Line1;
   delete ConnectionLine;

   delete contents;
#endif
}

long OptionsDialog::onDropBoxSelection(FXObject *obj, FXSelector sel, void *ptr) {
FXString DBSelection;

   TRACE();
   if (FXSELID(sel) != ID_DROPBOXSELECTION) return(0);

   ConnectionIndex = (FXint) ptr;
// Lets get the Drop Box Selected Item.
   DBSelection = ConnectionListBox->getItem(ConnectionIndex);

   if (ConnectionIndex == 0) {
//    This is the "DIRECT" Connection Method.
      HostInput->setEditable(FALSE);
      PortInput->setEditable(FALSE);
      UserInput->setEditable(FALSE);
      UserPassInput->setEditable(FALSE);
   }
   else {
      HostInput->setEditable(TRUE);
      PortInput->setEditable(TRUE);
      UserInput->setEditable(TRUE);
      UserPassInput->setEditable(TRUE);
   }
   return(1);
}

long OptionsDialog::onOptionsEntry(FXObject *obj, FXSelector sel, void *) {
FXButton *button = (FXButton *) obj;
TabBookWindow *owner;
FXString NickString, NickPass;
FXString Host, Port, User, UserPass;

   TRACE();

   if (FXSELID(sel) != ID_TEXTINPUT) return(0);

// Lets see which button was pressed;
   if ( (button == Accept) || (button == ReconnectButton) ) {
      owner = (TabBookWindow *) Parent;

//    Send the Info.
      NickString = NickInput->getText();
      NickPass = NickPassInput->getText();
      Host = HostInput->getText();
      Port = PortInput->getText();
      User = UserInput->getText();
      UserPass = UserPassInput->getText();
      owner->UIOptionsInput(NickString.text(), NickPass.text(), ConnectionIndex, Host.text(), Port.text(), User.text(), UserPass.text());
COUT(cout << "Calling UIOptionsInput: Nick: " << NickString.text() << " Nick Pass: " << NickPass.text() << " Connection Index: " << ConnectionIndex << " Host: " << Host.text() << " Port: " << Port.text() << " User: " << User.text() << " User Pass: " << UserPass.text() << endl;)

      if (button == ReconnectButton) {
      // We need to disconnect. and the rest should follow automatically.
          owner->UIReconnect();
      }

      this->hide();
   }
   return(1);
}

// This is called by TabBookWindow just before dialog->show is called
// so that it can have valid data in the options fields.
void OptionsDialog::setOptions() {
char Nick[64];
char Password[64];
ConnectionMethod CM;
TabBookWindow *owner = (TabBookWindow *) Parent;
const char *tmpstr;

   owner->UIgetExistingOptions(Nick, Password, CM);

   // Lets update the Options correctly.
   NickInput->setText(Nick);
   NickPassInput->setText(Password);

   switch (CM.howto()) {
      case CM_DIRECT:
        ConnectionIndex = 0;
        HostInput->setEditable(FALSE);
        PortInput->setEditable(FALSE);
        UserInput->setEditable(FALSE);
        UserPassInput->setEditable(FALSE);
        break;

      default:
        HostInput->setEditable(TRUE);
        PortInput->setEditable(TRUE);
        UserInput->setEditable(TRUE);
        UserPassInput->setEditable(TRUE);
        tmpstr = CM.getHost();
        HostInput->setText(tmpstr);
        char portnum[32];
        sprintf(portnum, "%d", CM.getPort());
        PortInput->setText(portnum);
        tmpstr = CM.getUser();
        UserInput->setText(tmpstr);
        tmpstr = CM.getPassword();
        UserPassInput->setText(tmpstr);
        break;
   }

   switch (CM.howto()) {
      case CM_BNC:
        ConnectionIndex = 1;
        break;

      case CM_SOCKS4:
        ConnectionIndex = 2;
        break;

      case CM_SOCKS5:
        ConnectionIndex = 3;
        break;

      case CM_PROXY:
        ConnectionIndex = 4;
        break;
   }
   ConnectionListBox->setCurrentItem(ConnectionIndex);
}
