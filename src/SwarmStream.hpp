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

#ifndef SWARMSTREAMHPP
#define SWARMSTREAMHPP

#include "Compatibility.hpp"

#include "SwarmNodeList.hpp"
#include "TCPConnect.hpp"

class SwarmStream {
public:
   // Constructor.
   SwarmStream();

   // Destructor.
   ~SwarmStream();

   bool setSwarmServer(char *Directory, char *FileName);
   bool setSwarmClient(char *Directory, char *FileName);
   bool isSwarmServer();
   bool isBeingUsed();
   bool haveGreatestFileSizeInSwarm();
   bool isFileBeingSwarmed(const char *FileName);
   const char *getSwarmFileName();
   bool generateStringHS(char *HSBuffer);
   bool isMatchingSHA(size_t FileSize, char *FileSHA);
   size_t getSwarmFileSize();
   bool renameNickToNewNick(char *old_nick, char *new_nick);

   // Read Messages from the Connections and update state.
   bool readMessagesAndUpdateState();

   // Write Messages to the Connections and update state.
   bool writeMessagesAndUpdateState();

   // Check for Connections which are dead.
   bool checkForDeadNodes();

   // Check for EndGame Piece.
   bool checkForEndGame();

   // To quit from the Swarm.
   // Returns true if the file is fully downloaded. (so can be moved)
   bool quitSwarm();

   // To get the Error code if any.
   int getErrorCode();

   // To get the Error code as a string.
   void getErrorCodeString(char *ErrorString);

   // To update the CAP values from Global.
   // Syncs up once in 5 seconds.
   bool updateCAPFromGlobal(size_t PerTransferMaxUploadBPS, size_t OverallMaxUploadBPS, size_t PerTransferMaxDownloadBPS, size_t OverallMaxDownloadBPS);

   // The returned array should not be deleted by caller.
   char **getSwarmUIEntries();

   // The Nodes which are Related to this Swarm.
   SwarmNodeList ConnectedNodes;
   SwarmNodeList YetToTryNodes;
   SwarmNodeList TriedAndFailedNodes;

private:

   // Swarm File Info.
   char *Directory;
   char *FileName;
   size_t FileSize;
   size_t MaxKnownFileSize;
   int FileDescriptor;

   char FileSHA[41];
   bool AmServer;
   bool IsBeingUsed;

   // To sync data to disk after so much data is received.
   size_t NextFlushFileSize;

   // For the UI.
   char **UIEntries;
};

#endif
