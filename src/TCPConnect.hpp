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

#ifndef CLASS_TCPCONNECT
#define CLASS_TCPCONNECT

#include <sys/types.h>
#ifdef __MINGW32__
   #include <winsock2.h>
   #include <ws2tcpip.h>
#else
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <unistd.h>
   #include <arpa/inet.h>
   #include <netdb.h>
#endif

#include <time.h>
#include "ConnectionMethod.hpp"
#include "Compatibility.hpp"

// Used in Unix, to specify how long a connect call can take, before failing.
#define TCPCONNECT_FAIL_TIMEOUT      15

typedef enum {
   TCP_UNKNOWN,
   TCP_LISTENING,
   TCP_ACCEPTED,
   TCP_NEGOTIATING,
   TCP_ESTABLISHED
} TCPState;

const SOCKET SOCK_UNUSED = ~((SOCKET) 0);
const time_t TIMEOUT_INF = ~((time_t) 0);

class TCPConnect {
public:
   TCPConnect();
   ~TCPConnect();
   TCPConnect& operator=(TCPConnect&);
   bool serverInit(unsigned short varport);
   bool getConnection(time_t TimeoutSecs); // Server accept.
   void disConnect();
   bool getConnection(char *varhost = NULL, unsigned short varport = 0); // Client.
// Client connect with already set host/ip/port when param passed is NULL
// as when called as getConnection();

// The setHost, setLongIP, setPort is only possible if its in state TCP_UNKNOWN
// and class is going to be used for an outgoing connect.
   void setHost(char *varhost);
   void setLongIP(unsigned long varip);
   void setPort(unsigned short varport);

// To modify the internal bool SetOptions
   void setSetOptions(bool val);

   // Modify the CAPing options for the object.
   void monitorForDwnldCap(bool);
   void monitorForUploadCap(bool);

// Helper functions
   void getDottedIpAddressFromLong(unsigned long ip, char *DottedHost);
   unsigned long getLongFromHostName(char *thehost);

   TCPState state();

// Return time of last contact as time_t.
   time_t getLastContactTime();

   unsigned long getRemoteIP();
   unsigned short getRemotePort();

// returns -1 if error in socket.
   ssize_t readLine(char *buf, size_t len, time_t TimeoutSecs = TIMEOUT_INF);
   ssize_t readData(void *Target, size_t len, time_t TimeoutSecs = TIMEOUT_INF);
   ssize_t writeData(const char *buf, size_t len, time_t TimeoutSecs = TIMEOUT_INF);
   bool canWriteData();
   bool canReadData();
   bool isSelectError();
   size_t getUploadBps(); // Based on a 20 sec rollover window
   size_t getDownloadBps(); // Based on a 20 sec rollover windows

   size_t getAvgUploadBps(); // Average Upload Transfer speed.
   size_t getAvgDownloadBps(); // Average Download Transfer speed.

   // CAP per Object methods.
   void setMaxUploadBps(size_t);
   size_t getMaxUploadBps();
   void setMaxDwnldBps(size_t);
   size_t getMaxDwnldBps();

   // CAP across all Object methods.
   void setOverallMaxUploadBps(size_t);
   size_t getOverallMaxUploadBps();
   void setOverallMaxDwnldBps(size_t);
   size_t getOverallMaxDwnldBps();

   // Returns the last monitored time and its cps.
   // value for the whole class.
   size_t getLastUploadBpsTime(time_t *);

#if 0
   void lock(); // To lock the class so it wont get destroyed.
   void unlock(); // To release the lock.

   // Used after lock(), and before calling getUploadBPS() or getDownloadBPS()
   // to avoid crash when object no longer exists.
   bool isValid();
#endif

   void printDebug();
   ConnectionMethod TCPConnectionMethod;

   time_t Born;
   size_t BytesSent;
   size_t BytesReceived;

private:
   SOCKET Sock;
   SOCKET SockServer;
   struct sockaddr_in ListenAddr;
   TCPState State;
   char *Host;
   unsigned long LongIP;
   unsigned short Port;
   unsigned long RemoteIP;
   unsigned short RemotePort;
   time_t LastContact;
   time_t UploadLastMarker;
   time_t DownloadLastMarker;
   size_t *UploadArray;
   size_t *DownloadArray;

   // To put together partial lines.
   char *PartialLine;

   // true -> set options (default), false => dont. (used for non transfers)
   bool   SetOptions;

   // Flags which denotes what are being monitored for CAP.
   bool   MonitorUploadForCap;
   bool   MonitorDwnldForCap;

   // CAP Global Max Bps, across all objects.
   static size_t OverallMaxUploadBps;
   static size_t OverallMaxDwnldBps;

   // To track all upload bps and download bps across all objects.
   // Updated by holding mutex. Used for overall CAP on up/down.
   static time_t ClassUploadCurrentTime;
   static time_t ClassDwnldCurrentTime;
   static size_t ClassUploadBps;
   static size_t ClassDwnldBps;

   // To track the upload of the last rolled over second.
   static time_t LastClassUploadTime;
   static size_t LastClassUploadBps;

   // CAP Per Object Max Bps
   size_t MaxUploadBps;
   size_t MaxDwnldBps;

   void setNonBlocking(SOCKET varsocket);
   void closeSocket(SOCKET varsocket);
   void cleanSockets();
   bool connectStep1();
   bool connectThruProxy();
   bool connectThruSocks4();
   bool connectThruSocks5();
   bool connectThruBNC();
   bool connectThruWingate();
   bool resolveInListenAddr();
   void updateUploadArray(ssize_t nstat);
   void updateDownloadArray(ssize_t nstat);
   time_t updateTimeOut(time_t timeout, time_t precall);

   // check if the error encountered can be retried.
   // returns true if cannot be retried.
   bool errorNotOK();

   // Returns the size to be used for send and recv respectively, considering
   // both Per Transfer CAP and Overall CAP.
   // 0 returned => no cap, can use the size sent as parameter.
   size_t getUploadSendSizeCauseOfCap(size_t size);
   size_t getDwnldSendSizeCauseOfCap(size_t size);

   // sleep till its ok to send/recv respectively.
   // Considers both Per Transfer CAP and overall CAP.
   void capSleepTillOKToUpload();
   void capSleepTillOKToDwnld();

   // Maintaining the ClassUploadBps/ClassDwnldBps with MUTEX
   void addUploadBytesToClassUploadBps(size_t bytes);
   void addDwnldBytesToClassDwnldBps(size_t bytes);

#if 0
// A global mutex to stop it from being destroyed, when someone wants to
// use it.
   static MUTEX Mutex_TCPConnect;
   static int TCPConnectClassCount; // Used to determine when to destroy it.
   static TCPConnect **ValidTCP;
#endif

   // Class wide mutex object for synchronisation across all objects
   static MUTEX Mutex_TCPConnect;

   // Used by all ports to determine only once initialisation.
   static bool WSADATA_Initialized;
};


#endif
