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

#include "Compatibility.hpp"

#include "SwarmStream.hpp"
#include "StackTrace.hpp"
#include "Utilities.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


// Constructor.
SwarmStream::SwarmStream() {

   TRACE();

   Directory = NULL;
   FileName = NULL;
   FileSize = 0;
   MaxKnownFileSize = 0;
   FileDescriptor = -1;
   FileSHA[0] = '\0';
   AmServer = false;
   IsBeingUsed = false;
   NextFlushFileSize = DOWNLOAD_FLUSHFILE_SIZE;

   UIEntries = NULL;
}

// Destructor.
SwarmStream::~SwarmStream() {

   TRACE();

   delete [] Directory;
   delete [] FileName;

   if (UIEntries) {
      delete [] UIEntries[0];
      delete [] UIEntries;
   }
}

bool SwarmStream::setSwarmServer(char *Dir, char *File) {
char *fullpath;

   TRACE();

   delete [] Directory;
   Directory = new char[strlen(Dir) + 1];
   strcpy(Directory, Dir);

   delete [] FileName;
   FileName = new char[strlen(File) + 1];
   strcpy(FileName, File);

   fullpath = new char[strlen(Dir) + strlen(File) + strlen(DIR_SEP) + 1];
   sprintf(fullpath, "%s%s%s", Dir, DIR_SEP, File);

   getFileSize(fullpath, &FileSize);
   MaxKnownFileSize = FileSize;
   FileDescriptor = open(fullpath, O_RDWR | O_BINARY);
   delete [] fullpath;

   if (FileDescriptor == -1) {
      IsBeingUsed = false;
      return(false);
   }

   ConnectedNodes.setFileDescriptor(FileDescriptor);

   getSHAOfSwarmFile(Dir, File, FileSize, FileSHA);

   AmServer = true;
   IsBeingUsed = true;

   NextFlushFileSize = FileSize + DOWNLOAD_FLUSHFILE_SIZE;

   return(true);
}

// We additionally make sure that the file exists.
// So create it if it doesnt exist.
bool SwarmStream::setSwarmClient(char *Dir, char *File) {
char *fullpath;
bool retvalb;

   TRACE();

   delete [] Directory;
   Directory = new char[strlen(Dir) + 1];
   strcpy(Directory, Dir);

   delete [] FileName;
   FileName = new char[strlen(File) + 1];
   strcpy(FileName, File);

   fullpath = new char[strlen(Dir) + strlen(File) + strlen(DIR_SEP) + 1];
   sprintf(fullpath, "%s%s%s", Dir, DIR_SEP, File);

   retvalb = getFileSize(fullpath, &FileSize);
   MaxKnownFileSize = FileSize;
   if (retvalb == false) {
      // Lets create the file.
      int fd = open(fullpath, O_RDWR | O_BINARY | O_CREAT, CREAT_PERMISSIONS);
      close(fd);

      retvalb = getFileSize(fullpath, &FileSize);
   }
   FileDescriptor = open(fullpath, O_RDWR | O_BINARY);
   delete [] fullpath;

   ConnectedNodes.setFileDescriptor(FileDescriptor);

   if (FileDescriptor == -1) {
      IsBeingUsed = false;
      return(false);
   }

   getSHAOfSwarmFile(Dir, File, FileSize, FileSHA);

   AmServer = false;
   IsBeingUsed = true;

   NextFlushFileSize = FileSize + DOWNLOAD_FLUSHFILE_SIZE;

   return(true);
}

const char *SwarmStream::getSwarmFileName() {
   TRACE();

   return(FileName);
}

size_t SwarmStream::getSwarmFileSize() {
   TRACE();

   return(FileSize);
}

bool SwarmStream::isSwarmServer() {

   TRACE();

   return(AmServer);
}

bool SwarmStream::isBeingUsed() {
   TRACE();

   return(IsBeingUsed);
}

bool SwarmStream::isFileBeingSwarmed(const char *f_name) {
bool retvalb = false;

   TRACE();

   do {
      if ( (FileName == NULL) || (f_name == NULL) ) break;

      if (strcasecmp(f_name, FileName)) break;

      retvalb = true;
   } while (false);

   return(retvalb);
}

bool SwarmStream::generateStringHS(char *HSBuffer) {

   TRACE();

   if (FileSHA[0] == '\0') {
      getSHAOfSwarmFile(Directory, FileName, FileSize, FileSHA);
   }

   sprintf(HSBuffer, "HS %lx %s %s\n",
                      FileSize,
                      FileSHA,
                      FileName);
   COUT(cout << "SwarmStream::generateStringHS: " << HSBuffer;)
   return(true);
}

bool SwarmStream::isMatchingSHA(size_t file_size, char *file_size_sha) {
bool retvalb = false;

   TRACE();

   do {
      if (FileSize == file_size) {
         if (strcasecmp(file_size_sha, FileSHA) == 0) retvalb = true;
         break;
      }

      // We need to generate the sha for file_size.
      char file_sha[64];
      getSHAOfSwarmFile(Directory, FileName, file_size, file_sha);

      COUT(cout << "SwarmStream::isMatchingSHA matching his SHA: " << file_size_sha << " with our sha: " << file_sha << endl;)
      if (strcasecmp(file_size_sha, file_sha) == 0) retvalb = true;

   } while (false);

   return(retvalb);
}

// Returns the array of Strings which represent the Swarm in a nice
// UI format. The first entry is the Header, and ends with a NULL string.
char **SwarmStream::getSwarmUIEntries() {
char **NodeUIEntry1; // For Connected.
char **NodeUIEntry2; // For Yet To Try.
char **NodeUIEntry3; // For Tried And Failed.
size_t TotalUploadBPS, TotalDownloadBPS, junk;
char s_upload_bps[32], s_download_bps[32];
int entries, totalentries, index;

   TRACE();

   NodeUIEntry1 = ConnectedNodes.getNodesUIEntries(&TotalUploadBPS, &TotalDownloadBPS);
   entries = getEntriesInStringArray(NodeUIEntry1);
   totalentries = entries;

   NodeUIEntry2 = YetToTryNodes.getNodesUIEntries(&junk, &junk);
   entries = getEntriesInStringArray(NodeUIEntry2);
   totalentries += entries;

   NodeUIEntry3 = TriedAndFailedNodes.getNodesUIEntries(&junk, &junk);
   entries = getEntriesInStringArray(NodeUIEntry3);
   totalentries += entries;

   // We have got all the entries.

   // We cant call deleteStreamArray for UIEntries here, as we own only
   // first entry and the array. Rest are allocated in SwarmNodeList
   if (UIEntries) {
      delete [] UIEntries[0];
      delete [] UIEntries;

      UIEntries = NULL;
   }

   totalentries += 2; // One for our Header, One NULL.
   UIEntries = new char *[totalentries];
   //COUT(cout << "SwarmStream: UIEntries allocated: " << UIEntries << " totalentries: " << totalentries << endl;)

   // Lets put the header.
   // FileName\tFileSize\tRateUP\tRateDown\tProgress\t---
   convertFileSizeToString(TotalUploadBPS, s_upload_bps);
   strcat(s_upload_bps, "/s");
   convertFileSizeToString(TotalDownloadBPS, s_download_bps);
   strcat(s_download_bps, "/s");

   // To get the progress % we need to know the greatest FileSize
   // available in the Swarm.
   size_t max_file_size = ConnectedNodes.getGreatestFileSize();
   if (MaxKnownFileSize < max_file_size) {
      MaxKnownFileSize = max_file_size;
   }
   else {
      max_file_size = MaxKnownFileSize;
   }
   float f_max_fs = max_file_size;
   float f_cur_fs = FileSize;
   float f_progress_percent;
   if (max_file_size < FileSize) {
      f_max_fs = FileSize;
      f_cur_fs = FileSize;
   }

   if (f_max_fs < 1.0) f_progress_percent = 100.0;
   else f_progress_percent = (f_cur_fs / f_max_fs) * 100.0;

   UIEntries[0] = new char[strlen(FileName) + 32 + 32 + 32 + 32];
   sprintf(UIEntries[0], "%s\t%lu\t%s\t%s\t%05.2f %c\t%lu",
                         FileName, 
                         FileSize,
                         s_upload_bps,
                         s_download_bps,
                         f_progress_percent, '%',
                         MaxKnownFileSize);

   // Lets put in the entries from NodeUIEntry1
   index = 0;
   entries = 1;
   while (NodeUIEntry1 && NodeUIEntry1[index] && (entries < totalentries) ) {
      UIEntries[entries] = NodeUIEntry1[index];
      entries++;
      index++;
   }

   // Lets put in the entries from NodeUIEntry2
   index = 0;
   while (NodeUIEntry2 && NodeUIEntry2[index] && (entries < totalentries) ) {
      UIEntries[entries] = NodeUIEntry2[index];
      entries++;
      index++;
   }

   // Lets put in the entries from NodeUIEntry2
   index = 0;
   while (NodeUIEntry3 && NodeUIEntry3[index] && (entries < totalentries) ) {
      UIEntries[entries] = NodeUIEntry3[index];
      entries++;
      index++;
   }

   if (entries >= totalentries) abort();

   UIEntries[entries] = NULL;

   return(UIEntries);
}

// To quit from the Swarm.
// Returns true if the file if full downloaded.
bool SwarmStream::quitSwarm() {
bool retvalb = false;

   TRACE();

   // Lets see if we need to move the File from Partial to Serving.
   // We need to move in the below case:
   // - ConnectedNodes.isDownloadComplete() == true
   if (ConnectedNodes.isDownloadComplete()) {
      retvalb = true;
   }

   // We need to clean out ConnectedNodes, YetToTryNodes and 
   // TriedAndFailedNodes
   ConnectedNodes.freeAndInitSwarmNodeList();
   YetToTryNodes.freeAndInitSwarmNodeList();
   TriedAndFailedNodes.freeAndInitSwarmNodeList();

   delete [] Directory;
   Directory = NULL;
   delete [] FileName;
   FileName = NULL;

   if (UIEntries) {
      delete [] UIEntries[0];
      delete [] UIEntries;
   }
   UIEntries = NULL;

   FileSize = 0;
   MaxKnownFileSize = 0;
   FileSHA[0] = '\0';
   AmServer = false;
   IsBeingUsed = false;

   if (FileDescriptor != -1) {
      close(FileDescriptor);
      FileDescriptor = -1;
      ConnectedNodes.setFileDescriptor(FileDescriptor);
   }
   // We have cleaned out and initialized ourselves.

   return(retvalb);
}

bool SwarmStream::renameNickToNewNick(char *old_nick, char *new_nick) {

   TRACE();

   ConnectedNodes.renameNickToNewNick(old_nick, new_nick);
   YetToTryNodes.renameNickToNewNick(old_nick, new_nick);
   TriedAndFailedNodes.renameNickToNewNick(old_nick, new_nick);

   return(true);
}

// Read Messages from the Connections and update state.
bool SwarmStream::readMessagesAndUpdateState() {
bool retvalb;
size_t OrigFileSize;

   TRACE();

   //COUT(cout << "SwarmStream::readMessagesAndUpdateState: Before call FileSize: " << FileSize << endl;)

   OrigFileSize = FileSize;
   retvalb = ConnectedNodes.readMessagesAndUpdateState(FileName, &FileSize);
   if (OrigFileSize != FileSize) {
      // FileSize Changed, invalidate the FileSHA.
      FileSHA[0] = '\0';
   }

   if (FileSize > NextFlushFileSize) {
      NextFlushFileSize = FileSize + DOWNLOAD_FLUSHFILE_SIZE;
      fdatasync(FileDescriptor);
      COUT(cout << "SwarmStream - fdatasync: FileSize: " << FileSize << endl;)
   }

   //COUT(cout << "SwarmStream::readMessagesAndUpdateState: After call FileSize: " << FileSize << endl;)

   return(retvalb);
}

// Write Messages to the Connections and update state.
bool SwarmStream::writeMessagesAndUpdateState() {
bool retvalb;

   TRACE();

   retvalb = ConnectedNodes.writeMessagesAndUpdateState(FileName, FileSize);

   return(retvalb);
}

// Check for Connections which are dead.
bool SwarmStream::checkForDeadNodes() {
bool retvalb;

   TRACE();

   retvalb = ConnectedNodes.checkForDeadNodes(FileName, FileSize);

   return(retvalb);
}

// Check for EndGame and do the necessary to get the Last Piece, if at all.
bool SwarmStream::checkForEndGame() {
bool retvalb;

   TRACE();

   retvalb = ConnectedNodes.checkForEndGame(FileName, FileSize);

   return(retvalb);
}

// To update the CAP values from Global.
// This call is called once in 5 seconds from SwarmThr.
bool SwarmStream::updateCAPFromGlobal(size_t PerTransferMaxUploadBPS, size_t OverallMaxUploadBPS, size_t PerTransferMaxDownloadBPS, size_t OverallMaxDownloadBPS) {
bool retvalb;

   TRACE();

   retvalb = ConnectedNodes.updateCAPFromGlobal(PerTransferMaxUploadBPS, OverallMaxUploadBPS, PerTransferMaxDownloadBPS, OverallMaxDownloadBPS);

   return(retvalb);
}

// To return the Error Code if Swarm met with a critical error
int SwarmStream::getErrorCode() {
int retval;

   TRACE();
   
   retval = ConnectedNodes.getErrorCode();

   return(retval);
}

// To return the Error Code as a string.
void SwarmStream::getErrorCodeString(char *ErrorString) {
   TRACE();

   ConnectedNodes.getErrorCodeString(ErrorString);
}

// Returns true if I hold the Max FileSize in the Swarm.
bool SwarmStream::haveGreatestFileSizeInSwarm() {

   TRACE();

   if (FileSize >= ConnectedNodes.getGreatestFileSize()) {
      return(true);
   }
   else {
      return(false);
   }
}
