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

#ifndef CLASS_FILESDETAILLIST
#define CLASS_FILESDETAILLIST

#ifdef __MINGW32__
#  include <windows.h>
#else
#  include <pthread.h>
#  include <semaphore.h>
#endif


// This Class maintains the database of all files information and how
// to get a file which are being served in channel
#include "Compatibility.hpp"

#include "TriggerType.hpp"
#include "TCPConnect.hpp"

typedef struct FilesDetail {
   int  ServingDirIndex;     // Used by MyFilesDB, to know the base dir.
   char *FileName;
   char *DottedIP;           // Used by DCC FServ and DCC Send
#define MANUALSEND_NONE          '\0'
#define MANUALSEND_DCCSEND       'D'
#define MANUALSEND_FILEPUSH      'P'
   char ManualSend;          // Used by DCC Send and File Push
   int  RetryCount;          // Written by Transfer/Read by TimerThr
   bool NoReSend;            // Used on receiving a CTCP NoReSend
   short Port;               // Used by DCC Resume.
   size_t FileSize;
   time_t Time;
   char *Nick;
   char *PropagatedNick;    // Used by Propagation Algo.
   TriggerTypeE TriggerType;
   char *TriggerName;
   char *DirName;
   long PackNum;
   long QueueNum;

   int  UpdateCount;

// File Server information
// Compatibility defines the Client Types as:
// #define IRCNICKCLIENT_UNKNOWN    ' '
// #define IRCNICKCLIENT_MASALAMATE 'M'
// #define IRCNICKCLIENT_SYSRESET   'S'
// #define IRCNICKCLIENT_IROFFER    'I'
   char ClientType;
   int  CurrentSends;
   int  TotalSends;
   int  CurrentQueues;
   int  TotalQueues;

// This is for the File Resume position in case of SEND/RECEIVE/GET in FServ
   size_t FileResumePosition;

// Below is for TCPConnect related information.
// BytesSent, BytesReceived, Born are copied over from the Connection.
// So that when Connection pointer is destroyed, someone accessing those
// fields in TCPConnect doesnt SEGV
// We also copy over UploadBps and DownloadBps (20 second rollover)
   TCPConnect *Connection;
#define CONNECTION_MESSAGE_NONE                           '\0'
#define CONNECTION_MESSAGE_DISCONNECT_REQUEUE_INCRETRY    'I'
#define CONNECTION_MESSAGE_DISCONNECT_NOREQUEUE           'Q'
#define CONNECTION_MESSAGE_DISCONNECT_REQUEUE_NOINCRETRY  'N'
#define CONNECTION_MESSAGE_DISCONNECT_DOWNLOAD            'D'
#define CONNECTION_MESSAGE_DISCONNECT_FSERV               'F'
   char   ConnectionMessage;
   size_t BytesSent;
   size_t BytesReceived;
   time_t Born;
   size_t UploadBps;
   size_t DownloadBps;

// Below is for status of a download which is over. partial or success.
// TransferThr uses this to set it to 'P' or 'S', which is interpreted
// when updating the UI.
   #define DOWNLOADSTATE_NONE        '\0'
   #define DOWNLOADSTATE_SERVING     'S'
   #define DOWNLOADSTATE_PARTIAL     'P'
   char DownloadState;

   // Below is the additional data for this entry.
   // Currently used as an array of strings by DwnldWaiting Queue, which is
   // populated in the Waiting TAB of UI.
   void *Data;

   struct FilesDetail *Next;
} FilesDetail;

// Default prune TimeOut value = 0 => 
#define FILES_DETAIL_LIST_PRUNE 0

// Prune no more than once in 10 seconds.
#define FILES_DETAIL_LIST_DELTA_PRUNE 1 // prune once in 1 second.

// The Update Count when it exceed this value, cause the FilesDetail Entry
// to be discarded.
#define FILES_DETAIL_LIST_UPDATE_ROLLOVER 10

// This defines the variation allowed in file size to be considered the
// same file.
#define FILES_DETAIL_LIST_SIZEDELTA 2048000

class FilesDetailList {
public:
   FilesDetailList();
   ~FilesDetailList();

// To initialise a FilesDetail structure after allocation.
   void initFilesDetail(FilesDetail *);

   // Compare function to compare two FDs passed in.
   // The compare criteria can be one of
   #define FD_COMPARE_FILENAME   0
   #define FD_COMPARE_FILESIZE   1
   #define FD_COMPARE_NICKNAME   2
   #define FD_COMPARE_DIRNAME    3
   #define FD_COMPARE_SENDS      4
   #define FD_COMPARE_QUEUES     5
   #define FD_COMPARE_CLIENT     6
   // descending = true => reverse the comparison answers.
   int compareFilesDetail(int criteria, bool descending, FilesDetail *a, FilesDetail *b);

// We assume that FilesDetail and all its pointers are for us to keep.
// So caller should not delete it.
// It can also be a list of FilesDetail to be added to our list.
   void addFilesDetail(FilesDetail *);

   bool isFilesOfNickPresent(const char *nick);
   bool isFilesDetailPresent(FilesDetail *);
   bool isNickPresent(const char *nick);

// Checks if the given FileName is already present in the list.
   bool isPresentMatchingFileName(const char *file);

   // Checks if the given PropagatedNick is already present in the list.
   bool isPresentMatchingPropagatedNick(const char *pro_nick);

// Checks if the given Nick, FileName is already present in the list.
// Returns the index in the list if present.
// 0 => Not in list.
   int isPresentMatchingNickFileName(const char *nick, const char *filename);

// To delete matching FileName and 
// In the event that a nick changes its nickname to another.
   void renameNickToNewNick(char *oldnick, char *newnick);

// In the event that a nick is kicked or parted or quit we remove
// its entry from the list.
   void delFilesOfNick(char *nick);

// To selectively delete just the Serving / Partial filelist of nick.
// Used by FFLC
   void delFilesOfNickByDownloadState(char *nick, char dstate);

// To selectivelly get the list of files with a particular DownloadState.
// Used by FFLC.
   FilesDetail *getFilesOfNickByDownloadState(char *nick, char dstate);

// Delete the FilesDetail structure from the list.
   void delFilesDetail(FilesDetail *);

// del FilesList from the List which match FileName and Nick.
   void delFilesDetailNickFile(char *nick, char *file);

// update FilesList from the List which match FileName and Nick with Q.
   void updateFilesDetailNickFileQ(char *nick, char *file, long q);

// update FilesList from the List which match FileName and Nick with Data.
   void updateFilesDetailNickFileData(char *nick, char *file, void *data);

// update FilesList from the List which match Nick with NoReSend.
   void updateFilesDetailNickNoReSend(char *nick, bool noresend);

// update FilesList from the List which match FileName and Nick with TCPConnect
// Used by DCCServerThr and TimerThr, to update the TCPConnect.
void updateFilesDetailNickFileConnection(char *nick, char *file, TCPConnect *);

// update FilesList from the List which match FileName and Nick with 
// the ConnectionMessage. Used by DwnldInitThr, TabBookWindow
// Returns true if update was done, else false
bool updateFilesDetailNickFileConnectionMessage(char *nick, char *file, char);

// update FilesList from the List which match Nick (all matches of nick)
// the ConnectionMessage. Used by FromServerThr
// Returns true if update was done, else false
bool updateFilesDetailAllNickConnectionMessage(char *nick, char);

// update all FilesDetail in List with the Connection Message.
// Used to terminate on Quitting the application.
// Returns true if update was done, else false
bool updateFilesDetailAllConnectionMessage(char);

// update FilesList from the List which match FileName and Nick with TCPConnect
// Used by Transfer class, to update the TCPConnect.
// Returns the Connection Message. Used by Transfer::updateFilesDetail
char updateFilesDetailNickFileConnectionDetails(char *nick, char *file, size_t bytes_sent, size_t bytes_recvd, time_t born, size_t upload_bps, size_t dwnld_bps);

// update FilesList from the List which match FileName and Nick with ResumePos
// Used by TimerThr, to update FileResumePosition
void updateFilesDetailNickFileResume(char *nick, char *file, size_t ResumePos);

// Used to update all files held by nick to current time. This helps in
// not accessing the trigger too frequently. This updates the update count too.
// On the 10th update it actually deletes the files of nick.
// As this will be called just before accessing the nick's trigger, it will
// work out as planned.
   void updateTimeFilesOfNick(char *nick);

// Used to update the UpdateCount to a particular value of all entries
// nick. used by the FFLC version 2 algo.
   void updateUpdateCountOfNick(char *nick, int newUpdateCount);

// Used to generate the checksum of the entries held by nick.
// For Serving and Partial Files.
// filenamelens is the sum of the lengths of the DirNames and FileNames.
// counts = number of files. (first pair for serving, second for partial)
   void getCheckSums(char *nick, long *filedirnamelens, long *counts, long *filedirnamelenp, long *countp);

// Assumes chksum is (4 * size of long) + 1 bytes long and allocated by caller.
// It generates hex string by
// totalfiledirnamelens (size of long) in hex = 2 * size of long
// counts (size of long) in hex = 2 * size of long
// On converting the leading 0's are removed so its more compact.
   void getCheckSumString(long filedirnamelen, long count, char *chksum);

// Used to update all files held by nick to the send and q information
// passed.
   void updateSendsQueuesOfNick(char *nick, int cur_sends, int tot_sends, int cur_queues, int tot_queues);

// The pointer returned by below needs to be deleted seperately by caller.
// The SearchString is compared only with matching filenames.
// and it matches as a substring.
// Result is in ascending order of filename, with bigger file sizes first.
// The ascending order is valid only when Sort is set appropriately.
// If CountIntPtr is not NULL, then it returns the count in it.
   FilesDetail *searchFilesDetailList(const char *SearchString, int *CountIntPtr = NULL);

// Returns an intelligent copy of the structure passed in.
   FilesDetail *copyFilesDetail(FilesDetail *);

// Returns the list which match the DirName as a substring.
// NULL Dir, or strlen(Dir) == 0 => lists all.
// Used mainly by FileServer.
   FilesDetail *searchDirFilesDetailList(char *SearchDir);

// Returns the list with approximate repetitions removed.
// repeat = same filename (case insensitive) approxi file size (within 2 MB)
// The returned pointer is the new List Head. Assumes list is in sorted order.
   FilesDetail *removeRepetitionsFilesDetailList(FilesDetail *);

// Returns a list containing same FileName and FileSizes (within 2MB)
// The caller needs to free the List returned.
   FilesDetail *getFilesDetailListMatchingFileAndSize(char *filename, size_t varsize);

// Returns a list containing the Files of Nick. if CountIntPtr is not NULL,
// it will return the count in it.
// The caller needs to free the List returned.
   FilesDetail *getFilesDetailListOfNick(const char *nick, int *CountIntPtr = NULL);

// Returns a list which match the Nick and FileName.
// The caller needs to free the List returned.
   FilesDetail *getFilesDetailListNickFile(char *nick, char *filename);

// Returns a list which match the Nick and FileName and DirName.
// DirName = ' ' => NULL.
// The caller needs to free the List returned.
   FilesDetail *getFilesDetailListNickFileDir(char *nick, char *filename, char *dirname);

// Returns a list containing the Files of DottedIP
// The caller needs to free the List returned.
   FilesDetail *getFilesDetailListOfDottedIP(char *dottedip);

// Returns the first FD which matches the given FileName and DottedIP.
// The caller needs to free the FD returned.
   FilesDetail *getFilesDetailFileNameDottedIP(char *filename, char *dottedip);

// del FilesList from the List which match FileName and DottedIP.
   void delFilesDetailFileNameDottedIP(char *filename, char *dottedip);

// del FilesList from the list which match DottedIP.
    void delFilesDetailDottedIP(char *dottedip);

// Returns a list which exactly match the FileName.
// The caller needs to free the List returned.
   FilesDetail *getFilesDetailListMatchingFileName(char *filename);

// Returns a copy of FilesDetail at index.
// The caller needs to free the FilesDetail Returned.
   FilesDetail *getFilesDetailAtIndex(int index);

// Insert the FilesDetail at index.
// Caller has allocated the FilesDetail to be added at index.
   void addFilesDetailAtIndex(FilesDetail *, int index);

// Returns the count of the FilesDetail in the List.
// If NULL passed, it gets count using the Head.
   int getCount(FilesDetail *);

// Returns the count and sum of FileSize of all entries in the list.
// If NULL passed, it gets count and size using the Head.
   int getCountFileSize(FilesDetail *, double *TotalFileSize);

// Returns the count of the FilesDetail in the List.
// If NULL passed, it gets count using the Head.
// This gets the count of the entries held by varnick, "No Files Present"
// entries for FileNames are not counted. TriggerTemplate is counted.
   int getFileCountOfNick(FilesDetail *, char *varnick);

// Set the TimeOut value for pruning.
   void setTimeOut(time_t);

   // Sort Flag. Defaults to no sorting. set to FILELIST_NO_SORT for Queues.
   // '\0' or 'N' => no sorting.
   // 'F' => sort only by FileName.
   // 'D' => sort considering both DirName and FileName.
   // Only MyFilesDB uses FILELIST_SORT_BY_DIR_FILENAME
   // #define FILELIST_NO_SORT              'N'
   // #define FILELIST_SORT_BY_FILENAME     'F'
   // #define FILELIST_SORT_BY_DIR_FILENAME 'D'

   void setSort(char);

   void freeFilesDetailList(FilesDetail *);

// Clear the list.
   void purge();

   void printDebug(FilesDetail *);

private:
   FilesDetail *Head;
   void pruneOldFiles(bool compulsory);

   // TimeOut for pruning the list.
   time_t TimeOut;

   // Last prune Time.
   time_t LastPruneTime;

   // Sort Flag. Defaults to no sorting. set to false for Queues.
   // '\0' or 'N' => no sorting.
   // 'F' => sort only by FileName.
   // 'D' => sort considering both DirName and FileName.
   #define FILELIST_NO_SORT              'N'
   #define FILELIST_SORT_BY_FILENAME     'F'
   #define FILELIST_SORT_BY_DIR_FILENAME 'D'
   char Sort;

// A mutex to serialise its access as it will exist in XChange Class
// and a seperate mutex wont be required to control it there.
   MUTEX FilesDetailListMutex;
};

#endif
