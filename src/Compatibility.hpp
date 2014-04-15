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

#ifndef CLASS_COMPATIBILITY
#define CLASS_COMPATIBILITY

#ifdef __MINGW32__

#  define PIPE HANDLE
#  define WritePipe WriteFile
#  define ReadPipe  ReadFile
#  define ClosePipe CloseHandle
#  define sleep(x) Sleep(1000 * x)
#  define msleep(x) Sleep(x)
#  define fdatasync _commit
#  define THREADID DWORD
#  define MUTEX HANDLE
#  define SEM HANDLE
#  define THR_HANDLE HANDLE
#  define THR_EXITCODE DWORD
#  define WaitForMutex(mutex) WaitForSingleObject(mutex, INFINITE)
#  define WaitForSemaphore(sem) WaitForSingleObject(sem, INFINITE)
#  define WaitForThread(thread) WaitForSingleObject(thread, INFINITE)
#  define DestroyMutex(mutex) CloseHandle(mutex)
#  define DestroySemaphore(sem) CloseHandle(sem)
#  define KillThread(thread) TerminateThread(thread, (DWORD) 1)
#  define DIR_SEP "\\"
#  define DIR_SEP_CHAR '\\'
#  define CREAT_PERMISSIONS ( S_IRUSR | S_IWUSR )

#else // Non Windows.

#  include <unistd.h>
#  include <signal.h>
#  define PIPE int
#  define WritePipe(fd, buf, len, a, b) write(fd, buf, len)
#  define ReadPipe(fd, buf, len, a, b) read(fd, buf, len)
#  define ClosePipe close
#  define msleep(x) usleep(1000 * x)
#  define DWORD int
#  define THREADID pthread_t
#  define MUTEX pthread_mutex_t
#  define SEM sem_t
#  define GetCurrentThreadId pthread_self
#  define THR_HANDLE pthread_t
#  define THR_EXITCODE int
#  define SOCKET int
#  define ExitThread(exitcode) pthread_exit(&exitcode)
#  define ReleaseMutex(mutex) pthread_mutex_unlock(&mutex)
#  define ReleaseSemaphore(sem, a, b) sem_post(&sem)
#  define WaitForMutex(mutex) pthread_mutex_lock(&mutex)
#  define WaitForSemaphore(sem) sem_wait(&sem)
#  define WaitForThread(thread) pthread_join(thread, NULL)
#  define DestroyMutex(mutex) pthread_mutex_destroy(&mutex)
#  define DestroySemaphore(sem) sem_destroy(&sem)
#  define KillThread(thread) pthread_kill(thread, (int) SIGTERM)
#  define DIR_SEP "/"
#  define DIR_SEP_CHAR '/'
#  define CREAT_PERMISSIONS ( S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH )
#  ifndef O_BINARY
#    define O_BINARY 0
#  endif
#  define closesocket close
#  define MAX_PATH PATH_MAX

#endif

// Define the ENDIANness
#ifdef __MINGW32__
  #define __LITTLE_ENDIAN 1234
  #define __BYTE_ORDER __LITTLE_ENDIAN
#else
  #if defined(__APPLE__) && defined(__MACH__)
    #if defined(__i386__) || defined(__i386_64__)
       #define __LITTLE_ENDIAN 1234
       #define __BYTE_ORDER __LITTLE_ENDIAN
    #endif
    #if defined(__ppc__) || defined(__ppc64__)
       #define __BIG_ENDIAN 4321
       #define __BYTE_ORDER __BIG_ENDIAN
    #endif
  #else
    #include <endian.h>
  #endif
#endif

// To set PLATFORM and UPGRADE_PROGRAM_NAME for each port.
#if defined(WIN32)
  #define PLATFORM               "WinX86_32"
  #define UPGRADE_PROGRAM_NAME   UPGRADE_PROGRAM_NAME_WIN_X86_32
#endif
#if defined(WIN64)
  #define PLATFORM               "WinX86_64"
     #define UPGRADE_PROGRAM_NAME   UPGRADE_PROGRAM_NAME_WIN_X86_64
#endif

// Above we have set PLATFORM only for Windows.
#if defined(linux)
  #if defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)
    #define PLATFORM                "LinuxX86_32"
    #define UPGRADE_PROGRAM_NAME    UPGRADE_PROGRAM_NAME_LINUX_X86_32
  #endif
  #if defined(__x86_64__)
    #undef PLATFORM
    #undef UPGRADE_PROGRAM_NAME
    #define PLATFORM                "LinuxX86_64"
    #define UPGRADE_PROGRAM_NAME    UPGRADE_PROGRAM_NAME_LINUX_X86_64
  #endif
  #if defined(__powerpc__)
    #if defined(__powerpc64__)
      #define PLATFORM              "LinuxPPC_64"
      #define UPGRADE_PROGRAM_NAME  UPGRADE_PROGRAM_NAME_LINUX_PPC_64
    #else
      #define PLATFORM              "LinuxPPC_32"
      #define UPGRADE_PROGRAM_NAME  UPGRADE_PROGRAM_NAME_LINUX_PPC_32
    #endif
  #endif
  
#endif

// Apple Macintosh related weirdness.
#if defined(__APPLE__) && defined(__MACH__)
  #ifdef __ppc__
  #  define PLATFORM               "ApplePPC_32"
  #  define UPGRADE_PROGRAM_NAME   UPGRADE_PROGRAM_NAME_APPLE_PPC_32
  #endif
  #ifdef __i386__
  #  define PLATFORM               "AppleX86_32"
  #  define UPGRADE_PROGRAM_NAME   UPGRADE_PROGRAM_NAME_APPLE_X86_32
  #endif
  #ifdef __i386_64__
  #  undef PLATFORM
  #  undef UPGRADE_PROGRAM_NAME
  #  define PLATFORM               "AppleX86_64"
  #  define UPGRADE_PROGRAM_NAME   UPGRADE_PROGRAM_NAME_APPLE_X86_64
  #endif
  #ifdef __ppc64__
  #  undef PLATFORM
  #  undef UPGRADE_PROGRAM_NAME
  #  define PLATFORM               "ApplePPC_64"
  #  define UPGRADE_PROGRAM_NAME   UPGRADE_PROGRAM_NAME_APPLE_PPC_64
  #endif
#  define fdatasync fsync
   // Only named semaphores.
#  define USE_NAMED_SEMAPHORE
#  undef SEM
#  define SEM sem_t *
#  undef ReleaseSemaphore
#  define ReleaseSemaphore(sem, a, b) sem_post(sem)
#  undef WaitForSemaphore
#  define WaitForSemaphore(sem) sem_wait(sem)
#  undef DestroySemaphore
#endif

#ifdef COUT_OUTPUT
# define COUT(a) a
#else
# define COUT(a) ;
#endif

// Below is Client related defines needed by most cpp files.

// Client Type defines.
#define IRCNICKCLIENT_UNKNOWN    ' '
#define IRCNICKCLIENT_MASALAMATE 'M'
#define IRCNICKCLIENT_SYSRESET   'S'
#define IRCNICKCLIENT_IROFFER    'I'

#define DATE_STRING       "Nov 17th 2005"
#define TRACE_DATE_STRING "Nov1705"
#define VERSION_STRING    "Beta"
#define CLIENT_NAME       "MasalaMate"
#define CLIENT_NAME_FULL  CLIENT_NAME " " PLATFORM
#define CLIENT_HTTP_LINK  "http://groups.yahoo.com/group/masalamate"
#define NETWORK_UI_NAME   "IRCSuper Network"
#define NETWORK_NAME      "IRCSuper"
#define FSERV_MAX_QUEUES_OVERALL    100
#define FSERV_MIN_QUEUES_OVERALL    10
#define FSERV_MIN_SENDS_OVERALL     2
#define FSERV_MIN_QUEUES_USER       1
#define FSERV_MIN_SENDS_USER        1
#define FSERV_SMALL_FILE_MAX_SIZE       200*1024*1024    // 200MB
#define FSERV_SMALL_FILE_DEFAULT_SIZE   60*1024*1024     // 60 MB
//#define CHANNEL_MAIN      "#MMTest"
#define CHANNEL_MAIN      "#IndianMasala"
#define CHANNEL_MAIN_KEY  ""
#define CHANNEL_CHAT      "#Masala-Chat"
#define CHANNEL_MM        CHANNEL_MAIN "MM"
#define CHANNEL_MM_KEY    "__Super_MM__"
#define FILE_RESUME_GAP   8192
#define DCCSERVER_PORT    8124

// Below needed for uniform FileServer interface across platforms.
// FFLC also needs to send a consistent seperator.
#define UNIX_DIR_SEP_CHAR '/'
#define WIN_DIR_SEP_CHAR  '\\'
#define UNIX_DIR_SEP      "/"
#define WIN_DIR_SEP       "\\"
#define FS_DIR_SEP_CHAR   WIN_DIR_SEP_CHAR
#define FS_DIR_SEP        WIN_DIR_SEP

// Below needed for the binary names for the upgrade.
#define UPGRADE_OP_NICK                      "UpgradeMM"
#define UPGRADE_DIR                          "Upgrade"
#define PROGRAM_NAME_WINDOWS                 "MasalaMate.exe"
#define PROGRAM_NAME_NON_WINDOWS             "MasalaMate"

// Make the upgrade names consistent - OS_PROCESSOR_BITMODE
// These names exist in an array in TabBookWindows::onUpgradeServer()
// So please update that array as new names are added here.
#define UPGRADE_PROGRAM_NAME_WIN_X86_32      "MasalaMate.WIN_X86_32"
#define UPGRADE_PROGRAM_NAME_WIN_X86_64      "MasalaMate.WIN_X86_64"
#define UPGRADE_PROGRAM_NAME_LINUX_X86_32    "MasalaMate.LINUX_X86_32"
#define UPGRADE_PROGRAM_NAME_LINUX_X86_64    "MasalaMate.LINUX_X86_64"
#define UPGRADE_PROGRAM_NAME_APPLE_PPC_32    "MasalaMate.APPLE_PPC_32"
#define UPGRADE_PROGRAM_NAME_APPLE_PPC_64    "MasalaMate.APPLE_PPC_64"
#define UPGRADE_PROGRAM_NAME_APPLE_X86_32    "MasalaMate.APPLE_X86_32"
#define UPGRADE_PROGRAM_NAME_APPLE_X86_64    "MasalaMate.APPLE_X86_64"
#define UPGRADE_PROGRAM_NAME_LINUX_PPC_32    "MasalaMate.LINUX_PPC_32"
#define UPGRADE_PROGRAM_NAME_LINUX_PPC_64    "MasalaMate.LINUX_PPC_64"

// Below defines the minimum CAP value that can be used.
#define MIN_CAP_VALUE_BPS                    2048

// Max number of Serving Directories that can be set.
#define FSERV_MAX_SERVING_DIRECTORIES        4

// How many bytes do we need transferred to be considered for recording
// the speed at which it was transferred in the RecordBPS
// Set as 5 MB for now.
#define MIN_BYTES_TRANSFERRED_FOR_RECORD     5242880

// All downloads, flush to disk adter receiving so many bytes.
// Set as 5 MB for now.
#define DOWNLOAD_FLUSHFILE_SIZE              5242880

// As of now we support only two Swarms
#define SWARM_MAX_FILES                      2
#define SWARM_CONNECTION_TIMEOUT             180
#define SWARM_DATAREQUEST_TIMEOUT            180
#ifndef IRCNICKIP_UNKNOWN
  #define IRCNICKIP_UNKNOWN                  0
#endif
#define SWARM_MAX_CONNECTED_NODES            48
#define SWARM_MAX_FUTURE_HOLES               SWARM_MAX_CONNECTED_NODES

// Each hole can take 3 states = not requested/requested/got.
// Each byte the first bit and second bit (offset 0/1) is always set.
// Hence each byte can hold information for 3 holes.
// So for 48 holes we need 16 bytes.
#define SWARM_FUTURE_HOLE_BYTE_ARRAY_LEN     ((SWARM_MAX_FUTURE_HOLES / 3) + 1)

#define BITS_PER_BYTE                        8

#endif
