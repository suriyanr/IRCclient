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

#include "Swarm.hpp"

#include "LineQueueBW.hpp"
#include "Compatibility.hpp"

#include <sys/types.h>
#include <sys/stat.h>


using namespace std;

// The File from which we read values and spawn the threads.
#define SWARMCONFIGFILE "Swarm.dat"

// The Global Array which dictates if the threads are finished.
bool ThreadOver[MAX_SWARM_NODES];

int TotalThreads = 0;

bool StartSimulation = false;

// The Global Array used for Message passing.
LineQueueBW RecvMessageQueue[MAX_SWARM_NODES];

int main(int argc, char *argv[]) {
SwarmNodeProperty *SNP;
FILE *fd;
int retval;
THR_HANDLE ThrH;
THREADID tempTID;
char FullFilePath[512];

   // Initialise ThreadOver.
   for (int i = 0; i < MAX_SWARM_NODES; i++) {
      ThreadOver[i] = false;
   }

   fd = fopen(SWARMCONFIGFILE, "r");
   if (fd == NULL) {
      cout << "Cannot open Swarm config file: " << SWARMCONFIGFILE << endl;
      exit(1);
   }

   while (true) {
      SNP = new SwarmNodeProperty;
      SNP->ThreadID = TotalThreads;
      retval = fscanf(fd, "%s %s %s %lu %lu %c",
                       SNP->Nick,
                       SNP->DirName,
                       SNP->FileName,
                       &(SNP->MaxUploadBPS),
                       &(SNP->MaxDownloadBPS),
                       &(SNP->FirewallState));

      if (retval != 6) {
         fclose(fd);
         delete SNP;
         break;
      }

      // Lets spawn the thread with this SNP.
      // Lets fill up the FileSize.
      SNP->FileSize = 0;
      cout << "Spawning Thread with ID: " << SNP->ThreadID << " Nick: " << SNP->Nick << " DirName: " << SNP->DirName << " FileName: " << SNP->FileName << " MaxUploadBPS: " << SNP->MaxUploadBPS << " MaxDownloadBPS: " << SNP->MaxDownloadBPS << " FirewallState: " << SNP->FirewallState << endl;

      #ifdef __MINGW32__
      ThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) SwarmNodeThread, 
                          SNP, 0, &tempTID);
      #else
      {
         pthread_attr_t thread_attr;
         pthread_attr_init(&thread_attr);
         pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
         pthread_create(&ThrH, &thread_attr, 
                        (void * (*)(void *)) SwarmNodeThread, SNP);
      }
      #endif

      TotalThreads++;
   }

   cout << "Spawned all the Threads representing the Swarm." << endl;

   // Signal them to start.
   StartSimulation = true;

   // We will let them exit on their own, once they find they have got
   // max file size.
   // Lets sleep and instruct them to terminate.
   sleep(3600);
   for (int i = 0; i < TotalThreads; i++) {
      RecvMessageQueue[i].putLine("QT");
   }

   // We need to wait on all them to be finished processing.
   int CheckThreads;
   while (true) {
      CheckThreads = 0;
      for (int i = 0; i < TotalThreads; i++) {
         if (ThreadOver[i] == false) {
            CheckThreads++;
         }
      }
      if (CheckThreads == 0) break;
      sleep(1);
   }

   cout << "All Spawned Threads exited." << endl;
#if 0
   for (int i = 0; i < TotalThreads; i++) {
      cout << "ThrID: " << i << " ";
      RecvMessageQueue[i].printStatistics();
   }
#endif
}

