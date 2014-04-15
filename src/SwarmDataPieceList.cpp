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

#ifdef __MINGW32__
#  include <windows.h>
#else
#  include <pthread.h>
#  include <semaphore.h>
#endif

#include "SwarmDataPieceList.hpp"
#include "StackTrace.hpp"
#include "LineParse.hpp"
#include "Utilities.hpp"

#include "Compatibility.hpp"

#include <string.h>

// Constructor.
SwarmDataPieceList::SwarmDataPieceList() {
   TRACE();
   Head = NULL;

#ifdef __MINGW32__
   SwarmDataPieceListMutex = CreateMutex(NULL, FALSE, NULL);
#else
   pthread_mutex_init(&SwarmDataPieceListMutex, NULL);
#endif
}

// Destructor
SwarmDataPieceList::~SwarmDataPieceList() {
   TRACE();
   freeSwarmDataPieceList(Head);
   DestroyMutex(SwarmDataPieceListMutex);
}

// To initiliaise a newly alloced SwarmNode structure.
void SwarmDataPieceList::initSwarmDataPiece(SwarmDataPiece *SDP) {

   TRACE();

   if (SDP) {
      memset(SDP, 0, sizeof(SwarmDataPiece));
   }
}

// To free the SwarmDataList structures and reinitialize itself.
// called by SwarmNodeList::freeAndInitSwarmNodeList()
bool SwarmDataPieceList::freeAndInitSwarmDataPieceList() {

   TRACE();

   WaitForMutex(SwarmDataPieceListMutex);

   freeSwarmDataPieceList(Head);
   Head = NULL;

   ReleaseMutex(SwarmDataPieceListMutex);

   return(true);
}


void SwarmDataPieceList::freeSwarmDataPieceList(SwarmDataPiece *SDP) {
SwarmDataPiece *Scan;

   TRACE();

   while (SDP != NULL) {
      Scan = SDP->Next;

      delete [] SDP->DataPiece;

      delete SDP;
      SDP = Scan;
   }
}

// Add to SwarmDataPiece given the offset, len of data and the data.
// The Data is copied over.
// Internally the Offsets are each at a difference of SWARM_DATAPIECE_LENGTH
// Hence it will intelligently create new Structures to put them in the
// right bucket. As of now DataLen should be <= SWARM_DATAPIECE_LENGTH
// Hence it can span two Pieces as stored internally.
bool SwarmDataPieceList::addToSwarmNodePieceOffsetLenData(size_t Offset, size_t DataLen, char *Data) {
SwarmDataPiece *Scan;
SwarmDataPiece *FirstPiece = NULL, *SecondPiece = NULL;
bool needSecondPiece;

   TRACE();

   if ( (DataLen > SWARM_DATAPIECE_LENGTH) ||
        (Data == NULL) ) {
      return(false);
   }

   WaitForMutex(SwarmDataPieceListMutex);

   Scan = Head;
   // First find the correct First Piece bucket.
   while (Scan) {
      if ( (Offset >= Scan->FileOffset) &&
           (Offset < (Scan->FileOffset + SWARM_DATAPIECE_LENGTH)) ) {
         // We found where the first piece will go.
         COUT(cout << "Found First Piece with FileOffset: " << Scan->FileOffset << " DataLen: " << Scan->DataPieceLength << endl;);
         FirstPiece = Scan;
         break;
      }
      Scan = Scan->Next;
   }

   Scan = Head;
   // Find the correct Second Piece bucket, only if FirstPiece Bucket is
   // found and FirstPiece cannot entirely hold the full Data.
   if ( (FirstPiece) &&
        ( (FirstPiece->FileOffset + SWARM_DATAPIECE_LENGTH) < (Offset + DataLen)) ) {
      needSecondPiece = true;
      COUT(cout << "Need Second Piece: Offset: " << Offset << " DataLen: " << DataLen << endl;)
      while (Scan) {
         if ( (Scan->FileOffset >= (Offset + DataLen)) &&
              ((Scan->FileOffset + SWARM_DATAPIECE_LENGTH) < (Offset + DataLen)) ) {
            // We found where the second piece will go.
            SecondPiece = Scan;
            break;
         }
         Scan = Scan->Next;
      }
   }
   else {
      needSecondPiece = false;
   }

   // We are here => the scenarios are as follows:
   // - if FirstPiece == NULL, create the piece, dump data and add to list.
   // - if FirstPiece != NULL and needSecondPiece == false, add the Data
   //   to FirstPiece.
   // - if FirstPiece != NULL and needSecondPiece == true and 
   //   SecondPiece == NULL, update partial Data in FirstPiece, 
   //   create SecondPiece and dump rest of Data
   // - if FirstPiece != NULL and needSecondPiece == true and
   //   SecondPiece != NULL, update partial Data in FirstPiece,
   //   update rest of partial Data in SecondPiece.

   if (FirstPiece == NULL) {
      FirstPiece = new SwarmDataPiece;
      FirstPiece->DataPiece = new char[SWARM_DATAPIECE_LENGTH];
      FirstPiece->FileOffset = Offset;
      FirstPiece->DataPieceLength = DataLen;
      memcpy(FirstPiece->DataPiece, Data, DataLen);
      addToSwarmNodePiece(FirstPiece);
      ReleaseMutex(SwarmDataPieceListMutex);
      return(true);
   }

   // So we do the case where FirstPiece is present and we need to add
   // Data to it.
   if (needSecondPiece) {
      memcpy(&FirstPiece->DataPiece[Offset - FirstPiece->FileOffset], Data, FirstPiece->FileOffset + SWARM_DATAPIECE_LENGTH - Offset);

      // Update DataPieceLength with what we just added.
      FirstPiece->DataPieceLength = FirstPiece->DataPieceLength + FirstPiece->FileOffset + SWARM_DATAPIECE_LENGTH - Offset;
   }
   else {
      // This data is <= capacity of this slot, so fits entirely in FirstPiece.
      memcpy(&FirstPiece->DataPiece[Offset - FirstPiece->FileOffset], Data, DataLen);
      FirstPiece->DataPieceLength = FirstPiece->DataPieceLength + DataLen;
   }

   // Create the SecondPiece if needSecondPiece is true.
   if ((needSecondPiece) && (SecondPiece == NULL)) {
      SecondPiece = new SwarmDataPiece;
      SecondPiece->DataPiece = new char[SWARM_DATAPIECE_LENGTH];
      SecondPiece->FileOffset = FirstPiece->FileOffset + SWARM_DATAPIECE_LENGTH;
      addToSwarmNodePiece(SecondPiece);
   }

   // Either case, we created or it existed, add the Data to it.
   if (SecondPiece) {
      SecondPiece->DataPieceLength = Offset + DataLen - SecondPiece->FileOffset;
      memcpy(SecondPiece->DataPiece, &Data[SecondPiece->FileOffset - Offset], SecondPiece->DataPieceLength);
   }

   ReleaseMutex(SwarmDataPieceListMutex);

   return(true);
}

// Locks are held by caller. Its a private function as of now.
// Adds to list maintaining ascending order of List wrt FileOffset.
// Its an error if the difference in FileOffsets is not a multiple of
// SWARM_DATAPIECE_LENGTH
bool SwarmDataPieceList::addToSwarmNodePiece(SwarmDataPiece *DP) {
SwarmDataPiece *Scan, *tmpScan;

   TRACE();

   DP->Next = Head;
   Head = DP;

   // Lets now insert the first entry as far down as it can go.
   Scan = Head ->Next;
   tmpScan = NULL;
   while (Scan) {
     if (Scan->FileOffset < Head->FileOffset) {
        tmpScan = Scan;
        Scan = Scan->Next;
     }
     else break;
   }

   // We need to insert the new item just after tmpScan.
   if (tmpScan) {
      Scan = Head->Next; // Scan holds the new Head.
      Head->Next = tmpScan->Next;
      tmpScan->Next = Head;

      // Get the correct Head.
      Head = Scan;
   }
}

// Returns the FutureHoleByteArray - FileSize is base.
// Used in the exchange of messages.
// Return true if there is at least one hole populated.
// We always set the 0th bit and 1st bit of all bytes. (so that a string 
// terminator is not injected). So only 6 bits used per byte.
bool SwarmDataPieceList::getFutureHolesByteArray(size_t FileSize, char FutureHoles[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1]) {
SwarmDataPiece *DP;
int HoleIndex;
int BitIndex;
bool retvalb = false;

   TRACE();

   // First '0' out all entries.
   memset(FutureHoles, 0, SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1);

   // Now for each byte mark 0th and 1st bit (1st and 2nd) as 1.
   // bits: 0, 1, 8, 9 ...
   for (int i = 0; i < SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN * BITS_PER_BYTE; i += BITS_PER_BYTE) {
      setBitInByteArray(FutureHoles, i);
      setBitInByteArray(FutureHoles, i + 1);
   }

   WaitForMutex(SwarmDataPieceListMutex);

   DP = Head;
   while (DP) {
      if (DP->DataPieceLength == SWARM_DATAPIECE_LENGTH) {
         // Count only whole pieces.
         if (DP->FileOffset >= FileSize) {
            HoleIndex = (DP->FileOffset - FileSize) / SWARM_DATAPIECE_LENGTH;

            if ( (HoleIndex >= 0) && (HoleIndex < SWARM_MAX_FUTURE_HOLES) ) {
               // 3, cause 3 holes per byte. + 2 => skip first 2 bits.
               BitIndex =  (2 * HoleIndex) + ((HoleIndex/ 3) * 2) + 2;
               setBitInByteArray(FutureHoles, BitIndex);
               setBitInByteArray(FutureHoles, BitIndex + 1);
               // The pieces we have are marked as 11.

               retvalb = true;
            }
         }
      }

      DP = DP->Next;
   }

   ReleaseMutex(SwarmDataPieceListMutex);

   return(retvalb);
}

// Return true and the DataPiece if present in our list of the required size
bool SwarmDataPieceList::getDataPiece(size_t FileOffset, size_t DataSize, char *buffer) {
SwarmDataPiece *DP;
bool retvalb = false;
size_t buffer_index;
size_t first_file_offset;

   TRACE();

   WaitForMutex(SwarmDataPieceListMutex);

   // Note that the list is in ascending order of FileOffset
   DP = Head;
   while (DP) {
      if ( (FileOffset >= DP->FileOffset) &&
           (FileOffset < (DP->FileOffset + DP->DataPieceLength)) ) {
         // This holder has the starting piece or possibly the whole piece
         // that is requested.
         // First copy what we can.
         first_file_offset = DP->FileOffset;
         size_t copy_len = DP->DataPieceLength - (FileOffset - DP->FileOffset);
         memcpy(buffer, &DP->DataPiece[FileOffset - DP->FileOffset], copy_len);

         buffer_index = copy_len;
         // Did we just copy a partial ?
         if (copy_len == DataSize) {
            retvalb = true;
            COUT(cout << "Swarm: GetDataPiece. Copied data from one hole" << endl;)
            break;
         }

         // Have more to copy into buffer[buffer_index].
         copy_len = DataSize - copy_len; // new copy_len.

         // Now this data should be available in the next slot as we have it
         // in ascending order of FileOffset.
         DP = DP->Next;
         if (DP == NULL) break;

         // Should be the next contiguous block.
         if ( (first_file_offset + SWARM_DATAPIECE_LENGTH) != DP->FileOffset) {
            break;
         }

         // So it is the next contiguous block, so copy the required rest.
         // if it has copy_len of data.
         if (DP->DataPieceLength < copy_len) break;

         memcpy(&buffer[buffer_index], DP->DataPiece, copy_len);
         retvalb = true;
         COUT(cout << "Swarm: GetDataPiece. Copied data from second hole" << endl;)
      }
      DP = DP->Next;
   }

   ReleaseMutex(SwarmDataPieceListMutex);

   return(retvalb);
}

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
bool SwarmDataPieceList::getAllignedEndGamePiece(size_t FileOffset, size_t *DataSize, char *buffer) {
bool retvalb = false;

   TRACE();

   WaitForMutex(SwarmDataPieceListMutex);

   // Note that it should be the only entry in the list.
   if ( (Head) && (Head->Next == NULL) ) {
      if ( (FileOffset == Head->FileOffset) && 
           (Head->DataPieceLength <= *DataSize) ) {
            // This is the entry that is asked for.
            retvalb = true;
            memcpy(buffer, Head->DataPiece, Head->DataPieceLength);
            *DataSize = Head->DataPieceLength;
         }
   }

   ReleaseMutex(SwarmDataPieceListMutex);

   return(retvalb);
}

// Return true and the DataPiece if present in our list of exactly
// DataSize size.
// Internally we assume that our bucket exactly starts with the FileOffset
// mentioned. This is used to write out the data we receive to file,
// and hence the allignment assumption is valid.
bool SwarmDataPieceList::getAllignedDataPiece(size_t FileOffset, size_t DataSize, char *buffer) {
SwarmDataPiece *DP;
bool retvalb = false;

   TRACE();

   WaitForMutex(SwarmDataPieceListMutex);

   // Note that the list is in ascending order of FileOffset
   DP = Head;
   while (DP) {
      if (FileOffset == DP->FileOffset) {
         if (DP->DataPieceLength == DataSize) {
            // This is the entry that is asked for.
            retvalb = true;
            memcpy(buffer, DP->DataPiece, DP->DataPieceLength);
         }
         break;
      }
      DP = DP->Next;
   }

   ReleaseMutex(SwarmDataPieceListMutex);

   return(retvalb);
}

// Delete the data piece if present in our list
// Called from SwarmNodeList::removeDisconnectedNodes() when it detects
// that a node in state SWARM_NODE_EXPECTING_DATAPIECE has disconnected.
// So we discard the partial data that we have received so far from that
// node. Note => SWARM_NODE_EXPECTING_DATAPIECE => not fully received
// data piece.
// Note that the FileOffset can point to anywhere between the DataPiece's
// FileOffset and to its end capacity = offset + SWARM_DATAPIECE_LENGTH
bool SwarmDataPieceList::deleteDataPiece(size_t FileOffset) {
SwarmDataPiece *DP, *prevDP;
bool retvalb = false;

   TRACE();

   WaitForMutex(SwarmDataPieceListMutex);

   // Note that the list is in ascending order of FileOffset
   DP = Head;
   prevDP = NULL;
   while (DP) {
      if ( (FileOffset >= DP->FileOffset) &&
           (FileOffset < (DP->FileOffset + SWARM_DATAPIECE_LENGTH)) ) {
         // So we got the one we need to delete.
         COUT(cout << "SwarmDataPieceList::deleteDataPiece: deleting DataPiece with DP->FileOffset: " << DP->FileOffset << " for Offset requested: " << FileOffset << endl;)
         if (prevDP == NULL) {
            // This is the Head Element.
            Head = DP->Next;
         }
         else {
            // This is an in between element, and we have prevDP set.
            prevDP->Next = DP->Next;
         }
         DP->Next = NULL;
         freeSwarmDataPieceList(DP);
         retvalb = true;
         break;
      }
      prevDP = DP;
      DP = DP->Next;
   }

   ReleaseMutex(SwarmDataPieceListMutex);

   return(retvalb);
}


// Delete the data piece if present in out list
// Called after writing a piece to disk. In this case we will get calls
// with aligned FileOffset and DataPieceLength, as its our own file pieces
// we are trying to get and write to disk.
bool SwarmDataPieceList::deleteAllignedDataPiece(size_t FileOffset, size_t DataSize) {
SwarmDataPiece *DP, *prevDP;
bool retvalb = false;

   TRACE();

   WaitForMutex(SwarmDataPieceListMutex);

   // Note that the list is in ascending order of FileOffset
   DP = Head;
   prevDP = NULL;
   while (DP) {
      if ( (DP->FileOffset == FileOffset) &&
           (DP->DataPieceLength == DataSize) ) {
         // So we got the one we need to delete.
         if (prevDP == NULL) {
            // This is the Head Element.
            Head = DP->Next;
         }
         else {
            // This is an in between element, and we have prevDP set.
            prevDP->Next = DP->Next;
         }
         DP->Next = NULL;
         freeSwarmDataPieceList(DP);
         retvalb = true;
         break;
      }
      prevDP = DP;
      DP = DP->Next;
   }

   ReleaseMutex(SwarmDataPieceListMutex);

   return(retvalb);
}

