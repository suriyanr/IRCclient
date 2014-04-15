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

#ifndef CLASS_FILESERVER
#define CLASS_FILESERVER

#include "ThreadMain.hpp"

// This Class handles the File Server talking interface
// We just list the files which are in MyFilesDB in XGlobal.
class FileServer {
public:
   FileServer();
   ~FileServer();
   void run(DCC_Container_t *);

private:
   bool welcomeMessage();
   void updateSendsAndQueues();
   bool fservCD(char *dirname);
   bool fservPWD();
   bool fservDir();
   bool fservGet(const char *filename, bool fromPartialDir, size_t resume_pos);
   bool fservSends();
   bool fservQueues();
   bool fservClearQueues(int q_num); // 0 => clear all queues.
   bool fservMetaInfo(char *filename);
   bool fservNicklist();
   bool fservFilelist(char *nickname, char dstate);
   bool fservEndlist();
   bool fservExit();
   bool fservUnknown();
   bool sendMessage();
   bool recvMessage(); // Used only during MM to MM file list exchange.

   // The Current Directory that we are serving. Initialised to empty string.
   // Note that this has directory with path seperator = FS_DIR_SEP
   char *CurrentDir;

   char *RetPointer;
   char MyNick[32];

   // Below are the current sends/queues in progress
   int TotalSends;
   int TotalQueues;
   int BigQueues;
   int SmallQueues;

   // Below are my maximas as got from XGlobal.
   int MyMaxSends;
   int MyMaxQueues;
   int MyMaxSendsToEachUser;
   int MyMaxQueuesToEachUser;
   size_t MyMaxOverallCPS;
   size_t MyMaxSmallFileSize;

   unsigned int RemoteNickMode;

   // FFLC Protocol Version number of client.
   int FFLCversion;

   // Below are saved by run() from what it receives in DCC_Container.
   // The Destructor should not destroy these.
   XChange *XGlobal;
   TCPConnect *Connection;
   char *RemoteNick;
   char *RemoteDottedIP;
};

#endif
