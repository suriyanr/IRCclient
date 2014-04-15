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

#ifndef CLASS_SWARMNODELIST
#define CLASS_SWARMNODELIST

#ifdef __MINGW32__
#  include <windows.h>
#else
#  include <pthread.h>
#  include <semaphore.h>
#endif


// This Class maintains the database of all Swarm Nodes
#include "Compatibility.hpp"

#include "SwarmDataPieceList.hpp"
#include "TCPConnect.hpp"

// Data Structures.
typedef struct SwarmNode {
      char          *Nick;
      unsigned long NodeIP;
      size_t        FileSize;
      char          FileFutureHoles[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1];

      size_t        RequestedFileOffset; // as per DR we recvd from him.
      size_t        RequestedDataLength; // as per DR we recvd from him.

      bool          EndGame; // true => end game piece requested.

      // When it exists in YetToTryNodes, it takes below States.
      #define SWARM_NODE_NOT_TRIED                       00
      #define SWARM_NODE_USERHOST_ISSUED                 01

      // When it exists in TriedAndFailedNodes, it takes below States.
      #define SWARM_NODE_IP_UNKNOWN                      10
      #define SWARM_NODE_WRITE_FAILED                    11
      #define SWARM_NODE_READ_FAILED                     12
      #define SWARM_NODE_ACNO                            13
      #define SWARM_NODE_WRONGFILESIZE                   14
      #define SWARM_NODE_HS_MISFORMED                    15
      #define SWARM_NODE_FILENAME_MISMATCH               16
      #define SWARM_NODE_SHA_LENGTH_MISMATCH             17
      #define SWARM_NODE_SHA_MISMATCH                    18

      // When it exists in ConnectedNodes, it takes below States.
      #define SWARM_NODE_HS_SUCCESS                      30
      #define SWARM_NODE_NL_SUCCESS                      31
      #define SWARM_NODE_DISCONNECTED                    32
      #define SWARM_NODE_INITIAL_FS_SENT                 33
      #define SWARM_NODE_DR_SENT                         34
      #define SWARM_NODE_DR_RECEIVED                     35
      #define SWARM_NODE_DP_SENDING                      36
      #define SWARM_NODE_DP_SENT                         37
      #define SWARM_NODE_EXPECTING_DATAPIECE             38
      #define SWARM_NODE_SEND_FS_EQUAL                   39
      int           NodeState;
      bool          ExpectingDataPiece;

      TCPConnect    *Connection;
      time_t        TimeDataRequestSent;   // DR message
      time_t        TimeFileSizeSent;      // FS message
      time_t        TimeOfLastContact;     // For checking if alive.
      SwarmNode  *Next;
} SwarmNode;

class SwarmNodeList {
public:
   SwarmNodeList();
   ~SwarmNodeList();

   // To initialise a SwarmNodeList structure after allocation.
   void initSwarmNode(SwarmNode *);

   // To free the SwarmNodeList structure.
   // If a valid Connection is present it disconnects and frees it.
   void freeSwarmNodeList(SwarmNode *);

   // To free the SwarmNodeList structures and reinitialize itself.
   // called by SwarmStream::quitSwarm()
   // If a valid Connection is present it disconnects and frees it.
   bool freeAndInitSwarmNodeList();

   // Add a Node/IP/Size/Connection to the List.
   bool addToSwarmNodeNickSizeStateConnection(char *SwarmNick, size_t SwarmFileSize, int state, TCPConnect *Connection);

   // Add a Node/IP to the List.
   bool addToSwarmNodeNickIPState(char *SwarmNick, unsigned long SwarmIP, int state);

   bool addToSwarmNode(SwarmNode *SN);

   // is this Node already present ?
   bool isSwarmNodePresent(char *SwarmNick, unsigned long SwarmIP);

   // Get the first Node from List and delete from List.
   SwarmNode *getFirstSwarmNode();

   // The returned array should not be deleted by caller.
   // Returns the total Upload and Download Speeds in BPS as well.
   char **getNodesUIEntries(size_t *TotalUploadBPS, size_t *TotalDownloadBPS);

   // Returns the current Upload and Download Speed. Used while generating
   // FServ Ad, but called from SwarmStream class.
   bool getCurrentSpeeds(size_t *UploadSpeed, size_t *DownloadSpeed);

   // generates the NL message string, not overflowing length.
   bool generateStringNL(char *Buffer, int BufLen);

   // return the number of entries we are holding in list.
   int getCount();

   // rename Nick Name.
   bool renameNickToNewNick(char *old_nick, char *new_nick);

   // Read Messages from the Connections and Update State.
   // returns true if a state was updated.
   bool readMessagesAndUpdateState(char *OurFileName, size_t *OurFileSize);

   // Write Messages to the Connections and Update State.
   // returns true if a state was updated.
   bool writeMessagesAndUpdateState(char *OurFileName, size_t OurFileSize);

   // Write FS Messages to the Connections whose LastUpdateTime is old.
   // returns true if something was written.
   // We do not update the state. We update LastUpdateTime if we succesfully
   // sent the FS out.
   bool checkForDeadNodes(char *OurFileName, size_t OurFileSize);

   // Check for EndGame and do the necessary to get the Last Piece if at all.
   // returns true if a state was updated.
   bool checkForEndGame(char *OurFileName, size_t OurFileSize);

   // Check if a FutureHoles has a hole present or in pending.
   bool isFutureHolePresentOrPending(char FutureHole[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1]);

   // Called by SwarmStream class to pass this guy the fd, so it can be
   // used to fetch data from file that is being swarmed.
   bool setFileDescriptor(int fd);

   // Used to get the Max FileSize present in any Node for the swarm file.
   // Currently used to populate the percentage in the UI.
   size_t getGreatestFileSize();

   // Used to check if the download is complete, so it can be moved.
   bool isDownloadComplete();

   // Used to check if some critical errors faced, which warrants us to
   // bail out of the swarm.
   #define SWARM_NO_ERROR                  0
   #define SWARM_LSEEK_FILE_FAILED         1
   #define SWARM_WRITE_FILE_FAILED         2
   int getErrorCode();

   // To get the Error code as a string.
   void getErrorCodeString(char *ErrorString);

   // Used to update the CAPs for the Connections.
   // Its called once in 5 seconds.
   bool updateCAPFromGlobal(size_t PerTransferMaxUploadBPS, size_t OverallMaxUploadBPS, size_t PerTransferMaxDownloadBPS, size_t OverallMaxDownloadBPS);

   // Used to get the BytesSent value = bytes sent so far.
   // Its reset to 0 once the value is read.
   size_t getBytesSent();

   // Used to get the BytesReceived value = bytes received so far.
   // Its reset to 0 once the value is read.
   size_t getBytesReceived();

   // Returns the DownloadCompletedSpeed. Note that it will be valid only
   // if DownloadComplete is true.
   size_t getDownloadCompletedSpeed();

private:
   SwarmNode *Head;

   // The DataPieces that we have collected so far.
   SwarmDataPieceList DataPieces;

   // Number of Entries we are currently holding.
   int ListCount;

   // Error Code we are holding.
   int ErrorCode;

   // For the UI.
   char **UIEntries;

   // If we are done downloading the Swarm File 100 %.
   bool DownloadComplete;

   // Hold the rate at which this download got completed.
   // This will be valid only if DownloadComplete is true.
   size_t DownloadCompletedSpeed;

   // Update when we send/receive a DP packet.
   size_t BytesSent;
   size_t BytesReceived;

   // The file Descriptor which is passed on by the SwarmStream class.
   // for accessing the file.
   int FileDescriptor;

   const char *convertNodeStateToString(int state);

   // get an apt offset and length to request as DR from the node.
   // returns true and modifies  offset, and modifies data_length.
   //  also marks the returned offset to be marked with '1' in ourfutureholes
   // returns false if nothing apt.
   bool getOffsetForDR(SwarmNode *SN, size_t *file_offset, size_t *data_length, size_t ourfilesize, char ourfutureholes[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1]);

   // get the requested data from offset and of length as in SN
   // returns false if we dont have that piece.
   // buffer is at least of length SWARM_DATAPIECE_LENGTH
   bool getRequestedDataPiece(SwarmNode *SN, char *buffer);

   // Go through List and mark the nodes that are SWARM_NODE_INITIAL_FS_SENT
   // and have FileSize < than OurFileSize, as SWARM_NODE_SEND_FS_EQUAL
   // Assume lock is held.
   bool markNodesForFSUpdate(size_t OurFileSize);

   // Go through the List and remove Node with 
   // NodeState = SWARM_NODE_DISCONNECTED
   // Assume lock is held.
   bool removeDisconnectedNodes();

   // Orders the SwarmNodes in ascending order of FileSize.
   // Called by readMessagesAndUpdateState() just before it returns.
   // as it is the one which possibly has updated the FileSize of a SwarmNode.
   // Assume lock is held.
   bool orderByFileSize();

   // Generates the FutureHoles string of what we hold.
   // Assume lock is held.
   bool generateFutureHolesString(size_t OurFileSize, char OurFutureHoles[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1]);

   // Obtains the sum of all download speeds across all nodes.
   // Called after end game piece is obtained.
   // Assume lock is held.
   size_t getAvgDownloadSpeed();

   // A mutex to serialise its access as it will exist in XChange Class
   // and a seperate mutex wont be required to control it there.
   MUTEX SwarmNodeListMutex;
};

#endif
