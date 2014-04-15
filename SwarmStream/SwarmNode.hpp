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

#ifndef SWARMNODEHPP
#define SWARMNODEHPP


// Returns -1 if no new node to try.
// else returns a node id which is not yet in ConnectedNodes and within
// TotalThreads.
int GetNewSwarmNodeToTry(int TryingNodes[], int TryingNodesTotal, int TotalThreads);

void getSHAOFLast8K(char *FileName, size_t FileSize, char *sha);

bool isNodeInList(int TryingNodes[], int TryingNodesTotal, int ID);


// Structure to list the current information of each Node.
// This is maintained locally by each node.
typedef struct NodeInfo {
   int NodeID;
   char FirewallState;
   char FileName[32];
   size_t FileSize;
   size_t OffsetRequested; // Filled up on DR_SENT
   #define DR_SENT                   'D'
   #define IDLE                      'I'
   char State;

   // BW related.
   size_t DataBytesIn;
   size_t DataBytesOut;
   size_t ControlBytesIn;
   size_t ControlBytesOut;
} NodeInfo;

typedef struct FilePieces {
   size_t FileOffset;
   char *Data;
   size_t DataLength;
} FilePieces;

bool FileOffsetInList(FilePieces DataPiecesRequested[], int DataPiecesRequestedCount, size_t NeededFileOffset);

int getIndexOfFileOffset(FilePieces DataPiecesRequested[], int DataPiecesRequestedCount, size_t file_offset);

int ScanAndUpdateFile(FilePieces DataPiecesRequested[], int DataPiecesRequestedCount, SwarmNodeProperty *Property);

void getFileChunk(char *FullFilePath, size_t file_offset, size_t data_length, char *Buffer);

int getConnectedNodeIndexInList(NodeInfo ConnectedNodes[], int ConnectedNodesTotal, int ID);
bool isConnectedNodeInList(NodeInfo ConnectedNodes[], int ConnectedNodesTotal, int ID);
#endif
