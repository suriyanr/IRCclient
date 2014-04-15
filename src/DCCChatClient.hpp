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

#ifndef CLASS_DCCCHATCLIENT
#define CLASS_DCCCHATCLIENT

#include "ThreadMain.hpp"

// This Class handles the DCC Chat Client talking interface
// We provide the functionality of extracting a recursive directory
// listing from the Server we are accesing and populate FilesDB.
// We also provide the GET interface.
// Also the interface to remove from Q.
// also a timepass Chatting interface. We can, actually pipe the justChat()
// messages to the "Messages" Tab, and direct Messages tab text to this Chat.
class DCCChatClient {
public:
   DCCChatClient();
   ~DCCChatClient();
   bool getDirListing(DCC_Container_t *);
   bool getFile(DCC_Container_t *);
   bool removeFromQ(DCC_Container_t *);
   bool justChat(DCC_Container_t *);

private:
   void init(DCC_Container_t *);

   bool getDirListingMM();

   void recurGetDirListing(char *current_dir);
   bool extractFileInformation(char *current_dir);
   bool sendLine();  // Send line in RetPointer.
   bool recvLine();  // Recv line in RetPointer.
   bool doCD(char *cur_dir, char *to_dir); // can be DIR1, not DIR1\DIR2
   bool waitForInitialPrompt();
   bool analyseGetResponse(long *q_pos); // Returns q_pos too
   bool analyseSendsResponse();
   bool analyseQueuesResponse();

   char *RetPointer; // For comunicating chat lines.
   char MyNick[64];  // Our Nick, used by getDirListingMM() related functions.

   // FFLC Protocol Version number of server
   int FFLCversion;

   // Below are saved by init(), from what it receives in DCC_Container.
   // The Destructor should not destroy these.
   XChange *XGlobal;
   TCPConnect *Connection;
   char *RemoteNick;
   char *RemoteDottedIP;
   char *TriggerName;
   char ClientType;
   int CurrentSends;
   int TotalSends;
   int CurrentQueues;
   int TotalQueues;
   char *FileName; // Used by getFile() and removeFromQ()

   int FileCount; // Used to check if we need to add dummy as no files present.
                  // Or stop if exceeded DIRLISTING_MAX_FILES
   int DirDepth;  // To not go further than a depth of DIRLISTING_MAX_DEPTH

#if 0
   // A global mutex on all objects of this class.
   // Used to serialise calls to getDirListingMM()
   static MUTEX Mutex_DCCChatClientMM;
#endif
   
};

#endif
