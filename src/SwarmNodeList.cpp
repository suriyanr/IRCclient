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

#include "Compatibility.hpp"

#include "SwarmNodeList.hpp"
#include "SHA1.hpp"
#include "TCPConnect.hpp"
#include "LineParse.hpp"
#include "Utilities.hpp"

#include "StackTrace.hpp"


// Constructor.
SwarmNodeList::SwarmNodeList() {
   TRACE();
   Head = NULL;
   UIEntries = NULL;
   ListCount = 0;
   FileDescriptor = -1;
   DownloadComplete = false;
   ErrorCode = SWARM_NO_ERROR;
   DownloadCompletedSpeed = 0;
   BytesSent = 0;
   BytesReceived = 0;

#ifdef __MINGW32__
   SwarmNodeListMutex = CreateMutex(NULL, FALSE, NULL);
#else
   pthread_mutex_init(&SwarmNodeListMutex, NULL);
#endif
}

// Destructor
SwarmNodeList::~SwarmNodeList() {
   TRACE();
   freeSwarmNodeList(Head);
   deleteStringArray(UIEntries);
   DestroyMutex(SwarmNodeListMutex);
}

// To initiliaise a newly alloced SwarmNode structure.
void SwarmNodeList::initSwarmNode(SwarmNode *SN) {

   TRACE();

   if (SN) {
      memset(SN, 0, sizeof(SwarmNode));
   }
}

// To free the SwarmNodeList structures and reinitialize itself.
// called by SwarmStream::quitSwarm()
// If a valid Connection is present it disconnects and frees it.
bool SwarmNodeList::freeAndInitSwarmNodeList() {
SwarmNode *Scan;

   TRACE();

   WaitForMutex(SwarmNodeListMutex);

   freeSwarmNodeList(Head);

   Head = NULL;
   ListCount = 0;

   deleteStringArray(UIEntries);
   UIEntries = NULL;
   FileDescriptor = -1;
   DownloadComplete = false;
   ErrorCode = SWARM_NO_ERROR;
   DownloadCompletedSpeed = 0;
   BytesSent = 0;
   BytesReceived = 0;

   DataPieces.freeAndInitSwarmDataPieceList();

   ReleaseMutex(SwarmNodeListMutex);

   return(true);
}

// Free the SwarmNode Link List.
// If a valid Connection exists it disconnects and frees it too.
void SwarmNodeList::freeSwarmNodeList(SwarmNode *SN) {
SwarmNode *Scan;

   TRACE();

   while (SN != NULL) {
      Scan = SN->Next;

      delete [] SN->Nick;

      if (SN->Connection) {
         // Disconnect and free it.
         SN->Connection->disConnect();
         delete SN->Connection;
      }

      delete SN;
      SN = Scan;
   }
}

// Add a Node/IP/State to the List.
bool SwarmNodeList::addToSwarmNodeNickIPState(char *Nick, unsigned long IP, int state) {
SwarmNode *SN;
bool retvalb;

   TRACE();

   SN = new SwarmNode;
   initSwarmNode(SN);
   SN->Nick = new char[strlen(Nick) + 1];
   strcpy(SN->Nick, Nick);
   SN->NodeIP = IP;
   SN->NodeState = state;

   retvalb = addToSwarmNode(SN);
   return(retvalb);
}

// Add a Nick/Size/State/Connection to the List.
bool SwarmNodeList::addToSwarmNodeNickSizeStateConnection(char *Nick, size_t FileSize, int state, TCPConnect *Connection) {
SwarmNode *SN;
bool retvalb;

   TRACE();

   SN = new SwarmNode;
   initSwarmNode(SN);
   SN->Nick = new char[strlen(Nick) + 1];
   strcpy(SN->Nick, Nick);
   SN->NodeIP = Connection->getRemoteIP();
   SN->FileSize = FileSize;
   SN->NodeState = state;
   SN->Connection = Connection;
   retvalb = addToSwarmNode(SN);
   return(retvalb);
}

// We will insert this guy wrt FileSize.
// To maintain in ascending order of FileSize.
bool SwarmNodeList::addToSwarmNode(SwarmNode *SN) {
SwarmNode *Scan, *tmpScan;;

   TRACE();

   WaitForMutex(SwarmNodeListMutex);

   SN->Next = Head;
   Head = SN;

   // Lets now insert the first entry as far down as it can go.
   Scan = Head->Next;
   tmpScan = NULL;
   while (Scan) {
      if (Scan->FileSize < Head->FileSize) {
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

   ListCount++;

   ReleaseMutex(SwarmNodeListMutex);

   return(true);
}

// is this Node already present ? If IP is not unknown, then check is done
// based on Nick
bool SwarmNodeList::isSwarmNodePresent(char *Nick, unsigned long IP) {
SwarmNode *Scan;
bool retvalb = false;

   TRACE();

   WaitForMutex(SwarmNodeListMutex);

   Scan = Head;
   while (Scan) {
      if ( (Scan->NodeIP == IRCNICKIP_UNKNOWN) || (IP == IRCNICKIP_UNKNOWN) ) {
         // Check if the nick matches if Nick is not NULL.
         if ( (Nick != NULL) && (strcasecmp(Scan->Nick, Nick) == 0) ) {
            retvalb = true;
            break;
         }
      }
      else {
         // Check if the IPs match.
         if (Scan->NodeIP == IP) {
            retvalb = true;
            break;
         }
      }
      Scan = Scan->Next;
   }

   ReleaseMutex(SwarmNodeListMutex);

   return(retvalb);
}

// Get the first Node from List and delete from List.
SwarmNode *SwarmNodeList::getFirstSwarmNode() {
SwarmNode *SN;

   TRACE();

   WaitForMutex(SwarmNodeListMutex);

   SN = Head;
   if (Head) {
      Head = Head->Next;
      SN->Next = NULL;
      ListCount--;
   }

   ReleaseMutex(SwarmNodeListMutex);

   return(SN);
}

// Returns the current Upload and Download Speed. Used while generating
// FServ Ad, but called from SwarmStream class.
bool SwarmNodeList::getCurrentSpeeds(size_t *UploadSpeed, size_t *DownloadSpeed) {
SwarmNode *SN;

   TRACE();

   *UploadSpeed = 0;
   *DownloadSpeed = 0;

   WaitForMutex(SwarmNodeListMutex);

   SN = Head;
   while (SN) {
      if (SN->Connection) {
         *UploadSpeed = *UploadSpeed + SN->Connection->getUploadBps();
         *DownloadSpeed = *DownloadSpeed + SN->Connection->getDownloadBps();
      }
      SN = SN->Next;
   }

   ReleaseMutex(SwarmNodeListMutex);

   return(true);
}


// Returns the array of Strings which represent the Nodes in a nice
// UI format. It ends with a NULL entry.
// Returns the total Upload and Download Speeds in BPS as well.
char **SwarmNodeList::getNodesUIEntries(size_t *TotalUploadBPS, size_t *TotalDownloadBPS) {
SwarmNode *SN;
int index;
const char *NodeStateString;

   TRACE();

   *TotalUploadBPS = 0;
   *TotalDownloadBPS = 0;

   WaitForMutex(SwarmNodeListMutex);

   // Lets get the # of entries to allocate the Array
   // ListCount has the # of entries.

   deleteStringArray(UIEntries);
   UIEntries = NULL;
   UIEntries = new char *[ListCount + 1]; // One for NULL.

   SN = Head;
   index = 0;
   while (SN) {
      char s_up_speed[32];
      char s_down_speed[32];
      if (index >= ListCount) break;

      // Now lets put the UI entries.
      // Nick\tFileSize\tUpRate\tDownRate\tState\tFileOffset
      UIEntries[index] = new char[strlen(SN->Nick) + 32 + 32 + 32 + 32 + 32];
      if (SN->Connection) {
         size_t uploadbps = SN->Connection->getUploadBps();
         size_t downloadbps = SN->Connection->getDownloadBps();
         *TotalUploadBPS = *TotalUploadBPS + uploadbps;
         *TotalDownloadBPS = *TotalDownloadBPS + downloadbps;

         convertFileSizeToString(uploadbps, s_up_speed);
         strcat(s_up_speed, "/s");
         convertFileSizeToString(downloadbps, s_down_speed);
         strcat(s_down_speed, "/s");
      }
      else {
         strcpy(s_up_speed, "0 B/s");
         strcpy(s_down_speed, "0 B/s");
      }
      sprintf(UIEntries[index], "%s\t%lu\t%s\t%s\t%s\t%lu",
                          SN->Nick,
                          SN->FileSize,
                          s_up_speed,
                          s_down_speed,
                          convertNodeStateToString(SN->NodeState),
                          SN->RequestedFileOffset);

      SN = SN->Next;
      index++;
   }
   UIEntries[index] = NULL;

   ReleaseMutex(SwarmNodeListMutex);

   return(UIEntries);
}

const char *SwarmNodeList::convertNodeStateToString(int state) {
static char NodeStateString[64];

   TRACE();

   switch (state) {
      case SWARM_NODE_NOT_TRIED:
      strcpy(NodeStateString, "NotTried-Idle");
      break;

      case SWARM_NODE_USERHOST_ISSUED:
      strcpy(NodeStateString, "NotTried-UserHost");
      break;

      case SWARM_NODE_IP_UNKNOWN:
      strcpy(NodeStateString, "Failed-IPUnknown");
      break;

      case SWARM_NODE_WRITE_FAILED:
      strcpy(NodeStateString, "Failed-SockWrite");
      break;

      case SWARM_NODE_READ_FAILED:
      strcpy(NodeStateString, "Failed-SockRead");
      break;

      case SWARM_NODE_ACNO:
      strcpy(NodeStateString, "Failed-ACNO");
      break;

      case SWARM_NODE_WRONGFILESIZE:
      strcpy(NodeStateString, "Failed-WrongFS");
      break;

      case SWARM_NODE_HS_MISFORMED:
      strcpy(NodeStateString, "Failed-WrongHS");
      break;

      case SWARM_NODE_FILENAME_MISMATCH:
      strcpy(NodeStateString, "Failed-WrongFN");
      break;

      case SWARM_NODE_SHA_LENGTH_MISMATCH:
      strcpy(NodeStateString, "Failed-WrongSHALen");
      break;

      case SWARM_NODE_SHA_MISMATCH:
      strcpy(NodeStateString, "Failed-WrongSHA");
      break;

      case SWARM_NODE_HS_SUCCESS:
      strcpy(NodeStateString, "Success-HS");
      break;

      case SWARM_NODE_NL_SUCCESS:
      strcpy(NodeStateString, "Success-NL");
      break;

      case SWARM_NODE_DISCONNECTED:
      strcpy(NodeStateString, "Disconnected");
      break;

      case SWARM_NODE_INITIAL_FS_SENT:
      strcpy(NodeStateString, "InitFS-Sent");
      break;

      case SWARM_NODE_DR_SENT:
      strcpy(NodeStateString, "DR-Sent");
      break;

      case SWARM_NODE_DR_RECEIVED:
      strcpy(NodeStateString, "DR-Received");
      break;

      case SWARM_NODE_DP_SENDING:
      strcpy(NodeStateString, "DP-Sending");
      break;

      case SWARM_NODE_DP_SENT:
      strcpy(NodeStateString, "DP-Sent");
      break;

      case SWARM_NODE_EXPECTING_DATAPIECE:
      strcpy(NodeStateString, "Expecting-DP");
      break;

      case SWARM_NODE_SEND_FS_EQUAL:
      strcpy(NodeStateString, "FS-Equal");
      break;

      default:
      strcpy(NodeStateString, "Unknown");
      break;
   }

   return(NodeStateString);
}

// generates the NL message string, not overflowing length.
// If no entries then returns false.
// Else Buffer is filled with NL line not exceeding BufLen.
bool SwarmNodeList::generateStringNL(char *Buffer, int BufLen) {
bool retvalb = true;
SwarmNode *Scan;
int CurBufLen;
char DeltaString[128];

   TRACE();

   strcpy(Buffer, "NL ");
   CurBufLen = 4;

   WaitForMutex(SwarmNodeListMutex);

   Scan = Head;
   while (Scan) {
      int DeltaLen;

      DeltaLen = strlen(Scan->Nick) + 8 + 2; // 2 for the 2 spaces.
      if (CurBufLen + DeltaLen > BufLen) break;
      sprintf(DeltaString, "%s %x ", Scan->Nick, Scan->NodeIP);
      strcat(Buffer, DeltaString);
      CurBufLen = CurBufLen + strlen(DeltaString);
      Scan = Scan->Next;
   }

   if (Head == NULL) retvalb = false;

   ReleaseMutex(SwarmNodeListMutex);

   // Convert the space at the end to a NewLine.
   Buffer[CurBufLen - 2] = '\n';

   COUT(cout << "SwarmNodeList::generateStringNL: " << Buffer;)
   return(retvalb);
}

int SwarmNodeList::getCount() {
int retval;

   TRACE();

   WaitForMutex(SwarmNodeListMutex);
   retval = ListCount;
   ReleaseMutex(SwarmNodeListMutex);

   return(retval);
}

// rename Nick Name.
bool SwarmNodeList::renameNickToNewNick(char *old_nick, char *new_nick) {
SwarmNode *SN;

   TRACE();

   WaitForMutex(SwarmNodeListMutex);
   SN = Head;

   while (SN) {
      if (strcasecmp(SN->Nick, old_nick) == 0) {
         if (strlen(new_nick) > strlen(old_nick)) {
            delete [] SN->Nick;
            SN->Nick = new char[strlen(new_nick) + 1];
         }
         strcpy(SN->Nick, new_nick);
      }
      SN = SN->Next;
   }

   ReleaseMutex(SwarmNodeListMutex);

   return(true);
}

// Read Messages from the Connections and Update State.
// FS message: FS SenderFileSize SenderFileHoleString SenderFileName
// DR message: DR RequestOffset SenderFileSize SenderFileName.
// ER message: ER SenderFileSize SenderFileHoleString SenderFileName
// If we change FileSize we pass it back to caller.
// returns true if a state was updated.
bool SwarmNodeList::readMessagesAndUpdateState(char *OurFileName, size_t *OurFileSize) {
SwarmNode *SN;
int retval;
char Message[512];
char DebugFutureHolesString[SWARM_MAX_FUTURE_HOLES + 1];
LineParse LineP;
const char *parseptr;
size_t temp_long;
char ReceivedData[SWARM_DATAPIECE_LENGTH];
bool retvalb = false;
bool DisconnectNode;
time_t CurrentTime = time(NULL);

   TRACE();

   WaitForMutex(SwarmNodeListMutex);

   // COUT(cout << "SwarmNodeList::readMessagesAndUpdateState Entering." << endl;)

   ErrorCode = SWARM_NO_ERROR;

   SN = Head;
   while (SN) {
      // If NodeState is SWARM_NODE_EXPECTING_DATAPIECE then we dont
      // do a readLine on it.
      // In the below do {} while (false); if we break and 
      // bool DisconnectNode is true, we disco that SN, free it and mark
      // it as SWARM_NODE_DISCONNECTED
      DisconnectNode = true;
      do {
         if (SN->Connection == NULL) break;

         if (SN->NodeState == SWARM_NODE_EXPECTING_DATAPIECE) {
            if (SN->Connection->canReadData() == false) {
               DisconnectNode = false;
               break;
            }

            // Read the Data in it and dump it in SwarmDataPieces.
            ssize_t data_read = SN->Connection->readData(ReceivedData, SN->RequestedDataLength, 0);
            if (data_read > 0) {
               BytesReceived += data_read;
               SN->ExpectingDataPiece = true;
               SN->TimeOfLastContact = CurrentTime;
               DisconnectNode = false;
               retvalb = true;

               COUT(cout << "DataPieces.addToSwarmNodePieceOffsetLenData: RequestedOffset: " << SN->RequestedFileOffset << " data_read: " << data_read << " RequestedDataLength: " << SN->RequestedDataLength << endl;)
               DataPieces.addToSwarmNodePieceOffsetLenData(SN->RequestedFileOffset, data_read, ReceivedData);
               if (data_read == SN->RequestedDataLength) {
                  SN->ExpectingDataPiece = false;
                  // We are all done reading.
                  SN->NodeState = SWARM_NODE_INITIAL_FS_SENT;

                  // This is where we should query DataPieces and see if
                  // we can advance our FileSize, and we do it as much
                  // as we can.
                  size_t LenOfDataPiece = SWARM_DATAPIECE_LENGTH;
                  bool b_change_in_fs = false;
                  while (DataPieces.getAllignedDataPiece(*OurFileSize, LenOfDataPiece, ReceivedData)) {
                     // We now need to write it out to disk
                     // We should error check below.
                     temp_long = lseek(FileDescriptor, *OurFileSize, SEEK_SET);
                     if (temp_long != *OurFileSize) {
                        // We should bail out of the Swarm.
                        COUT(cout << "lseek failed." << endl;)
                        ErrorCode = SWARM_LSEEK_FILE_FAILED;
                        goto readMesssagesAndUpdateStateExit;
                     }
                     temp_long = write(FileDescriptor, ReceivedData, LenOfDataPiece);
                     if (temp_long != LenOfDataPiece) {
                        // We should bail out of the Swarm.
                        COUT(cout << "write failed." << endl;)
                        ErrorCode = SWARM_WRITE_FILE_FAILED;
                        goto readMesssagesAndUpdateStateExit;
                     }

                     // Also delete that piece from DataPieces.
                     DataPieces.deleteAllignedDataPiece(*OurFileSize, LenOfDataPiece);

                     // This is where we notify to our equals of our FS.
                     b_change_in_fs = true;

                     // Increase our FileSize.
                     *OurFileSize = *OurFileSize + LenOfDataPiece;
                     COUT(cout << "OurFileSize increased to: " << *OurFileSize << endl;)

                     DownloadComplete = false;
                  }

                  // Check if EndGame bytes need to be written out.
                  if ( (SN->EndGame == true) &&
                       DataPieces.getAllignedEndGamePiece(*OurFileSize, &LenOfDataPiece, ReceivedData) ) {
                     // We now need to write it out to disk
                     // We should error check below.
                     temp_long = lseek(FileDescriptor, *OurFileSize, SEEK_SET);
                     if (temp_long != *OurFileSize) {
                        // We should get out of the swarm.
                        COUT(cout << "EndGame: lseek failed." << endl;)
                        ErrorCode = SWARM_LSEEK_FILE_FAILED;
                        goto readMesssagesAndUpdateStateExit;
                     }
                     temp_long = write(FileDescriptor, ReceivedData, LenOfDataPiece);
                     if (temp_long != LenOfDataPiece) {
                        // We should get out of the swarm.
                        COUT(cout << "EndGame: write failed." << endl;)
                        ErrorCode = SWARM_WRITE_FILE_FAILED;
                        goto readMesssagesAndUpdateStateExit;
                     }

                     // Also delete that piece from DataPieces.
                     DataPieces.deleteAllignedDataPiece(*OurFileSize, LenOfDataPiece);

                     // This is where we notify to our equals of our FS.
                     b_change_in_fs = true;

                     // Increase our FileSize.
                     *OurFileSize = *OurFileSize + LenOfDataPiece;
                     COUT(cout << "EndGame: OurFileSize increased to: " << *OurFileSize << endl;)

                     // Here we move the finished Swarm File to the Serving
                     // Directory.
                     // Or as it might be complicated, we just leave the moving
                     // part when we quit out of the Swarm. We just locally
                     // mark ourselves as done. So that someone can query
                     // us and know if the transfer is done fully.
                     DownloadComplete = true;

                     // Lets sync the data to disk.
                     fdatasync(FileDescriptor);
                     COUT(cout << "EndGame: fdatasync called." << endl;)

                     // As we completed this swarm download. lets get the
                     // average download speed at which we completed it.
                     DownloadCompletedSpeed = getAvgDownloadSpeed();
                  }

                  // If b_change_in_fs is true, our FS has changed
                  // *OurFileSize has the new FS.
                  // So we should loop through all our equals and mark them
                  // as SWARM_NODE_SEND_FS_EQUAL, so that we send them an FS
                  // so if a node is in FS_INIT and our size has become
                  // greater than his FS we send FS.
                  if (b_change_in_fs) {
                     markNodesForFSUpdate(*OurFileSize);
                  }
               }
               else if (data_read < SN->RequestedDataLength) {
                  // There is more data pending to be read.
                  // adjust the Offset and length for next call.
                  SN->RequestedFileOffset += data_read;
                  SN->RequestedDataLength -= data_read;
               }
               else {
                  // data_read is more than requested.
                  DisconnectNode = true;
               }
            }
            break;
         }

         retval = SN->Connection->readLine(Message, sizeof(Message), 0);
         // COUT(cout << "readLine returned: " << retval << endl;)
         if (retval == -1) {
            // Connection got Disconnected.
            break;
         }

         if (retval == 0) {
            // Nothing to be read.
            DisconnectNode = false;
            break;
         }
         BytesReceived += retval;

         // We have received a line in Message. Lets process it.
         COUT(cout << "SwarmNodeList received: " << Message << endl;)
         retvalb = true;
         SN->TimeOfLastContact = CurrentTime;

         // First three characters determine message type.
         if (strncasecmp(Message, "FS ", 3) == 0) {
            // FS message: FS SenderFileSize SenderFileHoleString
            LineP = Message;

            // Exactly 3 words.
            if (LineP.getWordCount() != 3) break;

            // Check if FileSize he has is valid since what we had last.
            parseptr = LineP.getWord(2);
            temp_long = strtoul(parseptr, NULL, 16);
            if (temp_long < SN->FileSize) break;
            SN->FileSize = temp_long;

            // Lets fill the hole information that he has sent.
            parseptr = LineP.getWord(3);
            strncpy(SN->FileFutureHoles, parseptr, SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN);
            SN->FileFutureHoles[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN] = '\0';
            COUT(cout << "Valid FS line" << endl;)
            DisconnectNode = false;

            // If this NodeState is SWARM_NODE_DP_SENT, we need to mark it as
            // SWARM_NODE_INITIAL_FS_SENT, which could prompt us to get data
            // from this guy if we has got more data now than us.
            if (SN->NodeState == SWARM_NODE_DP_SENT) {
               SN->NodeState = SWARM_NODE_INITIAL_FS_SENT;
               COUT(cout << "Changing NodeState from SWARM_NODE_DP_SENT to SWARM_NODE_INITIAL_FS_SENT" << endl;)
            }
            break;
         }

         // Check if ER message.
         if (strncasecmp(Message, "ER ", 3) == 0) {
            // ER message: ER SenderFileSize SenderFileHoleString
            LineP = Message;

            // Exactly 3 words.
            if (LineP.getWordCount() != 3) break;

            // As he has sent an error we might possibly have wrong
            // Filesize that we hold, so just copy this as correct filesize.
            SN->FileSize = strtoul(parseptr, NULL, 16);

            // Lets fill the hole information that he has sent.
            parseptr = LineP.getWord(3);
            strncpy(SN->FileFutureHoles, parseptr, SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN);
            SN->FileFutureHoles[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN] = '\0';
            COUT(cout << "Valid ER line" << endl;)
            DisconnectNode = false;
            break;
         }

         // Check if its a DR message.
         if (strncasecmp(Message, "DR ", 3) == 0) {
            // DR RequestedOffset RequestedLength HisFileSize HisFutureHoles
            LineP = Message;

            // Exactly 5 words
            if (LineP.getWordCount() != 5) break;

            // Check if FileSize he has is valid since what we had last.
            parseptr = LineP.getWord(4);
            temp_long = strtoul(parseptr, NULL, 16);
            if (temp_long < SN->FileSize) break;
            SN->FileSize = temp_long;

            // Lets fill the hole information that he has sent.
            parseptr = LineP.getWord(5);
            strncpy(SN->FileFutureHoles, parseptr, SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN);
            SN->FileFutureHoles[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN] = '\0';
            convertByteArrayToString(SN->FileFutureHoles, SWARM_MAX_FUTURE_HOLES, DebugFutureHolesString);
            COUT(cout << "Swarm: readMess: DebugFutureHoles: " << DebugFutureHolesString << endl;)

            parseptr = LineP.getWord(2);
            SN->RequestedFileOffset = strtoul(parseptr, NULL, 16);
            parseptr = LineP.getWord(3);
            SN->RequestedDataLength = strtoul(parseptr, NULL, 16);

            SN->NodeState = SWARM_NODE_DR_RECEIVED;

            COUT(cout << "Valid DR line" << endl;)
            DisconnectNode = false;
            break;
         }

         // Check if its a DP Message.
         if (strncasecmp(Message, "DP ", 3) == 0) {
            // DP FileOffset DataLength HisFileSize HisHoles
            LineP = Message;

            // Exactly 5 words
            if (LineP.getWordCount() != 5) break;

            // Check if FileSize he has is valid since what we had last.
            parseptr = LineP.getWord(4);
            temp_long = strtoul(parseptr, NULL, 16);
            if (temp_long < SN->FileSize) break;
            SN->FileSize = temp_long;

            // Lets fill the hole information that he has sent.
            parseptr = LineP.getWord(5);
            strncpy(SN->FileFutureHoles, parseptr, SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN);
            SN->FileFutureHoles[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN] = '\0';

            // Verify that we had really asked for this data from him.
            parseptr = LineP.getWord(2);
            temp_long = strtoul(parseptr, NULL, 16);
            if (temp_long != SN->RequestedFileOffset) break;

            parseptr = LineP.getWord(3);
            temp_long = strtoul(parseptr, NULL, 16);
            if (temp_long != SN->RequestedDataLength) break;

            SN->NodeState = SWARM_NODE_EXPECTING_DATAPIECE;

            COUT(cout << "Valid DP line" << endl;)
            DisconnectNode = false;
            break;
         }

      } while (false);

      // If DisconnectNode is true, lets disco and free it.
      if (DisconnectNode && SN->Connection) {
         SN->Connection->disConnect();
         delete SN->Connection;
         SN->Connection = NULL;
         SN->NodeState = SWARM_NODE_DISCONNECTED;
      }

      SN = SN->Next;
   }


readMesssagesAndUpdateStateExit:

   // Remove entries with NodeState = SWARM_NODE_DISCONNECTED
   removeDisconnectedNodes();

   // Arrange the List back in Order wrt FileSize.
   orderByFileSize();

   ReleaseMutex(SwarmNodeListMutex);

   return(retvalb);
}

// Write Messages to the Connections and Update State.
// So what do we write and when do we write it.
// If state = SWARM_NODE_NL_SUCCESS, we just connected sucesfully, send out
// an FS message. So we inform him of what we have and the future holes.
//  - FS FileSize FutureHoles
// FutureHoles will be SWARM_MAX_CONNECTED_NODES in length if we represent it
// as made up of characters 0 and 1
// DR message->
//  - DR RequestedOffset RequestedLength MyFileSize MyFutureHoles
// returns true if a state was updated.
bool SwarmNodeList::writeMessagesAndUpdateState(char *OurFileName, size_t OurFileSize) {
char FutureHoles[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1];
char DebugFutureHolesString[SWARM_MAX_FUTURE_HOLES + 1];
char FSMessage[512], Message[512];
SwarmNode *SN;
size_t DataRequestOffset, DataRequestLength;
int retval;
bool retvalb;
bool returnvalb = false;
char DataPieceOfDR[SWARM_DATAPIECE_LENGTH];
time_t CurrentTime = time(NULL);

   TRACE();

   WaitForMutex(SwarmNodeListMutex);

   generateFutureHolesString(OurFileSize, FutureHoles);
   //convertByteArrayToString(FutureHoles, SWARM_MAX_FUTURE_HOLES, DebugFutureHolesString);
   //COUT(cout << "Swarm: writeMess: DebugFutureHoles: " << DebugFutureHolesString << endl;)

   sprintf(FSMessage, "FS %lx %s\n", OurFileSize, FutureHoles);

   SN = Head;
   while (SN) {
      if (SN->Connection == NULL) {
         SN = SN->Next;
         continue;
      }

      // if Connection can accept a write.
      if (SN->Connection->canWriteData() == false) {
         SN = SN->Next;
         continue;
      }

      switch (SN->NodeState) {
         case SWARM_NODE_NL_SUCCESS:
         // Need to send him an FS message as we have just exchanged NickLists.
         case SWARM_NODE_SEND_FS_EQUAL:
         // Need to send him an FS message as we have more than him in FileSize.

         // We send this message not more than once a second ?
         if ( (SN->TimeFileSizeSent + 1) > CurrentTime) break;

         returnvalb = true;
         // Connection has just been established. Send FS message.
         COUT(cout << "SwarmNodeList::writeMessagesAndUpdateState: " << FSMessage;)
         retval = SN->Connection->writeData(FSMessage, strlen(FSMessage), SWARM_CONNECTION_TIMEOUT);
         if (retval != strlen(FSMessage)) {
            SN->Connection->disConnect();
            delete SN->Connection;
            SN->Connection = NULL;
            SN->NodeState = SWARM_NODE_DISCONNECTED;
         }
         else {
            BytesSent += retval;
            SN->NodeState = SWARM_NODE_INITIAL_FS_SENT;
            SN->TimeOfLastContact = CurrentTime;
            SN->TimeFileSizeSent = CurrentTime;
         }

         break;

         case SWARM_NODE_INITIAL_FS_SENT:
         // Lets see if we can send a DR to this guy.
         // This part is crucial. Who we send DR requests to.
         // Research later. As of now, send DR and get some data flowing.
         retvalb = getOffsetForDR(SN, &DataRequestOffset, &DataRequestLength, OurFileSize, FutureHoles);
         if (retvalb == false) break;

         returnvalb = true;
         // We are here so we can send a DR for DataRequestOffset from this
         // Node.
         //  - DR RequestedOffset RequestedLength MyFileSize MyFutureHoles
         sprintf(Message, "DR %lx %lx %lx %s\n",
                             DataRequestOffset,
                             DataRequestLength,
                             OurFileSize,
                             FutureHoles);
         COUT(cout << "SwarmNodeList::writeMessagesAndUpdateState: " << Message;)
         //convertByteArrayToString(FutureHoles, SWARM_MAX_FUTURE_HOLES, DebugFutureHolesString);
         //COUT(cout << "Swarm: writeMess: FutureHoles: " << FutureHoles << " DebugFutureHoles: " << DebugFutureHolesString << " OurFileName: " << OurFileName << endl;)
         retval = SN->Connection->writeData(Message, strlen(Message), SWARM_CONNECTION_TIMEOUT);
         if (retval != strlen(Message)) {
            SN->Connection->disConnect();
            delete SN->Connection;
            SN->Connection = NULL;
            SN->NodeState = SWARM_NODE_DISCONNECTED;
         }
         else {
            BytesSent += retval;
            SN->NodeState = SWARM_NODE_DR_SENT;
            SN->TimeDataRequestSent = CurrentTime;
            SN->TimeOfLastContact = CurrentTime;
            SN->RequestedFileOffset = DataRequestOffset;
            SN->RequestedDataLength = DataRequestLength;
         }

         break;

         case SWARM_NODE_DR_RECEIVED:
         returnvalb = true;
         // We have his requested offset and length in RequestedFileOffset,
         // RequestedDataLength
         // We need to see if we can fulfill his request. If not sent a
         // ER Error Message.
         retvalb = getRequestedDataPiece(SN, DataPieceOfDR);
         if (retvalb == false) {
            // Send the Error.
            // - ER MyFileSize MyFileHoles
            sprintf(Message, "ER %lx %s\n",
                                OurFileSize,
                                FutureHoles);
            COUT(cout << "SwarmNodeList::writeMessagesAndUpdateState: " << Message;)
            retval = SN->Connection->writeData(Message, strlen(Message), SWARM_CONNECTION_TIMEOUT);
            if (retval != strlen(Message)) {
               SN->Connection->disConnect();
               delete SN->Connection;
               SN->Connection = NULL;
               SN->NodeState = SWARM_NODE_DISCONNECTED;
            }
            else {
               BytesSent += retval;
               // We revert state to idle.
               SN->NodeState = SWARM_NODE_INITIAL_FS_SENT;
               SN->TimeOfLastContact = CurrentTime;
            }
         }
         else {
            // We have the Data that he requested. Lets send the DP init
            // line - DP FileOffset DataLength MyFileSize MyHoles MyFileName
            sprintf(Message, "DP %lx %lx %lx %s\n",
                              SN->RequestedFileOffset,
                              SN->RequestedDataLength,
                              OurFileSize,
                              FutureHoles);
            COUT(cout << "SwarmNodeList::writeMessagesAndUpdateState: " << Message;)
            retval = SN->Connection->writeData(Message, strlen(Message), SWARM_CONNECTION_TIMEOUT);
            if (retval != strlen(Message)) {
               SN->Connection->disConnect();
               delete SN->Connection;
               SN->Connection = NULL;
               SN->NodeState = SWARM_NODE_DISCONNECTED;
            }
            else {
               BytesSent += retval;
               SN->NodeState = SWARM_NODE_DP_SENDING;
               SN->TimeOfLastContact = CurrentTime;
            } 
         }
         break;

         case SWARM_NODE_DP_SENDING:
         returnvalb = true;
         // We have his requested offset and length in RequestedFileOffset,
         // RequestedDataLength
         retvalb = getRequestedDataPiece(SN, DataPieceOfDR);
         // This has to be true, as we checked last time.
         // Write the actual data out.
         COUT(cout << "SwarmNodeList::writeMessagesAndUpdateState: sending actual Data. Offset: " << SN->RequestedFileOffset << " Length: " << SN->RequestedDataLength << endl;)
         retval = SN->Connection->writeData(DataPieceOfDR, SN->RequestedDataLength, 0);
         if (retval <= 0) {
            SN->Connection->disConnect();
            delete SN->Connection;
            SN->Connection = NULL;
            SN->NodeState = SWARM_NODE_DISCONNECTED;
         }
         else {
            BytesSent += retval;
            SN->TimeOfLastContact = CurrentTime;
            SN->RequestedFileOffset += retval;
            SN->RequestedDataLength -= retval;
            if (SN->RequestedDataLength == 0) {
               SN->NodeState = SWARM_NODE_DP_SENT;
            }
         }
         break;

         case SWARM_NODE_EXPECTING_DATAPIECE:
         case SWARM_NODE_DR_SENT:
         if (SN->TimeDataRequestSent + SWARM_DATAREQUEST_TIMEOUT < CurrentTime) {
            SN->TimeDataRequestSent = CurrentTime;
            // Its been a while since we heard from him, send FS Message
            returnvalb = true;
            COUT(cout << "SwarmNodeList:: Checking if Alive: " << FSMessage;)
            retval = SN->Connection->writeData(FSMessage, strlen(FSMessage), SWARM_CONNECTION_TIMEOUT);
            if (retval != strlen(FSMessage)) {
               SN->Connection->disConnect();
               delete SN->Connection;
               SN->Connection = NULL;
               SN->NodeState = SWARM_NODE_DISCONNECTED;
            }
            else {
               BytesSent += retval;
               // After sending the FS message, we let the state remain the same.
               SN->TimeOfLastContact = CurrentTime;
            }
         }
         break;


         default:
         break;
      }

      SN = SN->Next;
   }

   // Remove entries with NodeState = SWARM_NODE_DISCONNECTED
   removeDisconnectedNodes();

   ReleaseMutex(SwarmNodeListMutex);

   return(returnvalb);
}

// get an apt offset and length to request as DR from the node.
// returns true and modifies offset, and modifies data_length.
//  also marks the returned offset to be marked with 01 in OurFutureHoles
// returns false if nothing apt.
// Caller has acquired Locks.
// This is called when node state is SWARM_NODE_INITIAL_FS_SENT, and hence is
// free to be sent a DR request.
bool SwarmNodeList::getOffsetForDR(SwarmNode *SN, size_t *file_offset, size_t *data_length, size_t OurFileSize, char OurFutureHoles[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1]) {
SwarmNode *Scan;
int BitIndex;
int HoleIndex;
bool retvalb = false;
int i;

   TRACE();

   *data_length = SWARM_DATAPIECE_LENGTH;

   // Find the nearest hole we need to cover and which can be provided by this
   // SN.
   *file_offset = OurFileSize;
   for (i = 0; i < SWARM_MAX_FUTURE_HOLES; i++) {
      // Lets check if this hole index can be requested.
      // 11 is hole got, 10 = hole requested. so just check if that first
      // bit is clear to indicate free to request.
      HoleIndex = i;
      BitIndex =  (2 * HoleIndex) + ((HoleIndex/ 3) * 2) + 2;
      if (!isSetBitInByteArray(OurFutureHoles, BitIndex)) {
         // We can fill this hole up.
         // Check if this SN can provide this hole.
         *file_offset = OurFileSize + (i * SWARM_DATAPIECE_LENGTH);
         if ((*file_offset + SWARM_DATAPIECE_LENGTH) <= SN->FileSize) {
            // His FileSize itself covers it up.
            retvalb = true;

            // Mark this hole to be in progress, marked as 10
            setBitInByteArray(OurFutureHoles, BitIndex);
            clrBitInByteArray(OurFutureHoles, BitIndex + 1);
            break;
         }

         // We are here => *file_offset + SWARM_DATAPIECE_LENGTH > SN->FileSize
         // Now check if his filesize with his hole matrix together
         // can provide us with the data.
         // Find the index in hole of SN for this file_offset.
         size_t sn_hole_offset; // so many bytes away from SN->FileSize.
         if (*file_offset >= SN->FileSize) {
            sn_hole_offset = (*file_offset - SN->FileSize);
         }
         else {
            sn_hole_offset = (SN->FileSize - *file_offset);
         }
         if ((sn_hole_offset % SWARM_DATAPIECE_LENGTH) == 0) {
            // Data will be contained in a single index.
            // and *file_offset is >= SN->FileSize.
            // See if he has it.
            HoleIndex = sn_hole_offset / SWARM_DATAPIECE_LENGTH;
            if (HoleIndex < SWARM_MAX_FUTURE_HOLES) {
               // It is contained, but is it available ? that is 11
               BitIndex = (2 * HoleIndex) + ((HoleIndex/ 3) * 2) + 2;
               if (isSetBitInByteArray(SN->FileFutureHoles, BitIndex) &&
                   isSetBitInByteArray(SN->FileFutureHoles, BitIndex + 1) ) {
                  retvalb = true;

                  // Mark this hole to be in progress, marked as 10
                  HoleIndex = i;
                  BitIndex =  (2 * HoleIndex) + ((HoleIndex/ 3) * 2) + 2;

                  setBitInByteArray(OurFutureHoles, BitIndex);
                  clrBitInByteArray(OurFutureHoles, BitIndex + 1);
                  COUT(cout << "Swarm: Requesting from his Hole contained in one Index" << endl;)
                  break;
               }
            }
         }
         else {
            // There are two cases here.
            // First case: SN->FS > *file_offset.
            //  - In this case we just need to see if he has his first hole
            //  - occupied, which is impossible, as he would have shown a
            //  - a greater filesize in the first place.
            //  - Hence no first case - we weed it out.
            // Second case:
            //   SN->FS < *file_offset.
            // Data will be contained in two consecutive indices.
            // See if its really contained.
            if (SN->FileSize < *file_offset) {
               HoleIndex = sn_hole_offset / SWARM_DATAPIECE_LENGTH;
               if (HoleIndex < SWARM_MAX_FUTURE_HOLES - 1) {
                  // It is contained, but is it available ?
                  BitIndex = (2 * HoleIndex) + ((HoleIndex/ 3) * 2) + 2;
                  HoleIndex++;
                  int BitIndex1 = (2 * HoleIndex) + ((HoleIndex/ 3) * 2) + 2;
                  if (isSetBitInByteArray(SN->FileFutureHoles, BitIndex) &&
                      isSetBitInByteArray(SN->FileFutureHoles, BitIndex + 1) &&
                      isSetBitInByteArray(SN->FileFutureHoles, BitIndex1) &&
                      isSetBitInByteArray(SN->FileFutureHoles, BitIndex1 + 1)) {

                     retvalb = true;

                     // Mark this hole to be in progress, marked as 10
                     HoleIndex = i;
                     BitIndex =  (2 * HoleIndex) + ((HoleIndex/ 3) * 2) + 2;

                     setBitInByteArray(OurFutureHoles, BitIndex);
                     clrBitInByteArray(OurFutureHoles, BitIndex + 1);
                     COUT(cout << "Swarm: Requesting from his Hole contained in two Index" << endl;)
                     break;
                  }
               }
            }
         }
      }
   }

   return(retvalb);
}

// get the requested data from offset and of length as in SN
// returns false if we dont have that piece.
// buffer is at least of length SWARM_DATAPIECE_LENGTH
bool SwarmNodeList::getRequestedDataPiece(SwarmNode *SN, char *buffer) {
bool retvalb;

   TRACE();

   do {
      // Lets see if its present in the DataPieces structure.
      retvalb = DataPieces.getDataPiece(SN->RequestedFileOffset, SN->RequestedDataLength, buffer);
      if (retvalb) break;

      // We need to get this data from the FileDescriptor.
      if (lseek(FileDescriptor, SN->RequestedFileOffset, SEEK_SET) == -1) break;

      // Lets read in the bytes.
      if (read(FileDescriptor, buffer, SN->RequestedDataLength) == -1) break;
      retvalb = true;

   } while (false);

   return(retvalb);
}

// called by SwarmStream class to set the FileDescriptor.
bool SwarmNodeList::setFileDescriptor(int fd) {
   TRACE();

   FileDescriptor = fd;

   return(true);
}

// Write FS Messages to the Connections whose LastUpdateTime is old.
// returns true if something was detected as a dead node.
// We do not update the state. We update LastUpdateTime if we succesfully
// sent the FS out.
// Do not send if NodeState is SWARM_NODE_DP_SENDING, as that will garbage
// out the piece we are in the middle of sending.
bool SwarmNodeList::checkForDeadNodes(char *OurFileName, size_t OurFileSize) {
SwarmNode *SN;
char FutureHoles[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1];
char FSMessage[512];
ssize_t retval;
bool retvalb = false;
time_t CurrentTime = time(NULL);

   TRACE();

   WaitForMutex(SwarmNodeListMutex);

   generateFutureHolesString(OurFileSize, FutureHoles);
   sprintf(FSMessage, "FS %lx %s\n", OurFileSize, FutureHoles);

   SN = Head;
   while (SN) {
      do {
         if (SN->Connection == NULL) break;
         if (SN->NodeState == SWARM_NODE_DP_SENDING) break;
         if (SN->Connection->canWriteData() == false) break;
         if (SN->TimeOfLastContact == 0) break;
         if (CurrentTime <= (SN->TimeOfLastContact + SWARM_CONNECTION_TIMEOUT))
            break;

         // Send FS message.
         COUT(cout << "SwarmNodeList::checkForDeadNodes: " << FSMessage;)
         retval = SN->Connection->writeData(FSMessage, strlen(FSMessage), SWARM_CONNECTION_TIMEOUT);
         if (retval != strlen(FSMessage)) {
            SN->Connection->disConnect();
            delete SN->Connection;
            SN->Connection = NULL;
            SN->NodeState = SWARM_NODE_DISCONNECTED;
            retvalb = true;
         }
         else {
            BytesSent += retval;
            SN->TimeOfLastContact = CurrentTime;
            SN->TimeFileSizeSent = CurrentTime;
         }
      } while (false);

      SN = SN->Next;
   }

   // Remove entries with NodeState = SWARM_NODE_DISCONNECTED
   if (retvalb) {
      removeDisconnectedNodes();
   }

   ReleaseMutex(SwarmNodeListMutex);
}

// Check for EndGame and do the necessary to get the Last Piece if at all.
// EndGame is defined when we have reached the state where there is only
// one piece left to be got which is towards the end of file, and the
// length of that piece is < SWARM_DATAPIECE_LENGTH.
// To detect if we have reached that state the following conditions hold
// true:
// - We do not have an outstanding DR request pending.
// - We do not have any future holes
// If all of the above scenarios are met, then go thru all the SwarmNode
// list, and check for nodes which have the last piece. All such nodes
// should have the same size, and their FutureHoles should also not have
// any pending or Hole.
// If that is also true, then send a DR request to one of the nodes, which
// is free.
// Returns true if we are in the EndGame phase.
// Returns false if we are not in the EndGame.
bool SwarmNodeList::checkForEndGame(char *OurFileName, size_t OurFileSize) {
char FutureHoles[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1];
SwarmNode *NodeWithMaxFileSize = NULL, *SN;
size_t MaxFileSize = 0;
size_t DataPieceLength = 0;
char Message[512];
bool retvalb = false;
size_t retval;

   TRACE();

   if (DataPieces.getFutureHolesByteArray(OurFileSize, FutureHoles) == true) {
      // There is at least one occupied Future Hole.
      return(retvalb);
   }

   // Now check if we have any pending Requests.
   // At the same time note down the max FileSize in the Swarm.
   retvalb = true;
   WaitForMutex(SwarmNodeListMutex);
   SN = Head;
   while (SN) {
      if ( (SN->NodeState == SWARM_NODE_DR_SENT) ||
           (SN->NodeState == SWARM_NODE_EXPECTING_DATAPIECE) ) {
         // We are pending some holes.
         // So not in EndGame yet.
         retvalb = false;
         break;
      }

      // Lets note down the Node with max FileSize.
      if (SN->FileSize > MaxFileSize) {
         NodeWithMaxFileSize = SN;
         MaxFileSize = SN->FileSize;
      }
      SN = SN->Next;
   }

   if ( (Head == NULL) || (MaxFileSize == OurFileSize) || (retvalb == false) ) {
      retvalb = false;
      ReleaseMutex(SwarmNodeListMutex);
      return(retvalb);
   }

   SN = NodeWithMaxFileSize;
   retvalb = false;

   // We are here => No pending DRs
   if (SN) {
      // Check if this Node has pending future holes or future holes.
      if (!isFutureHolePresentOrPending(SN->FileFutureHoles)) {
         // This node too doesnt have any holes or pending holes.
         DataPieceLength = MaxFileSize - OurFileSize;
         if (DataPieceLength < SWARM_DATAPIECE_LENGTH) {
            // Make sure this is really the last piece.
            retvalb = true;
         }
      }
   }


   // If retvalb == true, send DR request to NodeWithMaxFileSize.
   if (retvalb) {
      //  - DR RequestedOffset RequestedLength MyFileSize MyFutureHoles MyFileName
      sprintf(Message, "DR %lx %lx %lx %s\n",
                             OurFileSize,
                             DataPieceLength,
                             OurFileSize,
                             FutureHoles);
      COUT(cout << "SwarmNodeList::checkForEndGame: " << Message;)
      retval = SN->Connection->writeData(Message, strlen(Message), SWARM_CONNECTION_TIMEOUT);
      if (retval != strlen(Message)) {
         SN->Connection->disConnect();
         delete SN->Connection;
         SN->Connection = NULL;
         SN->NodeState = SWARM_NODE_DISCONNECTED;
         retvalb = false;
      }
      else {
         BytesSent += retval;
         SN->NodeState = SWARM_NODE_DR_SENT;
         SN->TimeDataRequestSent = time(NULL);
         SN->RequestedFileOffset = OurFileSize;
         SN->RequestedDataLength = DataPieceLength;
         SN->EndGame = true;
      }
   }

   ReleaseMutex(SwarmNodeListMutex);
   return(retvalb);
}

// Used to get the Max FileSize present in any Node for the swarm file.
// Currently used to populate the percentage in the UI.
size_t SwarmNodeList::getGreatestFileSize() {
size_t MaxFileSize = 0;
SwarmNode *SN;

   TRACE();

   WaitForMutex(SwarmNodeListMutex);

   SN = Head;
   while (SN) {
      if (SN->FileSize > MaxFileSize) {
         MaxFileSize = SN->FileSize;
      }
      SN = SN->Next;
   }

   ReleaseMutex(SwarmNodeListMutex);

   return(MaxFileSize);
}

// Go through List and mark the nodes that are SWARM_NODE_INITIAL_FS_SENT
// and have FileSize < than OurFileSize, as SWARM_NODE_SEND_FS_EQUAL
// Assume the lock is held by caller.
bool SwarmNodeList::markNodesForFSUpdate(size_t OurFileSize) {
SwarmNode *SN;
bool retvalb = false;

   TRACE();

   SN = Head;
   while (SN) {
      if ( (SN->NodeState == SWARM_NODE_INITIAL_FS_SENT) &&
           (SN->FileSize <= OurFileSize) ) {
         // This is the guy we mark.
         SN->NodeState = SWARM_NODE_SEND_FS_EQUAL;

         retvalb = true;
      }
      SN = SN->Next;
   }

   return(retvalb);
}

// Used to check if the download is complete, so it can be moved.
bool SwarmNodeList::isDownloadComplete() {
   TRACE();

   return(DownloadComplete);
}

// Used to update the CAPs for the Connections.
// Its called once in 5 seconds.
bool SwarmNodeList::updateCAPFromGlobal(size_t PerTransferMaxUploadBPS, size_t OverallMaxUploadBPS, size_t PerTransferMaxDownloadBPS, size_t OverallMaxDownloadBPS) {
SwarmNode *SN;

   TRACE();

   WaitForMutex(SwarmNodeListMutex);

   SN = Head;
   while (SN) {
      if (SN->Connection) {
         SN->Connection->setMaxUploadBps(PerTransferMaxUploadBPS);
         SN->Connection->setOverallMaxUploadBps(OverallMaxUploadBPS);
         SN->Connection->setMaxDwnldBps(PerTransferMaxDownloadBPS);
         SN->Connection->setOverallMaxDwnldBps(OverallMaxDownloadBPS);
      }
      SN = SN->Next;
   }

   ReleaseMutex(SwarmNodeListMutex);

   return(true);
}

// Go through the List and remove Node with
// NodeState = SWARM_NODE_DISCONNECTED
// Assume lock is held.
bool SwarmNodeList::removeDisconnectedNodes() {
SwarmNode *SN, *preSN, *tmpSN;

   TRACE();

   SN = Head;
   preSN = NULL;
   while (SN) {
      if (SN->NodeState == SWARM_NODE_DISCONNECTED) {
         // Check if he was receiving some DPs, in which case we need to 
         // delete the partial data that he has given us.
         if (SN->ExpectingDataPiece) {
            COUT(cout << "SwarmNodeList::removeDisconnectedNodes: found node from which we were receiving a piece. Calling DataPiece.deleteDataPiece with offset: " << SN->RequestedFileOffset << endl;)
            DataPieces.deleteDataPiece(SN->RequestedFileOffset);
         }

         ListCount--;
         // This guy needs to go.
         if (SN == Head) {
            // Head Element
            Head = Head->Next;
            SN->Next = NULL;
            freeSwarmNodeList(SN);
            SN = Head;
         }
         else {
            // Non Head element. => preSN has pointer to previous struct.
            tmpSN = preSN; // We will set SN to this value

            preSN->Next = SN->Next;
            SN->Next = NULL;
            freeSwarmNodeList(SN);
            SN = tmpSN;
         }
      }

      // If node got deleted above, SN could land up NULL here.
      // If only one node in list or deleting the last node cases.
      if (SN == NULL) break;

      preSN = SN;
      SN = SN->Next;
   }

   return(true);
}

// return the ErrorCode
int SwarmNodeList::getErrorCode() {

   TRACE();

   return(ErrorCode);
}

// To get the Error code as a string.
void SwarmNodeList::getErrorCodeString(char *ErrorString) {

   TRACE();

   switch (ErrorCode) {
      case SWARM_NO_ERROR:
      sprintf(ErrorString, "No Error.");
      break;

      case SWARM_LSEEK_FILE_FAILED:
      sprintf(ErrorString, "lseek() on Swarm File Failed.");
      break;

      case SWARM_WRITE_FILE_FAILED:
      sprintf(ErrorString, "write() on Swarm File Failed. Check if it can be written to, or you have space on disk.");
      break;

      default:
      sprintf(ErrorString, "Unknown Error - should not happen.");
      break;
   }
}


// Orders the SwarmNodes in ascending order of FileSize.
// Called by readMessagesAndUpdateState() just before it returns.
// as it is the one which possibly has updated the FileSize of a SwarmNode.
// Assume lock is held.
bool SwarmNodeList::orderByFileSize() {
SwarmNode *SN, *preSN, *tempSN, *nextSN;
bool retvalb = true;
bool swapped;

   do {
      swapped = false;
      SN = Head;
      while (SN && SN->Next) {
         // We can arrange only if there are at least two entries.
         if (SN->Next->FileSize < SN->FileSize) {
            // They are out of order, lets swap them.
            swapped = true;
            if (SN == Head) {
               // First Element
               Head = SN->Next;
               tempSN = SN->Next->Next;
               Head->Next = SN;
               SN->Next = tempSN;
               // End of swap.
               SN = Head; // So that we move to next in the loop.
            }
            else {
               // Non First Element => preSN is valid.
               nextSN = SN->Next;
               preSN->Next = nextSN;
               SN->Next = nextSN->Next;
               nextSN->Next = SN;
               // End of swap.
               SN = nextSN; // So that we move to next in the loop.
            }
         }

         preSN = SN;
         SN = SN->Next;
      }
   } while (swapped);

   return(retvalb);
}

// Generates the FutureHoles string of what we hold.
// We always set the 0th bit and 1st bit of all bytes. (so that a string
// terminator is not injected). So only 6 bits used per byte.
// Assume lock is held.
bool SwarmNodeList::generateFutureHolesString(size_t OurFileSize, char OurFutureHoles[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1]) {
SwarmNode *SN;
int HoleIndex;
int BitIndex;
bool retvalb = true;

   TRACE();

   DataPieces.getFutureHolesByteArray(OurFileSize, OurFutureHoles);
   // Of these FutureHoles, we need to mark the ones we already requested
   // as 10
   SN = Head;
   while (SN) {
      if ( (SN->NodeState == SWARM_NODE_DR_SENT) ||
           (SN->NodeState == SWARM_NODE_EXPECTING_DATAPIECE) ) {
         // Mark this one as already requested.
         if (SN->RequestedFileOffset >= OurFileSize) {
            HoleIndex = (SN->RequestedFileOffset - OurFileSize) / SWARM_DATAPIECE_LENGTH;
            // Just a sanity check.
            if ( (HoleIndex >= 0) && (HoleIndex < SWARM_MAX_FUTURE_HOLES) ) {
               // 3, cause 3 holes per byte. + 2 => skip first 2 bits.
               BitIndex =  (2 * HoleIndex) + ((HoleIndex/ 3) * 2) + 2;
               setBitInByteArray(OurFutureHoles, BitIndex);
               clrBitInByteArray(OurFutureHoles, BitIndex + 1);
               // The pieces we have requested are now marked as 10
            }
         }
      }
      SN = SN->Next;
   }

   return(retvalb);
}

// Check if a FutureHoles has a hole present or in pending.
// Hole with DataPiece = 11
// Hole with DataPiece requested and pending = 10
bool SwarmNodeList::isFutureHolePresentOrPending(char FutureHole[SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN + 1]) {
bool retvalb = false;
int HoleIndex;
int BitIndex;

   TRACE();

   for (int i = 0; i < SWARM_MAX_FUTURE_HOLES; i++) {
      HoleIndex = i;
      BitIndex =  (2 * HoleIndex) + ((HoleIndex/ 3) * 2) + 2;
      if (isSetBitInByteArray(FutureHole, BitIndex)) {
         retvalb = true;
         break;
      }
   }

   return(retvalb);
}

// Obtains the sum of all download speeds across all nodes.
// Called after end game piece is obtained.
// Assume lock is held.
size_t SwarmNodeList::getAvgDownloadSpeed() {
size_t download_speed;
SwarmNode *SN;

   TRACE();

   download_speed = 0;
   if (DownloadComplete != true) return(download_speed);

   SN = Head;
   while (SN) {
      if (SN->Connection) {
         download_speed = download_speed + SN->Connection->getAvgDownloadBps();
      }
      SN = SN->Next;
   }
   return(download_speed);
}

// Used to get the BytesSent value = bytes sent so far.
// Its reset to 0 once the value is read.
// Lock is not held.
size_t SwarmNodeList::getBytesSent() {
size_t var;

   TRACE();

   WaitForMutex(SwarmNodeListMutex);

   var = BytesSent;
   BytesSent = 0;

   ReleaseMutex(SwarmNodeListMutex);

   return(var);
}

// Used to get the BytesReceived value = bytes received so far.
// Its reset to 0 once the value is read.
size_t SwarmNodeList::getBytesReceived() {
size_t var;

   TRACE();

   WaitForMutex(SwarmNodeListMutex);

   var = BytesReceived;
   BytesReceived = 0;

   ReleaseMutex(SwarmNodeListMutex);

   return(var);
}

// Returns the DownloadCompletedSpeed. Note that it will be valid only
// if DownloadComplete is true.
size_t SwarmNodeList::getDownloadCompletedSpeed() {
size_t var;

   TRACE();

   WaitForMutex(SwarmNodeListMutex);

   var = DownloadCompletedSpeed;

   ReleaseMutex(SwarmNodeListMutex);

   return(var);
}

