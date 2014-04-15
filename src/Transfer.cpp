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

#include "Transfer.hpp"
#include "Utilities.hpp"
#include "StackTrace.hpp"
#include "Helper.hpp"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "Compatibility.hpp"

// To turn on Transfer Debugging Uncomment next line.
//#define DEBUG


// This Class provides the actual File Transfer.

Transfer::Transfer() {
   TRACE();
   Buffer = new char[TRANSFER_SIZE];
   RollbackBuffer = NULL;
   FileDescriptor = -1;
   DCCBytesAck = 0;
   LastUpdateFilesDetailTime = 0;
   memset(LastFourAckBytes, 0, 4);
   DisconnectMessage = CONNECTION_MESSAGE_NONE;
   LastCAPSyncTime = 0;
   NextFlushBytesReceived = DOWNLOAD_FLUSHFILE_SIZE;
}

Transfer::~Transfer() {
   TRACE();
   if (FileDescriptor != -1) {
      close(FileDescriptor);
      COUT(cout << "close: " << FileDescriptor << endl;)
   }
   delete [] Buffer;
   delete [] RollbackBuffer;
}

// Returns whatever upload/download return.
int Transfer::run(DCC_Container_t *DCC_Container) {
int retval = 0;

   TRACE();

   // First Thing is to save stuff in the DCC_Container so our private
   // functions have easy access to all the information.
   XGlobal = DCC_Container->XGlobal;
   Connection = DCC_Container->Connection;
   RemoteNick = DCC_Container->RemoteNick;
   RemoteDottedIP = DCC_Container->RemoteDottedIP;
   ResumePosition = DCC_Container->ResumePosition;
   // The FileName we get here is already prefixed with the directory
   FileName = DCC_Container->FileName;
   FileSize = DCC_Container->FileSize;

COUT(cout << "Transfer::run: RemoteNick: " << RemoteNick << " RemoteDottedIP: " << RemoteDottedIP << " ResumePosition: " << ResumePosition << " FileName: " << FileName << endl;)

   // Lets reset the BytesSent, BytesReceived track in the TCPConnect structure
   // So that we can check correctly against file size.
   Connection->BytesSent = 0;
   Connection->BytesReceived = 0;

   // Lets give control to the appropriate Send or Get routine.   
   switch (DCC_Container->TransferType) {
     case OUTGOING:
       updateCAPFromGlobals();
       Connection->monitorForUploadCap(true);
       retval = upload();
       break;

     case INCOMING:
       updateCAPFromGlobals();
       Connection->monitorForDwnldCap(true);
       retval = download();
       break;
   }

   // Close the File Descriptor after each run.
   if (FileDescriptor != -1) {
      close(FileDescriptor);
      COUT(cout << "close: " << FileDescriptor << endl;)
      FileDescriptor = -1;
   }

   return(retval);
}

// returns TRANSFER_UPLOAD_SUCCESS for a full complete upload
// returns TRANSFER_UPLOAD_FAILURE for a upload which didnt even start.
// returns TRANSFER_UPLOAD_PARTIAL for a upload which was partial.
// Once in two seconds update the correct FilesDetailList structure with the
//  correct Connection related info - bytes transfered/speed etc.
int Transfer::upload() {
off_t ret_off;
ssize_t ReadAmt;
ssize_t WriteAmt;
bool retvalb = true;
ssize_t retval;
Helper H;
int return_code;
size_t TransferBPS;

   TRACE();
   H.init(XGlobal);
   FileDescriptor = open(FileName, O_RDONLY | O_BINARY);
   COUT(cout << "open: " << FileName << " returned: " << FileDescriptor << endl;)
   if (FileDescriptor == -1) {
      sprintf(Buffer, "NOTICE %s :Error in trying to open file to send.",
              RemoteNick);
      H.sendLineToNick(RemoteNick, Buffer);
#ifdef DEBUG
      COUT(cout << "Transfer::upload() - FileName: " << FileName << endl;)
#endif
      return(TRANSFER_UPLOAD_FAILURE);
   }
   ret_off = lseek(FileDescriptor, ResumePosition, SEEK_SET);   
   if (ret_off == -1) {
      sprintf(Buffer, "NOTICE %s :Error in trying to seek file to send.",
              RemoteNick);
      H.sendLineToNick(RemoteNick, Buffer);
      return(TRANSFER_UPLOAD_FAILURE);
   }

#ifdef DEBUG
   COUT(cout << "upload: FileName: " << FileName << " FileSize: " << FileSize << endl;)
#endif

   // We are all set to upload.
   while (retvalb) {
      ReadAmt = read(FileDescriptor, Buffer, TRANSFER_SIZE);
      if (ReadAmt == -1) {
#ifdef DEBUG
         COUT(cout << "upload: read(filedes) returned -1" << endl;)
#endif
         sprintf(Buffer, "NOTICE %s :Error in reading from file to send.",
                 RemoteNick);
         H.sendLineToNick(RemoteNick, Buffer);
         retvalb = false;
         return_code = TRANSFER_UPLOAD_PARTIAL;
         continue;
      }
      if (ReadAmt == 0) {
#ifdef DEBUG
         COUT(cout << "upload: read(filedes) returned 0 = EOF" << endl;)
#endif
         // We are done sending the full file. (This is EOF reached).
         // sprintf(Buffer, "NOTICE %s :Send complete.", RemoteNick);
         // H.sendLineToNick(RemoteNick, Buffer);

         // Check if we have received Ack for all the bytes we have sent.
#ifdef DEBUG
COUT(cout << "Transfer: BytesSent: " << Connection->BytesSent << " Ack: " << DCCBytesAck << endl;)
#endif
         while (DCCBytesAck < (ResumePosition + Connection->BytesSent)) {
            COUT(cout << "Looping for last Ack" << endl;)
            // Get the remaining Acks
            if (readAckBytes(DCCSEND_TIMEOUT) == false) break;
         }

         // The above will wait a max of DCCSEND_TIMEOUT.
         // But usually will close as soon as the other side discos.
         // Bug here, it seems to be waiting indefintely in the while loop
         // in certain scenarios. Refer README.txt(search: I see an upload)

         // We need to remove this entry from SendsInProgress
         // XGlobal->SendsInProgress.delFilesOfNick(RemoteNick);
         // No need, its done by the TransferThr(), on our return.

         retvalb = false;
#ifdef DEBUG
COUT(cout << "Transfer:upload - all Done." << endl;)
#endif

         TransferBPS = Connection->getAvgUploadBps();
         sprintf(Buffer, "Server 09Upload: %s to %s Complete @%d Bps", FileName, RemoteNick, TransferBPS);
         XGlobal->IRC_ToUI.putLine(Buffer);
         // Update RecordUploadBPS
         if (Connection->BytesSent > MIN_BYTES_TRANSFERRED_FOR_RECORD) {
            XGlobal->lock();
            if (XGlobal->RecordUploadBPS < TransferBPS) {
               XGlobal->RecordUploadBPS = TransferBPS;
            }
            XGlobal->unlock();
         }

         // Write config files, so it doesnt have this completed upload.
         // We havent deleted it from SendsInProgress, so no point writing it
         // H.writeFServeConfig();
         return_code = TRANSFER_UPLOAD_SUCCESS;
         continue;
      }
      // Lets do the actual transfer.
      retval = Connection->writeData(Buffer, ReadAmt, DCCSEND_TIMEOUT);
#ifdef DEBUG
COUT(cout << "Transfer: got " << ReadAmt << " bytes from file. Wrote " << retval << " bytes to socket." << endl;)
#endif
      if (retval == -1) {
         TransferBPS = Connection->getAvgUploadBps();

         // Error writing to socket.
         // Its a real error if DisconnectMessage == CONNECTION_MESSAGE_NONE
         // else we have disconnected as instructed.
         if (DisconnectMessage != CONNECTION_MESSAGE_NONE) {
            // Not an error, we forced a disconnection.
            // Below we should actually change the string Imbalance Algorithm
            // as its not only that that discoes the send.
            sprintf(Buffer, "Server 11Imbalance Algorithm: Stopping the send of %s File: %s",  RemoteNick, getFileName(FileName));
            XGlobal->IRC_ToUI.putLine(Buffer);
         }
         else {
            sprintf(Buffer, "NOTICE %s :Error writing to Socket. DEBUG: errno: %d WriteAmt: %d", 
                    RemoteNick,
#ifdef __MINGW32__
                    WSAGetLastError(), 
#else
                    errno,
#endif
                    ReadAmt);
            H.sendLineToNick(RemoteNick, Buffer);
            sprintf(Buffer, "Server 04Upload: %s to %s @%d Bps - Error writing to Socket. DEBUG: errno: %d WriteAmt: %d", 
                    getFileName(FileName), RemoteNick, TransferBPS,
#ifdef __MINGW32__
                    WSAGetLastError(), 
#else
                    errno,
#endif
                    ReadAmt);
            XGlobal->IRC_ToUI.putLine(Buffer);
         }

         // Update RecordUploadBPS
         if (Connection->BytesSent > MIN_BYTES_TRANSFERRED_FOR_RECORD) {
            XGlobal->lock();
            if (XGlobal->RecordUploadBPS < TransferBPS) {
               XGlobal->RecordUploadBPS = TransferBPS;
            }
            XGlobal->unlock();
         }

         // We requeue this transfer.
         // only if its not CONNECTION_MESSAGE_DISCONNECT_NOREQUEUE
         if (DisconnectMessage != CONNECTION_MESSAGE_DISCONNECT_NOREQUEUE) {
            requeueTransfer();
         }
         retvalb = false;
         return_code = TRANSFER_UPLOAD_PARTIAL;
         continue;
      } else if (retval == 0) {
         // DCC Timeout trying to send.
         sprintf(Buffer, "NOTICE %s :DCC TimeOut trying to send. DEBUG: error: %d TimeOut: %lu ReadAmt: %d", RemoteNick,
#ifdef __MINGW32__
                 WSAGetLastError(), 
#else
                 errno,
#endif
                 DCCSEND_TIMEOUT, ReadAmt);
         H.sendLineToNick(RemoteNick, Buffer);
         retvalb = false;
         TransferBPS = Connection->getAvgUploadBps();
         sprintf(Buffer, "Server 04Upload: %s to %s @%d Bps - DCC Timeout. DEBUG: error: %d TimeOut: %lu ReadAmt: %d", getFileName(FileName), RemoteNick, TransferBPS, 
#ifdef __MINGW32__
                 WSAGetLastError(), 
#else
                 errno,
#endif
                 DCCSEND_TIMEOUT, ReadAmt);
         XGlobal->IRC_ToUI.putLine(Buffer);
         // Update RecordUploadBPS
         if (Connection->BytesSent > MIN_BYTES_TRANSFERRED_FOR_RECORD) {
            XGlobal->lock();
            if (XGlobal->RecordUploadBPS < TransferBPS) {
               XGlobal->RecordUploadBPS = TransferBPS;
            }
            XGlobal->unlock();
         }

         // we requeue this transfer.
         requeueTransfer();
         return_code = TRANSFER_UPLOAD_PARTIAL;
         continue;
      } else if (retval < ReadAmt) {
         ret_off = lseek(FileDescriptor, retval - ReadAmt, SEEK_CUR);
         if (ret_off == -1) {
            sprintf(Buffer, "NOTICE %s :Error in trying to seek file to send.",
                    RemoteNick);
            H.sendLineToNick(RemoteNick, Buffer);
            retvalb = false;
            return_code = TRANSFER_UPLOAD_PARTIAL;
            continue; 
         }
      }
      else {
         // Stuff has been written with no errors. Lets update the Connection
         // related stuff accordingly.
         updateFilesDetail(false);
         if (XGlobal->isIRC_QUIT()) {
            // requeue it so it lands in QueuesInProgress. On our return
            // TransferThr will remove this entry from SendsInProgress.
            // Will be lost on restart.
            requeueTransfer();
            retvalb = false;
            return_code = TRANSFER_UPLOAD_PARTIAL;
         }
      }
      readAckBytes(0); // Read the acks so that they return immediately.

      updateCAPFromGlobals(); // Sync CAP values from Global.
   }

   return(return_code);
}

// Return false if error on reading from socket.
// We are called with TimeOut = 0 -> transfer going on.
// or TimeOut = DCCSEND_TIMEOUT -> transfer complete, waiting for ack.
// When timeout = DCCSEND_TIMEOUT, and if we dont receive any data from
// readData(), we shoudl return false, so that the caller can break.
bool Transfer::readAckBytes(time_t TimeOut) {
int retval;
bool retvalb = true;

   TRACE();
   // Lets try to read the ACK data in the Read side of socket.
   retval = Connection->readData(Buffer, TRANSFER_SIZE, TimeOut);

   if (retval > 3) {
      memcpy(LastFourAckBytes, &Buffer[retval - 4], 4);
      DCCBytesAck = ntohl(* (unsigned long *) LastFourAckBytes);
#ifdef DEBUG
      COUT(cout << "DCCBytesAck: " << DCCBytesAck << endl;)
#endif
   }
   else {
      switch (retval) {
         case 0:
            // Do nothing.
            // If TimeOut = DCCSEND_TIMEOUT, return false.
            if (TimeOut == DCCSEND_TIMEOUT) {
               retvalb = false;
            }
            break;

         case 1:
            // Slide bytes 1 byte to the left.
            LastFourAckBytes[0] = LastFourAckBytes[1];
            LastFourAckBytes[1] = LastFourAckBytes[2];
            LastFourAckBytes[2] = LastFourAckBytes[3];
            LastFourAckBytes[3] = Buffer[0];
            break;

         case 2:
            // Slide bytes 2 bytes to the left.
            LastFourAckBytes[0] = LastFourAckBytes[2];
            LastFourAckBytes[1] = LastFourAckBytes[3];
            LastFourAckBytes[2] = Buffer[0];
            LastFourAckBytes[3] = Buffer[1];
            break;

         case 3:
            // Slide bytes 3 bytes to the left.
            LastFourAckBytes[0] = LastFourAckBytes[3];
            LastFourAckBytes[1] = Buffer[0];
            LastFourAckBytes[2] = Buffer[1];
            LastFourAckBytes[3] = Buffer[2];
            break;

         default:
            retvalb = false;
      }
      DCCBytesAck = ntohl(* (unsigned long *) LastFourAckBytes);

#ifdef DEBUG
      COUT(cout << "DCCBytesAck: " << DCCBytesAck << endl;)
#endif
   }
   return(retvalb);
}

// returns TRANSFER_DOWNLOAD_SUCCESS for a full complete download.
// returns TRANSFER_DOWNLOAD_FAILURE for a download which didnt even start.
// returns TRANSFER_DOWNLOAD_PARTIAL for a download which was partial.
// returns TRANSFER_DOWNLOAD_ROLLBACK_ERROR for a rollback terminated download.
int Transfer::download() {
off_t res_off;
ssize_t ReadAmt;
ssize_t WriteAmt;
ssize_t DeltaCmp;
bool retvalb = true;
ssize_t retval;
size_t TransferBPS;
int retcode = TRANSFER_DOWNLOAD_FAILURE;
Helper H;

   TRACE();

   H.init(XGlobal);

   FileDescriptor = open(FileName, O_RDWR | O_BINARY);
   COUT(cout << "open: " << FileName << " returned: " << FileDescriptor << endl;)
   if (FileDescriptor == -1) {
      // We possibly have to create this file only if ResumePosition is 0
      if (ResumePosition == 0) {
         FileDescriptor = open(FileName, O_RDWR | O_BINARY | O_CREAT, CREAT_PERMISSIONS);
         COUT(cout << "open: " << FileName << " returned: " << FileDescriptor << endl;)
      }
      if (FileDescriptor == -1) {
         sprintf(Buffer, "Server 04Download: From %s: Error creating file %s to write to local disk.", RemoteNick, FileName);
         XGlobal->IRC_ToUI.putLine(Buffer);
         return(TRANSFER_DOWNLOAD_FAILURE);
      }
      else {
         // Update MyPartialsFilesDB with this new file.
         Helper H;
         H.init(XGlobal);
         H.generateMyPartialFilesDB();
      }
   }

   res_off = lseek(FileDescriptor, ResumePosition, SEEK_SET);
   if (res_off == -1) {
      sprintf(Buffer, "Server 04Download: From %s: Error in seek", RemoteNick);
      XGlobal->IRC_ToUI.putLine(Buffer);
      return(TRANSFER_DOWNLOAD_FAILURE);
   }

   // Lets read FILE_RESUME_GAP bytes from this position, so we can compare
   // against incoming bytes for data validity.
   RollbackBuffer = new char[FILE_RESUME_GAP];
   RollbackLength = read(FileDescriptor, RollbackBuffer, FILE_RESUME_GAP);
   if (RollbackLength == -1) {
      sprintf(Buffer, "Server 04Download: File: %s From %s: Error in reading Rollback data from file", getFileName(FileName), RemoteNick);
      XGlobal->IRC_ToUI.putLine(Buffer);
      return(TRANSFER_DOWNLOAD_FAILURE);
   }
   RollbackCurrent = 0;

   // We are all set to download.
   while (retvalb) {
      ReadAmt = Connection->readData(Buffer, TRANSFER_SIZE, DCCSEND_TIMEOUT);
      if (ReadAmt == -1) {
         // Socket closed on the other end

         // Lets check if we are all done.
         if ( (FileSize - ResumePosition) <= Connection->BytesReceived) {
            // We are all done.
            // Lets move the file to the Serving directory - done in TransferThr
            retvalb = false;
            retcode = TRANSFER_DOWNLOAD_SUCCESS;
            continue;
         }

         TransferBPS = Connection->getAvgDownloadBps();
         sprintf(Buffer, "Server 04Download: File: %s From %s @%d Bps - Error in reading from Socket", getFileName(FileName), RemoteNick, TransferBPS);
         XGlobal->IRC_ToUI.putLine(Buffer);
         // Update RecordDownloadBPS
         if (Connection->BytesReceived > MIN_BYTES_TRANSFERRED_FOR_RECORD) {
            H.updateRecordDownloadAndAdjustUploadCaps(TransferBPS, false);
         }

         retvalb = false;
         retcode = TRANSFER_DOWNLOAD_PARTIAL;
         continue;
      }

      if (ReadAmt == 0) {
         // We have hit DCCSEND_TIMEOUT

         // Lets check if we are all done.
         if ( (FileSize - ResumePosition) <= Connection->BytesReceived) {
            // We are all done.
            // Lets move the file to the Serving directory - done in TransferThr
            retvalb = false;
            retcode = TRANSFER_DOWNLOAD_SUCCESS;
            continue;
         }

         TransferBPS = Connection->getAvgDownloadBps();
         sprintf(Buffer, "Server 04Download: File: %s From %s @%d Bps - DCC TimeOut", getFileName(FileName), RemoteNick, TransferBPS);
         XGlobal->IRC_ToUI.putLine(Buffer);
         // Update RecordDownloadBPS
         if (Connection->BytesReceived > MIN_BYTES_TRANSFERRED_FOR_RECORD) {
            H.updateRecordDownloadAndAdjustUploadCaps(TransferBPS, false);
         }
         retvalb = false;
         retcode = TRANSFER_DOWNLOAD_PARTIAL;
         continue;
      }

      // Lets first send out an Acknowledgement of what we have received.
      DCCBytesAck = htonl(Connection->BytesReceived + ResumePosition);
      retval = Connection->writeData((char *) &DCCBytesAck, 4, DCCSEND_TIMEOUT);
      if (retval != 4) {
         // Couldnt write out the Ack bytes.
         TransferBPS = Connection->getAvgDownloadBps();
         sprintf(Buffer, "Server 04Download: File: %s From %s @%d Bps - Error in sending out Ack", getFileName(FileName), RemoteNick, TransferBPS);
         XGlobal->IRC_ToUI.putLine(Buffer);
         // Update RecordDownloadBPS
         if (Connection->BytesReceived > MIN_BYTES_TRANSFERRED_FOR_RECORD) {
            H.updateRecordDownloadAndAdjustUploadCaps(TransferBPS, false);
         }
         retvalb = false;
         retcode = TRANSFER_DOWNLOAD_PARTIAL;
         continue;
      }

      // Check if we are still verifying the Rollback.
      if (RollbackCurrent != RollbackLength) {
      size_t write_extra = 0;
         // Lets check the data read against RollbackBuffer.
         if (ReadAmt <= (RollbackLength - RollbackCurrent) ) {
            DeltaCmp = ReadAmt;
         }
         else {
            DeltaCmp = RollbackLength - RollbackCurrent;
            // This case we need to write the extra data onto the file.
            write_extra = ReadAmt - DeltaCmp;
         }
         retval = memcmp(Buffer, &RollbackBuffer[RollbackCurrent], DeltaCmp);
         if (retval != 0) {
            sprintf(Buffer, "Server 4Download: File: %s From %s: Rollback detected mismatch in resume", getFileName(FileName), RemoteNick);
            XGlobal->IRC_ToUI.putLine(Buffer);

            // Send a NoResend and sleep 10 seconds.
            sprintf(Buffer, "PRIVMSG %s :\001NoReSend\001", RemoteNick);
            XGlobal->IRC_ToServerNow.putLine(Buffer);

            // Sleep 10 seconds before disconnecting. Assume message gets there.
            sleep(10);

            retvalb = false;
            retcode = TRANSFER_DOWNLOAD_ROLLBACK_ERROR;

            continue;
         }
         RollbackCurrent += DeltaCmp;

         // If we need to write out the extra bytes to file.
         if (write_extra) {
            retval = write(FileDescriptor, &Buffer[DeltaCmp], write_extra);
            if (retval != write_extra) {
               sprintf(Buffer, "Server 04Download: File: %s From %s: Rollback file write failed", getFileName(FileName), RemoteNick);
               XGlobal->IRC_ToUI.putLine(Buffer);
               retvalb = false;
               retcode = TRANSFER_DOWNLOAD_PARTIAL;
               continue;
            }
         }

         continue;
      }

      // Lets do the actual non Rollback related file writes.
      retval = write(FileDescriptor, Buffer, ReadAmt);
      if (retval != ReadAmt) {
         TransferBPS = Connection->getAvgDownloadBps();
         sprintf(Buffer, "Server 04Download: File: %s From %s @%d Bps - File write failed", getFileName(FileName), RemoteNick, TransferBPS);
         XGlobal->IRC_ToUI.putLine(Buffer);
         // Update RecordDownloadBPS
         if (Connection->BytesReceived > MIN_BYTES_TRANSFERRED_FOR_RECORD) {
            H.updateRecordDownloadAndAdjustUploadCaps(TransferBPS, false);
         }
         retvalb = false;
         retcode = TRANSFER_DOWNLOAD_PARTIAL;
         continue;
      }
      else {
         // Lets sync file if Connection->BytesReceived is an integral
         // of DOWNLOAD_FLUSHFILE_SIZE
         // This is so that if power goes off it will have at least this much.
         if (Connection->BytesReceived > NextFlushBytesReceived) {
            NextFlushBytesReceived += DOWNLOAD_FLUSHFILE_SIZE;
            fdatasync(FileDescriptor);
            COUT(cout << "Transfer::download() - fdatasync: BytesReceieved: " << Connection->BytesReceived << endl;)
         }

         // Lets update the appropriate FilesDetail with the Connection
         // information
         updateFilesDetail(true);
         if (XGlobal->isIRC_QUIT()) {
            // requeue it so it lands in QueuesInProgress. On our return
            // TransferThr will remove this entry from SendsInProgress.
            // Will be lost on restart.
            requeueTransfer();
            retvalb = false;
            retcode = TRANSFER_DOWNLOAD_PARTIAL;
         }
      }

      updateCAPFromGlobals(); // Sync CAP values from Global.

#if 0
      // Lets check if we are all done.
      if ( (FileSize - ResumePosition) == Connection->BytesReceived) {
         // We are all done.
         // Lets move the file to the Serving directory - done in TransferThr
         retvalb = false;
         retcode = TRANSFER_DOWNLOAD_SUCCESS;
         continue;
      }
#endif
   }
   return(retcode);
}

// Note that when we try to requeue, SendsInProgress still has this entry.
// And we are trying to add it to Small/QueuesInProgress as well.
// TransferThr, will ultimately remove it from SendsInProgress
void Transfer::requeueTransfer() {
FilesDetail *FD;
char *tmpfile;
int q_index;
bool SmallQ;
size_t MySmallFileSize;
Helper H;

   TRACE();

   H.init(XGlobal);

   // We requeue only if there is at least one person in Q. Else it will
   // keep sending endlessly. I guess its OK to send endlessly.
   // if (XGlobal->QueuesInProgress.getCount(NULL) == 0) return;

   tmpfile = getFileName(FileName);
   // We requeue it only if SendsInProgress, doesnt object.
   // RemoteNick can be outdated here as the nick might have changed.
   FD = XGlobal->SendsInProgress.getFilesDetailFileNameDottedIP(tmpfile, RemoteDottedIP);
   if (FD == NULL) {
      // Cant locate FD in SendsInProgress ! What happened ?
      COUT(cout << "Transfer:: Cant locate FD in SendsInProgress to requeue for Nick: " << RemoteNick << " File: " << tmpfile << endl;)
      return;
   }

   // We have a working FD here.
   if (FD->NoReSend == true) {
      XGlobal->SendsInProgress.freeFilesDetailList(FD);
      COUT(cout << "Transfer: Not requeueing " << RemoteNick << " as its marked as NOREQUEUE" << endl;)
      return;
   }

   // If RetryCount is greater than TRANSFER_MAX_RETRY do not requeue.
   if (FD->RetryCount > TRANSFER_MAX_RETRY) {
      XGlobal->SendsInProgress.freeFilesDetailList(FD);
      COUT(cout << "Transfer:: Not requeueing " << RemoteNick << " as its RetryCount is greater than " << TRANSFER_MAX_RETRY << endl;)
      return;
   }

   // Set SmallQ appropriately.
   // Below we update FileResumePosition we could possibly overshoot the
   // actual bytes client has received, as the send buffer is 64 K.
   // Punch in the values from DCC_Container for Resume and FileSize
   FD->FileResumePosition = ResumePosition + FD->BytesSent - 65536;
   FD->FileSize = FileSize;
   if (FD->FileResumePosition < 0) FD->FileResumePosition = 0;

   XGlobal->lock();
   MySmallFileSize = XGlobal->FServSmallFileSize;
   XGlobal->unlock();
   if ( (FileSize <= MySmallFileSize) ||
        (FileSize <= (ResumePosition + MySmallFileSize)) ) {
      SmallQ = true;
   }
   else {
      SmallQ = false;
   }

   // Lets get rid of the Connection entry in this FD.
   // Clear out the ConnectionMessage
   FD->ConnectionMessage = '\0';
   FD->Connection = NULL;
   FD->BytesSent = 0;
   FD->BytesReceived = 0;
   FD->Born = 0;
   FD->UploadBps = 0;
   FD->DownloadBps = 0;

   // Just put him up as the second entry. (we already verified there is entry)
   // Its OK if there is no entry, it can start sending again.
   // Put in Q 1 for DCC Send files => Manual Send = 'D'
   // if (XGlobal->QueuesInProgress.getCount(NULL) >= 1) {
   // Increase the retry count each time we requeue.
   // Increase it only if this was not disconnected my a Message to disconnect.

   // Increase it always, as the fservGet() will skip this entry. else there
   // is a chance that this nick is regular, and hence fservGet() might put
   // a voiced user ahead of this guy.
   // Hence for that case, we increase it only if its 0.
   if (DisconnectMessage == CONNECTION_MESSAGE_DISCONNECT_REQUEUE_NOINCRETRY) {
      if (FD->RetryCount == 0) {
         FD->RetryCount++;
      }
   }
   else {
      FD->RetryCount++;
   }

   if ( (FD->ManualSend == MANUALSEND_DCCSEND) ||
        (DisconnectMessage != CONNECTION_MESSAGE_NONE) ) {
      q_index = 1;
   }
   else {
      // Note that in the event there is no entry, adding at index 2
      // will basically add it at index 1, which is what we want too.
      q_index = 2;
   }
   COUT(cout << "Transfer::requeueTransfer: FD: ";)
   COUT(XGlobal->QueuesInProgress.printDebug(FD);)

   // Send a message to notify client of current q status.
   // when we have disconnected this transfer ourselves to be requeued.
   if (DisconnectMessage != CONNECTION_MESSAGE_NONE) {
      char q_str[6];
      if (SmallQ) {
         strcpy(q_str, "Small");
      }
      else {
         strcpy(q_str, "Big");
      }
      sprintf(Buffer, "NOTICE %s :Imbalance Algorithm. Requeued your file in %s Queue at index: %d File: %s",
              FD->Nick, q_str, q_index, FD->FileName);
      H.sendLineToNick(FD->Nick, Buffer);
      COUT(cout << "Transfer::requeueTransfer: " << Buffer << endl;)
   }

   if (SmallQ) {
      XGlobal->SmallQueuesInProgress.addFilesDetailAtIndex(FD, q_index);
   }
   else {
      XGlobal->QueuesInProgress.addFilesDetailAtIndex(FD, q_index);
   }

}

// This assumes that FileName, Connection are valid.
// This is used to periodically update the correct FilesDetail with
// BytesSent, BytesReceived, Born, UploadBPS, DownloadBps
// It does these updates only once in two seconds, no matter how frequently
// its called.
// dwnld = true => update entry in DwnldInProgress (FilesDetailList)
// dwnld = false => update entry in SendsInProgress (FilesDetailList)
// As its accessing FilesDetail, it monitors ConnectionMessage, and does
// what is requested. Usually a disconnect is asked for.
// This is so that we can remove Connection from FilesDetail, and avoid
// the problem of other threads disconnecting when the TCPConnect class
// is in the middle of doing something, and then accessing Sock in a select
// which is invalid, causing crashes.
void Transfer::updateFilesDetail(bool dwnld) {
time_t CurrentTime = time(NULL);
char ConnectionMessage;
FilesDetail *FD = NULL;
char *transfer_nick;
char *tmpstr;

   TRACE();

   if (CurrentTime > LastUpdateFilesDetailTime) {
      LastUpdateFilesDetailTime = CurrentTime;

      // We really need to update the Connection values.
      tmpstr = getFileName(FileName);

#if 0
      COUT(cout << "Transfer::updateFilesDetail updating Connection information in FilesDetailList. dwnld = " << dwnld << " nick: " << RemoteNick << " file: " << getFileName(FileName) << " bytes_sent: " << Connection->BytesSent << " bytes_recvd: " << Connection->BytesReceived << " born: " << Connection->Born << " upload_bps: " << Connection->getUploadBps() << " dwnld_bps: " << Connection->getDownloadBps() << endl;)
#endif
      if (dwnld) {
         // Lets get the right Nick Name, as RemoteNick could have changed.
         FD = XGlobal->DwnldInProgress.getFilesDetailFileNameDottedIP(tmpstr, RemoteDottedIP);
         if (FD && FD->Nick) {
            transfer_nick = FD->Nick;
         }
         else {
            transfer_nick = RemoteNick;
         }
          
         ConnectionMessage = XGlobal->DwnldInProgress.updateFilesDetailNickFileConnectionDetails(transfer_nick, tmpstr, Connection->BytesSent, Connection->BytesReceived, Connection->Born, Connection->getUploadBps(), Connection->getDownloadBps());
         if (ConnectionMessage != CONNECTION_MESSAGE_NONE) {
            Connection->disConnect();
            DisconnectMessage = ConnectionMessage;
         }
      }
      else {
         // Lets get the right Nick Name, as RemoteNick could have changed.
         FD = XGlobal->SendsInProgress.getFilesDetailFileNameDottedIP(tmpstr, RemoteDottedIP);
         if (FD && FD->Nick) {
            transfer_nick = FD->Nick;
         }
         else {
            transfer_nick = RemoteNick;
         }
         ConnectionMessage = XGlobal->SendsInProgress.updateFilesDetailNickFileConnectionDetails(transfer_nick, tmpstr, Connection->BytesSent, Connection->BytesReceived, Connection->Born, Connection->getUploadBps(), Connection->getDownloadBps());
         if (ConnectionMessage != CONNECTION_MESSAGE_NONE) {
            Connection->disConnect();
            DisconnectMessage = ConnectionMessage;
         }
      }

      // Lets free up the FD obtained.
      XGlobal->SendsInProgress.freeFilesDetailList(FD);
   }
}

// Sync CAP values from Global.
// Actually syncs once in 5 seconds.
void Transfer::updateCAPFromGlobals() {
time_t CurrentTime;

   TRACE();

   CurrentTime = time(NULL);
   if (CurrentTime > (LastCAPSyncTime + 5) ) {
      LastCAPSyncTime = CurrentTime;
      XGlobal->lock();
      Connection->setMaxUploadBps(XGlobal->PerTransferMaxUploadBPS);
      Connection->setOverallMaxUploadBps(XGlobal->OverallMaxUploadBPS);
      Connection->setMaxDwnldBps(XGlobal->PerTransferMaxDownloadBPS);
      Connection->setOverallMaxDwnldBps(XGlobal->OverallMaxDownloadBPS);
      XGlobal->unlock();
   }
}
