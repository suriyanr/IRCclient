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

#ifndef CLASS_LINEQUEUE
#define CLASS_LINEQUEUE

#ifdef __MINGW32__
#  include <windows.h>
#else
#  include <pthread.h>
#  include <semaphore.h>
#endif

#include "Compatibility.hpp"

#define LINEQUEUE_MAXLINES 1000

class LineQueue {
public:
   LineQueue();
   ~LineQueue();
   void getLine(char *varline);
   void delLine();
   void putLine(char *varline);
   void getLineAndDelete(char *varline);
   bool isEmpty();
#if 0
   void flush();
#endif
   void printDebug();

private:
   typedef struct LineHolder {
      char *Line;
      struct LineHolder *Next;
   } LineHolder;
   LineHolder *Head;
   LineHolder *Tail;

   // Mutex and semaphores
   MUTEX LineQueueMutex;
   SEM   LineQueueSem;
#ifdef USE_NAMED_SEMAPHORE
   int sem_number;
   static int UseNamedSemaphore;
#endif
};


#endif
