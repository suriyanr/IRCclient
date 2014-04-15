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
using namespace std;

#include <errno.h>

#ifdef __MINGW32__
# include <io.h>
#else
 #include <fcntl.h>
 #include <semaphore.h>
 #include <sys/types.h>
 #include <sys/stat.h>
#endif

#include <unistd.h>

#include "IRCClient.hpp"
#include "ThreadMain.hpp"
#include "StackTrace.hpp"
#include "Utilities.hpp"

#include "Compatibility.hpp"

#ifdef IRCSUPER
  #define ONLY_ONE_SEM_NAME          "MasalaMate"
#else
  #define ONLY_ONE_SEM_NAME          "MM-Testing"
#endif

// Constructor.
// We spawn the threads we need to spawn.
void IRCClient::init(XChange *XGlobal) {
THREADID tempTID;

   TRACE();
/*
   Thread 1 = The ToServer Q consumer thread.
   Thread 2 = The ToServerNow Q consumer thread.
   Thread 3 = The toUI Q consumer thread.
   Thread 4 = The DCC Q consumer thread.
   Thread 5 = The FromServer Q consumer thread. (UI) feeds UI and Trigger Q
   Myself   = Provider to some of the Q's.

   Thread 6 = User parameter updater thread. (UI)

   Thread 7 = Trigger Text scanning and XDCC scanning. (toTriggerThr) 

   Thread 8 = Try to Initiate a Download of a file (DwnldInitThrH)

   Thread 9 = Timer Thread - Does chores which are periodic. (TimerThr)

   Thread 10 = Upnp Thread - Related to the Upnp port forwarding. (UpnpThr)

   Thread 11 = Swarm Thread - Related to the SwarmStream Protocol. (SwarmThr)

*/

#ifdef __MINGW32__
   ToServerThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) toServerThr, XGlobal, 0, &tempTID);
   ToServerNowThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) toServerNowThr, XGlobal, 0, &tempTID);
   FromServerThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) fromServerThr, XGlobal, 0, &tempTID);
   toUIThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) toUIThr, XGlobal, 0, &tempTID);
   DCCThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) DCCThr, XGlobal, 0, &tempTID);
   fromUIThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) fromUIThr, XGlobal, 0, &tempTID);
   ToTriggerThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) toTriggerThr, XGlobal, 0, &tempTID);
   ToTriggerNowThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) toTriggerNowThr, XGlobal, 0, &tempTID);
   DCCServerThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) DCCServerThr, XGlobal, 0, &tempTID);
   DwnldInitThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) DwnldInitThr, XGlobal, 0, &tempTID);
   TimerThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) TimerThr, XGlobal, 0, &tempTID);
   UpnpThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) UpnpThr, XGlobal, 0, &tempTID);
   SwarmThrH = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) SwarmThr, XGlobal, 0, &tempTID);

#else
   pthread_create(&ToServerThrH, NULL, (void * (*)(void *)) toServerThr, XGlobal);
   pthread_create(&ToServerNowThrH, NULL, (void * (*)(void *)) toServerNowThr, XGlobal);
   pthread_create(&FromServerThrH, NULL, (void * (*)(void *)) fromServerThr, XGlobal);
   pthread_create(&toUIThrH, NULL, (void * (*)(void *)) toUIThr, XGlobal);
   pthread_create(&DCCThrH, NULL, (void * (*)(void *)) DCCThr, XGlobal);
   pthread_create(&fromUIThrH, NULL, (void * (*)(void *)) fromUIThr, XGlobal);
   pthread_create(&ToTriggerThrH, NULL, (void * (*)(void *)) toTriggerThr, XGlobal);
   pthread_create(&ToTriggerNowThrH, NULL, (void * (*)(void *)) toTriggerNowThr, XGlobal);
   pthread_create(&DCCServerThrH, NULL, (void * (*)(void *)) DCCServerThr, XGlobal);
   pthread_create(&DwnldInitThrH, NULL, (void * (*)(void *)) DwnldInitThr, XGlobal);
   pthread_create(&TimerThrH, NULL, (void * (*)(void *)) TimerThr, XGlobal);
   pthread_create(&UpnpThrH, NULL, (void * (*)(void *)) UpnpThr, XGlobal);
   pthread_create(&SwarmThrH, NULL, (void * (*)(void *)) SwarmThr, XGlobal);
#endif

}

// Destructor.
// We wait for the spawned threads to exit.
void IRCClient::endit(XChange *XGlobal) {
FilesDetail *FD, *ScanFD;
int alive_connections;
int loop_count;

   TRACE();

   COUT(cout << "IRCClient::endit Start" << endl;)

   COUT(cout << "IRCClient:: waiting for all FServClientInProgress to end" << endl;)
   loop_count = 0;
   do {
      loop_count++;
      alive_connections = 0;
      FD = XGlobal->FServClientInProgress.searchFilesDetailList("*");
      ScanFD = FD;
      while (ScanFD) {
         if (ScanFD->Connection) alive_connections++;
         ScanFD = ScanFD->Next;
      }
      XGlobal->FServClientInProgress.freeFilesDetailList(FD);
      if (loop_count > 10) {
         // Only loop 10 times.
         COUT(cout << "FServClientInProgress that didnt end: " << alive_connections << endl;)
         break;
      }
      if (alive_connections) sleep(1);
   } while (alive_connections);

   COUT(cout << "IRCClient:: waiting for all File Servers to end" << endl;)
   loop_count = 0;
   do {
      loop_count++;
      alive_connections = 0;
      FD = XGlobal->FileServerInProgress.searchFilesDetailList("*");
      ScanFD = FD;
      while (ScanFD) {
         if (ScanFD->Connection) alive_connections++;
         ScanFD = ScanFD->Next;
      }
      XGlobal->FileServerInProgress.freeFilesDetailList(FD);
      if (loop_count > 10) {
         // Only loop 10 times.
         COUT(cout << "File Servers that didnt end: " << alive_connections << endl;)
         break;
      }
      if (alive_connections) sleep(1);
   } while (alive_connections);

   COUT(cout << "IRCClient:: waiting for all DwnldInProgress to end" << endl;)
   loop_count = 0;
   do {
      loop_count++;
      alive_connections = 0;
      FD = XGlobal->DwnldInProgress.searchFilesDetailList("*");
      ScanFD = FD;
      while (ScanFD) {
         if (ScanFD->Connection) alive_connections++;
         ScanFD = ScanFD->Next;
      }
      XGlobal->DwnldInProgress.freeFilesDetailList(FD);
      if (loop_count > 10) {
         // Only loop 10 times.
         COUT(cout << "DwnldInProgress that didnt end: " << alive_connections << endl;)
         break;
      }
      if (alive_connections) sleep(1);
   } while (alive_connections);

   COUT(cout << "IRCClient:: waiting for all SendsInProgress to end" << endl;)

   loop_count = 0;
   do {
      loop_count++;
      alive_connections = 0;
      FD = XGlobal->SendsInProgress.searchFilesDetailList("*");
      ScanFD = FD;
      while (ScanFD) {
         if (ScanFD->Connection) alive_connections++;
         ScanFD = ScanFD->Next;
      }
      XGlobal->SendsInProgress.freeFilesDetailList(FD);
      if (loop_count > 10) {
         // Only loop 10 times.
         COUT(cout << "SendInProgress that didnt end: " << alive_connections << endl;)
         break;
      }
      if (alive_connections) sleep(1);
   } while (alive_connections);

   COUT(cout << "IRCClient:: All SendsInProgress ended" << endl;)

   // Delete the FD->Data if present in all of DwnldWaiting.
   FD = XGlobal->DwnldWaiting.searchFilesDetailList("*");
   ScanFD = FD;
   while (ScanFD) {
      deleteStringArray((char **) ScanFD->Data);

      // Update it in DwnldWaiting too.
      XGlobal->DwnldWaiting.updateFilesDetailNickFileData(ScanFD->Nick, ScanFD->FileName, NULL);
      ScanFD = ScanFD->Next;
   }
   if (FD) {
      XGlobal->DwnldWaiting.freeFilesDetailList(FD);
   }
  
   WaitForThread(ToServerThrH);
   WaitForThread(ToServerNowThrH);
   WaitForThread(FromServerThrH);
   WaitForThread(toUIThrH);
   WaitForThread(DCCThrH);
   WaitForThread(fromUIThrH);
   WaitForThread(ToTriggerThrH);
   WaitForThread(ToTriggerNowThrH);
   WaitForThread(DCCServerThrH);
   WaitForThread(DwnldInitThrH);
   WaitForThread(TimerThrH);
   WaitForThread(UpnpThrH);
   WaitForThread(SwarmThrH);
  
   COUT(cout << "IRCClient::endit End" << endl;)
}

// This is the main entry point for our client.
void IRCClient::run(XChange *XGlobal) {
bool FirstTime = true;
bool ServerJustConnected;
ConnectionMethod CM;
IRCServerList SL;
IRCChannelList CL;
char IRCNick[64];
char ServerName[256];
int servernum = 1;
int totalservernum;
int i;
char UILine[1024];

   TRACE();
   IRCNick[0] = '\0';

// Lets check if an instance is already running.
#ifdef __MINGW32__
   HANDLE OnlyOneSem = CreateSemaphore(NULL, 0, 1, ONLY_ONE_SEM_NAME);
   if (OnlyOneSem == NULL) {
      // Semaphore already exists.
      return;
   }
   if (GetLastError() == ERROR_ALREADY_EXISTS) {
      // Semaphore already exits.
      return;
   }
#else
// Fill this up for UNIX.
   sem_t *OnlyOneSem = sem_open(ONLY_ONE_SEM_NAME, O_CREAT | O_EXCL, S_IRWXU, 1);
   if (OnlyOneSem == (sem_t *) SEM_FAILED) {
      // Semaphore already exits.
      COUT(cout << "OnlyOneSem sem_open failed. errno: " << errno << endl;)
      //return;
   }
#endif

   // Lets initialize the random number generator.
   srand(getpid());

// Lets spawn the threads.
   init(XGlobal);

// Now we do an endless loop. Steps are as follows:
/*
1. If first time through the loop, then we wait for all the below elements to be true:
   If not first time, we do check for changes but not wait for them to be changed.
      a. isIRC_CM_Changed() => Connection method is updated by userParamThr().
      b. isIRC_SL_Changed() => Server List is updated by userParamThr().
      c. isIRC_CL_Changed() => Channel List is updated by userParamThr().
      d. isIRC_Nick_Changed() => Nick is updated by userParamThr().
2. Attempt connection to IRC Servers. Keep feeding the FromServer Q, so that the fromServerThr()
   can update the UI of the attempts etc.
3. If just connected to Server, do the initial required steps as follows:
      a. Set Nick.
      b. Set user mode.
      c. if we have password for nick, identify.
      d. Issue userhost so that we set our ip as server sees it.
4. Process regular lines from server. Take care of regular stuff.
   Queue the DCCs to DCC Q. The UIs to toUI Q.
*/

   while (true) {
   char IRC_Line[1024];

      if (FirstTime) {
         while (XGlobal->isIRC_CM_Changed(CM) != true) {
//          Lets wait for Connection Method to be available.
            COUT(cout << "Waiting on isIRC_CM_Changed()" << endl;)
            sleep(1);
         }
         while (XGlobal->isIRC_SL_Changed(SL) != true) {
//          Lets wait for Server List to be available.
            COUT(cout << "Waiting on isIRC_SL_Changed()" << endl;)
            sleep(1);
         }
// We get SL now and never get it again.
         SL = XGlobal->getIRC_SL();
         XGlobal->resetIRC_SL_Changed();
         COUT(SL.printDebug();)

         while (XGlobal->isIRC_CL_Changed(CL) != true) {
//          Lets wait for Channel List to be available.
            COUT(cout << "Waiting on isIRC_CL_Changed()" << endl;)
            sleep(1);
         }
// We get CL now and never get it again.
         CL = XGlobal->getIRC_CL();
         XGlobal->resetIRC_CL_Changed();
         COUT(CL.printDebug();)

         while (XGlobal->isIRC_Nick_Changed(IRCNick) != true) {
//          Lets wait for Nick to be available.
            COUT(cout << "Waiting on isIRC_Nick_Changed()" << endl;)
            sleep(1);
         }
//         FirstTime = false; // Set it down the line.
//       All prerequisites have been met !!
      }

//    All Changes have been gotten into the local variables.
//    Above will have to be revisited to take care of things needed to be done
//    while they are dynamically changed.

//    Lets wait indefinitely to connect to the server.
      if (XGlobal->IRC_Conn.state() != TCP_ESTABLISHED) {

//       Set all the Channels to not joined state.
//       All the LineQueue consumers will check the disconnected state
//       and purge the lines in q.
         for (i = 1; i <= CL.getChannelCount(); i++) {
            CL.setNotJoined(i);
         }
         XGlobal->putIRC_CL(CL); // update global with that state.
         

         totalservernum = SL.getServerCount();
         if (FirstTime == false) {
         THR_EXITCODE ExitCode;
            COUT(cout << "sleeping 10 seconds...Delaying the connect" << endl;)
            sprintf(UILine, "Server 15IRC: Sleeping 10 seconds ... To avoid being throttled...");
            XGlobal->IRC_ToUI.putLine(UILine);
            if (XGlobal->isIRC_QUIT()) {
               break;
            }

            sleep(10);
            if (XGlobal->isIRC_QUIT()) break;
         }
         else FirstTime = false;
         while (true) {
         char *server;
         unsigned short serverport;

            if (XGlobal->isIRC_CM_Changed(CM) == true) {
               COUT(cout << "Yes, we need to get the CM" << endl;)
               CM = XGlobal->getIRC_CM();
               XGlobal->resetIRC_CM_Changed();
               XGlobal->IRC_Conn.TCPConnectionMethod = CM;
               COUT(XGlobal->IRC_Conn.TCPConnectionMethod.printDebug();)
            }
            if (XGlobal->isIRC_Nick_Changed(IRCNick) == true) {
               XGlobal->getIRC_Nick(IRCNick);
               XGlobal->resetIRC_Nick_Changed();
            }

            server = SL.getServer(serverport, servernum);
            sprintf(UILine, "Server 15IRC: Connecting to %s (%d)", server, serverport);
            XGlobal->IRC_ToUI.putLine(UILine);
            if (XGlobal->IRC_Conn.getConnection(server, serverport) == true) {
               SL.setJoined(server, serverport);
               ServerJustConnected = true;
               COUT(cout << "Server number: " << servernum << " Connection to " << server << " at port " << serverport << " Established" << endl;)
               sprintf(UILine, "Server 09IRC: Connection Established");
               XGlobal->IRC_ToUI.putLine(UILine);
               servernum++; // we break out of here.
               if (servernum > totalservernum) {
                  servernum = 1;
               }
               break;
            }
            COUT(cout << "Server number: " << servernum << " Connection to " << server << " at port " << serverport << " failed. Sleeping 10 seconds." << endl;)
            sprintf(UILine, "Server 04IRC: Unable to Connect");
            XGlobal->IRC_ToUI.putLine(UILine);
            sprintf(UILine, "Server 15IRC: Sleeping 10 seconds ... To avoid being throttled ...");
            XGlobal->IRC_ToUI.putLine(UILine);
            if (XGlobal->isIRC_QUIT()) {
               break;
            }
            sleep(10);
            servernum++;
            if (servernum > totalservernum) {
               servernum = 1;
            }
         }
      }
//    We have a successfull server connection.

      if (XGlobal->isIRC_QUIT()) {
         break;
      }
      if (ServerJustConnected == true) {
         char *tmp_str;
         sprintf(IRC_Line, "NICK %s", IRCNick);
         XGlobal->IRC_ToServer.putLine(IRC_Line);

         // Lets replace the ` if present in nick to _
         // else IRCsuper doesnt let us connect.
         tmp_str = new char[strlen(IRCNick) + 1];
         strcpy(tmp_str, IRCNick);
         for (int i = 0; i < strlen(tmp_str); i++) {
            if (tmp_str[i] == '`') tmp_str[i] = '_';
         }

         sprintf(IRC_Line, "USER %s 32 . :%s", tmp_str, tmp_str);
         delete [] tmp_str;

         XGlobal->IRC_ToServer.putLine(IRC_Line);
         COUT(cout << "Nick: |" << IRCNick << "| --" << endl);
//         sprintf(IRC_Line, "MODE %s +i-Rv", IRCNick);
//         XGlobal->IRC_ToServer.putLine(IRC_Line);

      }

      ServerJustConnected = false;
      if (servernum > totalservernum) {
         servernum = 1;
      }

      // Wait indefinitely for text, as PING is handled elsewhere.
      // Loop every PING_TIMEOUT seconds. So we can ping server, if it doesnt.
      // #define PING_TIMEOUT 180
      ssize_t retval = XGlobal->IRC_Conn.readLine(IRC_Line, 1024, TIMEOUT_INF);
      if (XGlobal->isIRC_QUIT()) break;
      switch (retval) {
         case 0:
#if 0    // PING - PONG handled in TimerThr()
         // We timed out waiting for PING_TIMEOUT seconds from server.
         // Lets send it a PING.
         if (XGlobal->isIRC_Server_Changed(ServerName)) {
            XGlobal->getIRC_Server(ServerName);
            XGlobal->resetIRC_Server_Changed();
         }
//       Server Name should be set when we come here.
         sprintf(IRC_Line, "PING :%s", ServerName);
         XGlobal->IRC_ToServer.putLine(IRC_Line);
#endif
         break;

         case -1:
            COUT(cout << "readLine: " << retval << endl;)
            // Need to inform UI that we have been disconnected and clear
            // all the nick lists.
            sprintf(UILine, "Server 04IRC: Disconnected from Server");
            XGlobal->IRC_ToUI.putLine(UILine);
            for (i = 1; i <= CL.getChannelCount(); i++) {
               sprintf(UILine, "%s 04IRC: Disconnected from Server", CL.getChannel(i));
               XGlobal->IRC_ToUI.putLine(UILine);
               sprintf(UILine, "*NICKCLR* %s DUMMY", CL.getChannel(i));
               XGlobal->IRC_ToUI.putLine(UILine);
            }

            break;

         default:
//            COUT(cout << "IRCClient: putLineIRC_FromServer: " << IRC_Line << endl;)
            XGlobal->IRC_FromServer.putLine(IRC_Line);
            break;
      }
      if (XGlobal->isIRC_CM_Changed(CM) == true) {
         COUT(cout << "Yes, we need to get the CM" << endl;)
         CM = XGlobal->getIRC_CM();
         XGlobal->resetIRC_CM_Changed();
         XGlobal->IRC_Conn.TCPConnectionMethod = CM;
         COUT(XGlobal->IRC_Conn.TCPConnectionMethod.printDebug();)
      }
      if (XGlobal->isIRC_Nick_Changed(IRCNick) == true) {
         XGlobal->getIRC_Nick(IRCNick);
         XGlobal->resetIRC_Nick_Changed();
         COUT(cout << "Nick: " << IRCNick << endl;)
      }
      if (XGlobal->isIRC_QUIT()) break;
   }

// Lets wait for all the thread to exit.
   COUT(cout << "IRCClient::run quitting" << endl;)

   COUT(cout << "Calling endit" << endl;)
   endit(XGlobal); 

// Lets destroy the OnlyOneSem, created so only one instance can be run.
#ifdef __MINGW32__
   DestroySemaphore(OnlyOneSem);
#else
   sem_close(OnlyOneSem);
   sem_unlink(ONLY_ONE_SEM_NAME);
#endif

}

