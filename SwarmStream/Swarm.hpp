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

#ifndef SWARMHPP
#define SWARMHPP

#define MAX_SWARM_NODES  50

class LineQueueBW;

// Globals for Thread Management.
extern bool ThreadOver[MAX_SWARM_NODES];

extern bool StartSimulation;

extern int TotalThreads;

// Globals for Thread Message passing.
extern LineQueueBW RecvMessageQueue[MAX_SWARM_NODES];

typedef struct SwarmNodeProperty {
   int ThreadID;
   char Nick[32];
   char DirName[32];
   char FileName[32];
   size_t FileSize;
   size_t MaxUploadBPS;
   size_t MaxDownloadBPS;
#define FIREWALLED_YES 'Y'
#define FIREWALLED_NO  'N'
   char FirewallState;
} SwarmNodeProperty;

void SwarmNodeThread(SwarmNodeProperty *Property);


#endif
