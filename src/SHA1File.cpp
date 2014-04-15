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

#include "SHA1File.hpp"

#include "LineParse.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

// Constructor.
SHA1File::SHA1File() {
   TRACE();
   initialized = false;
   SHA1MetaInfo = NULL;
}

// Destructor.
SHA1File::~SHA1File() {
   TRACE();
   delete [] SHA1MetaInfo;
}

// Init the SHA1 info of fname and .fname. Checks if it exists.
// Corrects if it is wrong, creates if it doesnt exist.
bool SHA1File::initFile(char *dirname, char *filename) {
bool retvalb;

   TRACE();
   retvalb = checkSHA1OfFile(dirname, filename);
   if (retvalb == false) {
      retvalb = generateSHA1OfFile(dirname, filename);
   }
   initialized = retvalb;
}

// Used to check the SHA1 infomation as contained in .fname
// returns true if check is OK. returns false, if not OK => generate.
// Basically checks PayloadSHA1 and FileName and payload size
bool SHA1File::checkSHA1OfFile(char *dirname, char *fname) {
char *metafname = NULL;
struct stat mystat;
int retval;
FILE *fp = NULL;
int fd = -1;
char *inputline = NULL;
LineParse LineP;
const char *parseptr;
char *endptr;
size_t fsize, actfsize;
size_t to_read, cur_index;

   TRACE();
   initialized = false;

   metafname = new char[strlen(dirname) + strlen(DIR_SEP) + strlen(fname) + 2];
   sprintf(metafname, "%s%s.%s", dirname, DIR_SEP, fname);
   // We have generated the meta file name.
   retval = stat(metafname, &mystat);
   if (retval == -1) {
      // The meta file doesnt even exist. Lets return.
      goto done;
   }
   actfsize = mystat.st_size;

   // OK, so the metafile metafname exists. Lets check its integrity.
   fp = fopen(metafname, "r");
   COUT(cout << "fopen: " << metafname << endl;)
   if (fp == NULL) {
      goto done;
   }
   inputline = new char[1024];

   // Lets analyze Line 1
   inputline[0] = '\0'; // so that strlen doesnt bomb on error.
   fgets(inputline, 1023, fp);
   fsize = strlen(inputline);
   // fgets returns with the new line character as well.
   if (fsize != 0) {
      inputline[fsize - 1] = '\0';
   }

   // So inputline has: FileSize, FileSHA1, PayLoadSHA1, FileName
   FileSize = strtoul(inputline, &endptr, 10);
   if (*endptr != ' ') {
      // We didnt convert till we hit a space.
      goto done;
   }

   LineP = inputline;
   parseptr = LineP.getWord(2);
   // This is FileSHA1 (length = 40)
   if (strlen(parseptr) != 40) {
      goto done;
   }
   strcpy(FileSHA1, parseptr);
   convertSHA1ToRaw(FileSHA1, FileSHA1Raw);

   parseptr = LineP.getWord(3);
   // This is PayloadSHA1 (length = 40)
   if (strlen(parseptr) != 40) {
      goto done;
   }
   strcpy(SHA1OfPieces, parseptr);
   convertSHA1ToRaw(SHA1OfPieces, SHA1OfPiecesRaw);

   parseptr = LineP.getWordRange(4, 0);
   if (strcasecmp(parseptr, fname) != 0) {
      // The FileName is not the same.
      goto done;
   }

   // Now lets move to Line 2.
   inputline[0] = '\0'; // so that strlen doesnt bomb on error
   fgets(inputline, 1023, fp);
   // We are done with fp for now.
   fclose(fp);
   COUT(cout << "fclose:" << endl;)
   fp = NULL;

   fsize += strlen(inputline);
   // fgets returns with the new line character.
   if (strlen(inputline) != 0) {
      inputline[strlen(inputline) - 1] = '\0';
   }

   // So inputline has: payload size.
   // payload size = ( (FileSize shift right 17) + 1) * 20
   SHA1MetaInfoSize = strtoul(inputline, &endptr, 10);
   if ( (endptr == inputline) || (*endptr != '\0') ) {
      // We didnt convert the full line.
      goto done;
   }

   // Lets check if the SHA1MetaInfoSize is correct.
   if ( (((FileSize >> 17) + 1) * 20) != SHA1MetaInfoSize) {
      // The PieceSize doesnt match with FileSize.
      goto done;
   }

   // Now lets check if the filesize of the .file adds up.
   fsize += SHA1MetaInfoSize;
   if (actfsize != fsize) {
      // The size of the meta file doesnt match with what it should be.
      goto done;
   }

   delete [] SHA1MetaInfo;
   SHA1MetaInfo = new char[SHA1MetaInfoSize];

   // Lets open with fd and read the relevant parts.
   fd = open(metafname, O_RDONLY | O_BINARY);
   COUT(cout << "open: " << metafname << " returned: " << fd << endl;)
   if (fd == -1) {
      // Could open the meta file.
      goto done;
   }

   // Lets seek to the correct place in file.
   retval = lseek(fd, actfsize - SHA1MetaInfoSize, SEEK_SET);
   if (retval == -1) {
      // Couldnt seek to the correct place.
      goto done;
   }

   to_read = SHA1MetaInfoSize;
   cur_index = 0;
   do {
      retval = read(fd, &SHA1MetaInfo[cur_index], to_read);
      cur_index += retval;
      to_read -= retval;
   } while ( (retval > 0) && (to_read > 0) );

   // We are done with using fd.
   close(fd);
   COUT(cout << "close: " << fd << endl;)
   fd = -1;

   if (cur_index != SHA1MetaInfoSize) {
      // Couldnt read the pieces.
      goto done;
   }

   // Construct the SHA1 of the pieces.
   SHA1.Reset();
   SHA1.Update((UINT_8 *) SHA1MetaInfo, (UINT_32) SHA1MetaInfoSize);
   SHA1.Final();
   char temp41[41], temp20[20];
   SHA1.ReportHash(temp41);
   SHA1.GetHash((unsigned char *) temp20);

   // Check if the SHA's match.
   if (memcmp(SHA1OfPiecesRaw, temp20, 20) != 0) {
      // SHA's do not match.
      goto done;
   }

   // All is well
   initialized = true;

done:
   if (fd != -1) {
      close(fd);
      COUT(cout << "close: " << fd << endl;)
   }
   if (fp) {
      fclose(fp);
      COUT(cout << "fclose:" << endl;)
   }
   delete [] inputline;
   delete [] metafname;

   return(initialized);
}

// Returns the file size as mentioned in the meta file.
size_t SHA1File::getFileSize() {

   TRACE();

   if (initialized) {
      return(FileSize);
   }
   else {
      return(0);
   }
}

// Returns the RAW SHA1 of the File as mentioned in the meta file.
bool SHA1File::getFileSHA1Raw(char *raw) {

   TRACE();

   if (initialized) {
      memcpy(raw, FileSHA1Raw, 20);
   }
   else {
      memset(raw, 0, 20);
   }
   return(initialized);
}

// Returns the RAW SHA1 of the SHA1 of the Pieces as mentioned in the meta file.
bool SHA1File::getSHA1ofPiecesRaw(char *raw) {

   TRACE();

   if (initialized) {
      memcpy(raw, FileSHA1Raw, 20);
   }
   else {
      memset(raw, 0, 20);
   }
   return(initialized);
}

// Returns the HEX string of the SHA1 of the File.
// buf is at least 41 bytes.
bool SHA1File::getFileSHA1(char *buf) {

   TRACE();

   if (initialized) {
      strcpy(buf, FileSHA1);
   }
   else {
      buf[0] = '\0';
   }
   return(initialized);
}

// Returns the HEX string of the SHA1 of the MetaInfo Pieces.
// buf is at least 41 bytes.
bool SHA1File::getSHA1ofPieces(char *buf) {

   TRACE();

   if (initialized) {
      strcpy(buf, SHA1OfPieces);
   }
   else {
      buf[0] = '\0';
   }
   return(initialized);
}

// Return the whole array of SHA1 of the pieces.
const char *SHA1File::getSHA1MetaInfoRaw() {

   TRACE();

   if (initialized) {
      return(SHA1MetaInfo);
   }
   else {
      return(NULL);
   }
}

// Return the length of the SHA1MetaInfo
size_t SHA1File::getSHA1MetaInfoRawSize() {
   TRACE();

   if (initialized) {
      return(SHA1MetaInfoSize);
   }
   else {
      return(0);
   }
}

// Used to convert the HEX ascii SHA1 in raw 20 byte form.
// char array: hex is at least 41 bytes, and raw is at least 20 bytes.
void SHA1File::convertSHA1ToRaw(char *hex, char *raw) {
char pairs[3];
unsigned char a;

   TRACE();

   pairs[2] = '\0';
   for (int i = 0; i < 20; i++) {
      pairs[0] = hex[2*i];
      pairs[1] = hex[2*i + 1];
      a = (unsigned char) strtol(pairs, NULL, 16);
      raw[i] = a;
   }
}

// Used to generate the SHA1 information file .file.avi of file.avi
bool SHA1File::generateSHA1OfFile(char *dirname, char *fname) {
int fd;
char *metafname = NULL;
char *filename = NULL;
struct stat mystat;
int retval;
bool retvalb;
char *inputline = NULL;

   delete [] SHA1MetaInfo;
   SHA1MetaInfo = NULL;
   initialized = false;

   filename = new char[strlen(dirname) + strlen(DIR_SEP) + strlen(fname) + 1];
   sprintf(filename, "%s%s%s", dirname, DIR_SEP, fname);
   // We have generated the file name whose meta info file has to be created.
   
   retval = stat(filename, &mystat);
   if (retval == -1) {
      delete [] filename;
      return(false);
   }

   // Lets get the FileSize and SHA1MetaInfoSize
   FileSize = mystat.st_size;
   SHA1MetaInfoSize = (((FileSize >> 17) + 1) * 20);

   metafname = new char[strlen(dirname) + strlen(DIR_SEP) + strlen(fname) + 2];
   sprintf(metafname, "%s%s.%s", dirname, DIR_SEP, fname);
   // We have generated the meta file name.

   fd = open(metafname,  O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, CREAT_PERMISSIONS);
   COUT(cout << "open: " << metafname << " returned: " << fd << endl;)
   if (fd == -1) {
      delete [] filename;
      delete [] metafname;
      return(false);
   }

   // We have created or truncated file, with fd.
   // Lets generate all the data needed to write to file.

   // First lets get the SHA1 of the full file.
   SHA1.Reset();
   retvalb = SHA1.HashFile(filename, 0, 0);
   if (retvalb == false) {
      goto done;
   }
   SHA1.Final();
   SHA1.GetHash((unsigned char *) FileSHA1Raw);
   SHA1.ReportHash(FileSHA1);
   // We have gotten the SHA1 info of the full file.

   // Now lets get the SHA1 of each piece. (populate SHA1MetaInfo);
   SHA1MetaInfo = new char[SHA1MetaInfoSize];
   size_t start_offset, end_offset, pow2_17;
   start_offset = 0;
   pow2_17 = 1;
   pow2_17 << 17; // (2 power 17 = 128 KB)
   end_offset = pow2_17 - 1;
   for (int i = 0; i < (SHA1MetaInfoSize/20); i++) {
      SHA1.Reset();
      retvalb = SHA1.HashFile(filename, start_offset, end_offset);
      if (retvalb == false) {
         goto done;
      }
      SHA1.Final();
      SHA1.GetHash((unsigned char *) &SHA1MetaInfo[i * 20]);
      start_offset += pow2_17;
      end_offset += pow2_17;
      if (end_offset >= FileSize) {
         end_offset = FileSize - 1;
      }
   }

   // Now lets obtain the SHA1 of SHA1MetaInfo
   SHA1.Reset();
   SHA1.Update((UINT_8 *) SHA1MetaInfo, (UINT_32) SHA1MetaInfoSize);
   SHA1.Final();
   SHA1.GetHash((unsigned char *) SHA1OfPiecesRaw);
   SHA1.ReportHash(SHA1OfPieces);

   // All values have been got. Lets write to metafile.
   inputline = new char[1024];
   generatePreambleLines(inputline, fname);
   retval = write(fd, inputline, strlen(inputline));
   if (retval != strlen(inputline)) {
      goto done;
   }

   retval = write(fd, SHA1MetaInfo, SHA1MetaInfoSize);
   if (retval != SHA1MetaInfoSize) {
      goto done;
   }

   initialized = true;

   // Lets make metafname hidden under WINDOWS.
#ifdef __MINGW32__
   SetFileAttributes(metafname, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN);
#endif

done:
   close(fd);
   COUT(cout << "close: " << fd << endl;)
   delete [] inputline;
   delete [] filename;
   delete [] metafname;
   return(initialized);
}

// Generate the preamble Lines including \n
bool SHA1File::generatePreambleLines(char *inputline, char *fname) {

   sprintf(inputline, "%lu %s %s %s\n%lu\n",
                       FileSize,
                       FileSHA1,
                       SHA1OfPieces,
                       fname,
                       SHA1MetaInfoSize);
   return(true);
}
