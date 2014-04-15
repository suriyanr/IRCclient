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

#ifndef CLASS_LEAKTRACER
#define CLASS_LEAKTRACER

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


#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#ifdef __MINGW32__
   #include <winsock2.h>
   #include <ws2tcpip.h>
#else
   #include <pthread.h>
#endif

#include "Compatibility.hpp"

// File used for LeakTrace Output
#define LEAKTRACE_FILE "MasalaMate.memory"

/*
 * underlying allocation, de-allocation used within 
 * this tool
 */
#define LT_MALLOC  malloc
#define LT_FREE    free
#define LT_REALLOC realloc

/*
 * prime number for the address lookup hash table.
 * if you have _really_ many memory allocations, use a
 * higher value, like 343051 for instance.
 */
//#define SOME_PRIME 35323
#define SOME_PRIME 343051
#define ADDR_HASH(addr) ((unsigned long) addr % SOME_PRIME)

/**
 * allocate a bit more memory in order to check if there is a memory
 * overwrite. Either 0 or more than sizeof(unsigned int). Note, you can
 * only detect memory over_write_, not _reading_ beyond the boundaries. Better
 * use electric fence for these kind of bugs 
 *   <ftp://ftp.perens.com/pub/ElectricFence/>
 */
#define MAGIC "\x01\x23\x45\x67\x89\xAB\xCD\xEF"
#define MAGIC_SIZE (sizeof(MAGIC)-1)

/**
 * on 'new', initialize the memory with this value.
 * if not defined - uninitialized. This is very helpful because
 * it detects if you initialize your classes correctly .. if not,
 * this helps you faster to get the segmentation fault you're 
 * implicitly asking for :-). 
 *
 * Set this to some value which is likely to produce a
 * segmentation fault on your platform.
 */
#define SAVEVALUE   0xAA

/**
 * on 'delete', clean memory with this value.
 * if not defined - no memory clean.
 *
 * Set this to some value which is likely to produce a
 * segmentation fault on your platform.
 */
#define MEMCLEAN    0xEE

/**
 * Initial Number of memory allocations in our list.
 * Doubles for each re-allocation.
 */
#define INITIALSIZE 32768

static class LeakTracer {
	struct Leak {
		const void *addr;
		size_t      size;
		const void *allocAddr;
		bool        type;
		int         nextBucket;
	};
	
	int  newCount;      // how many memory blocks do we have
	int  leaksCount;    // amount of entries in the leaks array
	int  firstFreeSpot; // Where is the first free spot in the leaks array?
	int  currentAllocated; // currentAllocatedMemory
	int  maxAllocated;     // maximum Allocated
	unsigned long totalAllocations; // total number of allocations. stats.
	unsigned int  abortOn;  // resons to abort program (see abortReason_t)

	/**
	 * Have we been initialized yet?  We depend on this being
	 * false before constructor has been called!  
	 */
	bool initialized;	
	bool destroyed;		// Has our destructor been called?


	FILE *report;       // filedescriptor to write to

	/**
	 * pre-allocated array of leak info structs.
	 */
	Leak *leaks;

	/**
	 * fast hash to lookup the spot where an allocation is 
	 * stored in case of an delete. map<void*,index-in-leaks-array>
	 */
	int  *leakHash;     // fast lookup

	MUTEX mutex;

	enum abortReason_t {
		OVERWRITE_MEMORY    = 0x01,
		DELETE_NONEXISTENT  = 0x02,
		NEW_DELETE_MISMATCH = 0x04
	};

        void initialize();
public:
	LeakTracer();
	/*
	 * the workhorses:
	 */
	void *registerAlloc(size_t size, bool type);
	void  registerFree (void *p, bool type);

	/**
	 * write a hexdump of the given area.
	 */
	void  hexdump(const unsigned char* area, int size);
	
	/**
	 * Terminate current running progam.
	 */
	void progAbort(abortReason_t reason);

	/**
	 * write a Report over leaks, e.g. still pending deletes
	 */
	void writeLeakReport();

	~LeakTracer();
} leakTracer;

#endif
