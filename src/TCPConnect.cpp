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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#include "TCPConnect.hpp"
#include "Base64.hpp"
#include "StackTrace.hpp"

#include "Compatibility.hpp"

bool  TCPConnect::WSADATA_Initialized = false;

// Class wide private statics initialisation
// CAP initialised to no cap.
size_t TCPConnect::OverallMaxUploadBps = 0;
size_t TCPConnect::OverallMaxDwnldBps = 0;

time_t TCPConnect::ClassUploadCurrentTime = 0;
time_t TCPConnect::ClassDwnldCurrentTime = 0;
time_t TCPConnect::LastClassUploadTime = 0;
size_t TCPConnect::ClassUploadBps = 0;
size_t TCPConnect::ClassDwnldBps = 0;
size_t TCPConnect::LastClassUploadBps = 0;

#if 0
int TCPConnect::TCPConnectClassCount = 0;
TCPConnect **TCPConnect::ValidTCP = NULL;
#endif

// Class wide MUTEX for synchronising. Used for Overall CAP as of now.
#ifdef __MINGW32__
MUTEX TCPConnect::Mutex_TCPConnect = 0;
#else
MUTEX TCPConnect::Mutex_TCPConnect = PTHREAD_MUTEX_INITIALIZER;
#endif

// Constructor.
TCPConnect::TCPConnect() {
   TRACE();

#if 0
   // Create Mutex if first time.
   if (TCPConnectClassCount == 0) {
#ifdef __MINGW32__
      Mutex_TCPConnect = CreateMutex(NULL, FALSE, NULL);
#else
      pthread_mutex_init(&Mutex_TCPConnect, NULL);
#endif
   }
#endif

   State = TCP_UNKNOWN;
   Sock = SOCK_UNUSED;
   SockServer = SOCK_UNUSED;
   Host = NULL;
   Port = 0;
   LongIP = 0;
   RemoteIP = 0;
   RemotePort = 0;
   LastContact = 0;
   Born = time(NULL);
   BytesSent = 0;
   BytesReceived = 0;
   UploadLastMarker = Born;
   DownloadLastMarker = Born;
   UploadArray = new size_t[20];
   DownloadArray = new size_t[20];
   for (int i = 0; i < 20; i++) {
      UploadArray[i] = 0;
      DownloadArray[i] = 0;
   }

   PartialLine = new char[8192];
   PartialLine[0] = '\0';

   MaxUploadBps = 0;
   MaxDwnldBps = 0;

   if (WSADATA_Initialized == false) {
      // Create the MUTEX
#ifdef __MINGW32__
      Mutex_TCPConnect = CreateMutex(NULL, FALSE, NULL);

      // Initialize TCP for MINGW32
      WSADATA wsaData;
      WSAStartup(MAKEWORD(2, 2), &wsaData);
#else
      pthread_mutex_init(&Mutex_TCPConnect, NULL);
#endif
      WSADATA_Initialized = true;
   }

#if 0
   WaitForMutex(Mutex_TCPConnect);
   TCPConnectClassCount++;
   ValidTCP = (TCPConnect **) realloc(ValidTCP, TCPConnectClassCount * sizeof(void *));
   ValidTCP[TCPConnectClassCount - 1] = this;
#endif

#if 0
   fprintf(stderr, "TCP: Con: ClassCount: %d this: %p\n", TCPConnectClassCount, this);
   for (int i = 0; i < TCPConnectClassCount; i++) {
      fprintf(stderr, "TCP: index: %d TCP: %p\n", i, ValidTCP[i]);
   }
   fflush(stderr);
#endif

   SetOptions = true;

   // By default none of the objects are monitored for CAPing up/down.
   MonitorUploadForCap = false;
   MonitorDwnldForCap = false;

#if 0
   ReleaseMutex(Mutex_TCPConnect);
#endif
}

// Destructor
TCPConnect::~TCPConnect() {
   TRACE();

#if 0
   // We destroy the instantiation only if we can get the lock.
   WaitForMutex(Mutex_TCPConnect);

   TCPConnectClassCount--;
#endif

   if (Sock != SOCK_UNUSED) {
      closeSocket(Sock);
   }
   if (SockServer != SOCK_UNUSED) {
      closeSocket(SockServer);
   }
   delete [] Host;
   delete [] UploadArray;
   delete [] DownloadArray;

   delete [] PartialLine;

#if 0
   fprintf(stderr, "TCP: Des: ClassCount: %d\n", TCPConnectClassCount);
   fflush(stderr);
#endif

#if 0
   if (TCPConnectClassCount == 0) {
      ReleaseMutex(Mutex_TCPConnect);
      DestroyMutex(Mutex_TCPConnect);
      free(ValidTCP);
      ValidTCP = NULL;
   }
   else {
      for (int i = 0; i < TCPConnectClassCount; i++) {
         if (ValidTCP[i] == this) {
            ValidTCP[i] = ValidTCP[TCPConnectClassCount];
            break;
         }
      }
      ValidTCP = (TCPConnect **) realloc(ValidTCP, TCPConnectClassCount * sizeof(void *));
      ReleaseMutex(Mutex_TCPConnect);
   }
#endif
}

#if 0
// Used after lock(), and before calling getUploadBPS() or getDownloadBPS()
// to avoid crash when object no longer exists.
// As Mutex is acquired before this call, we dont lock()
bool TCPConnect::isValid() {

   TRACE();

   for (int i = 0; i < TCPConnectClassCount; i++) {
#if 0
      fprintf(stderr, "TCP:isValid: index: %d ptr: %p this: %p\n", i, ValidTCP[i], this);
#endif
      if (ValidTCP[i] == (TCPConnect *) this) {
         return(true);
      }
   }
   return(false);
}
#endif

// Helper Function
// Assumes DottedHost is allocated by caller.
// Converts the long ip given to dotted ip form.
void TCPConnect::getDottedIpAddressFromLong(unsigned long ip, char *DottedHost) {
struct in_addr myin_addr;
char *d_ip;

   TRACE();
   myin_addr.s_addr = htonl(ip);
   d_ip = inet_ntoa(myin_addr);
   strcpy(DottedHost, d_ip);
}

// Helper function
// returns 0 if invalid hostname.
unsigned long TCPConnect::getLongFromHostName(char *thehost) {
struct hostent *myhost;
struct in_addr myin_addr;
unsigned long ourip = 0;
struct sockaddr_in tsocka;
#ifdef __MINGW32__
unsigned long temp_addr;
#endif

   TRACE();
   if ( strspn(thehost, "0123456789.") == strlen(thehost) ) {
//    Dotted IP given.
#ifdef __MINGW32__
      temp_addr = inet_addr(thehost);
      memcpy(&myin_addr, &temp_addr, sizeof(temp_addr));
      if (temp_addr != INADDR_NONE) {
#else
      if (inet_aton(thehost, &myin_addr) != 0) {
#endif
         ourip = ntohl(myin_addr.s_addr);
      }
      return(ourip);
   }

   // Lets try as a hostname.
   myhost = gethostbyname(thehost);
   if (myhost != NULL) {
      memcpy(&tsocka.sin_addr, *((struct in_addr **)myhost->h_addr_list), sizeof(struct in_addr));
      ourip = ntohl(tsocka.sin_addr.s_addr);
   }

   return(ourip);
}

TCPState TCPConnect::state() {
   TRACE();
   return(State);
}

void TCPConnect::setHost(char *varhost) {
   TRACE();
   if (State != TCP_UNKNOWN) return;

   delete [] Host;
   Host = new char[strlen(varhost) + 1];
   strcpy(Host, varhost);
}

void TCPConnect::setPort(unsigned short varport) {
   TRACE();
   if (State != TCP_UNKNOWN) return;

   Port = varport;
}

void TCPConnect::setLongIP(unsigned long varlongip) {
   TRACE();
   if (State != TCP_UNKNOWN) return;

   LongIP = varlongip;
}

bool TCPConnect::serverInit(unsigned short varport) {
int sockoptset;
int sockoptdummy;

   TRACE();
   if (State != TCP_UNKNOWN) return(false);

// We are not connected yet, so proceed.
   SockServer = socket(AF_INET, SOCK_STREAM, 0);
   COUT(cout << "serverInit: SockServer: " << SockServer << endl;)
   if (SockServer < 0) {
      SockServer = SOCK_UNUSED;
      COUT(cout << "serverInit: socket returned < 0" << endl;)
      return(false);
   }
   Port = varport;
   sockoptset = 1;
   // Now reuse.
   setsockopt(SockServer, SOL_SOCKET, SO_REUSEADDR, (const char *) &sockoptset, sizeof(sockoptset));

   // Set SO_SNDBUF
   sockoptdummy = sizeof(sockoptdummy);
   getsockopt(SockServer, SOL_SOCKET, SO_SNDBUF, (char *) &sockoptset, (socklen_t *) &sockoptdummy);
   COUT(cout << "getsockopt: SO_SNDBUF: " << sockoptset << ". Will set it to 65535 if NOT SunOS" << endl;)

#if !defined(_OS_SunOS)
   sockoptset = 65535;
   setsockopt(SockServer, SOL_SOCKET, SO_SNDBUF, (const char *) &sockoptset, sizeof(sockoptset));
#endif

   ListenAddr.sin_family = AF_INET;
   ListenAddr.sin_port = htons(Port);
   ListenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   if (bind(SockServer, (struct sockaddr *) &ListenAddr, sizeof(ListenAddr)) < 0) {
      cleanSockets();
      COUT(cout << "serverInit: bind returned < 0" << endl;)
      return(false);
   }
   if (listen(SockServer, 32) != 0) {
      cleanSockets();
      COUT(cout << "serverInit: listen returned != 0" << endl;)
      return(false);
   }
   State = TCP_LISTENING;
   LastContact = time(NULL);
   return(true);
}

// timeout is in seconds. This call is when its acting as a Server.
bool TCPConnect::getConnection(time_t seconds) {
   TRACE();
   if (State != TCP_LISTENING) return(false);

// No need of Non Blocking Sockets.
//    setNonBlocking(SockServer);

   struct timeval tv;
   struct timeval *tvp = &tv;
   fd_set grp;

   if (seconds == TIMEOUT_INF) {
      tvp = NULL;
   }
   else {
      tv.tv_usec = 0;
      tv.tv_sec = seconds;
   }
   FD_ZERO(&grp);
   FD_SET(SockServer, &grp);
   int state = select(SockServer + 1, &grp, NULL, NULL, tvp);
   if (state == 0) return(false);
   if (state == 1) {
//  Accept the connection.
      int retval = accept(SockServer, NULL, NULL);
      if (retval == -1) {
         return(false);
      }
      Sock = retval;
      State = TCP_ACCEPTED;
      LastContact = time(NULL);

//    Lets get the Connections RemoteIP and RemotePort.
      struct sockaddr_in temp1;
      int addrlen = sizeof(temp1);
      if ((getpeername(Sock, (struct sockaddr *) &temp1, (socklen_t *) &addrlen)) == 0) {
         RemoteIP = ntohl(temp1.sin_addr.s_addr);
         RemotePort = ntohs(temp1.sin_port);
      }
      return(true);
// After this state the user has to do call =
   }
   else {

// Error condition
      cleanSockets();
      return(false);
   }
}

// The assignment operator, should be called only when socket is a 
// Server socket and it is in the TCP_ACCEPTED state.
// Changes in "from"
//     State = TCP_LISTENING, Sock = SOCK_UNUSED
// Changes in "to"
//     State = TCP_ESTABLISHED, SockServer = SOCK_UNUSED
//
// or to a socket which is to be just equated. (not duped).
// Come back here to copy Host info as well and free Host memory if used.
TCPConnect &TCPConnect::operator=(TCPConnect &from) {
   TRACE();

// test for assignment to itself, from = this.
   if (this == &from) return *this;

   switch (from.State) {
     case TCP_ACCEPTED:
        this->Sock = from.Sock;
        this->SockServer = SOCK_UNUSED;
        this->State = TCP_ESTABLISHED;
        from.State = TCP_LISTENING;
        from.Sock = SOCK_UNUSED;
        this->LastContact = from.LastContact;
        this->RemoteIP = from.RemoteIP;
        this->RemotePort = from.RemotePort;
        break;

     default:
        this->Sock = from.Sock;
        this->SockServer = from.SockServer;
        this->State = from.State;
        memcpy(&(this->ListenAddr), &(from.ListenAddr), sizeof(ListenAddr));
        this->Port = from.Port;
        this->LastContact = from.LastContact;
   }
   return *this;
}

// We use select call in all of them.
// No need of making it Non Blocking.
// Let them all be blocking sockets.
// accept outgoing sockets, where we need to not wait too long on a 
// connect() call.
void TCPConnect::setNonBlocking(SOCKET worksock) {
unsigned long nonblock = 1;

   TRACE();

   if (worksock != SOCK_UNUSED) {
#ifdef __MINGW32__
      ioctlsocket(worksock, FIONBIO, &nonblock);
#else
      int fflags = fcntl(worksock, F_GETFL);
      fflags |= O_NONBLOCK;
      fcntl(worksock, F_SETFL, fflags);
#endif
   }
}

void TCPConnect::disConnect() {
   TRACE();
   if (Sock != SOCK_UNUSED) {
      closeSocket(Sock);
      Sock = SOCK_UNUSED;
      LastContact = time(NULL);
   }
   State = TCP_UNKNOWN;
}

// All close socket calls come here. We do a shutdown first.
// and then really close it. We do it this way, so that shutdown forces
// socket to be shutdown in all the threads/processes that inherit this
// socket.
// We implement the below clean shutdown as follows:
// 1. shutdown with how = SD_SEND.
// 2. call recv() till zero returned or SOCKET_ERROR
// 3. call close().
// No improvement.
void TCPConnect::closeSocket(SOCKET worksock) {
char Target[1];
ssize_t retval;

   TRACE();
#ifdef __MINGW32__
  #define SHUT_RD SD_SEND
  #define SHUT_WR SD_RECEIVE
#endif
   shutdown(worksock, SHUT_RD | SHUT_WR); // I am not going to send anything.

   closesocket(worksock);
}

void TCPConnect::cleanSockets() {
   TRACE();
   if (SockServer != SOCK_UNUSED) {
      closeSocket(SockServer);
      SockServer = SOCK_UNUSED;
      LastContact = time(NULL);
   }
   if (Sock != SOCK_UNUSED) {
      closeSocket(Sock);
      Sock = SOCK_UNUSED;
      LastContact = time(NULL);
   }
   State = TCP_UNKNOWN;
   delete [] Host;
   Host = NULL;
}

// Returns true if can writeData.
// Returns false if cannot right now.
bool TCPConnect::canWriteData() {
fd_set grp;
struct timeval tv;
struct timeval *tvp = &tv;

   TRACE();
   if ( (State != TCP_ESTABLISHED) && (State != TCP_NEGOTIATING) ) return(false);
   if (Sock == SOCK_UNUSED) return(false);

   tv.tv_usec = 0;
   tv.tv_sec = 0;
   FD_ZERO(&grp);
   FD_SET(Sock, &grp);
   int state = select(Sock + 1, NULL, &grp, NULL, tvp);
   if (state == 1) return(true);
   else return(false);
}

bool TCPConnect::isSelectError() {
fd_set grp;
struct timeval tv;
struct timeval *tvp = &tv;

   TRACE();
   if ( (State != TCP_ESTABLISHED) && (State != TCP_NEGOTIATING) ) return(true);
   if (Sock == SOCK_UNUSED) return(true);

   tv.tv_usec = 0;
   tv.tv_sec = 0;
   FD_ZERO(&grp);
   FD_SET(Sock, &grp);
   int state = select(Sock + 1, NULL, NULL, &grp, tvp);
   if (state == 1) return(true);
   else return(false);
}

bool TCPConnect::canReadData() {
fd_set grp;
struct timeval tv;
struct timeval *tvp = &tv;

   TRACE();
   if ( (State != TCP_ESTABLISHED) && (State != TCP_NEGOTIATING) ) return(false);
   if (Sock == SOCK_UNUSED) return(false);

   tv.tv_usec = 0;
   tv.tv_sec = 0;
   FD_ZERO(&grp);
   FD_SET(Sock, &grp);
   int state = select(Sock + 1, &grp, NULL, NULL, tvp);
   if (state == 1) return(true);
   else return(false);
}


// Returns -1 on Error.
// Returns 0 => TimeOut reached and still couldnt write.
ssize_t TCPConnect::writeData(const char *buffer, size_t size, time_t timeout) {
ssize_t nstat;
ssize_t  totaln = 0;
struct timeval tv;
struct timeval *tvp = &tv;
time_t precall = 0;
fd_set grp;
char *Source = (char *) buffer;
size_t size_cause_of_cap;

   TRACE();
   if ( (State != TCP_ESTABLISHED) && (State != TCP_NEGOTIATING) ) return(-1);
   if (Sock == SOCK_UNUSED) {
      COUT(cout << "TCPConnect::writeData Sock == SOCK_UNUSED" << endl;)
      return(-1);
   }
   if (size < 0) return(0);


   while (true) {

      // Bandwidth CAP.
      capSleepTillOKToUpload();

      if (timeout == TIMEOUT_INF) {
         tvp = NULL;
      }
      else {
         tv.tv_usec = 0;
         tv.tv_sec = timeout;
         precall = time(NULL);
      }
      FD_ZERO(&grp);
      FD_SET(Sock, &grp);
      int state = select(Sock + 1, NULL, &grp, NULL, tvp);
      if (state == 0) {
         // Lets update for the 20 sec rolling rate.
         updateUploadArray(0);
         if (totaln) break;
         return(0); // Timeout.
      }
      if (state == 1) {
          // Bandwidth CAP.
          size_cause_of_cap = getUploadSendSizeCauseOfCap(size);

          // Write the stuff in it.
          if (size_cause_of_cap) {
             nstat = send(Sock, Source, size_cause_of_cap, 0);
          }
          else {
             nstat = send(Sock, Source, size, 0);
          }
//          COUT(cout << "writeData: send returned: " << nstat << "sending: " << size << endl;)
          if (nstat < 0) {
             // Lets update for the 20 sec rolling rate.
             updateUploadArray(0);

             COUT(cout << "TCPConnect::writeData send returned " << nstat << endl;)
#ifdef __MINGW32__
             COUT(cout << "TCPConnect::writeData WSAGetLastError: " << WSAGetLastError() << endl;)
#else
             COUT(cout << "TCPConnect::writeData errno: " << errno << " EAGAIN: " << EAGAIN << " ESPIPE: " << ESPIPE << " size: " << size << " timeout: " << timeout << endl;)
#endif
             if (errorNotOK()) {
                if (totaln) break;
                cleanSockets();
                return(-1);
             }
             else {
                // This is an OK error, so we need to continue looping till
                // timeout is exceeded. If we return 0 here, the caller assumes
                // a timeout has occured.
                timeout = updateTimeOut(timeout, precall);
                if (timeout == 0) break;
                continue;
             }
          }
//        send returning 0 is OK as per man page.
          if (nstat == 0) {
             COUT(cout << "writeData: send returned 0" << endl;)
          }

          // Lets update for the 20 sec rolling rate.
          updateUploadArray(nstat);

          LastContact = time(NULL);

          BytesSent += nstat;
          addUploadBytesToClassUploadBps(nstat); // To track overall CAP
          size -= nstat;
          Source += nstat;
          totaln += nstat;
          if ( (size <= 0) || (timeout == 0) ) break;

          timeout = updateTimeOut(timeout, precall);
       }
       else {
          // Lets update for the 20 sec rolling rate.
          // We get an error, so we need to check if its an OK error.
          updateUploadArray(0);

          COUT(cout << "TCPConnect::writeData select returned " << state << " errno: " << errno << endl;)
          if (errorNotOK()) {
             cleanSockets();
             if (totaln) break;
             return(-1);
          }
          else {
             // This is an OK error, so we need to continue looping till
             // timeout is exceeded. If we return 0 here, the caller assumes
             // a timeout has occured.
             timeout = updateTimeOut(timeout, precall);
             if (timeout == 0) break;
             continue;
          }
       }
   }
   LastContact = time(NULL);
   return(totaln);
}

// The first time we wait for data, we wait the full timeout seconds.
// Subsequently we wait 1 second each, so that in the END game of receiving
// a file, we dont wait the full timeout period, expecting data.
// If timeout reached and no data we return 0.
// If socket has closed on other end, recv returns 0. We return with -1.
ssize_t TCPConnect::readData(void *buffer, size_t size, time_t timeout) {
ssize_t nstat;
ssize_t totaln = 0;
struct timeval tv;
struct timeval *tvp = &tv;
time_t precall = 0, postcall = 1;
fd_set grp;
char *Target = (char *) buffer;
size_t size_cause_of_cap;

   TRACE();
   if ( (State != TCP_ESTABLISHED) && (State != TCP_NEGOTIATING) ) return(-1);
   if (Sock == SOCK_UNUSED) return(-1);
   if (size < 1) return(0);

   while (true) {

      // Bandwidth CAP.
      capSleepTillOKToDwnld();

      if (timeout == TIMEOUT_INF) {
         tvp = NULL;
      }
      else {
         tv.tv_usec = 0;
         if (totaln) {
            tv.tv_sec = 1;
         }
         else {
            tv.tv_sec = timeout;
         }
         if (postcall != precall) {
             precall = time(NULL);
         }
      }
      FD_ZERO(&grp);
      FD_SET(Sock, &grp);
      int state = select(Sock + 1, &grp, NULL, NULL, tvp);
      if (state == 0) {
         // Update the Download Array.
         updateDownloadArray(0);

         if (totaln) break;
         return(0);
      }
      if (state == 1) {
         // Bandwidth CAP.
         size_cause_of_cap = getDwnldSendSizeCauseOfCap(size);

         // Read the stuff in it.
         if (size_cause_of_cap) {
            nstat = recv(Sock, Target, size_cause_of_cap, 0);
         }
         else {
            nstat = recv(Sock, Target, size, 0);
         }
         if (nstat <= 0) {
            // Update the Download Array.
            updateDownloadArray(0);

            // nstat == 0 => socket closed on other end.
            if (totaln) break;

            if (errorNotOK()) {
               cleanSockets();
               return(-1);
            }
            else return(0);
         }

         // Update the Download Array.
         updateDownloadArray(nstat);

         LastContact = time(NULL);

         BytesReceived += nstat;
         addDwnldBytesToClassDwnldBps(nstat);  // For overall CAP.
         size -= nstat;
         Target += nstat;
         totaln += nstat;
         if ( (size <= 0) || (timeout == 0) ) break;

         if (timeout != TIMEOUT_INF) {
            postcall = time(NULL);
            if (postcall != precall) {
               if ( timeout <= (postcall - precall) ) {
                  break; // Duration of timeout exceeded.
               }
               timeout = timeout - (postcall - precall);
            }
         }
      }
      else {
         // Error condition
         // Update the Download Array.
         updateDownloadArray(0);

         if (totaln) break;
         if (errorNotOK()) {
            cleanSockets();
            return(-1);
         }
         else return(0);
      }
   }
   LastContact = time(NULL);
   return(totaln);
}

// Assumes that the line ends in 0d 0a, or 0a (\n). 0d = \r
// Doesnt pass the 0d0a to the caller though.
// Terminate buffer always.
// The line shouldnt be bigger than 8K characters.
// If 0 is returned => empty line, or timeout and nothing received.
ssize_t TCPConnect::readLine(char *buffer, size_t size, time_t timeout) {
time_t orig_timeout = timeout;
ssize_t nstat;
size_t  totaln = 0;
struct timeval tv;
struct timeval *tvp = &tv;
time_t precall = 0, postcall = 1;
fd_set grp;
char *Target = (char *) buffer;
bool got_newline = false;

   TRACE();
   Target[0] = '\0';

   if ( (State != TCP_ESTABLISHED) && (State != TCP_NEGOTIATING) ) return(-1);
   if (Sock == SOCK_UNUSED) return(-1);
   if (size < 1) return(0);

   while (true) {
      if (timeout == TIMEOUT_INF) {
         tvp = NULL;
      }
      else {
         tv.tv_usec = 0;
         tv.tv_sec = timeout;
         if (postcall != precall) {
             precall = time(NULL);
         }
      }
      FD_ZERO(&grp);
      FD_SET(Sock, &grp);
      int state = select(Sock + 1, &grp, NULL, NULL, tvp);
      if (state == 0) {
         // Update the Download Array.
         updateDownloadArray(0);

         if (totaln) break;
         return(0);
      }
      if (state == 1) {
//        Read the stuff in it.
          // If select returned 1, and we read 0 => EOF.
          nstat = recv(Sock, Target, 1, 0);
          if (nstat <= 0) {
             // Update the Download Array.
             updateDownloadArray(0);

             if (nstat == 0) {
                // Even if we have recieved a partial line, its still
                // incomplete and the other end has closed socket.
                // So just get it over with.
                cleanSockets();
                return(-1);
             }

             if (totaln) break;
             if (errorNotOK()) {
                cleanSockets();
                return(-1);
             }
             else return(0);
          }

          // Update the Download Array.
          updateDownloadArray(0);

          BytesReceived += nstat;
          size -= nstat;
          totaln += nstat;
          if (Target[0] == 0x0D) Target[0] = '\0';
          else if (Target[0] == 0x0A) {
             Target[0] = '\0';
             got_newline = true;
             break;
          }
          Target += nstat;

          if (size <= 0) break;

          if ( (timeout == 0) && (orig_timeout != 0) ) break;
          if ( (timeout < 0) && (timeout != TIMEOUT_INF) ) break;

          // We are here if orig_timeout is 0 or
          // timeout is +ve. or infinite.

          if ( (orig_timeout != 0) && (orig_timeout != TIMEOUT_INF) ) {
             postcall = time(NULL);
             if (postcall != precall) {
                if ( timeout <= (postcall - precall) ) {
                   break; // Duration of timeout exceeded.
                }
                timeout = timeout - (postcall - precall);
             }
          }
       }
       else {
          // Update the Download Array.
          updateDownloadArray(0);

          if (totaln) break;
          if (errorNotOK()) {
             cleanSockets();
             return(-1);
          }
          else return(0);
       }
   }
   // Terminate the line.
   Target[0] = '\0';

   #define SIZE_OF_PARTIAL_LINE          8192
   if (got_newline) {
      // A New Line was got in previous call, hence concatenate any half
      // ass lines got in previous calls to what we have got now and return.
      // We arent checking for any buffer overwrites here, though should.
      if ( (strlen(PartialLine) + strlen(buffer)) < SIZE_OF_PARTIAL_LINE) {
         // No buffer overflow.
         //COUT(cout << "TCPConnect: PartialLine: " << PartialLine << " will be catted with: " << buffer << endl;)
         strcat(PartialLine, buffer);
      }
      else {
         COUT(cout << "TCPConnect::readLine() Buffer overflow in strcat" << endl;)
      }
      if (size > strlen(PartialLine)) {
         //COUT(cout << "TCPConnect: PartialLine: " << PartialLine << " will be copied into buffer" << endl;)
         // The input buffer can handle this line.
         strcpy(buffer, PartialLine);
         //COUT(cout << "TCPConnect: Buffer after copy from PartialLine " << buffer << endl;)
      }
      else {
         COUT(cout << "TCPConnect::readLine() Buffer overflow in strcpy (got_newline is true)" << endl;)
      }
      PartialLine[0] = '\0';
      totaln = strlen(buffer);
   }
   else {
      // We have received a partial line, and not the full line yet.
      if ( (strlen(buffer) + strlen(PartialLine)) < SIZE_OF_PARTIAL_LINE) {
         strcat(PartialLine, buffer);
         //COUT(cout << "TCPConnect::readLine() cating into PartialLine: " << PartialLine << endl;)
      }
      else {
         COUT(cout << "TCPConnect::readLine() Buffer overflow in strcat (got_newline is false)" << endl;)
      }

      buffer[0] = '\0';
      totaln = 0; // Indicate a time out.
   }

   LastContact = time(NULL);
   return(totaln);
}

// This updates the member ListenAddr with the ip and the port information
// from the Host and Port private members.
// The port part is converted always.
bool TCPConnect::resolveInListenAddr() {

   TRACE();
   memset(&ListenAddr, 0, sizeof(ListenAddr));
   ListenAddr.sin_port = htons(Port);
   if ( strspn(Host, "0123456789.") == strlen(Host) ) {
//    Dotted IP given.
      ListenAddr.sin_family = AF_INET;
      ListenAddr.sin_addr.s_addr = inet_addr(Host);
   }
   else {
   struct hostent *ent;

      COUT(cout << "Calling gethostbyname " << Host << " ";)
      ent = gethostbyname(Host);
      if (ent) {
         memcpy(&(ListenAddr.sin_addr), ent->h_addr, ent->h_length);
         ListenAddr.sin_family = ent->h_addrtype;
         COUT(cout << "Success" << endl;)
      }
      else {
         COUT(cout << "Failed" << endl;)
         return(false);
      }
   }
   return(true);
}

// To help in setting up ListenAddr for an outgoing connect.
// Sets up ip port, and gets a connect.
// We use the private ListenAddr for this connect as well.
bool TCPConnect::connectStep1() {
int sockoptset, sockoptdummy;
struct timeval tv;
struct timeval *tvp = &tv;
fd_set grp;

   TRACE();
   COUT(cout << "connectStep1" << endl;)
   if (resolveInListenAddr() == false) {
      COUT(cout << "resolveInListenAddr failed" << endl;)
      return(false);
   }
   COUT(cout << "after resolv" << endl;)
   SockServer = socket(AF_INET, SOCK_STREAM, 0);
   if (SockServer < 0) {
      SockServer = SOCK_UNUSED;
      return(false);
   }
   COUT(cout << "after socket" << endl;)

   if (SetOptions) {
      // Set SO_SNDBUF
      sockoptdummy = sizeof(sockoptdummy);
      getsockopt(SockServer, SOL_SOCKET, SO_SNDBUF, (char *) &sockoptset, (socklen_t *) &sockoptdummy);
      COUT(cout << "getsockopt: SO_SNDBUF: " << sockoptset << ". Will set it to 65535 if NOT SunOS" << endl;)

#if !defined(_OS_SunOS)
      sockoptset = 65535;
      setsockopt(SockServer, SOL_SOCKET, SO_SNDBUF, (const char *) &sockoptset, sizeof(sockoptset));
#endif
   }
   else {
      COUT(cout << "TCPConnect: not setting options" << endl;)
   }

   // Non Blocking socket so we wait no more that TCPCONNECT_FAIL_TIMEOUT 
   // seconds, for a connect to succeed.
   setNonBlocking(SockServer);

   // Put this socket in the writefds, if its set on return, then connection
   // was successful
   tv.tv_usec = 0;
   tv.tv_sec = TCPCONNECT_FAIL_TIMEOUT;
   FD_ZERO(&grp);
   FD_SET(SockServer, &grp);

   connect(SockServer, (struct sockaddr *) &ListenAddr, sizeof(ListenAddr));

   int state = select(SockServer + 1, NULL, &grp, NULL, tvp);
   // state = 1 => connection successful.
   if (state != 1) {
      COUT(cout << "connect failure: state: " << state << endl;)
      return(false);
   }

#if 0
   if (connect(SockServer, (struct sockaddr *) &ListenAddr, sizeof(ListenAddr)) != 0) {
      COUT(cout << "connect failure" << endl;)
      return(false);
   }
#endif

   COUT(cout << "connect success" << endl;)

   Sock = SockServer;
   SockServer = SOCK_UNUSED;

   return(true);
}

// Negotiating actual connection with Proxy
bool TCPConnect::connectThruProxy() {
char buffer[1024];
ssize_t buflen;
Base64 b64;
int retval;

   TRACE();
   snprintf(buffer, 1020, "CONNECT %s:%d HTTP/1.0\r\n", Host, Port);
   buflen = strlen(buffer);
   COUT(cout << "Step2: buflen: " << buflen << " Buffer: " << buffer;)
   retval = writeData(buffer, buflen, 10);
   if (retval != buflen) {
      return(false);
   }
   if ( (TCPConnectionMethod.getUser() != NULL) && (TCPConnectionMethod.getPassword() != NULL) ) {
//    Have to Authenticate.
      snprintf(buffer, 1020, "%s:%s", TCPConnectionMethod.getUser(), TCPConnectionMethod.getPassword());
      snprintf(buffer, 1020, "Proxy-Authorization: Basic %s\r\n", b64.out());
      if (writeData(buffer, buflen, 10) != buflen) {
         return(false);
      }
   }
   strcpy(buffer, "\r\n");
   buflen = strlen(buffer);
   retval = writeData(buffer, buflen, 10);
   if (retval != buflen) {
      return(false);
   }

// We have sent data to Proxy server. Analyse response.
   while (true) {
      retval = readLine(buffer, 1020, 10);
      if (retval < 1) {
         return(false);
      }
      COUT(cout << "PROXY sent: " << buffer << endl;)
      if (retval > 2) break;
   }
// We have skipped blanked lines and now have the HTTP* XXX line
   // retval = atoi(strchr(buffer,' '));
   retval = (int) strtol(strchr(buffer,' '), NULL, 10);
   switch (retval) {
      case 200:
      //  We got the connection !
         break;

      case 401: // WWW-Auth required.
      case 407: // Proxy-Auth required.
      default:
      // Proxy refused connection.
         return(false);
         break;
   }

// Skip to end of response header.
// We just skip till we get an empty line.
   while (true) {
      retval = readLine(buffer, 1020, 1);
      if (retval == -1) return(false);
      if (retval == 0) break; // Timeout waiting for input.
      COUT(cout << "PROXY sent(discard): " << buffer << endl;)
      if (buffer[0] == '\0') break; // Empty line we were waiting for.
   }
   return(true);
}

// Negotiating actual connection thru Socks4 or Socks4a
bool TCPConnect::connectThruSocks4() {
unsigned char Packet[256], *ptr = Packet;
// make connect request packet
//       protocol v4:
//         VN:1, CD:1, PORT:2, ADDR:4, USER:n, NULL:1
//       protocol v4a:
//         VN:1, CD:1, PORT:2, DUMMY:4, USER:n, NULL:1, HOSTNAME:n, NULL:1
#define SOCKS4_REP_SUCCEEDED    90  // Request granted (succeeded)

   TRACE();
   *ptr++ = 4;              // Protocol version 4.
   *ptr++ = 1;              // CONNECT command.
   *ptr++ = Port >> 8;      // Destination Port.
   *ptr++ = Port & 0xFF;
   resolveInListenAddr();    // Try to resolve Host.
   memcpy(ptr, &ListenAddr.sin_addr, sizeof(ListenAddr.sin_addr));
   ptr += sizeof(ListenAddr.sin_addr);

   if (ListenAddr.sin_addr.s_addr == 0 ) {
      *(ptr-1) = 1;               // fake, protocol 4a
   }
   // User Name
   char *tempstr = TCPConnectionMethod.getUser();
   strcpy((char *) ptr, tempstr);
   ptr += strlen(tempstr) + 1;

   // Destination host name (for protocol 4a)
   if (ListenAddr.sin_addr.s_addr == 0) {
      strcpy((char *) ptr, Host);
      ptr += strlen(Host) + 1;
   }

   // Send command and get response
   // response is: VN:1, CD:1, PORT:2, ADDR:4
   if (writeData((char *) Packet, ptr - Packet, 10) != ptr - Packet) {
      return(false);
   }
   if (readData(Packet, 8, 10) != 8) {
      return(false);
   }
   if (Packet[1] != SOCKS4_REP_SUCCEEDED) {
      return(false);
   }

   return(true);
}

// Negotiating actual connection thru Socks5
bool TCPConnect::connectThruSocks5() {
unsigned char Packet[256], *ptr = Packet;

   TRACE();
// return true after implementing.
   return(false);
}

// Negotiating actual connection thru BNC
bool TCPConnect::connectThruBNC() {
char buffer[1024];
size_t buflen;
int retval;

   TRACE();
// First we send the "NICK <nick>" line.
   snprintf(buffer, sizeof(buffer) - 1, "NICK %s\nUSER %s 32 . :%s\n", TCPConnectionMethod.getUser(), TCPConnectionMethod.getUser(), TCPConnectionMethod.getUser());
   buflen = strlen(buffer);
   COUT(cout << "BNC: writing: " << buffer;)
   if (writeData(buffer, buflen, 10) != buflen) {
      return(false);
   }

   while (true) {
      retval = readLine(buffer, sizeof(buffer) - 1, 10);
      if (retval == -1) return(false);
      if (retval == 0) break; // Timeout waiting for input.
      COUT(cout << "BNC sent(discard): " << buffer << endl;)
   }
   snprintf(buffer, sizeof(buffer) - 1, "PASS %s\n", TCPConnectionMethod.getPassword());
   buflen = strlen(buffer);
   COUT(cout << "BNC: writing: " << buffer;)
   if (writeData(buffer, buflen, 10) != buflen) {
      return(false);
   }
   retval = readLine(buffer, sizeof(buffer) - 1, 10);
   COUT(cout << "BNC: readLine 1st line: retval: " << retval << " got: " << buffer << endl;)
   if (strncasecmp(buffer, "NOTICE AUTH :Welcome", 20) != 0) {
      return(false);
   }
   while (true) {
      retval = readLine(buffer, sizeof(buffer) - 1, 10);
      if (retval == -1) return(false);
      if (retval == 0) break; // Timeout waiting for input.
      COUT(cout << "BNC sent(discard): " << buffer << endl;)
   }

   snprintf(buffer, sizeof(buffer) - 1, "CONN %s %d\n", Host, Port);
   buflen = strlen(buffer);
   COUT(cout << "BNC: writing: " << buffer;)
   if (writeData(buffer, buflen, 10) != buflen) {
      return(false);
   }

   retval = readLine(buffer, sizeof(buffer), 10);
   COUT(cout << "BNC: reading: " << buffer << endl;)
   if (retval < 1) return(false);
   retval = readLine(buffer, sizeof(buffer), 10);
   COUT(cout << "BNC: reading: " << buffer << endl;)
   if (retval < 1) return(false);
   if (strncasecmp(buffer, "NOTICE AUTH :Suceeded connection", 35) == 0) {
      return(true);
   }

   return(false);
}

// Negotiating actual connection thru Wingate
// Wingate Proxy is just like other proxies. May not require this.
bool TCPConnect::connectThruWingate() {
char buffer[1024];
size_t buflen;

   TRACE();
   snprintf(buffer, sizeof(buffer) - 1, "%s %s", Host, Port);
   buflen = strlen(buffer);
   if (writeData(buffer, buflen, 10) != buflen) {
      return(false);
   }

   return(true);
}

// Making an Out going connection. The complicated one.
// If varhost == NULL => internally Host/LongIP and Port should be set.
bool TCPConnect::getConnection(char *varhost, unsigned short varport) {

   TRACE();
   COUT(cout << "Entering getConnection" << endl;)
   if (varhost == NULL) {
// Check if our internal Host/LongIP and Port are valid.
      if ( (Port == 0) || ( (Host == NULL) && (LongIP == 0) ) ) {
         return(false);
      }
      if ((Host == NULL) && (LongIP != 0)) {
//    Let us fill up Host as dotted IP, so rest of code works without mod.
          ListenAddr.sin_addr.s_addr = htonl(LongIP);
          char *tempstr = inet_ntoa(ListenAddr.sin_addr);
          Host = new char[strlen(tempstr) + 1];
          strcpy(Host, tempstr);
      }
   }

// Lets put some valid values in RemoteIP.
   if (Host) {
      RemoteIP = this->getLongFromHostName(Host);
   }
   else if (varhost) {
      RemoteIP = this->getLongFromHostName(varhost);
   }
   else if (LongIP != 0) {
      RemoteIP = LongIP;
   }

   if (TCPConnectionMethod.valid() == false) return(false);
   if (State != TCP_UNKNOWN) return(false);
// The Method is valid and the class is not Connected.

// Delete Host only if its a request not coming from getConnection(void)
   if (varhost != NULL) {
//    Host param is passed in command line, delete existing Host.
      delete [] Host;
      Host = NULL;
   }
   State = TCP_NEGOTIATING;

   char *tmpHost = Host;
   unsigned short tmpPort = Port;

   switch (TCPConnectionMethod.howto()) {

      case CM_DIRECT:
         COUT(cout << "DIRECT" << endl;)
         if (Host == NULL) {
            Host = new char[strlen(varhost) + 1];
            strcpy(Host, varhost);
            Port = varport;
         }
         if (connectStep1() == false) {
            cleanSockets();
            return(false);
         }
         break;

      case CM_BNC:
      case CM_SOCKS4:
      case CM_SOCKS5:
      case CM_WINGATE:
      case CM_PROXY:

         Host = TCPConnectionMethod.getHost();
         Port = TCPConnectionMethod.getPort();
         COUT(cout << "Calling connectStep1" << endl;)
         COUT(cout.flush();)
         if (connectStep1() == false) {
            Host = tmpHost;
            Port = tmpPort;
            cleanSockets();
            return(false);
         }
         Host = tmpHost;
         Port = tmpPort;
         if (Host == NULL) {
            Host = new char[strlen(varhost) + 1];
            strcpy(Host, varhost);
            Port = varport;
         }
//       Established Intermediate connect. Now onto Step 2.
         break;

      default:
         cleanSockets();
         return(false);
   }

   switch (TCPConnectionMethod.howto()) {
      case CM_PROXY:

         if (connectThruProxy() == false) {
            cleanSockets();
            return(false);
         }

//       We have a succesfull connection.
         State = TCP_ESTABLISHED;
         break;

      case CM_SOCKS4:
         if (connectThruSocks4() == false) {
            cleanSockets();
            return(false);
         }
//       We have a succesfull connection.
         State = TCP_ESTABLISHED;
         break;

      case CM_SOCKS5:
         if (connectThruSocks5() == false) {
            cleanSockets();
            return(false);
         }
//       We have a succesfull connection.
         State = TCP_ESTABLISHED;
         break;

      case CM_BNC:
         if (connectThruBNC() == false) {
            cleanSockets();
            return(false);
         }
//       We have a succesfull connection.
         State = TCP_ESTABLISHED;
         break;

      case CM_WINGATE:
         if (connectThruWingate() == false) {
            cleanSockets();
            return(false);
         }
//       We have a succesfull connection.
         State = TCP_ESTABLISHED;
         break;

      case CM_DIRECT:
//       We have a succesfull connection.
         State = TCP_ESTABLISHED;
         break;

   }
   LastContact = time(NULL);
   return(true);
}

void TCPConnect::printDebug() {
   TRACE();
   switch (State) {
      case TCP_UNKNOWN:
         COUT(cout << "TCP: State: UNKNOWN ";)
         break;

      case TCP_LISTENING:
         COUT(cout << "TCP: State: LISTENING ";)
         break;

      case TCP_ACCEPTED:
         COUT(cout << "TCP: State: ACCEPTED ";)
         break;

      case TCP_ESTABLISHED:
         COUT(cout << "TCP: State: ESTABLISHED ";)
         break;
   }
   COUT(cout << "Sock: ";)
   if (Sock == SOCK_UNUSED) COUT(cout << "UNUSED ";)
   else COUT(cout << Sock << " ";)
   COUT(cout << "SockServer: ";)
   if (SockServer == SOCK_UNUSED) COUT(cout << "UNUSED ";)
   else COUT(cout << SockServer;)
   if (Host != NULL) {
      COUT(cout << " HOST: " << Host;)
   }
   COUT(cout << " LongIP: " << LongIP;)
   COUT(cout << " Port: " << Port;)
   COUT(cout << " RemoteIP: " << LongIP;)
   COUT(cout << " RemotePort: " << Port;)
   COUT(cout << " LastContact: " << LastContact << endl;)
   TCPConnectionMethod.printDebug();
}

time_t TCPConnect::getLastContactTime() {
   TRACE();
   return(LastContact);
}

unsigned long TCPConnect::getRemoteIP() {
   TRACE();
   return(RemoteIP);
}

unsigned short TCPConnect::getRemotePort() {
   TRACE();
   return(RemotePort);
}

size_t TCPConnect::getUploadBps() {
size_t rate = 0;
int count = 0;
bool inc_count = false;

   TRACE();
   if (State != TCP_ESTABLISHED) {
      return(0);
   }

   // Update the array. So that it takes care of idle line.
   updateUploadArray(0);

   for (int i = 0; i < 20; i++) {
      if (UploadArray[i] != 0) {
         rate = rate + UploadArray[i];
         inc_count = true;
      }
      // We only dont count the consecutive 0's at start
      if (inc_count) count++;
//      COUT(cout << UploadArray[i] << " ";)
   }
//   COUT(cout << endl;)
   if (count == 0) count = 1;
   rate = rate / count;
   return(rate);
}

size_t TCPConnect::getDownloadBps() {
size_t rate = 0;
int count = 0;
bool inc_count = false;

   TRACE();
   if (State != TCP_ESTABLISHED) {
      return(0);
   }

   // Update the array. So that it takes care of idle line.
   updateDownloadArray(0);

   for (int i = 0; i < 20; i++) {
      if (DownloadArray[i] != 0) {
         rate = rate + DownloadArray[i];
         inc_count = true;
      }
      // We only dont count the consecutive 0's at start
      if (inc_count) count++;
//      COUT(cout << DownloadArray[i] << " ";)
   }
//   COUT(cout << endl;)
   if (count == 0) count = 1;
   rate = rate / count;
   return(rate);
}

#if 0
// To lock the class so it wont get destroyed.
void TCPConnect::lock() {

   TRACE();
   WaitForMutex(Mutex_TCPConnect);
}

// To unlock the class so it can be destroyed.
void TCPConnect::unlock() {

   TRACE();
   ReleaseMutex(Mutex_TCPConnect);
}
#endif

// To set or reset the bool SetOptions.
void TCPConnect::setSetOptions(bool val) {

   TRACE();

   SetOptions = val;
}

// Update for the 20 sec rolling rate. (torrent)
// Updates UploadLastMarker and UploadArray.
void TCPConnect::updateUploadArray(ssize_t nstat) {
time_t Cur_time;
time_t time_delta;
int i;

   TRACE();

   Cur_time = time(NULL);
   time_delta = Cur_time - UploadLastMarker;
   if (time_delta < 0) {
      time_delta = 0;
      UploadLastMarker = Cur_time;
   }

   if (time_delta == 0) {
   }
   else {
      UploadLastMarker = Cur_time;
      if (time_delta > 19) time_delta = 20;
      for (i = time_delta; i < 20; i++) {
         UploadArray[i - time_delta] = UploadArray[i];
      }
      for (i = 0; i < time_delta; i++) {
         UploadArray[19 - i] = 0;
      }
   }

   UploadArray[19] += nstat;
}

// Update for the 20 sec rolling rate. (torrent)
// Updates DownloadLastMarker and DownloadArray.
void TCPConnect::updateDownloadArray(ssize_t nstat) {
time_t Cur_time;
time_t time_delta;
int i;

   TRACE();

   Cur_time = time(NULL);
   time_delta = Cur_time - DownloadLastMarker;
   if (time_delta < 0) {
      time_delta = 0;
      DownloadLastMarker = Cur_time;
   }
   if (time_delta == 0) {
   }
   else {
      DownloadLastMarker = Cur_time;
      if (time_delta > 19) time_delta = 20;
      for (i = time_delta; i < 20; i++) {
         DownloadArray[i - time_delta] = DownloadArray[i];
      }
      for (i = 0; i < time_delta; i++) {
         DownloadArray[19 - i] = 0;
      }
   }
   DownloadArray[19] += nstat;
}

// Average Upload Transfer speed.
size_t TCPConnect::getAvgUploadBps() {
size_t speed;
time_t CurrentTime = time(NULL);

   TRACE();

   // Speed = BytesSent / (CurrentTime - Born)
   if (CurrentTime <= Born) {
      speed = BytesSent;
   }
   else {
      speed = BytesSent / (CurrentTime - Born);
   }
   return(speed);
}

// Average Download Transfer speed.
size_t TCPConnect::getAvgDownloadBps() {
size_t speed;
time_t CurrentTime = time(NULL);

   TRACE();

   // Speed = BytesReceived / (CurrentTime - Born)
   if (CurrentTime <= Born) {
      speed = BytesReceived;
   }
   else {
      speed = BytesReceived / (CurrentTime - Born);
   }
   return(speed);
}

// CAP Per Object Max Upload Bps
// Sanitizes it as its setting based on OverallMaxUploadBps
void TCPConnect::setMaxUploadBps(size_t cap) {
   TRACE();

   if ( (cap != 0) && (cap < MIN_CAP_VALUE_BPS) ) return;

   WaitForMutex(Mutex_TCPConnect);
   MaxUploadBps = cap;
   if ( (OverallMaxUploadBps != 0) &&
        (MaxUploadBps > OverallMaxUploadBps) ){
      MaxUploadBps = OverallMaxUploadBps;
   }
   ReleaseMutex(Mutex_TCPConnect);
}

size_t TCPConnect::getMaxUploadBps() {
   TRACE();
   return(MaxUploadBps);
}

// CAP Per Object Max download Bps
// Sanitizes it as its setting based on OverallMaxDwnldBps
void TCPConnect::setMaxDwnldBps(size_t cap) {
   TRACE();

   if ( (cap != 0) && (cap < MIN_CAP_VALUE_BPS) ) return;

   WaitForMutex(Mutex_TCPConnect);
   MaxDwnldBps = cap;
   if ( (OverallMaxDwnldBps != 0) &&
        (MaxDwnldBps > OverallMaxDwnldBps) ){
      MaxDwnldBps = OverallMaxDwnldBps;
   }
   ReleaseMutex(Mutex_TCPConnect);
}

size_t TCPConnect::getMaxDwnldBps() {
   TRACE();
   return(MaxDwnldBps);
}

// CAP across all Objects for Max Upload Bps
// Sanitizes MaxUploadBps as its being set. (only if it was set non zero)
void TCPConnect::setOverallMaxUploadBps(size_t cap) {
   TRACE();

   if ( (cap != 0) && (cap < MIN_CAP_VALUE_BPS) ) return;

   WaitForMutex(Mutex_TCPConnect);
   OverallMaxUploadBps = cap;
   if ( (cap != 0) &&
        (MaxUploadBps > OverallMaxUploadBps) ) {
      MaxUploadBps = OverallMaxUploadBps;
   }
   ReleaseMutex(Mutex_TCPConnect);
}

size_t TCPConnect::getOverallMaxUploadBps() {
size_t cap;

   TRACE();

   WaitForMutex(Mutex_TCPConnect);
   cap = OverallMaxUploadBps;
   ReleaseMutex(Mutex_TCPConnect);

   return(cap);
}

// CAP across all Objects for Max Dwnld Bps
// Sanitizes MaxDwnldBps as its being set. (only if it was set non zero)
void TCPConnect::setOverallMaxDwnldBps(size_t cap) {
   TRACE();

   if ( (cap != 0) && (cap < MIN_CAP_VALUE_BPS) ) return;

   WaitForMutex(Mutex_TCPConnect);
   OverallMaxDwnldBps = cap;
   if ( (cap != 0) &&
        (MaxDwnldBps > OverallMaxDwnldBps) ) {
      MaxDwnldBps = OverallMaxDwnldBps;
   }
   ReleaseMutex(Mutex_TCPConnect);
}

size_t TCPConnect::getOverallMaxDwnldBps() {
size_t cap;

   TRACE();

   WaitForMutex(Mutex_TCPConnect);
   cap = OverallMaxDwnldBps;
   ReleaseMutex(Mutex_TCPConnect);
   return(OverallMaxDwnldBps);
}

// We might want to improve on this by not allowing it to sleep for
// more than 1 or 2 seconds.
// First sleep for class CAP. (across all objects)
// Then sleep for object CAP
void TCPConnect::capSleepTillOKToUpload() {
int loop_count = 0;
size_t holder_ClassUploadBps;
size_t holder_OverallMaxUploadBps;

   TRACE();
   if (MonitorUploadForCap == false) return;

   // Class wide UploadBps CAP
   do {
      WaitForMutex(Mutex_TCPConnect);
      holder_ClassUploadBps = ClassUploadBps;
      holder_OverallMaxUploadBps = OverallMaxUploadBps;
      ReleaseMutex(Mutex_TCPConnect);

      // Get out of this loop if Overall CAP not set.
      if (holder_OverallMaxUploadBps == 0) break;

      if (holder_ClassUploadBps >= holder_OverallMaxUploadBps) {
         // We sleep for quarter of a second to slow down.
         msleep(250);

         loop_count++;
         // if (loop_count > 4) return;

         // Call the routine which will update ClassUploadBps
         // This is to get ClassUploadBps, which decides to quit the loop.
         addUploadBytesToClassUploadBps(0);
      }
      else break;

   } while (holder_ClassUploadBps >= holder_OverallMaxUploadBps);

   // UploadBps CAP.
   while ( (MaxUploadBps != 0) &&
           (getUploadBps() >= MaxUploadBps) ) {
      // We sleep for quarter of a second to slow down.
      msleep(250);

      loop_count++;
      // if (loop_count > 4) return;
   }
   // COUT(cout << "TCPConnect::capSleepTillOKToUpload slept for " << loop_count << " times 250 milliseconds" << endl;)
}

// We might want to improve on this by not allowing it to sleep for
// more than 1 or 2 seconds.
// First sleep for class CAP. (across all objects)
// Then sleep for object CAP
void TCPConnect::capSleepTillOKToDwnld() {
int loop_count = 0;
size_t holder_ClassDwnldBps;
size_t holder_OverallMaxDwnldBps;

   TRACE();
   if (MonitorDwnldForCap == false) return;

   // Class wide DwnldBps CAP
   do {
      WaitForMutex(Mutex_TCPConnect);
      holder_ClassDwnldBps = ClassDwnldBps;
      holder_OverallMaxDwnldBps = OverallMaxDwnldBps;
      ReleaseMutex(Mutex_TCPConnect);

      // Get out of this loop if Overall CAP not set.
      if (holder_OverallMaxDwnldBps == 0) break;

      if (holder_ClassDwnldBps >= holder_OverallMaxDwnldBps) {
         // We sleep for quarter of a second to slow down.
         msleep(250);

         loop_count++;
         // if (loop_count > 4) return;

         // Call the routine which will update ClassDwnldBps
         // This is to get ClassDwnldBps, which decides to quit the loop.
         addDwnldBytesToClassDwnldBps(0);
      }
      else break;

   } while (holder_ClassDwnldBps >= holder_OverallMaxDwnldBps);

   // DwnldBps CAP.
   while ( (MaxDwnldBps != 0) &&
           (getDownloadBps() >= MaxDwnldBps) ) {
      // We sleep for quarter of a second to slow down.
      msleep(250);

      loop_count++;
      // if (loop_count > 4) return;
   }
}

// Returns the size to be used for send, considering
// both Per Transfer CAP and Overall CAP.
// 0 returned => no cap, can use the size sent as parameter.
size_t TCPConnect::getUploadSendSizeCauseOfCap(size_t size) {
size_t size_cause_of_cap = 0;
size_t check_size;

   TRACE();
   if (MonitorUploadForCap == false) return(0);

   // The size should be determined against the smaller of Object CAP
   // and overall Class CAP.
   // We already sanitize the caps, and hence MaxUploadBps <= Overall
   check_size = MaxUploadBps;

   WaitForMutex(Mutex_TCPConnect);
   if (check_size == 0) check_size = OverallMaxUploadBps;
   ReleaseMutex(Mutex_TCPConnect);

   if ( (check_size != 0) && (size >= check_size) ) {
      size_cause_of_cap = check_size / 2 + 1;
      // The + 1, is so that we dont end up doing 3 loops when we can be done
      // with 2. Example 7, will be 4, 3. Otherwise it would have been
      // 3, 3, 1
   }

   return(size_cause_of_cap);
}

// Returns the size to be used for receiving, considering
// both Per Transfer CAP and Overall CAP.
// 0 returned => no cap, can use the size sent as parameter.
size_t TCPConnect::getDwnldSendSizeCauseOfCap(size_t size) {
size_t size_cause_of_cap = 0;
size_t check_size;

   TRACE();

   if (MonitorDwnldForCap == false) return(0);

   // The size should be determined against the smaller of Object CAP
   // and overall Class CAP.
   // We already sanitize the caps, and hence MaxDwnldBps <= Overall
   check_size = MaxDwnldBps;

   WaitForMutex(Mutex_TCPConnect);
   if (check_size == 0) check_size = OverallMaxDwnldBps;
   ReleaseMutex(Mutex_TCPConnect);

   if ( (check_size != 0) && (size >= check_size) ) {
      size_cause_of_cap = check_size / 2 + 1;
      // The + 1, is so that we dont end up doing 3 loops when we can be done
      // with 2. Example 7, will be 4, 3. Otherwise it would have been
      // 3, 3, 1
   }

   return(size_cause_of_cap);
}

// Add the current bytes uploaded to the class wide ClassUploadBps
// We have to do this all the time, to take care of the calls from
// TimerThr, which decides if the overall send cps is low.
// And we add the bytes up only if they are monitored.
void TCPConnect::addUploadBytesToClassUploadBps(size_t bytes) {
time_t cur_time;

   TRACE();

   if (MonitorUploadForCap == false) return;

   WaitForMutex(Mutex_TCPConnect);
   cur_time = time(NULL);
   if (cur_time != ClassUploadCurrentTime) {
      // Save the rollover into the last uploadcps variables.
      LastClassUploadTime = ClassUploadCurrentTime;
      LastClassUploadBps = ClassUploadBps;

      ClassUploadCurrentTime = cur_time;
      ClassUploadBps = 0;
   }
   ClassUploadBps += bytes;
   ReleaseMutex(Mutex_TCPConnect);
}

// Add the current bytes uploaded to the class wide ClassDwnldBps
// Only does the work if a Class Wide Download CAP is set
void TCPConnect::addDwnldBytesToClassDwnldBps(size_t bytes) {
time_t cur_time;
size_t holder_OverallMaxDwnldBps;

   TRACE();
   if (MonitorDwnldForCap == false) return;

   WaitForMutex(Mutex_TCPConnect);
   holder_OverallMaxDwnldBps = OverallMaxDwnldBps;
   ReleaseMutex(Mutex_TCPConnect);

   if (holder_OverallMaxDwnldBps == 0) return;

   WaitForMutex(Mutex_TCPConnect);
   cur_time = time(NULL);
   if (cur_time != ClassDwnldCurrentTime) {
      ClassDwnldCurrentTime = cur_time;
      ClassDwnldBps = 0;
   }
   ClassDwnldBps += bytes;
   ReleaseMutex(Mutex_TCPConnect);
}

// Modify the Download CAPing options for the object.
void TCPConnect::monitorForDwnldCap(bool monitor_flag) {
   TRACE();

   MonitorDwnldForCap = monitor_flag;
}

// Modify the Upload CAPing options for the object.
void TCPConnect::monitorForUploadCap(bool monitor_flag) {
   TRACE();

   MonitorUploadForCap = monitor_flag;
}

// check if the error encountered can be retried.
// returns true if cannot be retried.
bool TCPConnect::errorNotOK() {
#ifdef __MINGW32__
int retval;

   TRACE();

   retval = WSAGetLastError();
   if ( (retval == WSAEWOULDBLOCK) || (retval == WSAEINPROGRESS) ) {
      return(false);
   }
   else {
      return(true);
   }

#else
   if ( (errno == EAGAIN) || (errno == EINPROGRESS) ) {
      return(false);
   }
   else {
      return(true);
   }
#endif
}

// Returns the last monitored time and its cps.
// value for the whole class.
size_t TCPConnect::getLastUploadBpsTime(time_t *time_ptr) {
size_t bps;
time_t bps_time;

   TRACE();

    WaitForMutex(Mutex_TCPConnect);
    bps = LastClassUploadBps;
    bps_time = LastClassUploadTime;
    ReleaseMutex(Mutex_TCPConnect);
    *time_ptr = bps_time;
    return(bps);
}

// Precall is the time that was noted before a system call.
// timeout is adjusted to remaining time left and returned.
time_t TCPConnect::updateTimeOut(time_t timeout, time_t precall) {
time_t postcall;

   TRACE();

   if (timeout != TIMEOUT_INF) {
      postcall = time(NULL);
      if (postcall != precall) {
         if ( timeout <= (postcall - precall) ) {
            // Duration of timeout exceeded.
            timeout = 0;
         }
         else {
            timeout = timeout - (postcall - precall);
         }
      }
   }
   return(timeout);
}
