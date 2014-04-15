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
#include <fxkeys.h>

#include "TabBookWindow.hpp"
#include "MasalaMateAppUI.hpp"
#include "FXEmbeddedWindow.h"
#include "FXTray.h"

#include "Icons.hpp"

#include "XChange.hpp"
#include "LineParse.hpp"
#include "IRCChannelList.hpp"
#include "IRCNickLists.hpp"
#include "StackTrace.hpp"
#include "Helper.hpp"
#include "SHA1.hpp"
#include "Utilities.hpp"
#include "HelpWindow.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>

#include "Compatibility.hpp"

// The Pattern list for DCC send FXFileDialog.
const FXchar FileDialogPatterns[]=
  "All Files (*)"
  "\nAVI Video (*.avi)"
  "\nWMV Video (*.wmv)"
  "\nMPG Video (*.mpg,*.mpeg, *.dat)"
  "\nRM Video (*.rm)"
;

// The mirc Color Mappings. example Color 0 = white.
unsigned short ColorMapping[16][3] = {
  { 255, 255, 255 }, // White
  {   0,   0,   0 }, // Black
  {   0,   0, 128 }, // Dark Blue
  {   0, 128,   0 }, // Dark Green
  { 255,   0,   0 }, // Bright Red
  { 165,  42,  42 }, // Brown
  { 128,   0, 128 }, // Purple
  { 255, 102,   0 }, // Dark Orange
  { 255, 255,   0 }, // Yellow
  {   0, 255,   0 }, // Bright Green
  {   0, 139, 139 }, // Cyan
  {   0, 255, 255 }, // Light Cyan
  {   0,   0, 255 }, // Bright Blue
  { 255,   0, 255 }, // Magenta
  { 128, 128, 128 }, // Grey
  { 211, 211, 211 }  // Light Grey
};

// Map
FXDEFMAP(TabBookWindow) TabBookWindowMap[]={
   FXMAPFUNC(SEL_CLOSE, 0, TabBookWindow::onQuit),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_BUTTON_TRAY, TabBookWindow::onTrayButton),
   FXMAPFUNC(SEL_COMMAND, FXTray::ID_TRAY_CLICKED, TabBookWindow::onTraySingleClicked),
   FXMAPFUNC(SEL_COMMAND, FXTray::ID_TRAY_DBLCLICKED, TabBookWindow::onTrayDoubleClicked),
   FXMAPFUNC(SEL_COMMAND, FXTray::ID_TRAY_CONTEXT_MENU, TabBookWindow::onTrayRightClicked),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_PANEL,TabBookWindow::onCmdPanel),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_TEXTINPUT,TabBookWindow::onTextEntry),
   FXMAPFUNC(SEL_KEYPRESS,TabBookWindow::ID_TEXTINPUT,TabBookWindow::onExpandTextEntry),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_SELECTPMNICK,TabBookWindow::onNickSelectPM),
   FXMAPFUNC(SEL_DOUBLECLICKED,TabBookWindow::ID_SELECTPMNICK,TabBookWindow::onDCNickSelect),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_SELECTCHANNELTABTODISPLAY,TabBookWindow::onChannelTabToDisplay),
#if 0
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_FILEOPTIONS, TabBookWindow::onFileOptions),
#endif
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_PARTIALDIR, TabBookWindow::onPartialDir),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_SETPARTIALDIR, TabBookWindow::onSetPartialDir),
   FXMAPFUNCS(SEL_COMMAND,TabBookWindow::ID_OPENSERVINGDIR, TabBookWindow::ID_OPENSERVINGDIR + FSERV_MAX_SERVING_DIRECTORIES - 1, TabBookWindow::onOpenServingDir),
   FXMAPFUNCS(SEL_COMMAND,TabBookWindow::ID_SETSERVINGDIR, TabBookWindow::ID_SETSERVINGDIR + FSERV_MAX_SERVING_DIRECTORIES - 1, TabBookWindow::onSetServingDir),
   FXMAPFUNCS(SEL_COMMAND,TabBookWindow::ID_UNSETSERVINGDIR, TabBookWindow::ID_UNSETSERVINGDIR + FSERV_MAX_SERVING_DIRECTORIES - 1, TabBookWindow::onUnSetServingDir),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_UPDATESERVINGLIST, TabBookWindow::onUpdateServingList),
   FXMAPFUNC(SEL_TIMEOUT,TabBookWindow::ID_CLOCKTIME, TabBookWindow::onClock),
   FXMAPFUNC(SEL_TIMEOUT,TabBookWindow::ID_PERIODIC_UI_UPDATE, TabBookWindow::onPeriodicUIUpdate),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_HELP, TabBookWindow::onHelp),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_FONT, TabBookWindow::onFont),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_TOOLS_ROLLBACK_TRUNCATE, TabBookWindow::onToolsRollbackTruncate),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_TOOLS_CHECK_UPGRADE, TabBookWindow::onToolsCheckUpgrade),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_TOOLS_REQUEST_UNBAN, TabBookWindow::onToolsRequestUnban),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_TOOLS_SET_TRAY_PASS, TabBookWindow::onToolsSetTrayPassword),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_HELPABOUT, TabBookWindow::onHelpAbout),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_HELPDEMOS, TabBookWindow::onHelpDemos),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_FIND_TEXT_WRAP_LEFT, TabBookWindow::onFindNextPrevious),
   FXMAPFUNC(SEL_COMMAND,TabBookWindow::ID_FIND_TEXT_WRAP_RIGHT, TabBookWindow::onFindNextPrevious),
   FXMAPFUNC(SEL_LEFTBUTTONRELEASE, TabBookWindow::ID_MOUSERELEASE, TabBookWindow::onMouseRelease),
   FXMAPFUNC(SEL_RIGHTBUTTONRELEASE, TabBookWindow::ID_MOUSERELEASE, TabBookWindow::onMouseRelease),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_CLEAR, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_PORTCHECK, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_PORTCHECKME, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_WHOIS, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_PING, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_TIME, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_VERSION, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_CLIENTINFO, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_DCCALLOWADD, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_DCCALLOWDEL, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_DCCSEND, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_DCCQUEUE, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_DCCCHAT, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_OP, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_DEOP, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_VOICE, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_DEVOICE, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_KICK, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_TOOLBAR_BAN, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_SCROLLCHECK, TabBookWindow::onScrollCheck),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_POPUP_LISTFILES, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_POPUP_GETLISTING, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_POPUP_SEARCHTEXTINFILES, TabBookWindow::onToolBar),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_QUIT_VERIFY, TabBookWindow::onQuit),
#ifdef UI_SEM   
   FXMAPFUNC(SEL_TIMEOUT, TabBookWindow::ID_UI_UPDATE, TabBookWindow::onUIUpdate),
#endif
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_BUTTON_SEARCH_FILE, TabBookWindow::onSearchFile),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_FILESEARCH_DOWNLOAD, TabBookWindow::onSearchPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_FILESEARCH_SEARCH_FILE, TabBookWindow::onSearchPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_FILESEARCH_LIST_NICK, TabBookWindow::onSearchPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_FILESEARCH_UPDATE_NICK, TabBookWindow::onSearchPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_FILESEARCH_CHECK_INTEGRITY, TabBookWindow::onSearchPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_FILESEARCH_FEDEX, TabBookWindow::onSearchPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_FILESEARCH_HEADER_CLICK, TabBookWindow::onSearchHeaderClick),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_DOWNLOADS_REREQUEST, TabBookWindow::onDownloadsPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_DOWNLOADS_CANCEL, TabBookWindow::onDownloadsPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_DOWNLOADS_SEARCH_FILE, TabBookWindow::onDownloadsPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_DOWNLOADS_LIST_NICK, TabBookWindow::onDownloadsPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_DOWNLOADS_UPDATE_NICK, TabBookWindow::onDownloadsPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_DOWNLOADS_CLEAR_PARTIAL_COMPLETE, TabBookWindow::onDownloadsPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_DOWNLOADS_CLEAR_ALL_PARTIAL_COMPLETE, TabBookWindow::onDownloadsPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_DOWNLOADS_CHECK_INTEGRITY, TabBookWindow::onDownloadsPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_WAITING_REREQUEST, TabBookWindow::onWaitingPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_WAITING_CANCEL, TabBookWindow::onWaitingPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_WAITING_SEARCH_FILE, TabBookWindow::onWaitingPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_WAITING_LIST_NICK, TabBookWindow::onWaitingPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_WAITING_UPDATE_NICK, TabBookWindow::onWaitingPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_WAITING_CHECK_INTEGRITY, TabBookWindow::onWaitingPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_FILESERVER_FORCESEND, TabBookWindow::onFileServerPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_FILESERVER_ABORTSEND, TabBookWindow::onFileServerPopUp),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_SWARM_QUIT, TabBookWindow::onSwarmPopUp),
   FXMAPFUNC(SEL_RIGHTBUTTONRELEASE, TabBookWindow::ID_SELECTPMNICK, TabBookWindow::onNickSelectPM),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_NICKNAME_SET, TabBookWindow::onSetNickName),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_NICKPASS_SET, TabBookWindow::onSetNickPass),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_CONNECTION_TYPE, TabBookWindow::onConnectionTypeChanged),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_CONNECTION_RECONNECT, TabBookWindow::onConnectionReConnect),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_CONNECTION_HOST, TabBookWindow::onConnectionValuesChanged),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_CONNECTION_PORT, TabBookWindow::onConnectionValuesChanged),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_CONNECTION_USER, TabBookWindow::onConnectionValuesChanged),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_CONNECTION_PASS, TabBookWindow::onConnectionValuesChanged),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_CONNECTION_VHOST, TabBookWindow::onConnectionValuesChanged),
   FXMAPFUNC(SEL_COMMAND, TabBookWindow::ID_UPGRADE_SERVER, TabBookWindow::onUpgradeServer),
   
};

// Object implementation
FXIMPLEMENT(TabBookWindow,FXMainWindow,TabBookWindowMap,ARRAYNUMBER(TabBookWindowMap))

// This returns a new tab item which is for Listing Searches.
void TabBookWindow::makeNewSearchTabItem(char *WindowName) {
FXTabItem *tab;
FXHorizontalFrame *horfr1, *boxframe;
FXVerticalFrame *scrollframe;
FXFoldingList *foldinglist = NULL;
FXHeader *FLHeader;
int index;

   TRACE();
   index = getNewWindowIndex();
   if (index == -1) return;

   tab=new FXTabItem(tabbook, WindowName, NULL);
   tab->setTabOrientation(TAB_BOTTOM);
   tab->setTextColor(FXRGB(0,0,255));
   scrollframe=new FXVerticalFrame(tabbook,FRAME_LINE, 0,0,0,0, 0,0,0,0,0,0);
   boxframe=new FXHorizontalFrame(scrollframe,FRAME_LINE|LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_TOP, 0,0,0,0, 0,0,0,0,0,0);
   horfr1 = new FXHorizontalFrame(boxframe, FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL|LAYOUT_TOP, 0,0,0,0, 0,0,0,0); // for text

   //foldinglist = new FXFoldingList(horfr1, this, TabBookWindow::ID_MOUSERELEASE, LAYOUT_SIDE_TOP|LAYOUT_FILL, 0,0,0,0);
   foldinglist = new FXFoldingList(horfr1, this, TabBookWindow::ID_MOUSERELEASE, FOLDINGLIST_SHOWS_BOXES|FOLDINGLIST_ROOT_BOXES|LAYOUT_SIDE_TOP|LAYOUT_FILL|ICONLIST_DETAILED, 0,0,0,0);

   // Make the header resizable like it was when we used FXIconList.
   FLHeader = foldinglist->getHeader();
   FXuint header_style = FLHeader->getHeaderStyle();
   header_style = header_style | HEADER_RESIZE;
   FLHeader->setHeaderStyle(header_style);

   boxframe=new FXHorizontalFrame(scrollframe,FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL_X|LAYOUT_TOP, 0,0,0,0, 0,0,0,0);
   if (strcasecmp(WindowName, TAB_FILESEARCH) == 0) {
      // FileSearch TAB needs one more bottom frame.
      FileSearch2ndBottomFrame = new FXHorizontalFrame(scrollframe,FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL_X|LAYOUT_TOP, 0,0,0,0, 0,0,0,0);
   }

   foldinglist->setFont(font);
   foldinglist->setBackColor(FXRGB(0, 0, 0));
   foldinglist->setTextColor(FXRGB(255, 255, 255));

   strcpy(Windows[index].Name, WindowName);
   Windows[index].TabUI = tab;
   Windows[index].ScrollUI = NULL;
   Windows[index].SearchUI = foldinglist;
   Windows[index].BottomFrame = boxframe;
}

// This returns a new Server FXText item contained in a tab.
// returns the index that was created.
int TabBookWindow::makeNewServerTabItem(char *WindowName) {
FXTabItem *tab;
FXHorizontalFrame *boxframe;
FXVerticalFrame *scrollframe;
FXText *simplescroll;
int index;

   TRACE();
   index = getNewWindowIndex();
   if (index == -1) return(index);

   // tab=new FXTabItem(tabbook, NETWORK_UI_NAME, NULL);
   tab=new FXTabItem(tabbook, WindowName, NULL);
   tab->setTabOrientation(TAB_BOTTOM);
   tab->setTextColor(FXRGB(0,0,255));

   scrollframe=new FXVerticalFrame(tabbook,FRAME_LINE, 0,0,0,0, 0,0,0,0,0,0);
   boxframe=new FXHorizontalFrame(scrollframe,FRAME_LINE|LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_TOP, 0,0,0,0, 0,0,0,0,0);
//   simplescroll=new FXText(boxframe, NULL, 0, TEXT_WORDWRAP|LAYOUT_SIDE_TOP|LAYOUT_FILL, 0,0,0,0);
   simplescroll=new FXText(boxframe, this, TabBookWindow::ID_MOUSERELEASE, TEXT_WORDWRAP|LAYOUT_SIDE_TOP|LAYOUT_FILL, 0,0,0,0);
//   COUT(cout << "wrapcolumns: " << simplescroll->getWrapColumns() << endl;)

   boxframe = new FXHorizontalFrame(scrollframe,FRAME_LINE|LAYOUT_FILL_X|LAYOUT_TOP, 0,0,0,0, 0,0,0,0);

   // Removed - and [ and ] from what is present originally.
   simplescroll->setDelimiters("~.,/\\`'!@#$%^&*()=+|\":;<>?");

   simplescroll->setEditable(FALSE);
   simplescroll->setStyled();
   simplescroll->setFont(font);
   simplescroll->setHiliteStyles(styles);

   simplescroll->setBackColor(FXRGB(0, 0, 0));
   simplescroll->setActiveBackColor(FXRGB(0, 0, 0));
   simplescroll->setTextColor(FXRGB(255, 255, 255));
   strcpy(Windows[index].Name, WindowName);
   Windows[index].TabUI = tab;
   Windows[index].ScrollUI = simplescroll;
   Windows[index].SearchUI = NULL;
   Windows[index].NickListUI = NULL;
   Windows[index].BottomFrame = boxframe;
   Windows[index].InputUI = NULL;
   Windows[index].LabelUI = NULL;
   return(index);
}

// This makes a new Label Item in the specified frame.
FXLabel *TabBookWindow::makeNewBottomFrameLabel(FXHorizontalFrame *boxframe, char *label_str) {
int winindex;
FXLabel *label;

   TRACE();

   if (boxframe == NULL) return(NULL);

   label = new FXLabel(boxframe, label_str);
   label->setTextColor(FXRGB(0, 0, 0));

   return(label);
}

// This makes the said Button in the Bottom Frame.
// Used by the File Search UI.
FXButton *TabBookWindow::makeNewBottomFrameButton(FXHorizontalFrame *boxframe, char *button_str, int Sel) {
FXButton *button;

   TRACE();
   if (boxframe == NULL) return(NULL);

   button = new FXButton(boxframe, button_str, App->channeltextcurrent_icon, this, Sel, LAYOUT_FILL_Y|ICON_ABOVE_TEXT|BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_LEFT);

   // A Spacer
   new FXVerticalSeparator(boxframe);

   return(button);
}

// This makes an FXTextField Input in the Bottom Frame.
FXTextField *TabBookWindow::makeNewBottomFrameInput(FXHorizontalFrame *boxframe) {
FXTextField *inputfield;

   TRACE();
   if (boxframe == NULL) return(NULL);

   inputfield=new FXTextField(boxframe, 0, this, TabBookWindow::ID_TEXTINPUT, TEXTFIELD_ENTER_ONLY|JUSTIFY_LEFT|FRAME_SUNKEN|FRAME_THICK|LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
   inputfield->setBackColor(FXRGB(0, 0, 0));
   inputfield->setTextColor(FXRGB(255, 255, 255));

// Set the Cursor color for the input field.
   inputfield->setCursorColor(FXRGB(255, 255, 255));
   inputfield->setFont(font);

   return(inputfield);

   // Windows[winindex].InputUI = inputfield;

   // If it has a Input Text Field, then it should have a History
   // Windows[winindex].History = new HistoryLines;
}

// Used to create the 6 Combo Boxes in TAB_FILESEARCH
FXComboBox *TabBookWindow::makeNewBottomFrameComboBox(FXHorizontalFrame *boxframe, FXint ncols, char *str1, char *str2, char *str3, char *str4, char *str5, char *str6, char *str7, char *str8) {
FXComboBox *combobox;
FXint num_visible = 0;

   TRACE();

   combobox = new FXComboBox(boxframe, ncols, NULL, 0, COMBOBOX_NORMAL | COMBOBOX_STATIC);
   combobox->setBackColor(FXRGB(0, 0, 0));
   combobox->setTextColor(FXRGB(255, 255, 255));
   combobox->setSelBackColor(FXRGB(0, 0, 0));
   combobox->setSelTextColor(FXRGB(255, 255, 255));

   if (str1) {
      combobox->appendItem(str1);
      num_visible++;
   }
   if (str2) {
      combobox->appendItem(str2);
      num_visible++;
   }
   if (str3) {
      combobox->appendItem(str3);
      num_visible++;
   }
   if (str4) {
      combobox->appendItem(str4);
      num_visible++;
   }
   if (str5) {
      combobox->appendItem(str5);
      num_visible++;
   }
   if (str6) {
      combobox->appendItem(str6);
      num_visible++;
   }
   if (str7) {
      combobox->appendItem(str7);
      num_visible++;
   }
   if (str8) {
      combobox->appendItem(str8);
      num_visible++;
   }

   combobox->setNumVisible(num_visible);

   return(combobox);
}

// This makes a new Channel Tab Item. (Just scroll area and nick list area.
// Also the ChannelListUI. (only if WindowName is not TAB_MESSAGES)
int TabBookWindow::makeNewChannelTabItem(char *WindowName) {
FXTabItem *tab;
FXHorizontalFrame *boxfr1, *boxfr2, *horfr1, *horfr2, *boxframe;
FXVerticalFrame *scrollframe;
FXSplitter *splitter;
FXText *simplescroll;
FXList *simplelist, *ChannelList = NULL;
bool messages_tab = false;
int index;

   TRACE();
   index = getNewWindowIndex();
   if (index == -1) return(index);

   if (strcasecmp(WindowName, TAB_MESSAGES) == 0) {
      messages_tab = true;
   }

   tab=new FXTabItem(tabbook, WindowName, NULL);
   tab->setTextColor(FXRGB(0,0,255));
   tab->setTabOrientation(TAB_BOTTOM);
   scrollframe=new FXVerticalFrame(tabbook,FRAME_LINE, 0,0,0,0, 0,0,0,0,0,0);
   boxframe=new FXHorizontalFrame(scrollframe,FRAME_LINE|LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_TOP, 0,0,0,0, 0,0,0,0,0);
   splitter = new FXSplitter(boxframe, LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y|SPLITTER_REVERSED|SPLITTER_TRACKING);
   horfr1 = new FXHorizontalFrame(splitter, FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL|LAYOUT_TOP, 0,0,0,0, 0,0,0,0); // for text
   horfr2 = new FXHorizontalFrame(splitter, FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL_Y|LAYOUT_TOP, 0,0,150,0, 0,0,0,0); // for nick list and channel list

   if (!messages_tab) {
      splitter = new FXSplitter(horfr2, LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y|SPLITTER_VERTICAL|SPLITTER_REVERSED|SPLITTER_TRACKING);
   }

   simplescroll=new FXText(horfr1, this, TabBookWindow::ID_MOUSERELEASE, TEXT_WORDWRAP|LAYOUT_SIDE_TOP|LAYOUT_FILL, 0,0,0,0);
   if (!messages_tab) {
      simplelist = new FXList(splitter, this, TabBookWindow::ID_SELECTPMNICK, LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y|LIST_SINGLESELECT|HSCROLLING_OFF, 0, 0, 0, 0);
      if (ChannelListUI == NULL) {
         ChannelList = new FXList(splitter, this, TabBookWindow::ID_SELECTCHANNELTABTODISPLAY, LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|LAYOUT_FILL_Y|LIST_SINGLESELECT|HSCROLLING_OFF|VSCROLLING_OFF, 0, 0, 0, 0);
      }
      else {
         ChannelList = NULL;
      }
      Windows[index].ChannelListUIFather = splitter;
   }
   else {
      simplelist = new FXList(horfr2, this, TabBookWindow::ID_SELECTPMNICK, LAYOUT_SIDE_TOP|LAYOUT_FILL, 0, 0, 0, 0);
   }

   boxframe=new FXHorizontalFrame(scrollframe,FRAME_LINE|LAYOUT_FILL_X|LAYOUT_TOP, 0,0,0,0, 0,0,0,0);
   simplescroll->setEditable(FALSE);
   simplescroll->setStyled();
   simplescroll->setFont(font);
   simplescroll->setHiliteStyles(styles);
   simplelist->setFont(font);
   if (ChannelList) {
      ChannelList->setFont(font);
   }

   simplescroll->setBackColor(FXRGB(0, 0, 0));
   simplelist->setBackColor(FXRGB(0, 0, 0));
   if (ChannelList) {
      ChannelList->setBackColor(FXRGB(150, 150, 150));
   }

   simplescroll->setActiveBackColor(FXRGB(0, 0, 0));
   simplescroll->setTextColor(FXRGB(255, 255, 255));
   simplelist->setTextColor(FXRGB(255, 255, 255));
   if (ChannelList) {
      //ChannelList->setTextColor(FXRGB(255, 255, 255));
      ChannelList->setTextColor(FXRGB(0, 0, 0));

      // Also call create for App->channeltextnew_icon.
      App->channeltextnew_icon->create();
   }

   // Removed - and [ and ] from what is present originally.
   simplescroll->setDelimiters("~.,/\\`'!@#$%^&*()=+|\":;<>?");

   strcpy(Windows[index].Name, WindowName);
   Windows[index].TabUI = tab;
   Windows[index].ScrollUI = simplescroll;
   Windows[index].SearchUI = NULL;
   Windows[index].NickListUI = simplelist;
   if (ChannelList) {
      ChannelListUI = ChannelList;
   }
   // By default hide the windows, unless Name is CHANNEL_CHAT or TAB_MESSAGES
   // or TAB_DCC_CHAT
   if ( strcasecmp(WindowName, CHANNEL_CHAT) && (!messages_tab) ) {
      tab->hide();
   }
   Windows[index].BottomFrame = boxframe;
   return(index);
}

// Make some windows
// Basically 5: server Window, IM Channel Window, MC Channel Window.
//  Messages Window, Search Window
// Constructor
//TabBookWindow::TabBookWindow(FXApp *a, XChange *XG):FXMainWindow(a,CLIENT_NAME _FULL" " DATE_STRING " " VERSION_STRING "- " NETWORK_UI_NAME,NULL,NULL,DECOR_ALL,0,0,1000,400){
TabBookWindow::TabBookWindow(MasalaMateAppUI *a, XChange *XG):FXMainWindow(a,CLIENT_NAME_FULL " " DATE_STRING " " VERSION_STRING "- " NETWORK_UI_NAME,NULL,NULL,DECOR_ALL,0,0,1000,400){

FXHorizontalFrame *contents;
XGlobal = XG;
int i;
int windex;
IRCChannelList CL;
FXFontDesc fontdesc;
char tmpStr[512];

   TRACE();

   // Get Nick and Nick Pass from XGlobal;
   XGlobal->getIRC_Nick(tmpStr);
   Nick = new char[strlen(tmpStr) + 1];
   strcpy(Nick, tmpStr);
   XGlobal->getIRC_Password(tmpStr);
   NickPass = new char[strlen(tmpStr) + 1];
   strcpy(NickPass, tmpStr);
   XGlobal->resetIRC_Nick_Changed();

   App = a;
   AllowUpdates = true;
   FocusIndex = 0;
   OpenSendsButton = false;
   OpenQueuesButton = false;
   NonFirewalledButton = false;
   IgnorePartialsButton = false;
   ScrollEnable = true;
   UpgradeServerEnable = true;

   FileSearchTitle = new char[512];
   FileSearchTitle[0] = '\0';
   FileSearchFD    = NULL;
   FileSearchTotalFD = 0;

   SaveRestoreSelectedItems = NULL;
   SaveRestoreSelectedItemCount = 0;
   SaveRestoreExpandedItems = NULL;
   SaveRestoreExpandedItemCount = 0;
   SaveRestoreCurrentItem = 0;

   setIcon(a->wm_icon);

// Initialise the Windows[] to NULL.
   memset(Windows, 0, sizeof(UI_Items) * UI_MAXTABS);

   ChannelListUI = NULL;

// Let us initialize the style structure styles;
   styles = new FXHiliteStyle[32];
   for (i = 0; i < 16; i++) {
// Text with a different Foreground color.
       styles[i].normalForeColor = FXRGB(ColorMapping[i][0], ColorMapping[i][1], ColorMapping[i][2]);
       styles[i].normalBackColor = FXRGB(0, 0, 0);
       styles[i].selectForeColor = FXRGB(0, 0, 0);
       styles[i].selectBackColor = FXRGB(255, 255, 255);
       styles[i].hiliteForeColor = FXRGB(0, 0, 0);
       styles[i].hiliteBackColor = FXRGB(255, 255, 255);
       styles[i].activeBackColor = FXRGB(0, 0, 0);
       styles[i].style = 0x00;

// Text with a different background color.
       styles[i + 16].normalForeColor = FXRGB(0, 0, 0);
       styles[i + 16].normalBackColor = FXRGB(ColorMapping[i][0], ColorMapping[i][1], ColorMapping[i][2]);
       styles[i + 16].selectForeColor = FXRGB(0, 0, 0);
       styles[i + 16].selectBackColor = FXRGB(255, 255, 255);
       styles[i + 16].hiliteForeColor = FXRGB(0, 0, 0);
       styles[i + 16].hiliteBackColor = FXRGB(255, 255, 255);
       styles[i + 16].activeBackColor = FXRGB(0, 0, 0);
       styles[i + 16].style = 0x00;
   }

// Let us initialise the font.
   getApp()->getNormalFont()->getFontDesc(fontdesc);

//  Change some of the attributes. Here we change the font weight, making it bolder.
   XGlobal->lock();
   strcpy(fontdesc.face, XGlobal->FontFace);
   fontdesc.size = XGlobal->FontSize;
   XGlobal->unlock();

#ifdef __MINGW32__
   fontdesc.weight = FX::FONTWEIGHT_NORMAL;
#else
   fontdesc.weight = FX::FONTWEIGHT_BOLD;
#endif
   fontdesc.setwidth = FX::FONTSETWIDTH_DONTCARE;

   // Based on the font description we create a new font object.
   font = new FXFont(getApp(), fontdesc);

   // Tooltip
   // new FXToolTip(getApp());

   // The Top Line consists of Menu Bar, Tool Bar, Clock
   // So we create a Horizontal Frame to hold these 3 things.
   FXHorizontalFrame *topline = new FXHorizontalFrame(this,FRAME_NONE|LAYOUT_SIDE_TOP|LAYOUT_FILL_X, 0,0,0,0 ,0,0,0,0,0,0);

   // Menubar
   menubar=new FXMenuBar(topline, LAYOUT_SIDE_TOP);

   // ToolBar
   toolbar = new FXToolBar(topline, LAYOUT_SIDE_TOP | LAYOUT_FILL_X);

   // Separator
   new FXHorizontalSeparator(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|SEPARATOR_GROOVE);

   // Contents
   contents=new FXHorizontalFrame(this,FRAME_NONE|LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y|PACK_UNIFORM_WIDTH, 0,0,0,0 ,0,0,0,0,0,0);

   // Switcher
   tabbook=new FXTabBook(contents,this,ID_PANEL,PACK_UNIFORM_WIDTH|PACK_UNIFORM_HEIGHT|LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_RIGHT, 0,0,0,0 ,0,0,0,0);
   tabbook->setTabStyle(TABBOOK_BOTTOMTABS);

   // First Tab: Server Window.
   windex = makeNewServerTabItem("Server");
   Windows[windex].InputUI = makeNewBottomFrameInput(Windows[windex].BottomFrame);
   // If it has a Input Text Field, then it should have a History
   Windows[windex].History = new HistoryLines;

   while (true) {
      if (XGlobal->isIRC_CL_Changed(CL)) {
         CL = XGlobal->getIRC_CL();
         XGlobal->resetIRC_CL_Changed();
         break;
      }
      sleep(1);
   }
   COUT(CL.printDebug());

   // Second TAB to always display a channel as selected from ChannelListUI.
   int totalc = CL.getChannelCount();

   windex = makeNewChannelTabItem(CHANNEL_CHAT);
   Windows[windex].InputUI = makeNewBottomFrameInput(Windows[windex].BottomFrame);
   Windows[windex].History = new HistoryLines;

   windex = makeNewChannelTabItem(CHANNEL_MAIN);
   Windows[windex].InputUI = makeNewBottomFrameInput(Windows[windex].BottomFrame);
   Windows[windex].History = new HistoryLines;

   windex = makeNewChannelTabItem(CHANNEL_MM);
   Windows[windex].InputUI = makeNewBottomFrameInput(Windows[windex].BottomFrame);
   Windows[windex].History = new HistoryLines;

   ChannelListUI->appendItem(CHANNEL_CHAT, App->channeltextcurrent_icon);
   ChannelListUI->appendItem(CHANNEL_MAIN, App->channeltextcurrent_icon);
#ifdef COUT_OUTPUT
   ChannelListUI->appendItem(CHANNEL_MM, App->channeltextcurrent_icon);
#endif

   ChannelListUI->selectItem(0);

   for (i = 1; i <= totalc; i++) {
      if (strcasecmp(CL.getChannel(i), CHANNEL_MAIN) &&
          strcasecmp(CL.getChannel(i), CHANNEL_CHAT) &&
          strcasecmp(CL.getChannel(i), CHANNEL_MM) ) {
 
         windex = makeNewChannelTabItem(CL.getChannel(i));
         ChannelListUI->appendItem(CL.getChannel(i), App->channeltextcurrent_icon);
         Windows[windex].InputUI = makeNewBottomFrameInput(Windows[windex].BottomFrame);
         Windows[windex].History = new HistoryLines;
      }
   }

   // Create the other UI_MAX_OTHER_CHANNEL Channels with name 
   // UI_FREE_CHANNEL_NAME
   // These will be all hidden.
   for (i = 0; i < UI_MAX_OTHER_CHANNEL + 3 - totalc; i++) {   
      windex = makeNewChannelTabItem(UI_FREE_CHANNEL_NAME);
      Windows[windex].InputUI = makeNewBottomFrameInput(Windows[windex].BottomFrame);
      Windows[windex].History = new HistoryLines;
   }

   // DCC Chat TAB.
   windex = makeNewServerTabItem(TAB_DCC_CHAT); // as we need a bottom frame.
   Windows[windex].LabelUI = makeNewBottomFrameLabel(Windows[windex].BottomFrame, " **********");
   Windows[windex].InputUI = makeNewBottomFrameInput(Windows[windex].BottomFrame);
   Windows[windex].History = new HistoryLines;

   // Fourth Tab: Messages
   windex = makeNewChannelTabItem(TAB_MESSAGES);
   Windows[windex].LabelUI = makeNewBottomFrameLabel(Windows[windex].BottomFrame, " NickServ");
   Windows[windex].NickListUI->appendItem(" NickServ");
   Windows[windex].InputUI = makeNewBottomFrameInput(Windows[windex].BottomFrame);
   Windows[windex].History = new HistoryLines;

   // Fifth Tab: File Search
   makeNewSearchTabItem(TAB_FILESEARCH);
   windex = getWindowIndex(TAB_FILESEARCH); // It shouldnt be -1
   Windows[windex].SearchUI->appendHeader("File Name", NULL, 240);
   Windows[windex].SearchUI->appendHeader("File Size", NULL, 85);
   Windows[windex].SearchUI->appendHeader("Directory Name", NULL, 190);
   Windows[windex].SearchUI->appendHeader("Nick", NULL, 130);
   Windows[windex].SearchUI->appendHeader("Sends", NULL, 65);
   Windows[windex].SearchUI->appendHeader("Queues", NULL, 65);
   Windows[windex].SearchUI->appendHeader("FireWalled", NULL, 75);
   Windows[windex].SearchUI->appendHeader("Client", NULL, 75);
   Windows[windex].SearchUI->appendHeader(" #", NULL, 50);

   // The Clickable Search Button at the start.
   makeNewBottomFrameButton(Windows[windex].BottomFrame, "\tClick to Search", ID_BUTTON_SEARCH_FILE);

   makeNewBottomFrameLabel(Windows[windex].BottomFrame, "File Name contains ");
   Windows[windex].InputUI = makeNewBottomFrameInput(Windows[windex].BottomFrame);
   Windows[windex].History = new HistoryLines;

   makeNewBottomFrameLabel(Windows[windex].BottomFrame, " File Type ");
   FileSearchFileTypeComboBox = makeNewBottomFrameComboBox(Windows[windex].BottomFrame, 6, FILESEARCH_COMBOBOX_ANY, FILESEARCH_COMBOBOX_AVI, FILESEARCH_COMBOBOX_MPG, FILESEARCH_COMBOBOX_WMV, FILESEARCH_COMBOBOX_RM, FILESEARCH_COMBOBOX_3GP, FILESEARCH_COMBOBOX_IMAGES);

   makeNewBottomFrameLabel(Windows[windex].BottomFrame, " Directory Name contains ");
   FileSearchDirNameInputUI = makeNewBottomFrameInput(Windows[windex].BottomFrame);

   makeNewBottomFrameLabel(Windows[windex].BottomFrame, " Nick Name contains ");
   FileSearchNickNameInputUI = makeNewBottomFrameInput(Windows[windex].BottomFrame);

   // Below is to be populated in FileSearch2ndBottomFrame
   makeNewBottomFrameLabel(FileSearch2ndBottomFrame, "Size atleast ");
   FileSearchFileSizeGTInputUI = makeNewBottomFrameInput(FileSearch2ndBottomFrame);
   FileSearchFileSizeGTComboBox = makeNewBottomFrameComboBox(FileSearch2ndBottomFrame, 2, FILESEARCH_COMBOBOX_MBYTES, FILESEARCH_COMBOBOX_KBYTES, FILESEARCH_COMBOBOX_BYTES, FILESEARCH_COMBOBOX_GBYTES);

   makeNewBottomFrameLabel(FileSearch2ndBottomFrame, " Size atmost ");
   FileSearchFileSizeLTInputUI = makeNewBottomFrameInput(FileSearch2ndBottomFrame);
   FileSearchFileSizeLTComboBox = makeNewBottomFrameComboBox(FileSearch2ndBottomFrame, 2, FILESEARCH_COMBOBOX_MBYTES, FILESEARCH_COMBOBOX_KBYTES, FILESEARCH_COMBOBOX_BYTES, FILESEARCH_COMBOBOX_GBYTES);

   makeNewBottomFrameLabel(FileSearch2ndBottomFrame, " Sends ");
   // Menu to choose OPEN|ANY
   FileSearchSendsComboBox = makeNewBottomFrameComboBox(FileSearch2ndBottomFrame, 4, FILESEARCH_COMBOBOX_ANY, FILESEARCH_COMBOBOX_OPEN);

   makeNewBottomFrameLabel(FileSearch2ndBottomFrame, " Queues ");
   // Menu to choose OPEN|ANY
   FileSearchQueuesComboBox = makeNewBottomFrameComboBox(FileSearch2ndBottomFrame, 4, FILESEARCH_COMBOBOX_ANY, FILESEARCH_COMBOBOX_OPEN);

   makeNewBottomFrameLabel(FileSearch2ndBottomFrame, " Firewalled ");
   // Menu to choose NO|YES|ANY
   FileSearchFirewalledComboBox = makeNewBottomFrameComboBox(FileSearch2ndBottomFrame, 3, FILESEARCH_COMBOBOX_ANY, FILESEARCH_COMBOBOX_NO, FILESEARCH_COMBOBOX_YES);

   makeNewBottomFrameLabel(FileSearch2ndBottomFrame, " List Partials ");
   // Menu to choose NO|YES|ANY
   FileSearchPartialComboBox = makeNewBottomFrameComboBox(FileSearch2ndBottomFrame, 3, FILESEARCH_COMBOBOX_ANY, FILESEARCH_COMBOBOX_NO, FILESEARCH_COMBOBOX_YES);

   makeNewBottomFrameLabel(FileSearch2ndBottomFrame, " Client ");
   // Menu to choose MM|SYSRESET|IROFFER|ANY
   FileSearchClientComboBox = makeNewBottomFrameComboBox(FileSearch2ndBottomFrame, 10, FILESEARCH_COMBOBOX_ANY, FILESEARCH_COMBOBOX_MM, FILESEARCH_COMBOBOX_SYSRESET, FILESEARCH_COMBOBOX_IROFFER);

   // Get the Header of TAB_FILESEARCH and set it so that it sends messages
   // when clicked.
   FXHeader *Header = Windows[windex].SearchUI->getHeader();
   Header->setTarget(this);
   Header->setSelector(FXSEL(SEL_COMMAND, ID_FILESEARCH_HEADER_CLICK));


   // Sixth Tab: Waiting
   makeNewSearchTabItem("Waiting");
   i = getWindowIndex("Waiting"); // It shouldnt be -1
   Windows[i].SearchUI->appendHeader("File Name", NULL, 250);
   Windows[i].SearchUI->appendHeader("File Size", NULL, 100);
   Windows[i].SearchUI->appendHeader("Nick", NULL, 150);
   Windows[i].SearchUI->appendHeader("Queue", NULL, 75);
   Windows[i].SearchUI->appendHeader("ReSend", NULL, 75);
   Windows[i].SearchUI->appendHeader("Rate", NULL, 100);
   Windows[i].SearchUI->appendHeader("Percent", NULL, 75);
   Windows[i].SearchUI->appendHeader("ETA", NULL, 150);

   // Seventh Tab: Downloads
   makeNewSearchTabItem("Downloads");
   i = getWindowIndex("Downloads"); // It shouldnt be -1
   Windows[i].SearchUI->appendHeader("File Name", NULL, 240);
   Windows[i].SearchUI->appendHeader("File Size", NULL, 85);
   Windows[i].SearchUI->appendHeader("Cur Size", NULL, 85);
   Windows[i].SearchUI->appendHeader("Progress", NULL, 100);
   Windows[i].SearchUI->appendHeader("Rate", NULL, 125);
   Windows[i].SearchUI->appendHeader("Time Left", NULL, 150);
   Windows[i].SearchUI->appendHeader("Nick", NULL, 120);
   Windows[i].SearchUI->appendHeader("#", NULL, 50);

   // Eight Tab: File Server 
   makeNewSearchTabItem("File Server");
   i = getWindowIndex("File Server"); // It shouldnt be -1
   Windows[i].SearchUI->appendHeader("File Name", NULL, 250);
   Windows[i].SearchUI->appendHeader("File Size", NULL, 90);
   Windows[i].SearchUI->appendHeader("Nick", NULL, 125);
   Windows[i].SearchUI->appendHeader("Rate", NULL, 125);
   Windows[i].SearchUI->appendHeader("Progress", NULL, 80);
   Windows[i].SearchUI->appendHeader("Time Left", NULL, 150);
   Windows[i].SearchUI->appendHeader("Queue", NULL, 90);
   Windows[i].SearchUI->appendHeader("Type", NULL, 80);

   // Ninth Tab: Swarm
   makeNewSearchTabItem(TAB_SWARM);
   i = getWindowIndex(TAB_SWARM); // It shouldnt be -1
   Windows[i].SearchUI->appendHeader("File Name / Nick", NULL, 250);
   Windows[i].SearchUI->appendHeader("File Size", NULL, 90);
   Windows[i].SearchUI->appendHeader("Rate UP", NULL, 125);
   Windows[i].SearchUI->appendHeader("Rate Down", NULL, 125);
   Windows[i].SearchUI->appendHeader("Progress / State", NULL, 150);
   Windows[i].SearchUI->appendHeader("Max / Offset", NULL, 90);

   // Fill up the menu bar.
   // File Menu
   filemenu=new FXMenuPane(this);
#if 0
   new FXMenuCommand(filemenu,"&Options",NULL,this,TabBookWindow::ID_FILEOPTIONS);
#endif
   // Connection Menu
   connectionmenu = new FXMenuPane(this);
   FXMenuPane *typemenu = new FXMenuPane(this);

   // Initialise the DataTarget and variable associated with this Radio
   DataTargetConnectionType = new FXDataTarget(ConnectionType, this, TabBookWindow::ID_CONNECTION_TYPE);
   new FXMenuRadio(typemenu, "&Direct", DataTargetConnectionType, FXDataTarget::ID_OPTION + 1);
   new FXMenuRadio(typemenu, "&Proxy", DataTargetConnectionType, FXDataTarget::ID_OPTION + 2);;
   new FXMenuRadio(typemenu, "&BNC", DataTargetConnectionType, FXDataTarget::ID_OPTION + 3);
   new FXMenuRadio(typemenu, "Socks &4", DataTargetConnectionType, FXDataTarget::ID_OPTION + 4);
   new FXMenuRadio(typemenu, "Socks &5", DataTargetConnectionType, FXDataTarget::ID_OPTION + 5);

   new FXMenuCascade(connectionmenu, "&Type", NULL, typemenu);

   // Put a Menu Caption below Type, which reflects current Type.
   // This is used later on depending on what Type is selected.
   ConnectionCaption = new FXMenuCaption(connectionmenu, "Type:   Direct");
   ConnectionCaption->setTextColor(FXRGB(255, 0, 0));

   // The Connection Type MenuCommands.
   MenuConnectionTypeHost = new FXMenuCommand(connectionmenu, "", NULL, this, TabBookWindow::ID_CONNECTION_HOST);
   MenuConnectionTypePort = new FXMenuCommand(connectionmenu, "", NULL, this, TabBookWindow::ID_CONNECTION_PORT);
   MenuConnectionTypeUser = new FXMenuCommand(connectionmenu, "", NULL, this, TabBookWindow::ID_CONNECTION_USER);
   MenuConnectionTypePass = new FXMenuCommand(connectionmenu, "", NULL, this, TabBookWindow::ID_CONNECTION_PASS);
   MenuConnectionTypeVHost = new FXMenuCommand(connectionmenu, "", NULL, this, TabBookWindow::ID_CONNECTION_VHOST);

   // Inititalise the state of UI in sync with XGlobal.
   // Set ConnectionType as its used by DataTarget.
   // Then call onConnectionTypeChanged(NULL, 0, NULL) to set the UI in sync.
   CM = XGlobal->getIRC_CM();
   XGlobal->resetIRC_CM_Changed();
   ConnectionType = CM.howto(); // They are 1:1
   ConnectionHost = CM.getHost();
   ConnectionPort = CM.getPort();
   ConnectionUser = CM.getUser();
   ConnectionPass = CM.getPassword();
   ConnectionVHost = CM.getVhost();

   onConnectionTypeChanged(NULL, 0, NULL);

   //new FXMenuTitle(menubar, "&Connection", NULL, connectionmenu);
   new FXMenuCascade(filemenu, "&Connection", NULL, connectionmenu);

   // The ReConnect Menu Command.
   new FXMenuCommand(filemenu, "Re&Connect IRC", NULL, this, TabBookWindow::ID_CONNECTION_RECONNECT);

   // The Upgrade Server Check Box
   UpgradeServerMenuCheck = new FXMenuCheck(filemenu, "&Upgrade Server", this, TabBookWindow::ID_UPGRADE_SERVER);
   UpgradeServerMenuCheck->setCheck(FALSE);
   UpgradeServerMenuCheck->hide();

   // The Quit Menu Command.
   new FXMenuCommand(filemenu,"&Quit\tCtl-Q", NULL, this,TabBookWindow::ID_QUIT_VERIFY);
   new FXMenuTitle(menubar,"&File",NULL,filemenu);

   // Nick Menu
   nickmenu = new FXMenuPane(this);
   new FXMenuCommand(nickmenu, "Set Nick&Name", NULL, this, TabBookWindow::ID_NICKNAME_SET);
   new FXMenuCommand(nickmenu, "Set Nick&Pass", NULL, this, TabBookWindow::ID_NICKPASS_SET);
   new FXMenuTitle(menubar, "&Nick", NULL, nickmenu);

   // Dir Menu
   dirmenu = new FXMenuPane(this);
   new FXMenuCommand(dirmenu,"&Update Server", NULL,this,TabBookWindow::ID_UPDATESERVINGLIST);
   new FXMenuCommand(dirmenu,"Open &Partial Dir", NULL,this,TabBookWindow::ID_PARTIALDIR);
   new FXMenuCommand(dirmenu,"Set P&artial Dir", NULL,this,TabBookWindow::ID_SETPARTIALDIR);
   // Below is the Open Serving Dir Menu Pane.
   FXMenuPane *openservingdirmenupane = new FXMenuPane(this);
   for (i = 0; i < FSERV_MAX_SERVING_DIRECTORIES; i++) {
      sprintf(tmpStr, "Serving Dir &%d", i);
      MenuOpenServingDir[i] = new FXMenuCommand(openservingdirmenupane, tmpStr, NULL, this, TabBookWindow::ID_OPENSERVINGDIR + i);
   }
   new FXMenuCascade(dirmenu, "&Open Serving Dir", NULL, openservingdirmenupane);
   // Below is the Set Serving Dir Menu Pane.
   FXMenuPane *setservingdirmenupane = new FXMenuPane(this);
   for (i = 0; i < FSERV_MAX_SERVING_DIRECTORIES; i++) {
      sprintf(tmpStr, "Serving Dir &%d", i);
      MenuSetServingDir[i] = new FXMenuCommand(setservingdirmenupane, tmpStr, NULL, this, TabBookWindow::ID_SETSERVINGDIR + i);
   }
   new FXMenuCascade(dirmenu, "&Set Serving Dir", NULL, setservingdirmenupane);
   // Below is the UnSet Serving Dir Menu Pane.
   FXMenuPane *unsetservingdirmenupane = new FXMenuPane(this);
   for (i = 0; i < FSERV_MAX_SERVING_DIRECTORIES; i++) {
      sprintf(tmpStr, "Serving Dir &%d", i);
      MenuUnSetServingDir[i] = new FXMenuCommand(unsetservingdirmenupane, tmpStr, NULL, this, TabBookWindow::ID_UNSETSERVINGDIR + i);
   }
   // Now sync the DIR menu with what XGlobal has.
   syncDirMenuWithServingDir();

   new FXMenuCascade(dirmenu, "&UnSet Serving Dir", NULL, unsetservingdirmenupane);

   new FXMenuTitle(menubar,"&Dir",NULL,dirmenu);

   // Font Menu
   fontmenu = new FXMenuPane(this);
   new FXMenuCommand(fontmenu,"&Change Fonts",NULL,this,TabBookWindow::ID_FONT);
   new FXMenuTitle(menubar,"F&ont",NULL,fontmenu);

   // Tools Menu
   toolsmenu = new FXMenuPane(this);
   new FXMenuCommand(toolsmenu, "Rollback &Truncate File", NULL, this, TabBookWindow::ID_TOOLS_ROLLBACK_TRUNCATE);
   new FXMenuCommand(toolsmenu, "Check And &Upgrade", NULL, this, TabBookWindow::ID_TOOLS_CHECK_UPGRADE);
   new FXMenuCommand(toolsmenu, "&Request Unban", NULL, this, TabBookWindow::ID_TOOLS_REQUEST_UNBAN);
   new FXMenuCommand(toolsmenu, "&Set Tray Password", NULL, this, TabBookWindow::ID_TOOLS_SET_TRAY_PASS);
   new FXMenuTitle(menubar, "&Tools",NULL,toolsmenu);

   // Help Menu
   helpmenu=new FXMenuPane(this);
   new FXMenuCommand(helpmenu,"&Help",NULL,this,TabBookWindow::ID_HELP);
   new FXMenuCommand(helpmenu,"&Demos",NULL,this,TabBookWindow::ID_HELPDEMOS);
   new FXMenuCommand(helpmenu,"&About",NULL,this,TabBookWindow::ID_HELPABOUT);
   new FXMenuTitle(menubar,"&Help",NULL,helpmenu);

   // Fill up the toolbar.
   // We fill the toolbar from the right, with LAYOUT_RIGHT
   new FXToolBarGrip(toolbar,NULL,0,TOOLBARGRIP_DOUBLE|LAYOUT_RIGHT);

   // Now is the Progress Bar related to TAB_FILESEARCH, but can be used
   // by any future thing which can indicate a progress.
   ProgressBar = new FXProgressBar(toolbar, NULL, 0, FRAME_SUNKEN|FRAME_THICK|PROGRESSBAR_PERCENTAGE|LAYOUT_RIGHT|PROGRESSBAR_VERTICAL|LAYOUT_FILL_Y);
   ProgressBar->setBarSize(40);
   ProgressBar->setBarColor(FXRGB(0,200,0));
   ProgressBar->setTextColor(FXRGB(200,0,0));
   // Show 100 % => all is well.
   ProgressBar->setTotal(1);
   ProgressBar->setProgress(1);

   // We fill the toolbar from the right, with LAYOUT_RIGHT
   new FXToolBarGrip(toolbar,NULL,0,TOOLBARGRIP_DOUBLE|LAYOUT_RIGHT);

   // Now is the Button with Right arrow symbol for searching
   FXArrowButton *AB = new FXArrowButton(toolbar, this, TabBookWindow::ID_FIND_TEXT_WRAP_RIGHT, ARROW_RIGHT|ARROW_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_RIGHT);
   AB->setTipText("Find Text");
   AB->setArrowSize(17);
   AB->setArrowColor(FXRGB(255,0,0));

   // Now is the Text Search - FXTextField.
   textsearch = new FXTextField(toolbar,10,NULL,0,LAYOUT_CENTER_Y|LAYOUT_RIGHT);

   // Now is the Button with Left arrow symbol for searching.
   AB = new FXArrowButton(toolbar, this, TabBookWindow::ID_FIND_TEXT_WRAP_LEFT, ARROW_LEFT|ARROW_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_RIGHT);
   AB->setTipText("Find Text");
   AB->setArrowSize(17);
   AB->setArrowColor(FXRGB(255,0,0));
  
   // A Spacer
   new FXToolBarGrip(toolbar,NULL,0,TOOLBARGRIP_DOUBLE|LAYOUT_RIGHT);

   // Help
   new FXButton(toolbar, "\tHelp.",a->help_icon,this,ID_HELP, ICON_ABOVE_TEXT|BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_RIGHT);

   // A Spacer
   new FXToolBarGrip(toolbar,NULL,0,TOOLBARGRIP_SINGLE|LAYOUT_RIGHT);

   // The radio button for the firewall state.
   firewall = new FXRadioButton(toolbar, "\tFireWall State.", NULL,0,ICON_BEFORE_TEXT|LAYOUT_SIDE_TOP|LAYOUT_RIGHT);
   firewall->setRadioColor(FXRGB(255,0,0));
   firewall->setDiskColor(FXRGB(255,0,0));

   // A Spacer
   new FXToolBarGrip(toolbar,NULL,0,TOOLBARGRIP_SINGLE|LAYOUT_RIGHT);

   // Port Check Me
   new FXButton(toolbar, "\tPort Check Me.",a->portcheckme_icon,this,ID_TOOLBAR_PORTCHECKME, ICON_ABOVE_TEXT|BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_RIGHT);

   // Port Check
   new FXButton(toolbar, "\tPort Check.",a->portcheck_icon,this,ID_TOOLBAR_PORTCHECK, ICON_ABOVE_TEXT|BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_RIGHT);

   // A Spacer
   new FXToolBarGrip(toolbar,NULL,0,TOOLBARGRIP_SINGLE|LAYOUT_RIGHT);

   // Partial DIR
   new FXButton(toolbar, "\tPartial Dir.",a->partialdir_icon,this, ID_PARTIALDIR, ICON_ABOVE_TEXT|BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_RIGHT);

   // Serving DIR
   new FXButton(toolbar, "\tServing Dir.",a->servingdir_icon,this, ID_OPENSERVINGDIR, ICON_ABOVE_TEXT|BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_RIGHT);

   // A Spacer
   new FXToolBarGrip(toolbar,NULL,0,TOOLBARGRIP_SINGLE|LAYOUT_RIGHT);

   // DCC Send
   new FXButton(toolbar, "\tDCC Send.",a->dccsend_icon,this,ID_TOOLBAR_DCCSEND, ICON_ABOVE_TEXT|BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_RIGHT);

   // A Spacer
   new FXToolBarGrip(toolbar,NULL,0,TOOLBARGRIP_SINGLE|LAYOUT_RIGHT);

   // The check button for the scrolling state.
   FXCheckButton *CheckButton = new FXCheckButton(toolbar, "\tScroll", this, ID_SCROLLCHECK, CHECKBUTTON_NORMAL|LAYOUT_FILL_Y|LAYOUT_TOP|LAYOUT_RIGHT|ICON_BEFORE_TEXT);
   CheckButton->setCheck(TRUE);

   // Clear Screen.
   new FXButton(toolbar, "\tClear Screen.",a->clear_icon,this,ID_TOOLBAR_CLEAR, ICON_ABOVE_TEXT|BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_RIGHT);

   // Font
   new FXButton(toolbar, "\tChange Font.",a->font_icon,this,ID_FONT, ICON_ABOVE_TEXT|BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_RIGHT);

#if 0
   // File->Options
   new FXButton(toolbar, "\tOptions.",a->options_icon,this, ID_FILEOPTIONS, ICON_ABOVE_TEXT|BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_RIGHT);
#endif
   // END of populating the toolbar.

   // Clock as FXTextField. in the topline. was 25, now 22
   clock=new FXTextField(topline,22,NULL,0,JUSTIFY_LEFT|LAYOUT_CENTER_Y|TEXTFIELD_READONLY);
   clock->setBackColor(menubar->getBackColor());
   a->addTimeout(this, ID_CLOCKTIME, 1); // for the clock.

   // The Tray Button, in the topline -> last entry in topline.
   new FXButton(topline, "\tMinimize to Tray.", a->trayicon_icon, this, ID_BUTTON_TRAY, ICON_ABOVE_TEXT|BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_RIGHT);

   // A Spacer to be between time and tray icon.
   //new FXToolBarGrip(topline,NULL,0,TOOLBARGRIP_SINGLE|LAYOUT_RIGHT|LAYOUT_FILL_Y);

   // A Spacer
   new FXToolBarGrip(toolbar,NULL,0,TOOLBARGRIP_DOUBLE|LAYOUT_RIGHT);
   // End of filling up topline.

#if 0
   //  The File->Options Dialog Box
   dialog = new OptionsDialog(this, font);
#endif

   // The repetitive update for the Downloads/File Server Tab
   a->addTimeout(this, ID_PERIODIC_UI_UPDATE, PERIODIC_UI_UPDATE_TIMER);

   // The Popup menu on right click release in Channel and Messages Tab.
   PopUpMenu = new FXMenuPane(this);

   // Chumma added some nonsense text at end, to make the PopUp wider.
   new FXMenuCommand(PopUpMenu, "Port &Check Nick", a->portcheck_icon, this, ID_TOOLBAR_PORTCHECK);
   new FXMenuCommand(PopUpMenu, "&Port Check Me", a->portcheckme_icon, this, ID_TOOLBAR_PORTCHECKME);

   // A Spacer
   new FXMenuSeparator(PopUpMenu, 0);

   new FXMenuCommand(PopUpMenu, "&Search in Files", NULL, this, ID_POPUP_SEARCHTEXTINFILES);
   new FXMenuCommand(PopUpMenu, "&Update Files of Nick", NULL, this, ID_POPUP_GETLISTING);
   new FXMenuCommand(PopUpMenu, "&List Files of Nick", NULL, this, ID_POPUP_LISTFILES);

   // A Spacer
   new FXMenuSeparator(PopUpMenu, 0);

   new FXMenuCommand(PopUpMenu, "&WHOIS", NULL, this, ID_TOOLBAR_WHOIS);

   // A Spacer
   new FXMenuSeparator(PopUpMenu, 0);

   FXMenuPane *ctcpmenu = new FXMenuPane(this);
   new FXMenuCommand(ctcpmenu, "CTCP &Ping", NULL, this, ID_TOOLBAR_PING);
   new FXMenuCommand(ctcpmenu, "CTCP &Time", NULL, this, ID_TOOLBAR_TIME);
   new FXMenuCommand(ctcpmenu, "CTCP &Version", NULL, this, ID_TOOLBAR_VERSION);
   new FXMenuCommand(ctcpmenu, "CTCP &ClientInfo", NULL, this, ID_TOOLBAR_CLIENTINFO);
   new FXMenuCascade(PopUpMenu, "&CTCP", NULL, ctcpmenu); 

   FXMenuPane *dccmenu = new FXMenuPane(this);
   new FXMenuCommand(dccmenu, "DCC &Send", a->dccsend_icon, this, ID_TOOLBAR_DCCSEND);
   new FXMenuCommand(dccmenu, "DCC &Queue", NULL, this, ID_TOOLBAR_DCCQUEUE);
   new FXMenuCommand(dccmenu, "DCC &Chat", NULL, this, ID_TOOLBAR_DCCCHAT);
   new FXMenuCommand(dccmenu, "DCCAllow &Add", NULL, this, ID_TOOLBAR_DCCALLOWADD);
   new FXMenuCommand(dccmenu, "DCCAllow &Del", NULL, this, ID_TOOLBAR_DCCALLOWDEL);

   new FXMenuCascade(PopUpMenu, "&DCC", NULL, dccmenu);

   FXMenuPane *opmenu = new FXMenuPane(this);
   ToolBarOpButton = new FXMenuCommand(opmenu, "&OP", NULL, this, ID_TOOLBAR_OP);
   ToolBarDeOpButton = new FXMenuCommand(opmenu, "&DEOP", NULL, this, ID_TOOLBAR_DEOP);
   ToolBarVoiceButton = new FXMenuCommand(opmenu, "&VOICE", NULL, this, ID_TOOLBAR_VOICE);
   ToolBarDeVoiceButton = new FXMenuCommand(opmenu, "&DEVOICE", NULL, this, ID_TOOLBAR_DEVOICE);
   ToolBarKickButton = new FXMenuCommand(opmenu, "&KICK", NULL, this, ID_TOOLBAR_KICK);
   ToolBarBanButton = new FXMenuCommand(opmenu, "&BAN and KICK", NULL, this, ID_TOOLBAR_BAN);
   ToolBarOpCascade = new FXMenuCascade(PopUpMenu, "&OP Controls", NULL, opmenu);

   // The Popup menu on right click release in File Search TAB
   PopUpMenuFileSearch = new FXMenuPane(this);
   
   // Chumma added some nonsense text at end, to make the UI wider.
   new FXMenuCommand(PopUpMenuFileSearch, "&Download Selected Line", NULL, this, ID_FILESEARCH_DOWNLOAD);

   // A Spacer
   new FXMenuSeparator(PopUpMenuFileSearch, 0);

   new FXMenuCommand(PopUpMenuFileSearch, "&Search For File", NULL, this, ID_FILESEARCH_SEARCH_FILE);
   new FXMenuCommand(PopUpMenuFileSearch, "&List Files of Nick", NULL, this, ID_FILESEARCH_LIST_NICK);
   new FXMenuCommand(PopUpMenuFileSearch, "&Update Files of Nick", NULL, this, ID_FILESEARCH_UPDATE_NICK);

   // A Spacer
   new FXMenuSeparator(PopUpMenuFileSearch, 0);

   new FXMenuCommand(PopUpMenuFileSearch, "Check File &Integrity", NULL, this, ID_FILESEARCH_CHECK_INTEGRITY);

   // A Spacer
   new FXMenuSeparator(PopUpMenuFileSearch, 0);

   // The FedEx option.
   new FXMenuCommand(PopUpMenuFileSearch, "&FedEx File to Home", NULL, this, ID_FILESEARCH_FEDEX); 

   // The Popup menu on right click release in Downloads TAB
   PopUpMenuDownloads = new FXMenuPane(this);
  
   new FXMenuCommand(PopUpMenuDownloads, "&ReRequest Selected Partial", NULL, this, ID_DOWNLOADS_REREQUEST);
   new FXMenuCommand(PopUpMenuDownloads, "&Cancel Selected Download", NULL, this, ID_DOWNLOADS_CANCEL);

   // A Spacer
   new FXMenuSeparator(PopUpMenuDownloads, 0);

   new FXMenuCommand(PopUpMenuDownloads, "&Search For File", NULL, this, ID_DOWNLOADS_SEARCH_FILE);
   new FXMenuCommand(PopUpMenuDownloads, "&List Files of Nick", NULL, this, ID_DOWNLOADS_LIST_NICK);
   new FXMenuCommand(PopUpMenuDownloads, "&Update Files of Nick", NULL, this, ID_DOWNLOADS_UPDATE_NICK);

   // A Spacer
   new FXMenuSeparator(PopUpMenuDownloads, 0);

   new FXMenuCommand(PopUpMenuDownloads, "Clear &Selected PARTIAL/COMPLETEs", NULL, this, ID_DOWNLOADS_CLEAR_PARTIAL_COMPLETE);
   new FXMenuCommand(PopUpMenuDownloads, "Clear &ALL PARTIAL/COMPLETEs", NULL, this, ID_DOWNLOADS_CLEAR_ALL_PARTIAL_COMPLETE);

   // A Spacer
   new FXMenuSeparator(PopUpMenuDownloads, 0);

   new FXMenuCommand(PopUpMenuDownloads, "Check File &Integrity", NULL, this, ID_DOWNLOADS_CHECK_INTEGRITY);

   // The Popup menu on right click release in Waiting TAB
   PopUpMenuWaiting = new FXMenuPane(this);

   new FXMenuCommand(PopUpMenuWaiting, "Update &Queue Information", NULL, this, ID_WAITING_REREQUEST);
   new FXMenuCommand(PopUpMenuWaiting, "&Cancel Selected Queue", NULL, this, ID_WAITING_CANCEL);

   // A Spacer
   new FXMenuSeparator(PopUpMenuWaiting, 0);

   new FXMenuCommand(PopUpMenuWaiting, "&Search For File", NULL, this, ID_WAITING_SEARCH_FILE);
   new FXMenuCommand(PopUpMenuWaiting, "&List Files of Nick", NULL, this, ID_WAITING_LIST_NICK);
   new FXMenuCommand(PopUpMenuWaiting, "&Update Files of Nick", NULL, this, ID_WAITING_UPDATE_NICK);

   // A Spacer
   new FXMenuSeparator(PopUpMenuWaiting, 0);

   new FXMenuCommand(PopUpMenuWaiting, "Check File &Integrity", NULL, this, ID_WAITING_CHECK_INTEGRITY);

   // The Popup menu on right click release in File Server TAB
   PopUpMenuFileServer = new FXMenuPane(this);

   new FXMenuCommand(PopUpMenuFileServer, "&Send Selected Queue", NULL, this, ID_FILESERVER_FORCESEND);
   new FXMenuCommand(PopUpMenuFileServer, "&Abort Manual Send", NULL, this, ID_FILESERVER_ABORTSEND);

   // The Popup menu on right click release in Swarm TAB.
   PopUpMenuSwarm = new FXMenuPane(this);

   new FXMenuCommand(PopUpMenuSwarm, "&Quit Swarm", NULL, this, ID_SWARM_QUIT);

   // The Tray Object.
   Tray = new FXTray(App);
   Tray->create();
   Tray->setTipText("Keyboard - Double Click");
   App->trayicon_icon->create();
   Tray->setIcon(App->trayicon_icon);
   Tray->setTarget(this);
   Tray->enable();  // Remove this after FXTray.cpp is fixed.
   Tray->show();

   // Make Focus be on Chat Channel on startup.
   i = getWindowIndex(CHANNEL_CHAT);
   tabbook->setCurrent(i, TRUE);
}


// Clean up
// Destructor.
TabBookWindow::~TabBookWindow(){
int index;
char IRC_Line[128];
Helper H;

   TRACE();

   H.init(XGlobal);

   // Write out the sends ad queues info.
   H.writeFServeConfig();

   // Write out the Partials Information.
   H.writePartialConfig();

   // Write out the Waitings Information.
   H.writeWaitingConfig();

   getApp()->removeTimeout(this,ID_CLOCKTIME);

   // Let us not receive any more messages.
   setTarget(NULL);

   delete [] Nick;
   delete [] NickPass;

   for (index = 0; index < UI_MAXTABS; index++) {
      delete Windows[index].LabelUI;
      delete Windows[index].InputUI;
      delete Windows[index].NickListUI;
      delete Windows[index].ScrollUI;
      delete Windows[index].SearchUI;
      delete Windows[index].BottomFrame;
      delete Windows[index].TabUI;
      delete Windows[index].History;
   }
   delete FileSearch2ndBottomFrame;
   delete ChannelListUI;
   delete tabbook;
   delete helpmenu;
   delete filemenu;
   delete nickmenu;
   delete connectionmenu;
   delete fontmenu;
   delete clock;
   delete menubar;
   delete toolbar;
   delete PopUpMenu;
   delete PopUpMenuFileSearch;
   delete PopUpMenuDownloads;
   delete PopUpMenuWaiting;
   delete PopUpMenuFileServer;
   delete PopUpMenuSwarm;

   delete [] styles;
   delete font;
   delete DataTargetConnectionType;

   Tray->disable();
   Tray->hide();
   delete Tray;

   delete [] FileSearchTitle;
   XGlobal->FilesDB.freeFilesDetailList(FileSearchFD);

   delete [] SaveRestoreSelectedItems;
   delete [] SaveRestoreExpandedItems;

   // Lets call the Swarm Quit functions.
   for (int i = 0; i < SWARM_MAX_FILES; i++) {
      char *tempfname;
      const char *charp;

      if (XGlobal->Swarm[i].isBeingUsed()) {
         charp = XGlobal->Swarm[i].getSwarmFileName();
         tempfname = new char[strlen(charp) + 1];
         strcpy(tempfname, charp);
         if (XGlobal->Swarm[i].quitSwarm()) {
            // If it returns true, we need to move the File to serving folder.
            H.moveFile(tempfname, true);
         }
         delete [] tempfname;
      }
   }

// Lets tell the IRC Server that we are quitting.
   strcpy(IRC_Line, "QUIT :"CLIENT_NAME_FULL " " DATE_STRING " " VERSION_STRING);
// We directly write to server, as the ToServerThr() might not react
// fast enough, and would quit instead of writing the QUIT message.
   XGlobal->putLineIRC_Conn(IRC_Line);
   COUT(cout << "XGlobal->putLineIRC_Conn(IRC_Line): " << IRC_Line << endl;)

   XGlobal->setIRC_QUIT();
//   Now we queue "QUIT in all qs". We are the fromUI thread.

   // As of now all the connections check for isIRC_QUIT()
#if 0
   // We need to instruct all FServClientInProgress Connections to quit.
   XGlobal->FServClientInProgress.updateFilesDetailAllConnectionMessage(CONNECTION_MESSAGE_DISCONNECT);

   // We need to instruct all SendsInProgress Connections to quit.
   XGlobal->SendsInProgress.updateFilesDetailAllConnectionMessage(CONNECTION_MESSAGE_DISCONNECT);

   // We need to instruct all DwnldsInProgress Connections to quit.
   XGlobal->DwnldsInProgress.updateFilesDetailAllConnectionMessage(CONNECTION_MESSAGE_DISCONNECT);
#endif

   XGlobal->IRC_ToServer.putLine("QUIT");
   XGlobal->IRC_ToServerNow.putLine("QUIT");
   XGlobal->IRC_FromServer.putLine("QUIT");
   XGlobal->IRC_ToTrigger.putLine("QUIT");
   XGlobal->IRC_ToTriggerNow.putLine("QUIT");
   XGlobal->IRC_DCC.putLine("QUIT");
   XGlobal->IRC_ToUI.putLine("QUIT");
   XGlobal->UI_ToDwnldInit.putLine("QUIT");
   XGlobal->UI_ToUpnp.putLine("QUIT");
}

#if 0
// Process the ID_FILEOPTIONS coming from clicking File->Options.
long TabBookWindow::onFileOptions(FXObject *, FXSelector, void*) {
   TRACE();
   COUT(cout << "File->Options" << endl;)
   dialog->setOptions();
   dialog->show(PLACEMENT_OWNER);
   return(1);
}
#endif

// Process the ID_PARTIALDIR coming from clicking File->Partial Dir.
long TabBookWindow::onPartialDir(FXObject *, FXSelector, void*) {
char arglist[MAX_PATH];
   TRACE();

   COUT(cout << "File->Partial Dir" << endl;)
#ifdef __MINGW32__
STARTUPINFO si;
PROCESS_INFORMATION pi;

   ZeroMemory( &si, sizeof(si) );
   si.cb = sizeof(si);
   ZeroMemory( &pi, sizeof(pi) );

   XGlobal->lock();
   sprintf(arglist, "explorer %s", XGlobal->PartialDir);
   XGlobal->unlock();
   if (CreateProcess(NULL, arglist,
        NULL,       // Process handle not inheritable.
        NULL,       // Thread handle not inheritable.
        FALSE,      // Set handle inheritance to TRUE to access server_pid
        0,          // No creation flags.
        NULL,       // Use parent's environment block.
        NULL,       // Use parent's starting directory.
        &si,        // Pointer to STARTUPINFO structure.
        &pi )       // Pointer to PROCESS_INFORMATION structure.
        ) {
        // Handles in PROCESS_INFORMATION must be closed with CloseHandle 
        // when they are no longer needed.
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
   }
#else

   XGlobal->lock();
   sprintf(arglist, "nautilus %s", XGlobal->PartialDir);
   XGlobal->unlock(); 
   
   int retval = system(arglist);
   COUT(cout << "system(" << arglist << ") returned: " << retval << endl;)

   if (retval != 0) {
      // We failed to spawn nautilus. Lets try konqueror
      XGlobal->lock();
      sprintf(arglist, "konqueror %s &", XGlobal->PartialDir);
      XGlobal->unlock();
      retval = system(arglist);
      COUT(cout << "system(" << arglist << ") returned: " << retval << endl;)
   }
   
#endif

   return(1);
}

// Process the ID_UPDATESERVINGLIST coming from clicking
// Dir -> Update Server.
long TabBookWindow::onUpdateServingList(FXObject *, FXSelector, void*) {
Helper H;

   TRACE();

   COUT(cout << "Dir->Update Server" << endl;)

   H.init(XGlobal);
   H.generateMyFilesDB();

   H.generateMyPartialFilesDB();

   XGlobal->IRC_ToUI.putLine("Server 09FSERV: Refreshed the Serving Directory List.");

   return(1);
}


// Process the ID_SETPARTIALDIR coming from clicking Dir->Set Partial Dir.
long TabBookWindow::onSetPartialDir(FXObject *, FXSelector, void*) {

   TRACE();

   COUT(cout << "Dir->Set Partial Dir" << endl;)

   // We present user with the Dir Selection dialog.
   FXDirDialog Dir(this, "Set Partial Directory");
   XGlobal->lock();
   Dir.setDirectory(XGlobal->PartialDir);
   XGlobal->unlock();

   if (Dir.execute()) {
      char *dirstr;
      FXString fname;
      Helper H;
      bool PartialOK = true;

      H.init(XGlobal);
      fname = Dir.getDirectory();
      COUT(cout << "Set partial dir to: " << fname.text() << endl;)

      XGlobal->lock();

      // Check that the new directory is not the same as the Serving Dir.
      // Check if FileName is the same. (ignoring Dir)
      dirstr = new char[strlen(fname.text()) + 1];
      strcpy(dirstr, fname.text());
      for (int i = 0; i < FSERV_MAX_SERVING_DIRECTORIES; i++) {
         if (XGlobal->ServingDir[i] == NULL) break;
         if (strcasecmp(getFileName(dirstr), getFileName(XGlobal->ServingDir[i])) == 0) {
            XGlobal->unlock();

            FXMessageBox::error(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER, "Partial Dir", "Partial Directory cannot be same as the Serving Directory!");
            PartialOK = false;
            break;
         }
      }
      if (PartialOK) {
         delete [] XGlobal->PartialDir;
         XGlobal->PartialDir = new char[strlen(dirstr) + 1];
         strcpy(XGlobal->PartialDir, dirstr);
         XGlobal->unlock();

         H.writeFServeConfig();

         H.generateMyPartialFilesDB();
      }
      delete [] dirstr;
   }
   return(1);
}

// Process the ID_SETSERVINGDIR coming from clicking File->Set Serving Dir.
long TabBookWindow::onUnSetServingDir(FXObject *, FXSelector Sel, void*) {
int DirIndex;
char *DirStr;
char *message;

   TRACE();
   DirIndex = FXSELID(Sel) - ID_UNSETSERVINGDIR;
   COUT(cout << "File->UnSet Serving Dir Index: " << DirIndex << endl;)

   XGlobal->lock();
   if (XGlobal->ServingDir[DirIndex]) {
      DirStr = new char[strlen(XGlobal->ServingDir[DirIndex]) + 1];
      strcpy(DirStr, XGlobal->ServingDir[DirIndex]);
      XGlobal->unlock();
   }
   else {
      XGlobal->unlock();
      return(1);
   }

   message = new char[strlen(DirStr) + 128];
   sprintf(message, "Remove %s from the list of Serving Directories ?", DirStr);
   if (FXMessageBox::question(this,MBOX_YES_NO,"UnSet Serving Dir", message) == MBOX_CLICKED_YES) {
      Helper H;

      H.init(XGlobal);
      XGlobal->lock();
      delete [] XGlobal->ServingDir[DirIndex];
      // Shift all the ones after it to the deleted entry.
      for (int i = DirIndex + 1; i < FSERV_MAX_SERVING_DIRECTORIES; i++) {
         XGlobal->ServingDir[i - 1] = XGlobal->ServingDir[i];
      }
      XGlobal->ServingDir[FSERV_MAX_SERVING_DIRECTORIES - 1] = NULL;
      XGlobal->unlock();

      H.writeFServeConfig();

      // We might delay here as hash will be generated.
      H.generateMyFilesDB();

      // Sync the DIR menu with XGlobal->ServingDir[]
      syncDirMenuWithServingDir();
   }
   delete [] DirStr;
   delete [] message;
   return(1);
}

// Process the ID_SETSERVINGDIR coming from clicking Dir->Set Serving Dir.
// Handles ID_SETSERVINGDIR -> ID_SETSERVINGDIR + FSERV_MAX_SERVING_DIRECTORIES
// - 1
long TabBookWindow::onSetServingDir(FXObject *, FXSelector Sel, void*) {
int DirIndex;

   TRACE();

   DirIndex = FXSELID(Sel) - ID_SETSERVINGDIR;
   COUT(cout << "File->Set Serving Dir Index: " << DirIndex << endl;)

   // We present user with the Dir Selection dialog.
   FXDirDialog Dir(this, "Set Serving Directory");
   XGlobal->lock();
   Dir.setDirectory(XGlobal->ServingDir[DirIndex]);
   XGlobal->unlock();
   if (Dir.execute()) {
      char *dirstr;
      Helper H;
      FXString fname;

      H.init(XGlobal);
      fname = Dir.getDirectory();
      COUT(cout << "Set serving dir to: " << fname.text() << endl;)

      XGlobal->lock();

      // Check that the new directory is not the same as the Partial Dir.
      // Check if FileName is the same. (ignoring Dir)
      dirstr = new char[strlen(fname.text()) + 1];
      strcpy(dirstr, fname.text());
      if (strcasecmp(getFileName(dirstr), getFileName(XGlobal->PartialDir)) == 0) {
         XGlobal->unlock();

         FXMessageBox::error(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER, "Serving Dir", "Serving Directory cannot be same as the Partial Directory!");
      }
      else {
         delete [] XGlobal->ServingDir[DirIndex];
         XGlobal->ServingDir[DirIndex] = new char[strlen(dirstr) + 1];
         strcpy(XGlobal->ServingDir[DirIndex], dirstr);

         // Have to set the Tracing Directory too, if DirIndex == 0
         if (DirIndex == 0) {
            char *temp_ptr = new char[strlen(XGlobal->ServingDir[DirIndex]) + strlen(DIR_SEP) + 6];
            sprintf(temp_ptr, "%s%sCrash", XGlobal->ServingDir[DirIndex], DIR_SEP);
            Tracer.setTraceDir(temp_ptr);
            delete [] temp_ptr;
         }

         XGlobal->unlock();

         H.writeFServeConfig();

         // We might delay here as hash will be generated.
         H.generateMyFilesDB();

         // Sync the DIR menu with XGlobal->ServingDir[]
         syncDirMenuWithServingDir();
      }
      delete [] dirstr;
   }
   return(1);
}

// Process the ID_OPENSERVINGDIR coming from clicking File->Serving Dir.
long TabBookWindow::onOpenServingDir(FXObject *, FXSelector Sel, void*) {
char arglist[MAX_PATH];
int DirIndex;

   TRACE();

   DirIndex = FXSELID(Sel) - ID_OPENSERVINGDIR;
   COUT(cout << "File->Open Serving Dir Index: " << DirIndex << endl;)
#ifdef __MINGW32__
STARTUPINFO si;
PROCESS_INFORMATION pi;

   ZeroMemory( &si, sizeof(si) );
   si.cb = sizeof(si);
   ZeroMemory( &pi, sizeof(pi) );

   XGlobal->lock();
   sprintf(arglist, "explorer %s", XGlobal->ServingDir[DirIndex]);
   XGlobal->unlock();
   if (CreateProcess(NULL, arglist,
        NULL,       // Process handle not inheritable.
        NULL,       // Thread handle not inheritable.
        FALSE,      // Set handle inheritance to TRUE to access server_pid
        0,          // No creation flags.
        NULL,       // Use parent's environment block.
        NULL,       // Use parent's starting directory.
        &si,        // Pointer to STARTUPINFO structure.
        &pi )       // Pointer to PROCESS_INFORMATION structure.
        ) {
        // Handles in PROCESS_INFORMATION must be closed with CloseHandle
        // when they are no longer needed.
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
   }
#else

   XGlobal->lock();
   sprintf(arglist, "nautilus %s", XGlobal->ServingDir[DirIndex]);
   XGlobal->unlock();

   int retval = system(arglist);
   COUT(cout << "system(" << arglist << ") returned: " << retval << endl;)

   if (retval != 0) {
      // We failed to spawn nautilus. Lets try konqueror
      XGlobal->lock();
      sprintf(arglist, "konqueror %s &", XGlobal->ServingDir[DirIndex]);
      COUT(cout << "system(" << arglist << ")" << endl;)
      XGlobal->unlock();
      retval = system(arglist);
      COUT(cout << "system(" << arglist << ") returned: " << retval << endl;)
   }
#endif

   return(1);
}

// Called on LEFT MOUSE RELEASE
// also on RIGHT BUTTON RELEASE
// Called from FXText.
// Does 4 things:
// a) Highlight the word under cursor in nick list.
// b) Put that word in the topline Search Text Field.
// c) Copy the text in buffer to allow pasting. transfer focus to input.
// d) popup the menu.
// Left mouse release should do (a) and (b)
// Right button release should do (a) and (c)
long TabBookWindow::onMouseRelease(FXObject *sender, FXSelector sel, void *) {
int i;

   TRACE();

   i = FocusIndex;
   COUT(cout << "onMouseRelease" << endl;)

   // One of the things we try to do is if in a window which has a NickList,
   // we get the word under cursor, and check if that word is in the NickList.
   // If it is, we select that nick in the NickList.
   char *theword = new char[256];
   do {
      if (Windows[i].ScrollUI) {

         FXint CurPos;
         COUT(cout << "onMouseRelease: " << Windows[i].ScrollUI->getSelStartPos() << " " << Windows[i].ScrollUI->getSelEndPos() << " " << Windows[i].ScrollUI->getCursorPos() << " " << Windows[i].ScrollUI->getAnchorPos() << endl;)
         CurPos = Windows[i].ScrollUI->getCursorPos();
         FXint WordS = Windows[i].ScrollUI->wordStart(CurPos);
         FXint WordE = Windows[i].ScrollUI->wordEnd(CurPos);
         FXint WordLen = WordE - WordS;

         if (WordLen == 0) break;
         Windows[i].ScrollUI->extractText(&theword[1], WordS, WordLen);
         theword[0] = ' ';
         theword[WordLen + 1] = '\0';
         COUT(cout << "word under cursor: " << theword << endl;)

         // Lets put that word in the topline Search Text.
         textsearch->setText(&theword[1]);

         if (Windows[i].NickListUI) {
            // Lets see if that word is in NickListUI
            FXint NumList = Windows[i].NickListUI->getNumItems();
            if (NumList == 0) break;

            for (int j = 1; j <= 4; j++) {
               switch (j) {
                  case 1: theword[0] = '@';
                       break;

                  case 2: theword[0] = '%';
                       break;

                  case 3: theword[0] = '+';
                       break;

                  case 4: theword[0] = ' ';
                       break;
               }
               FXint ListIndex = Windows[i].NickListUI->findItem(theword, 0, SEARCH_FORWARD|SEARCH_IGNORECASE);
               if (ListIndex == -1) continue;

               if (ListIndex < NumList) {
                  // Got a hit. Give it focus.
                  Windows[i].NickListUI->killSelection();
                  Windows[i].NickListUI->makeItemVisible(ListIndex);
                  Windows[i].NickListUI->setCurrentItem(ListIndex);
                  Windows[i].NickListUI->selectItem(ListIndex);

                  // If its the messages window. We update the label too.
                  int messages = getWindowIndex("Messages");
                  if (messages == i) {
                     Windows[messages].LabelUI->setText(theword);
                  }
               }
            }
         }
      }
   } while (false);
   delete [] theword;

   if (FXSELTYPE(sel) == SEL_LEFTBUTTONRELEASE) {
      // Lets give the TextField widget focus. after auto copying
      if (Windows[i].ScrollUI) {
         Windows[i].ScrollUI->onCmdCopySel(NULL, 0, NULL);
      }
      if ( (Windows[i].InputUI) && 
           strcasecmp(Windows[i].Name, TAB_FILESEARCH) ) {
         // Give focus to the Input UI only if not File Search TAB
         // This is so that, Key UP/Down can be used to move up/down
         // the File Search List.
         Windows[i].InputUI->setFocus();
      }
   }

   // Check if its for a popup menu.
   if (FXSELTYPE(sel) == SEL_RIGHTBUTTONRELEASE) {
      COUT(cout << "SEL_RIGHTBUTTONRELEASE: Calling: onCmdPopUp" << endl;)
      onCmdPopUp();
   }
   return(1);
}

// Process the ID_HELPDEMOS coming from clicking Help->Demos.
long TabBookWindow::onHelpDemos(FXObject *, FXSelector, void*) {
char arglist[MAX_PATH];
const char *demo_page = "http://www.masalaboard.com/masalairc/masalamate.php";

   TRACE();

   COUT(cout << "Help->Demos" << endl;)

#ifdef __MINGW32__
   HWND top = GetTopWindow(NULL);
   ShellExecute(top, "open", demo_page, "", "", SW_SHOW);
#else

   sprintf(arglist, "firefox %s &", demo_page);

   int retval = system(arglist);
   COUT(cout << "system(" << arglist << ") returned: " << retval << endl;)

   if (retval != 0) {
      // We failed to spawn firefox. Lets try mozilla.
      sprintf(arglist, "mozilla %s &", demo_page);
      retval = system(arglist);
      COUT(cout << "system(" << arglist << ") returned: " << retval << endl;)
   }
#endif

   return(1);
}

// Process the ID_HELPABOUT coming from clicking Help->About.
long TabBookWindow::onHelpAbout(FXObject *, FXSelector, void*) {
char *message = new char[512];
   TRACE();

   sprintf(message, "MasalaIRC proudly presents: %s - %s - %s\n\n"
                    "                    http://www.masalaboard.com/masalairc\n\n"
                    "                  http://groups.yahoo.com/group/masalamate\n\n"
                    "             @IRCSuper.net #IndianMasala and #Masala-Chat\n\n"
                    "                         Copyright (C) 2004,2005 Sur4802\n\n\n"
                    "This software uses the FOX Toolkit Library (http://www.fox-toolkit.org).",
                    CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING);
   FXMessageBox about(this, "About MasalaMate", message, NULL,
                      MBOX_OK|DECOR_TITLE|DECOR_BORDER);
   about.execute();

   delete [] message;

  return(1);
}

// Function which attemptes to download the selected line in the 
// "tab_name" SearchUI.
// If TAB is Downloads, then we attempt it only if "Time Left" (7) entry
// is PARTIAL.
void TabBookWindow::onDownloadSearchUISelected(char *tab_name) {
int win_index;
FXFoldingItem *selection; // NULL => no selection.
FXString ItemStr;
LineParse LineP;
const char *charp;
char *tmpnick;
char *tmpdir;
char *DwnldInitLine;
int nick_index;
bool isDownloadsTAB = false;
bool isFileSearchTAB = false;
bool isWaitingTAB = false;
FilesDetail *FD;

   TRACE();

   win_index = getWindowIndex(tab_name);

   selection = Windows[win_index].SearchUI->getCurrentItem();
   if (selection == NULL) {
      return;
   }

   DwnldInitLine = new char[512];
   ItemStr = selection->getText();
   charp = ItemStr.text();

   LineP = (char *) charp;
   LineP.setDeLimiter('\t');

   // Now we set the right word index to use depending on the TAB
   if (strcasecmp(tab_name, TAB_FILESEARCH) == 0) {
      isFileSearchTAB = true;
      nick_index = HEADER_FILESEARCH_NICK + 1;
   }
   else if (strcasecmp(tab_name, "Downloads") == 0) {
      isDownloadsTAB = true;
      nick_index = HEADER_DOWNLOADS_NICK + 1;
      // Attempt only if "Time Left" tab is PARTIAL (6th field)
      charp = LineP.getWord(HEADER_DOWNLOADS_TIMELEFT + 1);
      if (strcasecmp(charp, "PARTIAL")) {
         FXMessageBox::error(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER, "Rerequest Partial File", "The selected File is Not in the PARTIAL STATE !");
         delete [] DwnldInitLine;
         return;
      }
   }
   else if (strcasecmp(tab_name, "Waiting") == 0) {
      isWaitingTAB = true;
      nick_index = HEADER_WAITING_NICK + 1;
   }

   // Get the nick_index word which is the nick.
   charp = LineP.getWord(nick_index);
   tmpnick = new char[strlen(charp) + 1];
   strcpy(tmpnick, charp);

   // Now first check if this nick is present in FServClientPending/InProgress.
   //  => a current file server access is in progress with that nick, 
   // so inform as such.
   if ( XGlobal->FServClientPending.isNickPresent(tmpnick) ||
        XGlobal->FServClientInProgress.isNickPresent(tmpnick)
      ) {
      FXMessageBox::information(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER, tab_name, "This Nick is currently busy. Please retry again in a few minutes.");
      delete [] DwnldInitLine;
      delete [] tmpnick;
      return;
   }

   // For the directory name, in case of "Downloads", we get from searching
   // DwnldInProgress Q
   // In case of "Waiting" we get from DwnldWaiting
   // In case of "File Search", get the third word which is the dir name.
   // For tmpdir make sure that space => empty, shouldnt be NULL.
   if (isFileSearchTAB) {
      charp = LineP.getWord(HEADER_FILESEARCH_DIRNAME + 1);
      if (strcasecmp(charp, UI_PARTIAL_DIRNAME) == 0) {
         // Consider this as an empty directory.
         tmpdir = new char[2];
         strcpy(tmpdir, " ");
      }
      else {
         tmpdir = new char[strlen(charp) + 1];
         strcpy(tmpdir, charp);
      }
   }
   else if (isDownloadsTAB || isWaitingTAB) {
      tmpdir = new char[2];
      strcpy(tmpdir, " "); // space => empty dir.
      if (isDownloadsTAB) {
         charp = LineP.getWord(HEADER_DOWNLOADS_FILENAME + 1);
         FD = XGlobal->DwnldInProgress.getFilesDetailListNickFile(tmpnick, (char *) charp);
      }
      else {
         charp = LineP.getWord(HEADER_WAITING_FILENAME + 1);
         FD = XGlobal->DwnldWaiting.getFilesDetailListNickFile(tmpnick, (char
 *) charp);
      }
      if (FD) {
         if (FD->DirName) {
            delete [] tmpdir;
            tmpdir = new char[strlen(FD->DirName) + 1];
            strcpy(tmpdir, FD->DirName);
         }
         XGlobal->DwnldInProgress.freeFilesDetailList(FD);
         FD = NULL;
      }
   }

   // Get the first word which is the file name.
   charp = LineP.getWord(HEADER_FILESEARCH_FILENAME + 1);

   if (XGlobal->MyFilesDB.isPresentMatchingFileName(charp) == false) {
      // We attempt a download if we dont have it in our Serving Folder.

      // We also attempt only if FileName is not "No Files Present"
      // and not "Inaccessible Server"
      if (strcasecmp(charp, "No Files Present") == 0) {
         sprintf(DwnldInitLine, "Server 07Download: If File Name is \"No Files Present\", it implies that the server is not serving any files.");
         XGlobal->IRC_ToUI.putLine(DwnldInitLine);
      }
      else if (strcasecmp(charp, "Inaccessible Server") == 0) {
         sprintf(DwnldInitLine, "Server 07Download: If File Name is \"Inaccessible Server\", it implies that the server is not accessible.");
         XGlobal->IRC_ToUI.putLine(DwnldInitLine);
      }
      else {
         // Here Check with nick/file/tmpdir if it exists in FilesDB
         // Give prompt accordingly depending on the TAB
         // Note tmpdir is NOT NULL, as required by this function.
         FD = XGlobal->FilesDB.getFilesDetailListNickFileDir(tmpnick, (char *) charp, tmpdir);
         if (FD) {
            // FDs in FilesDB reflect will reflect if its a partial or not.
            if (FD->DownloadState == DOWNLOADSTATE_PARTIAL) {
               sprintf(DwnldInitLine, "GETPARTIAL\001%s\001%s\001%s", tmpnick, charp, tmpdir);
            }
            else {
               sprintf(DwnldInitLine, "GET\001%s\001%s\001%s", tmpnick, charp, tmpdir);
            }

            COUT(cout << "TabBookWindow::onDownloadSearchUISelected: DwnldInitLine: " << DwnldInitLine << endl;)
            // Lets Q it to the UI_ToDwnldInit Queue.
            XGlobal->UI_ToDwnldInit.putLine(DwnldInitLine);

            XGlobal->FilesDB.freeFilesDetailList(FD);
            FD = NULL;
         }
         else {
            // Give the error message nicely depending on the TAB
            if (isFileSearchTAB) {
               sprintf(DwnldInitLine, "No information present to download the selected File.\n Please refresh the File Search TAB and try again.");
            }
            else if (isDownloadsTAB) {
               sprintf(DwnldInitLine, "Initiating this download is not possible currently.\nPossible reason is that nick is not in channel anymore,\nor if you have just logged in,\nwe do not have the information on this Nick yet.\nPlease try again later.");
            }
            else if (isWaitingTAB) {
               sprintf(DwnldInitLine, "Cannot update Queue information of Selected File.\n Possible reason is that nick is not in channel anymore,\nor if you have just logged in,\nwe do not have information on this Nick yet.\nPlease try again later.");
            }
            FXMessageBox::error(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER, "Error Initiating Update", DwnldInitLine);
         }
      }
   }
   else {
      // Inform User that we are not attempting.
      //XGlobal->lock();
      //sprintf(DwnldInitLine, "Server 07Download: %s already present in %s Folder.", charp, XGlobal->ServingDir);
      //XGlobal->unlock();
      //XGlobal->IRC_ToUI.putLine(DwnldInitLine);
      // Just give a information Message Box.
      sprintf(DwnldInitLine, "\"%s\" is already present in our Serving File. Not Attempting.", charp);
      FXMessageBox::information(this, MBOX_OK, "Download Attempt", DwnldInitLine);
   }
   delete [] tmpnick;
   delete [] tmpdir;
   delete [] DwnldInitLine;
   return;
}


// On clicking "Cancel selected download" in the right click popup menu
// in "Downloads" TAB
void TabBookWindow::cancelDownloadSelected(void) {
int win_index;
FXFoldingItem *selection; // NULL => no selection.
FXString ItemStr;
LineParse LineP;
const char *charp;
FilesDetail *FD;
char *message;

   TRACE();

   win_index = getWindowIndex("Downloads");

   selection = Windows[win_index].SearchUI->getCurrentItem();
   if (selection == NULL) {
      return;
   }

   ItemStr = selection->getText();
   charp = ItemStr.text();

   LineP = (char *) charp;
   LineP.setDeLimiter('\t');

   // Get the 6th word (Time Left). It shouldnt be COMPLETE to PARTIAL.
   charp = LineP.getWord(HEADER_DOWNLOADS_TIMELEFT + 1);
   if ( (strcasecmp(charp, "PARTIAL") == 0) ||
        (strcasecmp(charp, "COMPLETE") == 0) ) {
      // Give him a message.
      FXMessageBox::error(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER, "Cancel Download", "The Selected Download is not in Progress");
      return;
   }

   message = new char[512];

   // Get the first word which is the file name.
   charp = LineP.getWord(HEADER_DOWNLOADS_FILENAME + 1);
   sprintf(message, "File: %s\nThis download might take 10 seconds to actually stop.\nCancel this Download ?", charp);
   if (FXMessageBox::question(this,MBOX_YES_NO,"Download Cancel", message) == MBOX_CLICKED_YES) {
      // This download needs to be cancelled.
      FD = XGlobal->DwnldInProgress.getFilesDetailListMatchingFileName((char *) charp);
      if (FD) {
         COUT(cout << "CANCEL DOWNLOAD: FD: ";) 
         COUT(XGlobal->DwnldInProgress.printDebug(FD);)
         if (FD->TriggerType != XDCC) {
            // Iroffer XDCCs do not need this NoResend
            sprintf(message, "PRIVMSG %s :\001NoReSend\001", FD->Nick);
            XGlobal->IRC_ToServerNow.putLine(message);
            sprintf(message, "CANCELD\001%s\001%s", FD->Nick, FD->FileName);
            XGlobal->UI_ToDwnldInit.putLine(message);
            COUT(cout << "TabBookWindow::cancelDownloadSelected: Cancel " << message << endl;)
         }
         else {
            // This is an XDCC just disconnect it.
            if (FD) {
               XGlobal->DwnldInProgress.updateFilesDetailNickFileConnectionMessage(FD->Nick, FD->FileName, CONNECTION_MESSAGE_DISCONNECT_DOWNLOAD);
            }
         }
         XGlobal->DwnldInProgress.freeFilesDetailList(FD);
      }
   }
   delete [] message;
}

// Process the ID_SELECTCHANNELTABTODISPLAY coming from selecting the Channel
// Name in the FXList selection.
long TabBookWindow::onChannelTabToDisplay(FXObject *channel_list, FXSelector sel, void *ptr) {
FXList *TheList;
FXint TheItem;
FXString TheString;
const char *channel_text;
int i;

   TRACE();

   // channel_list is an FXList. We get the selected item from it.
   TheList = (FXList *) channel_list;
   TheItem = (FXint) (FXival) ptr;
   TheString = TheList->getItemText(TheItem);
   channel_text = TheString.text();

   // We need to make the channel_text TAB show.
   i = getWindowIndex((char *) channel_text);
   COUT(cout << "TabBookWindow::onChannelTabToDisplay channel: " << channel_text << " Current FocusIndex is: " << FocusIndex << " To be changed to: " << i << endl;)
   if ( (i != -1)  && (i != FocusIndex) ) {
      // We need to make FocusIndex Hide, only if its a Channel window.
      // or its a channel we just /part ed. => name is UI_FREE_CHANNEL_NAME
      if ( (Windows[FocusIndex].Name[0] == '#') ||
           (strcasecmp(Windows[FocusIndex].Name, UI_FREE_CHANNEL_NAME) == 0)
         ) {
         Windows[FocusIndex].TabUI->hide();
      }
      else {
         // We are coming here cause of right or left arrow switch tabs.
         // So current tab is Messages, going over to the channel tab.
         // We need to hide the tab before Messages which is a Channel
         // and the Channel TAB after it.
         // And compulsorily hide the CHANNEL_MM TAB.

         int j = FocusIndex - 1;
         while (j >= 0) {
            if ( (Windows[j].Name[0] == '#') &&
                 (strcasecmp(Windows[j].Name, CHANNEL_MM))
               ) {
               // CHANNEL WINDOW but not CHANNEL_MM
               break;
            }
            j--;
         }
         Windows[j].TabUI->hide();

         j = 0;
         while (j < UI_MAXTABS) {
            if ( (Windows[j].Name[0] == '#') &&
                 (strcasecmp(Windows[j].Name, CHANNEL_MM))
               ) {
               // CHANNEL WINDOW but not CHANNEL_MM
               break;
            }
            j++;
         }
         Windows[j].TabUI->hide();

         // Always hide the CHANNEL_MM window.
         j = getWindowIndex(CHANNEL_MM);
         Windows[j].TabUI->hide();
      }

      // Reparent ChannelListUI to this new window.
      ChannelListUI->reparent(Windows[i].ChannelListUIFather);

      // Call below will change FocusIndex to i.
      // Looks like below call shoudl not be made. The later call to
      // tabbool->setCurrent(i, TRUE) will issue the ID_PANEL
      //onCmdPanel(this, TabBookWindow::ID_PANEL, (void *) i);

      Windows[i].TabUI->show();
      tabbook->setCurrent(i, TRUE);
   }

   return(1);
}

// Process the ID_SELECTPMNICK coming from Double clicking NickList selection.
// Act on it only from non messages windows.
long TabBookWindow::onDCNickSelect(FXObject *obj, FXSelector sel, void* ptr) {
FXList *nick_list = (FXList *) obj;
FXint index = (FXint) (FXival) ptr; // 0 => first element etc.
FXString itemtext;
char *nicktext;
int messages;
FXint i;

   TRACE();
   if (FXSELID(sel) != ID_SELECTPMNICK) return(0);

// Lets grab the index of the Messages window.
   messages = getWindowIndex("Messages");
   // messages should not be -1

// Lets find out who is in focus. Its FocusIndex duh !
   COUT(cout << "DC: ID_SELECTPMNICK: Received: " << index << " " << Windows[FocusIndex].Name << endl;)


   // If its the messages window itself then lets leave.
   if (FocusIndex == messages) return(1);

   // Lets put that text in the Messages Label.
   itemtext = nick_list->getItemText(index);

   nicktext = new char[strlen(itemtext.text()) + 1];
   strcpy(nicktext, itemtext.text());
   nicktext[0] = ' ';
         
   Windows[messages].LabelUI->setText(nicktext); //1st char is space.

   // Add this nick to NickList in Messages tab.
   addToNickList(Windows[messages].NickListUI, nicktext);

   // Lets make the Messages tab become active.
   tabbook->setCurrent(messages, TRUE);

   // Select the Nick in NickList in Messages tab too.
   i = Windows[messages].NickListUI->findItem(nicktext);
   if (i >= 0) {
      Windows[messages].NickListUI->killSelection();
      Windows[messages].NickListUI->setCurrentItem(i);
      Windows[messages].NickListUI->selectItem(i);
      Windows[messages].NickListUI->makeItemVisible(i);
   }

   delete [] nicktext;

   return(1);
}


// Process the ID_SELECTPMNICK coming from NickList Nick selection.
// act on it only if its from the Messages Tab.
long TabBookWindow::onNickSelectPM(FXObject *obj, FXSelector sel, void* ptr) {
FXint index = (FXint) (FXival) ptr; // 0 => first element etc.
FXList *nicklistui = (FXList *) obj;
int messages_tab_index;
FXString itemtext;

   TRACE();
   if (FXSELID(sel) != ID_SELECTPMNICK) return(0);

   // Check if its for a popup menu.
   if (FXSELTYPE(sel) == SEL_RIGHTBUTTONRELEASE) {
      COUT(cout << "onNickSelectPM: SEL_RIGHTBUTTONRELEASE: calling onCmdPopUp" << endl;)
      onCmdPopUp();
      return(1);
   }

// Lets find out who is in focus. Its FocusIndex duh !
   // Do not use FocusIndex but the object passed to determin which window.
   // If not we do some erroneous selecting in case of Double Clicks.
   // Check README2.txt -> search "DOUBLE CLICK - SINGLE CLICK - BUG"
   // section o)
   messages_tab_index = getWindowIndex("Messages");
// Lets not do anything if its not the Messages Window.
   if (nicklistui != Windows[messages_tab_index].NickListUI) {
      return(1);
   }

   // Lets put that text in the Label.
   COUT(cout << "index: " << index << "getNumItems(): " << nicklistui->getNumItems() <<  endl;);
   itemtext = nicklistui->getItemText(index);
   Windows[messages_tab_index].LabelUI->setText(itemtext);

   // Lets give the TextField widget focus.
   Windows[messages_tab_index].InputUI->setFocus();

   return(1);
}

// Expand on the Text Inputs.
long TabBookWindow::onExpandTextEntry(FXObject *,FXSelector,void *ptr) {
FXEvent* event=(FXEvent*)ptr;
int item_cnt;
const char *parseptr1, *parseptr;
const char *cmp_nick;
char *tmpword;
LineParse LineP;
FXString fx_string, fx_string2;
int wordcnt, cmp_len, itemcnt, i, j;
bool found_last_expanded_entry = false;
static bool ControlPressed = false;

// COUT(cout << "TabBookWindow::onExpandTextEntry " << endl;)

   TRACE();
   switch (event->code) {
     case KEY_Control_L:
     case KEY_Control_R:
        break;

     case KEY_k:
        COUT(cout << "TabBookWindow::onExpandTextEntry: K pressed" << endl;)
        // Here we inject the \003 in the input, if ControlPressed is true.
        if (ControlPressed) {
           i = FocusIndex;
           if (Windows[i].InputUI) {
              FXString Str;
              Str = Windows[i].InputUI->getText();
              int cur_pos = Windows[i].InputUI->getCursorPos();
              Str.insert(cur_pos, '\003');
              Windows[i].InputUI->setText(Str);
              Windows[i].InputUI->setCursorPos(cur_pos + 1);
           }
        }
        ControlPressed = false;

        // Return 0 when we arent handling it.
        return(0);
        break;

     case KEY_Tab:
       // FocusIndex is the index of active window.
       // Lets try to complete the nick with the help of NickListUI
       // Basically Windows[FocusIndex].NickListUI->

       i = FocusIndex;
       // We expand only if Windows[i].NickListUI is non NULL.
       if (Windows[i].NickListUI == NULL) break;

       // First lets get the string we are trying to complete.
       // Windows[FocusIndex].InputUI->
       if (Windows[i].LastTabPartialNick[0] == '\0') {
          fx_string = Windows[i].InputUI->getText();
          parseptr = fx_string.text();
          LineP = (char *) parseptr;
          wordcnt = LineP.getWordCount();
          parseptr = LineP.getWord(wordcnt);
          strncpy(Windows[i].LastTabPartialNick, parseptr, sizeof(Windows[i].LastTabPartialNick));
          Windows[i].LastTabPartialNick[sizeof(Windows[i].LastTabPartialNick) - 1] = '\0';
       }

       // The word we are trying to expand is in Windows[i].LastTabPartialNick
       cmp_len = strlen(Windows[i].LastTabPartialNick);
       // And its length

       // We search for LastTabCompletedNick in Windows[i].NickListUI
       itemcnt = Windows[i].NickListUI->getNumItems();
       for (j = 0; j < itemcnt; j++) {
          fx_string = Windows[i].NickListUI->getItemText(j);
          cmp_nick = fx_string.text();
          // we compare omitting the 1st charater as its nicks mode.
          if (strncasecmp(Windows[i].LastTabPartialNick, &cmp_nick[1], cmp_len) == 0) {
             // We got a hit in the comparison.
//COUT(cout << "onExpandTextEntry: Got hit: " << &cmp_nick[1] << endl;)
             // See if this hit is same as we got last time,
             if (Windows[i].LastTabCompletedNick[0] == '\0') {
                found_last_expanded_entry = true;
             }

             if (found_last_expanded_entry) {

                // Update LastTabCompletedNick
                strncpy(Windows[i].LastTabCompletedNick, &cmp_nick[1], sizeof(Windows[i].LastTabCompletedNick));
                Windows[i].LastTabCompletedNick[sizeof(Windows[i].LastTabCompletedNick) - 1] = '\0';

                fx_string2 = Windows[i].InputUI->getText();
                parseptr = fx_string2.text();
                LineP = (char *) parseptr;
                wordcnt = LineP.getWordCount();
                if (wordcnt <= 1) {
                   parseptr = "";
                }
                else {
                   parseptr = LineP.getWordRange(1, wordcnt - 1);
                }
                tmpword = new char[strlen(parseptr) + strlen(Windows[i].LastTabCompletedNick) + 2];
                if (strlen(parseptr)) {
                   sprintf(tmpword, "%s %s", parseptr, Windows[i].LastTabCompletedNick);
                }
                else {
                   sprintf(tmpword, "%s", Windows[i].LastTabCompletedNick);
                }
                Windows[i].InputUI->setText(tmpword);
                Windows[i].InputUI->setCursorPos(strlen(tmpword));
//COUT(cout << "onExpandTextEntry: Line expanded to: " << tmpword << endl;)
                delete [] tmpword;

                break;
             }

             if (strcasecmp(Windows[i].LastTabCompletedNick, &cmp_nick[1]) == 0) {
                found_last_expanded_entry = true;
             }
          }
       }

       break;

     case KEY_Up:
     case KEY_KP_Up:
       // FocusIndex is the index of active window.
       parseptr = Windows[FocusIndex].History->getNextLine();
       if (parseptr) {
          COUT(cout << "History.getNextLine: " << parseptr << endl;)
          Windows[FocusIndex].InputUI->setText(parseptr);
          Windows[FocusIndex].InputUI->setCursorPos(strlen(parseptr));
       }
       break;

     case KEY_Down:
     case KEY_KP_Down:
       // FocusIndex is the index of active window.
       parseptr = Windows[FocusIndex].History->getPreviousLine();
       if (parseptr) {
          COUT(cout << "History.getPreviousLine: " << parseptr << endl;)
          Windows[FocusIndex].InputUI->setText(parseptr);
          Windows[FocusIndex].InputUI->setCursorPos(strlen(parseptr));
       }
       break;

    case KEY_Page_Up:
    case KEY_KP_Page_Up:
       // FocusIndex is the index of active window.
       // On pressing Left, we switch focus to a previous Channel TAB.
       i = FocusIndex;
       i--;
       do {
          if (i < 0) i = UI_MAXTABS - 1;
          // break if its Messages or # but not CHANNEL_MM
          if ( (strcasecmp(Windows[i].Name, "Messages") == 0) ||
               ( (strcasecmp(Windows[i].Name, CHANNEL_MM) != 0) &&
                 (Windows[i].Name[0] == '#')
               )
             ) {
             break;
          }
          i--;
       } while (true);

       if (Windows[i].Name[0] == '#') {
          // We need to do some more things when switching channels.
          FXint item = ChannelListUI->findItem(Windows[i].Name);
          if (item >= 0) ChannelListUI->selectItem(item);
          onChannelTabToDisplay(ChannelListUI, ID_SELECTCHANNELTABTODISPLAY, (void *) item);
       }
       else {
          // Switching to the Messages TAB.
          tabbook->setCurrent(i, TRUE);
       }
       break;

    case KEY_Page_Down:
    case KEY_KP_Page_Down:
       // FocusIndex is the index of active window.
       // On pressing Left, we switch focus to a previous Channel TAB.
       i = FocusIndex;
       i++;
       do {
          if (i >= UI_MAXTABS) i = 0;
          // break if its Messages or # but not CHANNEL_MM
          if ( (strcasecmp(Windows[i].Name, "Messages") == 0) ||
               ( (strcasecmp(Windows[i].Name, CHANNEL_MM) != 0) &&
                 (Windows[i].Name[0] == '#')
               )
             ) {
             break;
          }
          i++;
       } while (true);

       if (Windows[i].Name[0] == '#') {
          // We need to do some more things when switching channels.
          FXint item = ChannelListUI->findItem(Windows[i].Name);
          if (item >= 0) ChannelListUI->selectItem(item);
          onChannelTabToDisplay(ChannelListUI, ID_SELECTCHANNELTABTODISPLAY, (void *) item);
       }
       else {
          // Switching to the Messages TAB.
          tabbook->setCurrent(i, TRUE);
       }
       break;

     default:
       // Reinitialise the TAB parameters.
       i = FocusIndex;
       Windows[i].LastTabCompletedNick[0] = '\0';
       Windows[i].LastTabPartialNick[0] = '\0';
       ControlPressed = false;
       return(0); // Return this when we arent expanding anything.

   }

// Code to handle those keys.
   if ( (event->code == KEY_Control_L) || (event->code == KEY_Control_R) ) {
      ControlPressed = true;
   }
   else {
      ControlPressed = false;
   }
   return(1);
}

// Process Text Inputs.
long TabBookWindow::onTextEntry(FXObject *txt_entry,FXSelector sel,void* ptr) {
int windex;
char *c_ptr = (char *) ptr;
char *line, *uiline;
bool message_window;
bool server_window;
FXString label_string;
const FXchar *to;
LineParse LineP;
const char *parseptr;
char apt_channel[128];

   TRACE();

//   COUT(cout << "TextEntry: ";)
   if (FXSELID(sel) != ID_TEXTINPUT) return(0);

// Lets find out who is in focus. Its FocusIndex duh !
   windex = FocusIndex;
   if (strlen(c_ptr) == 0) {
      return(0);
   }

   // Check if the line is more than 320 bytes. truncate if it is.
   // We handle multi lines now, so let it flow thru.
   // if (strlen(c_ptr) >= 320) {
   //    c_ptr[320] = '\0';
   // }

   // Lets add this line to the History.
   if (Windows[windex].History) {
      Windows[windex].History->addLine(c_ptr);
      // COUT(Windows[windex].History->printDebug();)
   }

   // Lets reset LastTabCompletedNick and LastTabPartialNick
   Windows[windex].LastTabCompletedNick[0] = '\0';
   Windows[windex].LastTabPartialNick[0] = '\0';

//       COUT(cout << "ID_TEXTINPUT: Received: " << c_ptr << " " << Windows[windex].Name << endl;)

   // Lets see if this is the File Search Window
   if (strcasecmp(Windows[windex].Name, TAB_FILESEARCH) == 0) {
      doFileSearch();
      // Do not Clear the widget.
      // Windows[windex].InputUI->setText("");
      return(1);
   }

// Lets see if it needs to be processed differently if its a
// Messages window.
   if ( (strcasecmp(Windows[windex].Name, TAB_MESSAGES) == 0) ||
        (strcasecmp(Windows[windex].Name, TAB_DCC_CHAT) == 0) ) {
      message_window = true;
      server_window = false;
      label_string = Windows[windex].LabelUI->getText();
      to = label_string.text();
      to++; // Move past the space in first position.
   }
   else if (strcasecmp(Windows[windex].Name, "Server") == 0) {
      message_window = false;
      server_window = true;
      to = CHANNEL_MAIN;
   }
   else {
      message_window = false;
      server_window = false;
      to = Windows[windex].Name;
   }

   line = new char[512];
   uiline = new char[512];

   // Lets see if this is the TAB_DCC_CHAT window.
   if (strcasecmp(Windows[windex].Name, TAB_DCC_CHAT) == 0) {
      // So first get the FD from DCCChatInProgress, which matches "to"
      // which has correct nick value as set previously.
      if (strncasecmp(c_ptr, "/clear", 6)) {
         XGlobal->lockDCCChatConnection();
         FilesDetail *FD = XGlobal->DCCChatInProgress.getFilesDetailListOfNick(to);
         if (FD && FD->Connection) {
            // We just write out c_ptr in FD->Connection->writeData()
            sprintf(line, "%s\n", c_ptr);
            FD->Connection->writeData(line, strlen(line), DCCCHAT_TIMEOUT);
            sprintf(uiline, "%s <To %s> %s", TAB_DCC_CHAT, FD->Nick, c_ptr);
            XGlobal->IRC_ToUI.putLine(uiline);
         }
         XGlobal->DCCChatInProgress.freeFilesDetailList(FD);
         FD = NULL;
         XGlobal->unlockDCCChatConnection();

         // Clear the text as we have processed it.
         Windows[windex].InputUI->setText("");

         delete [] line;
         delete [] uiline;

         return(1);
      }
   }

// Lets prepare the line to be sent to server.

   line[0] = '\0';
   uiline[0] = '\0';

   // Lets first handle the non / line.
   do {
      if (strncasecmp("!list", c_ptr, 5) == 0) {
         // We do not allow a manual !list from being issued.
         // !list <nick> is OK
         LineP = c_ptr;
         parseptr = LineP.removeNonPrintable();
         LineP = (char *) parseptr;
         parseptr = LineP.removeConsecutiveDeLimiters();
         LineP = (char *) parseptr;
         if (LineP.getWordCount() == 1) {
            // Only one word and that is !list. Not allowed.
            sprintf(uiline, "%s 04!LIST is only allowed if followed by a Nick", Windows[windex].Name);
            break;
         }

         // It has more than one word. Get the second word. Assume its a nick.
         // and check if a FServPending of that nick is present => a fserv
         // possibly waiting or in progress, and hence do not issue it.
         parseptr = LineP.getWord(2);
         // Now first check if this nick is present in 
         // FServClientPending/InProgress => a current
         // file server access is in progress with that nick, so inform as such
         if ( XGlobal->FServClientPending.isNickPresent(parseptr) ||
              XGlobal->FServClientInProgress.isNickPresent(parseptr)
            ) {
            FXMessageBox::information(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER, Windows[windex].Name, "This Nick is currently busy. Please retry again in a few minutes.");
            break;
         }
         // Actually here is where we pick the most apt Channel, rather
         // than blindly CHANNEL_MAIN
         {
            Helper H;
            H.init(XGlobal);
            H.getAptChannelForNickToIssueList((char *) parseptr, Windows[windex].Name, apt_channel);
            if (strlen(apt_channel) == 0) break;
            to = apt_channel;
         }
      }

      if (c_ptr[0] != '/') {
         // This is where c_ptr has to be checked for multiple lines.
         // Handle it all and set line[0], and uiline[0] to '\0', so 
         // it doesnt send some junk after we are done here.
         LineP = c_ptr;
         // Now we replace all \r\n and \r to \n
         parseptr = LineP.replaceString("\r\n", "\n");
         LineP = (char *) parseptr;
         parseptr = LineP.replaceString("\r", "\n");
         LineP = (char *) parseptr;
         LineP.setDeLimiter('\n');
         parseptr = LineP.removeConsecutiveDeLimiters();
         LineP = (char *) parseptr;
         LineP.setDeLimiter('\n');
         int lines_count = LineP.getWordCount();
         for (int j = 1; j <= lines_count; j++) {
            parseptr = LineP.getWord(j);
            sprintf(line, "PRIVMSG %s :%s", to, parseptr);
            if (j == 1) {
               // First line goes out immediately.
               XGlobal->IRC_ToServerNow.putLine(line);
            }
            else {
               // Rest of them are delayed.
               XGlobal->IRC_ToServer.putLine(line);
            }

            if (message_window) {
               sprintf(uiline, "%s <TO %s> %s", Windows[windex].Name, to, parseptr);
            }
            else {
               Helper H;
               char color_coded_nick[512];
               H.init(XGlobal);
               H.generateColorCodedNick(to, Nick, color_coded_nick);
               sprintf(uiline, "%s %s %s", to, color_coded_nick, parseptr);
            }
            XGlobal->IRC_ToUI.putLine(uiline);
         }
         // We have handled everything, blank out line, uiline
         line[0] = '\0';
         uiline[0] = '\0';
         break;
      }

#if 0
      if (strncasecmp(c_ptr, "/nickdebug", 10) == 0) {
         COUT(XGlobal->NickList.printDebug();)
         break;
      }
#endif

      // Lets check the / commands we handle.
      if (strncasecmp(c_ptr, "/clear", 6) == 0) {
         // We clear the scroll text region
         // Note that nothing to be done with line.
         if (Windows[windex].ScrollUI) {
            FXint lastPos = Windows[windex].ScrollUI->getLength();
            Windows[windex].ScrollUI->removeText(0, lastPos);
         }

         // We also clear the InputUI if it exists for this window.
         if (Windows[windex].InputUI) {
            Windows[windex].InputUI->setText("");
         }

         if (message_window) {
            if (Windows[windex].NickListUI) {
               Windows[windex].NickListUI->clearItems();
            }
         }
         // Below will take care of all windows, including Messages
         sprintf(uiline, "%s * 09<Window cleared>", Windows[windex].Name);
         break;
      }

      // Handling the /mode command.
      // Allowed only in Channel window.
      if (strncasecmp(c_ptr, "/mode ", 6) == 0) {
         if (Windows[windex].Name[0] != '#') break;

         LineP = c_ptr;
         parseptr = LineP.getWord(2);
         // Check if 2nd word is Channel Name. If not force channel name.
         if (parseptr[0] != '#') {
            parseptr = LineP.getWordRange(2);
            sprintf(line, "MODE %s %s", Windows[windex].Name, parseptr);
         }
         else {
            strcpy(line, &c_ptr[1]);
         }
         break;
      }

      // Handling the /topic command.
      // Only allowed in Channel window. No setting topic allowed as of now.
      if (strncasecmp(c_ptr, "/topic", 6) == 0) {
         if (Windows[windex].Name[0] != '#') break;

         sprintf(line, "TOPIC %s", Windows[windex].Name);
         break;
      }

      // Handling the /blist command, to list the BAN list of channel.
      if (strncasecmp(c_ptr, "/blist", 6) == 0) {
         if (Windows[windex].Name[0] != '#') break;

         sprintf(line, "MODE %s +b", Windows[windex].Name);
         break;
      }

      // Handling the /list command to list the channels.
      if (strncasecmp(c_ptr, "/list", 5) == 0) {
         strcpy(line, &c_ptr[1]);
         break;
      }

      if (strncasecmp(c_ptr, "/part", 5) == 0) {
         // This is the /part command issued.
         // We process it only if its a Channel window.
         if (message_window) break;

         // See if the Channel Window is not belonging to CHANNEL_MAIN
         // or CHANNEL_CHAT or CHANNEL_MM
         if (strcasecmp(Windows[windex].Name, CHANNEL_MAIN) == 0) break;
         if (strcasecmp(Windows[windex].Name, CHANNEL_CHAT) == 0) break;
         if (strcasecmp(Windows[windex].Name, CHANNEL_MM) == 0) break;

         // Additional check which makes sure its a Channel Name.
         if (Windows[windex].Name[0] != '#') break;

         // We remove the entry from ChannelListUI for this WindowName.
         FXint selected_item;
         selected_item = ChannelListUI->findItem(Windows[windex].Name);
         if (selected_item >= 0) {
            ChannelListUI->removeItem(selected_item);
         }

         // Move focus to CHANNEL_MAIN entry = selection 0
         ChannelListUI->selectItem(0);

         // Lets send out the PART command.
         sprintf(line, "PART %s", Windows[windex].Name);

         IRCChannelList CL;
         CL = XGlobal->getIRC_CL();
         XGlobal->resetIRC_CL_Changed();
         CL.delChannel(Windows[windex].Name);
         COUT(CL.printDebug());

         XGlobal->putIRC_CL(CL);

         // Move this Current TAB back to the free channel tab list
         replaceWindowNameAtIndex(windex, UI_FREE_CHANNEL_NAME);

         // Clear the ScrollUI and NickListUI for this TAB we parted.
         FXint lastPos = Windows[windex].ScrollUI->getLength();
         Windows[windex].ScrollUI->removeText(0, lastPos);

         Windows[windex].NickListUI->clearItems();

         // Move focus to CHANNEL_MAIN = index 0.
         onChannelTabToDisplay(ChannelListUI, ID_SELECTCHANNELTABTODISPLAY, (void *) 0);

         // Write out the channel to the config file.
         Helper H;
         H.init(XGlobal);
         H.writeIRCChannelConfig();

         break;
      }

      if (strncasecmp(c_ptr, "/join ", 6) == 0) {
         // This is the /join command issued.
         // We process it only if its a Channel window.
         if (Windows[windex].Name[0] != '#') break;

         // Get the channel name, word 2. See if the UI is already created.
         // Key is word 3.
         LineP = c_ptr;
         parseptr = LineP.getWord(2);
         if (parseptr[0] != '#') break;

         // Do not allow manual joining of Sync Channel.
         // We can allow, but it gets a little complicated in the below
         // lines, as in Production build ChannelListUI does not have
         // an entry for this Channel, but there exist a channel TAB
         // with the same name. Basically, we will have to act differntly
         // if its production build vs Debug build. So to simplify
         // matters, just get out if its CHANNEL_MM
         if (strcasecmp(parseptr, CHANNEL_MM) == 0) break;

         strcpy(uiline, parseptr);
         parseptr = LineP.getWord(3);
         sprintf(line, "JOIN %s %s", uiline, parseptr);

         // Now lets ready up the UI. uiline = channel name as of now.
         int new_windex = getWindowIndex(uiline);
         if (new_windex == -1) {
            // Not yet formed, so form one.
            new_windex = getWindowIndex(UI_FREE_CHANNEL_NAME);
            if (new_windex == -1) {
               uiline[0] = '\0';
               line[0] = '\0';
               break; // No more TABs are free.
            }

            replaceWindowNameAtIndex(new_windex, uiline);

            // Update the label on this tab item to this new name.
            Windows[new_windex].TabUI->setText(uiline);

            ChannelListUI->appendItem(uiline);
         }
         // Make the item be selected (added or already existing)
         // new_windex = valid index in Windows.
         FXint item_num;
         item_num = ChannelListUI->findItem(Windows[new_windex].Name);
         if (item_num >= 0) {
            ChannelListUI->selectItem(item_num);
         }

         IRCChannelList CL;
         CL = XGlobal->getIRC_CL();
         XGlobal->resetIRC_CL_Changed();
         // Delete and add, in case this is a repeat /join
         CL.delChannel(uiline);
         CL.addChannel(uiline, (char *) parseptr);
         COUT(CL.printDebug());

         XGlobal->putIRC_CL(CL);

         // We now make the new TAB we just created the current TAB
         onChannelTabToDisplay(ChannelListUI, ID_SELECTCHANNELTABTODISPLAY, (void *) (item_num));

         // Write out the channel to the config file.
         Helper H;
         H.init(XGlobal);
         H.writeIRCChannelConfig();

         // blankout uiline, as we have just used it as a scratch.
         uiline[0] = '\0';
         break;
      }

      if (strncasecmp(c_ptr, "/me ", 4) == 0) {
         // This is a /me command
         if (message_window == false) {
            sprintf(line, "PRIVMSG %s :\001ACTION %s\001", to, &c_ptr[4]);
            sprintf(uiline, "%s 06* %s %s", Windows[windex].Name, Nick, &c_ptr[4]);
         }
         break;
      }

      // portcheckme should come before comparison of /portcheck
      if (strncasecmp(c_ptr, "/portcheckme", 12) == 0) {
         // This is a /portcheckme Nick Command.
         LineP = c_ptr;
         parseptr = LineP.getWord(2);
         // In all kinds of windows, if 2nd word exists, use that as nick.
         // In messages window if 2nd word doesnt exist then use "to" as nick.
         // In other windows, if 2nd word doesnt exist do nothing.
         if ( (strlen(parseptr) == 0) && (message_window == false) ) {
            // 2nd word doesnt exist and we are not the message window.
            break;
         }

         if (strlen(parseptr) == 0) {
            parseptr = to;
         }
         // Now we have parseptr always pointing corectly.
         
         // See if we are requesting ourselves - not allowed.
         if (strcasecmp(parseptr, Nick) == 0) {
            sprintf(uiline, "%s * 06PORTCHECKME: Cannot do it to ourselves", Windows[windex].Name);
         }
         else {
            sprintf(line, "PRIVMSG %s :\001PORTCHECK %d\001", parseptr, DCCSERVER_PORT);
            sprintf(uiline, "%s * 06PORTCHECKME: Requesting %s to check our ports.", Windows[windex].Name, parseptr);
         }
         break;
      }

      // /portcheck should come after comparison of /portcheckme.
      if (strncasecmp(c_ptr, "/portcheck", 10) == 0) {
         // This is a /portcheck Nick Command.
         LineP = c_ptr;
         parseptr = LineP.getWord(2);
         if ( (parseptr == NULL) || (strlen(parseptr) == 0) ) {
            break;
         }

         // We just issue a USERHOST. Copy Nick in XChange::PortCheckNick
         // And let DCCThr worry about it.
         XGlobal->lock();
         delete [] XGlobal->PortCheckNick;
         XGlobal->PortCheckNick = new char[strlen(parseptr) + 1];
         strcpy(XGlobal->PortCheckNick, parseptr);
         delete [] XGlobal->PortCheckWindowName;
         XGlobal->PortCheckWindowName = new char[strlen(Windows[windex].Name) + 1];
         strcpy(XGlobal->PortCheckWindowName, Windows[windex].Name);
         XGlobal->unlock();

         // See if we are checking ourselves - not allowed.
         if (strcasecmp(parseptr, Nick) == 0) {
            sprintf(uiline, "%s * 06PORTCHECK: Cannot do it to ourselves", Windows[windex].Name);
         }
         else {
            sprintf(line, "USERHOST %s", parseptr);
            sprintf(uiline, "%s * 06PORTCHECK: Checking the Ports of %s.", Windows[windex].Name, parseptr);
         }
         break;
      }

      if (strncasecmp(c_ptr, "/dccallow ", 10) == 0) {
         // We issue the command as is skipping the first character.
         sprintf(line, "%s", &c_ptr[1]);

         uiline[0] = '\0';
         break;
      }

      if (strncasecmp(c_ptr, "/nick ", 6) == 0) {

         // Handle the change nick command.
         LineP = c_ptr;
         if (LineP.getWordCount() != 2) break;

         parseptr = LineP.getWord(2);
         sprintf(line, "NICK %s", parseptr);

         // Change it in XGlobal for Proposed Nick.
         XGlobal->putIRC_ProposedNick((char *) parseptr);

         // Save value in Config File. Not yet, as its a proposal.
         // H.init(XGlobal);
         // H.writeIRCConfigFile();

         break;
      }

      if (strncasecmp(c_ptr, "/op ", 4) == 0) {
      char op_command[3];
         // Process all the op related commands: op/deop/voice/devoice
         // Have to issue: MODE Channel operation Nick
         // Channel = CHANNEL_MAIN or CHANNEL_CHAT
         // operation = +o, -o, +v, -v, +b, -b
         // Windows[i].Name is the channel name.

         // op command not allowed in CHANNEL_MM
         if (strcasecmp(Windows[windex].Name, CHANNEL_MM) == 0) break;

         LineP = c_ptr;

         // Command should have exactly 3 words.
         if (LineP.getWordCount() != 3) break;

         parseptr = LineP.getWord(2);
         // parseptr now has the actual command.

         if (strcasecmp(parseptr, "+o") &&
             strcasecmp(parseptr, "-o") &&
             strcasecmp(parseptr, "+v") &&
             strcasecmp(parseptr, "-v") &&
             strcasecmp(parseptr, "+b") &&
             strcasecmp(parseptr, "-b") ) {
            break;
         }
         strcpy(op_command, parseptr);

         // Lets get the nick, which is the third word
         parseptr = LineP.getWord(3);

         sprintf(line, "MODE %s %s %s", Windows[windex].Name, op_command, parseptr);
         break;
      }

      if (strncasecmp(c_ptr, "/kick ", 6) == 0) {
         char *to_nick;

         // kick command not allowed in CHANNEL_MM
         if (strcasecmp(Windows[windex].Name, CHANNEL_MM) == 0) break;

         // We translate command to "KICK channel nick :Reason"
         // nick = 2nd param
         // reason = 3rd to the last param if it exists.
         LineP = c_ptr;
         if (LineP.getWordCount() < 2) break;

         // Lets get to_nick
         parseptr = LineP.getWord(2);
         to_nick = new char[strlen(parseptr) + 1];
         strcpy(to_nick, parseptr);
 
         if (LineP.getWordCount() == 2) {
            // No reason given. Put default reason in uiline.
            strcpy(uiline, "Chumma");
         }
         else {
            // reason = 3rd to the last param.
            parseptr = LineP.getWordRange(3, 0);
            strcpy(uiline, parseptr);
         }

         sprintf(line, "KICK %s %s :%s", Windows[windex].Name, to_nick, uiline);
         uiline[0] ='\0';
         delete [] to_nick;
         break;
      }

      if (strncasecmp(c_ptr, "/whois ", 7) == 0) {
         // We issue the command as is, skipping the first character.
         sprintf(line, "%s", &c_ptr[1]);

         uiline[0] = '\0';
         break;
      }

      if (strncasecmp(c_ptr, "/msg ", 4) == 0) {
      char *to_nick;
         // This is a /msg command. Handle just like mirc.
         LineP = c_ptr;
         parseptr = LineP.getWord(2);
         if (strlen(parseptr) == 0) break;

         to_nick = new char[strlen(parseptr) + 1];
         strcpy(to_nick, parseptr);
         parseptr = LineP.getWordRange(3, 0);

         sprintf(line, "PRIVMSG %s :%s", to_nick, parseptr);
         sprintf(uiline, "Messages <TO %s> %s", to_nick, parseptr);
         delete [] to_nick;
         break;
      }

      if (strncasecmp(c_ptr, "/ctcp ", 6) == 0) {
      char *to_nick;
         // This is a /ctcp command.
         LineP = c_ptr;
         parseptr = LineP.getWord(2);
         if (strlen(parseptr) == 0) break;

         to_nick = new char[strlen(parseptr) + 1];
         strcpy(to_nick, parseptr);
         parseptr = LineP.getWordRange(3, 0);

         sprintf(line, "PRIVMSG %s :\001%s\001", to_nick, parseptr);
         // Show it in the UI only if its not a known CTCP like
         // PING, TIME, VERSION, CLIENTINFO.
         if ( (strncasecmp(parseptr, "PING", 4) == 0) ||
              (strcasecmp(parseptr, "TIME") == 0) ||
              (strcasecmp(parseptr, "VERSION") == 0) ||
              (strcasecmp(parseptr, "CLIENTINFO") == 0)
            ) {
         }
         else {
            // Allow this ctcp to be issue only if its not to ourselves.
            if (strcasecmp(to_nick, Nick) == 0) {
               // Cant allow this unknown CTCP to ourselves as it could be
               // a trigger access to ourselves which can result in us
               // being marked non firewalled by error.
               line[0] = '\0';
               uiline[0] = '\0';
            }
            else {
               sprintf(uiline, "%s * 07CTCP issued to %s: %s", Windows[windex].Name, to_nick, parseptr);
            }
         }
        
         delete [] to_nick;
         break;
      }

      // To try to connect to a swarm server. Manually done as of now.
      if (strncasecmp(c_ptr, "/swarm ", 7) == 0) {
         // This is /swarm Nick FileName
         // If FileName exists in serving Folder, we first go for that.
         // Next if it exists in Partial foler, we go for that.
         // All above failed, create file in Partial folder.
         LineP = c_ptr;
         if (LineP.getWordCount() != 3) break;
         parseptr = LineP.getWord(2);
         char *to_nick = new char[strlen(parseptr) + 1];
         strcpy(to_nick, parseptr);
         parseptr = LineP.getWordRange(3, 0);

         Helper H;
         H.init(XGlobal);
         int SwarmIndex = H.getSwarmIndexGivenFileName(parseptr);

         unsigned long to_nick_ip = XGlobal->NickList.getNickIP(to_nick);
         unsigned long my_ip = XGlobal->getIRC_IP(NULL);
         XGlobal->resetIRC_IP_Changed();

         // We only add if that Nick is not our own Nick or our own IP.
         if ( (strcasecmp(to_nick, Nick) == 0) ||
              (my_ip == to_nick_ip) ) {
            COUT(cout << "/swarm: Not adding in YetToTryNodes, Nick: " << to_nick << " as its our own nick or has our own ip." << endl;)
            delete [] to_nick;
            break;
         }
 
         if (SwarmIndex != -1) {
            // We add this Nick to the YetToTryNodes List of the Swarm.

            XGlobal->Swarm[SwarmIndex].YetToTryNodes.addToSwarmNodeNickIPState(to_nick, to_nick_ip, SWARM_NODE_NOT_TRIED);
            delete [] to_nick;
            break;
         }

         // We are here, Swarm doesnt already exist for the File.
         // If SwarmIndex is not set, get a free Swarm slot.
         // parseptr points to FileName.
         for (int i = 0; i < SWARM_MAX_FILES; i++) {
            if (XGlobal->Swarm[i].isBeingUsed() == false) {
               // This is the free slot. If File is present in the Serving Dir
               // we reuse that same file. (Incomplete or seeding)
               FilesDetail *FD = XGlobal->MyFilesDB.getFilesDetailListMatchingFileName((char *) parseptr);
               if (FD) {
                  // In case of Linux, correct the filename as it exists.
                  XGlobal->lock();
                  XGlobal->Swarm[i].setSwarmClient(XGlobal->ServingDir[FD->ServingDirIndex], FD->FileName);
                  XGlobal->unlock();
               }
               else {
                  FD = XGlobal->MyPartialFilesDB.getFilesDetailListMatchingFileName((char *) parseptr);
                  if (FD) {
                     // In case of Linux, correct the filename as it exists.
                     XGlobal->lock();
                     XGlobal->Swarm[i].setSwarmClient(XGlobal->PartialDir, FD->FileName);
                     XGlobal->unlock();
                  }
                  else {
                     // Doesnt exist in Serving/Partial.
                     // Dump it in Partial with given whatever name.
                     XGlobal->lock();
                     XGlobal->Swarm[i].setSwarmClient(XGlobal->PartialDir, (char *) parseptr);
                     XGlobal->unlock();
                  }
               }

               H.generateMyPartialFilesDB();
               XGlobal->Swarm[i].YetToTryNodes.addToSwarmNodeNickIPState(to_nick, to_nick_ip, SWARM_NODE_NOT_TRIED);
               if (FD) {
                  XGlobal->MyFilesDB.freeFilesDetailList(FD);
               }
               break;
            }
         }

         delete [] to_nick;
         break;
      }

      // To start a swarm, currently the server manually starts a swarm.
      // by issuing a /swarmstart command and choosing the file.
      if (strncasecmp(c_ptr, "/swarmstart", 11) == 0) {
         static char StreamDir[MAX_PATH];
         // We present user with the File Selection dialog.
         FXFileDialog streamFile(this, "Select File to Swarm");
         if (StreamDir[0] == '\0') {
            XGlobal->lock();
            strcpy(StreamDir, XGlobal->ServingDir[0]);
            XGlobal->unlock();
         }
         streamFile.setDirectory(StreamDir);
         streamFile.setPatternList(FileDialogPatterns);
         if (streamFile.execute()) {
            FXString fname;

            // Save Directory for next invocation.
            fname = streamFile.getDirectory();
            strcpy(StreamDir, fname.text());

            fname = streamFile.getFilename();
            char tmp_file[MAX_PATH];
            strcpy(tmp_file, fname.text());
            char *just_fn = getFileName(tmp_file);
            COUT(cout << "File to Swarm: " << just_fn << " Directory: " << StreamDir << endl;)

            // Initialize XGlobal->Swarm[0] as Server
            for (int i = 0; i < SWARM_MAX_FILES; i++) {
               if (XGlobal->Swarm[i].isBeingUsed() == false) {
                  // Got a free slot. So lets start the swarm
                  XGlobal->Swarm[i].setSwarmServer(StreamDir, just_fn);
                  break;
               }
            }
         }
         break;
      }

      if (strncasecmp(c_ptr, "/dcc ", 5) == 0) {
      bool dccsend;
      bool dccqueue;
      bool dccchat;
      int dccqueue_num;
      unsigned long longip;
      char DotIP[20];
      char str_sendq[6];
      TCPConnect *T;
      static char DCCSendDir[MAX_PATH];

         LineP = c_ptr;
         parseptr = LineP.getWord(2);
         if (strcasecmp(parseptr, "queue") == 0) {
            if (LineP.getWordCount() != 4) {
               sprintf(uiline, "%s * 06Syntax error: /dcc queue <nick> <queue num>", Windows[windex].Name);
               break;
            }
            strcpy(str_sendq, "QUEUE");
            dccqueue = true;
            dccsend = false;
            dccchat = false;
            parseptr = LineP.getWord(4);
            dccqueue_num = strtoul(parseptr, NULL, 10);
         }
         else if (strcasecmp(parseptr, "send") == 0) {
            if (LineP.getWordCount() != 3) {
               sprintf(uiline, "%s * 06Syntax error: /dcc send <nick>", Windows[windex].Name);
               break;
            }
            strcpy(str_sendq, "SEND");
            dccsend = true;
            dccqueue = false;
            dccchat = false;
         }
         else if (strcasecmp(parseptr, "chat") == 0) {
            if (LineP.getWordCount() != 3) {
               sprintf(uiline, "%s * 06Syntax error: /dcc chat <nick>", Windows[windex].Name);
               break;
            }
            strcpy(str_sendq, "CHAT");
            dccsend = false;
            dccqueue = false;
            dccchat = true;
         }

         // This is a /dcc send or queue or chat command. 
         // Syntax = /dcc send nick or /dcc queue nick qnum or /dcc chat nick
         // If all is well, we present a FileList dialog box to let user
         // select the file to be send. (for send/queue)
         LineP = c_ptr;
         // We have word 3 which is the nick to be sent to.
         // We check if we need to get his IP.
         parseptr = LineP.getWord(3);
         if (strcasecmp(parseptr, Nick) == 0) {
            // Do not allow sending/queueing to ourselves.
            sprintf(uiline, "%s * 06Cannot DCC %s to ourselves!", Windows[windex].Name, str_sendq);
            break;
         }

         if (dccchat ||
             (XGlobal->NickList.getNickIP((char *) parseptr) == IRCNICKIP_UNKNOWN)
            ) {
            // We need to get the IP of this guy. Lets issue a USERHOST
            sprintf(line, "USERHOST %s", parseptr);
            XGlobal->IRC_ToServerNow.putLine(line);
            line[0] = '\0';
            // We will get a IC_USERHOST response back which will set the IP.

            // If Chat, we just mark DCCChatNick with the To Chat Nick
            // The UserHost response will trigger the Chat from DCCThr.
            if (dccchat) {
               delete [] XGlobal->DCCChatNick;
               XGlobal->DCCChatNick = new char[strlen(parseptr) + 1];
               strcpy(XGlobal->DCCChatNick, parseptr);
               break;
            }
         }

         // We present user with the File Selection dialog.
         sprintf(line, "DCC %s File to %s", str_sendq, parseptr);
         FXFileDialog dccFile(this, line);
         line[0] = '\0';
         if (DCCSendDir[0] == '\0') {
            XGlobal->lock();
            strcpy(DCCSendDir, XGlobal->ServingDir[0]);
            XGlobal->unlock();
         }
         dccFile.setDirectory(DCCSendDir);
         dccFile.setPatternList(FileDialogPatterns);
         if (dccFile.execute()) {
            FXString fname;

            // Save Directory for next invocation.
            fname = dccFile.getDirectory();
            strcpy(DCCSendDir, fname.text());

            fname = dccFile.getFilename();
            COUT(cout << "File to DCC " << str_sendq << ": " << fname.text() << endl;)

            // Here we make sure we do not send a file which has .exe in it.
            if (strcasestr((char *) fname.text(), ".exe")) {
               sprintf(uiline, "%s * 06Cannot send or queue executable files (%s), to curb down on virus/trojan propagation.", Windows[windex].Name, fname.text());
               break;
            }

            if (XGlobal->NickList.getNickIP((char *) parseptr) == IRCNICKIP_UNKNOWN) {
               // We didnt get its IP.
               sprintf(uiline, "%s * 06IP of %s unknown. Reasons: Nick not in known channel, Nick host is masked, Nick's host could not be resolved, An issued USERHOST call hasnt replied, in which case try after sometime.", Windows[windex].Name, parseptr);
               break;
            }
            // Lets get longip and DotIP initialised.
            longip = XGlobal->NickList.getNickIP((char *) parseptr);
            T->getDottedIpAddressFromLong(longip, DotIP);

            // Here we act differently for dcc queue than dcc send.
            // The file to be sent should be from the Serving Directory.
            // whereas for dcc send it can be anywhere.

            char *fullfn, *fn;
            FilesDetail *FD;

            fullfn = new char [strlen(fname.text()) + 1];
            strcpy(fullfn, fname.text());
            // We keep filenames without the directory.
            // fullfn contains full path. Lets just get the filename.
         
            if (dccsend) {
               // We pump the send out. (Add it in pos 1 of QueuesInProgress)
               // No need of getting File Size as its filled up in dccSend
               // Well we need to get File Size to decide which Q to put it in
               FD = new FilesDetail;
               XGlobal->QueuesInProgress.initFilesDetail(FD);
               getFileSize(fullfn, &(FD->FileSize));
               FD->Nick = new char[strlen(parseptr) + 1];
               strcpy(FD->Nick, parseptr);
               fn = getFileName(fullfn);
               FD->FileName = new char[strlen(fn) + 1];
               strcpy(FD->FileName, fn);
               // Lets get the directory name.
               getDirName(fullfn);
               FD->DirName = new char[strlen(fullfn) + 1];
               strcpy(FD->DirName, fullfn);
               FD->DottedIP = new char[strlen(DotIP) + 1];
               strcpy(FD->DottedIP, DotIP);
               // Manual send indicator for DCC Send.
               FD->ManualSend = MANUALSEND_DCCSEND;
               // Manual Sends are full sends.
               FD->DownloadState = DOWNLOADSTATE_SERVING;
               sprintf(uiline, "%s * 06Attempting to send %s to %s", Windows[0].Name, FD->FileName, FD->Nick);
               // Manual sends are always at index 1
               size_t tempMaxSmallFileSize;
               XGlobal->lock();
               tempMaxSmallFileSize = XGlobal->FServSmallFileSize;
               XGlobal->unlock();
               if (FD->FileSize < tempMaxSmallFileSize) {
                  XGlobal->SmallQueuesInProgress.addFilesDetailAtIndex(FD, 1);
               }
               else {
                  XGlobal->QueuesInProgress.addFilesDetailAtIndex(FD, 1);
               }
               FD = NULL; // So we dont use it.
            }
            else {
               // This is a /dcc queue. file should be present in serving dir.
               size_t file_size;
               if (getFileSize(fullfn, &file_size) == false) {
                  sprintf(uiline, "%s * 06Could not obtain File Size of File to add to the DCC Queue.", Windows[0].Name);
                  delete [] fullfn;
                  break;
               }
               fn = getFileName(fullfn);
               FD = XGlobal->MyFilesDB.getFilesDetailListMatchingFileAndSize(fn, file_size);
               if (FD == NULL) {
                  sprintf(uiline, "%s * 06Only Files present in your Serving Folders can be Queued for DCC.", Windows[0].Name);
                  delete [] fullfn;
                  break;
               }
               // So we are here, we got an FD hit which exists in MyFilesDB.
               // We hope there is only one file in this list, else confusion.
               delete [] FD->Nick;
               FD->Nick = new char[strlen(parseptr) + 1];
               strcpy(FD->Nick, parseptr);
               delete [] FD->DottedIP;
               FD->DottedIP = new char[strlen(DotIP) + 1];
               strcpy(FD->DottedIP, DotIP);
               sprintf(uiline, "%s * 06Attempting to queue %s to %s at Queue Position %d", Windows[0].Name, FD->FileName, FD->Nick, dccqueue_num);
               size_t tempMaxSmallFileSize;
               XGlobal->lock();
               tempMaxSmallFileSize = XGlobal->FServSmallFileSize;
               XGlobal->unlock();
               if (FD->FileSize < tempMaxSmallFileSize) {
                  XGlobal->SmallQueuesInProgress.addFilesDetailAtIndex(FD, dccqueue_num);
               }
               else {
                  XGlobal->QueuesInProgress.addFilesDetailAtIndex(FD, dccqueue_num);
               }
               FD = NULL; // So we dont use it.
            }
            delete [] fullfn;
         }
         break;
      }

      if (strncasecmp(c_ptr, "/upnp ", 6) == 0) {
         LineP = c_ptr;
         parseptr = LineP.getWord(2);
         if (strcasecmp(parseptr, "SEARCH") == 0) {
            sprintf(line, "%s SEARCH", Windows[windex].Name);
            XGlobal->UI_ToUpnp.putLine(line);
            line[0] = '\0';
            break;
         }

         if (strcasecmp(parseptr, "GETEXTIP") == 0) {
            sprintf(line, "%s GET_EXTERNAL_IP", Windows[windex].Name);
            XGlobal->UI_ToUpnp.putLine(line);
            line[0] = '\0';
            break;
         }

         if (strcasecmp(parseptr, "ADDPORT") == 0) {
            parseptr = LineP.getWord(3);
            // int tmpport = atoi(parseptr);
            int tmpport = (int) strtol(parseptr, NULL, 10);
            if ( (tmpport <= 0) || (tmpport > 65535) ) {
               sprintf(uiline, "%s * 04UPNP: Error -> Port should be between 1 and 65535.", Windows[windex].Name);
               break;
            }
            parseptr = LineP.getWordRange(4, 0);
            if (strlen(parseptr) == 0) {
               sprintf(uiline, "%s * 04UPNP: Error -> Description is missing.", Windows[windex].Name);
               break;
            }
            sprintf(line, "%s ADD_PORT_MAPPING %d %s", Windows[windex].Name, tmpport, parseptr);
            XGlobal->UI_ToUpnp.putLine(line);
            line[0] = '\0';
            break;
         }

         if (strcasecmp(parseptr, "DELPORT") == 0) {
            parseptr = LineP.getWord(3);
            // int tempint = atoi(parseptr);
            int tempint = (int) strtol(parseptr, NULL, 10);
            if (tempint == DCCSERVER_PORT) {
               sprintf(uiline, "%s * 04UPNP: Error -> Will not allow deleting port mapping for 8124", Windows[windex].Name);
               break;
            }
            sprintf(line, "%s DEL_PORT_MAPPING %s", Windows[windex].Name, parseptr);
            XGlobal->UI_ToUpnp.putLine(line);
            line[0] = '\0';
            break;
         }

         if (strcasecmp(parseptr, "GETMAPPINGS") == 0) {
            sprintf(line, "%s GET_CURRENT_PORT_MAPPINGS", Windows[windex].Name);
            XGlobal->UI_ToUpnp.putLine(line);
            line[0] = '\0';
            break;
         }

         // We give syntax usage:
         sprintf(uiline, "%s * 09UPNP: Syntax: /upnp <search|getextip|getmappings|addport|delport>", Windows[windex].Name);
         break;
      }

      // FServ commands.
      if (strncasecmp(c_ptr, "/fserv ", 7) == 0) {
      long sq_value;
      bool show_syntax = true;

         LineP = c_ptr;
         parseptr = LineP.getWord(2);
         if (strcasecmp(parseptr, "SENDS") == 0) {
            parseptr = LineP.getWord(3);
            if (strcasecmp(parseptr, "OVERALL") == 0) {
               parseptr = LineP.getWord(4);
               sq_value = strtoul(parseptr, NULL, 10);
               if (sq_value < FSERV_MIN_SENDS_OVERALL) {
                  sq_value = FSERV_MIN_SENDS_OVERALL;
               }
               XGlobal->lock();
               XGlobal->FServSendsOverall = sq_value;
               XGlobal->unlock();
               show_syntax = false;
            }
            else if (strcasecmp(parseptr, "EACH") == 0) {
               parseptr = LineP.getWord(4);
               sq_value = strtoul(parseptr, NULL, 10);
               if (sq_value < FSERV_MIN_SENDS_USER) {
                  sq_value = FSERV_MIN_SENDS_USER;
               }
               XGlobal->lock();
               XGlobal->FServSendsUser = sq_value;
               XGlobal->unlock();
               show_syntax = false;
            }
         }
         else if (strcasecmp(parseptr, "QUEUES") == 0) {
            parseptr = LineP.getWord(3);
            if (strcasecmp(parseptr, "OVERALL") == 0) {
               parseptr = LineP.getWord(4);
               sq_value = strtoul(parseptr, NULL, 10);
               if (sq_value < FSERV_MIN_QUEUES_OVERALL) {
                  sq_value = FSERV_MIN_QUEUES_OVERALL;
               }
               XGlobal->lock();
               XGlobal->FServQueuesOverall = sq_value;
               XGlobal->unlock();
               show_syntax = false;
            }
            else if (strcasecmp(parseptr, "EACH") == 0) {
               parseptr = LineP.getWord(4);
               sq_value = strtoul(parseptr, NULL, 10);
               if (sq_value < FSERV_MIN_QUEUES_USER) {
                  sq_value = FSERV_MIN_QUEUES_USER;
               }
               XGlobal->lock(); 
               XGlobal->FServQueuesUser = sq_value;
               XGlobal->unlock();
               show_syntax = false;
            }
         }
         else if (strcasecmp(parseptr, "PRINT") == 0) {
            show_syntax = false;
         }
         else if (strcasecmp(parseptr, "SMALLFILESIZE") == 0) {
            parseptr = LineP.getWord(3);
            sq_value = strtoul(parseptr, NULL, 10);
            if ( (sq_value <= 0) ||
                 (sq_value > FSERV_SMALL_FILE_MAX_SIZE) ) {
               sq_value = FSERV_SMALL_FILE_DEFAULT_SIZE;
            }
            XGlobal->lock();
            XGlobal->FServSmallFileSize = sq_value;
            XGlobal->unlock();
            show_syntax = false;
         }

         if (show_syntax) {
            sprintf(uiline, "%s * 09FSERV: Syntax: /fserv <sends|queues|smallfilesize|print> <overall|each> <value>", Windows[windex].Name);
         }
         else {
            Helper H;
            XGlobal->lock();
            // Relative values sanity check.
            // overallsends > usersends, else usersends = overallsends - 1.
            // userq >= usersends, else userq = usersends.
            // overallq > userq, else userq = overallq - 1.
            if (XGlobal->FServSendsOverall <= XGlobal->FServSendsUser) {
               XGlobal->FServSendsUser = XGlobal->FServSendsOverall - 1;
            }
            if (XGlobal->FServQueuesUser < XGlobal->FServSendsUser) {
               XGlobal->FServQueuesUser = XGlobal->FServSendsUser;
            }
            if (XGlobal->FServQueuesOverall <= XGlobal->FServQueuesUser) {
               XGlobal->FServQueuesUser = XGlobal->FServQueuesOverall - 1;
            }
 
            sprintf(uiline, "%s * 09FSERV: Sends -> Each: %lu Overall: %lu | Queues -> Each: %lu Overall: %lu | SmallFileSize: %lu", Windows[windex].Name, XGlobal->FServSendsUser, XGlobal->FServSendsOverall, XGlobal->FServQueuesUser, XGlobal->FServQueuesOverall, XGlobal->FServSmallFileSize);
            XGlobal->unlock();
            // Save it in config file too.
            H.init(XGlobal);
            H.writeFServParamsConfig();
         }
         break;
      }

      // CAP commands.
      if (strncasecmp(c_ptr, "/cap ", 5) == 0) {
      size_t cap_value;
      bool show_syntax = true;
         LineP = c_ptr;
         parseptr = LineP.getWord(2);
         if (strcasecmp(parseptr, "UPLOAD") == 0) {
            parseptr = LineP.getWord(3);
            if (strcasecmp(parseptr, "EACH") == 0) {
               parseptr = LineP.getWord(4);
               cap_value = strtoul(parseptr, NULL, 10);
               if ( (cap_value == 0) || (cap_value >= MIN_CAP_VALUE_BPS) ) {
                  XGlobal->lock();
                  XGlobal->PerTransferMaxUploadBPS = cap_value;
                  if ( (XGlobal->OverallMaxUploadBPS != 0) &&
                       (cap_value > XGlobal->OverallMaxUploadBPS) ) {
                     XGlobal->PerTransferMaxUploadBPS = XGlobal->OverallMaxUploadBPS;
                  }
                  XGlobal->unlock();
               }
               show_syntax = false;
            }
            else if (strcasecmp(parseptr, "OVERALLMAX") == 0) {
               parseptr = LineP.getWord(4);
               cap_value = strtoul(parseptr, NULL, 10);
               if ( (cap_value == 0) || (cap_value >= MIN_CAP_VALUE_BPS) ) {
                  XGlobal->lock();
                  XGlobal->OverallMaxUploadBPS = cap_value;
                  if ( (cap_value != 0) &&
                       (XGlobal->PerTransferMaxUploadBPS > cap_value) ) {
                     XGlobal->PerTransferMaxUploadBPS = cap_value;
                  }
                  XGlobal->unlock();
               }
               show_syntax = false;
            }
            else if (strcasecmp(parseptr, "OVERALLMIN") == 0) {
               parseptr = LineP.getWord(4);
               cap_value = strtoul(parseptr, NULL, 10);
               XGlobal->lock();
               if (cap_value == 0) {
                  XGlobal->OverallMinUploadBPS = cap_value;
               }
               else if (cap_value > MIN_CAP_VALUE_BPS) {
                  if (XGlobal->OverallMaxUploadBPS == 0) {
                     XGlobal->OverallMinUploadBPS = cap_value;
                  }
                  else if (cap_value < XGlobal->OverallMaxUploadBPS) {
                     XGlobal->OverallMinUploadBPS = cap_value;
                  }
               }
               XGlobal->unlock();
               show_syntax = false;
            }
         }
         else if (strcasecmp(parseptr, "DOWNLOAD") == 0) {
            parseptr = LineP.getWord(3);
            if (strcasecmp(parseptr, "EACH") == 0) {
               parseptr = LineP.getWord(4);
               cap_value = strtoul(parseptr, NULL, 10);
               if ( (cap_value == 0) || (cap_value >= MIN_CAP_VALUE_BPS) ) {
                  XGlobal->lock();
                  XGlobal->PerTransferMaxDownloadBPS = cap_value;
                  if ( (XGlobal->OverallMaxDownloadBPS != 0) &&
                       (cap_value > XGlobal->OverallMaxDownloadBPS) ) {
                     XGlobal->PerTransferMaxDownloadBPS = XGlobal->OverallMaxDownloadBPS;
                  }
                  XGlobal->unlock();
               }
               show_syntax = false;
            }
            else if (strcasecmp(parseptr, "OVERALL") == 0) {
               parseptr = LineP.getWord(4);
               cap_value = strtoul(parseptr, NULL, 10);
               if ( (cap_value == 0) || (cap_value >= MIN_CAP_VALUE_BPS) ) {
                  XGlobal->lock();
                  XGlobal->OverallMaxDownloadBPS = cap_value;
                  if ( (cap_value != 0) &&
                       (XGlobal->PerTransferMaxDownloadBPS > cap_value) ) {
                     XGlobal->PerTransferMaxDownloadBPS = cap_value;
                  }
                  XGlobal->unlock();
               }
               show_syntax = false;
            }
         }
         else if (strcasecmp(parseptr, "PRINT") == 0) {
            show_syntax = false;
         }

         if (show_syntax) {
            sprintf(uiline, "%s * 09CAP: Syntax: /cap <upload|download|print> <each|overallmax|overallmin> <value>", Windows[windex].Name);
         }
         else {
         Helper H;

            H.init(XGlobal);

            // Now sanitize the CAP values based on the record set.
            H.updateRecordDownloadAndAdjustUploadCaps(0, true);

            XGlobal->lock();
            sprintf(uiline, "%s * 09CAP: Download -> Each: %lu Overall: %lu | Upload -> Each: %lu OverallMax: %lu OverallMin: %lu", Windows[windex].Name, XGlobal->PerTransferMaxDownloadBPS, XGlobal->OverallMaxDownloadBPS, XGlobal->PerTransferMaxUploadBPS, XGlobal->OverallMaxUploadBPS, XGlobal->OverallMinUploadBPS);
            XGlobal->unlock();

            // Save in config file.
            H.writeCapConfig();
         }
         break;
      }

   } while (false);

   if (line[0] != '\0') {
      XGlobal->IRC_ToServerNow.putLine(line);
   }

// Only one thread should access UI. So Q it to the toUI thread.
   if (uiline[0] != '\0') {
      XGlobal->IRC_ToUI.putLine(uiline);
   }

// Clear the widget.
   if (Windows[windex].InputUI) {
      Windows[windex].InputUI->setText("");
   }
   delete [] line;
   delete [] uiline;

   return(1);
}

// File Search helper function to check on the File Type.
// FileType is one of: FILESEARCH_COMBOBOX_MPG, FILESEARCH_COMBOBOX_AVI
// FILESEARCH_COMBOBOX_WMV, FILESEARCH_COMBOBOX_RM, FILESEARCH_COMBOBOX_3GP
// FILESEARCH_COMBOBOX_IMAGES, FILESEARCH_COMBOBOX_ANY
bool TabBookWindow::doFileSearch_FileTypeCheck(char *FileName, char *FileType) {

   TRACE(); // Can remove trace later to speed up the search.

   if (strcasecmp(FileType, FILESEARCH_COMBOBOX_ANY) == 0) {
      return(true);
   }
   else if (strcasecmp(FileType, FILESEARCH_COMBOBOX_MPG) == 0) {
      // MPG files are .mpg, .mpeg, .dat
      if ( strcasestr(FileName, ".mpg") ||
           strcasestr(FileName, ".mpeg") ||
           strcasestr(FileName, ".dat") ) {
         return(true);
      }
      return(false);
   }
   else if (strcasecmp(FileType, FILESEARCH_COMBOBOX_AVI) == 0) {
      // AVI files are .avi
      if ( strcasestr(FileName, ".avi") ) {
         return(true);
      }
      return(false);
   }
   else if (strcasecmp(FileType, FILESEARCH_COMBOBOX_WMV) == 0) {
      // WMV files are .wmv
      if (strcasestr(FileName, ".wmv") ) {
         return(true);
      }
      return(false);
   }
   else if (strcasecmp(FileType, FILESEARCH_COMBOBOX_RM) == 0) {
      // RM Files are .rm/.rmvb/.rma/etc
      if (strcasestr(FileName, ".rm") ) {
         return(true);
      }
      return(false);
   }
   else if (strcasecmp(FileType, FILESEARCH_COMBOBOX_3GP) == 0) {
      // 3gp Files are .3gp
      if (strcasestr(FileName, ".3gp") ) {
         return(true);
      }
      return(false);
   }
   else if (strcasecmp(FileType, FILESEARCH_COMBOBOX_IMAGES) == 0) {
      // Image Files are .jpg/.jpeg/.bmp/.png/.gif/.tif
      if ( strcasestr(FileName, ".jpg") ||
           strcasestr(FileName, ".jpeg") ||
           strcasestr(FileName, ".bmp") ||
           strcasestr(FileName, ".png") ||
           strcasestr(FileName, ".gif") ||
           strcasestr(FileName, ".tif") ) {
         return(true);
      }
      return(false);
   }

   return(false);
}

// The main File Search engine.
// It gleans the correct options from the UI inputs in FileSearch tab
// and does the appropriate search.
void TabBookWindow::doFileSearch() {

// Below are the search criteria variables.
char *FileNameContains;
char FileType[16];
char *DirNameContains;
char *NickNameContains;
size_t FileSizeGT;
size_t FileSizeLT;
size_t FileSizeMultiplier;
bool OpenSends;
bool OpenQueues;
char Firewalled; // 'Y', 'N', 'A'
char Partial;    // 'Y', 'N', 'A'
char Client;     // 'M', 'S', 'I', 'A'    A=> Any

// Below is used to set the search criteria variables.
FXString TempStr;

// Useful windex of TAB_FILESEARCH
int windex;

// Used to remove the arrow marks in the headers
FXHeader *Header;

// Search related UI updates.
int TotalFiles = 0;
int progressincrement = 0;
double TotalBytes = 0.0;

// Actial Search related FDs
FilesDetail *Scan, *FD, *TempFD, *TailFD;

   TRACE();

   // Lets start setting up the search criteria.
   windex = getWindowIndex(TAB_FILESEARCH);

   // Setup FileNameContains
   TempStr = Windows[windex].InputUI->getText();
   if (TempStr.length()) {
      FileNameContains = new char[TempStr.length() + 1];
      strcpy(FileNameContains, TempStr.text());
   }
   else {
      FileNameContains = NULL;
   }

   // Setup the File Type.
   TempStr = FileSearchFileTypeComboBox->getText();
   strcpy(FileType, TempStr.text());
   // This will be one of: FILESEARCH_COMBOBOX_MPG, FILESEARCH_COMBOBOX_AVI
   // FILESEARCH_COMBOBOX_WMV, FILESEARCH_COMBOBOX_RM, FILESEARCH_COMBOBOX_3GP
   // FILESEARCH_COMBOBOX_IMAGES, FILESEARCH_COMBOBOX_ANY

   // Setup DirNameContains
   TempStr = FileSearchDirNameInputUI->getText();
   if (TempStr.length()) {
      DirNameContains = new char[TempStr.length() + 1];
      strcpy(DirNameContains, TempStr.text());
   }
   else {
      DirNameContains = NULL;
   }

   // Setup NickNameContains
   TempStr = FileSearchNickNameInputUI->getText();
   if (TempStr.length()) {
      NickNameContains = new char[TempStr.length() + 1];
      strcpy(NickNameContains, TempStr.text());
   }
   else {
      NickNameContains = NULL;
   }

   // Setup FileSizeGT
   TempStr = FileSearchFileSizeGTInputUI->getText();
   FileSizeGT = strtoul(TempStr.text(), NULL, 10);
   // Setup the FileSize multiplier.
   TempStr = FileSearchFileSizeGTComboBox->getText();
   if (TempStr == FILESEARCH_COMBOBOX_BYTES) {
      FileSizeMultiplier = 1;
   }
   else if (TempStr == FILESEARCH_COMBOBOX_KBYTES) {
      FileSizeMultiplier = 1024;
   }
   else if (TempStr == FILESEARCH_COMBOBOX_MBYTES) {
      FileSizeMultiplier = 1024 * 1024;
   }
   else {
      FileSizeMultiplier = 1024 * 1024 * 1024;
   }
   FileSizeGT = FileSizeGT * FileSizeMultiplier;

   // Setup FileSizeLT
   TempStr = FileSearchFileSizeLTInputUI->getText();
   FileSizeLT = strtoul(TempStr.text(), NULL, 10);
   // Setup the FileSize multiplier.
   TempStr = FileSearchFileSizeLTComboBox->getText();
   if (TempStr == FILESEARCH_COMBOBOX_BYTES) {
      FileSizeMultiplier = 1;
   }
   else if (TempStr == FILESEARCH_COMBOBOX_KBYTES) {
      FileSizeMultiplier = 1024;
   }
   else if (TempStr == FILESEARCH_COMBOBOX_MBYTES) {
      FileSizeMultiplier = 1024 * 1024;
   }
   else {
      FileSizeMultiplier = 1024 * 1024 * 1024;
   }
   FileSizeLT = FileSizeLT * FileSizeMultiplier;

   // Setup OpenSends
   TempStr = FileSearchSendsComboBox->getText();
   if (TempStr == FILESEARCH_COMBOBOX_YES) {
      OpenSends = true;
   }
   else {
      OpenSends = false;
   }

   // Setup OpenQueues
   TempStr = FileSearchQueuesComboBox->getText();
   if (TempStr == FILESEARCH_COMBOBOX_YES) {
      OpenQueues = true;
   }
   else {
      OpenQueues = false;
   }

   // Setup Firewalled
   TempStr = FileSearchFirewalledComboBox->getText();
   if (TempStr == FILESEARCH_COMBOBOX_YES) {
      Firewalled = IRCNICKFW_YES;
   }
   else if (TempStr == FILESEARCH_COMBOBOX_NO) {
      Firewalled = IRCNICKFW_NO;
   }
   else {
      Firewalled = 'A';
   }

   // Setup Partial
   TempStr = FileSearchPartialComboBox->getText();
   if (TempStr == FILESEARCH_COMBOBOX_YES) {
      Partial = DOWNLOADSTATE_PARTIAL;
   }
   else if (TempStr == FILESEARCH_COMBOBOX_NO) {
      Partial = DOWNLOADSTATE_SERVING;
   }
   else {
      Partial = 'A';
   }

   // Setup Client
   TempStr = FileSearchClientComboBox->getText();
   if (TempStr == FILESEARCH_COMBOBOX_MM) {
      Client = IRCNICKCLIENT_MASALAMATE;
   }
   else if (TempStr == FILESEARCH_COMBOBOX_SYSRESET) {
      Client = IRCNICKCLIENT_SYSRESET;
   }
   else if (TempStr == FILESEARCH_COMBOBOX_IROFFER) {
      Client = IRCNICKCLIENT_IROFFER;
   }
   else {
      Client = 'A';
   }
   // All the variables are set.

   // Lets remove all arrows in the Header.
   Header = Windows[windex].SearchUI->getHeader();
   for (int i = 0; i < HEADER_FILESEARCH_COUNT; i++) {
      Header->setArrowDir(i, MAYBE);
   }

   // Free previous FileSearchFD, and NULL new one.
   XGlobal->FilesDB.freeFilesDetailList(FileSearchFD);
   FileSearchFD = NULL;
   TailFD = NULL;
   FileSearchTotalFD = 0;

   Windows[windex].SearchUI->clearItems();

   if (FileNameContains && (strcasecmp("/clear", FileNameContains)) == 0) {
      // Clear the Input UI.
      Windows[windex].InputUI->setText("");
      delete [] FileNameContains;
      delete [] DirNameContains;
      delete [] NickNameContains;
 
      return;
   }

   // Lets pop up the Progress Dialog initially at 2 %, as the FD
   // returned in next two lines takes a while.
   ProgressBar->setProgress(2);
   ProgressBar->setTotal(100);

   if (FileNameContains) {
      FD = XGlobal->FilesDB.searchFilesDetailList(FileNameContains, &TotalFiles);
   }
   else {
      FD = XGlobal->FilesDB.searchFilesDetailList("*", &TotalFiles);
   }
   ProgressBar->setTotal(TotalFiles);

   TotalFiles = 0;

   Scan = FD;
   while (Scan) {
      char SendStr[10], QueueStr[10];

      // Update the Progress Dialog.
      progressincrement++;
      if (progressincrement > 10) {
         ProgressBar->increment(10);
         progressincrement = 0;
      }

      if (strcasecmp(Scan->FileName, "TriggerTemplate") == 0) {
         // Skip the template FDs
         Scan = Scan->Next;
         continue;
      }
      // We are here => we have passed the FileNameContains criteria.

      if (doFileSearch_FileTypeCheck(Scan->FileName, FileType) == false) {
         // This doesnt satisy the FileType -> Skip
         Scan = Scan->Next;
         continue;
      }
      // We are here => we have passed the FileType criteria.

      if (DirNameContains) {
         if (Scan->DirName == NULL) {
            // This doesnt satisfy the DirNameContains criteria -> Skip
            Scan = Scan->Next;
            continue;
         }
         if (strcasestr(Scan->DirName, DirNameContains) == NULL) {
            // This doesnt satisfy the DirNameContains criteria -> Skip
            Scan = Scan->Next;
            continue;
         }
      }
      // We are here => we have passed the DirNameContains criteria.

      if (NickNameContains) {
         if (strcasestr(Scan->Nick, NickNameContains) == NULL) {
            // This doesnt satisfy the NickNameContains criteria -> Skip
            Scan = Scan->Next;
            continue;
         }
      }
      // We are here => we have passed the NickNameContains criteria.

      if (FileSizeGT) {
         if (Scan->FileSize < FileSizeGT) {
            // This doesnt satisfy the FileSizeGT criteria -> Skip
            Scan = Scan->Next;
            continue;
         }
      }
      // We are here => we have passed the FileSizeGT criteria.

      if (FileSizeLT) {
         if (Scan->FileSize > FileSizeLT) {
            // This doesnt satisfy the FileSizeLT criteria -> SKip
            Scan = Scan->Next;
            continue;
         }
      }
      // We are here => we have passed the FileSizeLT criteria.

      if (OpenSends) {
         if (Scan->TotalSends <= Scan->CurrentSends) {
            // Skip this.
            Scan = Scan->Next;
            continue;
         }
      }
      // We are here => we have passed the OpenSends criteria.

      if (OpenQueuesButton) {
         if (Scan->TotalQueues <= Scan->CurrentQueues) {
            // Skip this.
            Scan = Scan->Next;
            continue;
         }
      }
      // We are here => we have passed the OpenQueues criteria.

      if (Partial != 'A') {
         if (Scan->DownloadState != Partial) {
            // Skip this.
            Scan = Scan->Next;
            continue;
         }
      }
      // We are here => we have passed the Partial criteria.

      if (Firewalled != 'A') {
         char fw = XGlobal->NickList.getNickFirewall(Scan->Nick);
         // Consider MAYBE and NO as not firewalled.
         if ( (Firewalled == IRCNICKFW_NO) && (fw == IRCNICKFW_YES) ) {
            // We skip the firewalled nicks.
            Scan = Scan->Next;
            continue;
         }
         // Consider MAYBE and YES as firewalled.
         if ( (Firewalled == IRCNICKFW_YES) && (fw == IRCNICKFW_NO) ) {
            // We skip the non firewalled nicks.
            Scan = Scan->Next;
            continue;
         }
      }
      // We are here => we have passed the Firewall criteria.

      if (Client != 'A') {
         if (Client != Scan->ClientType) {
            // Skip this.
            Scan = Scan->Next;
            continue;
         }
      }
      // We are here => we have passed the Client criteria.

      // All criterias have been met ...

      appendItemInSearchUI(windex, TotalFiles + 1, Scan, false);
      TotalBytes = TotalBytes + Scan->FileSize;
      TotalFiles++;

      // Update Global variables - Save this entry (Scan) in FileSearchFD.
      TempFD = XGlobal->FilesDB.copyFilesDetail(Scan);
      if (FileSearchFD == NULL) {
         // Head Element.
         FileSearchFD = TempFD;
      }
      else {
         // Append it to the Tail and update TailFD.
         TailFD->Next = TempFD;
      }
      TailFD = TempFD;

      FileSearchTotalFD++;

      Scan = Scan->Next;
   }

   // Free up everything used above.
   XGlobal->FilesDB.freeFilesDetailList(FD);
   delete [] FileNameContains;
   delete [] DirNameContains;
   delete [] NickNameContains;

   // Change the Window Title.
   int overallcount;
   double overallfilesize;
   overallcount = XGlobal->FilesDB.getCountFileSize(NULL, &overallfilesize);

   // FileSearchTitle is allocated in class constructor.
   sprintf(FileSearchTitle, "%s %s %s - %s - File Search || Files: %d - %.2f GB > Total Files: %d - %.2f GB", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick, TotalFiles, TotalBytes / 1024 / 1024 / 1024, overallcount, overallfilesize / 1024 / 1024 / 1024);
   setTitle(FileSearchTitle);

   // Show 100 % => all is well.
   ProgressBar->setTotal(1);
   ProgressBar->setProgress(1);
}

// Append Item in the SearchUI TAB at index win_index as per Scan received.
// if MarkSelected = true, additionally mark it selected, current and visible.
void TabBookWindow::appendItemInSearchUI(int win_index, int file_index, FilesDetail *Scan, bool MarkSelected) {
char *IconDetail;
int alloc_len;
char fw_text[8]; // Unknown or Maybe or Off or On
char fw_state;
char client_text[9]; // Unknown, SysReset, MM, Iroffer
char EmptyDir[2];
char *DirName;
char SendStr[10];
char QueueStr[10];
char s_filesize[32];

   TRACE();

   // alloc_len to fit:
   // Scan->FileName, float, Scan->DirName, Scan->Nick, float = 12
   // SendStr, QueueStr, FWallStr, ClntString, Int = 8 + 8 + 10 + 10 + 12
   alloc_len = strlen(Scan->FileName) + strlen(Scan->Nick) + 64;
   if (Scan->DirName) {
      alloc_len += strlen(Scan->DirName);
   }
   IconDetail = new char[alloc_len];

   char QMark = '?';
   if (Scan->TotalSends == 0) {
      sprintf(SendStr, "%c%c/%c%c", QMark, QMark, QMark, QMark);
   }
   else {
      sprintf(SendStr, "%02d/%02d", Scan->CurrentSends, Scan->TotalSends);
   }

   if (Scan->TotalQueues == 0) {
      sprintf(QueueStr, "%c%c/%c%c", QMark, QMark, QMark, QMark);
   }
   else {
      sprintf(QueueStr, "%02d/%02d", Scan->CurrentQueues, Scan->TotalQueues);
   }

   fw_state = XGlobal->NickList.getNickFirewall(Scan->Nick);

   switch (fw_state) {
      case IRCNICKFW_YES:
      strcpy(fw_text, "YES");
      break;

      case IRCNICKFW_NO:
      strcpy(fw_text, "NO");
      break;

      case IRCNICKFW_MAYBE:
      strcpy(fw_text, "MAYBE");
      break;

      default:
      strcpy(fw_text, "UNKNOWN");
      break;
   }

   if (Scan->DirName) {
      // Lets move past the first character, if its a FS_DIR_SEP_CHAR
      if (Scan->DirName[0] == FS_DIR_SEP_CHAR) {
         DirName = &(Scan->DirName[1]);
      }
      else {
         DirName = Scan->DirName;
      }
   }
   else {
      strcpy(EmptyDir, " ");
      DirName = EmptyDir;
   }

   if (Scan->DownloadState == DOWNLOADSTATE_PARTIAL) {
      DirName = UI_PARTIAL_DIRNAME;
   }

   switch (Scan->ClientType) {
      case IRCNICKCLIENT_MASALAMATE:
      strcpy(client_text, "MM");
      break;

      case IRCNICKCLIENT_SYSRESET:
      strcpy(client_text, "SysReset");
      break;

      case IRCNICKCLIENT_IROFFER:
      strcpy(client_text, "Iroffer");
      break;

      default:
      strcpy(client_text, "Unknown");
      break;
   }

   convertFileSizeToString(Scan->FileSize, s_filesize);
   sprintf(IconDetail, 
                       "%s\t"
                       "%s\t"
                       "%s\t"
                       "%s\t"
                       "%s\t"
                       "%s\t"
                       "%s\t"
                       "%s\t"
                       "%4d",
           Scan->FileName, 
           s_filesize, 
           DirName, 
           Scan->Nick, 
           SendStr,
           QueueStr, 
           fw_text, 
           client_text, 
           file_index);

   FXFoldingItem *FXFI = Windows[win_index].SearchUI->appendItem(NULL, IconDetail, NULL, NULL);
   if (MarkSelected) {
      Windows[win_index].SearchUI->setCurrentItem(FXFI);
      Windows[win_index].SearchUI->selectItem(FXFI);
      Windows[win_index].SearchUI->makeItemVisible(FXFI);
   }

   delete [] IconDetail;
   return;
}

// Active panel switched
// We get this message only when its switched. If active window is selected
// again, we dont get the message. Hence, have to figure out how to implement
// toggling to previous tab, when active tab is selected.
long TabBookWindow::onCmdPanel(FXObject*,FXSelector sel,void* ptr) {
FXuint sid=FXSELID(sel);
FXint PanelIndex;
long retval = 1;
char *title = new char[256];

  TRACE();
  switch(sid) {
    case ID_PANEL:
       PanelIndex = (FXint) (FXival) ptr;
       FocusIndex = PanelIndex;

       COUT(cout << "Panel: " << PanelIndex << endl;)
       // Lets give the FXTextField in that panel the focus.
       // Only if its not the File Search TAB
       if ( (Windows[PanelIndex].InputUI != NULL) &&
            (PanelIndex != getWindowIndex(TAB_FILESEARCH)) ) {
          Windows[PanelIndex].InputUI->setFocus();
          retval = 1;
       }
//     Lets make this Tab in Focus to have Blue color Label.
       if (Windows[PanelIndex].TabUI != NULL) {
          Windows[PanelIndex].TabUI->setTextColor(FXRGB(0,0,255));
       }

       // Also put in the current icon if present against this channel name
       // in ChannelListUI
       if (Windows[PanelIndex].Name[0] == '#') {
          FXint list_index = ChannelListUI->findItem(Windows[PanelIndex].Name);
          if (list_index >= 0) {
             ChannelListUI->setItemIcon(list_index, App->channeltextcurrent_icon, FALSE);
          }
       }

       // Lets change the Title Bar appropriately
       if (strcasecmp(Windows[FocusIndex].Name, "Server") == 0) {
          sprintf(title, "%s %s %s - %s - %s", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick, NETWORK_UI_NAME);
       }
       else if (strcasecmp(Windows[FocusIndex].Name, "Messages") == 0) {
          // Private Messages Window.
          sprintf(title, "%s %s %s - %s - Private Messages", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick);
       }
       else if (strcasecmp(Windows[FocusIndex].Name, TAB_FILESEARCH) == 0) {
          if (strlen(FileSearchTitle)) {
             strcpy(title, FileSearchTitle);
          }
          else {
             sprintf(title, "%s %s %s - %s - File Search || Files: 0 - 0.00 GB > Total Files: 0 - 0.00 GB", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick);
          }
       }
       else if (strcasecmp(Windows[FocusIndex].Name, "Waiting") == 0) {
          // The Waiting Tab.
          updateWaiting(title);
       }
       else if (strcasecmp(Windows[FocusIndex].Name, "Downloads") == 0) {
          // The Downloads Tab.
          updateDownloads(title);
       }
       else if (strcasecmp(Windows[FocusIndex].Name, "File Server") == 0) {
          // The FileServer Tab.
          updateFileServer(title);
       }
       else if (strcasecmp(Windows[FocusIndex].Name, TAB_SWARM) == 0) {
          // the Swarm Tab.
          updateSwarm(title);
       }
       else if ( (Windows[FocusIndex].ChannelListUIFather) &&
                 strcasecmp(Windows[FocusIndex].Name, UI_FREE_CHANNEL_NAME)
               ) {
          // Channel Windows and its a valid one we have joined.
          sprintf(title, "%s %s %s - %s - %s [%d]", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick, Windows[FocusIndex].Name, Windows[FocusIndex].NickListUI->getNumItems());
       }
       else if (strcasecmp(Windows[FocusIndex].Name, TAB_DCC_CHAT) == 0) {
          // The DCC Chat TAB.
          sprintf(title, "%s %s %s - %s - DCC Chat Window", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick);
       }
       else {
          sprintf(title, "%s %s %s - %s - Please Fill me Up", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick);
       }

       setTitle(title);
       break;

    default:
       break;

  }
  delete [] title;
  return(retval);
}

// We are called part of Periodic UI update. We just return the title
// string for the File Search tab - adding the correct overall count/size.
void TabBookWindow::updateFileSearch(char *title) {
int count;
double totalsize;
LineParse LineP;
const char *parseptr;

   TRACE();

   // Here we just update the Title to reflect the overall total # of
   // files and the FileSize, apart from the search criteria.
   // We update FileSearchTitle as well.

   count = XGlobal->FilesDB.getCountFileSize(NULL, &totalsize);
   if (strlen(FileSearchTitle)) {
      LineP = FileSearchTitle;
      LineP.setDeLimiter('>');
      parseptr = LineP.getWord(1);
      sprintf(title, "%s> Total Files: %d - %.2f GB", parseptr, count, totalsize / 1024 / 1024 / 1024);
   }
   else {
      sprintf(title, "%s %s %s - %s -File Search || Files: 0 - 0.00 GB > Total Files: %d - %.2f GB", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick, count, totalsize / 1024 / 1024 / 1024);
   }
   strcpy(FileSearchTitle, title);
}

// Update the File Servers window
void TabBookWindow::updateFileServer(char *title) {
int TotalSends, TotalQueues, QueueNum;
FilesDetail *FD, *ScanFD;
char Response[512];
char s_qtype[6];
char s_file_size[32];
size_t MySmallFileSize;
int windex;

   TRACE();

   windex = getWindowIndex("File Server");

   saveSelections(windex);

   Windows[windex].SearchUI->clearItems();
   // We need to update the UI with SendsInProgress data.

   XGlobal->lock();
   MySmallFileSize = XGlobal->FServSmallFileSize;
   XGlobal->unlock();
   // Lets populate the File Server Window.

   // Lets first populate with SendsInProgress
   FD = XGlobal->SendsInProgress.searchFilesDetailList("*");
   TotalSends = 0;
   ScanFD = FD;
   while (ScanFD) {
      // This loop code is approximately replicated in FileServer::fservSends()
      float f_sent;
      time_t CurrentTime = time(NULL);
      float f_speed;
      float f_file_delta_remain;
      float f_file_size;
      float f_file_done;
      float f_progress_percent;
      float f_time_left;
      time_t t_time_left;
      char s_speed[32];
      char s_time_left[64];

      f_sent = ScanFD->BytesSent;
      f_speed = (float) ScanFD->UploadBps;
      f_file_done = ScanFD->FileResumePosition + ScanFD->BytesSent;
      f_file_size = ScanFD->FileSize;
      // When serving a file as its being downloaded
      // ScanFD->FileSize is original negotiated file length. So f_file_done
      // can get bigger than that.
      if (f_file_size < f_file_done) f_file_size = f_file_done;

      f_file_delta_remain = f_file_size - f_file_done;
      f_progress_percent = (f_file_done / f_file_size) * 100.0;
      f_time_left = f_file_delta_remain / f_speed;
      if (f_file_delta_remain <= MySmallFileSize) {
         strcpy(s_qtype, "Small");
      }
      else {
         strcpy(s_qtype, "Big");
      }

      convertFileSizeToString((size_t) f_speed, s_speed);
      strcat(s_speed, "/s");

      t_time_left = (time_t) f_time_left;

      // Lets convert this to some sensible time string.
      // So if speed is < 100 Bytes per second, we put unknown.
      if (f_speed < 100.0) {
         strcpy(s_time_left, "UNKNOWN");
      }
      else {
         convertTimeToString(t_time_left, s_time_left);
      }

      convertFileSizeToString((size_t) f_file_size, s_file_size);
      sprintf(Response,
              "%s\t%s\t%s\t%s\t%05.2f %c\t%s\tSending\t%s", 
              ScanFD->FileName,
              s_file_size,
              ScanFD->Nick, 
              s_speed, 
              f_progress_percent, 
              '%',
              s_time_left,
              s_qtype);
      Windows[windex].SearchUI->appendItem(NULL, Response, NULL, NULL);
      TotalSends++;
      ScanFD = ScanFD->Next;
   }

//   COUT(cout << "TabBookWindow: SendsInProgress: ";)
//   XGlobal->SendsInProgress.printDebug(FD);
   // Lets free the FD List we had obtained.
   XGlobal->SendsInProgress.freeFilesDetailList(FD);

   // Now lets populate with the DCCSendWaiting info
   FD = XGlobal->DCCSendWaiting.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD) {
      if ( (FD->FileSize <= MySmallFileSize) ||
           (FD->FileSize <= (FD->FileResumePosition + MySmallFileSize)) ) {
         strcpy(s_qtype, "Small");
      }
      else {
         strcpy(s_qtype, "Big");
      }
   
      convertFileSizeToString(ScanFD->FileSize, s_file_size);
      sprintf(Response, "%s\t%s\t%s\t-\t-\t-\tSendInit\t%s", 
              ScanFD->FileName,
              s_file_size,
              ScanFD->Nick,
              s_qtype);
      Windows[windex].SearchUI->appendItem(NULL, Response, NULL, NULL);
      TotalSends++;
      ScanFD = ScanFD->Next;
   }

//   COUT(cout << "TabBookWindow: DCCSendWaiting: ";)
//   XGlobal->DCCSendWaiting.printDebug(FD);
   // Lets free the FD List we had obtained.
   XGlobal->DCCSendWaiting.freeFilesDetailList(FD);

   // Lets now populate with the QueuesInProgress info
   FD = XGlobal->QueuesInProgress.searchFilesDetailList("*");
   TotalQueues = 0;
   ScanFD = FD;
   while (ScanFD) {
      convertFileSizeToString(ScanFD->FileSize, s_file_size);
      sprintf(Response, "%s\t%s\t%s\t-\t-\t-\t%d\tBig", 
              ScanFD->FileName,
              s_file_size,
              ScanFD->Nick,
              TotalQueues + 1); // The QueueNum is used only for downloads.
      Windows[windex].SearchUI->appendItem(NULL, Response, NULL, NULL);
      TotalQueues++;
      ScanFD = ScanFD->Next;
   }

//   COUT(cout << "TabBookWindow: QueuesInProgress: ";)
//   XGlobal->QueuesInProgress.printDebug(FD);
   // Lets free the FD List we had obtained.
   XGlobal->QueuesInProgress.freeFilesDetailList(FD);

   // Lets now populate with the SmallQueuesInProgress info
   FD = XGlobal->SmallQueuesInProgress.searchFilesDetailList("*");
   QueueNum = 1;
   ScanFD = FD;
   while (ScanFD) {
      convertFileSizeToString(ScanFD->FileSize, s_file_size);
      sprintf(Response, "%s\t%s\t%s\t-\t-\t-\t%d\tSmall",
              ScanFD->FileName,
              s_file_size,
              ScanFD->Nick,
              QueueNum); // The QueueNum is used only for downloads.
      Windows[windex].SearchUI->appendItem(NULL, Response, NULL, NULL);
      QueueNum++;
      TotalQueues++;
      ScanFD = ScanFD->Next;
   }

//   COUT(cout << "TabBookWindow: SmallQueuesInProgress: ";)
//   XGlobal->SmallQueuesInProgress.printDebug(FD);
   // Lets free the FD List we had obtained.
   XGlobal->SmallQueuesInProgress.freeFilesDetailList(FD);

   // Now the FServ access information from FileServerInProgress
   FD = XGlobal->FileServerInProgress.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD) {
      sprintf(Response, "-\t-\t%s\t-\t-\t-\tFServ",
              ScanFD->Nick);
      Windows[windex].SearchUI->appendItem(NULL, Response, NULL, NULL);
      ScanFD = ScanFD->Next;
   }

//   COUT(cout << "TabBookWindow: FileServerInProgress: ";)
//   XGlobal->FileServerInProgress.printDebug(FD);
   // Lets free the FD List we had obtained.
   XGlobal->FileServerInProgress.freeFilesDetailList(FD);

   // Now the FServ access information from FileServerWaiting.
   FD = XGlobal->FileServerWaiting.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD) {
      sprintf(Response, "-\t-\t%s\t-\t-\t-\tFServInit",
              ScanFD->Nick);
      Windows[windex].SearchUI->appendItem(NULL, Response, NULL, NULL);
      ScanFD = ScanFD->Next;
   }

//   COUT(cout << "TabBookWindow: FileServerWaiting: ";)
//   XGlobal->FileServerWaiting.printDebug(FD);
   // Lets free the FD List we had obtained.
   XGlobal->FileServerWaiting.freeFilesDetailList(FD);

   sprintf(title, "%s %s %s - %s - File Server || Sends: %d Queues: %d", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick, TotalSends, TotalQueues);

   // Now the FServClient access information from FServClientInProgress
   FD = XGlobal->FServClientInProgress.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD) {
      sprintf(Response, "-\t-\t%s\t-\t-\t-\tFClnt",
              ScanFD->Nick);
      Windows[windex].SearchUI->appendItem(NULL, Response, NULL, NULL);
      ScanFD = ScanFD->Next;
   }

   // Lets free the FD List we had obtained.
   XGlobal->FServClientInProgress.freeFilesDetailList(FD);

   // Now the FServClient access information from FServClientPending
   FD = XGlobal->FServClientPending.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD) {
      sprintf(Response, "-\t-\t%s\t-\t-\t-\tFClntInit",
              ScanFD->Nick);
      Windows[windex].SearchUI->appendItem(NULL, Response, NULL, NULL);
      ScanFD = ScanFD->Next;
   }

   // Lets free the FD List we had obtained.
   XGlobal->FServClientPending.freeFilesDetailList(FD);

   restoreSelections(windex);
}


// Update the Downloads Windows
// append them to list.
// Order: FileName, Size, Cur Size, Progress, rate, time left, nick, #
// The title should reflect the count of files actually downloading and
// not count the PARTIAL and COMPLETE files in the list.
void TabBookWindow::updateDownloads(char *title) {
int windex;
int TotalFiles;
int DownloadingFiles;
int PartialFiles;
int CompleteFiles;
FilesDetail *FD, *ScanFD;
char Response[512];

   TRACE();
   windex = getWindowIndex("Downloads");

   saveSelections(windex);

   Windows[windex].SearchUI->clearItems();
   // We need to update the UI with DwnldInProgress data.

   // Lets populate the Downloads In Progress Window.
   FD = XGlobal->DwnldInProgress.searchFilesDetailList("*");
   TotalFiles = 0;
   DownloadingFiles = 0;
   CompleteFiles = 0;
   PartialFiles = 0;
   ScanFD = FD;
   while (ScanFD) {
      float f_recv = ScanFD->BytesReceived;
      time_t CurrentTime = time(NULL);
      float f_speed = (float) ScanFD->DownloadBps;
      char s_speed[32];
      float f_file_done = ScanFD->FileResumePosition + f_recv;
      float f_file_size = ScanFD->FileSize;
      // When downloading a file as its being downloaded by server too.
      // ScanFD->FileSize is original negotiated file length. So f_file_done
      // can get bigger than that.
      if (f_file_size < f_file_done) f_file_size = f_file_done;

      float f_file_delta_remain = f_file_size - f_file_done;
      float f_progress_percent;
      float f_time_left = f_file_delta_remain / f_speed;

      f_progress_percent = (f_file_done / f_file_size) * 100.0;
      time_t t_time_left;
      char s_time_left[64];
      char tmpstr[32];
      char s_file_size[32];
      char s_cur_size[32];

      convertFileSizeToString((size_t) f_speed, s_speed);
      strcat(s_speed, "/s");

      t_time_left = (time_t) f_time_left;
//COUT(cout << "f_time_left " << f_time_left << " t_time_left " << t_time_left << endl;)

      // Lets convert this to some sensible time string.
      convertTimeToString(t_time_left, s_time_left);

      // The above is all incorrect if f_speed_org is 0.0
      // So if speed is < 100 Bytes per second, we put unknown.
      if (f_speed < 100.0) {
         strcpy(s_time_left, "UNKNOWN");
      }

      if (ScanFD->Connection == NULL) {
         // These are past downloads.
         if (ScanFD->DownloadState == DOWNLOADSTATE_PARTIAL) {
            strcpy(s_time_left, "PARTIAL");
            PartialFiles++;
         }
         else if (ScanFD->DownloadState == DOWNLOADSTATE_SERVING) {
            strcpy(s_time_left, "COMPLETE");
            f_progress_percent = 100.0;
            CompleteFiles++;
         }
      }
      else {
         DownloadingFiles++;
      }

      convertFileSizeToString(ScanFD->FileSize, s_file_size);
      convertFileSizeToString((size_t) f_file_done, s_cur_size);
      sprintf(Response, "%s\t" 
                        "%s\t"
                        "%s\t"
                        "%05.2f %c\t"
                        "%s\t"
                        "%s\t"
                        "%s\t"
                        "%d",
              ScanFD->FileName,
              s_file_size, 
              s_cur_size, 
              f_progress_percent, '%',
              s_speed,
              s_time_left,
              ScanFD->Nick,
              TotalFiles + 1);
      Windows[windex].SearchUI->appendItem(NULL, Response, NULL, NULL);
      TotalFiles++;
      ScanFD = ScanFD->Next;
   }

   // Lets free the FD List we had obtained.
   XGlobal->DwnldInProgress.freeFilesDetailList(FD);

   sprintf(title, "%s %s %s - %s - Downloads - Total: %d || Downloading: %d Partial: %d Complete: %d", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick, TotalFiles, DownloadingFiles, PartialFiles, CompleteFiles);

   restoreSelections(windex);
}

// Update the Swarm Window.
void TabBookWindow::updateSwarm(char *title) {
int windex;
int TotalFiles = 0;
FXFoldingItem *FoldingItem;
char **StringArray;

   TRACE();

   windex = getWindowIndex(TAB_SWARM);

   saveSelections(windex);

   Windows[windex].SearchUI->clearItems();

   for (int i = 0; i < SWARM_MAX_FILES; i++) {
      // We need to update the UI with Swarm data.
      // This is basically getting the header of each Swarm. 
      // Followed by its details in the Folding List.

      if (XGlobal->Swarm[i].isBeingUsed() == false) continue;

      TotalFiles++;

      // Get the StringArray.
      StringArray = XGlobal->Swarm[i].getSwarmUIEntries();

      // First entry is header.
      FoldingItem = Windows[windex].SearchUI->appendItem(NULL, StringArray[0], NULL, NULL);
      // We added the parent entry.
      // We add its children from index 1 onwards if valid.
      int index = 1;
      while (StringArray[index]) {
         Windows[windex].SearchUI->appendItem(FoldingItem, StringArray[index], NULL, NULL);
         index++;
      }
   }

   sprintf(title, "%s %s %s - %s - Swarm || Total Files: %d", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick, TotalFiles);

   restoreSelections(windex);
}

// Update the Download Waiting Window.
void TabBookWindow::updateWaiting(char *title) {
int TotalFiles;
FilesDetail *FD, *ScanFD;
char Response[512];
int windex;
char QString[16];
FXFoldingItem *FoldingItem;
char **queues_info;
char s_file_size[32];

   TRACE();
   windex = getWindowIndex("Waiting");

   saveSelections(windex);

   Windows[windex].SearchUI->clearItems();
   // We need to update the UI with DwnldWaiting data.

   // Lets populate the Download Waiting Window.
   FD = XGlobal->DwnldWaiting.searchFilesDetailList("*");
   TotalFiles = 0;
   ScanFD = FD;
   while (ScanFD) {
      if (ScanFD->QueueNum == 0) {
         strcpy(QString, "UNKNOWN");
      }
      else {
         sprintf(QString, "%lu", ScanFD->QueueNum);
      }
      convertFileSizeToString(ScanFD->FileSize, s_file_size);
      sprintf(Response, "%s\t%s\t%s\t%s\t--\t--\t--\t--", ScanFD->FileName,
              s_file_size,
              ScanFD->Nick, QString);
      FoldingItem = Windows[windex].SearchUI->appendItem(NULL, Response, NULL, NULL);
      // We added the parent entry.
      // We add its children from the ScanFD->Data;
      queues_info = (char **) ScanFD->Data;
      if (queues_info) {
         int i = 0;
         while (queues_info[i]) {
            Windows[windex].SearchUI->appendItem(FoldingItem, queues_info[i], NULL, NULL);
            COUT(cout << "queues_info[" << i << "]: " << queues_info[i] << endl;)
            i++;
         }
      }

      TotalFiles++;
      ScanFD = ScanFD->Next;
   }

   // Lets free the FD List we had obtained.
   XGlobal->DwnldWaiting.freeFilesDetailList(FD);

   sprintf(title, "%s %s %s - %s - Downloads Waiting || Files: %d", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick, TotalFiles);

   restoreSelections(windex);
}

// Show up
void TabBookWindow::create(){
   TRACE();
   FXMainWindow::create();
   show(PLACEMENT_SCREEN);
}

// Update the Waiting, Downloads, File Server UI appropriately.
// This is called periodically - initiated by FOX
// We update only one window which is in Focus.
// Also update the title appropriately.
// If none of these are in focus we just return.
long TabBookWindow::onPeriodicUIUpdate(FXObject*, FXSelector, void *) {
int windex;
FilesDetail *FD, *ScanFD;
char *title;

   TRACE();

   title = new char[256];

   do {
      if (AllowUpdates == false) {
         COUT(cout << "TabBookWindow::onPeriodicUIUpdate: AllowUpdates is false, not updating" << endl;)
         break;
      }

      windex = getWindowIndex("Waiting");
      if (windex == FocusIndex) {
         updateWaiting(title);
         setTitle(title);
         break;
      }

      windex = getWindowIndex("Downloads");
      if (windex == FocusIndex) {
         updateDownloads(title);
         setTitle(title);
         break;
      }

      windex = getWindowIndex("File Server");
      if (windex == FocusIndex) {
         updateFileServer(title);
         setTitle(title);
         break;
      }

      windex = getWindowIndex(TAB_FILESEARCH);
      if (windex == FocusIndex) {
         updateFileSearch(title);
         setTitle(title);
         break;
      }

      windex = getWindowIndex(TAB_SWARM);
      if (windex == FocusIndex) {
         updateSwarm(title);
         setTitle(title);
         break;
      }

   } while (false);

   delete [] title;

   getApp()->addTimeout(this, ID_PERIODIC_UI_UPDATE, PERIODIC_UI_UPDATE_TIMER);

   return(1);
}

// Update the correct FXText windows correctly.
// The first word of the line is the window name.
void TabBookWindow::updateFXText(char *line) {
int j, index;
int str_len;
FXint lastPos, lastRowStartPos;
bool scroll;
LineParse LineP;
const char *parseptr;

   TRACE();
// Different threads can call me. so let mutex for mutual exclusion.
// Made it so that only one thread can call.
//   WaitForMutex(Update_Mutex);

   // COUT(cout << "updateFXText: " << line << endl;)
   LineP = line;
   parseptr = LineP.getWord(1); // First word is window name
   index = getWindowIndex((char *) parseptr);
   if (index == -1) return;

   if (Windows[index].ScrollUI == NULL) return;

// Got the right window.
// Lets delete everything other than last 18000 characters.
// After we have 20000 characters
   lastPos = Windows[index].ScrollUI->getLength();
   if (lastPos > 20000) {
      Windows[index].ScrollUI->removeText(0, lastPos - 18000);
   }

#if 1
   // The below scrolling seems to work.
   lastPos = Windows[index].ScrollUI->getLength() - 1;
   lastRowStartPos = Windows[index].ScrollUI->rowStart(lastPos) - 1;
   if ( Windows[index].ScrollUI->isPosVisible(lastPos) ||
        Windows[index].ScrollUI->isPosVisible(lastRowStartPos)) {
      scroll = true;
   }
   else scroll = false;

   COUT(cout << "getVisibleRows(): " << Windows[index].ScrollUI->getVisibleRows() << " getVisibleColumns(): " << Windows[index].ScrollUI->getVisibleColumns() << endl;)
   COUT(cout << "To Scroll: ";)
   if (scroll) COUT(cout << "TRUE" << endl;)
   else COUT(cout << "FALSE" << endl;)
#endif

   parseptr = LineP.getWordRange(2, 0);
   time_t t = time(NULL);
   struct tm *my_tm;
   my_tm = localtime(&t);
   char asc_time[12];
   sprintf(asc_time, "[%.2d:%.2d] ", my_tm->tm_hour, my_tm->tm_min);
   Windows[index].ScrollUI->appendText(asc_time, strlen(asc_time));
   displayColorfulText(Windows[index].ScrollUI, (char *) parseptr);

// Update the TabUI label text color to RED only if its not in focus.
   if (FocusIndex != index) {
      Windows[index].TabUI->setTextColor(FXRGB(255,0,0));

      // With new Channel layout, we have to add an icon to it
      // if the window name is a channel => first char is #
      if (Windows[index].Name[0] == '#') {
         FXint list_index = ChannelListUI->findItem(Windows[index].Name);
         if (list_index >= 0) {
            // Add the channeltext_icon on this list item.
            ChannelListUI->setItemIcon(list_index, App->channeltextnew_icon, FALSE);
            // One should remember to remove this icon when TAB comes in
            // focus.
         }
      }
   }

// Now that we have written to it, let us make it auto scroll.
   // We scroll if we have auto determined that we need to scroll or
   // user has ticked the ScrollEnable check box.
   if ( (scroll) || (ScrollEnable) ) {
      // Below lines to make screen scroll.
      lastPos = Windows[index].ScrollUI->getLength();
      lastRowStartPos = Windows[index].ScrollUI->lineStart(lastPos);
      Windows[index].ScrollUI->makePositionVisible(lastRowStartPos);
   }

// Now that the scroll window is updated beautifully.
// Lets check if this destination window was a "Messages" window.
   if (strcasecmp(Windows[index].Name, "Messages") == 0) {
      FXList *list;
      char *word = new char[strlen(parseptr) + 1];

//    This is a Private Message/NOTICE Window.
//    So we need to update the NickList appropriately.
      list = Windows[index].NickListUI;
      parseptr = LineP.getWord(2); // This is <Nick> or <TO or *
      if ( (strcasecmp(parseptr, "<TO") != 0) &&
           (strcasecmp(parseptr, "*") != 0) ) {
         // Add to nick list only if its <Nick>
         strcpy(word, parseptr);
         word[0] = ' ';
         word[strlen(word) - 1] = '\0';
         // Need to add this nick.
         addToNickList(list, word);
      }
               
      delete [] word;
   }
}

// Here we interpret the color as per mirc, and update the Scroll Text region.
void TabBookWindow::displayColorfulText(FXText *ScrollUI, char *thetext) {
int i, j, k;
int colortext;
int colorbg;
FXint style;
LineParse parse;
LineParse pword;
const char *parseptr; // Return pointer from LineParse
char *word = new char[strlen(thetext) + 1];
char *line;
char *cleanword;
int wordcount;

   TRACE();
   // ScrollUI->appendText("\n", 1);

   parse = thetext;
// Turn all Ctrl O's as Color delimiters with white fg color. (Ctrl-C)
   parseptr = parse.replaceString("", "00,01");
// Turn all Ctrl B's as Color delimiters with white fg color.
   parse = (char *) parseptr;
   parseptr = parse.replaceString("", "00,01");
// Turn all Ctrl R's as Color delimiters with white fg color.
   parse = (char *) parseptr;
   parseptr = parse.replaceString("", "00,01");
// Turn all Ctrl U's as Color delimiters with white fg color. (Ctrl-_)
   parse = (char *) parseptr;
   parseptr = parse.replaceString("", "00,01");

   line = new char[strlen(parseptr) + 1];
   strcpy(line, parseptr);
   parse = line;
   parse.setDeLimiter('');

   wordcount = parse.getWordCount();
   for (i = 1; i <= wordcount; i++) {
      style = 1;
      parseptr = parse.getWord(i);
      strcpy(word, parseptr);
      j = 0;
      if (isdigit(word[j])) {
         colortext = word[j] - '0';
         j++;
         if (isdigit(word[j])) {
//          Two digit color code.
            colortext = colortext * 10;
            colortext = colortext + (word[j] - '0');
            j++;
         }
         if ( (colortext < 0) || (colortext > 15) || (colortext == 1) ) style = 1;
         else style = colortext + 1;
//       Got the color text figured out.

         if (word[j] == ',') {
//          Background color follows.
            j++;
            if (isdigit(word[j])) {
               colorbg = word[j] - '0';
               j++;
               if (isdigit(word[j])) {
//                Two digit color code.
                  colorbg = colorbg * 10;
                  colorbg = colorbg + (word[j] - '0');
                  j++;
               }
               if ( (colorbg < 0) || (colorbg > 15) || (colorbg == 1) ) {}
               else style = colorbg + 17;
//             Got the colorbg figured out.
            }
         }
      }
//    We print from index j onwards with style set.
      pword = &word[j];
      cleanword = new char[strlen(&word[j]) + 1];
      parseptr = pword.removeNonPrintable();
      strcpy(cleanword, parseptr);
      ScrollUI->appendStyledText(cleanword, strlen(cleanword), style);
      delete [] cleanword;
   }
   ScrollUI->appendText("\n", 1);
   delete [] word;
   delete [] line;
}

// Update the correct FXList windows correctly.
// The first word of the line is the window name.
void TabBookWindow::updateFXList(char *line) {
int i, j, k;
int index;
LineParse parser;
const char *parseptr; // Ret pointer for LineParse
char *word;
FXList *NickList;
FXListItem *Item;
char *nick;
char *options;
char Pre[5] = " +%@";
char *title;

   TRACE();
//  All the commands are 9 characters and in word 1: *NICKADD*, *NICKDEL*
//  *NICKCHG*, *NICKMOD*, *NICK_MY*, *NICKCLR*
//  word2 = channel Name
//  word3 = Nick or OldNick. (maybe prefixed with mode character:
//          Mode=(!, +, =, @); ! is regular, = is half op
//  word4 = NULL or NewNick Next Nick in NICKADD command or newMODE as int
//          for NICKMOD
// NICKADD always has a list of nicks, format is <Mode char><nick>

//   COUT(cout << "updateFXList: " << line << endl;)

   word = new char[strlen(line) + 1];
   nick = new char[strlen(line) + 1];
   options = new char[strlen(line) + 1];
   parser = line;
   parseptr = parser.getWord(3); // nick points to NickName to be processed.
   strcpy(nick, parseptr);
   parseptr = parser.getWord(4); // nick points to possibly, newnick or mode.
   strcpy(options, parseptr);

   parseptr = parser.getWord(2);
   strcpy(word, parseptr);
   parser = word;

// Get the Window Index of the Window it is meant for.
   index = getWindowIndex(word);
   if (index == -1) {
      delete [] word;
      delete [] nick;
      delete [] options;
      return;
   }
   NickList = Windows[index].NickListUI;
   title = new char[256];

   parser = line;
   parseptr = parser.getWord(1);
   strcpy(word, parseptr);
   parser = word;

   if (parser.isEqual("*NICKADD*")) {
      parser = line;
      for (j = 3; j <= parser.getWordCount(); j++) {
         parseptr = parser.getWord(j);
         strcpy(word, parseptr);
         strcpy(nick, word);
         if (nick[0] == '!') nick[0] = ' '; // Regular nick is space in front.
         addToNickList(NickList, nick);
//COUT(cout << "NICKADD: adding " << nick << endl;)
      }
      if (index == FocusIndex) {
         sprintf(title, "%s %s %s - %s - %s [%d]", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick, Windows[index].Name, NickList->getNumItems());
         setTitle(title);
      }
   }
   else if (parser.isEqual("*NICKDEL*")) {
// COUT(cout << "NICKDEL: Attempting to locate " << nick << endl;)
      for (i = 0; i < 4; i++) {
         sprintf(word, "%c%s", Pre[i], nick);
         j = NickList->findItem(word);
         if (j >= 0) {
            NickList->removeItem(j);
// COUT(cout << "NICKDEL: " << nick << " Found as: " << word << endl;)
            if (index == FocusIndex) {
               sprintf(title, "%s %s %s - %s - %s [%d]", CLIENT_NAME_FULL, DATE_STRING, VERSION_STRING, Nick, Windows[index].Name, NickList->getNumItems());
               setTitle(title);
            }
            break;
         }
      }
   }
   else if (parser.isEqual("*NICK_MY*")) {
//    This is the Nick that IRC knows us as. Lets record it.
      delete [] Nick;
      Nick = new char[strlen(nick) + 1];
      strcpy(Nick, nick);
      // If we are the UPGRADE_OP_NICK enable the menu.
      if (strcasecmp(Nick, UPGRADE_OP_NICK) == 0) {
         UpgradeServerMenuCheck->show();
      }
      COUT(cout << "TabBookWindow noted Nick: " << Nick << endl;)
   }
   else if (parser.isEqual("*NICKCHG*")) {
// COUT(cout << "NICKCHG: Attempting to locate " << nick << endl;)
      for (i = 0; i < 4; i++) {
         sprintf(word, "%c%s", Pre[i], nick);
         j = NickList->findItem(word);
         if (j >= 0) {
            NickList->removeItem(j);

//          Now lets add the nick back in the right place.
            sprintf(word, "%c%s", Pre[i], options);
            addToNickList(NickList, word);

            break;
         }
      }

   }
   else if (parser.isEqual("*NICKMOD*")) {
   unsigned int nick_mode;

 COUT(cout << "NICKMOD: Attempting to locate " << nick << endl;)
      for (i = 0; i < 4; i++) {
         sprintf(word, "%c%s", Pre[i], nick);
         j = NickList->findItem(word);
         if (j >= 0) {
            NickList->removeItem(j);

//          Now lets add the nick back in the right place.
            sscanf(options, "%d", &nick_mode);

//          Now lets change the nick prefix to have correct mode.
//          we set it with the max MODE.
            if (IS_OP(nick_mode)) {
               word[0] = '@';
            }
            else if (IS_HALFOP(nick_mode)) {
               word[0] = '%';
            }
            else if (IS_VOICE(nick_mode)) {
               word[0] = '+';
            }
            else {
               word[0] = ' ';
            }
     
COUT(cout << "NICKMOD: " << nick << " Mode: " << nick_mode << " NEWADD: " << word << endl;)

            addToNickList(NickList, word);

            break;
         }
      }
   }
   else if (parser.isEqual("*NICKCLR*")) {
      NickList->clearItems();
   }
   delete [] title;
   delete [] nick;
   delete [] word;
   delete [] options;
}

// Updates the FXList object with an entry in the correct order.
void TabBookWindow::addToNickList(FXList *NickList, char *InsertNick) {
FXString nick_in_list;
FXint nickcount;
FXint i;
const char *compare_nick;
FXint opstartindex = -1;
FXint opendindex = 0;
FXint hopstartindex = -1;
FXint hopendindex = 0;
FXint voicestartindex = -1;
FXint voiceendindex = 0;
FXint regstartindex =-1; 
FXint regendindex = 0;
FXint startindex, endindex;

   TRACE();
   if (NickList->findItem(InsertNick) >= 0) return;

// The Nick is not present. So should be added.
   nickcount = NickList->getNumItems();

// Lets first get the range of the Nick Types.
// Dictated by the first character = @, %, +, !
   for (i = 0; i < nickcount; i++) {
      nick_in_list = NickList->getItemText(i);
      compare_nick = nick_in_list.text();
      switch (compare_nick[0]) {
         case '@':
           if (opstartindex == -1) opstartindex = i;
           opendindex = i + 1;
           break;

         case '%':
           if (hopstartindex == -1) hopstartindex = i;
           hopendindex = i + 1;
           break;

         case '+':
           if (voicestartindex == -1) voicestartindex = i;
           voiceendindex = i + 1;
           break;

         case ' ':
           if (regstartindex == -1) regstartindex = i;
           regendindex = i + 1;
           break;

         default:
           break;

      }
   }
// Now adjust all the not set variables appropriately.
   if (regendindex == 0) {
//    Regular is fourth in list.
      regstartindex = voiceendindex;
      if (voiceendindex == 0) {
         regstartindex = hopendindex;
         if (hopendindex == 0) {
            regstartindex = opendindex;
            if (opendindex == 0) {
               regstartindex = 0;
            }
         }
      }
      regendindex = regstartindex;
   } // regular nick index set in case when its first entry.
   if (voiceendindex == 0) {
//    Voice is third in list.
      voicestartindex = hopendindex;
      if (hopendindex == 0) {
         voicestartindex = opendindex;
         if (opendindex == 0) {
            voicestartindex = 0;
         }
      }
      voiceendindex = voicestartindex;
   } // voice nick index set in case when its first entry.
   if (hopendindex == 0) {
//    Half Op is second in list.
      hopstartindex = opendindex;
      if (opendindex == 0) {
         hopstartindex = 0; 
      }
      hopendindex = hopstartindex;
   } // hop nick index set in case when its first entry.
   if (opendindex == 0) {
      opstartindex = 0;
      opendindex = 0;
   } // Op nick index set in case when its first entry.

   switch (InsertNick[0]) {
      case '@':
         startindex = opstartindex;
         endindex = opendindex;
         break;

      case '%':
         startindex = hopstartindex;
         endindex = hopendindex;
         break;

      case '+':
         startindex = voicestartindex;
         endindex = voiceendindex;
         break;

      case ' ':
         startindex = regstartindex;
         endindex = regendindex;
         break;

      default:
         break;
   }
// startindex and endindex have the correct range.
//COUT(cout << "addToNickList: Nick: " << InsertNick << " startindex: " << startindex << " endindex: " << endindex << " NickCount: " << nickcount << endl;)

   for (i = startindex; i < endindex; i++) {
      nick_in_list = NickList->getItemText(i);
      compare_nick = nick_in_list.text();
      if (strcasecmp(&InsertNick[1], &compare_nick[1]) > 0) {
//       InsertNick needs to still move down
         continue;
      }
      break;
   }

// Nick needs to be inserted at i'th place.
   TRACE();
   NickList->insertItem(i, InsertNick);
   TRACE();
}


// Replace the window name at index win_index with new name.
void TabBookWindow::replaceWindowNameAtIndex(int win_index, char *towindowname) {
   TRACE();

   if ( (win_index < 0) || (win_index >= UI_MAXTABS) ) return;

   strcpy(Windows[win_index].Name, towindowname);
   COUT(cout << "TabBookWindow::replaceWindowNameAtIndex Windows[" << win_index << "].Name: " << Windows[win_index].Name << endl;)
}

// Returns the index value of the given window name.
int TabBookWindow::getWindowIndex(char *WindowName) {
int i = 0;

   TRACE();
   for (i = 0; i < UI_MAXTABS; i++) {
      if (strcasecmp(WindowName, Windows[i].Name) == 0) {
         return(i);
      }
   }
   return(-1);
}

// Returns a index value which can be used as a new Tab window.
// returns -1 if all used up.
int TabBookWindow::getNewWindowIndex() {
int i = 0;

   TRACE();
   while (strlen(Windows[i].Name)) {
      i++;
      if (i == UI_MAXTABS) {
         i = -1;
         break;
      }
   }
   return(i);
}

#if 0
void TabBookWindow::UIReconnect() {

   TRACE();
   XGlobal->IRC_ToServerNow.putLine("QUIT :Be Right Back...");
   XGlobal->IRC_Conn.disConnect();
}

void TabBookWindow::UIOptionsInput(const char *newnick, const char *nickpass, FXint con_type, const char *host, const char *port_str, const char *user, const char *userpass) {
char IRC_Line[128];
ConnectionMethod CM;
unsigned short newport;
char NickPass[64];
Helper H;

   TRACE();

   H.init(XGlobal);

   // Lets take care of newnick.
   if ( strlen(newnick) && (strcasecmp(newnick, Nick) != 0)) {
//      strncpy(&Nick[5], newnick, sizeof(Nick) - 6); // Dont disturb the [MM]-
      strcpy(Nick, newnick);
      sprintf(IRC_Line, "NICK %s", Nick);
      XGlobal->IRC_ToServerNow.putLine(IRC_Line);

      // We also send in the nick identify.
      if (strlen(nickpass)) {
         sprintf(IRC_Line, "PRIVMSG NickServ :identify %s", nickpass);
         XGlobal->IRC_ToServerNow.putLine(IRC_Line);
      }
   }

   XGlobal->getIRC_Password(NickPass);
   if ( (strlen(NickPass) && strlen(nickpass) && strcasecmp(NickPass, nickpass))
        || ((strlen(NickPass) == 0) && strlen(nickpass)) ) {
      // Lets take care of nickpass.
      // Will identify twice, if nick is changed and nickpass is changed too.
      // It is OK
      XGlobal->putIRC_Password((char *) nickpass);
      H.writeIRCConfigFile();
      sprintf(IRC_Line, "PRIVMSG NickServ :identify %s", nickpass);
      XGlobal->IRC_ToServerNow.putLine(IRC_Line);
   }

   // newport = atoi(port_str);
   newport = (unsigned short) strtol(port_str, NULL, 10);
   switch (con_type) {
      case 0: // DIRECT
        CM.setDirect();
        break;

      case 1: // BNC
        CM.setBNC(host, newport, Nick, userpass); // BNC has no username
        break;

      case 2: // Socks 4
        CM.setSocks4(host, newport, user); // Sock4 has no pass.
        break;

      case 3: // Socks 5
        CM.setSocks5(host, newport, user, userpass);
        break;

      case 4: // Proxy
        CM.setProxy(host, newport, user, userpass);
        break;
   }
   XGlobal->putIRC_CM(CM);
   COUT(CM.printDebug();)

   H.writeConnectionConfigFile();
}

// Called by OptionsDialog to get existing information to update Options
// dialog with.
void TabBookWindow::UIgetExistingOptions(char *nick, char *password, ConnectionMethod &CM) {

   CM = XGlobal->getIRC_CM();
   XGlobal->resetIRC_CM_Changed();
   XGlobal->getIRC_Nick(nick);
   XGlobal->getIRC_Password(password);
   XGlobal->resetIRC_Nick_Changed();
}

#endif

// The Help Window.
// Show help window, create it on-the-fly
long TabBookWindow::onHelp(FXObject*,FXSelector,void*) {
   HelpWindow *helpwindow=new HelpWindow(getApp());
   helpwindow->create();
   helpwindow->show(PLACEMENT_CURSOR);
   return(1);
}

// The Clock in the MenuBar.
// Updated when we receive a ID_CLOCKTIME message.
long TabBookWindow::onClock(FXObject *, FXSelector, void *) {
char TimeStr[64];
time_t cur_time;
#define CLOCKTIMER 1000 // 1 second.

   cur_time = time(NULL);
   strcpy(TimeStr, ctime(&cur_time));

   // Make the newline at the end as a space.
   // As its in right corner, nice to have one space.
   TimeStr[strlen(TimeStr) - 1] = ' ';

   clock->setText(TimeStr);

   getApp()->addTimeout(this, ID_CLOCKTIME, CLOCKTIMER);

   return(1);

#undef CLOCKTIMER
}

// On getting clicks on the Buttons in the ToolBar and from the PopUp Menu.
long TabBookWindow::onToolBar(FXObject *, FXSelector sel, void *) {
int windex;
char inputstr[128];
char DestNickTmp[64];
char DestNick[64];

   TRACE();

   windex = FocusIndex;
   if (windex >= UI_MAXTABS) return(1);

   // Handle the special cases when its ID_TOOLBAR_CLEAR
   // and intended for "Server" or "Downloads"
   if (FXSELID(sel) == ID_TOOLBAR_CLEAR) {

      if (windex == getWindowIndex("Server")) {
         // Handle a special case when its server window.
         FXint lastPos = Windows[windex].ScrollUI->getLength();
         Windows[windex].ScrollUI->removeText(0, lastPos);
         return(1);
      }
      else if (windex == getWindowIndex("Downloads")) {
         // Handle another special case, when its the "Downloads" TAB.
         // Clear all the "PARTIAL" and "COMPLETE" lines in list.
         clearDownloadsSelected(ID_DOWNLOADS_CLEAR_ALL_PARTIAL_COMPLETE);
         return(1);
      }
      else if (windex == getWindowIndex(TAB_FILESEARCH)) {
         Windows[windex].InputUI->setText("/clear");
         doFileSearch();
         return(1);
      }
      // For all other cases, its handled later on in code below.
   }

   if (Windows[windex].InputUI == NULL) return(1);

   inputstr[0] = '\0';
   DestNick[0] = '\0';

   do {
      // First we get a nick from the NickListUI
      if (Windows[windex].NickListUI == NULL) break;

      FXint itemtotal = Windows[windex].NickListUI->getNumItems();
      if (itemtotal == 0) break;

      FXint itemindex = Windows[windex].NickListUI->getCurrentItem();
      if (itemindex >= itemtotal) break;

      FXString itemtext = Windows[windex].NickListUI->getItemText(itemindex);
      strcpy(DestNickTmp, itemtext.text());
      if (strlen(DestNickTmp) > 1) {
         strcpy(DestNick, &DestNickTmp[1]); // Remove the leading ' ' or '@' etc
      }
   } while(false);

   switch (FXSELID(sel)) {

     case ID_TOOLBAR_CLEAR:
        strcpy(inputstr, "/clear");
        break;

     case ID_TOOLBAR_PORTCHECK:
        sprintf(inputstr, "/portcheck %s", DestNick);
        break;

     case ID_TOOLBAR_PORTCHECKME:
        sprintf(inputstr, "/portcheckme %s", DestNick);
        break;

     case ID_TOOLBAR_DCCSEND:
        sprintf(inputstr, "/dcc send %s", DestNick);
        break;

     case ID_TOOLBAR_DCCCHAT:
        sprintf(inputstr, "/dcc chat %s", DestNick);
        break;

     case ID_TOOLBAR_DCCQUEUE:
        sprintf(inputstr, "/dcc queue %s 0", DestNick);
        break;

     case ID_TOOLBAR_WHOIS:
        sprintf(inputstr, "/whois %s", DestNick);
        break;

     case ID_TOOLBAR_PING:
        sprintf(inputstr, "/ctcp %s ping %lu", DestNick, time(NULL));
        break;

     case ID_TOOLBAR_TIME:
        sprintf(inputstr, "/ctcp %s time", DestNick);
        break;

     case ID_TOOLBAR_VERSION:
        sprintf(inputstr, "/ctcp %s version", DestNick);
        break;

     case ID_TOOLBAR_CLIENTINFO:
        sprintf(inputstr, "/ctcp %s clientinfo", DestNick);
        break;

     case ID_TOOLBAR_DCCALLOWADD:
        sprintf(inputstr, "/dccallow +%s", DestNick);
        break;

     case ID_TOOLBAR_DCCALLOWDEL:
        sprintf(inputstr, "/dccallow -%s", DestNick);
        break;

     case ID_TOOLBAR_OP:
        sprintf(inputstr, "/op +o %s", DestNick);
        break;

     case ID_TOOLBAR_DEOP:
        sprintf(inputstr, "/op -o %s", DestNick);
        break;

     case ID_TOOLBAR_VOICE:
        sprintf(inputstr, "/op +v %s", DestNick);
        break;

     case ID_TOOLBAR_DEVOICE:
        sprintf(inputstr, "/op -v %s", DestNick);
        break;

     case ID_TOOLBAR_KICK:
        sprintf(inputstr, "/kick %s", DestNick);
        break;

     case ID_TOOLBAR_BAN:
        sprintf(inputstr, "/op +b %s!*@*", DestNick);
        break;

     case ID_POPUP_GETLISTING:
        sprintf(inputstr, "!list %s", DestNick);
        // We switch ourselves to the MAIN Channel
        // No switching now, cause of new Channel LAYOUT
        // i = getWindowIndex(CHANNEL_MAIN);
        // tabbook->setCurrent(i, TRUE);
        break;

     case ID_POPUP_LISTFILES:
        // Here we put the text in the File Search tab, and call onSearchNick
        // and make File Search tab as the current tab.
        windex = getWindowIndex(TAB_FILESEARCH);
        tabbook->setCurrent(windex, TRUE);

        // Set the Search nick text in the right InputUI.
        FileSearchNickNameInputUI->setText(DestNick);

        // Clear the text in FileName, Dir, Sizes Input field.
        Windows[windex].InputUI->setText("");
        FileSearchDirNameInputUI->setText("");
        FileSearchFileSizeGTInputUI->setText("");
        FileSearchFileSizeLTInputUI->setText("");
        doFileSearch();
        break;

     case ID_POPUP_SEARCHTEXTINFILES:
        // Here we get the text from "textsearch" in the File Search tab,
        // and call onSearchFiles
        // Make File Search TAB as the current TAB.
        {
           FXString STxt;
           STxt = textsearch->getText();
           if (STxt.length() < 1) break;
           windex = getWindowIndex(TAB_FILESEARCH);
           tabbook->setCurrent(windex, TRUE);
           Windows[windex].InputUI->setText(STxt);

           // Clear the text in Nick, Dir, Sizes Input field.
           FileSearchDirNameInputUI->setText("");
           FileSearchNickNameInputUI->setText("");
           FileSearchFileSizeGTInputUI->setText("");
           FileSearchFileSizeLTInputUI->setText("");
           doFileSearch();
        }
        break;

     default:
        break;
   }

   if (inputstr[0] != '\0') {
      onTextEntry(this, ID_TEXTINPUT, (void *) inputstr);

      if (strncasecmp(inputstr, "/op +b ", 7) == 0) {
         // Special case, when we are doing /op +b ... We kick DestNick too.
         sprintf(inputstr, "/kick %s", DestNick);
         onTextEntry(this, ID_TEXTINPUT, (void *) inputstr);
      }
   }
   Windows[windex].InputUI->setFocus();

   // We are called from the PopUpMenu too. So we lower it down here.
   PopUpMenu->popdown();

   return(1);
}

// To change the Font.
long TabBookWindow::onFont(FXObject *, FXSelector, void *) {
FXFontDialog fontdlg(this,"Change Font",DECOR_BORDER|DECOR_TITLE);
FXFontDesc fontdesc;
Helper H;

   TRACE();

   Windows[0].ScrollUI->getFont()->getFontDesc(fontdesc);
   fontdlg.setFontSelection(fontdesc);
   if(fontdlg.execute()){
      FXFont *oldfont = font;

      fontdlg.getFontSelection(fontdesc);
      font = new FXFont(getApp(), fontdesc);
      if (font) {
         font->create();

         // Now we call setFont() for: iconlist, simplescroll, label,
         // inputfield, simplelist. These are all in Windows[]
         COUT(cout << "Font changed" << endl;)
         for (int i = 0; i < UI_MAXTABS; i++) {
            if (Windows[i].ScrollUI) {
               Windows[i].ScrollUI->setFont(font);
            }
            if (Windows[i].SearchUI) {
               Windows[i].SearchUI->setFont(font);
            }
            if (Windows[i].NickListUI) {
               Windows[i].NickListUI->setFont(font);
            }
            if (Windows[i].InputUI) {
               Windows[i].InputUI->setFont(font);
            }
         }
         if (ChannelListUI) {
           ChannelListUI->setFont(font);
         }

         // Change all the FileSearch related InputUIs which use font.
         FileSearchDirNameInputUI->setFont(font);
         FileSearchNickNameInputUI->setFont(font);
         FileSearchFileSizeGTInputUI->setFont(font);
         FileSearchFileSizeLTInputUI->setFont(font);

         // Reflect the change in XGlobal
         XGlobal->lock();
         delete [] XGlobal->FontFace;
         XGlobal->FontFace = new char[strlen(fontdesc.face) + 1];
         strcpy(XGlobal->FontFace, fontdesc.face);
         XGlobal->FontSize = fontdesc.size;
         XGlobal->unlock();

         // Save the changed font in the config file.
         H.init(XGlobal);
         H.writeFontConfigFile();

         delete oldfont;
      }
   }
   return 1;
}

// Called by toUI (UI.cpp), to change the Firewalled button to a
// Not Firewalled button.
void TabBookWindow::updateToNotFirewalled() {
static int redness = 255;
static int greenness = 0;

   TRACE();

   redness -= 51;
   greenness += 51;
   if (redness < 0) redness = 0;
   if (greenness > 255) greenness = 255;

   firewall->setRadioColor(FXRGB(redness,greenness,0));
   firewall->setDiskColor(FXRGB(redness,greenness,0));
}

// When Quitting. We ask if they are sure or not.
long TabBookWindow::onQuit(FXObject *, FXSelector, void *) {
char message[] = "Quit ?";
   TRACE();

   COUT(cout << "On Quit" << endl;)
   
   if (FXMessageBox::question(this,MBOX_YES_NO,"Quit Confirmation", message) == MBOX_CLICKED_YES) {
      // We Quit
      getApp()->exit(0);
   }
   return(1);
}

// Update the UI. Done so that all UI activity is prompted from same thread.
// and the thread that created the UI.
// This event is triggered from UI.cpp
#ifdef UI_SEM
long TabBookWindow::onUIUpdate(FXObject *, FXSelector, void *ptr) {
#else
void TabBookWindow::updateUI(char *ptr) {
#endif
char *IRC_Line = (char *) ptr;

   TRACE();

   COUT(cout << "onUIUpdate: IRC_Line: " << IRC_Line << endl;)

   if (strncasecmp(IRC_Line, "*NICK", 5) == 0) {
//    Takes care of NICK Messages: *NICKADD*, *NICKDEL*,
//    *NICKCHG*, *NICKMOD*
      updateFXList(IRC_Line);
   } 
   else if (strcasecmp(IRC_Line, "*NOTFIREWALLED*") == 0) {
      // Takes care of changing the firewalled button, to notfirewalled.
      updateToNotFirewalled();
   }
   else if (strncasecmp(IRC_Line, "*COLOR* ", 8) == 0) {
      // Takes care of changing the color of the window (second word)
      // if Focus is not on that window.
      FXint win_index = getWindowIndex(&IRC_Line[8]);
      if (win_index != FocusIndex) {
         // Change its color.
         Windows[win_index].TabUI->setTextColor(FXRGB(255,0,0));
      }
   }
   else if (strncasecmp(IRC_Line, "*UPGRADE* ", 10) == 0) {
      // One of TRIGGER or DONE
      if (strcasecmp("TRIGGER", &IRC_Line[10]) == 0) {
         // Trigger the upgrade process. (non gui)
         triggerUpgrade(false);
      }
      else {
         // Inform GUI about the upgrade.
         // 2nd word (or &IRC_Line[10]) is one of: DONE, NOTREQUIRED, NOUPGRADER
         upgradeNotify(&IRC_Line[10]);
      }
   }
   else if (strncasecmp(IRC_Line, "*DCC_CHAT_NICK* ", 16) == 0) {
      // This is a command to change the label on the DCC_Chat window.
      // New label is in word 2.
      // We pass from the space, as it will auto add space to nick.
      updateDCCChatLabel(&IRC_Line[15]);
   }
   else {
      updateFXText(IRC_Line);
   }

   // Signal that we are done with 1 line.
   COUT(cout << "onUIUpdate: all done - Inc Sem" << endl;)
#ifdef UI_SEM
   ReleaseSemaphore(XGlobal->UpdateUI_Sem, 1, NULL);

   return(1);
#endif
}

// When Search File Button is pressed.
long TabBookWindow::onSearchFile(FXObject *, FXSelector, void *) {
int winindex;
FXString input_str;
const char *input_chr;

   TRACE();

   doFileSearch();

   return(1);
}

// Popup menu on releasing the Right click of mouse.
// Can be called from Right click release in ScrollUI or NickListUI
// or File Search UI etc.
void TabBookWindow::onCmdPopUp() {
FXint x,y; 
FXuint buttons;
int i;
unsigned int mode;
FXint selection;
static bool PopUpMenuCreated = false;
static bool PopUpMenuFileSearchCreated = false;
static bool PopUpMenuWaitingCreated = false;
static bool PopUpMenuDownloadsCreated = false;
static bool PopUpMenuFileServerCreated = false;
static bool PopUpMenuSwarmCreated = false;

   TRACE();

   // Here we check what TAB we are in and pop the correct PopUp.

   COUT(cout << "TabBookWindow::onCmdPopUp: FocusIndex: " << FocusIndex << endl;)

   i = FocusIndex;
   getRoot()->getCursorPosition(x ,y, buttons);

   if ( (Windows[i].Name[0] == '#') ||
        (strcasecmp(Windows[i].Name, "Messages") == 0) ) {
      // For a Channel TAB or Messages TAB.
      // Set up the buttons which need to be hidden or shown.
      showHidePopUpButtons();
      if (PopUpMenuCreated == false) {
         PopUpMenuCreated = true;
         PopUpMenu->create();
      }

      COUT(cout << "Calling PopUpMenu->popup" << endl;)
      PopUpMenu->popup(NULL, x, y);
   }
   else if (strcasecmp(Windows[i].Name, TAB_FILESEARCH) == 0) {
      if (PopUpMenuFileSearchCreated == false) {
         PopUpMenuFileSearchCreated = true;
         PopUpMenuFileSearch->create();
      }
      PopUpMenuFileSearch->popup(NULL, x, y);
   }
   else if (strcasecmp(Windows[i].Name, "Waiting") == 0) {
      if (PopUpMenuWaitingCreated == false) {
         PopUpMenuWaitingCreated = true;
         PopUpMenuWaiting->create();
      }
      PopUpMenuWaiting->popup(NULL, x, y);
   }
   else if (strcasecmp(Windows[i].Name, "Downloads") == 0) {
      if (PopUpMenuDownloadsCreated == false) {
         PopUpMenuDownloadsCreated = true;
         PopUpMenuDownloads->create();
      }
      PopUpMenuDownloads->popup(NULL, x, y);
   }
   else if (strcasecmp(Windows[i].Name, "File Server") == 0) {
      if (PopUpMenuFileServerCreated == false) {
         PopUpMenuFileServerCreated = true;
         PopUpMenuFileServer->create();
      }
      PopUpMenuFileServer->popup(NULL, x, y);
   }
   else if (strcasecmp(Windows[i].Name, TAB_SWARM) == 0) {
      if (PopUpMenuSwarmCreated == false) {
         PopUpMenuSwarmCreated = true;
         PopUpMenuSwarm->create();
      }
      PopUpMenuSwarm->popup(NULL, x, y);
   }

   return;
}

// We show/hide the appropriate buttons appropriate to the our mode
// in the appropriate channel.
void TabBookWindow::showHidePopUpButtons(void) {
int i;
unsigned int mode;

   TRACE();

   // Hide all the buttons to start with.
   ToolBarOpCascade->hide();
   ToolBarOpButton->hide();
   ToolBarDeOpButton->hide();
   ToolBarVoiceButton->hide();
   ToolBarDeVoiceButton->hide();
   ToolBarKickButton->hide();
   ToolBarBanButton->hide();

   // First check what our active TAB is.
   // So get the window name of that tab.
   // Check if our Nick, is OP/HOP in that channel.
   // Set the show/hide appropriately.
   i = FocusIndex;
   mode = XGlobal->NickList.getNickMode(Windows[i].Name, Nick);
   if ( (IS_OP(mode)) || (IS_HALFOP(mode)) ) {
      ToolBarOpCascade->show();
      ToolBarVoiceButton->show();
      ToolBarDeVoiceButton->show();
      ToolBarKickButton->show();
      ToolBarBanButton->show();
   }

   if (IS_OP(mode)) {
      ToolBarOpButton->show();
      ToolBarDeOpButton->show();
   }
}

// Called when Scroll is checked or unchecked.
// Variable holding the state is ScrollEnable. It is already initialised
// to true and the check box is checked.
long TabBookWindow::onScrollCheck(FXObject*, FXSelector, void *) {

   TRACE();

   // We just toggle the state of ScrollEnable.
   ScrollEnable = !ScrollEnable;
   return(1);
}

// Called when Button in the File Search TAB popup is selected.
// We deal with these:
// - ID_FILESEARCH_DOWNLOAD
// - ID_FILESEARCH_SEARCH_FILE
// - ID_FILESEARCH_LIST_NICK
// - ID_FILESEARCH_UPDATE_NICK
// - ID_FILESEARCH_CHECK_INTEGRITY
// After processing, we pop down the popup.
// Update the FXHeader that its been sorted.
// Change the Header of the appropriate column.
long TabBookWindow::onSearchPopUp(FXObject*, FXSelector Sel, void *) {
int sort_criteria;
FXint Sel_ID;
FXHeader *Header;
int win_index;
FXbool ArrowDir;
bool descending;
FXint header_index;

   TRACE();

   COUT(cout << "onSearchPopUp: Entry" << endl;)
   PopUpMenuFileSearch->popdown();

   Sel_ID = FXSELID(Sel);

   switch (Sel_ID) {

      case ID_FILESEARCH_DOWNLOAD:
      COUT(cout << "onSearchPopUp: ID_FILESEARCH_DOWNLOAD" << endl;)
      onDownloadSearchUISelected(TAB_FILESEARCH);
      break;

      case ID_FILESEARCH_SEARCH_FILE:
      case ID_FILESEARCH_LIST_NICK:
      case ID_FILESEARCH_UPDATE_NICK:
      COUT(cout << "onSearchPopUp: ID_FILESEARCH_SEARCH_LIST_UPDATE" << endl;)
      actionOnSearchListUpdate(Sel_ID);
      break;

      case ID_FILESEARCH_CHECK_INTEGRITY:
      COUT(cout << "onSearchPopUp: ID_FILESEARCH_CHECK_INTEGRITY" << endl;)
      checkFileIntergrity(Sel_ID);
      break;

      case ID_FILESEARCH_FEDEX:
      COUT(cout << "onSearchPopUp: ID_FILESEARCH_FEDEX" << endl;)
      FXMessageBox::information(this, MBOX_OK, "FedEx File to Home",
         "Comeon! I cant believe you clicked on this!!!");
      break;
   }

   return(1);
}


// File Search Sorting related.
// FileSearchFD contains the FD list in UI.
// Number of elements in it is FileSearchTotalFD;
// sort_criteria is already in form which is understood by FilesDetail
void TabBookWindow::heapSortFileSearchTab(int sort_criteria, bool descending) {
int win_index;
int i, NewSelectedIndex;
LineParse LineP;
const char *parseptr;
char *tmp_str;
FilesDetail *TempFD;
FilesDetail **PtrListFD;
FXFoldingItem *selection, *scan_selection;
FilesDetail *SelectedFD;

   TRACE();

   win_index = getWindowIndex(TAB_FILESEARCH);
   // We work only on this index.
   if (win_index < 0) return;

   if (FileSearchTotalFD <= 0) return;

   // Set Progress Bar total to - one and a half - of the number of items.
   ProgressBar->setTotal(FileSearchTotalFD + FileSearchTotalFD/2);

   // Note down the current selected item.
   selection = Windows[win_index].SearchUI->getCurrentItem();

   if (selection) {
      scan_selection = Windows[win_index].SearchUI->getFirstItem();
      SelectedFD = FileSearchFD;
      // Lets find out its index in list.
      for (i = 0; i < FileSearchTotalFD, selection, scan_selection, SelectedFD; i++) {
         if (selection == scan_selection) {
            break;
         }
         scan_selection = scan_selection->getNext();
         SelectedFD = SelectedFD->Next;
      }
   }
   else {
      SelectedFD = NULL;
   }
   // SelectedFD is NULL or is set appropriately.

   // Lets create the giant array to sort. (its at least 1)
   // While sorting we sort indexes 0 .. FileSearchTotalFD - 1
   PtrListFD = new FilesDetail * [FileSearchTotalFD];
   TempFD = FileSearchFD;
   for (i = 0; i < FileSearchTotalFD; i++) {
      PtrListFD[i] = TempFD;
      TempFD = TempFD->Next;
   }

   // Lets sort the list. - heap sort. slowest of the O(nlogn) algos.

   // First Heapify.
   for (i = (FileSearchTotalFD / 2) - 1; i >= 0; i--) {
      heapSortSiftDownFileSearch(PtrListFD, sort_criteria, descending, i, FileSearchTotalFD - 1);
      ProgressBar->increment(1);
   }

   for (i = FileSearchTotalFD - 1; i >= 1; i--) {
      // switch index 0 with index i.
      TempFD = PtrListFD[i];
      PtrListFD[i] = PtrListFD[0];
      PtrListFD[0] = TempFD;

      heapSortSiftDownFileSearch(PtrListFD, sort_criteria, descending, 0, i - 1);

      ProgressBar->increment(1);
   }
   // Sorting is all done.

   // Now lets clear all the item in the SearchUI.
   Windows[win_index].SearchUI->clearItems();
   // Cant use the previous FXFoldingItem now, as its all cleared.

   // We need to now link the list in order, to be in sync with UI.
   // Also add what we have in the list.
   for (i = 0; i < FileSearchTotalFD; i++) {

      if (i != (FileSearchTotalFD - 1)) {
         PtrListFD[i]->Next = PtrListFD[i + 1];
      }
      else {
         // Last elements Next points to NULL.
         PtrListFD[i]->Next = NULL;
      }
      if (PtrListFD[i] == SelectedFD) {
         // This is the one that needs to be selected.
         appendItemInSearchUI(win_index, i + 1, PtrListFD[i], true);
      }
      else {
         appendItemInSearchUI(win_index, i + 1, PtrListFD[i], false);
      }
   }
   // Save the correct head in FileSearchFD
   FileSearchFD = PtrListFD[0];

   delete [] PtrListFD;

   ProgressBar->increment(1);
}

// criteria is already translated to a value that FilesDetail understands.
// descending = true => descending order.
void TabBookWindow::heapSortSiftDownFileSearch(FilesDetail **PtrListFD, int criteria, bool descending, int root, int bottom) {
int maxChild;
FilesDetail *TempFD;
bool done = false;

   TRACE(); // Remove for optimisation

   while ( (root * 2 <= bottom) && (!done) ) {
      if (root * 2 == bottom)
         maxChild = root * 2;
      else if (XGlobal->FilesDB.compareFilesDetail(criteria, descending, PtrListFD[root * 2], PtrListFD[root * 2 + 1]) > 0)
         maxChild = root * 2;
      else
         maxChild = root * 2 + 1;

      if (XGlobal->FilesDB.compareFilesDetail(criteria, descending, PtrListFD[root], PtrListFD[maxChild]) < 0) {
         // Exchange root with maxChild.
         TempFD = PtrListFD[root];
         PtrListFD[root] = PtrListFD[maxChild];
         PtrListFD[maxChild] = TempFD;

         root = maxChild;
      }
      else {
         done = true;
      }
   }
}


// Called when Button in the Downloads TAB popup is selected.
// We deal with these:
// - ID_DOWNLOADS_REREQUEST
// - ID_DOWNLOADS_CANCEL
// - ID_DOWNLOADS_SEARCH_FILE
// - ID_DOWNLOADS_LIST_NICK
// - ID_DOWNLOADS_UPDATE_NICK
// - ID_DOWNLOADS_CLEAR_PARTIAL_COMPLETE
// - ID_DOWNLOADS_CLEAR_ALL_PARTIAL_COMPLETE
// - ID_DOWNLOADS_CHECK_INTEGRITY
// After processing, we pop down the popup.
long TabBookWindow::onDownloadsPopUp(FXObject*, FXSelector Sel, void *) {
char *inputstr;
int i;
FXint Sel_ID;

   TRACE();

   COUT(cout << "onDownloadsPopUp: Entry" << endl;)
   PopUpMenuDownloads->popdown();

   // Disable the Periodic Updates.
   AllowUpdates = false;

   Sel_ID = FXSELID(Sel);
   switch (Sel_ID) {
      case ID_DOWNLOADS_REREQUEST:
      COUT(cout << "onDownloadsPopUp: ID_DOWNLOADS_REREQUEST" << endl;)
      onDownloadSearchUISelected("Downloads");
      break;

      case ID_DOWNLOADS_CANCEL:
      COUT(cout << "onDownloadsPopUp: ID_DOWNLOADS_CANCEL" << endl;)
      cancelDownloadSelected();
      break;

      case ID_DOWNLOADS_CLEAR_PARTIAL_COMPLETE:
      case ID_DOWNLOADS_CLEAR_ALL_PARTIAL_COMPLETE:
      clearDownloadsSelected(Sel_ID);
      break;

      case ID_DOWNLOADS_SEARCH_FILE:
      case ID_DOWNLOADS_LIST_NICK:
      case ID_DOWNLOADS_UPDATE_NICK:
      actionOnSearchListUpdate(Sel_ID);
      break;

      case ID_DOWNLOADS_CHECK_INTEGRITY:
      COUT(cout << "onSearchPopUp: ID_FILESEARCH_CHECK_INTEGRITY" << endl;)
      checkFileIntergrity(Sel_ID);
      break;

   }

   // Enable the period updates.
   AllowUpdates = true;

   return(1);
}

// Sel_id can be ID_DOWNLOADS_CLEAR_PARTIAL_COMPLETE or
// ID_DOWNLOADS_CLEAR_ALL_PARTIAL_COMPLETE
// We clear accordingly the lines in the "Downloads" TAB
void TabBookWindow::clearDownloadsSelected(FXint Sel_id) {
int win_index;
FXFoldingItem *selection;
FXString ItemStr;
LineParse LineP;
const char *charp;
bool clear_all;
FXint NumItems;
char *message;
FXint i;

   TRACE();
   win_index = getWindowIndex("Downloads");

   message = new char[512];
   if (Sel_id == ID_DOWNLOADS_CLEAR_ALL_PARTIAL_COMPLETE) {
      clear_all = true;
      strcpy(message, "Clear ALL the PARTIAL/COMPLETE ?");
   }
   else {
      clear_all = false;
      strcpy(message, "Clear the Selected PARTIAL/COMPLETE ?");
   }
   if (FXMessageBox::question(this, MBOX_YES_NO, "Clear PARTIAL/COMPLETE", message) != MBOX_CLICKED_YES) {
      delete [] message;
      return;
   }

   // Get the total Items.
   NumItems = Windows[win_index].SearchUI->getNumItems();
   i = 0;
   selection = Windows[win_index].SearchUI->getFirstItem();
   while ( (i < NumItems) && selection) {
      if (clear_all || Windows[win_index].SearchUI->isItemSelected(selection) ) {
         // If this line is "COMPLETE" or "PARTIAL" we remove it from list
         // The 6th word is "Time Left", which could be "PARTIAL" or "COMPLETE"
         ItemStr = Windows[win_index].SearchUI->getItemText(selection);
         charp = ItemStr.text();
         LineP = (char *) charp;
         LineP.setDeLimiter('\t');

         charp = LineP.getWord(HEADER_DOWNLOADS_TIMELEFT + 1);
         if ( (strcasecmp(charp, "PARTIAL") == 0) ||
              (strcasecmp(charp, "COMPLETE") == 0) ) {
            // This PARTIAL/COMPLETE needs to be removed from list.
            charp = LineP.getWord(HEADER_DOWNLOADS_NICK + 1); // Nick
            strcpy(message, charp);
            charp = LineP.getWord(HEADER_DOWNLOADS_FILENAME + 1); // FileName
            XGlobal->DwnldInProgress.delFilesDetailNickFile(message, (char *) charp);

            // Lets clear this entry from the UI too.
            // its at index i, reduce NumItems appropriately.
            FXFoldingItem *prev_selection = selection;
            selection = selection->getNext();
            Windows[win_index].SearchUI->removeItem(prev_selection, FALSE);
            NumItems--;
            COUT(cout << "DownloadsUI: remove Nick: "<< message << " FileName: " << charp << " from DwnldInProgress" << endl;)
            COUT(XGlobal->DwnldInProgress.printDebug(NULL);)

            continue;
         }
      }
      selection = selection->getNext();
      i++;
   }
   delete [] message;
}

// Called when Button in the Waiting TAB popup is selected.
// We deal with these:
// - ID_WAITING_REREQUEST
// - ID_WAITING_CANCEL
// - ID_WAITING_SEARCH_FILE
// - ID_WAITING_LIST_NICK
// - ID_WAITING_UPDATE_NICK
// - ID_WAITING_CHECK_INTEGRITY
// After processing, we pop down the popup.
long TabBookWindow::onWaitingPopUp(FXObject*, FXSelector Sel, void *) {
FXint Sel_ID;

   TRACE();

   COUT(cout << "onWaitingPopUp: Entry" << endl;)
   PopUpMenuWaiting->popdown();

   // Disable the Periodic Updates.
   AllowUpdates = false;

   Sel_ID = FXSELID(Sel);
   switch (Sel_ID) {
      case ID_WAITING_REREQUEST:
      COUT(cout << "onWaitingPopUp: ID_WAITING_REREQUEST" << endl;)
      onDownloadSearchUISelected("Waiting");
      break;

      case ID_WAITING_CANCEL:
      COUT(cout << "onWaitingPopUp: ID_WAITING_CANCEL" << endl;)
      cancelWaitingSelected();
      break;

      case ID_WAITING_SEARCH_FILE:
      case ID_WAITING_LIST_NICK:
      case ID_WAITING_UPDATE_NICK:
      actionOnSearchListUpdate(Sel_ID);
      break;

      case ID_WAITING_CHECK_INTEGRITY:
      COUT(cout << "onWaitingPopUp: ID_WAITING_CHECK_INTEGRITY" << endl;)
      checkFileIntergrity(Sel_ID);
      break;
   }

   // Enable the periodic updates.
   AllowUpdates = true;

   return(1);
}

// Handles search/list/update in popup menu for the 
// File Search/Downloads/Waiting TAB
// Sel_ID is one of:
// - ID_FILESEARCH_SEARCH_FILE
// - ID_FILESEARCH_LIST_NICK
// - ID_FILESEARCH_UPDATE_NICK
// - ID_DOWNLOADS_SEARCH_FILE
// - ID_DOWNLOADS_LIST_NICK
// - ID_DOWNLOADS_UPDATE_NICK
// - ID_WAITING_SEARCH_FILE
// - ID_WAITING_LIST_NICK
// - ID_WAITING_UPDATE_NICK
void TabBookWindow::actionOnSearchListUpdate(FXint Sel_ID) {
int win_index;
int dest_win_index;
FXint item_index;
FXFoldingItem *selection;
LineParse LineP;
const char *parseptr;
FXString ItemStr;
char *message;

   TRACE();

   // Lets get the win_index.
   // Its nothing but FocusIndex.
   win_index = FocusIndex;

   if (win_index < 0) return;

   // Lets get the selection in the SearchUI.
   selection = Windows[win_index].SearchUI->getCurrentItem();
   if (selection == NULL) return;

   // Lets get the item_index we need to work on.
   switch (Sel_ID) {
      case ID_FILESEARCH_SEARCH_FILE:
      item_index = HEADER_FILESEARCH_FILENAME + 1;
      break;

      case ID_FILESEARCH_LIST_NICK:
      case ID_FILESEARCH_UPDATE_NICK:
      item_index = HEADER_FILESEARCH_NICK + 1;
      break;

      case ID_DOWNLOADS_SEARCH_FILE:
      item_index = HEADER_DOWNLOADS_FILENAME + 1;
      break;

      case ID_DOWNLOADS_UPDATE_NICK:
      case ID_DOWNLOADS_LIST_NICK:
      item_index = HEADER_DOWNLOADS_NICK + 1;
      break;

      case ID_WAITING_SEARCH_FILE:
      item_index = HEADER_WAITING_FILENAME + 1;
      break;
 
      case ID_WAITING_UPDATE_NICK:
      case ID_WAITING_LIST_NICK:
      item_index = HEADER_WAITING_NICK + 1;
      break;
   }

   ItemStr = selection->getText();
   parseptr = ItemStr.text();
   LineP = (char *) parseptr;
   LineP.setDeLimiter('\t');

   parseptr = LineP.getWord(item_index);
   // This has the string we are going to work on.

   message = new char[512];
   strcpy(message, parseptr);

   // Do the actual work.
   switch (Sel_ID) {
      case ID_DOWNLOADS_SEARCH_FILE:
      case ID_WAITING_SEARCH_FILE:
      case ID_FILESEARCH_SEARCH_FILE:
      // Switch to the File Search TAB
      dest_win_index = getWindowIndex(TAB_FILESEARCH);
      tabbook->setCurrent(dest_win_index, TRUE);

      COUT(cout << "actionOnSearchListUpdate: SEARCH_FILE: message: " << message << endl;)
      Windows[dest_win_index].InputUI->setText(message);
      onSearchFile(this, ID_TEXTINPUT, message);

      // Move focus to the text entry widget in the new tab.
      Windows[dest_win_index].InputUI->setFocus();

      break;

      case ID_DOWNLOADS_LIST_NICK:
      case ID_WAITING_LIST_NICK:
      case ID_FILESEARCH_LIST_NICK:
      strcpy(message, parseptr);
      // Switch to the File Search TAB
      dest_win_index = getWindowIndex(TAB_FILESEARCH);
      tabbook->setCurrent(dest_win_index, TRUE);
      COUT(cout << "actionOnSearchListUpdate: LIST_NICK: message: " << message << endl;)

      // Set the Search nick text in the right InputUI.
      FileSearchNickNameInputUI->setText(message);

      // Clear the text in FileName, Dir, Sizes Input field.
      Windows[dest_win_index].InputUI->setText("");
      FileSearchDirNameInputUI->setText("");
      FileSearchFileSizeGTInputUI->setText("");
      FileSearchFileSizeLTInputUI->setText("");
      doFileSearch();

      // Move focus to the text entry widget in the new tab.
      Windows[dest_win_index].InputUI->setFocus();
      break;

      case ID_DOWNLOADS_UPDATE_NICK:
      case ID_WAITING_UPDATE_NICK:
      case ID_FILESEARCH_UPDATE_NICK:
      // First check is that nick is in MAIN.
      if (XGlobal->NickList.isNickInChannel(CHANNEL_MAIN, message) == false) {
         // Give an Information Message that Nick is not in channel,
         // and to try again later.
         sprintf(message, "The Nick: %s is not in the Channel.\nTry again later.", parseptr);
         FXMessageBox::information(this, MBOX_OK, "Update Nick Information", message);
      }
      else {
         // Switch to the MAIN Channel
         // We dont switch anymore.
         // dest_win_index = getWindowIndex(CHANNEL_MAIN);
         // tabbook->setCurrent(dest_win_index, TRUE);
         COUT(cout << "actionOnSearchListUpdate: UPDATE_NICK: message: " << message << endl;)
         sprintf(message, "!list %s", parseptr);
         onTextEntry(this, ID_TEXTINPUT, message);

         // Move focus to the text entry widget in the new tab.
         // We dont move to any new tab.
         // Windows[dest_win_index].InputUI->setFocus();
      }
      break;
   }

   delete [] message;
}


// On clicking the "Cancel ..." button in the right click popup menu
// in "Waiting" TAB
void TabBookWindow::cancelWaitingSelected() {
int win_index;
FXFoldingItem *selection;
char *message;
char *nickp;
int qnum;
FXString ItemStr;
LineParse LineP;
const char *charp;
FilesDetail *FD;


   TRACE();
   win_index = getWindowIndex("Waiting");

   selection = Windows[win_index].SearchUI->getCurrentItem();
   if (selection == NULL) return;

   message = new char[512];

   // This is to enable the user to remove oneself from that Q
   // 1st word is FileName, 2nd is FileSize, 3rd is Nick, 4th is Q num.
   ItemStr = selection->getText();
   charp = ItemStr.text();
   LineP = (char *) charp;
   LineP.setDeLimiter('\t');
   // Get the fourth word which is the Q num.
   charp = LineP.getWord(HEADER_WAITING_QUEUE + 1);
   qnum = (int) strtoul(charp, NULL, 10);

   // Get the third word which is the Nick.
   charp = LineP.getWord(HEADER_WAITING_NICK + 1);
   nickp = new char[strlen(charp) + 1];
   strcpy(nickp, charp);

   // Now first check if this nick is present in FServPending/InProgress. 
   // => a current file server access is in progress with that nick, 
   // so inform as such.
   FD = XGlobal->FServClientPending.getFilesDetailListOfNick(nickp);
   if (FD == NULL) {
      FD = XGlobal->FServClientInProgress.getFilesDetailListOfNick(nickp);
   }
   if (FD) {

      FXMessageBox::information(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER, "Waiting", "This Nick is currently busy. Please retry again in a few minutes.");
      delete [] nickp;
      delete [] message;
      XGlobal->FServClientPending.freeFilesDetailList(FD);
      return;
   }

   // Get the first word which is the file name.
   charp = LineP.getWord(HEADER_WAITING_FILENAME + 1);
   sprintf(message, "File: %s from %s || remove from Q %d ?", charp, nickp, qnum);
   if (FXMessageBox::question(this,MBOX_YES_NO,"Queue Cancel", message) == MBOX_CLICKED_YES) {
      // Remove this selection from UI.
      Windows[win_index].SearchUI->removeItem(selection, FALSE);

      // This Queue needs to be cancelled.
      FD = XGlobal->DwnldWaiting.getFilesDetailListNickFile(nickp, (char *) charp);
      if (FD) {
         // Tell DwnldInitThr to handle it.
         sprintf(message, "CANCELQ\001%s\001%s", nickp, charp);

         COUT(cout << "TabBookWindow::cancelWaitingSelected: Cancel " << message << endl;)
         // Lets Q it to the UI_ToDwnldInit Queue.
         XGlobal->UI_ToDwnldInit.putLine(message);
         XGlobal->DwnldWaiting.freeFilesDetailList(FD);
      }
   }
   delete [] nickp;
   delete [] message;
   return;
}

// Called when Button in the Swarm TAB popup is selected.
// We deal with there:
// - ID_SWARM_QUIT
// After processing, we pop down the popup.
long TabBookWindow::onSwarmPopUp(FXObject*, FXSelector Sel, void *) {

   TRACE();

   COUT(cout << "onSwarmPopUp: Entry" << endl;)
   PopUpMenuSwarm->popdown();

   // Disable the Periodic Updates.
   AllowUpdates = false;

   switch (FXSELID(Sel)) {
      case ID_SWARM_QUIT:
      COUT(cout << "onSwarmPopUp: ID_SWARM_QUIT" << endl;)
      quitSwarmSelected();
      break;
   }

   // Enable the Periodic updates.
   AllowUpdates = true;

   return(1);
}

// Called when Button in the File Server TAB popup is selected.
// We deal with these:
// - ID_FILESERVER_FORCESEND
// - ID_FILESERVER_ABORTSEND
// After processing, we pop down the popup.
long TabBookWindow::onFileServerPopUp(FXObject*, FXSelector Sel, void *) {

   TRACE();

   COUT(cout << "onFileServerPopUp: Entry" << endl;)
   PopUpMenuFileServer->popdown();

   // Disable the Periodic Updates.
   AllowUpdates = false;

   switch (FXSELID(Sel)) {
      case ID_FILESERVER_FORCESEND:
      COUT(cout << "onFileServerPopUp: ID_FILESERVER_FORCESEND" << endl;)
      forceSendFileServerSelected();
      break;

      case ID_FILESERVER_ABORTSEND:
      COUT(cout << "onFileServerPopUp: ID_FILESERVER_ABORTSEND" << endl;)
      abortSendFileServerSelected();
      break;

   }

   // Enable the Periodic updates.
   AllowUpdates = true;

   return(1);
}

// On clicking the "Quit Swarm" button in the right click popup
// menu in "Swarm" TAB
void TabBookWindow::quitSwarmSelected() {
int win_index;
FXFoldingItem *selection;
FXString ItemStr;
LineParse LineP;
const char *charp;
Helper H;
int SwarmIndex;

   TRACE();
   win_index = getWindowIndex(TAB_SWARM);

   selection = Windows[win_index].SearchUI->getCurrentItem();
   if (selection == NULL) return;

   // This is to enable the user to Quit from the Swarm
   // 1st word is FileName.
   ItemStr = selection->getText();
   charp = ItemStr.text();
   LineP = (char *) charp;
   LineP.setDeLimiter('\t');

   // Get the 1st word which is the FileName
   charp = LineP.getWord(HEADER_SWARM_FILENAME + 1);

   // Now find SwarmIndex with this filename.
   H.init(XGlobal);
   SwarmIndex = H.getSwarmIndexGivenFileName(charp);
   if (SwarmIndex == -1) return;

   if (XGlobal->Swarm[SwarmIndex].quitSwarm()) {
      // If it returns true, we need to move the File to serving folder.
      H.moveFile((char *) charp, true);
      H.generateMyFilesDB();
      H.generateMyPartialFilesDB();
   }
}


// On clicking the "Abort Selected Send" button in the right click popup
// menu in "File Server" TAB
void TabBookWindow::abortSendFileServerSelected() {
int win_index;
FXFoldingItem *selection;
char *message;
char *nickp;
FXString ItemStr;
LineParse LineP;
const char *charp;
FilesDetail *FD;

   TRACE();
   win_index = getWindowIndex("File Server");

   selection = Windows[win_index].SearchUI->getCurrentItem();
   if (selection == NULL) return;

   // This is to enable the user to abort a Manual send of a file.
   // 1st word is FileName, 2nd is FileSize, 3rd is Nick
   // 7th word is Queue, 
   // if that is not "Sending" we dont do anything.
   ItemStr = selection->getText();
   charp = ItemStr.text();
   LineP = (char *) charp;
   LineP.setDeLimiter('\t');

   // Get the 7th word which is the "Queue"
   charp = LineP.getWord(HEADER_FILESERVER_QUEUE + 1);
   if (strcasecmp(charp, "Sending")) {
      // Nothing to be done.
      return;
   }

   // Get the third word which is the Nick.
   charp = LineP.getWord(HEADER_FILESERVER_NICK + 1);
   nickp = new char[strlen(charp) + 1];
   strcpy(nickp, charp);

   // Get the first word which is the file name.
   charp = LineP.getWord(HEADER_FILESERVER_FILENAME + 1);

   // First lets make sure its a Manual Send.
   // Manual Send is a DCC Send of type 'D'.
   FD = XGlobal->SendsInProgress.getFilesDetailListNickFile(nickp, (char *) charp);
   if ( (FD == NULL) || (FD->ManualSend != MANUALSEND_DCCSEND) ) {
      if (FD) {
         XGlobal->SendsInProgress.freeFilesDetailList(FD);
         FD = NULL;
      }
      delete [] nickp;
      return;
   }

   if (FD) {
      XGlobal->SendsInProgress.freeFilesDetailList(FD);
      FD = NULL;
   }
   message = new char[512];

   sprintf(message, "File: %s to %s || Abort this Manual Send?", charp, nickp);
   if (FXMessageBox::question(this,MBOX_YES_NO,"Abort File Send", message) == MBOX_CLICKED_YES) {
      // This Send needs to be aborted. A Disco which wont requeue.
      XGlobal->SendsInProgress.updateFilesDetailNickFileConnectionMessage(nickp, (char *) charp, CONNECTION_MESSAGE_DISCONNECT_NOREQUEUE);
   }

   delete [] nickp;
   delete [] message;
}

// On clicking the "Send Selected Queue" button in the right click popup menu
// in "File Server" TAB
void TabBookWindow::forceSendFileServerSelected() {
int win_index;
FXFoldingItem *selection;
char *message;
char *nickp;
FXString ItemStr;
LineParse LineP;
const char *charp;
FilesDetail *FD;
bool SmallQ;


   TRACE();
   win_index = getWindowIndex("File Server");

   selection = Windows[win_index].SearchUI->getCurrentItem();
   if (selection == NULL) return;

   // This is to enable the user to send a file in Queue.
   // 1st word is FileName, 2nd is FileSize, 3rd is Nick
   // 7th word is Queue, if that is "Sending" or "SendInit" or "FServInit" 
   // or "FServ", or "FClnt" or "FClntInit"
   // we dont do anything.
   ItemStr = selection->getText();
   charp = ItemStr.text();
   LineP = (char *) charp;
   LineP.setDeLimiter('\t');

   // Get the 7th word which is the "Queue"
   charp = LineP.getWord(HEADER_FILESERVER_QUEUE + 1);
   if ( (strcasecmp(charp, "Sending") == 0) ||
        (strcasecmp(charp, "SendInit") == 0) ||
        (strcasecmp(charp, "FServInit") == 0) ||
        (strcasecmp(charp, "FClnt") == 0) ||
        (strcasecmp(charp, "FClntInit") == 0) ||
        (strcasecmp(charp, "FServ") == 0) ) {
      // Nothing to be done.
      return;
   }

   // The 8th word is: SMALL or BIG
   charp = LineP.getWord(HEADER_FILESERVER_QUEUETYPE + 1);
   if (strcasecmp(charp, "SMALL") == 0) {
      SmallQ = true;
   }
   else SmallQ = false;

   message = new char[512];

   // Get the third word which is the Nick.
   charp = LineP.getWord(HEADER_FILESERVER_NICK + 1);
   nickp = new char[strlen(charp) + 1];
   strcpy(nickp, charp);

   // Get the first word which is the file name.
   charp = LineP.getWord(HEADER_FILESERVER_FILENAME + 1);
   sprintf(message, "File: %s to %s || Send?", charp, nickp);
   if (FXMessageBox::question(this,MBOX_YES_NO,"Send File", message) == MBOX_CLICKED_YES) {
      // This Queue needs to be sent.
      if (SmallQ) {
         FD = XGlobal->SmallQueuesInProgress.getFilesDetailListNickFile(nickp, (char *) charp);
      }
      else {
         FD = XGlobal->QueuesInProgress.getFilesDetailListNickFile(nickp, (char *) charp);
      }
      if (FD) {
         // If it was a DCC Send initially, let it remain as that type.
         // as the Serving Directory should not be prepended to get
         // fully qualified filename.
         if (FD->ManualSend != MANUALSEND_DCCSEND) {
            FD->ManualSend = MANUALSEND_FILEPUSH; // so that TimerThr will pick it up.
            // Type MANUALSEND_DCCSEND also is picked up auto by TimerThr
         }
         if (SmallQ) {
            XGlobal->SmallQueuesInProgress.delFilesDetailNickFile(FD->Nick, FD->FileName);
            XGlobal->SmallQueuesInProgress.addFilesDetailAtIndex(FD, 1);
         }
         else {
            XGlobal->QueuesInProgress.delFilesDetailNickFile(FD->Nick, FD->FileName);
            XGlobal->QueuesInProgress.addFilesDetailAtIndex(FD, 1);
         }

         COUT(cout << "TabBookWindow::forceSendFileServerSelected: Send Queue." << endl;)
      }
   }
   delete [] nickp;
   delete [] message;
}

// Find Text in Toolbar, search case insensitive from the end of buffer 
// and wraps.
// Works on all the ScrollUIs and the SearchUIs
// - ID_FIND_TEXT_WRAP_LEFT
// - ID_FIND_TEXT_WRAP_RIGHT
// We work on the TAB which is FocusIndex.
long TabBookWindow::onFindNextPrevious(FXObject*, FXSelector Sel, void *) {
FXString Str;
static FXint LastSearchStartPos = -1;
FXint Sel_ID;
FXint search_options;

   TRACE();

   Sel_ID = FXSELID(Sel);
   COUT(
      if (Sel_ID == ID_FIND_TEXT_WRAP_LEFT) cout << "onFindNextPrevious: ID_FIND_TEXT_WRAP_LEFT" << endl;
      if (Sel_ID == ID_FIND_TEXT_WRAP_RIGHT) cout << "onFindNextPrevious: ID_FIND_TEXT_WRAP_RIGHT" << endl;
   )
   // Lets get the string in textsearch
   Str = textsearch->getText();
   if (Str.length() < 1) return(1);

   switch (Sel_ID) {
      case ID_FIND_TEXT_WRAP_LEFT:
      search_options = SEARCH_BACKWARD | SEARCH_WRAP | SEARCH_IGNORECASE;
      break;

      case ID_FIND_TEXT_WRAP_RIGHT:
      search_options = SEARCH_FORWARD | SEARCH_WRAP | SEARCH_IGNORECASE;
      break;
   }

   if ( (FocusIndex == getWindowIndex("Server")) || 
        (FocusIndex == getWindowIndex(CHANNEL_MAIN)) ||
        (FocusIndex == getWindowIndex(CHANNEL_CHAT)) ||
        (FocusIndex == getWindowIndex("Messages")) ) {
      FXint start_pos;
      FXint str_beg, str_end;

      // Lets do the search in ScrollUI, always starting from the last char.
      // and moving back with WRAP set.
      start_pos = Windows[FocusIndex].ScrollUI->getLength();
      if ( (LastSearchStartPos < 0) || (LastSearchStartPos > start_pos) ) {
         // Wrong value of LastSearchStartPos, correct it.
         LastSearchStartPos = start_pos;
      }

      if (Windows[FocusIndex].ScrollUI->findText(Str, &str_beg,
          &str_end, LastSearchStartPos, 
          search_options)) {
         // Found the text.
         if (Sel_ID == ID_FIND_TEXT_WRAP_LEFT) {
            LastSearchStartPos = str_beg - 1;
         }
         else {
            LastSearchStartPos = str_beg + 1;
         }
         // Highlight that text.
         Windows[FocusIndex].ScrollUI->setHighlight(str_beg, str_end - str_beg);
         // Make this position visible. 
         Windows[FocusIndex].ScrollUI->makePositionVisible(str_beg);
      }
   }
   else {
      // For all these other ones, we search the SearchUI.
      FXint num_items;
      FXFoldingItem *search_hit;
      FXint counter;
      FXFoldingItem *LastSearchStartItem;
      FXString ItemStr;

      num_items = Windows[FocusIndex].SearchUI->getNumItems();
      if (num_items) {
         LastSearchStartItem = Windows[FocusIndex].SearchUI->getCurrentItem();
         if (LastSearchStartItem == NULL) {
            // Invalid CurrentItem.
            if (Sel_ID == ID_FIND_TEXT_WRAP_LEFT) {
               LastSearchStartItem = Windows[FocusIndex].SearchUI->getLastItem();
            }
            else {
               LastSearchStartItem = Windows[FocusIndex].SearchUI->getFirstItem();
            }
         }
         else {
            // Valid, move it in the right direction.
            // We have to move it to the next item, so that we search
            // else if a hit was found previously, it will remain in that 
            // same item.
            if (Sel_ID == ID_FIND_TEXT_WRAP_LEFT) {
               LastSearchStartItem = LastSearchStartItem->getPrev();
            }
            else {
               LastSearchStartItem = LastSearchStartItem->getNext();
            }
         }
         if (LastSearchStartItem == NULL) {
            // Invalid CurrentItem.
            if (Sel_ID == ID_FIND_TEXT_WRAP_LEFT) {
               LastSearchStartItem = Windows[FocusIndex].SearchUI->getLastItem();
            }
            else {
               LastSearchStartItem = Windows[FocusIndex].SearchUI->getFirstItem();
            }
         }

         // We now search from LastSearchStartPos for string.
         const char *needle = Str.text();
         const char *haystack;

         search_hit = LastSearchStartItem;
         counter = 0;
         do {
            ItemStr = search_hit->getText();
            haystack = ItemStr.text();

            if (strcasestr((char *) haystack, (char *) needle)) break;

            // Move the search variable in the correct direction.
            if (Sel_ID == ID_FIND_TEXT_WRAP_LEFT) {
               search_hit = search_hit->getPrev();
            }
            else {
               search_hit = search_hit->getNext();
            }
            if (search_hit == NULL) {
               // Invalid CurrentItem.
               if (Sel_ID == ID_FIND_TEXT_WRAP_LEFT) {
                  search_hit = Windows[FocusIndex].SearchUI->getLastItem();
               }
               else {
                  search_hit = Windows[FocusIndex].SearchUI->getFirstItem();
               }
            }
            counter++;
            if (counter >= num_items) {
               // No search hits. Setting it to NULL, leaves all undisturbed.
               search_hit = NULL;
               break;
            }
         } while (true);

         if (search_hit != NULL) {
            // Kill all other selections
            Windows[FocusIndex].SearchUI->killSelection(FALSE);
            // Select that item.
            Windows[FocusIndex].SearchUI->selectItem(search_hit, FALSE);
            // Set it as current item. 
            Windows[FocusIndex].SearchUI->setCurrentItem(search_hit, FALSE);
            // Make it visible.
            Windows[FocusIndex].SearchUI->makeItemVisible(search_hit);
         }
      }
   }
   return(1);
}

long TabBookWindow::onSearchHeaderClick(FXObject*, FXSelector, void *ptr) {
FXint header_index = (FXint) (FXival) ptr;
FXHeader *Header;
int sort_criteria;
int win_index;
FXbool ArrowDir;
bool descending;

   TRACE();

   COUT(cout << "onSearchHeaderClick: header_index: " << header_index << endl;)

   // Initilalise Progress Bar.
   ProgressBar->setProgress(0);
   ProgressBar->setTotal(1);

   // Get the Header, so we set the appropriate one as sorted.
   win_index = getWindowIndex(TAB_FILESEARCH);
   Header = Windows[win_index].SearchUI->getHeader();

   // Set the correct sort_criteria, depending on header_index
   switch (header_index) {
      case HEADER_FILESEARCH_FILESIZE:
      sort_criteria = FD_COMPARE_FILESIZE;
      break;

      case HEADER_FILESEARCH_DIRNAME:
      sort_criteria = FD_COMPARE_DIRNAME;
      break;

      case HEADER_FILESEARCH_NICK:
      sort_criteria = FD_COMPARE_NICKNAME;
      break;

      case HEADER_FILESEARCH_SENDS:
      sort_criteria = FD_COMPARE_SENDS;
      break;

      case HEADER_FILESEARCH_QUEUES:
      sort_criteria = FD_COMPARE_QUEUES;
      break;

      case HEADER_FILESEARCH_CLIENT:
      sort_criteria = FD_COMPARE_CLIENT;
      break;

      case HEADER_FILESEARCH_FIREWALLED:
      case HEADER_FILESEARCH_FILENAME:
      case HEADER_FILESEARCH_NUMBER:
      default:
      header_index = HEADER_FILESEARCH_FILENAME;
      sort_criteria = FD_COMPARE_FILENAME;
      break;
   }

   ArrowDir = Header->getArrowDir(header_index);
   // Get the ArrowDir and set descending correctly.
   if ( (ArrowDir == MAYBE) || (ArrowDir == TRUE) ) {
      ArrowDir = FALSE;
      descending = false;
   }
   else {
      ArrowDir = TRUE;
      descending = true;
   }

   // Remove direction in all the columns.
   for (int i = 0; i < HEADER_FILESEARCH_COUNT; i++) {
      Header->setArrowDir(i, MAYBE);
   }

   // Turn on the correct Arrow Direction in the header index.
   Header->setArrowDir(header_index, ArrowDir);
 
   heapSortFileSearchTab(sort_criteria, descending);

   // Show 100 % => all is well.
   ProgressBar->setTotal(1);
   ProgressBar->setProgress(1);

   return(1);

}

// handler for clicking set Nick Name in the Nick Menu.
long TabBookWindow::onSetNickName(FXObject*, FXSelector, void *) {
FXString NewNick;
FXString label;
char *message;

   TRACE();

   COUT(cout << "onSetNickName" << endl;)
   message = new char[strlen(Nick) + 64];
   sprintf(message, "%s, Change Nick to: ", Nick);
   label = message;

   // Here we pop up a FXInPutDialog::getString() dialog
   if (FXInputDialog::getString(NewNick, this, "Set Nick Name", label, NULL) &&
       (NewNick.length() > 0) ) {
      Helper H;

      H.init(XGlobal);

      // Record the New Nick, not yet, its a proposal.
      // delete [] Nick;
      // Nick = new char[strlen(NewNick.text()) + 1];
      // strcpy(Nick, NewNick.text());

      // Issue the change nick.
      sprintf(message, "NICK %s", NewNick.text());
      XGlobal->IRC_ToServerNow.putLine(message);

      // Change it in XGlobal ProposedNick.
      XGlobal->putIRC_ProposedNick(Nick);

      // Save value in Config File. Not yet, its a proposal.
      // H.writeIRCConfigFile();
   }

   delete [] message;
}

// handler for clicking set Nick Pass in the Nick Menu.
long TabBookWindow::onSetNickPass(FXObject*, FXSelector, void *) {
FXString NewPass;
FXString label;
char *message;

   TRACE();

   COUT(cout << "onSetNickPass" << endl;)

   message = new char[strlen(NickPass) + 64];
   sprintf(message, "Pass: %s, Change Pass to: ", NickPass);
   label = message;
   
   // Here we pop up a FXInPutDialog::getString() dialog 
   // Password can be empty.
   if (FXInputDialog::getString(NewPass, this, "Set Nick Pass", label, NULL)) {
   Helper H;

      H.init(XGlobal);

      // Record the new password.
      delete [] NickPass;
      NickPass = new char[strlen(NewPass.text()) + 1];
      strcpy(NickPass, NewPass.text());

      // Attempt to identify only if its non zero in length.
      if (strlen(NickPass)) {
         sprintf(message, "PRIVMSG NickServ :identify %s", NickPass);
         XGlobal->IRC_ToServerNow.putLine(message);
      }

      // Change it in XGlobal too.
      XGlobal->putIRC_Password(NickPass);

      // Save value in Config File.
      H.writeIRCConfigFile();
   }

   delete [] message;
}

// handler for setting the FXMenuPane and label on change in Connection Type
long TabBookWindow::onConnectionTypeChanged(FXObject*, FXSelector, void *) {
char *message;
char *menu_text;
Helper H;

   TRACE();

   COUT(cout << "onConnectionTypeChanged: " << ConnectionType << endl;)
   message = new char[256];

   // Set the MenuPane options show/hide, change label
   // depending on the Connection Type.
   // ConnectionCaption is the FXMenuCaption to be changed.
   switch (ConnectionType) {
      case CM_PROXY:
      strcpy(message, "Type:   Proxy");
      break;

      case CM_BNC:
      strcpy(message, "Type:   BNC");
      break;

      case CM_SOCKS4:
      strcpy(message, "Type:   Socks 4");
      break;

      case CM_SOCKS5:
      strcpy(message, "Type:   Socks 5");
      break;

      case CM_DIRECT:
      default:
      H.init(XGlobal);
      strcpy(message, "Type:   Direct");
      CM.setDirect();
      XGlobal->putIRC_CM(CM);
      H.writeConnectionConfigFile();
      break;
   }

   // Change the Caption.
   ConnectionCaption->setText(message);

   if (ConnectionType == CM_DIRECT) {
      // Hide all the FXMenuCommands
      MenuConnectionTypeHost->hide();
      MenuConnectionTypePort->hide();
      MenuConnectionTypeUser->hide();
      MenuConnectionTypePass->hide();
      MenuConnectionTypeVHost->hide();
   }
   else if ( (ConnectionType > CM_DIRECT) && (ConnectionType <= CM_SOCKS5) ) {
      sprintf(message, "Host: %s    ->", ConnectionHost.text());
      COUT(cout << "Host: " << message << endl;)
      MenuConnectionTypeHost->setText(message);
      MenuConnectionTypeHost->show();

      sprintf(message, "Port: %d    ->", ConnectionPort);
      MenuConnectionTypePort->setText(message);
      MenuConnectionTypePort->show();

      if (ConnectionType != CM_BNC) { // BNC has no User
         sprintf(message, "User: %s    ->", ConnectionUser.text());
         MenuConnectionTypeUser->setText(message);
         MenuConnectionTypeUser->show();
      }
      else {
         MenuConnectionTypeUser->hide();
      }

      if (ConnectionType != CM_SOCKS4) { // Socks 4 has no Password
         sprintf(message, "Pass: %s    ->", ConnectionPass.text());
         MenuConnectionTypePass->setText(message);
         MenuConnectionTypePass->show();
      }
      else {
         MenuConnectionTypePass->hide();
      }
   }
   if (ConnectionType == CM_BNC) {
      sprintf(message, "VHost: %s    ->", ConnectionVHost.text());
      MenuConnectionTypeVHost->setText(message);
      MenuConnectionTypeVHost->show();
   }
   else {
      MenuConnectionTypeVHost->hide();
   }

   //connectionmenu->resize(connectionmenu->getDefaultWidth(), connectionmenu->getDefaultHeight());
   delete [] message;

   return(1);
}

// handler for getting the inputs for the Connection Type
// Only come here when we are NOT CM_DIRECT
long TabBookWindow::onConnectionValuesChanged(FXObject*, FXSelector Sel, void *) {
FXint Sel_ID;
FXString Str_Result;
FXint Int_Result;
bool values_changed = false;
Helper H;
char *message;

   TRACE();

   COUT(cout << "onConnectionValuesChanged:" << endl;)

   Sel_ID = FXSELID(Sel);

   message = new char[256];
   switch (Sel_ID) {
      case ID_CONNECTION_HOST:
      // Pop up a FXInputDialog for entry.
      sprintf(message, "Host: %s. Enter New Host:", ConnectionHost.text());
      if (FXInputDialog::getString(Str_Result, this, "Set Host", message, NULL) && (Str_Result.length() > 0) ) {
         // Save this host
         ConnectionHost = Str_Result;
         values_changed = true;
      }
      break;

      case ID_CONNECTION_PORT:
      sprintf(message, "Port: %d. Enter New Port:", ConnectionPort);
      if (FXInputDialog::getInteger(Int_Result, this, "Set Port", message, NULL, 1, 65535)) {
         // Save this Port.
         ConnectionPort = Int_Result;
         values_changed = true;
      }
      break;

      case ID_CONNECTION_USER:
      sprintf(message, "User: %s. Enter New User:", ConnectionUser.text());
      if (FXInputDialog::getString(Str_Result, this, "Set User", message, NULL) && (Str_Result.length() > 0) ) {
         // Save this User
         ConnectionUser = Str_Result;
         values_changed = true;
      }
      break;

      case ID_CONNECTION_PASS:
      sprintf(message, "Pass: %s. Enter New Pass:", ConnectionPass.text());
      if (FXInputDialog::getString(Str_Result, this, "Set Pass", message, NULL) && (Str_Result.length() > 0) ) {
         // Save this Pass
         ConnectionPass = Str_Result;
         values_changed = true;
      }
      break;

      case ID_CONNECTION_VHOST:
      sprintf(message, "VHost: %s. Enter New VHost:", ConnectionVHost.text());
      if (FXInputDialog::getString(Str_Result, this, "Set VHost", message, NULL) && (Str_Result.length() > 0) ) {
         // Save this VHost
         ConnectionVHost = Str_Result;
         values_changed = true;
      }
      break;

   }
   delete [] message;

   if (values_changed == false) return(1);

   // Now depending on the ConnectionType, save CM accordingly with the values.
   // and write to the Config File.

   switch (ConnectionType) {
      case CM_PROXY:
      CM.setProxy(ConnectionHost.text(),
                  ConnectionPort,
                  ConnectionUser.text(),
                  ConnectionPass.text()
                 );
      break;

      case CM_BNC:
      CM.setBNC(ConnectionHost.text(),
                ConnectionPort,
                Nick,
                ConnectionPass.text(),
                ConnectionVHost.text()
               );
      break;

      case CM_SOCKS4:
      CM.setSocks4(ConnectionHost.text(),
                   ConnectionPort,
                   ConnectionUser.text()
                  );
      break;

      case CM_SOCKS5:
      CM.setSocks5(ConnectionHost.text(),
                   ConnectionPort,
                   ConnectionUser.text(),
                   ConnectionPass.text()
                  );
      break;
   }

   // Now save CM in XGlobal.
   XGlobal->putIRC_CM(CM);

   // Save It in the Config File.
   H.init(XGlobal);
   H.writeConnectionConfigFile();

   // Call onConnectionTypeChanged() to update the labels.
   onConnectionTypeChanged(NULL, 0, NULL);

   return(1);
}

// handler for Disconnecting from IRC. to reconnect.
long TabBookWindow::onConnectionReConnect(FXObject*, FXSelector, void *) {

   TRACE();

   XGlobal->IRC_ToServerNow.putLine("QUIT :Be Right Back... - " CLIENT_NAME_FULL " " DATE_STRING " " VERSION_STRING " - Get it from " CLIENT_HTTP_LINK);
   // The Server should disconnect us hopefully.
}

// Handles the "Check File Integrity" in the pop up in the SearchUI TABs.
// Sel_ID is one of:
// - ID_FILESEARCH_CHECK_INTEGRITY
// - ID_DOWNLOADS_CHECK_INTEGRITY
// - ID_WAITING_CHECK_INTEGRITY
// Used to get the index into item to extract the relevant information.
void TabBookWindow::checkFileIntergrity(FXint Sel_ID) {
int win_index;
FXFoldingItem *selection;
FXint nick_index;
FXint filename_index;
FXString ItemStr;
LineParse LineP;
const char *parseptr;
char *full_filename;
size_t file_size;
char *buffer;
time_t cur_time;
CSHA1 SHA;

   TRACE();

   // The window in focus is where we are working.
   win_index = FocusIndex;

   switch (Sel_ID) {
      case ID_FILESEARCH_CHECK_INTEGRITY:
      nick_index = HEADER_FILESEARCH_NICK + 1;
      filename_index = HEADER_FILESEARCH_FILENAME + 1;
      break;

      case ID_DOWNLOADS_CHECK_INTEGRITY:
      nick_index = HEADER_DOWNLOADS_NICK + 1;
      filename_index = HEADER_DOWNLOADS_FILENAME + 1;
      break;

      case ID_WAITING_CHECK_INTEGRITY:
      nick_index = HEADER_WAITING_NICK + 1;
      filename_index = HEADER_WAITING_FILENAME + 1;
      break;

      default:
      // Shouldnt come here.
      return;
      break;
   }

   selection = Windows[win_index].SearchUI->getCurrentItem();
   if (selection == NULL) return;

   ItemStr = selection->getText();
   parseptr = ItemStr.text();

   LineP = (char *) parseptr;
   LineP.setDeLimiter('\t');

   // Get the nick and check if its MM client.
   parseptr = LineP.getWord(nick_index);

   if (XGlobal->NickList.getMMNickIndex(CHANNEL_MAIN, (char *) parseptr) == 0) {
      // This Nick chosen is currently not in channel or not a known MM client.
      FXMessageBox::information(this, MBOX_OK, "File Integrity Check",
            "This command can only be used with other MasalaMate clients.\n"
            "This nick is not currently known to be a MasalaMate client.\n"
            "Try again later if you are sure this is a MasalaMate client.");
      return;
   }

   // Check if a previous invocation is pending.
   // If XGlobal->SHA1_FileName is NULL => not pending.
   // If XGlobal->SHA1_FileName is non NULL, and cur time > SHA1_Time
   // => not pending.
   // All other cases its pending.
   bool sha1_can = false;
   XGlobal->lock();
   if (XGlobal->SHA1_FileName) {
      cur_time = time(NULL);
      if (cur_time > XGlobal->SHA1_Time) {
         // Clear the stale SHA1 entries.

         delete [] XGlobal->SHA1_FileName;
         XGlobal->SHA1_FileName = NULL;
         delete [] XGlobal->SHA1_SHA1;
         XGlobal->SHA1_SHA1 = NULL;
         sha1_can = true;
      }
   }
   else {
      sha1_can = true;
   }
   XGlobal->unlock();

   if (sha1_can == false) {
      // Message user that we are still waiting on previous Check Integrity.
      FXMessageBox::information(this, MBOX_OK, "File Integrity Check",
            "We are still waiting on a previously issued File Integrity Check\n"
            "Try again later");
      return;
   }

   // We can issue file integrity checks on Partial Files only which reside
   // in the PartialDir.
   parseptr = LineP.getWord(filename_index);
   XGlobal->lock();
   full_filename = new char[strlen(parseptr) + strlen(DIR_SEP) + strlen(XGlobal->PartialDir) + 1];
   sprintf(full_filename, "%s%s%s", XGlobal->PartialDir, DIR_SEP, parseptr);
   XGlobal->unlock();

   // Check if file size > FILE_RESUME_GAP.
   bool retvalb;
   retvalb = getFileSize(full_filename, &file_size);
   if (retvalb == false) {
      // Message user that file doesnt exist on our side.
      FXMessageBox::error(this, MBOX_OK, "File Integrity Check",
            "Cannot do an Integrity Check on a file we do not have\n"
            "The File should reside in the Partial Folder\n");
      delete [] full_filename;
      return;
   }
   if (file_size <= FILE_RESUME_GAP) {
      // Message user that file is too small for Integrity checks.
      FXMessageBox::information(this, MBOX_OK, "File Integrity Check",
            "The file is too small for Integrity Checks\n",
            "If you have problems resuming, just delete this file");
      delete [] full_filename;
      return;
   }

   // Now that we have the full file, size etc, lets prepare for the 
   // grand finale.
   buffer = new char[FILE_RESUME_GAP];
   if (getFileResumeChunk(full_filename, file_size, buffer) == false) {
      // Message user that error in getting rollback data to verify.
      FXMessageBox::error(this, MBOX_OK, "File Integrity Check",
            "Could not extract RollBack Information of File.");
      delete [] full_filename;
      delete [] buffer;
      return;
   }

   // Generate SHA
   SHA.Reset();
   SHA.Update((unsigned char *) buffer, FILE_RESUME_GAP);
   SHA.Final();

   // Fill up all the values in XGlobal.
   XGlobal->lock();
   XGlobal->SHA1_SHA1 = new char[41];
   SHA.ReportHash(XGlobal->SHA1_SHA1);
   XGlobal->SHA1_FileName = new char[strlen(parseptr) + 1];
   strcpy(XGlobal->SHA1_FileName, parseptr);
   XGlobal->SHA1_Time = cur_time + 180; // 180 second timeout.
   XGlobal->SHA1_FileSize = file_size;

   // Prepare the /ctcp to be issued in buffer.
   parseptr = LineP.getWord(nick_index);
   sprintf(buffer, "PRIVMSG %s :\001FILESHA1 %lu %s %s\001",
                    parseptr,
                    XGlobal->SHA1_FileSize,
                    XGlobal->SHA1_SHA1,
                    XGlobal->SHA1_FileName);
   XGlobal->unlock();

   // Send this off to the Server.
   XGlobal->IRC_ToServer.putLine(buffer);

   delete [] full_filename;
   delete [] buffer;
   return;
}


// Handler for TOOLS->Rollback Truncate.
long TabBookWindow::onToolsRollbackTruncate(FXObject*, FXSelector, void *) {
FilesDetail *FD;

   TRACE();

   COUT(cout << "onToolsRollbackTruncate" << endl;)

   FXFileDialog truncateFile(this, "Rollback Truncate File");
   XGlobal->lock();
   truncateFile.setDirectory(XGlobal->PartialDir);
   XGlobal->unlock();
   truncateFile.setPatternList(FileDialogPatterns);
   if (truncateFile.execute()) {
      FXString fname;
      char *full_fname;
      char *just_filename;
      char *tmpstr;
      size_t file_size;

      fname = truncateFile.getFilename();
      COUT(cout << "File to truncate: " << fname.text() << endl;)
      // This contains the full file name with path.
      full_fname = new char[strlen(fname.text()) + 1];
      strcpy(full_fname, fname.text());
      just_filename = getFileName(full_fname);
      XGlobal->lock();
      tmpstr = new char[strlen(XGlobal->PartialDir) + 2 + strlen(just_filename)];
      sprintf(tmpstr, "%s%s%s", XGlobal->PartialDir, DIR_SEP, just_filename);
      XGlobal->unlock();

      // Check that this file is not downloading currently
      // That is it shouldnt be in DwnldInProgress with Connection Non NULL
      FD = XGlobal->DwnldInProgress.getFilesDetailListMatchingFileName(just_filename);
      if (FD && FD->Connection) {
         FXMessageBox::error(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER, "Rollback Truncate", "You SHOULD NOT truncate a file that is currently downloading!");
         XGlobal->DwnldInProgress.freeFilesDetailList(FD);
         delete [] full_fname;
         delete [] tmpstr;
         return(1);
      }
      XGlobal->DwnldInProgress.freeFilesDetailList(FD);
      FD = NULL;

      delete [] full_fname;

      // Check if this file exists.
      if (getFileSize(tmpstr, &file_size)) {

         // We can now truncate this guy. Give warning.
         if (file_size < FILE_RESUME_GAP) {
            FXMessageBox::error(this, MBOX_OK, "Rollback Truncate",
              "File size is less that 8K - Not Truncating\n"
              "Recommendation - Just delete the file.");
         }
         else if (FXMessageBox::warning(this, MBOX_YES_NO, "Rollback Truncate",
                  "You should only do this to a file in your Partial Directory which you are having problems Resuming.\n"
                  "Pressing OK will truncate the file by about 8K. It does not guarantee that your File will be Resumable.\n"
                  "Do not repeatedly try it on the same file. After truncating, ReIssue \"Check File Integrity\"")
                  == MBOX_CLICKED_YES) {
            // Truncate the file.
            
            truncate(tmpstr, file_size - FILE_RESUME_GAP);
         }
      }
      else {
         // Inform user that we work only in files in the Partial Folder.
         FXMessageBox::information(this, MBOX_OK, "Rollback Truncate",
             "Rollback Truncate, can only be done to files residing in the Partial Folder");
      }
      delete [] tmpstr;
   }
   return(1);
}

// handler for File -> Upgrade Server.
// Handles the ID_UPGRADE_SERVER message.
long TabBookWindow::onUpgradeServer(FXObject*, FXSelector, void *) {
DIR *d;
char *full_file_name;
size_t file_size;
bool retvalb;
char *UpgradeProgramNames[] =
{
UPGRADE_PROGRAM_NAME_WIN_X86_32,
UPGRADE_PROGRAM_NAME_WIN_X86_64,
UPGRADE_PROGRAM_NAME_LINUX_X86_32,
UPGRADE_PROGRAM_NAME_LINUX_X86_64,
UPGRADE_PROGRAM_NAME_APPLE_PPC_32,
UPGRADE_PROGRAM_NAME_APPLE_PPC_64,
UPGRADE_PROGRAM_NAME_APPLE_X86_32,
UPGRADE_PROGRAM_NAME_APPLE_X86_64,
UPGRADE_PROGRAM_NAME_LINUX_PPC_32,
UPGRADE_PROGRAM_NAME_LINUX_PPC_64,
""
};

   TRACE();

   COUT(cout << "onUpgradeServer:" << endl;)

   // Toggle state
   retvalb = UpgradeServerMenuCheck->getCheck();

   XGlobal->lock();
   XGlobal->UpgradeServerEnable = false;
   XGlobal->unlock();

   if (retvalb == false) {
      return(1);
   }

   // We just assume he cant be a Upgrade Server and set state as such.
   UpgradeServerMenuCheck->setCheck(FALSE);

   // He has set it to true. Lets see if all prerequisites are met.
   // 1st Prerequisite: Should be OP in im and mc.
   if (!IS_OP(XGlobal->NickList.getNickMode(CHANNEL_MAIN, Nick)) ||
       !IS_OP(XGlobal->NickList.getNickMode(CHANNEL_CHAT, Nick))) {

      FXMessageBox::error(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER, 
            "Upgrade Server",
            "You are not an OP in the required Channel to act as an Upgrade Server.");
      // UpgradeServerMenuCheck->hide();
      return(1);
   }

   // 2nd Prerequisite: Upgrade Directory to exist.
   d = opendir(UPGRADE_DIR);
   if (d == NULL) {
      FXMessageBox::error(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER,
            "Upgrade Server",
            "Upgrade Directory does not exist!");
      return(1);
   }
   closedir(d);

   // 3rd Prerequisite: All builds to exist. Currently 8 defined.
   // For ports we dont have just touch the file in Upgrade Directory.
   int upgrade_index = 0;
   while (strlen(UpgradeProgramNames[upgrade_index])) {
      full_file_name = new char[strlen(UPGRADE_DIR) + strlen(DIR_SEP) + strlen(UpgradeProgramNames[upgrade_index]) + 1];
      sprintf(full_file_name, "%s%s%s", UPGRADE_DIR, DIR_SEP, UpgradeProgramNames[upgrade_index]);
      if (getFileSize(full_file_name, &file_size) == false) {
         char UpgradeMessage[1024];
         sprintf(UpgradeMessage, "Upgrade File: %s does not exist!", UpgradeProgramNames[upgrade_index]);
         FXMessageBox::error(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER,
                             "Upgrade Server", UpgradeMessage);
         delete [] full_file_name;
         return(1);
      }
      delete [] full_file_name;
      upgrade_index++;
   }

   // All OK.
   UpgradeServerMenuCheck->setCheck(TRUE);

   XGlobal->lock();
   XGlobal->UpgradeServerEnable = true;
   XGlobal->unlock();
}

// ID_TOOLS_CHECK_UPGRADE handler.
long TabBookWindow::onToolsCheckUpgrade(FXObject*, FXSelector, void *) {
   TRACE();

   COUT(cout << "onToolsCheckUpgrade" << endl;)
   // GUI trigger.
   triggerUpgrade(true);

   return(1);
}

// Initiates the upgrade progress.
// If our UpgradeServerEnable is true, we dont trigger.
// gui = true => triggered from gui.
void TabBookWindow::triggerUpgrade(bool gui) {
char sha[41];
int chat_win_index;
FXint nick_count;
FXString ItemText;
const char *charp;
char Response[256];
unsigned long ip;
char DottedIP[20];

   TRACE();

   COUT(cout << "triggerUpgrade" << endl;)

   XGlobal->lock();
   if (XGlobal->UpgradeServerEnable == true) {
      XGlobal->unlock();
      if (gui) {
         FXMessageBox::error(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER,
            "Check And Upgrade",
            "We are the Upgrade Server. Cannot request an upgrade!");
      }
      return;
   }
   XGlobal->unlock();

   // If UpgradeTime is > current time. An upgrade is in progress.
   // So wont reissue.
   XGlobal->lock();
   if (XGlobal->Upgrade_Time > time(NULL)) {
      XGlobal->unlock();
      if (gui) {
         FXMessageBox::error(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER,
            "Check And Upgrade",
            "An Upgrade Process is still in Progress.\n"
            "Please try after half an hour");
      }
      return;
   }
   XGlobal->unlock();

#ifdef __MINGW32__
   if (getSHAOfFile(PROGRAM_NAME_WINDOWS, sha) == false) {
#else
   if (getSHAOfFile(PROGRAM_NAME_NON_WINDOWS, sha) == false) {
#endif
      if (gui) {
         FXMessageBox::error(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER,
            "Check And Upgrade",
            "Couldnt generate SHA of running binary");
      }
      return;
   }

   // Dump sha in XGlobal->Upgrade_SHA so we can compare with received value.
   XGlobal->lock();
   delete [] XGlobal->Upgrade_SHA;
   XGlobal->Upgrade_SHA = new char[strlen(sha) + 1];
   strcpy(XGlobal->Upgrade_SHA, sha);
   XGlobal->unlock();

   // Delete the possible binary which might be received from 
   // Partial and Serving.
   XGlobal->lock();
   sprintf(Response, "%s%s%s", XGlobal->ServingDir[0], DIR_SEP,
           UPGRADE_PROGRAM_NAME);
   XGlobal->unlock();
   if (unlink(Response) == 0) {
      Helper H;
      // We just deleted, update MyFilesDB.
      H.init(XGlobal);
      H.generateMyFilesDB();
   }

   XGlobal->lock();
   sprintf(Response, "%s%s%s", XGlobal->PartialDir, DIR_SEP,
           UPGRADE_PROGRAM_NAME);
   XGlobal->unlock();
   if (unlink(Response) == 0) {
      Helper H;
      // We just deleted, update MyPartialFilesDB.
      H.init(XGlobal);
      H.generateMyPartialFilesDB();
   }

   ip = XGlobal->getIRC_IP(NULL);
   XGlobal->resetIRC_IP_Changed();

   // We issue a /ctcp UPGRADE OS SHA to UpgradeMM if he is op in chat channel
   if (IS_OP(XGlobal->NickList.getNickMode(CHANNEL_CHAT, UPGRADE_OP_NICK))) {
      if (strcasecmp(Nick, UPGRADE_OP_NICK) ) {
         // Send the UPGRADE CTCP to this nick if its not us ourselves.
         sprintf(Response, "PRIVMSG %s :\001UPGRADE %s %lu %s\001",
            UPGRADE_OP_NICK,
            sha,
            ip,
            UPGRADE_PROGRAM_NAME
            );
         XGlobal->IRC_ToServer.putLine(Response);
         sprintf(Response, "Server 07UPGRADE: Sending Upgrade Request to %s",
                  UPGRADE_OP_NICK);
         XGlobal->IRC_ToUI.putLine(Response);
      }
   }
   else {
      if (gui) {
         XGlobal->IRC_ToUI.putLine("*UPGRADE* NOUPGRADER");
      }
   }

}

// Handles the GUI upgrade Notify information.
// notify_message one of DONE, NOTREQUIRED, NOUPGRADER
void TabBookWindow::upgradeNotify(char *notify_message) {
char *charp1 = NULL, *charp2 = NULL;
   TRACE();

   charp1 = new char[128];
   charp2 = new char[128];

   if (strcasecmp(notify_message, "DONE") == 0) {
      strcpy(charp1, "Server 09UPGRADE: Completed Successfully");
      strcpy(charp2, "An Upgrade Process completed successfully\n"
                     "Please restart your client.");
   }
   else if (strcasecmp(notify_message, "NOTREQUIRED") == 0) {
      strcpy(charp1, "Server 09UPGRADE: No Upgrade Required. Your Client is up to date.");
      strcpy(charp2, "No Upgrade is Required\n"
                     "You are currently running the latest version.");
   }
   else if (strcasecmp(notify_message, "NOUPGRADER") == 0) {
      strcpy(charp1, "Server 09UPGRADE: No Upgrader Detected in Channel.");
      strcpy(charp2, "No Upgrader is present in the Channel Currently\n"
                     "Please try again later.");
   }
   if (charp1) {
      XGlobal->IRC_ToUI.putLine(charp1);
   }
   if (charp2) {
      FXMessageBox::information(this, MBOX_OK|DECOR_TITLE|DECOR_BORDER,
            "Upgrade",
            charp2);
   }
   delete [] charp1;
   delete [] charp2;
}

// handler for TOOLS->Requent Unban
long TabBookWindow::onToolsRequestUnban(FXObject*, FXSelector, void *) {
char Response[512];
IRCChannelList CL;

   TRACE();

   CL = XGlobal->getIRC_CL();
   XGlobal->resetIRC_CL_Changed();

   if (CL.isJoined(CHANNEL_MAIN) == true) {
      XGlobal->IRC_ToUI.putLine("Server 04UNBAN: Am currently joined in Channel");
   }
   else {
      Helper H;
      char color_coded_nick[512];
      H.init(XGlobal);

      H.generateColorCodedNick(CHANNEL_CHAT, Nick, color_coded_nick);
      sprintf(Response, "%s %s !unban", CHANNEL_CHAT, color_coded_nick);
      XGlobal->IRC_ToUI.putLine(Response);
      sprintf(Response, "PRIVMSG %s :!unban", CHANNEL_CHAT);
      XGlobal->IRC_ToServer.putLine(Response);

      joinChannels(XGlobal);
   }
}

// Handler for TOOLS->Set Tray Password.
// We just update XGlobal->TrayPassword if a password is set.
long TabBookWindow::onToolsSetTrayPassword(FXObject*, FXSelector, void *) {
FXString result;

   TRACE();

   // Ask for password which is to be set.
   if (FXInputDialog::getString(result, this, "Set Tray Password", "Set Top Secret Password: ", NULL)) {
      XGlobal->lock();
      delete [] XGlobal->TrayPassword;
      XGlobal->TrayPassword = new char[result.length() + 1];
      strcpy(XGlobal->TrayPassword, result.text());
      XGlobal->unlock();

      // Write this out to the Config file.
      Helper H;
      H.init(XGlobal);
      H.writeIRCConfigFile();
   }

   return(1);
}


// These will save the highlighted items in SelectedItems
// Additionally save the expanded state of the root nodes.
// Will be used by restoreSelections to restore.
// Should be called from within the same update* functions.
void TabBookWindow::saveSelections(FXint index) {
FXint sel_number, exp_number;
FXFoldingItem *selection;
FXint i;

   TRACE();

   // delete the array in case
   delete [] SaveRestoreSelectedItems;
   SaveRestoreSelectedItems = NULL;
   delete [] SaveRestoreExpandedItems;
   SaveRestoreExpandedItems = NULL;
   SaveRestoreCurrentItem = 0;

   // Lets note down the items that have been selected.
   SaveRestoreSelectedItemCount = 0;
   SaveRestoreExpandedItemCount = 0;

   selection = Windows[index].SearchUI->getFirstItem();
   // Lets get the count of items selected.
   while (selection) {
      if (selection->isSelected()) {
         SaveRestoreSelectedItemCount++;
      }
      if (selection->isExpanded()) {
         SaveRestoreExpandedItemCount++;
      }
      selection = selection->getNext();
   }

   // Lets now save the items selected and the Expanded ones.
   if (SaveRestoreSelectedItemCount) {
      SaveRestoreSelectedItems = new FXint[SaveRestoreSelectedItemCount];
   }
   if (SaveRestoreExpandedItemCount) {
      SaveRestoreExpandedItems = new FXint[SaveRestoreExpandedItemCount];
   }

   selection = Windows[index].SearchUI->getFirstItem();

   sel_number = 0;
   exp_number = 0;
   i = 0;
   while (selection) {
      if (  Windows[index].SearchUI->isItemSelected(selection) &&
            (sel_number < SaveRestoreSelectedItemCount) ) {
         SaveRestoreSelectedItems[sel_number] = i;
         // COUT(cout << "saveSelections: SaveRestoreSelectedItems[" << sel_number << "] = " << i << endl;)
         sel_number++;
      }
      if ( Windows[index].SearchUI->isItemExpanded(selection) &&
           (exp_number < SaveRestoreExpandedItemCount) ) {
         SaveRestoreExpandedItems[exp_number] = i;
         // COUT(cout << "saveExpanded: SaveRestoreExpandedItems[" << exp_number << "] = " << i << endl;)
         exp_number++;
      }
      if (Windows[index].SearchUI->isItemCurrent(selection)) {
         SaveRestoreCurrentItem = i;
      }
      i++;
      selection = selection->getNext();
   }
}

// These will restore the highlighted items in SelectedItems
// which should have been saved by a call to saveSelections()
// Should be called from within the same update* functions.
void TabBookWindow::restoreSelections(FXint index) {
FXFoldingItem *selection;
FXint sel_number, exp_number;
FXint i, sel_index, exp_index;

   TRACE();

   selection = Windows[index].SearchUI->getFirstItem();

   sel_number = -1;
   exp_number = -1;
   sel_index = 0;
   exp_index = 0;
   i = 0;
   while (selection) {
      TRACE();
//      if ( (SaveRestoreSelectedItemCount > 0) && (i > sel_number) ) {
      if ( (SaveRestoreSelectedItemCount > sel_index) && (i > sel_number) ) {
         TRACE();
         sel_number = SaveRestoreSelectedItems[sel_index];
         sel_index++;
      }
//      if ( (SaveRestoreExpandedItemCount > 0) && (i > exp_number) ) {
      if ( (SaveRestoreExpandedItemCount > exp_index) && (i > exp_number) ) {
         TRACE();
         exp_number = SaveRestoreExpandedItems[exp_index];
         exp_index++;
      }
      if (i == sel_number) {
         TRACE();
         Windows[index].SearchUI->selectItem(selection);
      }
      if (i == exp_number) {
         TRACE();
         Windows[index].SearchUI->expandTree(selection);
      }
      if (i == SaveRestoreCurrentItem) {
         TRACE();
         Windows[index].SearchUI->setCurrentItem(selection);
      }
      TRACE();
      selection = selection->getNext();
      i++;
   }

   // delete the array in case
   delete [] SaveRestoreSelectedItems;
   SaveRestoreSelectedItems = NULL;
   SaveRestoreSelectedItemCount = 0;
   delete [] SaveRestoreExpandedItems;
   SaveRestoreExpandedItems = NULL;
   SaveRestoreExpandedItemCount = 0;
   SaveRestoreCurrentItem = 0;

   TRACE(); // Crash happens in this function. So put a leaving trace.
}

// Used by the ServingDir related message handlers to sync menu
// with XGlobal->ServingDir[]
void TabBookWindow::syncDirMenuWithServingDir() {
bool SetEnabled = false;

   TRACE();

   XGlobal->lock();
   for (int i = 0; i < FSERV_MAX_SERVING_DIRECTORIES; i++) {
      if (XGlobal->ServingDir[i]) {
         // this index exists => can Open, can Set, can UnSet.
         // We do not allow unsetting Index 0. compulsory serving
         MenuOpenServingDir[i]->show();
         MenuSetServingDir[i]->show();
         if (i == 0) {
            MenuUnSetServingDir[i]->hide();
         }
         else {
            MenuUnSetServingDir[i]->show();
         }
      }
      else {
         // this index doesnt exist => cannot Open, can Set, cannot UnSet.
         // Leave only one, the smallest slot open for Setting.
         MenuOpenServingDir[i]->hide();
         if (SetEnabled == false) {
            MenuSetServingDir[i]->show();
            SetEnabled = true;
         }
         else {
            MenuSetServingDir[i]->hide();
         }
         MenuUnSetServingDir[i]->hide();
      }
   }
   XGlobal->unlock();
}

// Put the new label in the Chat window, with the nick, which is input param
void TabBookWindow::updateDCCChatLabel(char *new_label) {
int dcc_windex;

   TRACE();

   if ( (new_label == NULL) || (strlen(new_label) == 0) ) return;

   dcc_windex = getWindowIndex(TAB_DCC_CHAT);
   Windows[dcc_windex].LabelUI->setText(new_label);
}

// The Tray Button in Window is pressed to hide in Tray.
long TabBookWindow::onTrayButton(FXObject *, FXSelector, void *) {
   TRACE();

   COUT(cout << "On TrayButton" << endl;)

   this->hide();

   return(1);
}

// When Tray is clicked we do nothing.
long TabBookWindow::onTraySingleClicked(FXObject *, FXSelector, void *) {
   TRACE();

   return(1);
}

// When Tray is double clicked we just show the window. or hide it.
long TabBookWindow::onTrayDoubleClicked(FXObject *, FXSelector, void *) {
FXString result;
FXInputDialog FID(this, "Keyboard Translator", "Enter Top Secret Password: ", NULL, INPUTDIALOG_STRING|INPUTDIALOG_PASSWORD);
char *TP;

   TRACE();

   // We need to ask for password only if window is currently not shown.
   if (this->shown() == TRUE) {
      this->hide();
      return(1);
   }

   // Now window is shown, so ask for password only if existing password
   // is not null sting or NULL.
   bool show_window = false;
   XGlobal->lock();
   if ( (XGlobal->TrayPassword == NULL) ||
        (strlen(XGlobal->TrayPassword) == 0) ) {
      show_window = true;
   }
   XGlobal->unlock();

   // Ask for password.
   if ( (show_window == false) && FID.execute() ) {
      result = FID.getText();
      XGlobal->lock();
      TP = XGlobal->TrayPassword;
      if (TP == NULL) {
         TP = "";
      }
      if (strcasecmp(result.text(), TP) == 0) {
         show_window = true;
      }
      XGlobal->unlock();
   }

   if (show_window) {
      this->show();
   }

   return(1);
}

// When Tray is right clicked we do nothing.
long TabBookWindow::onTrayRightClicked(FXObject *, FXSelector, void *) {
   TRACE();

   return(1);
}
