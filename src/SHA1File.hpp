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

#ifndef CLASS_SHA1FILE
#define CLASS_SHA1FILE

#include "SHA1.hpp"

#include "Compatibility.hpp"

/* 
   Format of .filename file:
   FileSize FileSHA1 PayloadSHA1 FileName\n
   <payload size>\n
   <payload length bytes of piecewise SHA1 in raw>

   This is exactly the info passed on when "metainfo filename" is issued in
   the fileserver.
   Note that the FileSHA1 and PayloadSHA1 are in ascii and not raw.
   PiecesSize = (((FileSize >> 17) + 1) * 20)
   as 2^17 bytes correspond to 1 piece = 20 byte SHA1
*/


class SHA1File {
public:
   SHA1File();
   ~SHA1File();

   // Init the SHA1 info of fname and .fname. Checks if it exists.
   // Corrects if it is wrong, creates if it doesnt exist.
   bool initFile(char *dirname, char *fname);

   // Once initFile() is called, we can then use the below functions.
   size_t getFileSize();
   bool   getFileSHA1Raw(char *raw); // at least 20 bytes.
   bool   getFileSHA1(char *buf);    // at least 41 bytes.
   const char *getSHA1MetaInfoRaw();
   size_t getSHA1MetaInfoRawSize();
   bool   getSHA1ofPiecesRaw(char *raw); // at least 20 bytes.
   bool   getSHA1ofPieces(char *buf);    // at least 41 bytes.

   // Generate the preamble lines, including \n
   // Returns line in line, fname = filename
   bool   generatePreambleLines(char *line, char *fname);
   
protected:
   bool initialized;

   // Used to check the SHA1 infomation of fname as contained in .fname
   // returns true if check is OK. returns false, if not OK => generate.
   bool checkSHA1OfFile(char *dirname, char *fname);

   // Used to generate the SHA1 information file .file.avi of file.avi
   bool generateSHA1OfFile(char *dirname, char *fname);

   // Used to convert the HEX ascii SHA1 in raw 20 byte form.
   // char array: hex is at least 41 bytes, and raw is at least 20 bytes.
   void convertSHA1ToRaw(char *hex, char *raw);

   CSHA1 SHA1;

   char *SHA1MetaInfo;
   size_t FileSize;
   size_t SHA1MetaInfoSize;
   char FileSHA1Raw[20];
   char FileSHA1[41];
   char SHA1OfPiecesRaw[20];
   char SHA1OfPieces[41];
};


#endif
