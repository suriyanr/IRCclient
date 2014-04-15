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

// Copied from below:
/* 
 * Homepage: <http://www.andreasen.org/LeakTracer/>
 *
 * Authors:
 *  Erwin S. Andreasen <erwin@andreasen.org>
 *  Henner Zeller <H.Zeller@acm.org>
 *
 * This program is Public Domain
 */

#include "LeakTracer.hpp"

// Constructor
LeakTracer::LeakTracer() {
   initialize();
}
	
void LeakTracer::initialize() {
   // Unfortunately we might be called before our constructor has actualy fired
   if (initialized) {
      return;
   }

   // fprintf(stderr, "LeakTracer::initialize()\n");
   initialized = true;
   newCount = 0;
   leaksCount = 0;
   firstFreeSpot = 1; // index '0' is special
   currentAllocated = 0;
   maxAllocated = 0;
   totalAllocations = 0;
//   abortOn =  OVERWRITE_MEMORY; // only _severe_ reason
   abortOn = OVERWRITE_MEMORY | DELETE_NONEXISTENT | NEW_DELETE_MISMATCH;
   report = 0;
   leaks = 0;
   leakHash = 0;

   const char *filename = LEAKTRACE_FILE;
   report = fopen(filename, "w+");
   if (report ==  NULL) {
      fprintf(stderr, "LeakTracer: cannot open %s: %m\n", filename);
      report = stderr;
   }

   time_t t = time(NULL);
   fprintf (report, "# starting %s", ctime(&t));

   leakHash = (int*) LT_MALLOC(SOME_PRIME * sizeof(int));
   memset ((void*) leakHash, 0x00, SOME_PRIME * sizeof(int));

#ifdef MAGIC
   fprintf (report, "# memory overrun protection of %d Bytes\n", MAGIC_SIZE);
#endif
		
#ifdef SAVEVALUE
   fprintf (report, "# initializing new memory with 0x%2X\n", SAVEVALUE);
#endif

#ifdef MEMCLEAN
   fprintf (report, "# sweeping deleted memory with 0x%2X\n", MEMCLEAN);
#endif
   // if (getenv("LT_ABORTREASON")) {
      // abortOn = atoi(getenv("LT_ABORTREASON"));
   // }

#define PRINTREASON(x) if (abortOn & x) fprintf(report, "%s ", #x);
   fprintf (report, "# aborts on ");
   PRINTREASON( OVERWRITE_MEMORY );
   PRINTREASON( DELETE_NONEXISTENT );
   PRINTREASON( NEW_DELETE_MISMATCH );
   fprintf (report, "\n");
#undef PRINTREASON

   fprintf (report, "# thread safe\n");
// Assume the Mutex is created.
#ifdef __MINGW32__
   mutex = CreateMutex(NULL, FALSE, NULL);
#else
   pthread_mutex_init(&mutex, NULL);
#endif
   fflush(report);
}
	
/*
 * the workhorses:
 */

/**
 * Terminate current running progam.
 */
void LeakTracer::progAbort(abortReason_t reason) {
   if (abortOn & reason) {
      fprintf(report, "# abort; DUMP of current state\n");
      fprintf(stderr, "LeakTracer aborting program\n");
      writeLeakReport();
      fclose(report);
      abort();
   }
   else {
      fflush(report);
   }
}


// Destructor
LeakTracer::~LeakTracer() {
   // fprintf(stderr, "LeakTracer::destroy()\n");
   time_t t = time(NULL);
   fprintf (report, "# finished %s", ctime(&t));
   writeLeakReport();
   fclose(report);
   free(leaks);
   DestroyMutex(mutex);
   destroyed = true;
}

void* LeakTracer::registerAlloc (size_t size, bool type) {
   initialize();

   // fprintf(stderr, "LeakTracer::registerAlloc()\n");

   if (destroyed) {
      fprintf(stderr, "Oops, registerAlloc called after destruction of LeakTracer (size=%d)\n", size);
      return LT_MALLOC(size);
   }

   void *p = LT_MALLOC(size + MAGIC_SIZE);
   // Need to call the new-handler
   if (!p) {
      fprintf(report, "LeakTracer malloc %m\n");
      _exit(1);
   }

#ifdef SAVEVALUE
   /* initialize with some defined pattern */
   memset(p, SAVEVALUE, size + MAGIC_SIZE);
#endif
	
#ifdef MAGIC
   /*
    * the magic value is a special pattern which does not need
    * to be uniform.
    */
   memcpy((char*)p+size, MAGIC, MAGIC_SIZE);
#endif

   WaitForMutex(mutex);

   ++newCount;
   ++totalAllocations;
   currentAllocated += size;
   if (currentAllocated > maxAllocated) {
      maxAllocated = currentAllocated;
   }

   for (;;) {
      for (int i = firstFreeSpot; i < leaksCount; i++) {
         if (leaks[i].addr == NULL) {
            leaks[i].addr = p;
            leaks[i].size = size;
            leaks[i].type = type;
            leaks[i].allocAddr=__builtin_return_address(1);
            firstFreeSpot = i+1;
            // allow to lookup our index fast.
            int *hashPos = &leakHash[ ADDR_HASH(p) ];
            leaks[i].nextBucket = *hashPos;
            *hashPos = i;
            ReleaseMutex(mutex);
            return p;
         }
      }

      // Allocate a bigger array
      // Note that leaksCount starts out at 0.
      int new_leaksCount = (leaksCount == 0) ? INITIALSIZE : leaksCount * 2;
      leaks = (Leak*)LT_REALLOC(leaks, sizeof(Leak) * new_leaksCount);
      if (!leaks) {
         fprintf(report, "# LeakTracer realloc failed: %m\n");
         _exit(1);
      }
      else {
         fprintf(report, "# internal buffer now %d\n", new_leaksCount);
         fflush(report);
      }
      memset(leaks+leaksCount, 0x00, sizeof(Leak) * (new_leaksCount-leaksCount));
      leaksCount = new_leaksCount;
   }
}

void LeakTracer::hexdump(const unsigned char* area, int size) {
   fprintf(report, "# ");
   for (int j=0; j < size ; ++j) {
      fprintf (report, "%02x ", *(area+j));
      if (j % 16 == 15) {
         fprintf(report, "  ");
         for (int k=-15; k < 0 ; k++) {
            char c = (char) *(area + j + k);
            fprintf (report, "%c", isprint(c) ? c : '.');
         }
         fprintf(report, "\n# ");
      }
   }
   fprintf(report, "\n");
}

void LeakTracer::registerFree (void *p, bool type) {
   initialize();

   if (p == NULL) {
      return;
   }

   if (destroyed) {
//      fprintf(stderr, "Oops, allocation destruction of LeakTracer (p=%p)\n", p);
      const char *filename = LEAKTRACE_FILE;
      report = fopen(filename, "a");

      fprintf(report,
              "I %10p %10p  # Destroyed - Ignore ptr if listed as Leak\n",
              __builtin_return_address(1), p);
      fclose(report);

      return;
   }

   WaitForMutex(mutex);
   int *lastPointer = &leakHash[ ADDR_HASH(p) ];
   int i = *lastPointer;

   while (i != 0 && leaks[i].addr != p) {
      lastPointer = &leaks[i].nextBucket;
      i = *lastPointer;
   }

   if (leaks[i].addr == p) {
      *lastPointer = leaks[i].nextBucket; // detach.
      newCount--;
      leaks[i].addr = NULL;
      currentAllocated -= leaks[i].size;
      if (i < firstFreeSpot) {
         firstFreeSpot = i;
      }

      if (leaks[i].type != type) {
         fprintf(report, 
                 "S %10p %10p  # new%s but delete%s "
                 "; size %d\n",
                 leaks[i].allocAddr,
                 __builtin_return_address(1),
                 ((!type) ? "[]" : " normal"),
                 ((type) ? "[]" : " normal"),
                 leaks[i].size);

         progAbort( NEW_DELETE_MISMATCH );
      }
#ifdef MAGIC
      if (memcmp((char*)p + leaks[i].size, MAGIC, MAGIC_SIZE)) {
         fprintf(report, "O %10p %10p  "
                 "# memory overwritten beyond allocated"
                 " %d bytes\n",
                 leaks[i].allocAddr,
                 __builtin_return_address(1),
                 leaks[i].size);
                 fprintf(report, "# %d byte beyond area:\n",
                 MAGIC_SIZE);
         hexdump((unsigned char*)p+leaks[i].size, MAGIC_SIZE);
         hexdump((unsigned char*)p, leaks[i].size + MAGIC_SIZE);
         progAbort( OVERWRITE_MEMORY );
      }
#endif

#ifdef MEMCLEAN
      int allocationSize = leaks[i].size;
#endif
      ReleaseMutex(mutex);

#ifdef MEMCLEAN
      // set it to some garbage value.
      memset((unsigned char*)p, MEMCLEAN, allocationSize + MAGIC_SIZE);
#endif
      LT_FREE(p);
      return;
   }

   ReleaseMutex(mutex);
   fprintf(report, "D %10p             # delete non alloc or twice pointer %10p\n", 
           __builtin_return_address(1), p);
   progAbort( DELETE_NONEXISTENT );
}


void LeakTracer::writeLeakReport() {
   initialize();

   if (newCount > 0) {
      fprintf(report, "# LeakReport\n");
      fprintf(report, "# %10s | %9s  # Pointer Addr\n",
                      "from new @", "size");
   }
   for (int i = 0; i <  leaksCount; i++) {
      if (leaks[i].addr != NULL) {
         // This ought to be 64-bit safe?
         fprintf(report, "L %10p   %9ld  # %p\n",
                 leaks[i].allocAddr,
                 (long) leaks[i].size,
                 leaks[i].addr);
      }
   }
   fprintf(report, "# total allocation requests: %6ld ; max. mem used"
           " %d kBytes\n", totalAllocations, maxAllocated / 1024);
   fprintf(report, "# leak %6d Bytes\t:-%c\n", currentAllocated,
           (currentAllocated == 0) ? ')' : '(');
   if (currentAllocated > 50 * 1024) {
      fprintf(report, "# .. that is %d kByte!! A lot ..\n", 
              currentAllocated / 1024);
   }
}


/** -- The actual new/delete operators -- **/

// Overloaded new operator.
void* operator new(size_t size) {
   return leakTracer.registerAlloc(size,false);
}

// Overloaded new[] operator.
void* operator new[] (size_t size) {
   return leakTracer.registerAlloc(size,true);
}

// Overloaded delete operator.
void operator delete (void *p) {
   leakTracer.registerFree(p,false);
}

// Overloaded delete[] operator.
void operator delete[] (void *p) {
   leakTracer.registerFree(p,true);
}
