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

#ifdef NO_STACKTRACE
#  undef NO_STACKTRACE
#endif


#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "StackTrace.hpp"
#include "Compatibility.hpp"

//#ifndef __MINGW32__
//#include <execinfo.h>
//#endif

// Global which we will be used by the TRACE() macro.
StackTrace Tracer;

// Constructor.
StackTrace::StackTrace() {

#ifdef __MINGW32__
   StackTraceMutex = CreateMutex(NULL, FALSE, NULL);
#else
   pthread_mutex_init(&StackTraceMutex, NULL);
#endif

   // Already History Lines constructors have been called.
   setLines(TRACE_LINES);

   // Instead of deleting it lets just create the trace file in the
   // TraceDir. I expect this to be set and update at IRCclient init and
   // whenever Serving Dir is changed.
   // Aim is to get trace files in Serving Dir/Crash/trace file
   TraceDir = NULL;

   // Initialise CrashThreadIDs.
   memset(CrashThreadIDs, 0, sizeof(CrashThreadIDs));
   COUT(cout << "StackTrace:: sizeof CrashThreadIDs: " << sizeof(CrashThreadIDs) << endl;)
}

// Set the directory where the Traces need to go on a crash.
void StackTrace::setTraceDir(char *dir) {

   delete [] TraceDir;
   TraceDir = NULL;
   if ( (dir == NULL) || (strlen(dir) == 0) ) return;

   TraceDir = new char[strlen(dir) + 1];
   strcpy(TraceDir, dir);
}

// Each thread/including the main program needs to call this if it wants
// the whole program terminated if this thread crashes.
// Part of macro: TRACE_INIT_CRASH()
void StackTrace::registerForCrash(const char *ThreadName) {
int i;
THREADID tid;

   tid = GetCurrentThreadId();

   for (i = 0; i < MAX_CRASH_THREADS; i++) {
      if (CrashThreadIDs[i] == 0) {
         // Lets add this in this free slot.
         CrashThreadIDs[i] = tid;
         CrashThreadNames[i] = new char[strlen(ThreadName) + 1];
         strcpy(CrashThreadNames[i], ThreadName);

         COUT(cout << "StackTrace:: Registering thread: " << tid << " Name: " << CrashThreadNames[i] << " entry # " << i << " marked to crash on signal." << endl;)
         break;
      }
   }
}

// Each new thread needs to call this. Including the main program.
void StackTrace::setSignals() {
   signal(SIGABRT, gotCrash);
   signal(SIGILL,  gotCrash);
   signal(SIGFPE,  gotCrash);
   signal(SIGSEGV, gotCrash);

// Ignore these ones as we dont want to crash.
#ifndef __MINGW32__
   signal(SIGPIPE, SIG_IGN);
#endif
}

// Destructor
StackTrace::~StackTrace() {
   // Restore all the signals back to default.
   signal(SIGABRT, SIG_DFL);
   signal(SIGILL , SIG_DFL);
   signal(SIGFPE , SIG_DFL);
   signal(SIGSEGV, SIG_DFL);

   delete [] TraceDir;

   for (int i = 0; i < MAX_CRASH_THREADS; i++) {
      delete [] CrashThreadNames[i];
   }

   DestroyMutex(StackTraceMutex);
}

void StackTrace::addTrace(const char *file, const char *func, int line) {
char *cptr;

   WaitForMutex(StackTraceMutex);
   cptr = new char[strlen(file) + strlen(func) + 90];
   sprintf(cptr, "Thread: %lu File: %s Function: %s() Line: %d", GetCurrentThreadId(), file, func, line);
   addLine(cptr);
   delete [] cptr;
   ReleaseMutex(StackTraceMutex);
}

// Print the Trace to the trace file TRACE_FILE
void StackTrace::printTrace() {
FILE *fp = NULL;
char fname[MAX_PATH];

   WaitForMutex(StackTraceMutex);
   // Lets create the TraceDir if it doesnt exist yet.
   if (TraceDir) {
#ifdef __MINGW32__
      mkdir(TraceDir);
#else
      mkdir(TraceDir, S_IRWXU);
#endif
      sprintf(fname, "%s%s%s.%lu.trc", TraceDir, DIR_SEP, TRACE_FILE, GetCurrentThreadId());
      fp = fopen(fname, "w");
   }
   if (fp == NULL) {
      // Just try to write in the current directory.
      sprintf(fname, "%s.%lu.trc", TRACE_FILE, GetCurrentThreadId());
      fp = fopen(fname, "w");
   }
   COUT(cout << "fopen: " << fname << endl;)

   fprintf(fp, "%s %s %s\n", CLIENT_NAME_FULL, VERSION_STRING, DATE_STRING);
   // 1st line is the signal received message.
   fprintf(fp, "%s\n", HistoryArray[0]);
   fprintf(fp, "History of the last 1000 Traces is as below\n");

   for (int i = 1; i < Lines; i++) {
      if (HistoryArray[i]) {
         fprintf(fp, "[%3d] %s\n", i, HistoryArray[i]);
      }
   }
   fclose(fp);
   COUT(cout << "fclose:" << endl;)
   ReleaseMutex(StackTraceMutex);
}

// The signal Handler - gets called on a crash.
// calls printTrace to write out the file.
void gotCrash(int sig) {
char HeadLine[256];
#define MAX_STACK_DEPTH 25
void *StackArray[MAX_STACK_DEPTH + 1];
int stack_ret;
int i;

   signal(SIGABRT, SIG_DFL);
   signal(SIGILL , SIG_DFL);
   signal(SIGFPE , SIG_DFL);
   signal(SIGSEGV, SIG_DFL);

//#ifdef __MINGW32__
   // Our custom made hack for MINGW32
//   stack_ret = mingw32BackTrace(StackArray, MAX_STACK_DEPTH);
//#else
   // Use backtrace for Linux.
   stack_ret = backtrace(StackArray, MAX_STACK_DEPTH);
//#endif
   for (i = stack_ret; i >= 1; i--) {
      sprintf(HeadLine, "Thread: %lu Function[%d]: %10p", GetCurrentThreadId(), i, StackArray[i - 1]);
      Tracer.addLine(HeadLine);
   }

   for (i = 0; i < MAX_CRASH_THREADS; i++) {
      if (Tracer.CrashThreadIDs[i] != 0) {
         // Print out the Thread ID to Thread Name relation.
         sprintf(HeadLine, "Thread ID: %lu Name: %s", Tracer.CrashThreadIDs[i], Tracer.CrashThreadNames[i]);
         Tracer.addLine(HeadLine);
      }
   }

   switch(sig) {
     case SIGABRT:
        sprintf(HeadLine, "Thread %lu Received SIGABRT at %10p %10p %10p", GetCurrentThreadId(), __builtin_return_address(0), __builtin_return_address(1), __builtin_return_address(2));
        Tracer.addLine(HeadLine);
        break;

     case SIGILL:
        sprintf(HeadLine, "Thread %lu Received SIGILL at %10p %10p %10p", GetCurrentThreadId(), __builtin_return_address(0), __builtin_return_address(1), __builtin_return_address(2));
        Tracer.addLine(HeadLine);
        break;

     case SIGFPE:
        sprintf(HeadLine, "Thread %lu Received SIGFPE at %10p %10p %10p", GetCurrentThreadId(), __builtin_return_address(0), __builtin_return_address(1), __builtin_return_address(2));
        Tracer.addLine(HeadLine);
        break;

     case SIGSEGV:
        sprintf(HeadLine, "Thread %lu Received SIGSEGV at %10p %10p %10p", GetCurrentThreadId(), __builtin_return_address(0), __builtin_return_address(1), __builtin_return_address(2));
        Tracer.addLine(HeadLine);
        break;

   }

   Tracer.printTrace();

   // Will crash when we leave function. as we have set default handlers.
   // So exit gracefully.
   // Only when if this thread is present in CrashThreadIDs.
   // If its not, we ExitThread, and continue on as if nothing happened.
   // We already have dumped the trace.
   THREADID tid;
   tid = GetCurrentThreadId();
   for (i = 0; i < MAX_CRASH_THREADS; i++) {
      if (Tracer.CrashThreadIDs[i] == tid) {
         // This has to be crashed.
         COUT(cout << "StackTrace:: crashing program." << endl;)
         abort();
      }
   }

   // Not necessary to crash. just exit this thread.
   COUT(cout << "StackTrace:: Not crashing program." << endl;)
   int exit_code;
   ExitThread(exit_code);
}

#if defined(DO_NOT_HAVE_BACKTRACE)
// Below is for implementing a cursory stacktrace in MINGW32 and APPLE

long *getfp(int a) {
long *cur_fp;
long prev_fp;
long ret_address;

   cur_fp = (long *) &a; // address of 1st arg
   cur_fp--;
   ret_address = cur_fp[0] ; // Ret address = caller address.
   cur_fp--;
   prev_fp = cur_fp[0]; // Address of callers frame pointer.
   COUT(printf("getfp(): ret_address: %p prev_fp: %p\n", ret_address, prev_fp);)
   return((long *) prev_fp);
}

//int mingw32BackTrace(void *StackArray[], int max_depth) {
int backtrace(void *StackArray[], int max_depth) {
long *fp;
int retval = 0;

   fp = getfp(1);

   // We keep unwinding frame.
   while ((retval < max_depth) && fp && fp[1]) {

      StackArray[retval] = (void *) fp[1];
      retval++;

      COUT(printf("fp: %p, Ret Address: %p\n", fp, fp[1]);)
      fp = (long *) fp[0];
   }

   return(retval);
}
#endif // DO_NOT_HAVE_BACKTRACE
