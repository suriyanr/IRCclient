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

#include <errno.h>
#endif

#include <stdio.h>
#include <string.h>
#include "LineQueue.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

#ifdef USE_NAMED_SEMAPHORE
int LineQueue::UseNamedSemaphore = 0;
#endif

// Constructor.
LineQueue::LineQueue() {
   TRACE();
   Head = NULL;
   Tail = NULL;

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
   int sem_ret = sem_init(&LineQueueSem, 0, 0);
   if (sem_ret == -1) {
      COUT(cout << "CRITICAL: sem_init returned -1: errno: " << errno << endl;)
   }
#endif // USE_NAMED_SEMAPHORES
#endif
}

// Destructor.
LineQueue::~LineQueue() {
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
bool LineQueue::isEmpty() {
bool retvalb;

   TRACE();

   WaitForMutex(LineQueueMutex);

   if (Head == NULL) retvalb = true;
   else retvalb = false;

   ReleaseMutex(LineQueueMutex);

   return(retvalb);
}

// Return the FIFO Line. Doesnt effect SEM, as line is not consumed.
void LineQueue::getLine(char *buffer) {
   TRACE();

   // Do a Semaphore DOWN operation. So we wait till data is available.
   WaitForSemaphore(LineQueueSem);

   WaitForMutex(LineQueueMutex);

   if (Head != NULL) {
      strcpy(buffer, Head->Line);
   }
   else {
      buffer[0] = '\0';
   }

   ReleaseMutex(LineQueueMutex);

   // Do a Semaphore UP operation. As we havent consumed line.
   ReleaseSemaphore(LineQueueSem, 1, NULL);
}

// Get the line and delete it. Effects MUTEX and SEM
void LineQueue::getLineAndDelete(char *buffer) {
LineHolder *tmpptr;

   TRACE();

   // Do a Semaphore DOWN operation. So we wait till data is available.
   WaitForSemaphore(LineQueueSem);
   WaitForMutex(LineQueueMutex);

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

// delete the FIFO Line.
void LineQueue::delLine() {
LineHolder *tmpptr;

   TRACE();

   WaitForSemaphore(LineQueueSem);
   WaitForMutex(LineQueueMutex);
   if (Head != NULL) {
      delete [] Head->Line;
      tmpptr = Head->Next;
      delete Head;
      Head = tmpptr;
   }
   ReleaseMutex(LineQueueMutex);
}

// Add a Line to the FIFO. Effects MUTEX and SEM.
void LineQueue::putLine(char *varline) {

   TRACE();

   WaitForMutex(LineQueueMutex);
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

#if 0
// Flushes the Queue. No more lines in it. Resets Semaphore to 0 too.
// On analysis, this call should be called only by the consumer in our case.
// The writer should not call flush, as ther eis a chance of a whack.
// writer only ups the semaphore, all down events should be handled by
// consumer.
void LineQueue::flush() {
LineHolder *tmp_ptr;

   TRACE();

   WaitForMutex(LineQueueMutex);

   // Delete the whole list. We get out of the while(), when Head is NULL
   while (Head) {
      // Reduce the semaphore count.
      ReleaseSemaphore(LineQueueMutex, 1, NULL);

      delete [] Head->Line;
      tmp_ptr = Head->Next;
      delete Head;
      Head = tmp_ptr;
   }

   // Note, that previously we used to destroy the semaphore and recreate
   // it to initialise it to 0. That is wrong. Cause note that, the other
   // threads are waiting on that semaphore which we will delete.
   // Once deleted, it appears to them as a successful down operation.
   // and they proceed to get the erroneous/non exitent line and possibly
   // crash

   ReleaseMutex(LineQueueMutex);
   return;
}
#endif

void LineQueue::printDebug() {
LineHolder *tmp_ptr;
int i = 1;

   TRACE();
   WaitForMutex(LineQueueMutex);
   tmp_ptr = Head;
   while (tmp_ptr != NULL) {
      COUT(cout << i++ << " " << tmp_ptr->Line << endl;)
      tmp_ptr = tmp_ptr->Next;
   }
   ReleaseMutex(LineQueueMutex);
}

