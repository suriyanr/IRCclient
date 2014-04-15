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

#ifndef CLASS_TRANSFER
#define CLASS_TRANSFER

#include "ThreadMain.hpp"

// Return codes for Transfer::download()
#define TRANSFER_DOWNLOAD_SUCCESS 0
#define TRANSFER_DOWNLOAD_FAILURE 1
#define TRANSFER_DOWNLOAD_PARTIAL 2
#define TRANSFER_DOWNLOAD_ROLLBACK_ERROR 3

// Return codes for Transfer::upload()
#define TRANSFER_UPLOAD_SUCCESS 0
#define TRANSFER_UPLOAD_FAILURE 1
#define TRANSFER_UPLOAD_PARTIAL 2
#define TRANSFER_UPLOAD_REQUEUE 3

#define TXSIZE 1460 // max ethernet size tcp payload
// #define TXSIZE 8192
#define TRANSFER_SIZE TXSIZE

// We do not requeue if RetryCount is more than this value
#define TRANSFER_MAX_RETRY     20
// This Class handles the actual Transfer
class Transfer {
public:
   Transfer();
   ~Transfer();
   int run(DCC_Container_t *);

private:
   int upload();
   int download();
   bool readAckBytes(time_t TimeOut);

   void requeueTransfer();
   void updateFilesDetail(bool download);
   void updateCAPFromGlobals(); // Sync CAP values from Global.

   char *Buffer; // This is TRANSFER_SIZE

   int FileDescriptor;
   char LastFourAckBytes[4];
   unsigned long DCCBytesAck;
   time_t LastUpdateFilesDetailTime;

   // Last time we synced CAP from Global.
   time_t LastCAPSyncTime;

   // We flush data to dick after we have got so many bytes.
   size_t NextFlushBytesReceived;

   // This is when we disconnect based on the message we receive.
   char DisconnectMessage;

   // Below is used only by downloads for data integrity, for resume.
   char *RollbackBuffer; // This is FILE_RESUME_GAP
   size_t RollbackLength;
   size_t RollbackCurrent;

   // Below are saved by run() from what it receives in DCC_Container.
   // The Destructor should not destroy these.
   XChange *XGlobal;
   TCPConnect *Connection;
   char *RemoteNick;
   char *RemoteDottedIP;
   char *FileName;
   int  ServingDirIndex;
   off_t ResumePosition;
   size_t FileSize;
};

#endif
