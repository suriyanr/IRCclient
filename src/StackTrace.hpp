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

#ifndef CLASS_STACKTRACE
#define CLASS_STACKTRACE

#include <signal.h>

// for getpid();
#include <sys/types.h>
#include <unistd.h>

#ifdef __MINGW32__
#  include <windows.h>
#endif


#if defined (__MINGW32__) || (defined(__APPLE__) && defined(__MACH__))
# define DO_NOT_HAVE_BACKTRACE
#else
# undef DO_NOT_HAVE_BACKTRACE
#endif

#if defined(DO_NOT_HAVE_BACKTRACE)
   long *getfp(int a);
   int backtrace(void *StackArray[], int max_depth);
#else
#  include <execinfo.h>
#endif

#ifndef __MINGW32__
#  include <pthread.h>
#  include <semaphore.h>
#endif

#include "HistoryLines.hpp"
#include "Compatibility.hpp"

#ifdef NO_STACKTRACE
#  define TRACE() ;
#  define TRACE_INIT() srand(getpid());
#else

// StackTrace Class is derived from HistoryLines.
// We add some functions and some macros so we can use it in our code
// for tracing function calls. This is where we capture the signals
// so we can print out details on a crash.

#define TRACE_LINES 1024

#ifndef MAX_CRASH_THREADS
  #define MAX_CRASH_THREADS  20
#endif

#define TRACE_FILE "MasalaMate" TRACE_DATE_STRING
#define TRACE() Tracer.addTrace(__FILE__, __FUNCTION__, __LINE__)

// Below needs to be placed in start of all new Threads.
// Including the main thread.
#define TRACE_INIT_NOCRASH() { Tracer.setSignals(); srand(getpid()); }

#define TRACE_INIT_CRASH(thread_name) { Tracer.setSignals(); srand(getpid()); Tracer.registerForCrash(thread_name);}

class StackTrace : public HistoryLines {
public:
   // This constructor will set the signal handlers
   StackTrace();
   ~StackTrace();
   void printTrace();
   void addTrace(const char *, const char *, int);
   void setSignals();
   void setTraceDir(char *);
   void registerForCrash(const char *ThreadName);

   THREADID  CrashThreadIDs[MAX_CRASH_THREADS];
   char *CrashThreadNames[MAX_CRASH_THREADS];

private:
   MUTEX StackTraceMutex;
   char *TraceDir;
};

// The Signal Handler.
static void gotCrash(int);

extern StackTrace Tracer;

#endif

#endif
