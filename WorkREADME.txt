Done:
=====
a) The Exit from GUI is not clean. It SEGVs on destructors. So as of 
   now am having a brute force exit(0) on a QUIT from GUI. 
   This is outdated => d) is used now.
b) Hang at start - fixed - check comments in XChange.cpp:registerThread()
c) Implemented /me
d) Exit from GUI revisited.
	All threads, when reading from the Q, immediately on getting data.
   should check first if QUIT is triggered. If so, then quit.
   So when we QUIT from GUI, we populate all the queues with a dummy message.

e) Created a ToTrigger Thread. It basically reads from the
   ToTrigger Q. This Q is populated by the FromServerThr on channel messages 
   which are potential xdcc file messages or FServe triggers.
   The ToTrigger Thread actions will be as below:
   If its a xdcc file line -> update FilesList structure.
   If its a FServ trigger ->
	Check if its already in FServWaitingNicks. If so, ignore.
	If not, add it in FServWaitingNicks, and issue the trigger,
	by populating, toServerQ (slow drain)
        sleep 2 minutes.
   Note: FServWaitingNicks class is used only by the ToTrigger Thread,
   and is not manipulated by any other outside Thread -> need not have
   mutexes or be available globally in XChange.

f) Have to implement a FServParse class which will parse
   the line as FServ or XDCC or valid etc.
   Make a test case for this and test it first.

g) Implemented a SpamFilter class. To not show spam related PRIVMSGs.
   It basically will hold a list of regular expressions against which all
   PRIVMSG, ACTION, NOTICE will pass through.

h) Have to start the DCCServer Thread listening on port 8124. 

i) Integrated DCCserver/DCC chat. And made them extract directory listings.

j) Have to implement the Memory Debugging Class. Its coredumping cause of 
   memory corruption sometimes. Look into this with high proprity.
   LeakTracer class is born.

k) Remove ExitThread() calls, and let them exit out of scope, helps in 
   calling the destructors and freeing up memory.
   Nov 6th = no memory corruption and no memory leaks.

l) when some characters are present in the InputText widget. And we move
   to a different tabs. It triggers a Enter key press in that widget, causing
   half assed messages to be sent.

m) InActive window to change color to red on new text arrival. Normal color
   to be blue text.

n) Pressing Tab does nick completion like mirc.

o) Have implemented the FilesDetailList Class for storing all the scanned
   Files XDCC/FSERV.
   Functions for searching are yet to be implemented.

p) Have a 10 line history of entered text. Scrollable with up and down arrow.
   Class HistoryLines . Test file also created.

r) Have to implement a Stack Trace Class for debugging crashes.
   Class StackTrace created derived from HistoryLines. Test case also created.

s) Added sort etc to FilesDetailList class.

t) Need to fine tune the FilesDetailList class. Currently it auto prunes files
   which are 1 hour old. This will cause the Class to not hold valid file
   data for a nick, after prune, and till its trigger is seen next.
   Hence what we could do is, as soon as a nick's trigger is seen, we update
   the time on all his files, in the FilesDetailList Class. We also maintain
   a count of how many times the 'time' is being updated. After a certain
   number of updates, say 20, the filesOfNickPresent() should delete their
   entries. prune() continues to prune if time is older than pruneTime. (1 hr)

u) Break ThreadMain.cpp into files = 1 for each thread.

v) How the Search list interacts with other components.
   The components in play are:
   1. Search List (selecting and double clicking on a line is the trigger)
   2. Downloads In Progress (Actual bytes are being transferred)
   3. Downloads In Queue (Queued in various servers)
   4. Downloads In Limbo (Trying to figure out how to get these files)

   So as soon as I double click on a file in the Search List. The events that
   occur are as below:
      The FileName and FileSize are dumped in the Downloads in Limbo UI.
      Which also is associated with a XChange object, which maintains a
      list of Files/Size info, which is fed to a Thread.
      This thread, lets call it the Download Initiator(DwnldInitThr). 
      It waits on a Q called UI_ToDwnldInit
      which is in the XChange. This Line from the Q, has file size and
      file name information. It queries the FilesDB object, to give it a
      list of nicks/trigger information for just that Filesize/filename.
      => FilesDB.getFilesDetailListMatchingFileAndSize(...)
      It works on that list serially attempting to get the download of the
      file started (1st preference), or get it in some sort of Q. On not
      being able to succeed, we as of now, discard the attempt.
      Two scenarios:
        a) xdcc trigger => update the Download waiting nicks datastructure
           with nick and file name infomation, so that the DCC or DCCSERVER
           send is accepted.
           => update the Notice waiting nicks datastructure with nick and
           filename information, so that the NOTICE regarding Q or transfer
           or denied is accepted.
           If its a download, then the Download In Progress UI is updated
           appropriately.
           If its a Q, then the Download in Q UI is updated appropriately.
           if its a reject, remove the Nick and Filename from the
           Notice waiting nicks datastructure.
        b) fserv trigger => update the waiting nicks datastructure with
           nick and filename/dir information, so that the DCC or DCCSERVER
           chat is accepted.
           The DCC/DCCServer Chat component, establishes the connection,
           if succesful, and requests the files. That component, also
           registers wether server is sending now or putting in Q or
           denying and updates the information in the UIs as section a)

=>      Implemented the Thr and the Q it feeds on. Got the double click to
      feed the Q. 

      Before trying out each way to get the file, we check
      if the file is being transferred by looking at the DwnldInProgress
      structure (which is same as WaitingNick structure)
      All of the below will be in XChange, and be a FServWaitingNicks structure.
      FServWaitingNicks, will hold FileName, FileSize, Trigger, Nick info.
      This helps the DCCServerThr, in deciding what to allow. Also on issuing
      trigger the DCCThr will know if it needs to issue a GET in addition to
      doing the regular DIR command.

      DwnldWaiting
      DwnldInProgress
      FServPending

      When a CHAT is issued and FServ Pending has the Nick listed with Filename
      , dir etc information. We issue a GET. If FileName is NULL its a DIR.
      Before issuing a GET, we add the info in the DwnldWaiting, just so
      that if it sends immediately we dont refuse it.
      We note down the servers response. If it is "Sending ...", then its OK.
      If its, "You are in Q ...", then its OK. For everything else, we remove
      the entry from DwnldWaiting.
      We remove entry from FServPending and our job is done.

      On receiving NOTICE from the server, we check if its from a Nick present
      in DwnldWaiting. If so, we update the various DwnldWaiting UI 
      component appropriately.

=>    Added DwnldWaiting, DwnldInProgress in XChange.

Response from FServ when issued a GET.
Sending File. (in red color)
Adding your file to queue slot 1. The file will send when the next send slot is open.
That file has already been queued in slot 1!

      Looks like FServWaitingNicks is becoming more like FilesDetailList
      Have to replace FServWaitingNicks with FilesDetailList and tweak
      FilesDetailList. Also add a Queue number in the structure.
      Also while pruning, we can remember last prune timestamp, and not
      prune if last pruning was done say 10 seconds ago.
      This is done. <-====

w) Have our longIP ready to be used in the global area along with
   So send a USERHOST on connect. Make changes in IRCLineInterpret
   to classify the 302 reply correctly.

x) In FilesDetailList add function init() which initialises an allocated
   FilesDetail

y) A TimerThr().
   This does the below duties:
   1. Sending Trigger when appropriate.
   2. Keep track of Total Upload Speed.
   3. Keep track of Total Download Speed.
   4. Current # of Sends.
   5. Current # of Gets.
   6. Initiating a Send if appropriate from the QueuesInProgress structure.
   7. Send To_UI() commands to update the appropriate UIs with information.

z) File Server implementation - Thread FileServerThr
   Our Trigger is [/ctcp <nick> Masala Of <nick>]
   Try workaround file serving first. (outgoing connection)
   then try normal DCC (incoming connection)
   DCCServer doubles as DCC too. This gives us a new set of problems.
   When an incoming connection gets established in the DCCServer, it
   needs to decide, what to feed that connection ? a) DCC Send a file ?
   b) DCC CHAT a File Server ? c) DCCSERVER CHAT (remote nick giving us
   CHAT/FileServer)? d) DCCSERVER Get a file ? (remote nick sending us
   a file)?

   a) and b) are applicable when we are FILE SERVING.
   c) and d) are applicable when we are LEECHING.
   
   For File Server:
   As a solution we will save the dotted ip of user in the
   DottedIP field of FileServerWaiting. That way, a lookup for the dotted
   ip will yield us a hit and the Nick and provide validation.
   The disadvantage of this method is that all users should not have
   host masking. It will fail if host masking is enabled.
   Add FilesDetailList function getFilesDetailListOfDottedIP()

   For DCC Send:
   We will have solution along same lines above. The Structure used will
   be DCCSendWaiting.

a) Transfer Threads will attach the TCPConnect structure they
   are using to do the actual transfer, to the appropriate FilesDetailList
   ex: DwnldInProgress, UploadInProgress. Hence when they transfer the
   BytesSent/BytesReceived will be auto avaiable in the FilesDetailList.
   As the pointer is going to be saved, it can cause a bit of problem:
   - TimerThr gets list of FilesDetail.
   - goes thru the List and adds the speeds from individual 
     TCPConnect pointers.
   - The actual download/upload Thr which is using the TCPConnect, is
     done its job and destroys the TCPConnect.
   - TimerThr, acceses that same TCPConnect, which now is non existent.
   => SEGV.
   Hence when search...(*) returns a list of FDs, we make sure that the
   TCPConnect related values of BytesSent, BytesReceived, and Born Time
   are copied over seperately in the FilesDetail structure.

b) Take care of nick changes in all FilesDetailList structure.
   Similarly the parts/quits/kicks.

c) Respond to !list <nick> and Respond if a hit for @find <search>

d) Use CHANNEL_MAIN, CHANNEL_CHAT
    For QUIT, check InfoLine if it has QUIT in it, then its a wanted Quit.
    For other kinds of quit, dont delete the guy from qs and sends.
    Parts and Kicks, should effect sends/queues only if they
    happen in CHANNEL_MAIN.

e) when we are not joined in CHANNEL_MAIN, do not try to attempt a
   send from queues in TimerThr.cpp

f) Disable detailed queues. But enable detailed sends.

g) The FilesDetail orders the list in ascending order. And search returns
   that list in descending order. For Queues, we shouldnt be having them
   in order, but strictly append. So introduce one more private variable
   called bool Sort. default is set to true. For the FileServer Q we turn
   it to false.

i) While Exiting if a File Server is connected, am not able to disconnect
   it. Dont know if its cause of the select that its waiting on ?
   It doesnt even printDebug() when called, so possibly not even calling
   disConnect. The pointer is the same used in Fileserve and the one in
   FileServerInProgress.
   Also note that the FileServerInProgress and FileServerInQueue part
   does not fill up when accessing the file server in the UI tab.
   This was bug in copyFilesDetail when used by search. - fixed.

j) Implement priority to anyone other than a regular user.

k) Dont give File Server to people who are already in the process or already
   in the File Server.

l) If the sender has one connection in state TIME_WAIT and then tries to
   connect to the same port again, it fails. Have to find out why its in
   TIME_WAIT when the socket was closed.
   To reproduce, keep sending same file again and again. After 5th time
   we hit this issue. Also confirmed that after the TIME_WAIT connection
   disappears the next send succeeds. In this discussion Sender is mirc
   on remote machine and receiver is masalamate
   Changed TCPConnect, so that its shutdown and then closes sockets.
   When accessing mirc's fileserver, it doesnt close its file serving
   chat session properly and hence on its side there lingers a TIME_WAIT
   even after we are done with our end.
   On 6.14, this stops the next file server access or the GET.
   On 6.16, even though TIME_WAIT lingers, the connection gets established,
   and hence all is OK.

m) Addition in FileSearch UI. Probably it should also search for by nick,
   if it doesnt get any hit when it does a search.
   Also Add a header called Nick after the FileSize.

n) onHelpAbout() completed.

o) double clicking on a Download In Progress, gives an option to cancel it.

p) Make text gray on black background. Changed all RGB 255,255,255 to
   RGB 211,211,211

q) In message: "Please unmask your host" Add the personalised command too.
   ie: Please unmask your host by typing: /mode -v nickname


s) In Messages window, /clear will clear nick list and text window.
   In all other windows, clear only text.

t) graceful shutdown as in windows:
   1. shutdown with how = SD_SEND.
   2. call recv() till zero returned or SOCKET_ERROR
   3. call closesocket().
   This should be tested with 6.14 serving workaround.
   No improvement with 6.14. The only thing we can do is when accessing
   the file server, and issuing the GET, if the response we get is Sending...
   then we delay the QUIT by 10 seconds, so that it attempts to connect
   us and succeeds. (as we havent discoed the server yet, for it to become
   TIME_WAIT)

u) A nick on being kicked from CHANNEL_MAIN, didnt get its download cancelled.

r) If KICKED, PARTED, QUIT, disconnect the FileServerInProgress too.

s) Before starting a download or FileServer, make sure nick is in 
   CHANNEL_MAIN
   (Dec 8 2004)

t) When a Send gets disconnected, cause of Errors, put it back in the
   end of the queues.
   This wont recurse, as we wont put in q if send slots are open.

u) Settings - Read on startup. Save on changes.
   Sections that would be present are:
   [IRC]
   Nick=String

   [Connection]
   How=DIRECT | BNC | WINGATE | SOCKS4 | SOCKS5 | PROXY
   Host=String
   Port=Integer
   User=String
   Password=String
   VHost=String

   [FServe]
   Queue1=Nick DottedIP FileName
   Queue2=Nick DottedIP FileName
   ...
   Queue12=Nick DottedIP FileName
   There are 12 cause of 2 sends and 10 queues.
   With FileName, we get the FD from MyFilesDB, and then drop it in
   QueuesInProgress.
   For the Queues, we need the Nick's DottedIP as well.

v) Exit from FileServer saying Thread not registered from 
   XChange::resetIRC_Nick_Changed -> Thread not registered.
   Have to mutex protect the ThreadCount usage.
   Also bug in updating ThreadCount.

w) Double clicking on the Waiting Tab should present an option to cancel
   that Queue and remove it from that list. (DwnldWaiting)

x) Move fully downloaded file to the Serving folder and update MyFilesDB

y) Implemented clr_queues in the FileServer.

z) Only initiate a download, if we dont already have it in the Serving
   folder.

a) The Rate of Download or Upload should additionally be in rolling 20 
   second window as that is required by Torrent.

b) Each time a queue is added, update cfg file with that information.

c) Add SHA1 class, for piece checksums for torrents.

d) Take care of TOPIC changes.

e) If nick in use, try a different nick.

f) In Options menu, reflect the right data we currently hold.

g) When canceling a download, send /ctcp <nick> NoReSend, so that it doesnt
   resend.

h) Let only one instance of the program run.

i) Identify nick, if password is given.

j) Added the icon on the left corner of title. Get a nice .gif file for it
   now. (transparency)

k) In the put semaphores there is bug: 
   current order is : mutexlock, putline, semaup, mutexrelease
   it should be : mutexlock, putline, mutexrelease, semaup

l) Get the mutex and semaphore inside the LineQueue Class
   then we wont need any additional wrapper on top of it in XChange.cpp
   It can exist like the FilesDetailList class.

m) Add SHA1File Class to take care of generation of SHA1 of pieces etc
   of files in the Serving directory.

n) Make trace file be generated in SERVING_BASE_DIR so that I can download
   it for analysis.

o) MetaInfo file only obtained from server using the command:
   metainfo filename
   The server can possibly reply in the DCC Chat connection itself.
   using a special format.
   1st line:
   FileSize FileSHA1 PayloadSHA1 FileName
   2nd line:
   payload size in bytes of forthcoming piece wise SHA1
   <size bytes of piecewise SHA1>
   It automatically quits after that.

   If it doenst have the metainfo information. It replies with:
   Error:
   It doesnt quit automatically.

   Where do we save the metainfo file and what name do we give it ?
   Save it in "Serving" directory.
   Name it: .filename

   If we already have such a file in the "Serving" directory, implies
   we dont need to get it again.

   The MyFilesDB generation checks and updates the "Serving" directory
   with the .filename file for all files therein. This implies that all partial
   files are always in "Partial". Its only moved to "Serving" when its 
   downloaded fully.

   Format of .filename file:
   FileSize FileSHA1 PayloadSHA1 FileName\n
   <payload size>\n
   <payload length bytes of piecewise SHA1>

   This is exactly the info passed on when "metainfo filename" is issued in
   the fileserver.
   The piecewise SHA1 = 20 bytes per 128 KB
   This translates to: 160 bytes for each 1 MB.
   So payload size:
      128 KB = 20 bytes
      FileSize = (FileSize  / 128 KB) * 20 bytes. = (shift right by 17) * 20
      is (FileSize  / 128 KB) * 20 bytes

   Where do we save the bitmap of the pieces we have obtained ? possibly in
   the partial downloading file in the "Partial" directory. (towards the end
   past the filesize).

p) On double clicking on nick, sometimes crash. (Bug - resolved)

q) on pressing Tab in Search String window - crash.
   bug in TabBookWindow::onExpandTextEntry()

r) When the mouse press is released, make focus go to the TextEntry widget.
   Above has a problem, now cant copy paste from simplescroll.
   so when we release, first we copy the text, then move focus.
   so call ScrollUI->onCmdCopySel(), before moving focus.

s) A new command called, "/portcheck Nick" to be created
   that sends a /ctcp to that nick with portcheck (only if Nick is present 
   in main). This will prompt that nick to try to open a connection at 
   portnum
   It replies back with success or failure

t) Add /msg support. Should work just like in mirc.

u) Add /ctcp support. Assumes its a trigger access, and hence will do all
   arrangements assuming an FServ access will take place.

v) Add "help" in FServ command.

w) The meta files which are created, make them hidden under WINDOWS.
   Its a . file under unix, which is hidden anyway.

x) save MasalaMate.trace as MasalaMate.pid.trc. This will help in downloading
   the trace files, from all the core dumping nicks.
   Jan 7th

y) Make the structures in XChange, be accessible easier. Rather than having
   the Thread array etc.
   The functions are:
   isIRC_CM_Changed()
   isIRC_SL_Changed()
   isIRC_CL_Changed()
   isIRC_Nick_Changed()
   isIRC_Server_Changed()
   isIRC_IP_Changed()

   Now we can change these functions, so that they pass the structure they
   are checking for the change as an argument. Based on that they return
   true or false. With this approach, everything can remain the same.
   And, register/unregister can be discarded.
   Jan 8th

z) Have a single point srand(getpid()) in IRCClient.run()
   Remove from everywhere else.
   Jan 8th

a) The Channel list and Server List are structures which are static
   once set in our case. Hence lets not repeatedly check for them to change
   in IRCCLient.cpp
   Jan 8th

b) In the Tab for File search, list all files, without removing repetitions.
   Jan 8th

c) Also, when double clicking, only get that file from that nick.
   For this we change DwnldInitThr, so that it now looks for Nick and
   FileName. It previously looked at Size and FileName.
   Jan 8th

d) Increase the history of lines to 20000 from 10000.
   Jan 8th

e) In the trace file, mention the Client name and version number.
   Jan 9th

f) Initialised Nick local variable in the various threads. Was done to avoid,
   access of uninitialised variables.
   Jan 9th

g) Increased char array from 128 to 512 in the update(download/fserv/waiting) 
   functions in TabBook
   Jan 9th

h) Change IRCNickLists Class to hold client information and IP information.
   Jan 11th

i) DCCThr(), now handles IC_USERHOST. as will be used in manual dcc send.
   Jan 11th

j) Add manual dcc send. Should send immediately. 
   syntax: /dcc send nick
   We first check if we have nick's ip in NickList. If not, we issue a
   /userhost and hope that the ip is populated.
   We present the user with the file list selection dialog, and then on 
   getting a file to send, we first check if its ip is known. If not known
   we inform the user. (possible failures are: nick not in known channel,
   nick is not mode -v, nick's host could not be resolved, or havent got 
   reply back from an issued USERHOST)
   All files in any directory should be able to be sent.
   For that, start using the DirName field in FilesDetail.
   Manual DCC sends, will have an appropriate directory set.
   if DirName is NULL => SERVING_BASE_DIR
   Jan 13th

k) All sends are initiated purely by TimerThr.cpp. Hence add a field
   in FilesDetail which mentions a manual send. They are cumpolsorily at
   index 1. So TimerThr, will look at index 1 too and push it out
   if its a DCC manual send.
   Jan 13th
  
l) Add more statistics like record cps, bytes sent etc in trigger ad,
   like sysreset.
   Add RecordCPS as double in XChange.
   Add TotalBytesSent as double in XChange.
   RecordCPS comparison is done every 5 seconds in TimerThr.cpp
   with call to Helper::updateRecordCPS()

   TotalBytesSent is incremented in TransferThr.

   Values are Loaded at startup. (Helper::readConfigFile())

   Its values are written out when the FServ Config section is updated.

   Incorporate these values in the FServ ad.
   Jan 13th

m) The trace files, when requested, should be sent immediately.
   Just like a manual dcc send.
   Jan 13th

n) Add more statistics: Record CPS Up|Down: [] Bytes Sent|Rcvd: []
   Jan 14th

o) Add UPNP support. Class Upnp.
   We have a UpnpThr.cpp which handles it.
   Communication with it is done using LineQueue IRC_ToUpnp;
   We will define the commands that it will take in.
   It in turn, can use XChange to update the UI.
   Commands:
     1. SEARCH
     2. GET_EXTERNAL_IP
     3. GET_CURRENT_PORT_MAPPINGS
     4. ADD_PORT_MAPPING <port> <descrip>
     5. DEL_PORT_MAPPING <port> <descrip>
     6. SEARCH_IF_NOT_OK // Issued only by TimerThr once a minute or so.
                         // It does a Search only if Status is not UPNP_OK.
   We can incorporate a /upnp command to initiate these.
   /upnp search
   /upnp getextip
   /upnp getmappings 
   /upnp addport <port>
   /upnp delport <port> // will not allow DCCSERVER_PORT to be deleted
   Automatically, we will do SEARCH, ADD for 8124, and DEL for 8124 on exit.
   Jan 18th

p) Add File->Partial Dir to open the directory holding Partially downloading 
   files.
   Add File->Serving Dir to open the directory holding the Serving files.
   Jan 18th

q) on receiving a portcheck request, works only 1st time.
   Jan 18th

r) A Help window, to provide help.
   Jan 19th

s) Added a Clock in the Menubar.
   Jan 19th

t) Crash in TCPConnect->getUploadBPS();
   The scenario is: FileServer Tab was being updated.
   It does a searchFilesDetailList(*) to get the full list.
   This search, gets each entry and copies it over.
   In copyFilesDetail(), The TCPConnect if present is copied over,
   along with calls to UploadBPS(), DownloadBPS() etc.
   In the scenario that, the connection is closed and discarded, and
   UploadBPS is called, the UploadArray is already destroyed and Nulled.
   Leading to a crash.
   We need to add a lock() interface in TCPConnect class.
   This interface will just stop the Class from being destroyed.
   We TCPConnect.lock()
      get data and calls we want.
      TCPConnect.unlock()
   Within a lock and unlock, we are guaranteed that the class wont
   be destroyed.
   Hence, the destructor, will try to get the lock, before destroying
   the class.
   Jan 19th

u) Move Help to be next of Dir in menu.
   Jan 20th

v) There should be a Mutex to access the common global variables in XChange.
   like UploadBps etc. Added XChange->lock() and unlock() to access
   those.
   Jan 20th

w) User changeable Fonts.
   Jan 20th

x) Pass the IC_NOTICE messages for trigger scans just like IC_PRIVMSG.
   This will help !list <nick> processing
   Also for triggers being issued, update in server window.
   Jan 20th

y) To stop the user from doing lot of !list, we do not allow !list at all.
   We issue a !list, when we join channel ourselves.
   Jan 20th

z) All accesses to FServs in IC_NOTICE, should be considered manual and
   immediately accessed, instead of delaying. This takes care of the inital
   !list and manually issued !list <nick>
   Jan 20th

a) Add Copy/Paste in help file.
   Jan 21st

b) While accessing triggers and getting directory information. If no files
   present, then add an entry with filename = "No Files Present". This will stop
   us from repeatedly accessing his server when an ad is displayed.
   Jan 21st

c) Change NODELAY triggers to issue trigger in 15 seconds to avoid 
   the "Message target change too fast", got from server.
   This is IRC Server specific. 15 seems ideal for IRCSuper.
   Need to change this when moving to different server.
   Add #define IRCSERVER_TARGET_CHANGE_DELAY 15
   in ToTriggerThr.cpp
   Jan 21st

d) Add entry with filename = "No Files Present" for servers we cannot connect
   to. This will help us in not trying the non connectible server again and
   again.
   The scenario is as follows:
   1. I issue the ctcp to access a trigger.
   Case I) Remote user tries to contact me at port 8124.
        I am firewalled so dont get any connection.
        This we cannot detect. And its not much waste of resource,
        as we have already issued the ctcp.

   Case II) Remote user sends me a DCC CHAT.
        I try to connect as per the DCC CHAT, but cant make the connection.
        This we definitely can detect.
        In this case, we can add entry.
   Jan 21st

e) Do not attempt to download FileName = "No Files Present".
   Give information in the Server window.
   Jan 21st

f) Move #define IRCSERVER_TARGET_CHANGE_DELAY 15
   from ToTriggerThr.cpp to ThreadMain.hpp. It is used in DwnldInitThr.cpp
   as well, sleeping between two ctcp's which access the trigger.
   Jan 21st

h) Lets give more information in server window as we try to retrieve files
   by double clicking on GUI.
   Jan 21st

i) Remove srand(getpid()) from Helper constructor, as it might keep resetting
   the rand generator. We explicitly call srand(getpid(); at the start of each 
   thread. Hence we just add it in TRACE_INIT(); and keep one in IRCClient.
   Jan 21st

j) bug in TCPConnect() with proxy. So on success connection from proxy, we
   just readLines till an empty line, and take that as an indication that
   from that point on its actual data from destination.
   Jan 21st

k) Make the frames FRAME_NONE. Will allow window to hold more space for stuff.
   Jan 21st

l) Rename /portcheck to /portcheckme
   The command that basically sends a /ctcp portcheck nick 8124
   Jan 21st
  
m) Add /portcheck <nick>
   Jan 21st

n) The NODELAY Triggers should be processed ASAP. Currently they all get
   queued in FIFO basis. Hence the NODELAY trigers get behind DELAY
   triggers. This can cause anxiety problems in newbie users.
   Simplest solution is to form another Thr and Q. Hence we create a
   ToTriggerNowThr.cpp and a IRC_ToTriggerNow Q.
   Then we can remove the second word in the line having to be DELAY or
   NODELAY.
   So for this we revert back the change to FServParse to recognise the
   second word as DELAY or NODELAY.
   Jan 21st

o) Changed filter. block only if server and join and # and no masala in it.
   Jan 21st

p) Add buttons like in mirc, for things like:
   /clear, /portcheck, /portcheckme, /dcc send
   The Nick highlighted in the window is taken as the
   nick to which its directed.
   Jan 22nd --- Released

q) TCConnect::readData() to return -1 when it recv returns 0 bytes.
   Receiving 0 bytes => socket closed on other end.
   The bug that prompted the above is as follows:
   I see an upload in FileServer, stuck at 100 %. 
   Scenario was, that I had contacted the nicks port 8124 and sending him file.
   The socket is in CLOSE_WAIT
   TCP    192.168.0.26:3435      24.24.197.163:8124     CLOSE_WAIT
   So basically, TransferThr, calls T.run, which hasnt returned.
   If it had returned, it would have removed from SendsInProgress.
   something wrong in readAckBytes/TCPConnect.readData, not returning on error.
   Jan 23rd

r) Add a button, which is red if firewalled. green if not.
   So it starts of red, and if we get a valid incoming we gradually make
   it green. As of now 5 incomings = green.
   DCCServer, starts of with Firewall = 0. On getting a valid
   incoming, it increases by 1 till 5, and send the UI a "*NOTFIREWALLED*"
   message
   Jan 23rd

s) When X pressed in window, it doesnt quit cleanly.
   Make it clean gracefully.
   Added addSignall() calls, and no deleting of UI in UI.cpp.
   Jan 24th

t) Mirc like feature:
   On clicking on a word, if such a nick exists, we select it in the NickList.
   Jan 24th

u) Make messages in Server window consistent. That is instead of " * ",
   put-> component: message. example: UPNP: Router detected.
   Most of components done. Its ongoing, if I find something new, I will
   proactively update.
   Jan 24th

v) file server ad should mention its firewall state. First Ad that is
   displayed on joining channel, should list UNKNOWN.
   - FireWalled:[NO|nn %]
   Jan 24th

w) Before Quitting, ask confirmation.
   On pressing X it still abruptly quits.
   Jan 25th

x) Allow user to change Partial and Serving Directories.
   Add it under Dir Menu.
   Side effect, trace file created only in run directory.
   Jan 26th

y) Issuing a !list <nick>, we always try to get the file list.
   Jan 26th

z) Pasting lines more than 320 characters get truncated.
   Jan 26th

a) Crash in TCPConnect->getUploadBPS();
   Again, inspite of having the lock in TCPConnect(). (Search above for bug -
   Jan 19th section t)
   There is a narrow window when TCPConnect:: can still be destroyed.
   if (oldFD->Connection) {
      after this, a different thread, TCPConnect locks and destroys.
      oldFD->Connection->lock();
      Lock succeeds and we try to use UploadBPS().
   So possibly we can first lock, before we check oldFD->Connection
   Jan 27th

b) Make IRCSERVER_TARGET_CHANGE_DELAY be 20.
   Jan 27th

c) Linux portability changes.
   Jan 28th

d) Made UI be updated only by the UI creating thread.
   Now Linux and Windows UI behave similar, wrt scrolling.
   Jan 28th

e) Made the addTimeout() process of UI updation in both Windows and unix
   as they will have very similar code.
   Jan 29th

f) Add "Do not attempt to exploit me." in SpamFilter.
   Jan 30th

g) Allow !list <nick> and @find <str> to be visible in the windows.
   Jan 30th

h) Change the orange for Trigger prints to something else. Its too close to
   the red color which is not spotted.
   Change it to 15 => light grey
   Jan 30th

u) When there are more than one sends to a nick (manual), when one of them 
   completes, it removes all the entries.
   Jan 30th

v) Still getting crashes in UploadBps() and DownloadBps() inspite of being
   locked. Looks like the lock is not working. Maybe the lock is not using
   the static lock of the TCPConnect class.
   Made change from:
   MUTEX TCPConnect::Mutex_TCPConnect;
   to:
   MUTEX TCPConnect::Mutex_TCPConnect = 0;
   Maybe the first one, created one more lock rather than initialising the
   static lock of class.
   Jan 30th

w) As part of above, if TCP state is not TCP_ESTABLISHED; then return 0
   on call to UploadBps() and DownloadBps();
   Jan 30th

x) On quitting and restarting the sends and queues when quit, were not
   maintained again. Bug introduced.
   Moved the MyFilesDB  population code before setting up queues.
   Jan 30th

y) Make more messages in Server window consistent.
   Jan 30th

z) In Messages window. If I click on a text which happens to be the nick,
   now, it highlights the nick in the nicklist. We should also make the
   label text = nick.
   Jan 30th

a) Correct the Messages window once and for all.
   Messages to nick appear as: <to Nick> text
   Messages from nick appear as: <Nick> text
   Jan 31st

b) On issuing portcheck etc, let the results be seen in the window its issued 
   in.
   Jan 31st

c) In File Search Tab. Rename the Scan Time column as "Information".
   All will have a default message of "Double click to download"   
   On Double Clicking it, change the message to "Check Network Tab for
   details"
   Feb 1st

d) In Linux, the upnp port forwarding gets an erroneous local ip address.
   No it doesnt. The linux box was not configured correctly.
   Its hostname had an entry in hosts to 127.0.0.2
   NOT A BUG.
   Feb 1st ----> Released

e) Improve on the FileSearch Tab.
   Add new columns like:
   Sends   Queues
   02/02   05/20

   This firstly, introduces 2 new columns in that TAB.
   Secondly, it means we need to add those information in the
   FilesDetail structure.

   In the ToTriggerThr, ToTriggerNowThr and FromServerThr, we first delete 
   NickFile(Nick, "TriggerTemplate");
   then add FD with Nick and File = "TriggerTemplate", with the sends and
   the queus information.
   Later when we parse xdcc or fserv, we first get the 
   NickFile(Nick, "TriggerTemplate") FD. Note down the sends and q info.
   And update each entry with that information.
   In the TabBook update, we do not display the information if filename
   is "TriggerTemplate"
   So if we are not going to issue the trigger, we will update all files
   held by the nick, with the latest sends and q information.

   So have to change the FServParse class to identify the sends and queues
   of xdcc iroffers and FServ Triggers.
   Added a TriggerType called SENDS_QS_LINE. This will be returned by
   TriggerParse.getTriggerType, so that we know what is being updated.
   Feb 5th

f) Add Buttons and Toggle buttons in FileSearch.
   |Search string|List Nick|List All|Free Sends|Free Qs| TEXT Entry.
   All in the bottom line.
   Feb 5th

g) On receiving a ctcp NoReSend, we should not requeue that send when it
   disconnects.
   So added IC_CTCPNORESEND
   Add a flag in FilesDetailList which => no resend, bool NoResend.
   add a function which updates NoResend of Nick.
   The Transfer.requeueTransfer(), will check the NoResend flag, before
   attempting to requeue it.
   Feb 6th

h) Crash in DCCThr on ACCEPT. Was freeing something that wasnt even
   allocated on a failure case.
   Feb 7th

i) Pop a menu on right click. This presents options for:
   -> portcheck, portcheckme, dcc send, list files, get listing
   and works on the selected nick.
   Feb 7th

j) Forgot to add the logic of grabbing client info and sends / queues
   information in the IC_NOTICE case. Added now.
   Feb 7th

k) Take care of one more xdcc sends line variation.
   Feb 7th

l) There is a leak showing up at one copyFilesDetail. It was from
   Transfer::requeueTransfer()
   Feb 9th

m) Add the same right click popup, for the channel UI.
   Feb 9th

n) When doing a /ctcp nick ping|version etc we got a message saying attempting
   trigger of so and so, though it didnt really attempt.
   Feb 10th

o) Move to fox 1.4.2 stable.
   Feb 10th

p) Use FXGUISignal in Fox 1.4 to make GUI updations. So we remove our
   up / down semaphore dance in conjunction with addTimeOut()
   Feb 10th

q) Do not use FXGUISignal. Doesnt seem to work nice with Linux.
   Revert back to our code.
   Feb 11th

r) Added a Splash Screen during startup.
   Feb 11th

s) TCPConnect() does not update the speeds when timeouts on receiving occur
   and no data is transferred.
   Move the update code as a private routine, and call it from relevant
   places.
   Feb 11th

t) On double click on the File Server tab. If in Q, ask if that needs to be
   sent immediately. and handle accordingly.
   Feb 11th

u) Remove the !list at start. It just creates traffic. They can do a 
   !list <nick>, and get the file listings. As it is !list is not permitted
   from MasalaMate.
   Feb 12th

v) IP remembrance to avoid many dns calls. In DCCThr.cpp, case IC_CTCPFSERV,
   try to get the IP of user if it already exists in the NickList. If it
   doesnt, then issue the getLongFromHostName(), to obtain it. On obtaining
   it update the NickList to have that IP, so we dont issue it again.
   Another feature to add is, to recognise the IP as last parameter of the ctcp
   trigger. Example: /ctcp nick masala of nick IP. The ip here is in long.
   If we hit a case where we are not able to obtain the nick's ip, we
   use that IP as present in the trigger line. We do not save this ip with
   the nick, as it might not be reliable.
   Feb 13th

w) Issue the MasalaMate client trigger, appending our longip with it.
   Feb 13th

x) We still get crashes when updating the UploadBPS()
   This is why:
   Thread 1				Thread 2
   copyFilesDetail()
   have a TCPConnect.
					delFilesDetail entry.
					delete TCPConnect.
   Access TCPConnect.UploadBPS()
   Crash.
   So think on how to resolve this.
   Above scenario does not seem possible as we always delete entry before
   deleting the TCPConnect.

   Looks like some other copyFilesDetail is crashing. used in Helper,
   DCCChatThr, DwnldInitThr.cpp, where a variation of above scenario as
   below can exist.
   Thread 1				Thread 2
   in copyFilesDetail()
   before lock TCPConnect		free FD in FilesDetailList
					delete TCPConnect
   lock TCPConnect.
   Access TCPConnect.UploadBPS()
   Crash.
   The above can be avoided if the FilesDetailList mutex is locked,
   during the copy process. The FilesDetail functions themselves lock
   the mutex before calling copyFilesDetail().

   But it cant avoid the scenario below:
   Thread 1				Thread 2
   get an FD.
					that FD gets freed.
					delete TCPConnect
   in copyFilesDetail()
   lock TCPConnect()
   Access TCPConnect.UploadBPS()
   Crash.

   Hence we need some intelligent solution. How do we know that the TCPConnect
   object we have is valid ? Or better, how to make sure getUploadBPS() does
   not crash. Same with getDownloadBPS().
   We can maintain a list of valid TCPConnect, which could be static to the
   TCPConnect class. Introduce a member function called: isValid(), which 
   takes in the TCPConnect *, and sees if its valid or not. The Constructor
   and Destructor can update this static structure.
   So we TCPConnect->lock(), then TCPConnect->isValid(Connection *), and
   only then call the UploadBPS() stuff.
   Feb 17th
   -> Released.

y) Change "Upload Speed:[2.67kB/s] - Download Speed:[0.00kB/s] -"
   To     "Speed Up|Down:[2.67kB/s | 0.00kB/s] -"
   Mar 13th

z) If we get disconnected from server, we should flush all the queues.
   Added a .flush() function in LineQueue class to accomplish this.
   Mar 16th

a) On a download that starts, the time left is absurd. Made it show
   "UNKNOWN"
   Mar 16th

b) When no bytes are received, and download tab is updated. It refreshes with
   last known speed, as TCP does not adjust the speed, on idle connection.
   Mar 16th

c) As !list is removed at start, TriggerNowThr activated on a !list nick,
   issues immediately. No more wait.
   Mar 17th

d) On DCC Send, remember last directory which was used to open file.
   ReOpen with same directory.
   Mar 17th

e) MM should be able to traverse all subdirectories within Serving and
   make the serving list.
   Mar 26th

f) Double clicking on a file in "File Server" tab, a file which is already 
   "Sending" or "FServ" or "FServInit" shouldnt do anything.
   Mar 27th

g) Instead of sending an *UPDATES* message for UI updates (downloads/etc)
   Register it with FOX to be called like a timer every so seconds.
   Mar 27th

h) FServ to handle dir/subdir listings, and cd commands. 
   Basically - serve a full directory tree.
   Modify FilesDetailList, as follows.
   The Sort flag takes on one more value => sort considering dir names as well.
   This mode is used only by MyFilesDB. This enables a list as below:
   Dir      FileName
   ---      --------
   NULL     file1.avi
   NULL     file2.avi
   Dir1     file1.avi
   Dir1     file3.avi
   Dir2     filen.avi
   ...
   With this we can represent a DIR listing easier.
   Mar 27th

i) Add fservPWD(), to handle the pwd command.
   Mar 28th

j) Cause of handling sub dirs in Serving folder, correct Helper::dccSend(num)
   so that files get sent.
   Mar 28th

k) A GUI Command to update serving files list. (Dir->Update Server)
   Mar 28th

l) Traverse sub directories while accessing another's FServ.
   Write a new Class DCCChatClient for talking with an FServ, which can handle
   GET and recursive DIR listings. So have to branch code out from 
   DCCChatThr.cpp.
   Mar 29th

m) The Downloads Tab lists files as Partial\filename.avi.
   It should just be filename.avi.
   Furthermore, when I attempt to get the same file, even though its
   already downloading it still attempts.
   Mar 30th

n) When download is finished, mark it as "COMPLETE" or "PARTIAL" in 
   the "Time Left" column.
   Leave it hanging in the Downloads Tab.
   Mar 30th

o) Double clicking in "Downloads" Tab. Check for "COMPLETE" or "PARTIAL"
   and hence give prompt appropriately, to remove from list.
   Mar 30th

p) Cause of the "COMPLETE" and "PARTIAL" DwnldInProgress lingering,
   while attempting to download a new file, if we find that DwnldInProgress
   already contains that filename, but its in "PARTIAL" or "COMPLETE" state
   we delete that entry, and try the download.
   Mar 30th

q) Generic bug found, in which when we update our ip/dotip after issuing
   a userhost, we updated the dotip erroneously => random memory over writes
   occurred. Error was in DCCThr.cpp when it handled IC_USERHOST.
   Mar 30th

r) Added Spam block on:
   Come watch me on my webcam and chat /w me :-) 
   http://dialup-215.225.221.203.acc52-kent-syd.comindico.com.au:1571/me.mpg
   Mar 31st

s) In recursive directory listing. Grab all files in main folder.
   All other files in sub directories, notice it if its > 1 MB.
   Exit out of Recursion - no more DIR listing on that server - if -
   have got a list of 1000 files, or reached a DIR depth of 3.
   Tweak these values with DIRLISTING_MAX_FILES, DIRLISTING_MAX_DEPTH,
   DIRLISTING_MAX_FILESIZE in DCCChatClient.cpp
   Mar 31st

t) After doing a search in "File Search", when you switch to a different TAB,
   and come back to the "File Search" Tab, the # of Files is correct, but the
   File Size is 0.00 GB.
   Mar 31st

u) Changed IRCChannelList class to be capable of handling keys.
   Added CHANNEL_MAIN_KEY so we can use it to join MB for testing.
   Mar 31st

v) USERHOST returns a * after the nick, if nick is ircop. Handle it.
   Mar 31st

w) !list in #Masala-Chat also gives the FServ Ad. It shouldnt.
   !list Nick in pm or in CHANNEL_MAIN should only respond.
   Mar 31st

x) Try to stop illegitimate CTCP FServes as soon as posisble, and not wait
   till connection is established. Added check in DCCThr which handles
   the IC_CTCPFSERV
   Mar 31st

y) During periodic updates of the "Downloads" folder, first note down all
   the items that are highlighted. Refresh list. Highlight all the items 
   again. This will help in implementing multi line clearing for "COMPLETE"
   and "PARTIAL"
   Mar 31st

z) On Double clicking in the Downloads folder, take action on multiple 
   items selected, rather on just one selected item. Action implying -> 
   clearing those items from the list, if possible. Possible ones are the
   'PARTIAL" and "COMPLETE" entries. Downloads can only be cancelled one
   at a time.
   Mar 31st

a) Not catching the xdcc listings of bots in MB - check why. Ah, it catches
   listing of bots which start with [IM]- only.
   This is FYI.

b) In "Downloads", Clicking on the clear screen, clears all the "PARTIAL"
   and "COMPLETE" entries.
   Apr 1st

c) Update NickClient in class IRCNickLists, when we know what client Nick
   is in ToTriggerThr.cpp, ToTriggerNowThr.cpp, FromServerThr.cpp, as seen by
   FServParse class.
   Apr 1st

d) Fast File Listing Collection Algorithm
   On joining channel, it gives out ad. Each MM client that contacts it for a
   dir listing, We can in turn take advantage, of getting the file list info
   that its holding. This will imply a very fast file list update.

   OK, so I think we support the normal dir commands etc. (used by other
   lame clients)
   If we recognise it as a MM client, below will be the transaction.
   Client                 Server
   nicklist ->
                       <- nick_1 update_count (server)
                       <- nick_2 update_count
                       <- ...
                       <- nick n update_count
                       < nicklist end
   So we compare our update count with what (note file_count is meaningless)
   we receive to decide if we want that nick's information.
   The below is optional (though will be issued from 0 to n times)
   Now Client, knows which all nicks he doesnt have the list of.
   or if the nick info that he has is older than what server has.

   filelist nick 3 ->
                       <- file 1
                       <- ...
                       <- file n
                       <- filelist end
   The above filelist for that nick is added with update_count received,
   in the FilesDB.

   filelist nick_n ->
   ... etc

   Client sends that its done on its side. (compulsory)
   endlist ->

   Server now tries to update itself with what client has.
                       <- nicklist

   Now its client's turn to send its information.
   The client already knows what all nicks the server should be interested in.
   So we can optimise and send list of only those nicks. - defer for 
   performance, for later on.
   (client nick) nick_1 update_count ->
   nick_2 update_count ->
   ... ->
   nick_n update_count ->
   nicklist end

   The below is optional. (and can occur as many times as nick info is needed)
   Now Server, knows which all nicks he doesnt have the list of, or which are
   more recent on client.
                       <- filelist nick_2
   file_1 ->
   ...
   file_n ->
   filelist end

                       <- filelist nick_n
   ...

                       <- endlist
   The filelist exchange protocol is over.

   Format of first file list is the TriggerTemplate as follows:
   TriggerTemplate*TriggerType*ClientType*TriggerName*CurrentSends*TotalSends*
       CurrentQueues*TotalQueues
   Followed by:
   FileSize*DirName*FileName  (For CTCP)
     or
   FileSize*PackNum*FileName (For XDCC)

   Above, we have used * as the Delimiter.
   TriggerType = C, X. C = CTCP, X = XDCC

   It adds this info with update_count received.

   Implementation:
   ---------------
   Have to add commands "nicklist" and "filelist" to MM Server => FileServer
   class now has two additional interfaces, nickList() and fileList().
   DCCChatClient.getDirListing(), will call DCCChatClient.getDirListingMM()
   if server is a MasalaMate server.
   The code will be similar for the information interchange in both
   DCCChatClient and FileServer.
   Hence, code has been added in the Helper Class, and used by both
     DCCChatClient and FileServer.

   Apr 2nd

e) Because of above implementation, 
   Start with a clean FilesDB as soon as we join CHANNEL_MAIN
   Also, as others nicks leave CHANNEL_MAIN we should delete the file
   information of those nicks in FilesDB.
   Apr 2nd

f) Modify IRCNickLists to keep nicks in ascending order. This will be
   helpful for Ad synchronization and other purposes.
   Change in IRCNickLists.addNick()
   Apr 2nd
   
g) Make the Ads synchronised. (Following our own Iroffer Ad sync algo)

   We have an ascending order of MM clients list - its already there as
   part of NickList. How to use that in determining when we need to throw
   our Ads out.

   1. Come into channel, display Ad (has "Firewall:[UNKNOWN]")
   2. Set Next Ad Delay time  = AT = to 25 minutes from now.
   3. On detecting an MM ad in main (which doesnt have "Firewall:[UNKNOWN]")
      We do the below:
      The bots are numbered from 1 to N.
      We find our MM index in NickList, say its m.
      We find the MM index of the MM guy who ad, say its x.
      Total number of MMs in Channel = N.
      Time delay between M ads in main = T.
      if (m < x), AT = (N - x + m) * T
      if (m > x), AT = (m - x) * T
      We can use T = 180 seconds = 3 minutes. 
      FSERV_RELATIVE_AD_TIME 180     // 3 minutes
      FSERV_INITIAL_AD_TIME  3 * FSERV_RELATIVE_AD_TIME // 9 minutes
   4. Helper:: on noting that our AT is past Current time, will issue the Ad.
   END of STORY.

   To implement above, we need to add the following:
   On joining CHANNEL_MAIN, and getting IC_NICKLIST in FromServerThr,
   update our nick as being an MM client. - done
   IRCNickLists::getMMNickCount(channel)  - done
   IRCNickLists::getMMNickIndex(channel, varnick); // return 0 if not MM - done
   Add FServAdTime in XChange. Access it using XGlobal->lock/unlock

   Testing: 
     one MM  -> Ad should come out once in 9 minutes.
     two MMs -> they should give ad out in distance of 3 minutes after 
                the first 9 minutes.
   Test passed.

h) An oversight in IRCNicksList. When a nick changes its nick, we need to 
   put it in the right position, as list is in ascending order. So make changes
   to IRCNickLists.changeNick();
   Apr 3rd

i) Tweaking for Fast FileList algo
   As of now, we do not distinguish between a server which has no files served,
   and a server which we cannot connect to. Both have a "TriggerTemplate"
   and a "No Files Present" entry. This is not good. As when, a firewalled
   user, has "No Files Present" for servers he couldnt connect to, and thus
   would exchange this information with another client, resulting in
   the propagation of erroneous lists. Hence we introduce "Inaccessible Server".
   When doing filelist exchange, we do not present the nick having
   "Inaccessible Server", in response to the nicklist command. 
   "No Files Present" should be exchanged and presented in response to nicklist,
   as is being done already. In FileList exchange, we will present "No Files
   Present" in case we are not serving any files.

   Documented for easy reference.
   -----------------------------
   In Trigger/TriggerNow, once decision has been made to issue trigger, before
   the trigger is actully sent, setup the "TriggerTemplate" and "Inaccessible
   Server" in FilesDB against the nick. So FilesDB will have exactly those two
   entries. So, if DCCChatClient, fails to get called, those two will prevail.
   If DCCChatClient gets called, the first thing it does is to delete the
   "Inaccessible Server" in FilesDB for that nick. If no files are being
   served, it will add the "No Files Present" entry.
   Double clicking for download, should not download the "Inaccessible Server"
   entry, but give a meaningful message in the Server window.
   Apr 3rd

j) If nick has ` in it, it does not connect to server. example: Apache`
   Looks like its issue with ircsuper not accepting ` possibly in the
   username field. As in the nick its allowed.
   Problem was IRCSuper rejcted connection, if USER had ` in name etc.
   During USER command, replace all instances of ` with _
   Apr 3rd

k) Further optimisation in minimising FileList transfer payload when an
   MM connects with another MM.
   With current scenario, as soon as an MM client enters channel, say MM0
   All the other MMs will access this new one.
   Lets say that the order of access is roughly: MM1, MM2, ...
   MM0 <-> MM1 (exchange between MM0 and MM1)
     mm0 -> mm0
     mm0 <- mm1, mm2, ..., mmn
     where mm0 denotes the filelist payload of MM0, and so on.
     So total payload = n + 1
     In this transaction, MM0 contains the latest info of MM1.

   MM0 <-> MM2 (exchange between MM0 and MM2)
     mm0 -> mm0, mm1
     mm0 <- mm2 (at least)
     So the least possible payload is 3.

   MM0 <-> MM3 (exchange between MM0 and MM3)
     mm0 -> mm0, mm1, mm2
     mm0 <- mm3 (at least)
     So the least possible payload is 4.

   MM0 <-> MMn (exchange between MM0 and MMn)
     mm0 -> mm0, mm1, mm2, ..., mmn
     mm 0 <- mmn (at least)
     So the least possible payload is (n + 1)

   Hence just for that one entry Advertisement, the filelist payload generated
   is (n + 1) + (3) + (4)  + ... + (n + 1) = roughly 3n bare minimum.
   So if on an average file list information is 1K.
   and there are 100 MM client.
   Connections generated are 100. payload excanged = 3 * 100 * 1 K bytes
   = 300 Kilo bytes.
   which is unwarranted.

   Alternative proposed.
   ---------------------
   As the Ad displayed on channel entry creates this traffic, lets do away
   with it.
   a) On joining channel, set AdTime = cur_time + (3 * T)
   b) Access triggers as they become visible.
   c) If sysreset or xdcc, we just update ourselves.
      If MM, we exchange and increase each of our information.
   Lets see how this fares.
   Apr 3rd

l) Update Help.
   Apr 3rd

m) Moved to Fox 1.4.11. Ported to SLES 9.2
   Apr 4th

n) Production build, move to -O2. O3 might change stuff.
   Apr 4th

o) Found a bug in IRCNickLists.getNickMMIndex(), was returning non zero
   even if Nick was not an MM nick. This threw the Ads off.
   Apr 4th

p) Enclosing all print statements within COUT()
   Apr 4th

q) FileList transfer payload -> With above modification, when a nick joins
   channel and accesses the trigger that it sees. It syncs up with all the
   files that the advertisign nick has, with same UpdateCount.
   This makes all the nicks to have same update count. Which implies that
   the UpdateCount will make the entry invalid for all nicks at the same
   time, and hence will trigger them to access an Ad of nick, at the
   same time, when presented.
   To avoid this one more modification is proposed:
   ------------------------------------------------
     Response to a nicklist command.
     First nick in list is ours with update count = 0 (same as before)
     All the nicks that follow are listed if the UpdateCount is not
     FILES_DETAIL_LIST_UPDATE_ROLLOVER - 2. And all the nicks that are listed,
     are listed with UpdateCount = one more than what we have. This will
     make sure that all the MMs have a different UpdateCount. Helping it
     distribute when an MM will issue the trigger, so that they dont fall
     on an Advertisement all at once.
     Have to analyse this.
   Apr 5th

r) Set FSERV_INITIAL_AD_TIME to 30 minutes. It was previously
   3 * FSERV_RELATIVE_AD_TIME. So on joining channel or after sending
   out an advertisement we set our Ad Time to FSERV_INITIAL_AD_TIME
   Apr 5th

s) The above backfired as we do not register it as an MM client on seeing 
   the ad, but while accessing the trigger. Hence the ADSYNC algo
   would ignore that ad which is seen the first time.
   So made changes so that its registered as an MM client as soon Ad is seen.
   Removed the Client type setting in ToTriggerThr() and ToTriggerNowThr()
   Apr 5th

t) If file size is not available, Sysreset lists the filename entry as:
   file.ext N/A
   Currently MM interprets it as a directory and hence hangs in the file
   server.
   Implement this.
   Apr 5th

u) In Linux, getConnection(outgoing), on a failure case, takes 180 seconds.
   So given scenario: Linux not firewalled, sysreset firewalled: When
   sysreset issues the linux trigger: Linux MM tries to connect to sysreset.
   For it to fail it takes 3 minutes. By this time, Linux MM has removed this
   entry from FileServerWaiting. But it issues a DCC chat invite, and on
   sysreset connecting to that chat invite, Linux MM has no clue what that
   connect is for. Hence things dont work out.
   Hence increasing TimeOut to 240 seconds for all ports for 
   FileServerWaiting. Note on Windows it is OK.
   Apr 5th - Released
   ====>>>> RELEASED - contains FFLC version 1.0

v) AdSync issue. On joining channel, though it sets ad delay for a long time.
   But, on seeing an MM ad, it calculates the ad to be 3 minutes after it, as
   it knows of only the recently seen MM ad, and itself. Hence the ad is
   forced out in 3 minutes - throwing the ad sequence off, and all jumping
   on it as its a new client. Hence we add a patch up - If N < 5, FServAdTime
   is calculated as current time + FSERV_INITIAL_AD_TIME. Which translates to
   if there are < 5 MMs, then give ad out spaced 30 minutes apart. We hope
   that within the next 5 Ads some one in channel long enough, is contacted
   and a transfer of MM info is done.
   Apr 6th

w) Trying to cancel a file in waiting from a sysreset client, Crash in
   DCCChatClient(). Happens only in DEBUG build. Wrong COUT statement.
   Apr 6th

x) Bug - in DCCChatThr.cpp: While getting ready to issue the trigger,
   we fill up DCC_Container->TriggerName, only if SendFD is non NULL. It is
   filled up from value in FServFD, hence needs to be filled out nonetheless.
   This resulted in SEGVs as code down the line accesses TriggerName assuming
   its non NULL.
   Apr 7th

y) We are not recognising an Iroffer FServ. Had to Correct FServParse
   class to recognise the Iroffer FServ as a sysreset client. Also, there was
   a bug, in which it asumed the iroffer ad by a [IM]- nick as an XDCC and
   was trying to process it as such.
   Apr 8th

z) Memory leak spotted in Helper::helperFServNicklist(). FD was not getting
   freed, if its update count was greater than 
   FILES_DETAIL_LIST_UPDATE_ROLLOVER
   Apr 8th

a) Collect overall Bytes sent/received for purely the FFLC. (Fast File List 
   Collection algorithm). We can use it to see the overhead that it costs
   us. Do not add normal Sysreset traffic in this. Have to add in XChange.hpp
   the variables: FFLC_BytesIn, FFLC_BytesOut.
   The BytesIn and BytesOut will be doubles.
   We print this information after each filelist exchange, wether we be
   the client or the server.
   Apr 8th

b) In DCCChatThr.cpp, in the case when we are not able to connect to 
   the server, we are currently adding an FD entry of "No Files Present".
   This is wrong. We should add it as "Inaccessible Server". Correct this.
   Also in the FFLC, we do not exchange TriggerTemplate and fileinfo if the
   FilesDB has "Inaccessible Server" for that nick.
   Apr 8th

c) Update the IP addresses of nicks when we get a chance. Currently we update
   the IP in:
    DCCThr.cpp : On a userhost response.
                 On a CTCPFSERV received.
   This takes care of the case when someone tries to access our trigger.

   What about when we try to access someone elses trigger ?
   In this case, we issue the trigger, and wait. We can be accessed in two
   ways. One is in getting a IC_DCC_CHAT in DCCThr.cpp. Other is an incoming
   connection in DCCServerThr.cpp as a DCC SERVER CHAT by another nick for us
   to access his file server. (basically receive "100 nick")

   Updated IP of Nick in DCCThr.cpp in IC_DCC_CHAT case.
   The second case (DCC SERVER CHAT), we update it in DCCChatThr. Now, DCCThr
   anyway calls DCCChatThr. Hence we just update in DCCChatThr for all cases.
   Add a RemoteLongIP in DCC_Container, which is set before spawning
   DCCChatThr. And all updates done in DCCChatThr.
   Apr 8th

d) TimeLeft in Downloads, always showing UNKNOWN. bug introduced earlier in
   making it show UNKNOWN on speeds less thatn 500. Corrected it properly.
   Apr 9th

e) To have a better exchange, we serialise DCCChatClient.getDirListingMM(), 
   so that the transfers are more complete. ie, no half ass info sent 
   wrt a nick is sent as part of exchange.
   So add a class specific MUTEX which serialises only getDirListingMM().
   - This is later revoked, will see if it really needs to be done in the
     future.
   Apr 9th

f) Sysreset can also give this thing at the start:
   ['C' for more, 'S' to stop]
   In that case, we need to send an S, and continue on.
   Change in DCCChatClient::waitForInitialPrompt()
   Apr 9th - have to test.

g) FFLC - one more optimisation. Right now we always get list of nick
   if he has updatecount lower than what we have. So we introduce a checksum
   associated with the files held by nick. Some checksum which takes, filename
   , dirname, filesize, to generated it. Hence if checksum is same, instead
   of exchanging, we just update our updatecount appropriately.
   So add FilesDetailList:getCheckSumString(char *nick, char *chksum);

   It returns the 20 character kind of SHA1 of all the Files of Nick, 
   based on the FileSize, DirName, FileName.
   Hence nicklist will return: <nick> <updatecount> <ip in hex> <SHA>
   If we have nick info, and our updatecount is more than what is received,
   we do as follows:
   Calculate the SHA of that nick as per what we hold. If SHA differ, we
   exchange as usual. If SHA is same, we just update the updatecount of the
   nick with the new updatecount, and no exchange takes place.

   We make it compatible with Apr 5 exchange too. FFLC version 2
   The version is determined as follows:
   If version 2 MM is server, then its OK. he sees the initial nicklist command
   issued and will know what client he is talking to. Example:
    nicklist => version 1
    nicklist 2 => version 2
   But what do we do if version 2 MM is client and has to issue nicklist
   first ? no other way than to check in the initial prompt.
   Hence need to modify the nicklist command parsing. If we are server,
   and receive "nicklist" we assume version 1. If second parameter, then
   that dictates the version number.
   If MM is the client, then at the prompt, we check the MasalaMate version, 
   and determine what version protocol it supports. So we change
   FileServer::welcomeMessage() to add an FFLC protocol number.
   So FileServer::welcomeMessage() changed to say:
      "Use: commands listed above - FFLC num\n", instead of
      "Use: commands listed above\n
   Hence we get the version number if we are client.
   So #define FFLC_VERSION in FileServer.cpp

   And as client, we change Helper::helperFServStartEndlist() and
   check what kind of a "nicklist" line we get.
   nicklist without param = version 1.0
   nicklist with param, then param = version.
   version is an integer. Each change we increase by 1.
   So that function will return 

   Hence add a private "int FFLCversion" in DCCChatClient class. Use that
   version number as parameter when calling the Helper class functions.

   Case when we are client: (implementation)
   Added FFLC version in FileServer welcome message.
   correspondingly DCCChatClient::waitForPrompt() catches the version number,
   and updates its FFLCversion - note default is set to 1 anyway.
   Now client issues, nicklist version which is FFLCversion and not
   FFLC_VERSION. -> we talk in the version that the server understands.
   so issued "nicklist version"

   Case when we are server: (implementation)
   Added a private called FFLCversion which notes down the version of the
   client. This is from how the client issues a nicklist command.
   Apr 9th

h) In ToTriggerThr.cpp, where we issue the trigger after sleeping for a while,
   after finishing the sleep, we check again if we need to issue the trigger.
   As an FFLC would have gotten us the information.
   Apr 9th 

i) Bug - FilesDetailList:getCheckSumString(), The CheckSum should not contain
   space or string terminator. We did take care of substituting '0' for
   '0'. We also need to substitute '_' for ' ', 'N' for '\n', 'R' for '\r',
   'T' for '\t'. We need to do this as, we treat it as a string, and dont 
   want '\0'. Plus, we use LineParse with a space delimiter. Plus, getLine
   of TCPConnect might get us truncated line on spotting '\r' or '\n'.
   Apr 9th

j) On a forced trigger access (TriggerNowThr and not TriggerThr), when
   the trigger is accessed, the server will list its nick's checksum against
   MyFilesDB. We compare it with what we have in FilesDB. That will never
   match, as FilesDB contains "TriggerTemplate" and posibly "Inaccessible
   Server", for that nick. Hence in FilesDetailList:getCheckSumString(),
   we ignore FileNames which are those. Also, if MM server is being accessed
   we dont delete all its files, as we will update only if checksums differ.
   Apr 9th

k) Collect overall Bytes sent/received for purely the Sysreset dir updates.
   We can use it to see the overhead that it costs us. Have to add in 
   XChange.hpp the variables: DirAccess_BytesIn, DirAccess_BytesOut.
   The BytesIn and BytesOut will be doubles.
   We print this information after accessing a FileServer for DIR listing.
   Apr 9th

l) One more change in FFLC v2. In response to a nicklist, we also pass on
   the nick's send and queue information, and Firewall information - cause 
   we possibly wont exchange the filelist
   Hence nicklist will return: <nick> <updatecount> <ip in hex> <sends>
   <total sends> <queues> <total qeues> <firewall> <SHA>
   = 9 words.
   We dont update firewall entry if firewall is set as IRCNICKFW_UNKNOWN
   Apr 10th

m) In TCPConnect class, for server socket (used by everything for us), 
   backlog is set to 1. Change it to 32. Max allowed seems to be 128 in
   linux, 32 in solaris.
   Apr 10th

n).We should internally hold information regarding a nick's firewall status 
   as and when we access their trigger. The three states being: 
   Not Firewalled -> 'N'. Firewalled -> 'F'. Maybe Firewalled -> 'M'. 
   Unknown -> 'U';
   This information needs to be internally collected for SysReset, Iroffer,
   MasalaMate. The states should be maintained in XGlobal->NickList. Initialise
   to 'U'. As we go along and if successfully able to connect to a remote 
   MM at its DCCServer port, as part of file serving it, 
   we mark it as "Not Firewalled". We start with 'U'.
   If not able to connect at DCCServer port, mark it as 'M'. If on next
   attempt again not able to connect at DCCServer port and state is 'M', mark
   it as 'F'.
   For Iroffer, we get that information from the Firewall line.
   For SysReset, we get that information on accessing the server, the 
   8124 line that we get back.

   Implementation:
   taken care of for sysreset firewall, in IC_NOTICE in FromServerThr.cpp
   taken care of in DCCChatThr.cpp, when trying to connect outgoing.
   Iroffer, we check in FromServerThr for IC_PRIVMSG to channel main, and
   in IC_NOTICE.
   In DCCServerThr, on a valid Chat access, we mark our nick as non firewalled.
   The RED/GREEN logic remains the same. We send our Nick's firewall state
   by looking at XGlobal->Nicklist

   Testing:

   1. MM -> !list sysreset (non firewalled) - check file list - pass
   2. MM -> !list sysreset (firewalled) - check file list - pass
   3. MM -> sysreset ad (non firewalled) - check file list - pass
   4. MM -> sysreset ad (firewalled) - check file list - pass
   5. MM -> sysreset ad 10 times (non firewalled) - check file list - pass
   6. MM -> sysreset ad 10 times (firewalled) - check file list - pass
   7. same as 1 thru 6, with MM being firewalled.

   8. MM Stealth <-> MM WinME
      Stealth issues !list WinME
      WinME connects to stealth => stealth = non firewalled.
      so Stealth will list winME files with unknown.
      WinME lists Stealth files as non firewalled
      - pass

      Now WinME issues !list Stealth
      Stealth connects to WinME => WinME = non firewalled
      stealth sends winme open, stealth open
      so stealth will received: winme open, stealth open
      So stealth should show winme as open now.
      - pass
   Apr 10th

o) Add the Firewall information in the listing for "File Search"
   Apr 10th

p) Add a filter like "Open Sends", "Open Queues" called "Non Firewalled"
   and filter results taking that into consideration as well.
   Apr 10th

q) Do not serve nor accept file in dir listing which have name "thumbs.db"
   Apr 10th

r) Modify FServParse class to parse the Firewall line of IROFFER.
   Add an IROFFER_FIREWALL_LINE of TriggerTypeE, for the firewall line
   parsing. Added isIrofferFirewalled() to return bool, if line is of type
   IROFFER_FIREWALL_LINE
   Apr 10th

s) Have to change IRCLineInterpret class, so that it recogizes the response
   of a CTCP PING issued. The response is as follows:
   We issue: /ctcp nick PING timestamp.
   We get the reply: NOTICE mynick :\001PING <same timestamp>\001
   We need to identify this correctly and update the Server window.
   So created: IC_CTCPPINGREPLY.
   Apr 10th

t) Same lines as IC_CTCPPINGREPLY, for IC_CTCPVERSIONREPLY.
   Apr 10th

u) Have to change IRCLineInterpret class, so that it recognizes the response
   of a DCCALLOW issued:
   We issue: DCCALLOW +nick or DCCALLOW -nick
   Response is as follows:
   617 mynick :nick has been added to your DCC allow list
   617 mynick :nick has been removed from your DCC allow list
   So created, IC_DCCALLOWREPLY
   Apr 10th

v) Add the below in the Right click menu, acting on the highlighted nick.
   PING, VERSION, +DCCALLOW, -DCCALLOW
   Apr 10th

w) Allow /whois command, and add in popup too.
   Apr 11th

x) Add the below in the Right click menu, acting on the highlighted nick.
   Op commands: OP/DEOP/VOICE/DEVOICE
   Commands such as: op/deop, hop/dehop, voice/unvoice,
   To make it easier to parse the text, we generate all of these commands
   with a prefix of /op
   -> /op op nick etc.
   Apr 11th

y) For kick its /kick nick reason. We put a default reason = Chumma
   Apr 11th

z) Support the /nick command.
   Apr 11th

a) Serialisation of getDirListinMM() is not enough. Have to serialise
   the access from FileServer too, that is when I am server and serving
   to other FFLC clients.
   Actually removing all serialisation, till we see some major reason
   why it might be really necessary.
   Apr 11th

b) The Op commands in the popup should be grayed out if not op.
   can show or hide the OP buttons.
   Two new TabBookWindow.cpp functions: showOpButtons() and hideOpButtons()
   So onCmdPopUP(), should check our nick and if we are an OP in that
   channel and call show.. or hide.. appropriately.
   Apr 11th

c) Still Firewall shows unknown for iroffer files. Also it doesnt seem to
   be grabbing the send information right on the iroffer ads.
   Bug in FromServerThr, in condition to call FServParse to analyse
   it as a Firewall line.
   Apr 11th

d) After manually accessing a trigger of:
   <Kaltak> [Fserve Active] - Triggers:[/ctcp Kaltak MasalaMovies & 
   /ctcp Kaltak moreMasala1 & /ctcp Kaltak MoreMasala2] - Users:[0/5] - 
   Sends:[0/1] - Queues:[0/5] - SysReset 2.53
   by issuing: /ctcp Kaltak MasalaMovies
   I got the list. When I double clicked to get the file, it went 
   and did a DIR listing, instead of issuing a GET.
   Code in TabBookWindow.cpp handling the /ctcp case, synced up with
   ToTriggerThr.cpp code.
   So this brings us to another issue, When we issue such manual ctcp triggers
   we do not have the Sends and Q information. Hence when then connection is 
   established, we should for all DIR requests, when we are waiting for
   the prompt, see if a sends and q information line pops up. If it does,
   we update all entries in FilesDB with that latest sends/q info.

   MM line: Transfer Status: Sends:[1/2] - Queues:[0/10] (no color)
   SysReset line: Transfer Status: Sends:[1/1] - Queues:[10/10] (color)
   Apr 11th

e) Pinging the iroffer bots returns arbitrary seconds.
   Bug in calucalaitng the ping, we did rcvd - cur time, hence worked
   only in cases when it was 0 seconds ping.
   Apr 11th

f) If hop, show the voice/devoice/kick/ban in right click.
   Apr 12th

g) Before issuing trigger in ToTriggerThr(), ie after it wakes up, we
   should check is that nick is in CHANNEL_MAIN. If not, discard and move on.
   Apr 12th

h) When cancelling a download, We do not do the 10 second delay if its from
   an XDCC server.
   Apr 12th

i) It disconnects and never is able to successfully reconnect.
   Some bug. No clue as of now. Might be cause of memory corruption too.
   OK - got it, introduced by the LineQueue.flush being called on an
   IRC disconnect. The producer = IRCClient issues flush. It used to 
   destroy the semaphore and recreate it to set it to 0. All the consumers
   were waiting on a down, and hence they error out and we dont check for
   error, and consume the line, which is all wrong and garbage.
   So no destroy and recreate of semaphore. So modified flush() to
   look thru the list and delete entry at the same time do a down on the
   semaphore. This also can lead to problem if producer calls it, as
   say consumer does a down, and before can lock, the producer locks mutex,
   it will have one more entry than the semaphore count, => will do one
   more semaphore down than what its value is => it will hang.
   Hence only the consumer should call flush().
   We want to flush all the LineQueues except IRC_ToUI, UI_ToUpnp
   Hence all the other queues: IRC_ToServer, IRC_ToServerNow, 
   IRC_FromServer, IRC_DCC, IRC_ToTrigger, IRC_ToTriggerNow,
   UI_ToDwnldInit: all these on getting a line, should check the State
   of the IRC connection. If its not in connected state, just continue the
   while look without processing.
   We added function bool isIRC_DisConnected() in XChange Class. It returns
   the state of IRC connection by looking at IRC_Con.
   for safety removed the LineQueue.flush() function. - commented out.
   Apr 12th

j) Update Help.cpp to make it current.
   Apr 12th

k) Forgot to return true on success in FileServer::fservGet()
   Apr 13th

l) I updated server with new file. Apache did a !list on me, and updated.
   Still the new file didnt show. Problem in helperFServStartEndlist().
   We should always update an entry for which we receive an update count
   of 0.
   Apr 13th

m) LG reported hang in the UI update, but all else working. Seems like some
   problem in App->addTimeOut() possibly not being mutex locked.
   So in UI.cpp, made all callse to ->addTimeOut or some such - mutex
   protected with the Applications global mutex.
   Apr 13th

n) Time Left shows as -ve in FileServer Tab. Last change was done for
   the Download TAB. Now fixed that too.
   Apr 14th

o) As in Iroffer code, set SO_SNDBUF in setsockopt() to 65535 except
   for _OS_SunOS - Both DCCServer and outgoing connect - TCPConnect.cpp 
   Apr 14th
   -> Release

p) Bug in TransferThr.cpp, FD being accessed on NULL. Corrected.
   Apr 14th
   -> Sneak Release

q) The connection to the IRC server shouldnt have SO_SNDBUF set, as per
   Iroffer code. Hence default we set the socket options. But for IRC
   socket, we call it with .setSetOptions(false), which will bypass it.
   Same is true for the UPNP socket.
   Also cleaned up TCPConnect => getSockServer(), getSockClient(). These
   are not used at all.
   Apr 15th

r) In UI.cpp, we no longer need to add it as a timeout. We get lock
   and call the update function directly. The semaphore stays as we want
   to process only one line at a time.
   This cant be done, as the Mutex is for the App and not for the UI.
   Somehow it hangs - have to research or revert back.
   We revert back.
   Apr 15th

s) Mirc like feature:
   If I am not in last line of window, extra text should not scroll.
   - Doesnt work properly yet.
   Apr 15th 

t) If Dir Name has more than one space between words, sysreset has a bug 
   in which it doesnt allow CDing to that directory. - sysreset bug.
   The message returned is: "Invalid folder". Hence we just note that the CD
   has failed and move on.
   Apr 17th

u) Some users say it ping timeouts and reconnects. We need to spiff up the
   ping. Currently we only ping if we dont receive any data from server for
   3 minutes. We need to put this in Timer, and a global variable which
   notes the time of the last ping/pong. If TimerThr() finds that time is
   larger than 2 minutes, it issues a ping. Hence now, in IRCClient.cpp
   we do not issue a ping on recvLine timeout.
   Apr 17th

v) PORTCHECK results should print as * lines. Currently it prints as if
   the user is saying that in channel.
   Apr 17th

w) Crash in calls to removeColors(), if the input string is an empty string.
   Happens in DCCChatClient::waitForInitialPrompt(), when it calls
   removeColors(), which calls removeConsecutiveDeLimiters() of LineParse
   in the end. That last call creates junk.
   Apr 18th

x) Queues and Sends not cancelled properly on user QUIT. Modified FromServerThr
   so that if the word "Quit" is found in the QUIT message, assume its a user
   forced quit. Ping Timeouts, Connection reset..., do not count as a forced
   user quit.
   Added a condition to disconnect Connection if non NULL. 
   Apr 20th

y) Memory leak when pressing Clear Screen in clearing complete/partial
   downloads
   Apr 20th

z) The Ad algo needs a little tweak. For example, do not participate in the
   Ad algo unless we have seen 10 other MM ads. So till that time do not change
   the Ad time, which will be set to 30 minutes from channel join time.
   Also the AD with Firewall: UNKNOWN is not required now - to be removed.
   Note that the problem also stems from the fact that when an ad is displayed
   the confusion is usually between the next guys who should Ad. So they both
   display at the same time. And correct, and the next one again displays.
   So a little repetetion dance. It might be OK.

   - Initial AD time set to current time + 30 minutes in Channel join.
   - Helper::generateFServAd() - removed UNKNOWN.
     -- removed static AdFirstTime
   - In FromServerThr(), take a note of how many MM ads we have seen so far.
     Update FServAdTime only if we have crossed 10 add #define FSERV_MM_ADS_SEEN
     int mm_ads_seen, initialised to 0 on channel join.
   Apr 20th

a) When cancelling a download, We do not do the 10 second delay if its from
   an XDCC server. We did do this before, but it had an error. We should
   check if TriggerType != XDCC.
   Apr 21st

b) On typing: /ctcp nick NoResend, it assumes it as a file server access. 
   It shouldnt.
   Apr 21st

c) Linux version not getting files from sub directories.
   There is more to this than what meets the eye. FilesDB on all clients
   has dir names seperated by '\'. MyFilesDB is dependent on OS. Hence
   windows has '\' and Linux has '/'. So Firstly we need to maintain a
   consistent dir seperator which works for all OS'es as this information
   is propagated using FFLC. The File Server should also use consistent
   notation independent of OS, ie. '\' for File Server Dir Names.
   So components to be changed ->
   - MyFilesDB to hold information with DIR_SEP as FS_DIR_SEP - done
   Hence the DirName in all FDs will hold FS_DIR_SEP, for unix and windows.
   - FileServer changes in fservDIR - done
   - FileServer changes in fservGET - done
   - FileServer changes in fservCD - done
   - DCCChatThr changes in populating FileName - done
   Testing ->
   - Windows MM ,sysreset, 2 level dir serving. try send/recv on each - pass
   - Linux MM, sysreset, 2 level dir serving, try send/recv on each - pass
   - Windows MM, Linux MM, 2 level dir serving, try send/recv on each - pass
   Apr 23rd
  
d) Linux version shows two %% signs in the Progress field.
   Apr 23rd

e) Show the user mode in the lines eg. @, %, +v etc prefixed before that user's
   name in the messages sent/received in channel.
   Apr 24th

f) On issuing a /ctcp nick TIME, the reply is got as a notice, and with all
   notices it goes in the Messages TAB.
   Add CTCPTIME, to recognice a ctcp time, reply to it appropriately.
   Add CTCPTIMEREPLY, to recognise a ctcp time reply. Put it in Server tab.
   Apr 24th

g) Add CTCP time in the right click popup menu.
   Apr 24th

h) Modified FilesDetailList::isFilesDetailPresent(FilesDetail *FD), such that
   it checks against Nick, FileName, DirName to decide if they are same.
   Apr 24th

i) In "File Search", move the order of the tabs.
   File Name, File Size (MB), Directory Name, Nick, Sends, Queues, FireWalled, #
   So DwnldInitThr, now gets a string. \001 is word seperator.
   first word = GET, second = nick, third = filename, fourth = dir name.
   if dirname comes out to be space, then no directory name.
   Apr 24th

j) On double clicking in the File Search, we need to pass on the nick, filename,
   and the directory name. The dowload should take place of the FD which has
   all those 3 conditions met.
   Added FilesDetailList::getFilesDetailListNickFileDir(), used to get the
   correct file.
   Apr 24th

k) Scroll control stops working if minimised. Hence to lay it to rest all
   these problems, have a Scroll Toggle button amongst the buttons. default
   it to force scrolling. If you want to stop scrolling, untick it.
   Apr 25th

l) Something similar to system info in sysreset .. giving details of the 
   os etc of the user which might help in debugging
   info as in -> MM version, Win/Unix version, Upnp, internal/externalip, 
   firewall status. Access is from ctcp clientinfo.
   Taken care of as: IC_CLIENTINFO, IC_CLIENTINFOREPLY
   Add it in the Right click pop up menu too.
   Apr 25th

m) Got some functions which were independednt of other classes in the
   utilities category - Utilities.cpp/hpp is born. Get some functions
   of Helper.cpp in it.
   Apr 25th

n) Linux downloads, files created with 000 permissions.
   Apr 25th

o) In Search, the DirName of files from MM clients starts with a \
   They work when download attempted, but investigate.
   As it is functional, let it be as is. Just fix the display so that
   its consistent.
   Apr 26th

p) Replace Client information with OS information in CLIENTINFO response.
   Add function getOSVersionString(char *osversion) in Utilities.cpp
   osversion should be able to hold about 128 bytes at least.
   Apr 26th

q) It would be cool if the File Search list was sortable by Nick/FileName/
   DirName, Size.
   To get this to work, we need to add a right click popup on the File Search
   tab. It will have buttons for:
   - Sort by FileName
   - Sort by FileSize
   - Sort by Nick
   - Sort by DirName
   - Download
   So for this popup menu, we need to make IconList UI (File Search etc)
   to respond to TabBookWindow::ID_MOUSERELEASE, so that onCmdPopUp gets 
   called.
   Apr 26th

r) Create PopUp for the "Downloads" UI. It will have buttons for:
   - Cancel Selected Download 
   - Clear Selected PARTIAL/COMPLETEs
   - Clear All PARTIAL/COMPLETEs
   Apr 26th

s) Ignore SIGPIPE in non Windows.
   Apr 27th

t) Create PopUp for the "Waiting" UI. It will have buttons for:
   - Cancel Selected Queue
   Apr 27th

u) Create PopUp for the "File Server" UI. It will have buttons for:
   - Send Selected File
   Apr 27th

v) Clear Screen in Server window doesnt work.
   Apr 27th

w) Save Font name/size in the MasalaMate.cfg file. So we start with 
   user chosen font on restart.
   Apr 27th

x) Do not allow portcheck or portcheckme to be directoed to our own nick.
   Its invalidand gives wrong information.
   Apr 28th

y) Add a Client line in the "File Search" tab.
   Apr 28th

z) Change color in the label for the TAB for Waiting/Downloads
   on new activity.
   As this even is known only to the non UI classes. We add a UI message
   called "*COLOR* WinDowName". This will prompt the UI to mark that window
   as RED, if its not the current FocusIndex.
   So each time we do something which changes the "Waiting" or the "Downloads"
   Window, we send a message to UI, with -> "*COLOR* window_name"
   Waiting -> DwnldWaiting 
   So when new entries are added to DwnldWaiting, we change color.
     Do not worry about entries getting deleted.
     Change in DCCChatClient.cpp and DwnldInitThr.cpp
   Downloads -> DwnldInProgress
   DwnldInProgress, we change color, if an entry is added, or when
   the download is stopped or complete.
     Change in TransferThr.cpp.
   Apr 28th 

a) Add a Find Text in the top menu bar. It should be made up of:
   INPUT TEXT, Arrow button
   So inputting text, and hitting the key, will search the current tab
   (ScrollUI/SearchUI) and highlight it. All are defaulted to case insensitive
   searches. They wrap around.
   Apr 28th

b) When moving from partial to serving. First try a link, unlink. If that
   fails, only then do it the hard way of copying.
   Valid for non Windows port. MINGW doesnt seem to have link - recommended
   to use SHFileOperation, but then copy is fine.
   Apr 28th

c) On clicking tabs, switching from FileSearch and back. It takes a long
   time when list is long. I think we do a scan to update the title.
   Hence when we first populate the TAB, and set the title, save the title,
   and reuse it, rather than scanning the whole list.
   Apr 28th

d) Downloads TAB - change fields as below:
   File Name, File size (MB), Nick, Rate, Progress, Cur Size (MB), Time Left, #
   Functions which update that TAB need to be modified
   - cancelDownloadSelected()
   - updateDownloads()
   Apr 28th

e) On a download terminating - cause of partial or complete. Update, its
   FD->FileResumePosition, to reflect initial resume position + bytes
   received in download. This will ensure that the values printed in % complete
   and current file size are accurate.
   Apr 28th

e) In the Downloads TAB. An entry which says PARTIAL under Time Left, should
   be able to be re requested on right clicking on it.
   For this, we need to do the below:
   - Add "Re-Request File" in the Right click popup for Downloads.
   - Make the click on it call TabBookWindow::onDownloadFileSearchSelected()
   - Make sure that DirName gets propagated on an attempted download, from
     DwnldWaiting q, to DwnldInProgress q.
   - Change onDownloadSearchUISelected(), such that it takes an input
      which is the Window Name. So it can initiate a download accordingly.
   - Before actually Rerequesting, check if the nick/file/dir is present
     in FilesDB, if not, give a prompt. - This alone is not tested.
   Apr 28th

f) BUG: Cancel download, if XDCC its still delaying 10 seconds.
   On immediately getting a download we bypassed the waiting Q, and hence
   lost the DirName and TriggerType information.
   Apr 28th

g) In the Waiting TAB. The entries should be selectable and retried. Just
   like we did for the Downloads TAB.
   For this we need to do the below:
   - Add "Update Queue Information" in the Right click popup for Waiting.
   - Make the click on it, call TabBookWindow::onDownloadFileSearchSelected()
   Apr 29th

h) Request to have the fields in Downloads TAB as follows:
   Change from: FileName, Size, Nick, rate, Progress, Cur Size, TimeLeft, #
   - FileName, Size, Cur Size, Progress, rate, time left, nick, #
   Functions which use/update that TAB need to be modified
   - clearDownloadSelected()
   - cancelDownloadSelected()
   - updateDownloads()
   - onDownloadSearchUISelected()
   Apr 29th

j) A Progress Bar, when sorting files in File Search Tab. As listings
   get big, it takes quite a noticeable amount of time to sort them.
   So we have a ProgressBar horizontal line with % text embedded in the
   Menu Bar, next to the Search Text. It needs to be global as will
   be accessed via the sort function too.
   Apr 29th

k) Couldnt have fine grain control over ProgressBar, when the SearchUI 
   sorts items.
   So wrote our own heap sort for the SearchUI TAB.
   Apr 29th

l) During sorting in File Search TAB, make sure the # is update correctly.
   Now remove the sorting in FilesDB.
   Its the 9th word
   Apr 29th

m) If we have huge list in FilesDB. When user clicks "List All" in File Search
   TAB, we call search...(), to get the count, of the list, to initialise
   ProgressBar. That itself take some time and there is a pause.
   Modify the search...() function to return count in second parameter,
   if its not NULL.
   Functions to change in FilesDetailList:: ->
   - getFilesDetailListOfNick(searchstr)
   - searchFilesDetailList(searchstr)
   They will now take optional second parameter, if Non NULL, assume it to 
   be a pointer to int. Update it with count on return.
   Apr 29th

n) Clear Selected or Clear All partial/complete not working in Downloads
   folder.
   Apr 29th

o) On being sorted it should update the corresponding header with the sort
   symbol in the Search UI tab.
   Apr 30th

p) Clicking on the Header elements in the SearchUI TABS, should automatically
   sort that UI based on the header.
   Apr 30th

q) Optimising the sorting, as using the internal sorting and updating the
   ItemStr in the UI as we sort, is very slow.
   To bypass that, what we could do is,
   always hang on to an FD list, which is in display in the UI. (instead of 
   deleting it after populating the UI.). We could then create an array, which
   holds the list. And then run it through our sorter.
   Then we link them in order.
   So we do the below:
   - have a private in TabBookWindow, which holds FD link list of File Search
     It is saved there each time a new search is done. This list will be in
     sync with what is shown in the UI -> FileSearchFD, FileSearchTotalFD.
   - Now in heapsortFileSearchTab(), we create an array of FDs, and sort them.
   - Create a compare function in FilesDetailList class, which takes a criteria
     and two FDs and returns +ve, 0, -ve accordingly: compareFilesDetail()
   Apr 30th

r) For each of the SearchUI, create a #define for the header item index.
   This stays in TabBookWindow.hpp. This helps in easier, repositioning
   of the header items. All code in the cpp, files to use these #defines.
   Apr 30th

s) When Ad becomes too big, some servers do not show the Sysreset word in
   Ad. Hence set an identified CTCP trigger as a SysReset line by
   default.
   Apr 30th

t) On clicking on word in Channel text, we do highlight the nick if its a nick.
   Also paste that word in the Search Text field in the topline widget.
   Apr 30th

u) Preserve, Waiting and Partial or ongoing Gets, over quits/restarts.
   Call it PartialWaiting
   Partial and Ongoing Gets, basically fall in the category of Partial.
   Partial needs to save these information:
   - FileName, File Size, Cur Size, Nick, Dir Name (space = NULL)
     Cur Size is nothing but FileResumePosition in case of a Partial one,
     and FileResumePosition + BytesReceived in case of a Downloading one.
   Waiting needs to save these information:
   - FileName, File Size, Nick, Dir Name (space = NULL)
   Dir Name though is not present in the UI, it is needed to be in the FD.
   so in readConfigFile(), we populate both DwnldInProgress from [Partial]
   and DwnldWaiting from [Waiting]
   Its in form Partial1=all info as listed above, seperated by * as DeLimiter.
   We write this information out only when we are quitting and periodically
   every say 10 minutes, from TimerThr
   Apr 30th

v) Added one new IRC line called IC_SERVER_NOTICE.
   Looks like a bug in the ircd. Its sending out a line as:
   NOTICE Stealth :*** Your host is ...
   This then was erroneously getting interpreted as a NOTICE line
   with a NULL LineInterpret.From, which caused problems.
   May 1st

w) Right click menu, when mouse hovers over button, the button should be 
   highlighted. Even with Button set to TOOLBAR_BUTTON, it doesnt seem
   to behave like that. 
   So, they shouldnt be FXButtons, but FXMenuPane. Now they work.
   Also the Seperators should be FXMenuSeparators.
   May 1st

w) PARTIAL_WAITING_SAVE_TIME_INTERVAL, make it 10 minutes again after the
   Partial/Waiting save is verified to work.
   May 1st

x) Add MenuCommands in the Downloads, Waiting and File Search TABs, called
   - Search for File
   - List Files of Nick
   - Update Files of Nick
   They will work on the highlighted line, and act on it.
   Function actionOnSearchListUpdate(int Sel_ID) handles them all.
   May 1st

y) File Search UI on getting /clear, should just clear out and not search
   for /clear.
   May 1st

z) Add "List All Files" in popup for File search.
   May 1st.

a) The Search Text can be made to search very much better in the "File Search"
   tab, as we can now go thru the list we maintain, and can highlight that
   particular item.
   Improve: onFindNextPrevious()
   Implement strcasestr() in Utilities.cpp
   May 1st

b) When we join channel, we should say:
   * I, known as Nick, have joined channel ...
   So we know the nick.
   May 1st

c) Canceling a download in progress doesnt work.
   Alpha tried it with a bot download.
   The Periodic UI Update on the Waiting/Download/FileServer TABs, save 
   and restore the Selection. But they trash the Current Item => 
   reset to first element.
   Hence those functions should preserve that as well.
   Change in updateDownloads(), updateWaiting(), updateFileServer()
   May 1st.

d) Clicking on label sorts in order, and each time clicking will reverse
   the order. So get the Arrow Dir, and use it as guideline for sorting
   order, reversing it each time.
   So we start with ArrowDir as FALSE = ascending.
   heapSortFileSearchTab(), is called with the criteria, we add one more
   parameter bool = descending. = same as ArrowDir.
   same with heapSortSiftDownFileSearch()
   same with FilesDetail::compareFilesDetail()
   May 1st

e) Add a Nick in the Menu. Its items will be like:
   - Nick: CurrentNick
   - Pass: CurrentPass
   So on clicking on it, the value can be changed. A click brings up
   a FXInPutDialog::getString() 
   May 1st
   
f) After a sort is issued, we need to position ourselves at item index 0.
   May 2nd

g) On clearing entries, (partial/complete in downloads) (cancel in waiting),
   remove the item in the UI immediately.
   Change in clearDownloadsSelected, cancelWaitingSelected
   May 2nd

h) The right click menu on channel is very long.
   Put them in CTCP, DCC, OP Control as FXMenuCascade.
   May 2nd

i) Remove the right click menu in File Search, related to sroting.
   Dont need it anymore.
   Remove ID_'s related to it.
   Remove FXMAPFUNCs related to that ID_s
   Remove creation of those FXMenuCommands.
   Remove handling of these events in onSearchPopUp
   All these are currently under #ifdef 0
   Sometime in the future we can delete those lines.
   May 2nd

j) Ad tweak revisited. How about participating in Ad sync, only if N ads
   are seen. where, N = max of 10 and number of MMs known to us are in channel
   Hence participate if
   (mm_ads_seen >= FSERV_MM_ADS_SEEN) && (mm_ads_seen > N)
   May 2nd

k) Move the whole Connection part of File -> Options, 
   move it under a heading called Connection.
   Its items will be like: 
   - Type, Host, Port, User, Pass - Just like its in Options now.
   Possibly an Accept and Cancel Button too.
   So clicking Connection will give a pane which will have the below commands.
   - Type.
   Type will further open a Menu Pane consisting of the below which can have
   one check mark.
     - Direct
     - Proxy
     - BNC
     - Socks 4
     - Socks 5
   So the menu items below Type, are dictated by what Type is ticked.
   If Type = Direct, no menu panes under Type.
   If Type = Proxy, menu panes under it are as follows:
     - Proxy Host:
     - Proxy Port:
     - Proxy User:
     - Proxy Pass:
   If Type = BNC, menu panes under it are as follows:
     - BNC Host:
     - BNC Port:
     - BNC User:
     - BNC Pass:
     - BNC VHost:
   If Type = Socks 4, menu panes under it are as follows:
     - Socks 4 Host:
     - Socks 4 Port:
     - Socks 4 User:
   If Type = Socks 5, menu panes under it are as follows:
     - Socks 5 Host:
     - Socks 5 Port:
     - Socks 5 User:
     - Socks 5 Pass:

   Type is always enabled. Default it as Direct clicked, and the 5 command
   panes below it as Hide mode.
   First to implement is the menu pane on clicking Type, which gives the
   diferent types that we could choose.
   May 3rd

l) Add "Get it from: http://groups.yahoo.com/group/masalamate" in the 
   Version replies. Reconnect QUIT message can have it too. Normal QUIT
   message to not have it.
   May 3rd

m) In File Search, on entering text and searching, dont blank out the 
   search string. As user can refine the search without retyping.
   May 3rd

n) In File Search, the UP and DOWN arrow keys should move up and down the
   search items. Currently the Entry Text widget grabs focus.
   May 3rd

o) Update Help Text
   May 3rd

p) When not particiapting in the FServ Ad, we were not moving forward our 
   Ad time to, cur_time + the default FSERV_INITIAL_AD_TIME
   Hence it just dumped the Ad every FSERV_INITIAL_AD_TIME seconds.
   Corrected.
   May 3rd
   
q) Pressing clear button should clear the input text too.
   May 4th

r) Connection to be moved under File.
   May 4th

s) BUG: In TabBookWindow::onDownloadSearchUISelected(), in the "Downloads"
   case we were copying FD->DirName to tmpdir, but tmpdir was being allocated
   as strlen(charp) + 1, which was wrong.
   May 5th

t) BUG: In FilesDetail:getFilesDetailListNickFileDir(), we need to remove
   the FS_DIR_SEP from both the src and dest directory names, before
   comparison. as both are either can be with or without it.
   May 5th

u) Change FilesDetail::searchFilesDetailList() such that it returns the list
   in same order it has stored internally. This change, will change the logic
   in how FileServer.cpp adds a nick in Q, and check other files
   where getFilesDetailAtIndex() and addFilesDetailAtIndex() are used.
   Changes in Helper, Timer etc. whereever the position seems ot be important.
   May 5th
  
u) When queued, and repositioned cause of priority, it tells wrong q # to 
   client. FileServer.cpp bug.
   May 5th

v) Chimero tries to download from me. (both same MM latest version). He
   gets file size as 0 and hence all other data in the Downloads is wrong
   when the send occurs. Happens when sent to a firewalled user.
   Possibly cause of & in filename.
   Bug in IRCLineInterpret. For IC_DCC_SEND, we are not picking up the
   right File Size. It should be the last word in line.
   May 5th

w) When a file is sent without us accessing the file server, we do not check
   if we already have the file in serving folder, or its already currently
   downloading. We should check in both DCCServer and DCC_SEND, and send a 
   notice rejecting the transfer.
   May 5th

x) MMs can download of each other Partial Directory Files.

   Basis of the thought: No one wants to serve. All want to download, be it
   a resume of a previous partial download, or a fresh one. This means,
   for the download to succeed, the Partial folder will not be tampered
   with, as it its disadvantageous to them.
   The Partial folder holds "Files which are incomplete", or which are
   currently downloading. There is, in fact, no need to differentiate the two.

   Now, the files which reside in the Partial Directory is what we will
   tap into.

   Normal operation = File Server.
   That is, files in the serving folder can be sent to clients. Each MM client
   has atleast 2 sends. So if MM server has files in the serving folder, then
   all clients can get at it.
   An MM client, can initiate a download of a file in the MM server's Partial
   directory

   Hence, an MM server with no files in the serving folder, but any file in
   the Partial folder, can potentially give to two other MM clients. That is
   two more sends. Hence we are creating 2 sends when intially there
   were none. Note, also that, an MM can request the Partial file, even if
   all sends are taken, it will be queued, like a normal file.

   This will indeed propagate Partial files faster. This might turn out
   usefull, as MMs can get lot of partial files sooner, and then later
   on can complete the file, from other servers.

   We just use the existing rollback check to take care of preventing
   corrupt files from resuming.

   The gets get initiated via the File Server, same as before. But for the
   partial we add a hidden command, called:
   Client issues:
    -> getpartial <filename>
   This call will only work if client is MM. It will respond correctly.
   For all other clients it returns "Unrecognized command"

   So now comes the part of Partial List propagation. As its to be used
   only for MMs, we propagate it with FFLC3. Internally we can hold the
   list in MyPartialFilesDB. It will only grab files which are at least
   1 MB. This list is updated along with the FilesDB update, or when
   a Download completes partially (File size > 1 MB)
   That takes care of internal list. (and as its different from MyFilesDB,
   it wont be visible in the File Server access.).
   Propagate this info to all FFLCs. The older ones can see the list, but not
   initiate a download.
   It somehow needs to be coded in FileSize*DirName*FileName
   as the Partial file wont have a DirName anyway, we can use it.
   If DirName == "\001", assume its a Partial File. This is what will stop
   older FFLC version to not be able to downloa dthe file, even though
   it will show  up in their listing.
   In FilesDB, we reuse the DownloadState flag. We set it to 'P' for the
   partial.
   Hence on full completion of a partial file, we do not update its Download
   State to 'S', it will remain in 'P' and should not be moved to Serving
   folder.

   Implementation:
   ---------------
   -> Added MyPartialFilesDB in XChange
   -> Added Helper::generateMyPartialFilesDB()
   -> Modified FileServer::fservGet() to handle "getpartial" command.
   -> Modified TransferThr() to not move Partial Files, even if they
      are downloaded in full.
   -> In FFLC transaction, receiving filelist response: If DirName is "\001"
      set its DownloadState to 'P'. All other cases set it it to 'S'.
   -> We do not differntiate in FFLC versions. As DirName will be erroneously
      interpreted by FFLC v2 clients, it will be incentive for them to upgrade.
      So let files be visible for all FFLC versions.
      ************** No need of changing FFLC VERION ***********************
   -> In the File Search TAB, under Directory, if the DownloadState of the
      file is 'P', specify it as "PARTIAL - MASALAMATE" in the DirName
      field. Change in TabBookWindow::appendItemInSearchUI()
   -> when initiating download of these Partials, we need to check if
      DirName in UI is "PARTIAL - MASALAMATE", and apprpriately ignore it.
   -> Call generateMyPartialFilesDB whenever generateMyFilesDB is called,
      wrt initial and periodic updates - includes File -> update Server
      also onSetPartialDir.
   -> GETs of Partial files to issue getpartial <filename>
   May 5th - Have to test downloading a file already downloading in server.

y) When a download starts. It is usually 0 bytes in size, and hence not seen
   in the listing. If it is about 1 mb in an hour or the download breaks,
   it will immediately get listed in the partial folder.
   As its downloading, when we request it, the Downloads TAB is showing its
   old filesize. Shouldnt it reflect the new file size as seen while
   doing the actual DCCServer/DCC ? - investigate - Have to update the
   FD correctly in TransferThr.cpp. The INCOMING part code correctly gets
   the filesize from the DCCServer/DCC transaction and updates the FD for
   downloads. The problem is the send part, we use the FileSize that is
   present in our FD and assume its correct. We should check if the Download
   State is 'P', in which case we update the FileSize, with what is current.
   Change in Helper::dccSend()
   May 6th

z) Delete a File after an attempt is done to download it if its size is 0.
   In TransferThr.cpp, on TRANSFER_DOWNLOAD_FAILURE
   May 6th

a) While Quitting, Partials are saved in config file, but DwnldInProgress
   entries are not. They should also be saved as Partials. It used to
   before. It now doesnt as the DownloadState is 'S' on files being
   downloaded from an MMs serving folder.
   May 6th

b) Change color in Waiting TAB only if Q number is not 0. Moved it to 
   DCCChatClient::analyseGetResponse().
   May 7th

c) Currently getResumePosition() in Utilities.cpp creates the file, if it
   doesnt exist. We shouldnt do it there but rather do it in Transfer.cpp
   The truncating the file if size < FILE_RESUME_GAP is OK in getResumePos
   But do not create the file if it doesnt exist. We will do it solely
   in Transfer.cpp
   May 7th

d) In Helper::dccSend(), remove from the QueuesInProgress, only if its not
   a Manual send. (Removed this change, Dont know why this was in)
   May 7th

e) In Helper::dccSend(), when we try to send at DCCServer Port, if we fail
   for any reason to pass the negotiation stage, try to use the DCC method.
   May 7th

f) In PortCheck, we actually try to establish a DCCServer CHAT at
   DCCSERVER_PORT. Same with PortCheckMe.
   May 7th

g) Manual Sends will be considered as full files if completed. Change in
   TransferThr.cpp, in assuming 'S' if no data found in DwnldWaiting.
   May 7th

h) As the MMs have increased in channel. We need to increase the Trigger
   Delay Time to be proportional to number of MMs in channel as I see it.
   Hence if I have just entered channel, then I will have low MM count,
   and this will make me catch the trigger sooner. If I have been in channel
   a long time, then I will issue the trigger later. Now instead of randomly
   issuing the trigger, we can issue the trigger at 3 seconds * my MM index.
   May 7th

i) A new CTCP which can help in verifying file integrity.
   Example: /ctcp nick FILESHA1 FileSize SHA1 FileName
   It replies if it has the file of that name and at least that size.
   /ctcp nick FILESHA1REPLY FileSize SHA1 FileName
   So before issuing we fill up FileName, FileSize, SHA1, time and send ctcp
   A new ctcp of same can be issued only if that time + timeout > cur_time or
   the placeholders for FileName etc are empty.
   On receiving response, we can match against the place holder.
   So each time, the user has to initiate a cut, and possibly check SHA1
   against another MM client who has the file.

   So process is: right click in file search list, choose File Check.
   We issue ctcp FILESHA1 on that file with that nick.
   If one is already in progress, we inform user and keep quite.
   Response is listed in the Server TAB.
   User can then try with other users to see if his is really corrupt.
   If satisfied, he then can choose Tools->Truncate, which will bring up
   a file dialog, and he can then cut of the last 8K chunk and then go thru
   process again.

   Note that we dont verify against the whole SHA. We calculate SHA of
   the last 8 K bytes from the File Size issued. This saves time.

   Implementation:
   Add in IRCLinetInterpretGlobal.hpp
   IC_CTCPFILESHA1 = "FILESHA1"
   IC_CTCPFILESHA1REPLY = "FILESHA1REPLY"
   IC_CTCPFILESHA1 is of form: \001FILESHA1 <size> <sha1string> <filename>
   IC_CTCPFILESHA1REPLY is also of same form.
   These are handled in DCCThr.
   Add entries for storing SHA1_FileName, SHA1_FileSize, SHA1_SHA1, SHA1_Time
   in XChange.hpp
   Add menu in TabBookWindow, in right click popup in all the SearchUI TABs.
   "Check File Integrity". It should give error message if Nick is not MM nick.
    saying, "This command can only be used on other MasalaMate clients. This
    Nick's client is ... (unknown or not MasalaMate)"
    Added ID_FILESEARCH_CHECK_INTEGRITY, ID_DOWNLOADS_CHECK_INTEGRITY and
    ID_WAITING_CHECK_INTEGRITY, for each of the popups in the respective TABs.
   Add function TabBookWindow::checkFileIntergrity() to handle those IDs.

   On receiving a IC_CTCPFILESHA1, if we dont have that file, we return
   error messages with size and sha1string = 0, and Error message in FileName.
   May 8th

j) While generating checksum for our files for FFLC 2, we should consider
   both MyFilesDB and MyPartialFilesDB. So we add the checksums of MyFilesDB
   and MyPartialFilesDB, to get the total checksum. For this to be matched
   we should generate checksum in same way when comparing against FilesdB.
   Helper::helperFServNicklist(), FilesDetailList::getCheckSumString()
   should calculate in same way.

   More thoughts on how FFLC should decide if it has the same files belonging
   to a nick, between two MM clients doing FFLC. I think its enough if the
   sum of the filesizes match. This is good in the sense that it will be
   independent of how internally the two clients have their FileList. (order
   of files in the FileList change, but sum of filesizes will remain constant)
   This will make it incompatible with previous FFLCs. so the previous versions
   will keep getting full listing with the current version ones.
   Say a guy serves about  1 TB = 1K * 1K * 1K * 1K = 1099511627776
   which is 13 bytes represnted in decimal = FFFFFFFFFF in hex = 10 digits.
   = 5 bytes. 
   On more thinking, if a file changes path we wont catch it by just measuring
   the file size total. Hence we also need a count of the FileName and DirName
   lengths. So sum of file sizes = 8 bytes = 16 hex digits. Sum of string 
   lengths of FileName and DirName = 8 bytes = 16 hex digits.
   If a file is moved from Partial to Serving, we wont catch it, or vice
   versa. So we use another 8 bytes = 16 hex digits for the count of files
   in the Partial Dir (MyPartialFilesDB) <=> DownloadState = 'P' (FilesDB)
   Total Checksum = 49 byte hex string. "FileSizeTotal" "FileDirNameTotal"
   "PartialFileCount" 16 + 16 + 16 = 48 bytes + 1 string terminator = 49
   So we use unsigned long long for adding file sizes and filenames.
   To save on transfer, in generating the CheckSum string, we remove the
   leading 0's of each of the entries. Hence max array = 50, but usually it
   will be about 15 bytes or so.
   Implementation:
   Helper::helperFServNicklist(), FilesDetailList::getCheckSumString()
     Introduced getCheckSums(char *nick, &unsigned long long filesize,
     &unsigned long long *filedirnamelengths,
     &unsigned long long  *countpartial)
     Introduced getCheckSumString(unsigned long long totalfilesize,
     unsigned long long totalfiledirnamelens, char chksum[29]);
   May 8th

k) MasalaMateAppUI should not catch SEL_SIGNAL. We are already taking care
   of the signals in StackTrace. Research on this. Remove it and see.
   May 8th

l) A tool to truncate a file - each time in 8K chunks from the end.
   - Create a Tools in Menu.
   - Under it add - Rollback Truncate. It generates ID_TOOLS_ROLLBACK_TRUNCATE
   - long onToolsRollbackTruncate()
   We pop up the FXFileDialog to choose file to truncate.
   Only allow files to be truncated if they reside in the Partial Folder.
   May 8th

m) Use MoveFile in Helper::moveFile, for windows.
   May 8th

n) Transfer.upload() made to report an appropriate error code,
   just like Transfer.download()
   May 8th

o) If Files with spaces are in the sends and queues. Then on an exit and 
   restart, they are lost.
   May 8th

p) writeFServConfig. How to save the SendsInProgress, DCCSendWaiting,
   QueuesInProgress correctly in the FServ section on a quit or crash.
   For crash saves, we need to update it at points where changes occur.
   Transfer.cpp adds to QueuesInProgress when entry still is in
      SendsInProgress
   FileServer.cpp adds to QueuesInProgress
   TabBookwindow destructor calls writeFServeConfig. Also called when
   Partial/ Serving Dir are changed.
   For crash saves, we can make TimerThr periodicaly call writeFServConfig.
   For exits, we only let TabBookWindow save it.
   Also save DCCSendWaiting ones in writeFServeConfig.
   May 8th

q) To keep it simple, VERSION reply to not give second line which was
   the link to yahoo groups for download.
   May 8th

r) In Transfer::requeueTransfer(), queue in second place.
   May 9th

s) As Downloads now do not remove the ones that are not really downloading,
   we need to update the Title appropriately.
   May 9th

t) Dont allow a file to be Truncated if its in DwnldInProgress with an
   active download.
   May 9th

u) Update Help.cc with some Help text.
   May 9th

v) When we disconnect a download cause of a Rollback mismatch, we should
   send a ctcp NoResend to that nick, and wait 10 seconds, and then return
   in Transfer.cpp
   May 9th

w) A Rollback Errored download is deleted from the Downloads TAB. It shouldnt
   be. Added TRANSFER_DOWNLOAD_ROLLBACK_ERROR in Transfer.hpp. Made it return
   that on that kind of error. So it stays as Partial in the Downloads TAB.
   May 9th
   --- RELEASED --- as May 10th Version
