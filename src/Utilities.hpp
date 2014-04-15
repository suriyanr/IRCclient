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

#ifndef CLASS_UTILITIES
#define CLASS_UTILITIES

#include <sys/types.h>

// Non class related utility/helper functions.
// Are independent of other classes. No relation with XChange etc.

// Converts the time to a human readable string.
// Assumes s_time has enough space to fit it.
void convertTimeToString(time_t time, char *s_time);

// Converts the filesize to a human readable string.
// Assumes s_filesize has enough space to fit it.
void convertFileSizeToString(size_t filesize, char *s_filesize);

// Returns the local dotted ip in ip. Assumes ip is allocated enough.
bool getInternalDottedIP(char *ip);

// Returns the Directory Name from the full file path.
// The FullFileName is lost after this call.
void getDirName(char *FullFileName);

// Returns the FileName, from the full file path.
char *getFileName(char *FullFileName);

// Returns the Windows FileName, from the file path (File Server notation)
// Required particulary in non windows.
char *getFileServerFileName(char *FullFileName);

// This tries to generate a meaningful nick whose length is between
// 7 and 9 characters. nick is assumed allocated.
void generateNick(char *nick);

// Below is related to determining the ResumeFilePosition of a download.
// It creates a file if it doesnt exist, and returns resume as 0.
// If file size less than 8K = 8192, it makes file 0 size and resume 0.
// If file size > 8K, it sets resume position at current filesize - 8K
// The actual download will compare the bytes it receives to what the file
// already has to check if its resuming the correct file.
// Use File_RESUME_GAP instead of 8192.
size_t getResumePosition(char *filename);

// Get the size of the file given the full path.
// Returns true if file exists, and file_size is filled up
// Returns false if file doesnt exist.
bool getFileSize(char *filefullpath, size_t *file_size);

// Delete the file given full path.
void delFile(char *filefullpath);

// Gets the FILE_RESUME_GAP bytes from full_filename before file_size.
// assumes buffer can hold FILE_RESUME_GAP of data.
// Assumes file is at least of size FILE_RESUME_GAP
// Return true if success, else false on failure.
bool getFileResumeChunk(char *full_fn, size_t file_size, char *buffer);

// Returns osversion with the OS version. osversion is assumed allocated
// and can hold the information requested.
void getOSVersionString(char *osversion);

#ifdef __MINGW32__
// Returns pointer in haystack where needle is present.
// Else NULL. Same as strstr(), but a case insensitive version.
char *strcasestr(char *haystack, char *needle);

// Returns 0 if success. -1 on failure. like the unix version.
int truncate(char *filename, off_t size);
#endif

// Returns the SHA string in char *sha, of the running binary.
// Assume enough space in char *sha.
// full_file_name is full path to use for open() call
// Returns true on success, false otherwise.
bool getSHAOfFile(char *full_file_name, char *sha);

// We get SHA of last FILE_RESUME_GAP of File or less if size is less.
bool getSHAOfSwarmFile(char *dir, char *file, size_t file_size, char *sha);

// Will convert inplace, DIR_SEP to FS_DIR_SEP.
void convertToFSDIRSEP(char *filename);

// Will convert inplace, FS_DIR_SEP to DIR_SEP.
void convertToDIRSEP(char *filename);

// Manipulating String Array functions.

// The Last String is a NULL String.
void deleteStringArray(char **StringArray);

// Returns the number of char pointers in this string Array
int getEntriesInStringArray(char **StringArray);

// Bit Manipulation routines.
// Bit numbering starts from 0.
bool setBitInByteArray(char *ByteArray, int bitnum);
bool clrBitInByteArray(char *ByteArray, int bitnum);
bool isSetBitInByteArray(char *ByteArray, int bitnum);
bool convertByteArrayToString(char *ByteArray, int ArrayLen, char *StringArray);

#endif
