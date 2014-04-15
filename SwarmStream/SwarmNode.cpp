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

#include <iostream>

#ifdef __MINGW32__
#  include <windows.h>
#else
#  include <pthread.h>
#  include <semaphore.h>
#endif


#include "LineQueueBW.hpp"
#include "LineParse.hpp"
#include "Compatibility.hpp"
#include "Swarm.hpp"
#include "SwarmNode.hpp"

#include "SHA1.hpp"
#include "Utilities.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


using namespace std;

void SwarmNodeThread(SwarmNodeProperty *Property) {
int i;
char Message[10240];
// To maintain the Nodes that we are trying.
int TryingNodes[MAX_SWARM_NODES];
int TryingNodesTotal = 0;
// To maintain the Nodes that we have successfully connected.
char sha[64];
int MyID;
LineParse LineP;
const char *parseptr;
char Buffer[8193];

// Connected Nodes Info.
NodeInfo ConnectedNodesInfo[MAX_SWARM_NODES];
int ConnectedNodesTotal = 0;

// Data Pieces we have sent a request for.
FilePieces DataPiecesRequested[MAX_SWARM_NODES];
int DataPiecesRequestedCount = 0;

   // Lets initialise ConnectedNodesInfo.
   for (i = 0; i < MAX_SWARM_NODES; i++) {
      ConnectedNodesInfo[i].State = IDLE;
      ConnectedNodesInfo[i].DataBytesIn = 0;
      ConnectedNodesInfo[i].DataBytesOut = 0;
      ConnectedNodesInfo[i].ControlBytesIn = 0;
      ConnectedNodesInfo[i].ControlBytesOut = 0;
      DataPiecesRequested[i].Data = NULL; 
   }
   cout << "Starting Thread ID: " << Property->ThreadID << " Nick: " << Property->Nick << " FileName: " << Property->FileName << " FileSize: " << Property->FileSize << " MaxUploadBPS: " << Property->MaxUploadBPS << " MaxDownloadBPS: " << Property->MaxDownloadBPS << " FirewallState: " << Property->FirewallState << endl;

   // Now each node needs to communicate using RecvMessageQueue[].
   // For i to send a message to j, putLine() in RecvMessageQueue[j].
   // And i, will read and act on messages in RecvMessageQueue[i].
   // We know the validity of the range is 0 to TotalThreads - 1.
   MyID = Property->ThreadID;

   // Set my node's BW capacities.
   RecvMessageQueue[MyID].setUploadBPS(Property->MaxUploadBPS);
   RecvMessageQueue[MyID].setDownloadBPS(Property->MaxDownloadBPS);

   // Wait for instruction to start the simulation.
   while (!StartSimulation);

   // First what we try to do is handshake with all the guys to see if we
   // can connect or if can connect, we are trying to swarm stream the
   // same file.
   // So from 0 to TotalThreads - 1, skipping ourselves, we send a
   // handshake message: unless we have SWARM_MAX_CLIENTS connections
   // already established.
   // HS MyThreadID MyFirewallState MyFileName MyFileSize MySHA1OfLast8K
   // On receiving this the other party does following:
   // - If he is FW Y and I am FW Y, then dont respond.
   // - If our FileNames dont match, send a reject message.
   // - If we have greater FileSize than what he has then verify SHA1 and if
   //   it doesnt match reject, if it matches accept.
   // - If we have lesser FileSize than what he has then send him our
   //   HS MyThreadID MyFirewallState MyFileName MyFileSize MySHA1OfLast8K
   //   and let him verify from his end and accept or reject us.
   // The Accept message is: AC YS ThreadID FWState FileName FileSize
   // The Reject message is: AC NO ThreadID

   // We shoudnt try to connect to ourselves.
   TryingNodes[TryingNodesTotal] = MyID;
   TryingNodesTotal++;
   while (true) {
      // Lets try to connect to swarm.
      // Who do we send it to ?
      i = GetNewSwarmNodeToTry(TryingNodes, TryingNodesTotal, TotalThreads);
      if (i != -1) {
         // Add this entry in our collection.
         TryingNodes[TryingNodesTotal] = i;
         TryingNodesTotal++;

         // Update FileSize.
         sprintf(Message, "%s%s%s", Property->DirName, DIR_SEP, Property->FileName);
         getFileSize(Message, &(Property->FileSize));
         getSHAOFLast8K(Message, Property->FileSize, sha);
         sprintf(Message, "HS %lu %c %s %lu %s",
                     Property->ThreadID,
                     Property->FirewallState,
                     Property->FileName,
                     Property->FileSize, 
                     sha
          );
         cout << "Sending Thread: " << MyID << " To Thread: " << i << " : " << Message << endl;
         
         RecvMessageQueue[i].putLine(Message);
      }

      // Check if we need to process any incoming messages.
      if (RecvMessageQueue[MyID].isEmpty()) {
         goto GenerateRequests;
      }

      int HisID;
      // Process what we have got here.
      RecvMessageQueue[MyID].getLineAndDelete(Message);
      strncpy(Buffer, Message, 20);
      Buffer[20] = '\0';
      cout << "Receiving Thread: " << MyID << " : " << Buffer << endl;

      // We are right now just getting the swarm up. So its either a
      // HS or a AC message.
      LineP = Message;
      parseptr = LineP.getWord(1);
      if (strcasecmp(parseptr, "HS") == 0) {
         char HisFWState;
         char HisFileName[32];
         size_t HisFileSize;
         char HisSHA[64];
         // If we are already connected to it, then just send an 
         // AC YS Id FWState FileName FileSize
         // HS ThreadID FirewallState FileName FileSize SHA
         parseptr = LineP.getWord(2);
         HisID = (int) strtoul(parseptr, NULL, 10);
         if (isConnectedNodeInList(ConnectedNodesInfo, ConnectedNodesTotal, HisID)) {
            // We are already connected to him, send 
            // AC YS Id FWState FileName FileSize
            sprintf(Message, "AC YS %d %c %s %lu", MyID, Property->FirewallState, Property->FileName, Property->FileSize);
            RecvMessageQueue[HisID].putLine(Message);
            continue;
         }

         // Validate this HS Line.
         parseptr = LineP.getWord(3);
         HisFWState = *parseptr;
         if ( (HisFWState == 'Y') && (Property->FirewallState == 'Y') ) {
            // Both are firewalled, so no response to be sent.
            continue;
         }

         // We need to validate and send message back.
         parseptr = LineP.getWord(4);
         strcpy(HisFileName, parseptr);
         parseptr = LineP.getWord(5);
         HisFileSize = strtoul(parseptr, NULL, 10);
         parseptr = LineP.getWord(6);
         strcpy(HisSHA, parseptr);
         // If FileNames are different send a reject.
         if (strcasecmp(HisFileName, Property->FileName)) {
            sprintf(Message, "AC NO %d", MyID);
            RecvMessageQueue[HisID].putLine(Message);
            continue;
         }

         // Now validate if we can.
         if (Property->FileSize < HisFileSize) {
            // We cannot validate, so send a HS back.
            sprintf(Message, "%s%s%s", Property->DirName, DIR_SEP, Property->FileName);
            getSHAOFLast8K(Message, Property->FileSize, sha);
            sprintf(Message, "HS %lu %c %s %lu %s",
                                 Property->ThreadID,
                                 Property->FirewallState,
                                 Property->FileName,
                                 Property->FileSize, 
                                 sha
                       );
            RecvMessageQueue[HisID].putLine(Message);
            continue;
         }

         // We can validate this.
         sprintf(Message, "%s%s%s", Property->DirName, DIR_SEP, HisFileName);
         getSHAOFLast8K(Message, HisFileSize, sha);

         cout << "Thread: " << MyID << " validating SHA. mine: " << sha << " thread: " << HisID << " his: " << HisSHA << endl;
         if (strcasecmp(sha, HisSHA) == 0) {
            // valid one. so send AC YS ThrIDFWState FileName FileSize
            sprintf(Message, "AC YS %d %c %s %lu", MyID, Property->FirewallState, Property->FileName, Property->FileSize);
            // We need to update our ConnectedNodes with this id.
            // if its not already in list.
            if (!isConnectedNodeInList(ConnectedNodesInfo, ConnectedNodesTotal, HisID)) {

               // Now update ConnectedNodesInfo
               ConnectedNodesInfo[ConnectedNodesTotal].NodeID = HisID;
               ConnectedNodesInfo[ConnectedNodesTotal].FirewallState = HisFWState;
               strcpy(ConnectedNodesInfo[ConnectedNodesTotal].FileName, HisFileName);
               ConnectedNodesInfo[ConnectedNodesTotal].FileSize = HisFileSize;

               ConnectedNodesTotal++;
            }
         }
         else {
            // invalid one. so send AC NO ThrID
            sprintf(Message, "AC NO %d", MyID);
         }
         RecvMessageQueue[HisID].putLine(Message);
         continue;
      }

      if (strcasecmp(parseptr, "AC") == 0) {
         // This is the accept message, check if its YS or NO.
         parseptr = LineP.getWord(3);
         HisID = strtoul(parseptr, NULL, 10);
         parseptr = LineP.getWord(2);
         if (strcasecmp(parseptr, "YS") == 0) {
            // We need to update our ConnectedNodes with this id.
            // if its not already in list.
            if (!isConnectedNodeInList(ConnectedNodesInfo, ConnectedNodesTotal, HisID)) {

               // Now update ConnectedNodesInfo
               ConnectedNodesInfo[ConnectedNodesTotal].NodeID = HisID;
               parseptr = LineP.getWord(4);
               ConnectedNodesInfo[ConnectedNodesTotal].FirewallState = *parseptr;
               parseptr = LineP.getWord(5);
               strcpy(ConnectedNodesInfo[ConnectedNodesTotal].FileName, parseptr);
               parseptr = LineP.getWord(6);
               size_t file_size = strtoul(parseptr, NULL, 10);
               ConnectedNodesInfo[ConnectedNodesTotal].FileSize = file_size;

               ConnectedNodesTotal++;
            }
         }
         else {
            // Node has rejected us.
         }
         continue;
      }

      if (strcasecmp(parseptr, "QT") == 0) {
         // End simulation.
         break;
      }

      if (strcasecmp(parseptr, "DR") == 0) {
         // DR ThrID FileOffset FileSize FileName
         // This is a data piece request message.
         // 2nd word is ThreadID
         parseptr = LineP.getWord(2);
         HisID = (int) strtoul(parseptr, NULL, 10);

         // 3rd word is FileSize. 5th and beyond is FileName
         parseptr = LineP.getWord(3);
         size_t req_file_size = strtoul(parseptr, NULL, 10);

         // 4th word is the FileSize that he currently has.
         parseptr = LineP.getWord(4);
         int list_index = getConnectedNodeIndexInList(ConnectedNodesInfo, ConnectedNodesTotal, HisID);
         ConnectedNodesInfo[list_index].FileSize = strtoul(parseptr, NULL, 10);

         // Validate filesize.
         parseptr = LineP.getWord(5);
         size_t file_size;
         sprintf(Message, "%s%s%s", Property->DirName, DIR_SEP, parseptr);
         getFileSize(Message, &file_size);
         if (file_size <= req_file_size) {
            // What do we send here ?
            continue;
         }
         // Get the data ready and send it to this dude.
         size_t data_length = file_size - req_file_size;
         if (data_length > 8192) data_length = 8192;
         getFileChunk(Message, req_file_size, data_length, Buffer);
         Buffer[data_length] = '\0';

         sprintf(Message, "DP %d %lu %lu %s", MyID, data_length, Property->FileSize, Buffer);
         cout << "ThrID: " << MyID << " DP to Thr: " << HisID << " Message: " << "DP " << MyID << " " << data_length << " " << Property->FileSize << " <Data>" << endl;
         ConnectedNodesInfo[list_index].DataBytesOut += strlen(Message);
         RecvMessageQueue[HisID].putLine(Message);

         continue;
      }

      // Handle the DP message received.
      if (strcasecmp(parseptr, "DP") == 0) {
         // DP ThrID DataLength FileSize <Data>
         // 2nd word is Thread id.
         parseptr = LineP.getWord(2);
         HisID = (int) strtoul(parseptr, NULL, 10);
         // 3rd word is length of Data Packet.
         parseptr = LineP.getWord(3);
         size_t data_length = strtoul(parseptr, NULL, 10);
         // 4th word is his current filesize.
         int list_index = getConnectedNodeIndexInList(ConnectedNodesInfo, ConnectedNodesTotal, HisID);
         parseptr = LineP.getWord(4);
         ConnectedNodesInfo[list_index].FileSize = strtoul(parseptr, NULL, 10);

         // First make sure that we did request from this dude.
         // His state would be DR_SENT.
         if (ConnectedNodesInfo[list_index].State != DR_SENT) {
            // Never asked him, so discard.
            continue;
         }

         // The data payload.
         // Get the 4th space + 1.
         char *start_ptr = strchr(Message, ' ');
         start_ptr++; // Start of 2nd word.
         start_ptr = strchr(start_ptr, ' ');
         start_ptr++; // Start of 3rd word.
         start_ptr = strchr(start_ptr, ' ');
         start_ptr++; // Start of 4th word.
         start_ptr = strchr(start_ptr, ' ');
         start_ptr++; // Start of 5th word.
     
         strcpy(Buffer, start_ptr);

         // This could be out of sequence, hence cant really write it out
         // to file yet. We put it in DataPiecesRequested[];
         int entry_index = getIndexOfFileOffset(DataPiecesRequested, DataPiecesRequestedCount, ConnectedNodesInfo[list_index].OffsetRequested);
         if (entry_index != -1) {
            DataPiecesRequested[entry_index].Data = new char[data_length + 1];
            memcpy(DataPiecesRequested[entry_index].Data, Buffer, data_length);
            DataPiecesRequested[entry_index].Data[data_length] = '\0';
            DataPiecesRequested[entry_index].DataLength = data_length;
         }
         else continue;

         // Now we need to mark this guys state as IDLE, as he has sent us the
         // piece we requested, and is now idle.
         ConnectedNodesInfo[list_index].State = IDLE;

         ConnectedNodesInfo[list_index].DataBytesIn += strlen(Message);

         // Here as we have recieved data, go through the list and see 
         // if we can write out data to disk, and update our FileSize.
         size_t my_org_fs = Property->FileSize;
         DataPiecesRequestedCount = ScanAndUpdateFile(DataPiecesRequested, DataPiecesRequestedCount, Property);
         if (my_org_fs != Property->FileSize) {
            // Our FileSize has changed. Lets inform our equals.
            // Equals are those who have filesizes within 8K up and down
            // of my_org_file_size
            for (i = 0; i <= ConnectedNodesTotal; i++) {
               size_t his_fs = ConnectedNodesInfo[i].FileSize;
               size_t upper_limit = my_org_fs - 8192;
               size_t lower_limit = my_org_fs + 8192;
               if ( (his_fs >= lower_limit) &&
                    (his_fs <= upper_limit) ) {
                  // Send the FS message to this guy.
                  sprintf(Message, "FS %d %lu %s", MyID, Property->FileSize, Property->FileName);
                  int his_id = ConnectedNodesInfo[i].NodeID;
                  cout << "MyThrID: " << MyID << "Sending FS to Thr: " << his_id << " Message: " << Message << endl;
                  RecvMessageQueue[his_id].putLine(Message);
               }
            }
         }

         cout << "ThrID: " << MyID << " Got a DP from Thr: " << HisID << " He sent us: " << data_length << " bytes, from offset: " << ConnectedNodesInfo[list_index].OffsetRequested << " Our File Length is: " << Property->FileSize << endl;
         continue;
      }

      // Handle the FS message received.
      if (strcasecmp(parseptr, "FS") == 0) {
         // FS ThrID FileSize FileName
         parseptr = LineP.getWord(2);
         HisID = (int) strtoul(parseptr, NULL, 10);
         int list_index = getConnectedNodeIndexInList(ConnectedNodesInfo, ConnectedNodesTotal, HisID);
         parseptr = LineP.getWord(3);
         ConnectedNodesInfo[list_index].FileSize = strtoul(parseptr, NULL, 10);
         continue;
      }

GenerateRequests:
      // We have handled the response we can recieve. Lets now start sending
      // messages.
      // Send a DR message if possible.
      // We need to keep track of what request we have sent so that we
      // dont request the same piece from someone else.
      // So, lets find out what piece we need. We maintain the File Pieces
      // requested in DataPiecesRequested[]
      size_t NeededFileOffset;
      NeededFileOffset = Property->FileSize;
      while (true) {
         if (FileOffsetInList(DataPiecesRequested, DataPiecesRequestedCount, NeededFileOffset)) {
            // Already waiting for this piece.
            NeededFileOffset += 8192;
         }
         else {
            break;
         }
      }
      //cout << "ThrID: " << MyID << " NeededFileOffset: " << NeededFileOffset << endl;

      // Now we check if the NeededFileOffset is served by some connected node.
      int RequestFromNode = -1;
      for (i = 0; i < ConnectedNodesTotal; i++) {
         if (ConnectedNodesInfo[i].FileSize >= (NeededFileOffset + 8192)) {
            // It can give us the full 8K block.
            // Check if its not in DR_SENT state => already pending some data.
            if (ConnectedNodesInfo[i].State != DR_SENT) {
               RequestFromNode = i;
               break;
            }
         }
      }
      //cout << "ThrID: " << MyID << " RequestFromNode: " << RequestFromNode << endl;

      if (RequestFromNode != -1) {
         // Lets send the request.
         DataPiecesRequested[DataPiecesRequestedCount].FileOffset = NeededFileOffset;
         DataPiecesRequested[DataPiecesRequestedCount].Data = NULL;
         DataPiecesRequestedCount++;
         // Lets mark this guys state as DR_SENT
         ConnectedNodesInfo[RequestFromNode].State = DR_SENT;
         ConnectedNodesInfo[RequestFromNode].OffsetRequested = NeededFileOffset;
         sprintf(Message, "DR %d %lu %lu %s", MyID, NeededFileOffset, Property->FileSize, Property->FileName);
         cout << "Thr: " << MyID << " DR to Thr: " << RequestFromNode << " Message: " << Message << endl;
         int node_id = ConnectedNodesInfo[RequestFromNode].NodeID;
         RecvMessageQueue[node_id].putLine(Message);
      }
      else {
         // We are here -> cannot recieve any more full 8K blocks.
         // If we still have pending Requests then nothing to do.
         if (DataPiecesRequestedCount != 0) {
            msleep(250);
         }
         else {
            // No pending requests. Find the one which has more FileSize than
            // us, and request that dude. NeededFileOffset is still valid value
            RequestFromNode = -1;
            for (i = 0; i < ConnectedNodesTotal; i++) {
               if (ConnectedNodesInfo[i].FileSize > NeededFileOffset) {
                  RequestFromNode = i;
                  break;
               }
            }
            if (RequestFromNode != -1) {
               // This is the guy we need to get the end piece from.
               DataPiecesRequested[DataPiecesRequestedCount].FileOffset = NeededFileOffset;
               DataPiecesRequested[DataPiecesRequestedCount].Data = NULL;
               DataPiecesRequestedCount++;
               // Lets mark this guys state as DR_SENT
               ConnectedNodesInfo[RequestFromNode].State = DR_SENT;
               ConnectedNodesInfo[RequestFromNode].OffsetRequested = NeededFileOffset;
               sprintf(Message, "DR %d %lu %lu %s", MyID, NeededFileOffset, Property->FileSize, Property->FileName);
               int node_id = ConnectedNodesInfo[RequestFromNode].NodeID;
               cout << "Thr: " << MyID << " End Piece DR to Thr: " << node_id << " Message: " << Message << endl;
               RecvMessageQueue[node_id].putLine(Message);
            }
            else {
               // all requests over, nothign to get.
               msleep(250);
            }
         }
      }

   } // while (true) end block

   // Lets print out the ConnectedNodesInfo.
   size_t TotalIn = 0, TotalOut = 0;
   for (i = 0; i < ConnectedNodesTotal; i++) {
      TotalIn += ConnectedNodesInfo[i].DataBytesIn;
      TotalOut += ConnectedNodesInfo[i].DataBytesOut;
      cout << "MyID: " << MyID << " NodeID: " << ConnectedNodesInfo[i].NodeID << " DataBytesIn: " << ConnectedNodesInfo[i].DataBytesIn << " DataBytesOut: " << ConnectedNodesInfo[i].DataBytesOut << " Firewall State: " << ConnectedNodesInfo[i].FirewallState << " FileName: " << ConnectedNodesInfo[i].FileName << " FileSize: " << ConnectedNodesInfo[i].FileSize << " State: " << ConnectedNodesInfo[i].State << endl;
   }
   cout << "MyID: " << MyID << " Total Bytes In: " << TotalIn << " Total Bytes Out: " << TotalOut << endl;

   // We are done, so lets post that in ThreadOver.
   ThreadOver[Property->ThreadID] = true;

   cout << "Ending Thread ID: " << Property->ThreadID << endl;
   // Lets delete the SwarmNodeProperty structure.
   delete Property;
}

int GetNewSwarmNodeToTry(int TryingNodes[], int TryingNodesTotal, int TotalThreads) {
int retval = -1;

   for (int i = 0; i < TotalThreads; i++) {
      // Lets first check if this i, is already present in ConnectedNodes[]

      if (isNodeInList(TryingNodes, TryingNodesTotal, i) == false) {
         retval = i;
         break;
      }
   }

   return(retval);
}

bool isNodeInList(int TryingNodes[], int TryingNodesTotal, int ID) {
   for (int i = 0; i < TryingNodesTotal; i++) {
      if (TryingNodes[i] == ID) {
         return(true);
      }
   }
   return(false);
}

int getConnectedNodeIndexInList(NodeInfo ConnectedNodes[], int ConnectedNodesTotal, int ID) {
int retval = -1;

   for (int i = 0; i < ConnectedNodesTotal; i++) {
      if (ConnectedNodes[i].NodeID == ID) {
         retval = i;
         break;
      }
   }
   if (retval == -1) abort();
   return(retval);
}

bool isConnectedNodeInList(NodeInfo ConnectedNodes[], int ConnectedNodesTotal, int ID) {
bool retvalb = false;

   for (int i = 0; i < ConnectedNodesTotal; i++) {
      if (ConnectedNodes[i].NodeID == ID) {
         retvalb = true;
         break;
      }
   }
   return(retvalb);
}

// FileName is full filename. We need to get the sha of 8K bytes before
// FileSize.
void getSHAOFLast8K(char *FileName, size_t FileSize, char *sha) {
CSHA1 SHA;
size_t start_offset;
size_t end_offset;

   // Remember that its the offset, so 1 less than filesize.
   // as counting starts from 0.
   end_offset = FileSize - 1;
   start_offset = end_offset - 8192;
   if (start_offset < 0) start_offset = 0;
   SHA.HashFile(FileName, start_offset, end_offset);
   SHA.Final();
   SHA.ReportHash(sha);
}

bool FileOffsetInList(FilePieces DataPiecesRequested[], int DataPiecesRequestedCount, size_t NeededFileOffset) {
bool retvalb = false;
   for (int i = 0; i < DataPiecesRequestedCount; i++) {
      if (DataPiecesRequested[i].FileOffset == NeededFileOffset) {
         retvalb = true;
         break;
      }
   }
   return(retvalb);
}

int getIndexOfFileOffset(FilePieces DataPiecesRequested[], int DataPiecesRequestedCount, size_t file_offset) {
int retval = -1;

   for (int i = 0; i < DataPiecesRequestedCount; i++) {
      if (DataPiecesRequested[i].FileOffset == file_offset) {
         retval = i;
         break;
      }
   }
   return(retval);
}

// Returns DataPiecesRequestedCount
int ScanAndUpdateFile(FilePieces DataPiecesRequested[], int DataPiecesRequestedCount, SwarmNodeProperty *Property) {
bool loop;
char FullFileName[512];
int fd;

   do {

      loop = false;

      for (int i = 0; i < DataPiecesRequestedCount; i++) {
         // Lets see if we have received Data for CurrentOffset.
         if ( (DataPiecesRequested[i].FileOffset == Property->FileSize) &&
              (DataPiecesRequested[i].Data) ) {
            // Lets write this out to disk.
            sprintf(FullFileName, "%s%s%s", Property->DirName, DIR_SEP, Property->FileName);
            fd = open(FullFileName, O_RDWR | O_BINARY);
            lseek(fd, Property->FileSize, SEEK_SET);
            write(fd, DataPiecesRequested[i].Data, DataPiecesRequested[i].DataLength);
            close(fd);
            delete [] DataPiecesRequested[i].Data;
            DataPiecesRequested[i].Data = NULL;

            // Update our FileSize.
            Property->FileSize += DataPiecesRequested[i].DataLength;

            // Free up this DataPiecesRequested entry.
            if (DataPiecesRequestedCount == 1) {
               DataPiecesRequestedCount = 0;
            }
            else {
               // Move the last entry in this place.
               DataPiecesRequested[i].FileOffset = DataPiecesRequested[DataPiecesRequestedCount - 1].FileOffset;
               DataPiecesRequested[i].DataLength = DataPiecesRequested[DataPiecesRequestedCount - 1].DataLength;
               DataPiecesRequested[i].Data = DataPiecesRequested[DataPiecesRequestedCount - 1].Data;
               DataPiecesRequestedCount--;
            }

            loop = true;
            break;
         }
      }

   } while (loop);

   return(DataPiecesRequestedCount);
}

void getFileChunk(char *FullFilePath, size_t file_offset, size_t data_length, char *Buffer) {
int fd;

   fd = open(FullFilePath, O_RDWR | O_BINARY);
   lseek(fd, file_offset, SEEK_SET);
   read(fd, Buffer, data_length);
   close(fd);
}

