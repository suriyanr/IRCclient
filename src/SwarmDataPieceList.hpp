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

#ifndef CLASS_SWARMDATAPIECELIST
#define CLASS_SWARMDATAPIECELIST

#ifdef __MINGW32__
#  include <windows.h>
#else
#  include <pthread.h>
#  include <semaphore.h>
#endif


// This Class maintains the File data pieces held by the Swarm
#include "Compatibility.hpp"

#define SWARM_DATAPIECE_LENGTH    8192

// Data Structures.
typedef struct SwarmDataPiece {
      size_t         FileOffset;
      size_t         DataPieceLength;
      char           *DataPiece;
      SwarmDataPiece *Next;
} SwarmDataPiece;


class SwarmDataPieceList {
public:
   SwarmDataPieceList();
   ~SwarmDataPieceList();

   // To initialise a SwarmDataList structure after allocation.
   void initSwarmDataPiece(SwarmDataPiece *);

   // To free the SwarmDataList structure.
   void freeSwarmDataPieceList(SwarmDataPiece *);

   // To free the SwarmDataList structures and reinitialize itself.
   // called by SwarmNodeList::freeAndInitSwarmNodeList() 
   bool freeAndInitSwarmDataPieceList();

   // Add to SwarmDataPiece given the offset, len of data and the data.
   // The Data is copied over.
   // Internally the Offsets are each at a difference of SWARM_DATAPIECE_LENGTH
   // Hence it will intelligently create new Structures to put them in the
   // right bucket. As of now DataPiece <= SWARM_DATAPIECE_LENGTH
   bool addToSwarmNodePieceOffsetLenData(size_t Offset, size_t DataLen, char *Data);

   // Returns the FutureHoleByteArray - FileSize is base.
   // Used in the exchange of messages.
   // Return true if there is at least one hole populated.
   bool getFutureHolesByteArray(size_t FileSize, char FutureHoles[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1]);

   // Return true and the DataPiece if present in our list of the required size
   bool getDataPiece(size_t FileOffset, size_t DataSize, char *buffer);

   // Return true and the DataPiece if present in our list of exactly
   // DataSize size.
   // Internally we assume that our bucket exactly starts with the FileOffset
   // mentioned. This is used to write out the data we receive to file,
   // and hence the allignment assumption is valid.
   bool getAllignedDataPiece(size_t FileOffset, size_t DataSize, char *buffer);

   // Return true and the DataPiece if present in our list of at least
   // DataSize size. If less than update DataSize to reflect the size returned.
   // Note that it will return true only if its the only entry in the list.
   // which will be the case when its the EndGame.
   // and if its not the endgame, we wont do much damage but just write
   // out delta bytes to file.
   // Internally we assume that our bucket exactly starts with the FileOffset
   // mentioned. This is used to write out the data we receive to file,
   // and hence the allignment assumption is valid.
   // This is used to write out the EndGame bytes.
   bool getAllignedEndGamePiece(size_t FileOffset, size_t *DataSize, char *buffer);

   // Delete the data piece if present in our list
   // Called after writing a piece to disk. In this case we will get calls
   // with aligned FileOffset and DataPieceLength, as its our own file pieces
   // we are trying to get and write to disk.
   bool deleteAllignedDataPiece(size_t FileOffset, size_t DataSize);

   // Delete the data piece if present in our list
   // Called from SwarmNodeList::removeDisconnectedNodes() when it detects
   // that a node in state SWARM_NODE_EXPECTING_DATAPIECE has disconnected.
   // So we discard the partial data that we have received so far from that
   // node. Note => SWARM_NODE_EXPECTING_DATAPIECE => not fully received
   // data piece.
   bool deleteDataPiece(size_t FileOffset);

private:
   SwarmDataPiece *Head;

   // Adds in List in ascending order of FileOffset.
   bool addToSwarmNodePiece(SwarmDataPiece *DP);

// A mutex to serialise its access
   MUTEX SwarmDataPieceListMutex;
};

#endif
