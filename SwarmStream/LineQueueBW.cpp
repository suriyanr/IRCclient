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
using namespace std;

#include <string.h>
#include "LineQueueBW.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

#ifdef USE_NAMED_SEMAPHORE
int LineQueueBW::UseNamedSemaphore = 0;
#endif

// Constructor.
LineQueueBW::LineQueueBW() {
   TRACE();
   Head = NULL;
   Tail = NULL;
   UploadBPS = 0;
   DownloadBPS = 0;
   DataBytesIn = 0;
   DataBytesOut = 0;
   ControlBytesIn = 0;
   ControlBytesOut = 0;
   UploadBufferLength = 16384;

   // Initialise the MUTEX and the SEM
#ifdef __MINGW32__
   LineQueueMutex = CreateMutex(NULL, FALSE, NULL);
   LineQueueSem = CreateSemaphore(NULL, 0, LINEQUEUE_MAXLINES, NULL);
#else
   pthread_mutex_init(&LineQueueMutex, NULL);
#ifdef USE_NAMED_SEMAPHORE
   char sem_name[64];
   sprintf(sem_name, "%s%d", CLIENT_NAME, UseNamedSemaphore);
   sem_number = UseNamedSemaphore;
   UseNamedSemaphore++;
   LineQueueSem = sem_open(sem_name, O_CREAT, S_IRWXU, 0);
   if (LineQueueSem == NULL) {
      COUT(cout << "CRITICAL: sem_open returned NULL" << endl;)
   }
#else
   sem_init(&LineQueueSem, 0, 0);
#endif // USE_NAMED_SEMAPHORES
#endif
}

// Destructor.
LineQueueBW::~LineQueueBW() {
   TRACE();
   while (Head != NULL) {
      Tail = Head->Next;
      delete [] Head->Line;
      delete Head;
      Head = Tail;
   }

   // Destroy the MUTEX and the SEM
   DestroyMutex(LineQueueMutex);
#ifdef USE_NAMED_SEMAPHORE
   char sem_name[64];
   sprintf(sem_name, "%s%d", CLIENT_NAME, sem_number);
   sem_unlink(sem_name);
#else
   DestroySemaphore(LineQueueSem);
#endif
}

// Returns true if no lines present in Queue, false otherwise.
bool LineQueueBW::isEmpty() {
bool retvalb;

   TRACE();

   WaitForMutex(LineQueueMutex);

   if (Head == NULL) retvalb = true;
   else retvalb = false;

   ReleaseMutex(LineQueueMutex);

   return(retvalb);
}

// Get the line and delete it. Effects MUTEX and SEM
void LineQueueBW::getLineAndDelete(char *buffer) {
LineHolder *tmpptr;

   TRACE();

   // Do a Semaphore DOWN operation. So we wait till data is available.
   WaitForSemaphore(LineQueueSem);
   WaitForMutex(LineQueueMutex);

   // Calculate the msecs it would take to get the data down the wire.
   if (DownloadBPS != 0) {
      unsigned long msecs = (1000 * strlen(buffer)) / DownloadBPS;
      msleep(msecs);
   }

   // Add to statistics.
   if (strlen(buffer) > 100) {
      DataBytesIn += strlen(buffer);
   }
   else {
      ControlBytesIn += strlen(buffer);
   }

   if (Head != NULL) {
      strcpy(buffer, Head->Line);
      delete [] Head->Line;
      tmpptr = Head->Next;
      delete Head;
      Head = tmpptr;
   }
   else {
      buffer[0] = '\0';
   }
   ReleaseMutex(LineQueueMutex);
}

// Add a Line to the FIFO. Effects MUTEX and SEM.
// Once its issued, we should sleep the amount of time it would take
// to send that much data out depending on our upload BW.
// Before adding check if we have enough space to add it, governed by
// UploadBufferLength
void LineQueueBW::putLine(char *varline) {
size_t size_of_line;
size_t cur_buf_len;

   TRACE();

   size_of_line = strlen(varline);
   do {
      cur_buf_len = getLengthOfQueuedData();
      if ((size_of_line + cur_buf_len) < UploadBufferLength) break;
      sleep(1);
   } while (true);

   WaitForMutex(LineQueueMutex);
   // Calculate the msecs it would take to send down the wire.
   if (UploadBPS != 0) {
      unsigned long msecs = (1000 * strlen(varline)) / UploadBPS;
      msleep(msecs);
   }

   // Add to statistics.
   if (strlen(varline) > 100) {
      DataBytesOut += strlen(varline);
   }
   else {
      ControlBytesOut += strlen(varline);
   }

   if (Head == NULL) {
      Head = new LineHolder;
      Head->Next = NULL;
      Head->Line = new char[strlen(varline) + 1];
      strcpy(Head->Line, varline);
      Tail = Head;
      ReleaseMutex(LineQueueMutex);
      ReleaseSemaphore(LineQueueSem, 1, NULL);
      return;
   }

// At least 1 element is present in FIFO. Add to Tail.
   Tail->Next = new LineHolder;
   Tail = Tail->Next;
   Tail->Next = NULL;
   Tail->Line = new char[strlen(varline) + 1];
   strcpy(Tail->Line, varline);
   ReleaseMutex(LineQueueMutex);
   ReleaseSemaphore(LineQueueSem, 1, NULL);
   return;
}

size_t LineQueueBW::getLengthOfQueuedData() {
LineHolder *tmp_ptr;
size_t size_of_data = 0;

   WaitForMutex(LineQueueMutex);

   tmp_ptr = Head;
   while (tmp_ptr != NULL) {
      size_of_data += strlen(tmp_ptr->Line);
      tmp_ptr = tmp_ptr->Next;
   }
   ReleaseMutex(LineQueueMutex);

   return(size_of_data);
}

void LineQueueBW::printStatistics() {

   TRACE();
   cout << "LineQueueBW Statistics: Data In: " << DataBytesIn << " Data Out: " << DataBytesOut << " Control In: " << ControlBytesIn << " Control Out: " << ControlBytesOut << endl;

}

void LineQueueBW::printDebug() {
LineHolder *tmp_ptr;
int i = 1;

   TRACE();
   WaitForMutex(LineQueueMutex);
   COUT(cout << "LineQueueBW: UploadBPS: " << UploadBPS << " DownloadBPS: " << DownloadBPS << " Upload Buf Len: " << UploadBufferLength << endl;)
   tmp_ptr = Head;
   while (tmp_ptr != NULL) {
      COUT(cout << i++ << " " << tmp_ptr->Line << endl;)
      tmp_ptr = tmp_ptr->Next;
   }
   ReleaseMutex(LineQueueMutex);
}

void LineQueueBW::setUploadBPS(size_t bps) {

   TRACE();
   WaitForMutex(LineQueueMutex);
   UploadBPS = bps;
   ReleaseMutex(LineQueueMutex);
}

void LineQueueBW::setDownloadBPS(size_t bps) {

   TRACE();
   WaitForMutex(LineQueueMutex);
   DownloadBPS = bps;
   ReleaseMutex(LineQueueMutex);
}

void LineQueueBW::setUploadBufferLength(size_t buf_len) {
   TRACE();

   WaitForMutex(LineQueueMutex);
   UploadBufferLength = buf_len;
   ReleaseMutex(LineQueueMutex);
}
