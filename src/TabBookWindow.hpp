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

#ifndef TABBOOKWINDOWHPP
#define TABBOOKWINDOWHPP

class MasalaMateAppUI;
class XChange;
class HistoryLines;
class FXTray;

#include "ConnectionMethod.hpp"
#include "FilesDetailList.hpp"

#define UI_MAXTABS 20

// Pre created Channels but hidden.
#define UI_FREE_CHANNEL_NAME "FreeChannelTab"

// Window Names.
#define TAB_FILESEARCH       "File Search"
#define TAB_MESSAGES         "Messages"
#define TAB_DCC_CHAT         "DCC-Chat"
#define TAB_SWARM            "Swarm"

// Max Other channels other than MAIN and CHAT that one can join.
#define UI_MAX_OTHER_CHANNEL 5

// The Directory Name String for Representing a File in the Partial directory.
#define UI_PARTIAL_DIRNAME  "--8<-- PARTIAL -->8--"

// Lets define the HEADER item index for the headings in the File Search TAB
// These assume first one is 0, so in getWord() use + 1, with \t as delimiter.
#define HEADER_FILESEARCH_COUNT               9   // 0 thru 8
#define HEADER_FILESEARCH_FILENAME            0
#define HEADER_FILESEARCH_FILESIZE            1
#define HEADER_FILESEARCH_DIRNAME             2
#define HEADER_FILESEARCH_NICK                3
#define HEADER_FILESEARCH_SENDS               4
#define HEADER_FILESEARCH_QUEUES              5
#define HEADER_FILESEARCH_FIREWALLED          6
#define HEADER_FILESEARCH_CLIENT              7
#define HEADER_FILESEARCH_NUMBER              8

#define HEADER_WAITING_COUNT                  8
#define HEADER_WAITING_FILENAME               0
#define HEADER_WAITING_FILESIZE               1
#define HEADER_WAITING_NICK                   2
#define HEADER_WAITING_QUEUE                  3
#define HEADER_WAITING_RESEND                 4
#define HEADER_WAITING_RATE                   5
#define HEADER_WAITING_PERCENT                6
#define HEADER_WAITING_ETA                    7

#define HEADER_DOWNLOADS_COUNT                8
#define HEADER_DOWNLOADS_FILENAME             0
#define HEADER_DOWNLOADS_FILESIZE             1
#define HEADER_DOWNLOADS_CURSIZE              2
#define HEADER_DOWNLOADS_PROGRESS             3
#define HEADER_DOWNLOADS_RATE                 4
#define HEADER_DOWNLOADS_TIMELEFT             5
#define HEADER_DOWNLOADS_NICK                 6
#define HEADER_DOWNLOADS_NUMBER               7

#define HEADER_FILESERVER_COUNT               8
#define HEADER_FILESERVER_FILENAME            0
#define HEADER_FILESERVER_FILESIZE            1
#define HEADER_FILESERVER_NICK                2
#define HEADER_FILESERVER_RATE                3
#define HEADER_FILESERVER_PROGRESS            4
#define HEADER_FILESERVER_TIMELEFT            5
#define HEADER_FILESERVER_QUEUE               6
#define HEADER_FILESERVER_QUEUETYPE           7  // BIG or SMALL

// Note there are two entries with 0, as its same header.
// Same for entry 4.
#define HEADER_SWARM_COUNT                    6
#define HEADER_SWARM_FILENAME                 0
#define HEADER_SWARM_NICK                     0
#define HEADER_SWARM_FILESIZE                 1
#define HEADER_SWARM_RATE_UP                  2
#define HEADER_SWARM_RATE_DOWN                3
#define HEADER_SWARM_PROGRESS                 4
#define HEADER_SWARM_STATE                    4
#define HEADER_SWARM_FILEOFFSET               5

// The Text in combo box in the FileSearch Tab.
#define FILESEARCH_COMBOBOX_OPEN              "Open"
#define FILESEARCH_COMBOBOX_ANY               "Any"
#define FILESEARCH_COMBOBOX_YES               "Yes"
#define FILESEARCH_COMBOBOX_NO                "No"
#define FILESEARCH_COMBOBOX_MM                "MasalaMate"
#define FILESEARCH_COMBOBOX_SYSRESET          "SysReset"
#define FILESEARCH_COMBOBOX_IROFFER           "Iroffer"
#define FILESEARCH_COMBOBOX_BYTES             " B"
#define FILESEARCH_COMBOBOX_KBYTES            "KB"
#define FILESEARCH_COMBOBOX_MBYTES            "MB"
#define FILESEARCH_COMBOBOX_GBYTES            "GB"
#define FILESEARCH_COMBOBOX_AVI               "AVI"
#define FILESEARCH_COMBOBOX_MPG               "MPG"
#define FILESEARCH_COMBOBOX_WMV               "WMV"
#define FILESEARCH_COMBOBOX_RM                "RM"
#define FILESEARCH_COMBOBOX_3GP               "3GP"
#define FILESEARCH_COMBOBOX_IMAGES            "Images"

// Update Downloads/File Server Tab every 5 seconds.
#define PERIODIC_UI_UPDATE_TIMER 5000

// Main Window
class TabBookWindow : public FXMainWindow {
   FXDECLARE(TabBookWindow)

public:
   //TabBookWindow(FXApp* a, XChange *XG);
   TabBookWindow(MasalaMateAppUI* a, XChange *XG);
   virtual void create();

   // Return MasalaMateAppUI application
   MasalaMateAppUI* getApp() const { return (MasalaMateAppUI*)FXMainWindow::getApp(); }

   virtual ~TabBookWindow();

// This one updates the correct FXText Window with the text as input.
// First word of vartext is the window name.
   void updateFXText(char *vartext);

// Change the Label of TAB_DCC_CHAT window to the input.
   void updateDCCChatLabel(char *new_label);

// This one updates the correct FXList Window with the text as input.
// Firstword starts and ends with *, and contains the NICK command.
   void updateFXList(char *vartext);

// This one updates the Firewalled Button to a NotFirewalled Button.
   void updateToNotFirewalled();


   void updateWaiting(char *title);
   void updateDownloads(char *title);
   void updateFileServer(char *title);
   void updateFileSearch(char *title);
   void updateSwarm(char *title);

// Make new windows for DCC Chats.
   void makeNewChatTabItem(char *windowname);
   int getWindowIndex(char *windowname);
   void replaceWindowNameAtIndex(int win_index, char *towindowname);

#ifndef UI_SEM
   // The function called from a different thread to update the UI text
   // in all windows.
   void updateUI(char *ptr);
#endif

   // Message handlers
   long onCmdPanel(FXObject*,FXSelector,void*);
   long onTextEntry(FXObject*, FXSelector, void *);
   long onExpandTextEntry(FXObject*, FXSelector, void *);
   long onNickSelectPM(FXObject*, FXSelector, void *);
   long onDCNickSelect(FXObject*, FXSelector, void *);
   long onChannelTabToDisplay(FXObject*, FXSelector, void *);
   long onPartialDir(FXObject*, FXSelector, void *);
   long onSetPartialDir(FXObject*, FXSelector, void *);
   long onOpenServingDir(FXObject*, FXSelector, void *);
   long onSetServingDir(FXObject*, FXSelector, void *);
   long onUnSetServingDir(FXObject*, FXSelector, void *);
   long onUpdateServingList(FXObject*, FXSelector, void *);
   long onClock(FXObject*, FXSelector, void *);
   long onFindNextPrevious(FXObject*, FXSelector, void *);

   // Tray related.
   long onTrayButton(FXObject*, FXSelector, void *);
   long onTraySingleClicked(FXObject*, FXSelector, void *);
   long onTrayDoubleClicked(FXObject*, FXSelector, void *);
   long onTrayRightClicked(FXObject*, FXSelector, void *);

// Calls updateWaiting() to update Waiting window if in Focus.
// Calls undateDownloads() to update Downloads window if in Focus.
// Calls updateFileServer() to update File Server window if in Focus
// Calls updateSwam() to update Swarm windows if in Focus.
// This is called periodically - initiated through the FOX addTimeOut()
// We update only one window which is in Focus.
// If none of these are in focus we just return.
   long onPeriodicUIUpdate(FXObject*, FXSelector, void *);

   long onHelp(FXObject*, FXSelector, void *);
   long onHelpDemos(FXObject*, FXSelector, void *);
   long onHelpAbout(FXObject*, FXSelector, void *);
   long onMouseRelease(FXObject*, FXSelector, void *);
   long onFont(FXObject*, FXSelector, void *);
   long onToolBar(FXObject*, FXSelector, void *);
   long onScrollCheck(FXObject*, FXSelector, void *);
#ifdef UI_SEM
   long onUIUpdate(FXObject*, FXSelector, void *);
#endif
   long onQuit(FXObject*, FXSelector, void *);
   long onSearchFile(FXObject*, FXSelector, void *);
   long onSearchPopUp(FXObject*, FXSelector, void *);
   long onSearchHeaderClick(FXObject*, FXSelector, void *);
   long onDownloadsPopUp(FXObject*, FXSelector, void *);
   long onWaitingPopUp(FXObject*, FXSelector, void *);
   long onFileServerPopUp(FXObject*, FXSelector, void *);
   long onSwarmPopUp(FXObject*, FXSelector, void *);

   // handler for clicking set Nick Name in the Nick Menu.
   long onSetNickName(FXObject*, FXSelector, void *);
   // handler for clicking set Nick Pass in the Nick Menu.
   long onSetNickPass(FXObject*, FXSelector, void *);

   // handler for setting the FXMenuPane and label on change in Connection Type
   long onConnectionTypeChanged(FXObject*, FXSelector, void *);

   // handler for getting the inputs for the Connection Type
   long onConnectionValuesChanged(FXObject*, FXSelector, void *);

   // handler for Disconnecting from IRC. to reconnect.
   long onConnectionReConnect(FXObject*, FXSelector, void *);

   // handler for TOOLS->Rollback Truncate.
   long onToolsRollbackTruncate(FXObject*, FXSelector, void *);

   // handler for TOOLS->Check and Upgrade.
   long onToolsCheckUpgrade(FXObject*, FXSelector, void *);

   // handler for TOOLS->Requent Unban
   long onToolsRequestUnban(FXObject*, FXSelector, void *);

   // handler for TOOLS -> Set Tray Password.
   long onToolsSetTrayPassword(FXObject*, FXSelector, void *);

   // handler for File -> Upgrade Server.
   long onUpgradeServer(FXObject*, FXSelector, void *);


   typedef struct UI_Items {
       char               Name[32];
       FXTabItem          *TabUI;
       FXText             *ScrollUI;
       FXFoldingList      *SearchUI;
       FXList             *NickListUI;
       FXSplitter         *ChannelListUIFather;
       FXHorizontalFrame  *BottomFrame;
       FXTextField        *InputUI;
       char               LastTabCompletedNick[64];
       char               LastTabPartialNick[64];
       HistoryLines       *History;
       FXLabel            *LabelUI;
   } UI_Items;

   enum {
      ID_PANEL=FXMainWindow::ID_LAST,
      ID_TEXTINPUT,
      ID_NICKNAME_SET,
      ID_NICKPASS_SET,
      ID_PARTIALDIR,
      ID_SETPARTIALDIR,
      ID_OPENSERVINGDIR,
      ID_SETSERVINGDIR=ID_OPENSERVINGDIR + FSERV_MAX_SERVING_DIRECTORIES,
      ID_UNSETSERVINGDIR=ID_SETSERVINGDIR + FSERV_MAX_SERVING_DIRECTORIES,
      ID_UPDATESERVINGLIST=ID_UNSETSERVINGDIR + FSERV_MAX_SERVING_DIRECTORIES,
      ID_SELECTPMNICK,
      ID_SELECTCHANNELTABTODISPLAY,
      ID_CLOCKTIME,
      ID_BUTTON_TRAY,
      ID_HELP,
      ID_HELPDEMOS,
      ID_HELPABOUT,
      ID_FONT,
      ID_MOUSERELEASE,
      ID_TOOLBAR_CLEAR,
      ID_TOOLBAR_PORTCHECK,
      ID_TOOLBAR_PORTCHECKME,
      ID_TOOLBAR_WHOIS,
      ID_TOOLBAR_PING,
      ID_TOOLBAR_VERSION,
      ID_TOOLBAR_CLIENTINFO,
      ID_TOOLBAR_TIME,
      ID_TOOLBAR_DCCALLOWADD,
      ID_TOOLBAR_DCCALLOWDEL,
      ID_TOOLBAR_DCCSEND,
      ID_TOOLBAR_DCCQUEUE,
      ID_TOOLBAR_DCCCHAT,
      ID_TOOLBAR_OP,
      ID_TOOLBAR_DEOP,
      ID_TOOLBAR_VOICE,
      ID_TOOLBAR_DEVOICE,
      ID_TOOLBAR_KICK,
      ID_TOOLBAR_BAN,
      ID_SCROLLCHECK,
#ifdef UI_SEM
      ID_UI_UPDATE,
#endif
      ID_PERIODIC_UI_UPDATE, // Used with FOX Timer for Downloads/uploads updates.
      ID_BUTTON_SEARCH_FILE, // Here on used with the Search UI.
      ID_BUTTON_SEARCH_NICK,
      ID_POPUP_LISTFILES,
      ID_POPUP_GETLISTING,
      ID_POPUP_SEARCHTEXTINFILES,

      // Below is for PopUp in the File Search TAB
      ID_FILESEARCH_DOWNLOAD,
      ID_FILESEARCH_SEARCH_FILE,
      ID_FILESEARCH_LIST_NICK,
      ID_FILESEARCH_UPDATE_NICK,
      ID_FILESEARCH_CHECK_INTEGRITY,
      ID_FILESEARCH_FEDEX,

      // Header is clicked upon the in File Search TAB
      ID_FILESEARCH_HEADER_CLICK,

      // Below is for PopUp in the Downloads TAB
      ID_DOWNLOADS_REREQUEST,
      ID_DOWNLOADS_CANCEL,
      ID_DOWNLOADS_SEARCH_FILE,
      ID_DOWNLOADS_LIST_NICK,
      ID_DOWNLOADS_UPDATE_NICK,
      ID_DOWNLOADS_CLEAR_PARTIAL_COMPLETE,
      ID_DOWNLOADS_CLEAR_ALL_PARTIAL_COMPLETE,
      ID_DOWNLOADS_CHECK_INTEGRITY,

      // Below is for PopUp in the Waiting TAB
      ID_WAITING_REREQUEST,
      ID_WAITING_CANCEL,
      ID_WAITING_SEARCH_FILE,
      ID_WAITING_LIST_NICK,
      ID_WAITING_UPDATE_NICK,
      ID_WAITING_CHECK_INTEGRITY,

      // Below is for PopUP in the File Server TAB
      ID_FILESERVER_FORCESEND,
      ID_FILESERVER_ABORTSEND,

      // Below is for PopUP in the Swarm TAB
      ID_SWARM_QUIT,

      // Below is for Find Text in Toolbar, search case insensitive from
      // the end of buffer and wraps.
      ID_FIND_TEXT_WRAP_RIGHT,
      ID_FIND_TEXT_WRAP_LEFT,

      // The ConnectionType Data target will get this ID, so we can then
      // set the menus which need to be shown and how they are to be shown.
      ID_CONNECTION_TYPE,

      // ReConnect Menu Option.
      ID_CONNECTION_RECONNECT,

      // The Connection related Menu Commands.
      ID_CONNECTION_HOST,
      ID_CONNECTION_PORT,
      ID_CONNECTION_USER,
      ID_CONNECTION_PASS,
      ID_CONNECTION_VHOST,

      // The TOOLS Menu option.
      ID_TOOLS_ROLLBACK_TRUNCATE,
      ID_TOOLS_CHECK_UPGRADE,
      ID_TOOLS_REQUEST_UNBAN,
      ID_TOOLS_SET_TRAY_PASS,

      // To act as an Upgrade Server.
      ID_UPGRADE_SERVER,

      ID_QUIT_VERIFY
   };


protected:

   // Member data
   UI_Items Windows[UI_MAXTABS]; // Can have max 20 Window TABS as of now.
   FXint    FocusIndex; // Which Window Index currently has the focus.

   // The application object -> Not to be deleted by us.
   // For convenience in accessing the icons.
   MasalaMateAppUI    *App;

   // The Tray object.
   FXTray             *Tray;

   bool               AllowUpdates;
   FXList             *ChannelListUI; // List of channels

   FXToolBar          *toolbar;
   FXMenuBar          *menubar;
   FXMenuPane         *filemenu;
   FXMenuPane         *nickmenu;
   FXMenuPane         *connectionmenu;
   FXMenuPane         *dirmenu;
   FXMenuPane         *fontmenu;
   FXMenuPane         *toolsmenu;
   FXMenuPane         *helpmenu;
   FXTextField        *clock;
   FXTextField        *textsearch;
   FXProgressBar      *ProgressBar;
   FXRadioButton      *firewall;
   FXTabBook          *tabbook;
   XChange	      *XGlobal;
   char               *Nick;
   char               *NickPass;
   FXHiliteStyle      *styles;
   FXFont             *font;
   FXMenuPane         *PopUpMenu;
   FXMenuPane         *PopUpMenuFileSearch;
   FXMenuPane         *PopUpMenuWaiting;
   FXMenuPane         *PopUpMenuDownloads;
   FXMenuPane         *PopUpMenuFileServer;
   FXMenuPane         *PopUpMenuSwarm;
   bool               OpenSendsButton;
   bool               OpenQueuesButton;
   bool               NonFirewalledButton;
   bool               IgnorePartialsButton;
   FXbool             ScrollEnable;
   FXMenuCheck        *UpgradeServerMenuCheck;
   FXbool             UpgradeServerEnable;

   // Data Target and variable to work with the ConnectionType radio buttons
   FXDataTarget       *DataTargetConnectionType;
   FXint              ConnectionType; // This has same value as ConnectionHowE
   FXMenuCaption      *ConnectionCaption;
   ConnectionMethod   CM; // A copy as UI sees it.
   // Contents of CM.
   FXString           ConnectionHost;
   FXint              ConnectionPort;
   FXString           ConnectionUser;
   FXString           ConnectionPass;
   FXString           ConnectionVHost;

   // The Menu Commands in popup related to Connection Type.
   // we need them to be grayed out or active conditionally, hence saving.
   FXMenuCommand      *MenuConnectionTypeHost;
   FXMenuCommand      *MenuConnectionTypePort;
   FXMenuCommand      *MenuConnectionTypeUser;
   FXMenuCommand      *MenuConnectionTypePass;
   FXMenuCommand      *MenuConnectionTypeVHost;

   // The Menu Commands in popup related to OP operations.
   // We need them to be grayed out or active conditionally, hence saving
   FXMenuCascade      *ToolBarOpCascade;
   FXMenuCommand      *ToolBarOpButton;
   FXMenuCommand      *ToolBarDeOpButton;
   FXMenuCommand      *ToolBarVoiceButton;
   FXMenuCommand      *ToolBarDeVoiceButton;
   FXMenuCommand      *ToolBarKickButton;
   FXMenuCommand      *ToolBarBanButton;

   // The Menu Commands in popup related to Serving Directory hide/show.
   FXMenuCommand      *MenuOpenServingDir[FSERV_MAX_SERVING_DIRECTORIES];
   FXMenuCommand      *MenuSetServingDir[FSERV_MAX_SERVING_DIRECTORIES];
   FXMenuCommand      *MenuUnSetServingDir[FSERV_MAX_SERVING_DIRECTORIES];

   // All FileSearch related nuances.
   char               *FileSearchTitle; // As it takes time to recompute it.
   FilesDetail        *FileSearchFD;    // list sync with UI.
   int                FileSearchTotalFD; // Count of entries in FileSearchFD.

   FXHorizontalFrame  *FileSearch2ndBottomFrame;
   FXComboBox         *FileSearchFileTypeComboBox;
   FXTextField        *FileSearchDirNameInputUI;
   FXTextField        *FileSearchNickNameInputUI;
   FXTextField        *FileSearchFileSizeGTInputUI;
   FXComboBox         *FileSearchFileSizeGTComboBox;
   FXTextField        *FileSearchFileSizeLTInputUI;
   FXComboBox         *FileSearchFileSizeLTComboBox;
   FXComboBox         *FileSearchSendsComboBox;
   FXComboBox         *FileSearchQueuesComboBox;
   FXComboBox         *FileSearchFirewalledComboBox;
   FXComboBox         *FileSearchPartialComboBox;
   FXComboBox         *FileSearchClientComboBox;

   TabBookWindow(){} // Need when we map.
   int makeNewChannelTabItem(char *windowname);
   int makeNewServerTabItem(char *windowname);
   void makeNewSearchTabItem(char *windowname);
   FXLabel *makeNewBottomFrameLabel(FXHorizontalFrame *boxframe, char *labelname);
   FXTextField *makeNewBottomFrameInput(FXHorizontalFrame *boxframe);
   FXComboBox  *makeNewBottomFrameComboBox(FXHorizontalFrame *boxframe, FXint ncols, char *str1 = NULL, char *str2 = NULL, char *str3 = NULL, char *str4 = NULL, char *str5 = NULL, char *str6 = NULL, char *str7 = NULL, char *str8 = NULL);
   FXButton *makeNewBottomFrameButton(FXHorizontalFrame *boxframe, char *buttonstring, int selector);
   void displayColorfulText(FXText *UI, char *line);
   int getNewWindowIndex(void);
   void addToNickList(FXList *list, char *element);
   void onCmdPopUp(void);
   void showHidePopUpButtons(void);

   // File Search Sorting related.
   // The criteria is what FilesDetail understands.
   void heapSortFileSearchTab(int criteria, bool descending);

   // The criteria is what FilesDetail understands.
   void heapSortSiftDownFileSearch(FilesDetail **PtrListFD, int criteria, bool descending, int root, int bottom);

   // On clicking "download" in the right click popup menu in "File Search"
   // or clicking "re request" in the right click popup menu in "Downloads"
   void onDownloadSearchUISelected(char *tab_name);

   // On clicking "Cancel selected download" in the right click popup menu
   // in "Downloads" TAB
   void cancelDownloadSelected(void);

   // On clicking the "Clear ..." buttons in the right click popup menu
   // in "Downloads" TAB
   void clearDownloadsSelected(FXint ID);

   // On clicking the "Cancel ..." button in the right click popup menu
   // in "Waiting" TAB
   void cancelWaitingSelected();

   // On clicking the "Send Selected Queue" button in the right click popup
   // menu in "File Server" TAB
   void forceSendFileServerSelected();

   // On clicking the "Abort Selected Send" button in the right click popup
   // menu in "File Server" TAB
   void abortSendFileServerSelected();

   // On clicking the "Quit Swarm" button in the right click popup
   // menu in "Swarm" TAB
   void quitSwarmSelected();

   void doFileSearch();
   // Helper function for doFileSearch()
   bool doFileSearch_FileTypeCheck(char *FileName, char *FileType);

   // Append Items in the SearchUI at win_index, with the FilesDetail passed.
   // file_index is the integer to printed in the # entry.
   // If MarkSelected = true, it marks this as current/selected/visible.
   void appendItemInSearchUI(int win_index, int file_index, FilesDetail *FD, bool MarkSelected);

   // Handles search/list/update in popup menu for the Downloads/Waiting TAB
   void actionOnSearchListUpdate(FXint Sel_ID);

   // Handles the "Check File Integrity" in the pop up in the SearchUI TABs.
   void checkFileIntergrity(FXint Sel_ID);

   // Handles the *UPGRADE* TRIGGER message, triggers the upgrade process.
   // if gui is true give gui messages.
   void triggerUpgrade(bool gui);

   // Handles the GUI upgrade Notify information.
   void upgradeNotify(char *word);

   // Called from the update functions. Called before updates and before 
   // exiting function.
   // Save the selected items in the SearchUI at index;
   void saveSelections(FXint index);
   // Restore the selected items in the SearchUI at index.
   void restoreSelections(FXint index);
   FXint *SaveRestoreSelectedItems;
   FXint SaveRestoreSelectedItemCount;
   FXint *SaveRestoreExpandedItems;
   FXint SaveRestoreExpandedItemCount;
   FXint SaveRestoreCurrentItem;

   // used by the ServingDir related message handlers to sync menu
   // with XGlobal->ServingDir[]
   void syncDirMenuWithServingDir();
};

#endif
