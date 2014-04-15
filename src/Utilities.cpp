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

#include "Utilities.hpp"
#include "SHA1.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

// For getting version information.
#ifndef __MINGW32__
  #include <sys/utsname.h>
  #include <netdb.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
#endif


// Non class related utility/helper functions.
// Are independent of other classes. No relation with XChange etc.

// Returns the local dotted ip in ip. Assumes ip is allocated enough.
bool getInternalDottedIP(char *ip) {
bool retvalb = false;
char hname[160];
struct hostent *phe;
struct in_addr addr;

   TRACE();

   do {
      strcpy(ip, "127.0.0.1");
      if (gethostname(hname, sizeof(hname)) == -1) {
         break;
      }

      phe = gethostbyname(hname);
      if (phe == NULL) {
         break;
      }
      // We choose the first IP.
      if (phe->h_addr_list[0] == 0) {
         break;
      }
      memcpy(&addr, phe->h_addr_list[0], sizeof(struct in_addr));
      strcpy(ip, inet_ntoa(addr));
      COUT(cout << "Local IP: " << ip << endl;)
      retvalb = true;

   } while (false);

   return(retvalb);
}


// Returns the Directory Name from the full file path.
// The FullFileName is lost after this call.
void getDirName(char *FullFileName) {
int i, lastslash;

   TRACE();

   lastslash = 0;
   for (i = 0; i < strlen(FullFileName); i++) {
      if (FullFileName[i] == DIR_SEP_CHAR) {
         lastslash = i;
      }
   }
   FullFileName[lastslash] = '\0';
}

// Returns the FileName, from the full file path.
char *getFileName(char *FullFileName) {
int i, lastslash;

   TRACE();

   lastslash = -1;
   for (i = 0; i < strlen(FullFileName); i++) {
      if (FullFileName[i] == DIR_SEP_CHAR) {
         lastslash = i;
      }
   }

   return(FullFileName + lastslash + 1);
}

// Returns the FileName, from the full file path (In File Server notation)
char *getFileServerFileName(char *FullFileName) {
int i, lastslash;

   TRACE();

   lastslash = -1;
   for (i = 0; i < strlen(FullFileName); i++) {
      if (FullFileName[i] == FS_DIR_SEP_CHAR) {
         lastslash = i;
      }
   }

   return(FullFileName + lastslash + 1);
}

// This tries to generate a meaningful nick whose length is between
// 7 and 9 characters. nick is assumed allocated.
void generateNick(char *nick) {
int i;
long r, s;
int length;
char Letters[98] = {
 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
 'b',
 'c', 'c', 'c',
 'd', 'd', 'd', 'd',
 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e',
 'f', 'f',
 'g', 'g',
 'h', 'h', 'h', 'h', 'h', 'h',
 'i', 'i', 'i', 'i', 'i', 'i', 'i',
 'k',
 'l', 'l', 'l', 'l',
 'm', 'm',
 'n', 'n', 'n', 'n', 'n', 'n', 'n',
 'o', 'o', 'o', 'o', 'o', 'o', 'o',
 'p', 'p',
 'r', 'r', 'r', 'r', 'r', 'r',
 's', 's', 's', 's', 's', 's',
 't', 't', 't', 't', 't', 't', 't', 't', 't',
 'u', 'u', 'u',
 'v',
 'w', 'w',
 'y', 'y'
};

char Digraph[30][3]= {
 "at", "ar", "as", "an",
 "de",
 "er", "ed", "en", "es", "ea",
 "ha", "he",
 "in", "it", "io", "is",
 "le",
 "nd", "nt",
 "on", "of", "or", "ou",
 "re", "rt",
 "st",
 "th", "ti", "to",
 "ve"
};

   TRACE();

   r = (1 + (int) (3.0 * rand()/(RAND_MAX+1.0)));
   length = r + 6; // length varies from 7 to 9
   i = 0;
   nick[0] = '\0';
   while (i < length) {
      r = (1 + (int) (98.0 * rand()/(RAND_MAX+1.0)));
      r--;
      switch (Letters[r]) {
        case 'a':
          s = (1 + (int) (4.0 * rand()/(RAND_MAX+1.0)));
          s--;
          strcat(nick, Digraph[s]);
          i++;
          break;

        case 'd':
          strcat(nick, Digraph[4]);
          i++;
          break;

        case 'e':
          s = (1 + (int) (5.0 * rand()/(RAND_MAX+1.0)));
          s--;
          strcat(nick, Digraph[s + 5]);
          i++;
          break;
        case 'h':
          s = (1 + (int) (2.0 * rand()/(RAND_MAX+1.0)));
          s--;
          strcat(nick, Digraph[s + 10]);
          i++;
          break;

        case 'i':
          s = (1 + (int) (4.0 * rand()/(RAND_MAX+1.0)));
          s--;
          strcat(nick, Digraph[s + 12]);
          i++;
          break;

        case 'l':
          strcat(nick, Digraph[16]);
          i++;
          break;

        case 'n':
          s = (1 + (int) (2.0 * rand()/(RAND_MAX+1.0)));
          s--;
          strcat(nick, Digraph[s + 17]);
          i++;
          break;

        case 'o':
          s = (1 + (int) (4.0 * rand()/(RAND_MAX+1.0)));
          s--;
          strcat(nick, Digraph[s + 19]);
          i++;
          break;

        case 'r':
          s = (1 + (int) (2.0 * rand()/(RAND_MAX+1.0)));
          s--;
          strcat(nick, Digraph[s + 23]);
          i++;
          break;

        case 's':
          strcat(nick, Digraph[25]);
          i++;
          break;

        case 't':
          s = (1 + (int) (3.0 * rand()/(RAND_MAX+1.0)));
          s--;
          strcat(nick, Digraph[s + 26]);
          i++;
          break;

        case 'v':
          strcat(nick, Digraph[29]);
          i++;
          break;

        default:
          nick[i] = Letters[r];
          nick[i+1] = '\0';
          break;
      }
      i++;
   }
   // Lets randomly capitalise some letters.
   for (i = 0; i < strlen(nick); i++) {
      r = (1 + (int) (100.0 * rand()/(RAND_MAX+1.0)));
      if ( (r > 30) && (r < 60) ) {
         nick[i] = toupper(nick[i]);
      }
   }
}

// Get the size of the file given the full path.
// Returns true if file exists, and file_size is filled up
// Returns false if file doesnt exist.
bool getFileSize(char *filefullpath, size_t *file_size) {
struct stat s;
int retval;

   TRACE();

   *file_size = 0;
   retval = stat(filefullpath, &s);

   // file doesnt exist.
   if (retval == -1) return(false);

   *file_size = s.st_size;
   return(true);
}

// Delete the file given full path.
void delFile(char *filefullpath) {

   TRACE();

   unlink(filefullpath);
}


// Below is related to determining the ResumeFilePosition of a download.
// It creates a file if it doesnt exist, and returns resume as 0.
// If file size less than 8K = 8192, it makes file 0 size and resume 0.
// If file size > 8K, it sets resume position at current filesize - 8K
// The actual download will compare the bytes it receives to what the file
// already has to check if its resuming the correct file.
// Use File_RESUME_GAP instead of 8192.
size_t getResumePosition(char *filename) {
int fd;
struct stat s;
size_t res_pos;
int retval;

   TRACE();

   retval = stat(filename, &s);
   if (retval == -1) {
      // File doesnt exist, resume from 0.
      res_pos = 0;
   }
   else if (s.st_size < FILE_RESUME_GAP) {
   // If file exists with size < FILE_RESUME_GAP
      res_pos = 0;
   }
   else {
      res_pos = s.st_size - FILE_RESUME_GAP;
   }
   return(res_pos);
}

// Returns osversion with the OS version. osversion is assumed allocated
// and can hold the information requested.
void getOSVersionString(char *osversion) {

   TRACE();

   // default if we fail.
   strcpy(osversion, PLATFORM);

#ifdef __MINGW32__

OSVERSIONINFOEX osvi;
BOOL bOsVersionInfoEx;

   // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
   // If that fails, try using the OSVERSIONINFO structure.

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

   if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) ) {
      osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
      if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) {
         return;
      }
   }

   switch (osvi.dwPlatformId) {
      // Test for the Windows NT product family.
      case VER_PLATFORM_WIN32_NT:

      // Test for the specific product.
      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 ) {
         strcpy(osversion, "Windows Server 2003, ");
      }
      else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 ) {
         strcpy(osversion, "Windows XP ");
      }
      else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 ) {
         strcpy (osversion, "Windows 2000 ");
      }
      else if ( osvi.dwMajorVersion <= 4 ) {
         strcpy(osversion, "Microsoft Windows NT ");
      }

      // Test for specific product on Windows NT 4.0 SP6 and later.
      if ( bOsVersionInfoEx ) {
         // Test for the workstation type.
         if ( osvi.wProductType == VER_NT_WORKSTATION ) {
            if( osvi.dwMajorVersion == 4 ) {
               strcat(osversion, "Workstation 4.0 " );
            }
            else if( osvi.wSuiteMask & VER_SUITE_PERSONAL ) {
               strcat(osversion, "Home Edition " );
            }
            else {
               strcat(osversion, "Professional " );
            }
         }

         // Test for the server type.
         else if ( osvi.wProductType == VER_NT_SERVER || 
                   osvi.wProductType == VER_NT_DOMAIN_CONTROLLER ) {
            if (osvi.dwMajorVersion==5 && osvi.dwMinorVersion==2) {
               if ( osvi.wSuiteMask & VER_SUITE_DATACENTER ) {
                  strcat(osversion, "Datacenter Edition " );
               }
               else if ( osvi.wSuiteMask & VER_SUITE_ENTERPRISE ) {
                  strcat(osversion, "Enterprise Edition " );
               }
               else if ( osvi.wSuiteMask == VER_SUITE_BLADE ) {
                  strcat(osversion, "Web Edition " );
               }
               else {
                  strcat(osversion, "Standard Edition " );
               }
            }
            else if (osvi.dwMajorVersion==5 && osvi.dwMinorVersion==0) {
               if ( osvi.wSuiteMask & VER_SUITE_DATACENTER ) {
                  strcat(osversion, "Datacenter Server " );
               }
               else if ( osvi.wSuiteMask & VER_SUITE_ENTERPRISE ) {
                  strcat(osversion, "Advanced Server " );
               }
               else {
                  strcat(osversion, "Server " );
               }
            }
            else { // Windows NT 4.0 
               if ( osvi.wSuiteMask & VER_SUITE_ENTERPRISE ) {
                  strcat(osversion, "Server 4.0, Enterprise Edition " );
               }
               else {
                  strcat(osversion, "Server 4.0 " );
               }
            }
         }
      }
      // Test for specific product on Windows NT 4.0 SP5 and earlier
      else  
      {
         HKEY hKey;
         #define BUFSIZE 128
         char szProductType[BUFSIZE];
         DWORD dwBufLen=BUFSIZE;
         LONG lRet;

         lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
            0, KEY_QUERY_VALUE, &hKey );
         if( lRet != ERROR_SUCCESS )
            return;

         lRet = RegQueryValueEx( hKey, "ProductType", NULL, NULL,
            (LPBYTE) szProductType, &dwBufLen);
         if( (lRet != ERROR_SUCCESS) || (dwBufLen > BUFSIZE) )
            return;
         #undef BUFSIZE

         RegCloseKey( hKey );
         if ( lstrcmpi( "WINNT", szProductType) == 0 ) {
            strcat(osversion, "Workstation " );
         }
         else if ( lstrcmpi( "LANMANNT", szProductType) == 0 ) {
            strcat(osversion, "Server " );
         }
         else if ( lstrcmpi( "SERVERNT", szProductType) == 0 ) {
            strcpy(osversion, "Advanced Server " );
         }
         char version[30];
         sprintf(version, "%d.%d ", osvi.dwMajorVersion, osvi.dwMinorVersion );
         strcat(osversion, version);
      }

      // Display service pack (if any) and build number.

      char build[64];
      if( osvi.dwMajorVersion == 4 && 
          lstrcmpi( osvi.szCSDVersion, "Service Pack 6" ) == 0 )
      { 
         HKEY hKey;
         LONG lRet;

         // Test for SP6 versus SP6a.
         lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
  "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009",
            0, KEY_QUERY_VALUE, &hKey );
         if( lRet == ERROR_SUCCESS ) {
            sprintf(build, "Service Pack 6a (Build %d)", 
                    osvi.dwBuildNumber & 0xFFFF );
         }
         else // Windows NT 4.0 prior to SP6a
         {
            sprintf(build, "%s (Build %d)",
                    osvi.szCSDVersion,
                    osvi.dwBuildNumber & 0xFFFF);
         }
         RegCloseKey( hKey );
      }
      else // not Windows NT 4.0 
      {
         sprintf(build, "%s (Build %d)",
                 osvi.szCSDVersion,
                 osvi.dwBuildNumber & 0xFFFF);
      }
      strcat(osversion, build);

      break;

      // Test for the Windows Me/98/95.
      case VER_PLATFORM_WIN32_WINDOWS:

      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
      {
          strcpy(osversion, "Windows 95 ");
          if (osvi.szCSDVersion[1]=='C' || osvi.szCSDVersion[1]=='B') {
             strcat(osversion, "OSR2 " );
          }
      } 
      else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10) {
          strcpy(osversion, "Windows 98 ");
          if ( osvi.szCSDVersion[1] == 'A' ) {
             strcat(osversion, "SE " );
          }
      } 
      else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90) {
          strcpy(osversion, "Windows Millennium Edition");
      } 
      break;

      case VER_PLATFORM_WIN32s:

      strcpy(osversion, "Microsoft Win32s");
      break;
   }

#else

// Non Windows version information.
struct utsname name;

   if (uname(&name) == -1) return;

   sprintf(osversion, "%s %s %s",
                       name.sysname,
                       name.release,
                       name.machine);

#endif
}


#ifdef __MINGW32__
// Returns pointer in haystack where needle is present.
// Else NULL. Same as strstr(), but a case insensitive version.
char *strcasestr(char *haystack, char *needle) {
char *cap_haystack;
char *cap_needle;
char *cap_return;

   TRACE();

   if (needle == NULL) return(haystack);

   if (haystack == NULL) return(NULL);

   if (*needle == '\0') return(haystack);

   if (*haystack == '\0') return(NULL);

   cap_haystack = new char[strlen(haystack) + 1];
   cap_needle = new char[strlen(needle) + 1];

   for (int i = 0; i <= strlen(haystack); i++) {
      cap_haystack[i] = toupper(haystack[i]);
   }
   for (int i = 0; i <= strlen(needle); i++) {
      cap_needle[i] = toupper(needle[i]);
   }

   // Now return whatever strstr says.
   cap_return = strstr(cap_haystack, cap_needle);

   if (cap_return) {
      cap_return = haystack + (cap_haystack - cap_return);
   }

   delete [] cap_haystack;
   delete [] cap_needle;

   return(cap_return);
}
#endif


// Gets the FILE_RESUME_GAP bytes from full_filename before file_size.
// assumes buffer can hold FILE_RESUME_GAP of data.
// Assumes file is at least of size FILE_RESUME_GAP
// Return true if success, else false on failure.
bool getFileResumeChunk(char *full_fn, size_t file_size, char *buffer) {
size_t start_pos; 
int fd;
bool retvalb = false;

   TRACE();

   do {
      start_pos = file_size - FILE_RESUME_GAP;

      if (start_pos <= 0) break;

      fd = open(full_fn, O_RDONLY | O_BINARY);
      COUT(cout << "open: " << full_fn << " returned: " << fd << endl;)
      if (fd == -1) break;

      if (lseek(fd, start_pos, SEEK_SET) == -1) {
         close(fd);
         COUT(cout << "close: " << fd << endl;)
         break;
      }

      // Lets read in the bytes.
      if (read(fd, buffer, FILE_RESUME_GAP) == -1) {
         COUT(cout << "close: " << fd << endl;)
         close(fd);
         break;
      }

      close(fd);
      COUT(cout << "close: " << fd << endl;)
      retvalb = true;
   } while (false);

   return(retvalb);
}

#ifdef __MINGW32__

// Returns 0 if success. -1 on failure. like the unix version.
int truncate(char *filename, off_t size) {
HANDLE myfile;
int retval = 0;

   TRACE();

   myfile = CreateFile(filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, NULL);
   if (myfile == INVALID_HANDLE_VALUE) {
      return(-1);
   }
   if (SetFilePointer(myfile, size, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
      retval = -1;
      goto truncate_failed;
   }
   if (SetEndOfFile(myfile) == 0) {
      retval = -1;
      goto truncate_failed;
   }

truncate_failed:
   CloseHandle(myfile);
   return(retval);
}

#endif

// Returns the SHA string in char *sha of File of the bytes before file_size
// FILE_RESUME_GAP of bytes for SwarmFile verification.
// full_file_name is full path to use for open() call.
// Returns true on success, false otherwise.
bool getSHAOfSwarmFile(char *dir, char *file, size_t file_size, char *sha) {
char *fullpath;
CSHA1 SHA;
size_t file_pos;

   sha[0] = '\0';

   SHA.Reset();

   // Have to position it at right place.
   if (file_size > FILE_RESUME_GAP) {
      file_pos = file_size - FILE_RESUME_GAP;
   }
   else {
      file_pos = 0;
   }

   // Special case is when file_pos is 0 and file_size is 0.
   if ( (file_pos == 0) && (file_size == 0) ) {
      SHA.Update((UINT_8 *) sha, (UINT_32) 0);
   }
   else {
      fullpath = new char[strlen(dir) + strlen(file) + strlen(DIR_SEP) + 1];
      sprintf(fullpath, "%s%s%s", dir, DIR_SEP, file);

      // third  parameter is end_offset, hence, filesize - 1.
      SHA.HashFile(fullpath, file_pos, file_size - 1);

      delete [] fullpath;
   }

   SHA.Final();

   SHA.ReportHash(sha);

   return(true);
}

// Returns the SHA string in char *sha, of the running binary.
// Assume enough space in char *sha.
// full_file_name is full path to file.
// Returns true on success, false otherwise.
bool getSHAOfFile(char *full_file_name, char *sha) {
CSHA1 SHA;

   TRACE();

   sha[0] = '\0';
   SHA.Reset();

   SHA.HashFile(full_file_name, 0, 0);

   SHA.Final();
   SHA.ReportHash(sha);
   return(true);
}

// Will convert inplace, UNIX_DIR_SEP_CHAR to FS_DIR_SEP_CHAR
// Assumes filename is \0 terminated.
// Needed only by Non Windows Ports.
void convertToFSDIRSEP(char *filename) {
   TRACE();

   if (filename == NULL) return;

   for (int i = 0; ; i++) {
      if (filename[i] == '\0') break;
      if (filename[i] == UNIX_DIR_SEP_CHAR) {
         filename[i] = FS_DIR_SEP_CHAR;
      }
   }
}


// Will convert inplace, FS_DIR_SEP to DIR_SEP.
// Assumes filename is \0 terminated.
// Needed only by Non Windows Ports.
void convertToDIRSEP(char *filename) {
   TRACE();

   if (filename == NULL) return;

   for (int i = 0; ; i++) {
      if (filename[i] == '\0') break;
      if (filename[i] == FS_DIR_SEP_CHAR) {
         filename[i] = DIR_SEP_CHAR;
      }
   }
}

// Converts the filesize to a human readable string.
// Assumes s_filesize has enough space to fit it.
void convertFileSizeToString(size_t filesize, char *s_filesize) {

   TRACE();

   if (filesize < 1024) {
      sprintf(s_filesize, "%liB", filesize);
      return;
   }
   filesize = filesize / 1024;
   if (filesize < 1024) {
      sprintf(s_filesize, "%liKB", filesize);
      return;
   }
   filesize = filesize / 1024;
   if (filesize < 1024) {
      sprintf(s_filesize, "%liMB", filesize);
      return;
   }
   filesize = filesize / 1024;
   if (filesize < 1024) {
      sprintf(s_filesize, "%liGB", filesize);
      return;
   }
   filesize = filesize / 1024;
   sprintf(s_filesize, "%liTB", filesize);
   return;
}

// Converts the time to a human readable string.
// Assumes s_time has enough space to fit it.
void convertTimeToString(time_t t_time_left, char *s_time_left) {
int i_days, i_hours, i_minutes, i_seconds;
char tmpstr[32];

   TRACE();

   // Lets convert this to some sensible time string.
   s_time_left[0] = '\0';
   i_days = t_time_left / 86400;
   if (i_days) {
      sprintf(tmpstr, "%d d ", i_days);
      strcat(s_time_left, tmpstr);
   }
   t_time_left = t_time_left - i_days * 86400;
   i_hours = t_time_left / 3600;
   if (i_hours) {
      sprintf(tmpstr, "%d h ", i_hours);
      strcat(s_time_left, tmpstr);
   }
   t_time_left = t_time_left - i_hours * 3600;
   i_minutes = t_time_left / 60;
   if (i_minutes) {
      sprintf(tmpstr, "%d m ", i_minutes);
      strcat(s_time_left, tmpstr);
   }
   t_time_left = t_time_left - i_minutes * 60;
   i_seconds = t_time_left;
   if (i_seconds) {
      sprintf(tmpstr, "%d s ", i_seconds);
      strcat(s_time_left, tmpstr);
   }

   // Lets remove the ' ' if present at the end of the string.
   int string_len = strlen(s_time_left);
   if (string_len && (s_time_left[string_len - 1] == ' ')) {
      s_time_left[string_len - 1] = '\0';
   }
}

// The Last String is a NULL String.
void deleteStringArray(char **StringArray) {
int index;

   TRACE();

   if (StringArray == NULL) return;

   index = 0;
   while (StringArray[index]) {
      delete [] StringArray[index];
      index++;
   }
   delete [] StringArray;
}

// Get number of entries in String Array.
int getEntriesInStringArray(char **StringArray) {
int index = 0;

   TRACE();

   if (StringArray == NULL) return(index);

   while (StringArray[index]) {
      index++;
   }
   return(index);
}

// Bit Manipulation routines.
// Bit numbering starts from 0.
bool setBitInByteArray(char *ByteArray, int bitnum) {
int ArrayIndex;
int IndexInByte;
char mask = 0x01;

   TRACE();

   ArrayIndex = bitnum / BITS_PER_BYTE;
   IndexInByte = bitnum % BITS_PER_BYTE;
   mask = mask << IndexInByte;
   ByteArray[ArrayIndex] = ByteArray[ArrayIndex] | mask;

   return(true);
}

bool clrBitInByteArray(char *ByteArray, int bitnum) {
int ArrayIndex;
int IndexInByte;
char mask = 0x01;

   TRACE();

   ArrayIndex = bitnum / BITS_PER_BYTE;
   IndexInByte = bitnum % BITS_PER_BYTE;
   mask = mask << IndexInByte;
   mask = ~mask;
   ByteArray[ArrayIndex] = ByteArray[ArrayIndex] & mask;

   return(true);
}

bool isSetBitInByteArray(char *ByteArray, int bitnum) {
bool retvalb;
int ArrayIndex;
int IndexInByte;
char mask = 0x01;

   TRACE();

   ArrayIndex = bitnum / BITS_PER_BYTE;
   IndexInByte = bitnum % BITS_PER_BYTE;
   mask = mask << IndexInByte;
   retvalb = ByteArray[ArrayIndex] & mask;

   return(retvalb);
}

// Assumes StringArray has enough space to hold the string + terminating char.
// = MaxBitLen + 1 characters.
bool convertByteArrayToString(char *ByteArray, int MaxBitLen, char *StringArray) {
int ArrayIndex;
int IndexInByte;
char mask;

   TRACE();

   for (int i = 0; i < MaxBitLen; i++) {
      mask = 0x01;
      ArrayIndex = i / BITS_PER_BYTE;
      IndexInByte = i % BITS_PER_BYTE;
      mask = mask << IndexInByte;
      if (ByteArray[ArrayIndex] & mask) {
         StringArray[i] = '1';
      }
      else {
         StringArray[i] = '0';
      }
   }
   StringArray[MaxBitLen] = '\0';

   return(true);
}

