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

#include <string.h>

#ifdef __MINGW32__
#  include <windows.h>
#else
#  include <pthread.h>
#  include <semaphore.h>
#endif

#include "FilesDetailList.hpp"
#include "SHA1.hpp"
#include "TCPConnect.hpp"
#include "StackTrace.hpp"
#include "LineParse.hpp"

#include "Compatibility.hpp"

// Constructor.
FilesDetailList::FilesDetailList() {
   TRACE();
   Head = NULL;
   setTimeOut((time_t) FILES_DETAIL_LIST_PRUNE);
   LastPruneTime = 0;
   setSort(FILELIST_NO_SORT);

#ifdef __MINGW32__
   FilesDetailListMutex = CreateMutex(NULL, FALSE, NULL);
#else
   pthread_mutex_init(&FilesDetailListMutex, NULL);
#endif
}

// Destructor.
FilesDetailList::~FilesDetailList() {
   TRACE();
   freeFilesDetailList(Head);
   DestroyMutex(FilesDetailListMutex);
// Possible core dump if someone is using that data and we happily 
// go out of scope.
}

void FilesDetailList::setTimeOut(time_t t) {
   TRACE();
   TimeOut = t;
}

void FilesDetailList::setSort(char s) {
   TRACE();
   Sort = s;
}

// To initiliaise a newly alloced FilesDetail structure.
void FilesDetailList::initFilesDetail(FilesDetail *FD) {

   TRACE();

   if (FD) {
      memset(FD, 0, sizeof(FilesDetail));
   }
}

// Compare function to compare two FDs passed in.
// The compare criteria can be one of
//   #define FD_COMPARE_FILENAME   0
//   #define FD_COMPARE_FILESIZE   1
//   #define FD_COMPARE_NICKNAME   2
//   #define FD_COMPARE_DIRNAME    3
//   #define FD_COMPARE_SENDS      4
//   #define FD_COMPARE_QUEUES     5
//   #define FD_COMPARE_CLIENT     6
// descending = true => reverse the comparison answers
int FilesDetailList::compareFilesDetail(int criteria, bool descending, FilesDetail *a, FilesDetail *b) {
int retval = 0;
char *DirName1;
char *DirName2;
char EmptyDir[2];

   TRACE();  // Remove later for optimisation.

   if ( (a == NULL) || (b == NULL) ) return(retval);

   switch (criteria) {
      case FD_COMPARE_CLIENT:
      if (a->ClientType < b->ClientType) {
         retval = -1;
      }
      else if (a->ClientType > b->ClientType) {
         retval = 1;
      }
      break;

      case FD_COMPARE_QUEUES:
      if (a->CurrentQueues < b->CurrentQueues) {
         retval = -1;
      }
      else if (a->CurrentQueues > b->CurrentQueues) {
         retval = 1;
      }
      break;

      case FD_COMPARE_SENDS:
      if (a->CurrentSends < b->CurrentSends) {
         retval = -1;
      }
      else if (a->CurrentSends > b->CurrentSends) {
         retval = 1;
      }
      break;

      case FD_COMPARE_FILESIZE:
      if (a->FileSize < b->FileSize) {
         retval = -1;
      }
      else if (a->FileSize >  b->FileSize) {
         retval = 1;
      }
      break;

      case FD_COMPARE_DIRNAME:
      strcpy(EmptyDir, " ");
      if (a->DirName == NULL) {
         DirName1 = EmptyDir;
      }
      else {
         DirName1 = a->DirName;
      }
      if (b->DirName == NULL) {
         DirName2 = EmptyDir;
      }
      else {
         DirName2 = b->DirName;
      }
      // If both are empty DirNames, then we return partial file < serving file
      // So that in listing, Partial files appear earlier.
      retval = strcasecmp(DirName1, DirName2);
      if (retval == 0) {
         retval = a->DownloadState - b->DownloadState;
      }
      break;

      case FD_COMPARE_NICKNAME:
      if ( (a->Nick == NULL) || (b->Nick == NULL) ) {
         break;
      }
      retval = strcasecmp(a->Nick, b->Nick);
      break;

      case FD_COMPARE_FILENAME:
      default:
      if ( (a->FileName == NULL) || (b->FileName == NULL) ) {
         break;
      }
      retval = strcasecmp(a->FileName, b->FileName);
      break;
   }

   // Here we reverse the answer is descending is true.
   if (descending) {
      retval = 0 - retval;
   }
   return(retval);
}


// Not Mutex protected.
void FilesDetailList::freeFilesDetailList(FilesDetail *Start) {
FilesDetail *Scan;

   TRACE();
   while (Start != NULL) {
      Scan = Start->Next;
      delete [] Start->FileName;
      delete [] Start->DottedIP;
      delete [] Start->Nick;
      delete [] Start->PropagatedNick;
      delete [] Start->TriggerName;
      delete [] Start->DirName;

      // Start->Data is not our responsibility to free.

      delete Start;
      Start = Scan;
   }
}

// Delete the whole entire list.
void FilesDetailList::purge() {
FilesDetail *ScanFD;

   TRACE();

   WaitForMutex(FilesDetailListMutex);
   ScanFD = Head;
   Head = NULL;
   ReleaseMutex(FilesDetailListMutex);

   freeFilesDetailList(ScanFD);
}

// Returns the count and sum of FileSize of all entries in the list.
// If NULL passed, it gets count and size using the Head.
// "TriggerTemplate" entries for FileNames are not counted.
int FilesDetailList::getCountFileSize(FilesDetail *FD, double *TotalFileSize) {
FilesDetail *ScanFD;
int count = 0;

   TRACE();
   pruneOldFiles(false);

   *TotalFileSize = 0.0;

   WaitForMutex(FilesDetailListMutex);

   if (FD == NULL) {
      ScanFD = Head;
   }
   else {
      ScanFD = FD;
   }

   while (ScanFD) {
      if ( (ScanFD->FileName) &&
           (strcasecmp(ScanFD->FileName, "TriggerTemplate"))
         ) {
         count++;
         *TotalFileSize = *TotalFileSize + (double) ScanFD->FileSize;
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

   return(count);
}

// Return the count of the List. If NULL, get count of the internal list.
int FilesDetailList::getCount(FilesDetail *FD) {
FilesDetail *ScanFD;
int count = 0;

   TRACE();
   pruneOldFiles(false);

   WaitForMutex(FilesDetailListMutex);

   if (FD == NULL) {
      ScanFD = Head;
   }
   else {
      ScanFD = FD;
   }

   while (ScanFD) {
      count++;
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

   return(count);
}

// Returns the count of the FilesDetail in the List.
// If NULL passed, it gets count using the Head.
// This gets the count of the entries held by varnick, "No Files Present"
// entries for FileNames are not counted. TriggerTemplate is counted.
int FilesDetailList::getFileCountOfNick(FilesDetail *FD, char *varnick) {
FilesDetail *ScanFD;
int count = 0;

   TRACE();
   pruneOldFiles(false);

   if (varnick == NULL) return(count);

   WaitForMutex(FilesDetailListMutex);

   if (FD == NULL) {
      ScanFD = Head;
   }
   else {
      ScanFD = FD;
   }

   while (ScanFD) {
      if (ScanFD->Nick && (strcasecmp(ScanFD->Nick, varnick) == 0) ) {
      // This is the nick we were looking for.
         if (ScanFD->FileName && strcasecmp(ScanFD->FileName, "No Files Present")) {
            // Count only if FileName exists and is not "No Files Present"
            count++;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

   return(count);
}

// To selectively delete just the Serving / Partial filelist of nick.
// Used by FFLC
void FilesDetailList::delFilesOfNickByDownloadState(char *nick, char dstate) {
FilesDetail *ScanFD;

   TRACE();
   if (nick == NULL) return;

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if ( (strcasecmp(ScanFD->Nick, nick) == 0) &&
           (ScanFD->DownloadState == dstate) ) {
         ScanFD->Time = 0;
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

   // Call prune so that it removes Time Entries with 0
   pruneOldFiles(true);
}


// Delete Files belonging to nick
// Used when nick parts/quits/kicked from channel.
void FilesDetailList::delFilesOfNick(char *nick) {
FilesDetail *ScanFD;

   TRACE();
   if (nick == NULL) return;

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if (strcasecmp(ScanFD->Nick, nick) == 0) {
         ScanFD->Time = 0;
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

   // Call prune so that it removes Time Entries with 0
   pruneOldFiles(true);
}

// del FilesDetail from the list which match DottedIP.
void FilesDetailList::delFilesDetailDottedIP(char *dottedip) {
FilesDetail *ScanFD;

   TRACE();
   if ( (dottedip == NULL) || (strlen(dottedip) == 0)
      ) {
      return;
   }

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if (ScanFD->DottedIP) {
         if (strcasecmp(ScanFD->DottedIP, dottedip) == 0) {
            ScanFD->Time = 0;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

   // Call prune so that it removes Time Entries with 0
   pruneOldFiles(true);
}

// del FilesList from the List which match FileName and DottedIP.
void FilesDetailList::delFilesDetailFileNameDottedIP(char *filename, char *dottedip) {
FilesDetail *ScanFD;

   TRACE();
   if ( (filename == NULL) || (strlen(filename) == 0) ||
        (dottedip == NULL) || (strlen(dottedip) == 0)
      ) {
      return;
   }

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if ( (ScanFD->FileName) && (ScanFD->DottedIP) ) {
         if ( (strcasecmp(ScanFD->FileName, filename) == 0) &&
              (strcasecmp(ScanFD->DottedIP, dottedip) == 0) ) {
            ScanFD->Time = 0;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

   // Call prune so that it removes Time Entries with 0
   pruneOldFiles(true);
}

// del Files from the List which match FileName and Nick.
void FilesDetailList::delFilesDetailNickFile(char *nick, char *file) {
FilesDetail *ScanFD;

   TRACE();
   if ( (nick == NULL) || (file == NULL) ) return;

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if ( (ScanFD->Nick) && (ScanFD->FileName) ) {
         if ( (strcasecmp(ScanFD->Nick, nick) == 0) &&
              (strcasecmp(ScanFD->FileName, file) == 0) ) {
            ScanFD->Time = 0;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

   // Call prune so that it removes Time Entries with 0
   pruneOldFiles(true);
}

// update FilesList from the List which match Nick with NoReSend.
// Mark all entries of Nick as NoReSend
void FilesDetailList::updateFilesDetailNickNoReSend(char *nick, bool noresend) {
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();
   if (nick == NULL) return;

   WaitForMutex(FilesDetailListMutex);
   ScanFD = Head;
   while (ScanFD) {
      if (ScanFD->Nick) {
         if (strcasecmp(ScanFD->Nick, nick) == 0) {
            ScanFD->NoReSend = noresend;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);
}

// update FilesList from the List which match FileName and Nick with Data
void FilesDetailList::updateFilesDetailNickFileData(char *nick, char *file, void *data) {
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();
   if ( (nick == NULL) || (file == NULL) ) return;

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if ( (ScanFD->Nick) && (ScanFD->FileName) ) {
         if ( (strcasecmp(ScanFD->Nick, nick) == 0) &&
              (strcasecmp(ScanFD->FileName, file) == 0) ) {
            ScanFD->Data = data;
            break;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);
}

// update FilesList from the List which match FileName and Nick with Q.
void FilesDetailList::updateFilesDetailNickFileQ(char *nick, char *file, long q) {
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();
   if ( (nick == NULL) || (file == NULL) ) return;

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if ( (ScanFD->Nick) && (ScanFD->FileName) ) {
         if ( (strcasecmp(ScanFD->Nick, nick) == 0) &&
              (strcasecmp(ScanFD->FileName, file) == 0) ) {
            ScanFD->QueueNum = q;
            break;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);
}

// update FilesList from the List which match FileName and Nick with ResumePos
// Used by TimerThr, to update FileResumePosition
void FilesDetailList::updateFilesDetailNickFileResume(char *nick, char *file, size_t ResumePos) {
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();
   if ( (nick == NULL) || (file == NULL) ) return;

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if ( (ScanFD->Nick) && (ScanFD->FileName) ) {
         if ( (strcasecmp(ScanFD->Nick, nick) == 0) &&
              (strcasecmp(ScanFD->FileName, file) == 0) ) {
            ScanFD->FileResumePosition = ResumePos;
            break;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);
}


// update FilesList from the List which match FileName and Nick with TCPConnect
// Used by DCCThr, DCCServerThr and TimerThr, to update the TCPConnect.
// If file passes is NULL, check agaisnt Nick alone (DCCThr)
void FilesDetailList::updateFilesDetailNickFileConnection(char *nick, char *file, TCPConnect *TCPCon) {
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();
   if (nick == NULL) return;

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if ( (ScanFD->Nick) &&
           (file) &&
           (ScanFD->FileName) ) {
         if ( (strcasecmp(ScanFD->Nick, nick) == 0) &&
               (strcasecmp(ScanFD->FileName, file) == 0) ) {
             ScanFD->Connection = TCPCon;
             break;
         }
      } else if ( (ScanFD->Nick) &&
                  (file == NULL) ) {
         if (strcasecmp(ScanFD->Nick, nick) == 0) {
            ScanFD->Connection = TCPCon;
             break;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);
}

// update FilesList from the List which match FileName and Nick with TCPConnect
// Used by Transfer class, to update the TCPConnect.
// Returns the ConnectionMessage in that FD
char FilesDetailList::updateFilesDetailNickFileConnectionDetails(char *nick, char *file, size_t bytes_sent, size_t bytes_recvd, time_t born, size_t upload_bps, size_t dwnld_bps) {
FilesDetail *ScanFD;
char ConMessage = CONNECTION_MESSAGE_NONE;

   pruneOldFiles(false);

   TRACE();
   if ( (nick == NULL) || (file == NULL) ) return(ConMessage);

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if ( (ScanFD->Nick) && (ScanFD->FileName) ) {
         if ( (strcasecmp(ScanFD->Nick, nick) == 0) &&
              (strcasecmp(ScanFD->FileName, file) == 0) ) {
            // Got the match, update it all.
            ScanFD->BytesSent = bytes_sent;
            ScanFD->BytesReceived = bytes_recvd;
            ScanFD->Born = born;
            ScanFD->UploadBps = upload_bps;
            ScanFD->DownloadBps = dwnld_bps;
            ConMessage = ScanFD->ConnectionMessage;
            break;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

   return(ConMessage);
}

// update FilesList from the List which match FileName and Nick with
// the ConnectionMessage. Used by DwnldInitThr, TabBookWindow
// Returns true if update was done, else false
bool FilesDetailList::updateFilesDetailNickFileConnectionMessage(char *nick, char *file, char message) {
FilesDetail *ScanFD;
bool updateflag = false;

   pruneOldFiles(false);

   TRACE();
   if ( (nick == NULL) || (file == NULL) ) return(updateflag);

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if ( (ScanFD->Nick) && (ScanFD->FileName) ) {
         if ( (strcasecmp(ScanFD->Nick, nick) == 0) &&
              (strcasecmp(ScanFD->FileName, file) == 0) ) {
            // Got the match, update it.
            ScanFD->ConnectionMessage = message;
            updateflag = true;
            break;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

   return(updateflag);
}

// update FilesList with the ConnectionMessage. Used by TabBookWindow
// Returns true if update was done, else false
bool FilesDetailList::updateFilesDetailAllConnectionMessage(char message) {
FilesDetail *ScanFD;
bool updateflag = false;

   pruneOldFiles(false);

   TRACE();

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      // Got the match, update it.
      ScanFD->ConnectionMessage = message;
      // We walk through the full list, and update all.
      updateflag = true;
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

   return(updateflag);
}


// update FilesList from the List which match Nick (all matches of nick)
// with the ConnectionMessage. Used by FromServerThr
// Returns true if update was done, else false
bool FilesDetailList::updateFilesDetailAllNickConnectionMessage(char *nick, char message) {
FilesDetail *ScanFD;
bool updateflag = false;

   pruneOldFiles(false);

   TRACE();
   if (nick == NULL) return(updateflag);

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if (ScanFD->Nick) {
         if (strcasecmp(ScanFD->Nick, nick) == 0) {
            // Got the match, update it.
            ScanFD->ConnectionMessage = message;
            // We walk through the full list, and update all Nick matches.
            updateflag = true;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

   return(updateflag);
}

// del Files from the List based on FilesDetail.
// criteria is that the pointer to actual structure should match
// Used by DwnldWaiting
// Should only be called to add and later delete same structure.
void FilesDetailList::delFilesDetail(FilesDetail *FD) {
FilesDetail *ScanFD;

   TRACE();
   if (FD == NULL) return;

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if (ScanFD == FD) {
         ScanFD->Time = 0; // Let prune take care of it.
         break;
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);

   // Call prune so that it removes Time Entries with 0
   pruneOldFiles(true);
}


// Add Files to the List.
// Assume that the callee has allocated FD and all its pointers for us.
// It can also be a list of FilesDetail to be added to our list.
void FilesDetailList::addFilesDetail(FilesDetail *FD) {
bool Bubbled;
FilesDetail *ScanFD, *preScanFD, *tempFD, *LastFD;

   TRACE();
   if (FD == NULL) return;

   // this check only if its a single entry.
   if ( (FD->Next == NULL) && isFilesDetailPresent(FD)) {
      // Free up FD, as the callee has allocated exclusively for us.
      freeFilesDetailList(FD);
      return;
   }
   // Only add if Nick is not in our list.

   // Set the Time Entry on the list to be added.
   // Also set the UpdateCount to 0. <-- we dont do this, as part of
   // Fast FileList Algo, we add files with a set UpdateCount as received
   // from other clients.
   ScanFD = FD;
   LastFD = FD;
   while (ScanFD) {
      ScanFD->Time = time(NULL);
      // ScanFD->UpdateCount = 0;

      // Lets note the last entry.
      if (ScanFD->Next == NULL) LastFD = ScanFD;

      ScanFD = ScanFD->Next;
   }
   // LastFD has the last entry.

   WaitForMutex(FilesDetailListMutex);

   LastFD->Next = Head;
   Head = FD;
   // We have just added the list right in front.

   // Just return if not to be sorted.
   if ( (Sort != FILELIST_SORT_BY_FILENAME) && (Sort != FILELIST_SORT_BY_DIR_FILENAME) ) {
      ReleaseMutex(FilesDetailListMutex);
      return;
   }

   // Lets add it in sorted order, which will be efficient in the long run.
   // Lets do a bubble sort. Only if Sort is set appropriately.
   // FILELIST_NO_SORT (All Queues)
   // FILELIST_SORT_BY_FILENAME (Other things which are needed in filename
   //                            sorted order)
   // FILELIST_SORT_BY_DIR_FILENAME (MyFilesDB uses this)
   do {

      // Initialise FD and ScanFD to the supposed head.
      ScanFD = Head;
      preScanFD = NULL;
      Bubbled = false;

      while (ScanFD->Next != NULL) {
         char SrcFile1[1024], SrcFile2[1024]; // Possible overflow on long file/dir

         SrcFile1[0] = '\0';
         SrcFile2[0] = '\0';

         // Lets form the comparison strings based on Sort.
         if (Sort == FILELIST_SORT_BY_DIR_FILENAME) {
            if (ScanFD->DirName) {
               strcpy(SrcFile1, ScanFD->DirName);
            }
            if (ScanFD->Next->DirName) {
               strcpy(SrcFile2, ScanFD->Next->DirName);
            }
         }
         if (ScanFD->FileName) {
            strcat(SrcFile1, ScanFD->FileName);
         }
         if (ScanFD->Next->FileName) {
            strcat(SrcFile2, ScanFD->Next->FileName);
         }

         if ( (strcasecmp(SrcFile1, SrcFile2) > 0) ||
              ( 
                (strcasecmp(SrcFile1, SrcFile2) == 0) &&
                (ScanFD->FileSize < ScanFD->Next->FileSize) 
              )
            ) {
            // They need to be swapped.
            Bubbled = true;
            if (preScanFD == NULL) {
            // Change in the Head of List.
               Head = ScanFD->Next;
               tempFD = ScanFD->Next->Next;
               ScanFD->Next->Next = ScanFD;
               ScanFD->Next = tempFD;
               preScanFD = Head;
            }
            else {
               tempFD = ScanFD->Next->Next;
               ScanFD->Next->Next = ScanFD;
               preScanFD->Next = ScanFD->Next;
               ScanFD->Next = tempFD;
               preScanFD = preScanFD->Next;
            }
            continue;
         }
         preScanFD = ScanFD;
         ScanFD = ScanFD->Next;
      }
   } while (Bubbled);

   ReleaseMutex(FilesDetailListMutex);
}

// Returns a copy of the FilesDetail sent.
FilesDetail *FilesDetailList::copyFilesDetail(FilesDetail *oldFD) {
FilesDetail *FD;

   TRACE();
   FD = new FilesDetail;
   this->initFilesDetail(FD);
   if (oldFD->FileName) {
      FD->FileName = new char[strlen(oldFD->FileName) + 1];
      strcpy(FD->FileName, oldFD->FileName);
      FD->ServingDirIndex = oldFD->ServingDirIndex;
   }
   FD->FileSize = oldFD->FileSize;
   FD->Time = oldFD->Time;
   FD->UpdateCount = oldFD->UpdateCount;
   FD->TriggerType = oldFD->TriggerType;
   FD->Port = oldFD->Port;
   if (oldFD->Nick) {
      FD->Nick = new char[strlen(oldFD->Nick) + 1];
      strcpy(FD->Nick, oldFD->Nick);
   }
   if (oldFD->PropagatedNick) {
      FD->PropagatedNick = new char[strlen(oldFD->PropagatedNick) + 1];
      strcpy(FD->PropagatedNick, oldFD->PropagatedNick);
   }
   if (oldFD->TriggerName) {
      FD->TriggerName = new char[strlen(oldFD->TriggerName) + 1];
      strcpy(FD->TriggerName, oldFD->TriggerName);
   }
   if (oldFD->DirName) {
      FD->DirName = new char[strlen(oldFD->DirName) + 1];
      strcpy(FD->DirName, oldFD->DirName);
   }
   if (oldFD->DottedIP) {
      FD->DottedIP = new char[strlen(oldFD->DottedIP) + 1];
      strcpy(FD->DottedIP, oldFD->DottedIP);
   }
   FD->ManualSend = oldFD->ManualSend;
   FD->NoReSend = oldFD->NoReSend;
   FD->RetryCount = oldFD->RetryCount;
   FD->PackNum = oldFD->PackNum;
   FD->QueueNum = oldFD->QueueNum;
   FD->FileResumePosition = oldFD->FileResumePosition;

   FD->ClientType = oldFD->ClientType;
   FD->CurrentSends = oldFD->CurrentSends;
   FD->TotalSends = oldFD->TotalSends;
   FD->CurrentQueues = oldFD->CurrentQueues;
   FD->TotalQueues = oldFD->TotalQueues;

   FD->DownloadState = oldFD->DownloadState;

#if 0
   oldFD->Connection->lock();
   if (oldFD->Connection && oldFD->Connection->isValid()) {
      FD->Connection = oldFD->Connection;
      FD->BytesSent = FD->Connection->BytesSent;
      FD->BytesReceived = FD->Connection->BytesReceived;
      FD->Born = FD->Connection->Born;
      FD->UploadBps = FD->Connection->getUploadBps();
      FD->DownloadBps = FD->Connection->getDownloadBps();
   }
   else {
      FD->Connection = NULL;
      FD->BytesSent = 0;
      FD->BytesReceived = 0;
      FD->Born = 0;
      FD->UploadBps = 0;
      FD->DownloadBps = 0;
   }
   oldFD->Connection->unlock();
#endif
   FD->Connection = oldFD->Connection;
   FD->ConnectionMessage = oldFD->ConnectionMessage;
   FD->BytesSent = oldFD->BytesSent;
   FD->BytesReceived = oldFD->BytesReceived;
   FD->Born = oldFD->Born;
   FD->UploadBps = oldFD->UploadBps;
   FD->DownloadBps = oldFD->DownloadBps;

   FD->Data = oldFD->Data;

   return(FD);
}

// Checks if the given Nick, FileName is already present in the list.
// Returns the index at which its present.
// 0 => Not present.
int FilesDetailList::isPresentMatchingNickFileName(const char *nick, const char *file) {
FilesDetail *ScanFD;
int index = 0;

   pruneOldFiles(false);

   TRACE();
   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD != NULL) {
      index ++;
      if ( (ScanFD->FileName) &&
           (strcasecmp(ScanFD->FileName, file) == 0) &&
           (ScanFD->Nick) &&
           (strcasecmp(ScanFD->Nick, nick) == 0) ) {
         ReleaseMutex(FilesDetailListMutex);
         return(index);
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);
   return(0);
}

// Checks if the given PropagatedNick is already present in the list.
bool FilesDetailList::isPresentMatchingPropagatedNick(const char *pro_nick) {
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();

   if ( (pro_nick == NULL) || (strlen(pro_nick) == 0) ) return(false);

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD != NULL) {
      if ( (ScanFD->PropagatedNick) &&
           (strcasecmp(ScanFD->PropagatedNick, pro_nick) == 0) ) {
         ReleaseMutex(FilesDetailListMutex);
         return(true);
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);
   return(false);
}

// Checks if the given FileName is already present in the list.
bool FilesDetailList::isPresentMatchingFileName(const char *file) {
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();

   if ( (file == NULL) || (strlen(file) == 0) ) return(false);

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD != NULL) {
      if ( (ScanFD->FileName) &&
           (strcasecmp(ScanFD->FileName, file) == 0) ) {
         ReleaseMutex(FilesDetailListMutex);
         return(true);
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);
   return(false);
}

// Adds the FilesDetail at index.
// Caller has allocated FilesDetail to be added.
// index = 0 => put as last entry.
void FilesDetailList::addFilesDetailAtIndex(FilesDetail *FD, int index) {
FilesDetail *ScanFD, *LastFD = NULL;
int cur;
bool added;

   pruneOldFiles(false);

   TRACE();

   if ( (FD == NULL) || (index < 0) ) return;

   if (isFilesDetailPresent(FD)) {
      // Free up FD, as the callee has allocated exclusively for us.
      freeFilesDetailList(FD);
      delete FD;
      return;
   }
   // Only add if Nick is not in our list.

   FD->Time = time(NULL);
   // FD->UpdateCount = 0; Dont do anythign to UpdateCount, though in this
                           // it doesnt matter.

   WaitForMutex(FilesDetailListMutex);

   if ( (Head == NULL) || (index == 1) ) {
      FD->Next = Head;
      Head = FD;
      ReleaseMutex(FilesDetailListMutex);
      return;
   }

   // We are here => we have at least 1 item in list. and index is at least 2
   // index can also be 0, in which case there will be no hit for cur == index
   // and will end up putting the entry as the last entry.
   ScanFD = Head;
   index--; // so we get the item after which it needs to be added.
   cur = 1;

   // Note that if # of elements is 4, and we come in with index 10.
   // In such cases it needs to be added as the last element.
   added = false;
   while (ScanFD != NULL) {
      if (cur == index) {
         FD->Next = ScanFD->Next;
         ScanFD->Next = FD;
         added = true;
         break;
      }
      cur++;
      if (ScanFD->Next == NULL) LastFD = ScanFD;
      ScanFD = ScanFD->Next;
   }

   if ( (added == false) && (LastFD) ) {
      LastFD->Next = FD;
      FD->Next = NULL;
   }

   ReleaseMutex(FilesDetailListMutex);
}

// Returns the FilesDetail at index.
// The caller needs to free the FilesDetail returned.
FilesDetail *FilesDetailList::getFilesDetailAtIndex(int index) {
FilesDetail *FD = NULL;
FilesDetail *ScanFD;
int cur = 0;

   pruneOldFiles(false);

   TRACE();
   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD != NULL) {
      cur++;
      if (cur == index) {
         FD = copyFilesDetail(ScanFD);
         ReleaseMutex(FilesDetailListMutex);
         return(FD);
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);
   return(NULL);
}

// Returns a list which match the Nick and DownloadState
// The caller needs to free the List returned.
FilesDetail *FilesDetailList::getFilesOfNickByDownloadState(char *nick, char dstate) {
FilesDetail *FD = NULL;
FilesDetail *HeadFD = NULL;
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();
   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD != NULL) {
      if ( ScanFD->Nick && (strcasecmp(ScanFD->Nick, nick) == 0) &&
           (ScanFD->DownloadState == dstate) ) {
         FD = copyFilesDetail(ScanFD);
         FD->Next = HeadFD;
         HeadFD = FD;
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);
   return(HeadFD);
}

// Returns a list which match the Nick and FileName.
// The caller needs to free the List returned.
FilesDetail *FilesDetailList::getFilesDetailListNickFile(char *nick, char *fn) {
FilesDetail *FD = NULL;
FilesDetail *HeadFD = NULL;
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();
   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD != NULL) {
      if ( (ScanFD->FileName && strcasecmp(ScanFD->FileName, fn) == 0) 
           && (ScanFD->Nick && strcasecmp(ScanFD->Nick, nick) == 0) ) {
         FD = copyFilesDetail(ScanFD);
         FD->Next = HeadFD;
         HeadFD = FD;
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);
   return(HeadFD);
}


// Returns a list which match the Nick and FileName and DirName
// DirName = space => empty Dir Name. (Note it shouldnt be NULL)
// The caller needs to free the List returned.
// Used possibly only by DwnldInitThr
// Also the Dirame coming in as "dn", might not have the leading 
// FS_DIR_SEP_CHAR
FilesDetail *FilesDetailList::getFilesDetailListNickFileDir(char *nick, char *fn, char *dn) {
FilesDetail *FD = NULL;
FilesDetail *HeadFD = NULL;
FilesDetail *ScanFD;
char *dir_name;
char *dn_dir_name;
char EmptyDir[2];

   pruneOldFiles(false);

   TRACE();

   if ( (nick == NULL) || (fn == NULL) || (dn == NULL) ) return(NULL);

   if (dn[0] == FS_DIR_SEP_CHAR) {
      dn_dir_name = &dn[1];
   }
   else {
      dn_dir_name = dn;
   }

   strcpy(EmptyDir, " ");

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD != NULL) {
      if ( (ScanFD->FileName && strcasecmp(ScanFD->FileName, fn) == 0) 
           && (ScanFD->Nick && strcasecmp(ScanFD->Nick, nick) == 0) ) {
         if (ScanFD->DirName == NULL) {
            dir_name = EmptyDir;
         }
         else {
            // Lets compare ignoring the FS_DIR_SEP_CHAR at start.
            if (ScanFD->DirName[0] == FS_DIR_SEP_CHAR) {
               dir_name = &(ScanFD->DirName[1]);
            }
            else {
               dir_name = ScanFD->DirName;
            }
         }
         if (strcasecmp(dn_dir_name, dir_name) == 0) {
            FD = copyFilesDetail(ScanFD);
            FD->Next = HeadFD;
            HeadFD = FD;
         }
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);
   return(HeadFD);
}


// Returns a list which exactly match the FileName.
// The caller needs to free the List returned.
FilesDetail *FilesDetailList::getFilesDetailListMatchingFileName(char *fn) {
FilesDetail *FD = NULL;
FilesDetail *HeadFD = NULL;
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();
   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD != NULL) {
      if (ScanFD->FileName && strcasecmp(ScanFD->FileName, fn) == 0) {
         FD = copyFilesDetail(ScanFD);
         FD->Next = HeadFD;
         HeadFD = FD;
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);
   return(HeadFD);
}

// Returns the first FD which matches the given FileName and DottedIP.
// The caller needs to free the FD returned.
FilesDetail *FilesDetailList::getFilesDetailFileNameDottedIP(char *filename, char *dottedip) {
FilesDetail *FD = NULL;
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();
   if ( (filename == NULL) || (strlen(filename) == 0) ||
        (dottedip == NULL) || (strlen(dottedip) == 0) ) {
      return(FD);
   }

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD != NULL) {
      if ( (ScanFD->FileName && strcmp(ScanFD->FileName, filename) == 0) &&
           (ScanFD->DottedIP && strcasecmp(ScanFD->DottedIP, dottedip) == 0)
         ) {
         FD = copyFilesDetail(ScanFD);
         break;
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);
   return(FD);
}

// Returns a list containing the Files of DottedIP.
// The caller needs to free the List returned.
FilesDetail *FilesDetailList::getFilesDetailListOfDottedIP(char *dip) {
FilesDetail *FD = NULL;
FilesDetail *HeadFD = NULL;
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();
   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD != NULL) {
      if (ScanFD->DottedIP && (strcasecmp(ScanFD->DottedIP, dip) == 0)) {
         FD = copyFilesDetail(ScanFD);
         FD->Next = HeadFD;
         HeadFD = FD;
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);
   return(HeadFD);
}

// Returns a list containing the Files of Nick. if CountIntPtr is not NULL,
// it will return the count in it.
// The caller needs to free the List returned.
FilesDetail *FilesDetailList::getFilesDetailListOfNick(const char *nick, int *CountIntPtr) {
FilesDetail *FD = NULL;
FilesDetail *HeadFD = NULL;
FilesDetail *ScanFD;
int count = 0;

   pruneOldFiles(false);

   TRACE();
   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD != NULL) {
      if (strcasecmp(ScanFD->Nick, nick) == 0) {
         FD = copyFilesDetail(ScanFD);
         FD->Next = HeadFD;
         HeadFD = FD;
         count++;
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);

   if (CountIntPtr) {
      *CountIntPtr = count;
   }

   return(HeadFD);
}

// Returns a list which satisfies the FileName and FileSize (within 2MB)
// The caller needs to call freeFilesDetailList(...) to delete this list.
FilesDetail *FilesDetailList::getFilesDetailListMatchingFileAndSize(char *file, size_t size) {
FilesDetail *FD = NULL;
FilesDetail *HeadFD = NULL;
FilesDetail *ScanFD;
size_t sizedelta;

   pruneOldFiles(false);

   TRACE();
   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD != NULL) {
      if (strcasecmp(ScanFD->FileName, file) == 0) {
         if (ScanFD->FileSize > size) {
            sizedelta = ScanFD->FileSize - size;
         }
         else {
            sizedelta = size - ScanFD->FileSize;
         }
         if (sizedelta < FILES_DETAIL_LIST_SIZEDELTA) {
            FD = copyFilesDetail(ScanFD);
            FD->Next = HeadFD;
            HeadFD = FD;
         }
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);

   return(HeadFD);
}

// Returns a search list which satisfies the SearchString criteria.
// The caller needs to call freeFilesDetailList(...) to delete this list.
// Order of list is in same order as the internal list.
// If CountIntPtr is non NULL, return the count in it.
FilesDetail *FilesDetailList::searchFilesDetailList(const char *SearchString, int *CountIntPtr) {
FilesDetail *FD = NULL;
FilesDetail *HeadFD = NULL;
FilesDetail *TailFD = NULL;
FilesDetail *ScanFD;
LineParse LineP;
bool listall = false;
int count = 0;

   pruneOldFiles(false);

   TRACE();
   if (strcmp(SearchString, "*") == 0) {
      listall= true;
   }

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      LineP = ScanFD->FileName;
      if (listall || LineP.isWordsInLine((char *)SearchString)) {
         FD = copyFilesDetail(ScanFD); // FD->Next is already NULL
         if (HeadFD == NULL) {
           HeadFD = FD;
           TailFD = FD;
         }
         else {
            TailFD->Next = FD;
            TailFD = FD;
         }
         count++;
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);

   if (CountIntPtr) {
      *CountIntPtr = count;
   }

   return(HeadFD);
}

// Returns a search list which satisfies the SearchDir criteria.
// The caller needs to call freeFilesDetailList(...) to delete this list.
// The order returned is in ascending order already as addFiles...
// stores stuff in ascending order.
// And DirName entry which matches all chars of SearchDir from start
// is considered a hit.
FilesDetail *FilesDetailList::searchDirFilesDetailList(char *SearchDir) {
FilesDetail *FD = NULL;
FilesDetail *HeadFD = NULL;
FilesDetail *ScanFD;
bool listall = false;
char DirString[1];
char *DirString2;

   pruneOldFiles(false);

   DirString[0] = '\0';

   TRACE();
   if ( (SearchDir == NULL) || (strlen(SearchDir) == 0) ) {
      listall = true;
   }

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {

      if (ScanFD->DirName == NULL) {
         DirString2 = DirString;
      }
      else {
         DirString2 = ScanFD->DirName;
      }
      if (listall || 
           (strncasecmp(SearchDir, DirString2, strlen(SearchDir)) == 0)
         ) {
         // Got a hit.
         FD = copyFilesDetail(ScanFD);
         FD->Next = HeadFD;
         HeadFD = FD;
      }
      ScanFD = ScanFD->Next;
   }

   ReleaseMutex(FilesDetailListMutex);
   return(HeadFD);
}


// Takes a list of FilesDetail. Removes aproximate repetitions and presents
// new list with possible new Head of List.
// Deprecated - not used anymore.
FilesDetail *FilesDetailList::removeRepetitionsFilesDetailList(FilesDetail *TheHead) {
FilesDetail *ScanFD, *tmpScanFD;
size_t sizedelta;
 
   TRACE();
//   FD = sortFilesDetailList(TheHead);
   // We have it sorted. Lets just do a scan and remove the repetitions.

   ScanFD = TheHead;
  
   while ( ScanFD && ScanFD->Next ) {
      if (strcasecmp(ScanFD->FileName, ScanFD->Next->FileName) == 0) {
      // Same File Name.
         if (ScanFD->FileSize > ScanFD->Next->FileSize) {
            sizedelta = ScanFD->FileSize - ScanFD->Next->FileSize;
         }
         else {
            sizedelta = ScanFD->Next->FileSize - ScanFD->FileSize;
         }
COUT(cout << "File1: " << ScanFD->FileName << " File2: " << ScanFD->Next->FileName << " Size1: " << ScanFD->FileSize << " Size2: " << ScanFD->Next->FileSize << " sizedelta: " << sizedelta << endl;)
         if (sizedelta < FILES_DETAIL_LIST_SIZEDELTA) {
         // FileSize is within 2 MB difference.
         // This can be deleted. (Delete 2nd entry)
            delete [] ScanFD->Next->FileName;
            delete [] ScanFD->Next->DottedIP;
            delete [] ScanFD->Next->Nick;
            delete [] ScanFD->Next->PropagatedNick;
            delete [] ScanFD->Next->TriggerName;
            delete [] ScanFD->Next->DirName;
            tmpScanFD = ScanFD->Next->Next;
            delete ScanFD->Next;
            ScanFD->Next = tmpScanFD;
            continue;
         }
      }
      ScanFD = ScanFD->Next;
   }

   return(TheHead);
}


// Flush all the entries which are present in the List and are older than
// TimeOut. If compulsory = true, ignores if TimeOut is set to 0 or the 
// LastPruneTime and compulsorily goes thru the list and prunes.
void FilesDetailList::pruneOldFiles(bool compulsory) {
FilesDetail *Scan;
FilesDetail *ScanButOne;
time_t CurrentTime;

   TRACE();
   if ( (TimeOut == 0) && (compulsory == false) ) return;

   CurrentTime = time(NULL);

   WaitForMutex(FilesDetailListMutex);
   if ( ((CurrentTime - LastPruneTime) < FILES_DETAIL_LIST_DELTA_PRUNE) &&
        (compulsory == false) ) {
      ReleaseMutex(FilesDetailListMutex);
      return;
   }

   LastPruneTime = CurrentTime;

   Scan = Head;
   ScanButOne = Head;
   while (Scan != NULL) {
      if ( (((CurrentTime - Scan->Time) > TimeOut) && (compulsory == false))
           || ((Scan->Time == 0) && compulsory) ) {
// First condition is when its a normal list with a TimeOut.
// second condition is when list needs to be deleted of Scan->Time == 0
         // This one needs to be deleted.
         if (Scan == ScanButOne) {
         // 1st Element
            Head = Head->Next;
            Scan->Next = NULL;
            freeFilesDetailList(Scan);
            Scan = Head;
            ScanButOne = Head;
         }
         else {
         // Not 1st Element. Head is intact.
            ScanButOne->Next = Scan->Next;
            Scan->Next = NULL;
            freeFilesDetailList(Scan);
            Scan = ScanButOne->Next;
         }
      }
      else {
         ScanButOne = Scan;
         Scan = Scan->Next;
      }
   }
   ReleaseMutex(FilesDetailListMutex);
}

// Check if this FilesDetail is already in our DB.
// Criteria is that Nick, FileName, DirName is matched. (FileSize is not)
// Also if either or both FileNames are NULL then its assumed a hit
// As a side effect update Time on this one. Its called only by addFiles
// Updates the already existing FD with FileSize though.
bool FilesDetailList::isFilesDetailPresent(FilesDetail *FD) {
FilesDetail *Scan;
char EmptyDir[1];
char *Dir1;
char *Dir2;

   TRACE();
   if (FD == NULL) return(true);

   pruneOldFiles(false);

   EmptyDir[0] = '\0';
   if (FD->DirName == NULL) {
      Dir1 = EmptyDir;
   }
   else {
      Dir1 = FD->DirName;
   }

   WaitForMutex(FilesDetailListMutex);

   Scan = Head;
   while (Scan != NULL) {
      if ( FD->Nick && Scan->Nick && (strcasecmp(FD->Nick, Scan->Nick) == 0) ) {
         if ( (FD->FileName == NULL) || (Scan->FileName == NULL) ||
            (strcasecmp(FD->FileName, Scan->FileName) == 0) ) {
            if (Scan->DirName == NULL) {
               Dir2 = EmptyDir;
            }
            else {
               Dir2 = Scan->DirName;
            }
            if (strcasecmp(Dir1, Dir2) == 0) {
               Scan->Time = time(NULL);
               Scan->FileSize = FD->FileSize;
               ReleaseMutex(FilesDetailListMutex);
               return(true);
            }
         }
      }
      Scan = Scan->Next;
   }

   ReleaseMutex(FilesDetailListMutex);
   return(false);
}

// Used to update all files held by nick to the send and q information
// passed.
void FilesDetailList::updateSendsQueuesOfNick(char *nick, int cur_sends, int tot_sends, int cur_queues, int tot_queues) {
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();
   if (nick == NULL) return;

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if (ScanFD->Nick) {
         if (strcasecmp(ScanFD->Nick, nick) == 0) {
            ScanFD->CurrentSends = cur_sends;
            ScanFD->TotalSends = tot_sends;
            ScanFD->CurrentQueues = cur_queues;
            ScanFD->TotalQueues = tot_queues;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);
}

// Used to update all files held by nick to current time. This helps in
// not accessing the trigger too frequently. This updates the update count too.
// On the 10th update it actually deletes the files of nick.
// As this will be called just before accessing the nick's trigger, it will
// work out as planned.
void FilesDetailList::updateTimeFilesOfNick(char *varnick) {
FilesDetail *Scan;
time_t CurrentTime = time(NULL);

   TRACE();

   WaitForMutex(FilesDetailListMutex);

   Scan = Head;
   while (Scan != NULL) {
      if (strcasecmp(varnick, Scan->Nick) == 0) {
         Scan->Time = CurrentTime;
         Scan->UpdateCount = Scan->UpdateCount + 1;
         if (Scan->UpdateCount > FILES_DETAIL_LIST_UPDATE_ROLLOVER) {
            Scan->Time = 0; // This will cause prune to remove this entry.
         }
      }
      Scan = Scan->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

   // Call prune so that it removes Time Entries with 0
   pruneOldFiles(true);
}

// Used to update the UpdateCount to a particular value of all entries
// nick. used by the FFLC version 2 algo.
void FilesDetailList::updateUpdateCountOfNick(char *nick, int newUpdateCount) {
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();
   if (nick == NULL) return;
   if (newUpdateCount > FILES_DETAIL_LIST_UPDATE_ROLLOVER) return;

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   while (ScanFD) {
      if (ScanFD->Nick) {
         if (strcasecmp(ScanFD->Nick, nick) == 0) {
            ScanFD->UpdateCount = newUpdateCount;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);
}

// Used to generate the checksum of the entries held by nick.
// filedirnamelens is the sum of the lengths of the DirNames and FileNames.
// count = number of files.
// We dont count FileNames which are "TriggerTemplate", "Inaccessible Server"
// or "No Files Present"
// This is so, as, we compare Nick's serving file lists checksum, with what 
// we locally hold, and if his is in MyFilesDB and we hold in FilesDB, they
// will give different checksum values. We want it to be same.
// To be in sync with how we generate checksum for our files, we follow same
// procedure. Get Checksum of 'S' files and 'P' file seperately.
// To be in sync with Helper::helperFServNicklist()
void FilesDetailList::getCheckSums(char *nick, long *filedirnamelens, long *counts, long *filedirnamelenp, long *countp) {
FilesDetail *ScanFD;

   pruneOldFiles(false);

   TRACE();
   if (nick == NULL) return;

   WaitForMutex(FilesDetailListMutex);

   ScanFD = Head;
   *filedirnamelens = 0;
   *counts = 0;
   *filedirnamelenp = 0;
   *countp = 0;

   while (ScanFD) {
      if ( (ScanFD->Nick) && (strcasecmp(ScanFD->Nick, nick) == 0) ) {
         if (ScanFD->FileName) {
            if ( (strcasecmp(ScanFD->FileName, "TriggerTemplate") == 0) ||
                 (strcasecmp(ScanFD->FileName, "Inaccessible Server") == 0) ||
                 (strcasecmp(ScanFD->FileName, "No Files Present") == 0) ) {
               // These dont count.
               ScanFD = ScanFD->Next;
               continue;
            }
         }

         if (ScanFD->DownloadState == DOWNLOADSTATE_SERVING) {

            // Add DirName.
            if (ScanFD->DirName) {
               *filedirnamelens = *filedirnamelens + strlen(ScanFD->DirName);
            }

            // Add FileName
            if (ScanFD->FileName) {
               *filedirnamelens = *filedirnamelens + strlen(ScanFD->FileName);
            }

            *counts = *counts + 1;
         }
         else if (ScanFD->DownloadState == DOWNLOADSTATE_PARTIAL) {
             // Partials - No DirName - Add FileName
            if (ScanFD->FileName) {
               *filedirnamelenp = *filedirnamelenp + strlen(ScanFD->FileName);
            }

            *countp = *countp + 1;
         }
      }
      ScanFD = ScanFD->Next;
   }
   ReleaseMutex(FilesDetailListMutex);

}

// Assumes chksum is (4 * size of long) + 1 bytes long and allocated by caller.
// It generates hex string by
// totalfiledirnamelens (size of long) in hex = 2 * size of long
// counts (size of long) in hex = 2 * size of long
// On converting the leading 0's are removed so its more compact.
void FilesDetailList::getCheckSumString(long filedirnamelen, long count, char *chksum) {
int i, k;
unsigned char *j;
char hexpair[3];
bool skip;

   TRACE();

   chksum[0] = '\0';

   j = (unsigned char *) &filedirnamelen;
   skip = true;
#if __BYTE_ORDER == __LITTLE_ENDIAN
   for (i = sizeof(long) - 1; i >= 0; i--) {
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
   for (i = 0; i < sizeof(long); i++) {
#endif
      if (skip && (j[i] == 0)) {
         continue;
      }
      skip = false;
      sprintf(hexpair, "%02X", j[i]);
      strcat(chksum, hexpair);
   }

   j = (unsigned char *) &count;
   skip = true;
#if __BYTE_ORDER == __LITTLE_ENDIAN
   for (i = sizeof(long) - 1; i >= 0; i--) {
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
   for (i = 0; i < sizeof(long); i++) {
#endif
      if (skip && (j[i] == 0)) {
         continue;
      }
      skip = false;
      sprintf(hexpair, "%02X", j[i]);
      strcat(chksum, hexpair);
   }

   if (chksum[0] == '\0') {
      strcpy(chksum, "0");
   }

   COUT(cout << "getCheckSumString: filedirnamelen " << filedirnamelen << " count: " << count << " chksum: " << chksum << endl;)
}

// Checks if the list contains any entry of varnick.
// Doesnt check if Filename has any entry.
bool FilesDetailList::isNickPresent(const char *varnick) {
FilesDetail *Scan;
bool retvalb = false;

   TRACE();

   pruneOldFiles(false);

   WaitForMutex(FilesDetailListMutex);

   Scan = Head;
   while (Scan != NULL) {
      if (strcasecmp(varnick, Scan->Nick) == 0) {
         retvalb = true;
         break;
      }
      Scan = Scan->Next;
   }
   ReleaseMutex(FilesDetailListMutex);
   return(retvalb);
}

// Check if the nick is present in our list.
// If present and FileName is "TriggerTemplate" then it doesnt count 
// as it existing.
// If "No Files Present" exists, or "Inaccessible Server" exists, we consider
// it as having the file listing. Cause all this is an effort to reduce
// repeated trigger access.
bool FilesDetailList::isFilesOfNickPresent(const char *varnick) {
bool retvalb = false;
FilesDetail *Scan;

   TRACE();
   pruneOldFiles(false);

   WaitForMutex(FilesDetailListMutex);

   Scan = Head;
   while (Scan != NULL) {
      if (strcasecmp(varnick, Scan->Nick) == 0) {
         if ( (Scan->FileName) && 
              strcasecmp(Scan->FileName, "TriggerTemplate") ) {
            retvalb = true;
            break;
         }
      }
      Scan = Scan->Next;
   }

   ReleaseMutex(FilesDetailListMutex);
   return(retvalb);
}

// Rename oldnick to newnick through the list. -> nick change.
void FilesDetailList::renameNickToNewNick(char *oldnick, char *newnick) {
FilesDetail *Scan;

   pruneOldFiles(false);

   TRACE();
   WaitForMutex(FilesDetailListMutex);

   Scan = Head;
   while (Scan != NULL) {
      if (strcasecmp(oldnick, Scan->Nick) == 0) {
      // Got a hit, rename this guy to new nick.
         delete [] Scan->Nick;
         Scan->Nick = new char[strlen(newnick) + 1];
         strcpy(Scan->Nick, newnick);
      }
      Scan = Scan->Next;
   }
   ReleaseMutex(FilesDetailListMutex);
}

void FilesDetailList::printDebug(FilesDetail *var) {
FilesDetail *Scan;
int i = 1;
double TotalBytes = 0.0;

   TRACE();
   WaitForMutex(FilesDetailListMutex);
   Scan = var;
   if (Scan == NULL) Scan = Head;
   COUT(cout << "FilesDetailList::printDebug. TimeOut: " << TimeOut;)
   COUT(cout << " LastPruneTime: " << LastPruneTime << endl;)
   while (Scan != NULL) {
      TotalBytes = TotalBytes + Scan->FileSize;
      COUT(cout << i << " Nick: " << Scan->Nick << " Time: " << Scan->Time << " Update Count: " << Scan->UpdateCount << " Size: " << Scan->FileSize << " ResumeAt: " << Scan->FileResumePosition << " Queue: " << Scan->QueueNum << " Port: " << Scan->Port << " DownloadState: " << Scan->DownloadState << " ManualSend: " << Scan->ManualSend << " RetryCount: " << Scan->RetryCount << " NoReSend: " << Scan->NoReSend;)
      if (Scan->Connection) {
         COUT(cout << " Connection: " << Scan->Connection;)
         if (Scan->ConnectionMessage == CONNECTION_MESSAGE_NONE) {
            COUT(cout << " Message: None";)
         }
         else if (Scan->ConnectionMessage == CONNECTION_MESSAGE_DISCONNECT_REQUEUE_INCRETRY) {
            COUT(cout << " Message: DisConnect_ReQueue_IncRetry";)
         }
         else if (Scan->ConnectionMessage == CONNECTION_MESSAGE_DISCONNECT_NOREQUEUE) {
            COUT(cout << " Message: DisConnect_NoReQueue";)
         }
         else if (Scan->ConnectionMessage == CONNECTION_MESSAGE_DISCONNECT_REQUEUE_NOINCRETRY) {
            COUT(cout << " Message: DisConnect_ReQueue_NoIncRetry";)
         }
         else if (Scan->ConnectionMessage == CONNECTION_MESSAGE_DISCONNECT_DOWNLOAD) {
            COUT(cout << " Message: DisConnect_Download";)
         }
         else {
            COUT(cout << " Message: UnKnown";)
         }
      }
      if (Scan->DirName) {
         COUT(cout << " Dir: " << Scan->DirName;)
      }
      if (Scan->FileName) {
         COUT(cout << " File: " << Scan->FileName;)
         COUT(cout << " ServingDirIndex: " << Scan->ServingDirIndex;)
      }
      switch (Scan->TriggerType) {
        case XDCC:
          COUT(cout << " XDCC Pack # " << Scan->PackNum;)
          break;

        case FSERVMSG:
          if (Scan->TriggerName) {
             COUT(cout << " FSERVMSG Trigger: " << Scan->TriggerName;)
          }
          break;

        case FSERVCTCP:
          if (Scan->TriggerName) {
             COUT(cout << " FSERVCTCP Trigger: " << Scan->TriggerName;)
          }
          break;
      }
      if (Scan->DottedIP) {
         COUT(cout << " Dotted IP: " << Scan->DottedIP;)
      }
      if (Scan->PropagatedNick) {
          COUT(cout << " Propagated Nick: " << Scan->PropagatedNick;)
      }
      COUT(cout << endl;)

      i++;
      Scan = Scan->Next;
   }
   COUT(cout << "Summary: Total Files: " << i - 1 << " Total Size: " << TotalBytes / 1024 / 1024 / 1024 << " GB" << endl;)
   ReleaseMutex(FilesDetailListMutex);
}
