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

#ifndef CLASS_HELPER
#define CLASS_HELPER

#include "XChange.hpp"
#include "ThreadMain.hpp"
//#include <fx.h>

// Config file related labels.
#define CONFIG_FILE                        "MasalaMate.cfg"
#define CONFIG_IRC_SECTION                 "IRC"
#define CONFIG_IRC_NICK                    "Nick"
#define CONFIG_IRC_NICK_PASSWORD           "Password"
#define CONFIG_IRC_TRAY_PASSWORD           "TrayPassword"

#define CONFIG_IRC_CHANNEL_SECTION         "Channel"
#define CONFIG_IRC_CHANNEL                 "Channel"

#define CONFIG_CONNECTION_SECTION          "CONNECTION"
#define CONFIG_CONNECTION_HOW              "How"
#define CONFIG_CONNECTION_HOST             "Host"
#define CONFIG_CONNECTION_PORT             "Port"
#define CONFIG_CONNECTION_USER             "User"
#define CONFIG_CONNECTION_PASSWORD         "Password"
#define CONFIG_CONNECTION_VHOST            "VHost"

#define CONFIG_FSERV_SECTION               "FServe"
#define CONFIG_FSERV_QUEUE                 "Queue"
#define CONFIG_FSERV_QUEUES_OVERALL        "QueuesOverall"
#define CONFIG_FSERV_QUEUES_USER           "QueuesUser"
#define CONFIG_FSERV_SENDS_OVERALL         "SendsOverall"
#define CONFIG_FSERV_SENDS_USER            "SendsUser"
#define CONFIG_FSERV_SMALL_FILE_SIZE       "SmallFileSize"
#define CONFIG_FSERV_RECORD_DOWNLD_BPS     "RecordDownloadBPS"
#define CONFIG_FSERV_RECORD_UPLOAD_BPS     "RecordUploadBPS"
#define CONFIG_FSERV_TOTAL_BYTES_SENT      "TotalBytesSent"
#define CONFIG_FSERV_TOTAL_BYTES_RCVD      "TotalBytesRcvd"
#define CONFIG_FSERV_PARTIAL_DIR           "PartialDir"
#define CONFIG_FSERV_SERVING_DIR           "ServingDir"

#define CONFIG_CAP_SECTION                 "Cap"
#define CONFIG_CAP_UPLOAD_OVERALL_MAX      "UploadOverallMax"
#define CONFIG_CAP_UPLOAD_OVERALL_MIN      "UploadOverallMin"
#define CONFIG_CAP_UPLOAD_EACH_MAX         "UploadEachMax"
#define CONFIG_CAP_DOWNLOAD_OVERALL_MAX    "DownloadOverallMax"
#define CONFIG_CAP_DOWNLOAD_EACH_MAX       "DownloadEachMax"

#define CONFIG_FONT_SECTION                "Font"
#define CONFIG_FONT_FACE                   "Face"
#define CONFIG_FONT_SIZE                   "Size"

#define CONFIG_PARTIAL_SECTION             "Partial"
#define CONFIG_PARTIAL_PARTIAL             "Partial"
#define CONFIG_PARTIAL_MAXPARTIALS         100

#define CONFIG_WAITING_SECTION             "Waiting"
#define CONFIG_WAITING_WAITING             "Waiting"
#define CONFIG_WAITING_MAXWAITINGS         100


// Max number of filenames we will hold in MyFilesDB. Used in
// generateMyFilesDB()
#define MAX_FSERV_FILENAMES                1000

// Return value from Helper::stopImbalancedSends()
#define SEND_FROM_ANY_QUEUE               -1
#define SEND_FROM_SMALL_QUEUE              0
#define SEND_FROM_BIG_QUEUE                1

// Imabalance algo cancels a send max once in this time - 3 minutes as of now.
#define IMBALANCE_ALGORITHM_TIME           180

// A Helper Class which has miscellaneous functions
class Helper {
public:
   Helper();
   ~Helper();
   void init(XChange *X);
   void init(XChange *X, TCPConnect *T);

// Below are related to File Server.
   void generateFServAd(char *buffer);
   void generateFindHit(char *searchstr, char *buffer);
   int getSendsAndTotalUploadSpeed(float *speed);
   int getGetsAndTotalDownloadSpeed(float *speed);
   int getTotalQueues();

   // Below is related to update the RecordDownloadBPS, and as a side effect
   // move the Upload CAPS around if they are set.
   // We update the record, if record is higer than what we have currently.
   // If adjust is false, cap is sanitized only if a new record is being set.
   // If adjust is true, cap is sanitized even if a new record is not set.
   // used to sanitize entry input by user by /cap command.
   void updateRecordDownloadAndAdjustUploadCaps(size_t record, bool adjust);

   // Gets the sum of the UploadBPS's from the SendsInProgress.
   // Used by TimerThr, to maintain OverallMinBPS.
   size_t getCurrentOverallUploadBPSFromSends();

   // More functions used by TimerThr.
   // Converts SEND_FROM_SMALL_QUEUE, SEND_FROM_BIG_QUEUE to boolean SmallQ
   bool convertQueueIndicatorToSmallQueueBoolean(int q_indicator);

   // Check if there are open sends and if so push a send out from the
   // Q dictated by SmallQ.
   void checkAndNormalSendFromQueue(bool SmallQ, time_t ChannelJoinTime);

   // Check if we are consistently below OverallMinUploadBPS, and if so
   // push a send out.
   void checkForOverallMinCPSAndSendFromQueue(bool SmallQ, time_t ChannelJoinTime);

   // Check if we need to push a Manual Send or File Push out.
   void checkAndManualSendFromQueue(time_t ChannelJoinTime);

   // Used by FromServerThr.
   void removeNickFromAllFDs(char *nick);

// Below is related to dcc sending a file.
// Sends the file as per FD which is in QueuesInProgress
// SmallQ true => SmallQueueInProgress, else QueueInProgress.
   THR_HANDLE dccSend(FilesDetail *FD, bool SmallQ);

   // Returns a suitable FD from QueuesInProgress which is good for a Send.
   // This can then be passes to dccSend()
   FilesDetail *getSuitableQueueForSend(bool SmallQ);

   // Stops an imbalanced send if a suitable Q is waiting to be sent.
   int stopImbalancedSends();

// Used to get fully qualified filename with PATH given an FD.
// Generates it considering DownloadState, ManualSend. Used before spawning
// TransferThr.
   char *getFilenameWithPathFromFD(FilesDetail *FD);

// Used by FromUIThr.cpp
   void readConfigFile();
   void writeIRCConfigFile();
   void writeConnectionConfigFile();
   void writeWaitingConfig();
   void writePartialConfig();
   void writeFServParamsConfig();
   void writeCapConfig();
   void writeFServeConfig();
   void writeFontConfigFile();
   void writeIRCChannelConfig();

// Used to generate Files in MyFilesDB.
   void generateMyFilesDB();

// Used to generate Files in MyPartialFilesDB
   void generateMyPartialFilesDB();

// Used to move a file from PartialDir to ServingDir, if normal is true.
// if normal = false, this is a move from PartialDir to run position (upgrade)
   bool moveFile(char *filename, bool normal);

// Used by TimerThr to process IRC_ToUpgrade Queue if any need be.
   void processUpgrade();

// Calls, XGlobal->putLineIRC_ToServer(), only if nick is present in one
// of the channels.
   void sendLineToNick(char *nick, char *Line);

// Below functions are used by FileServer and DCCChatClient
   bool helperFServNicklist(int FFLCversion);
   bool helperFServFilelist(char *varnick, char dstate); //dstate = S or P
   // if server is false, we are client, use FFLCversion as the FFLC version
   // of the server we are talking to.
   // unused if we are server, cause we have already announced what FFLC
   // version that we support in the welcome message.
   bool helperFServStartEndlist(char *RemoteNick, bool server, int FFLCversion);
   void helperFFLCStatistics(char *RemoteNick, int FFLCversion);

   // Used by FromServerThr() to handle IC_PRIVMSG and IC_NOTICE of XDCC
   // File Listings.
   void processXDCCListing(char *xdcc_line);

   // Used by FromServerThr() to handle IC_MODE, IC_JOIN, IC_NICKLIST
   // events in CHANNEL_MM
   void markAsMMClient(char *nick);
   void doOpDutiesIfOpInChannelMM(char *our_nick);

   // Used by FromServerThr() to calculate the Next Ad Time
   // ADSYNC algo.
   void calculateFServAdTime(char *ad_nick, char *our_nick);

   // Used by TabBookWindow, to select the channel where !list should
   // be done for the given nick.
   // apt_channel is assumed to be allocated by caller and has enough space.
   void getAptChannelForNickToIssueList(char *nick, char *window_name, char *apt_channel);

   // Used to generated the <<mode>Nick> string.
   // For ease of color coding the < ... > section in text.
   void generateColorCodedNick(const char *Channel, const char *Nick, char ColorCodedNick[]);

   // Get the SwarmIndex of Filename being swarmed.
   // Returns -1 if doesnt match any Swarm.
   int getSwarmIndexGivenFileName(const char *FileName);

   // Used by SwarmThr/DCCServerThr/DCCThr to do swarm HandShake.
   // Does the HS and if successful calls nicklistReadWriteSwarmConnection
   // Return true if successful Handshake. (Connection object in SwarmNodeList)
   // false otherwise. (Connection object deleted)
   bool handshakeWriteReadSwarmConnection(int SwarmIndex, char *SwarmNick, TCPConnect *Connection);

   // Called by handshake routines on successful HS, to go ahead and
   // exchange the NL List/Message.
   // if true returned -> successful exchange.
   // if false then some problem.
   // Note that it populates ToBeTried from the NL obtained.
   bool nicklistWriteReadSwarmConnection(int SwarmIndex, char *SwarmNick, TCPConnect *Connection);

private:

   // Below is called by the helperFServ functions and used by them.
   bool fservSendMessage();
   bool fservRecvMessage();
   TCPConnect *Connection;
   char *RetPointer;
   char *MyNick;


   // Below is called by generateMyFilesDB() - and private.
   // index is the ServingDir[index] which is being used.
   void recurGenerateMyFilesDB(char *nick, char *basedir, char *dir, int ServingDirIndex); 
   int CountFilesInMyFilesDB;

   XChange *XGlobal;
};

#endif
