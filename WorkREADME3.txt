Done:

a) In FFLC the client information is also transferred. If its an MM client
   that information should not be updated in our nick list, as we solely
   rely on the nick having joined CHANNEL_MAIN + MM to be a MM client.
   Change in Helper::helperFServStartEndlist()
   Sep 23

b) In the scenario that a nick is kicked from main, it loses the Client info
   against the nicks in CHANNEL_MAIN. Hence when we join channel MAIN or
   the SYNC channel, we should update MM client information
   In fact we can update the MM client information on an IC_NAMES_END in
   FromServerThr. So, it gets updated for the IC_NAMES_END for all channels.
   Sep 23

c) Allow a Manual Send to a user even if not in CHANNEL_MAIN.
   Change in Helper::dccSend();
   Sep 23

d) Manual send to firewalled user. User existing only in CHANNEL_MAIN,
   example: [IM]-Art0411. Couldnt get a send started to it.
   This is a big in iroffer. We send a DCC SEND ... "filename in quotes" ...
   When iroffer sends the resume it changes " to _, and hence it appears
   to us that he is trying to resume _filename in quotes_ instead.
   Hence we are not able to match the resume.
   So, nothing wrong in sending the file name in quotes. So no change there.
   We further will try to match ignoring the first and last character.
   Change in DCCThr.cpp, just before it sends the DCC RESUME.
   OK, If we have two DCC sends pending, we will again bomb here as we only
   compare with the first in List. Hence, just accept all resumes, if the
   Nick matches.

   Note, that iroffer will erronously name the files we send, prepending
   and appending the filename with _. This is an iroffer bug.
   Sep 23

e) FEATURE: Window Close to Tray. Minimise does only Minimise, as we are
   not able to catch it in Windows. File -> Quit to quit program
   Double click on tray will prompt for password = MasalaMate, and will
   show the window. Password made case insensitive.
   Sep 24

f) Above not good, as they might want to exit and find it hidign in tray bar.
   So added a button which will minimise to tray.
   Sep 25

g) Update IP of Nick in all channels we are joined in. 
   Change IRCNickLists class, 
   such that setNick<IP|Client|Firewall|Client> are set across all
   channels that its in.
   Correspondingly, change getNick<IP|Client|Firewall|Client> such that it
   gets from the first hit as it checks the channels.
   So these functions now dont take channel as the parameter.
   Also, in addNick(), we make sure that if the nick's ip, fwstate, client
   info is already known and update it accordingly.

   This resolves the issue of DCC sending a file to a nick, who is present
   in at least one common channel with us.
   Sep 25

h) Should be able to cancel a manual send.
   Sep 25

i) The disconnect messages have to be named so they are easily understandable.
   CONNECTION_MESSAGE_NONE : is good.
   CONNECTION_MESSAGE_DISCONNECT_REQUEUE_INCRETRY: disconnect and requeue and
     inc its retry count: applicable to cancelling an upload. - We cannot
     cancel an upload, and hence not used at all.
   CONNECTION_MESSAGE_DISCONNECT_NOREQUEUE: disconnect and do not requeue:
     applicable to cancelling an upload when the downloader quits channel.
   CONNECTION_MESSAGE_DISCONNECT_REQUEUE_NOINCRETRY: disconnect and requeue but
     do not inc its retry count: applicable to cancelling an upload cause of
     an imbalance.
   CONNECTION_MESSAGE_DISCONNECT_DOWNLOAD: disconnect the download.
   CONNECTION_MESSAGE_DISCONNECT_FSERV: disconnect the fileserver access.
   So changed to above in FilesDetailList.hpp
   Sep 26

j) Add meaningful #defines to FD->ManualSend entry.
   It currently has three states.
   MANUALSEND_NONE: not a manual send.
   MANUALSEND_DCCSEND: This is a manual DCC Send.
   MANUALSEND_FILEPUSH: This is a manual File Push.
   Sep 26

k) Add meaningful #defines to FD->DownloadState entry.
   It currently has two states.
   DOWNLOADSTATE_NONE: No idea.
   DOWNLOADSTATE_SERVING: File exists in Serving folder.
   DOWNLOADSTATE_PARTIAL: File exists in Partial folder.
   Sep 27

l) Scenario as described during power cut: If file is downloading from
   scratch, an dpower goes off, file is seen as 0 byte, on computer
   restart.
   So, we will flush the stream after every 5 MB is got ?
   DOWNLOAD_FLUSHFILE_SIZE  5242880
   Hence, in Transfer.cpp, when Connection->BytesReceived is an integral
   of DOWNLOAD_FLUSHFILE_SIZE, we issue a fdatasync(fd)/_commit(fd)
   Sep 28

m) Moved Tray to be inside the UI object. It now toggles show/hide on 
   double clicking on the Tray.
   Currently do not know if people want the Tray to be hidden when the
   Ui is visible.
   Sep 28

n) The Trace files should include the release date, so that its easy
   to get the proper crash trace files.
   Added in Compatibility.hpp TRACE_DATE_STRING
   Change in StackTrace.hpp to include TRACE_DATE_STRING in Trace file name.
   Sep 28

o) We should allow /mode commands. Basically just let it go to server
   after removing the / and putting in the channel name in parameter 2
   if necessary.
   Sep 28

p) Allow a /topic in channel to retrieve topic.
   /topic => retrieve.
   Sep 28

q) Allow a /banlist in channel to retrieve the BAN List.
   /banlist => retrieve.
   It Basically issues a MODE #Channel +b
   This returns the banlist, and a End of Ban List message, which we
   need to interpret.
   RPL_BANLIST = 367, and RPL_ENDOFBANLIST is 368
   Introduced: IC_BANLIST, IC_BANLIST_END.
   Oct 3

r) Have CHANNEL_CHAT visible on first startup, rather than CHANNEL_MAIN
   In fact have that in focus as we start up MM.
   Oct 4

s) Added markAsNotFireWalled() in DCCServerThr.cpp. Its used in DCCChatThr()
   and DCCServerThr()
   Oct 6

t) Deleted FD->Data in DwnldWaiting FD structure before exit.
   Oct 8

u) TCPConnect::readLine() improvements. It returns -1 on error. 0 on timeout
   or on receiving only a partial line, which is saved internally. And a
   positive value which is length of line on getting a full line, taking
   care of adding what was partially received in a previous call.
   This will be usefull in SwarmThr, as we do a readLine() with timeout 0,
   for all connections, to process the lines received.
   - This will be tested while swarming.
   Oct 11

v) Bug in TCPConnect:readLine(), when timeout is called with 0.
   Oct 15

w) Swarm Streaming. First we need to integrate it in MM in such a way that
   it doesnt disturb the existing clients. We will first have it for
   streaming, a file, which we can use to test how good it is with many
   users.
   So, for this the interface will be as such. We start a stream command:
   /swarmstart pathtofile. This is in the server. We have a command 
   in the client called /swarm ServerNick FileName. This instructs 
   the client that the server is ServerNick, and the File is FileName.
   We get ServerNick, by issuing an @stream command. The Server who has
   issued /streamstart will respond in NOTICE, and hence we will know
   ServerNick and FileName.
   Response: [Swarm] FileSize: ... FileName: 
   We can then start the swarm connectivity process starting with the server.
   (Includes handshake and CL exchanges etc). It will just connect and idle.
   We have it this way so we can have control on when to start the streaming
   process and study its behavior. Now with Streaming audio or video files,
   we should possibly have a logical time unit instead of FileSize to track.
   But we will think of that later. Currently we just try to start a swarm
   for a file and see how that goes.

   New Files/Classes created to handle Swarm. New functions added to Helper.
   SwarmThr, SwarmStream, SwarmNodelList, SwarmDataPieceList.

   We should show the SwarmStream structure in a Swarm TAB with Folding Lists.
   Top of Folding List could be:
   FileName, MaxFileSize, TotalSpeedUp, TotalSpeedDown, Progress

   On unfolding it can have below:
   Nick, FileSize, SpeedUp, SpeedDown, State, FileOffset

   FileName/Nick - FileSize - Speed Up - Speed Down - Progress/State -
     File Offset

   NOTE: On thinking it seems right to send DR requests to nodes whose file
   sizes are closest to the FileOffset that we are requesting.

   Testing so far:
   - Start Swarm Server on one MM: /swarmstart
     Check if Swarm UI Tab is Ok.
     Exit and check memory leak
   - In Another MM client.
     - @swarm <file> and see if you get result.
     - Use result above to join swarm. /swarm Nick FileName
     - Check if Swarm UI Tab is OK.
   - Check if the Handshake is successful.
   - Check Swarm UI Tab is OK on both.

   All of above with combination:
   - /swarmstart on NF, /swarm on NF (pass)
   - /swarmstart on FW, /swarm on NF (pass)
   - /swarmstart on NF, /swarm on FW (pass)

   A Right click menu in Swarm Tab. Used to Quit a Swarm.
   It will disconnect itself from the Swarm.
   SwarmStream Class has a quitSwarm() function, which disconnects
   itself from the swarm which is swarming FileName.
   - Join Swarm and Disconnect by clicking from UI (pass)

   While trying nodes, avoid connecting to ourselves - so add that check.
   Calls to YetToTryNodes.addToSwarmNodeNickIPState() need to be checked.
   - as of now just in TabBookWindow (pass)

   Get the (Node List) NL exchange message in, right after the HS exchange.
   So once HS is successful, both sides call 
   Helper::nicklistReadWriteSwarmConnection(SwarmIndex, SwarmNick, Connection)
   This function will write out the NLs (end with "NL ND\n"), and then start
   reading and adding to ToBeTried till "NL ND\n", is reached.
   Adding should be done, if it doesnt exist in Connected or TriedButFailed,
   and its not our own Nick/Ip.
   The NL list sent is from Connected and ToBeTried.
   NL Nick HexIP Nick HexIP ... \n
   NL Nick ... \n
   NL ND\n
   As they prepare list first, before sending, or trying to read what the
   others NL is, we wont exchange what we recieve too. (reduces some bytes)
   - Done.

   Simplification of HS Message handling.
   For a DCCServer connect, we write "140 Nick FileName\n". This will
   enable the DCCServer guy to get to know SwarmIndex. if SwarmIndex
   comes out to be -1, we close with "AC NO\n"

   For a DCC Swarm Connect, FileName is sent and hence, the guy initiating
   connect knows SwarmIndex, and the guy waiting for the connect has the
   info in SwarmWaiting FD.

   For this simplified HS, a successful ack will be just: "AC YS". Cause,
   each receives the others HS line, and hence knows the others FileSize.
   - Done.

   #define SWARM_MAX_CONNECTED_NODES 50 (in ThreadMain.hpp)
   Possibly can experiment and check for some value which suites us.
   --------------------------------------------------------------------
   Note - possibly in the future, when we exchange NL list, we should
   exchange SWARM_MAX_CONNECTED_NODES at max picked at random, to form
   a random list.
   --------------------------------------------------------------------

   DR messages implemented.
   - DR RequestedOffset RequestedLength MyFileSize MyFutureHoles MyFileName
   Response can be an Error:
   - ER HisFileSize HisFileHoles HisFileName
   Or a DP => data piece on the way.
   - DP FileOffset DataLength HisFileSize HisFutureHoles HisFileName

   Now for the End Game to get the possibly Last Piece which is < 8K
   That is done.

   Now to incorporate the FS exchange between equals.
   That is done.
   Oct 16

x) It should heed to the XGlobal BW CAPs.
   Move to Serving folder once swarm is done to completion, and we quit 
   the swarm.
   If one of the Swarm Nodes Errors or Disconnects we should remove it
   from the ConnectedNodes list.
   Sync data of Swarm File, each time we get about DOWNLOAD_FLUSHFILE_SIZE
   bytes, and once when download is completely done.
   When Quitting client, we should also call Swarm.quitSwarm(), so it cleans
   up => move files to serving etc if fully downloaded.
   Oct 16

y) We generate the FileSHA at swarm start. This is good when we are just
   just starting up, but once our FileSize starts increasing, we were not
   updating the FileSHA with it. So now we just invalidate the FileSHA
   as soon as FileSize changes. For generating the HS string, we regenerate
   the FileSHA if its a NULL string.
   Oct 18

z) We should check on errors on all connections and terminate the ones
   which have their error set.
   Note that we generate our FutureHoles with '1' and '2' in every call
   and hence dont have to worry about resetting the '1' to '0' for the 
   disconnected nodes to which DR has been sent.
   Oct 18

a) Try attempting new connections once in 2 seconds. Also, if IP is unknown
   and we issue a USERHOST, try again after 20 seconds.
   Oct 18

b) On a /swarm, first check if file exists in Serving folder. If it exists,
   we just open and use that file. It could be an incomplete file, or we are
   just joining to seed.
   Oct 19

c) On a @swarm, if 2nd word doesnt exist, respond to all swarms that you are
   the seeder. If 2nd word exists, then respond only if substring matches
   second word.
   Oct 19

d) In TCPConnect.readLine(), if select returns that socket has something
   to read, but recv() returns 0 => other end has closed socket. In this case
   we should return -1, and discard partial line, as its of no use.
   Oct 19

e) We are supposed to maintain the SwarmNodes in ascending order of FileSize.
   Currently we keep them in order only during the initial phase. Basically
   in SwarmNodeList::addToSwarmNode().
   As FS information is updated, we should rearrange their position in 
   the List dynamically. Else, we lose the order that we intend to maintain.
   So each time we get an FS from a SwarmNode, we should rearrange its
   position in the List. FS is received in readMessage..., DataPiece is
   selected in writeMessage.... Hence we should just have a call to 
   SwarmNodeList::orderByFileSize(), at the end of a readMessage...
   To check if this works, just see if the UI is giving list in ascending
   order of their FileSize.
   Oct 19

f) Added CTCP_LAG for the LAG CTCP send in IRCsuper.
   Oct 20

g) We are not able to clearly identify when a client dies on the other end.
   This is crucial as all nodes that are alive are present in ConnectedNodes.
   And if we have sent a DR request to that node, we mark it in our Future
   Holes as a '1', and dont request from an another node. In this case, when
   the node has indeed died, that hole will exist as '1' forever.
   Hence we use the Connection->getLastContactTime(), and if we havent had
   contact in SWARM_KEEPALIVE_TIME, then we send it a FS line, just as a
   keepalive message, only if its in state = SWARM_NODE_EXPECTING_DATAPIECE
   or SWARM_NODE_DR_SENT. So, this is done in SwarmNodeList.writeMess...
   Oct 20

h) Made a write() for the actual datapiece = 8K to be done with timeout of 0.
   So, that it doesnt wait for the write to complete for a slower node,
   impacting other nodes.
   Oct 21

i) Crash while choosing font as Comic Sans. I guess its cause font is becoming
   NULL.
   Oct 22

j) In Linux, we should open same file if found ignoring case, in the
   Partial directory. Example, we have Abc.avi, and if we receive
   a file called abc.avi, we resume Abc.avi.
   This has to be corrected in two places. 
   First, is on a DCCServer 120 receive, the FileName we fill up should 
     be checked against MyPartialFilesDB.
   Second, is on receiving a DCC SEND in DCCThr, when we populate
     DCCAcceptWaiting.
   Oct 22

k) On TAB, we should rotate through other nicks which match original
   partial nick, like mIRC.
   Each window has an entry called, LastTabCompletedNick, LastTabPartialNick.
   TabBookWindow::onExpandTextEntry() uses the above to get the next Nick
   which matches the partial entry.
   In TabBookWindow.onTextEntry() we initialize LastTabCompletedNick and
   LastTabPartialNick to null string.
   Oct 22

l) Optimise FutureHoleString exchanged.
   FutureHoles now holds the hole information in bits.
   Each Byte the bits are as follows:
   bit 0, 1 = always set.
   bit 2, 3 = node state of hole 0
   bit 4, 5 = node state of hole 1
   bit 6, 7 = node state of hole 2
   Node state = 11 => hole occupied with bytes.
   Node state = 10 => request sent to fill hole.
   Node state = 00 => hole empty.
   Oct 23

m) Forgot to free and initialize DataPieces in the SwarmNodeList strcuture
   when we quit Swarm. So, it had previous DPs left over from the previous
   swarm.
   Oct 23

n) We do not enlarge buffer for SWARM connections, hence we call
   Connection->setSetOptions(false);
   Oct 23

o) In the case when an FS is sent to people with <= FileSizes and state
   SWARM_NODE_SEND_FS_EQUAL. We make sure we send max once a second.
   So we have a field in SwarmNode called TimeFileSizeSent where we note
   when we last sent a FS message.
   Oct 23

p) The Swarm should remember the greatest FileSize recorded, so that it 
   shows the correct Progress %, in the UI. So we have a field in SwarmStream
   called MaxKnownFileSize where we keep track of this.
   Oct 23

q) Remove FileName from being exchanged all the time -> its irrelevant.
   HandShake is the only place where we use FileName etc, and that is taken
   care of in Helper.cpp.
   So all messages after handshake should not have FileName tagged along.
   Oct 23

r) If I join a swarm, first open the file if present in Serving folder.
   next try the Partial is failing.
   It was already doing that but if in same session, when we close the swarm,
   and say the file was complete, it moves to serving folder, but MyFilesDB
   wasnt updated, and hence it didnt find file there.
   Oct 24

s) paandu present in 2 connections in UpgradeMM. One with DP-Sent and the
   IP with which he Read errored. In server we should be able to detect
   when a line is errored but just hanging. For DR-Sent or DP-pending
   nodes we detect breakage by sending an FS. Looks like we should do
   so for other node states as well after some minutes of inactivity.
   So, we should send some kind of alive check on all states, so that the
   dead ones are weeded out.
   Send the FS as an alive check. Now, do not send it if state is
   SWARM_NODE_DP_SENDING, else it will garbage out the piece we are sending.
   Oct 25

t) Bug scenario -> Say I have received a partial piece and its added to
   DataPieces. Now the node that I am to receive the rest of the piece
   from, dies. In this case I will for ever have a partial DataPiece
   hanging. Need to clear it out, so it can be requested afresh.
   Such a node will be in state: SWARM_NODE_EXPECTING_DATAPIECE.
   We can, remove its DP entry in removeDisconnectedNodes().
   Oct 25

u) Move to FOX Stable 1.4.20
   Oct 25
   
v) Download/Upload of Swarm should somehow come up in the FServ Ad. Also
   statistics of Bytes Sent/Received, Speed Up/Down, Record Speed Up/Down
   should be recorded for swarm as well. We add those values to the already
   existing fields of FServ Ad. 
   We add an additional field before "Firewalled" called - Swarms:[%d] 
   Speed Up/Down includes Swarm speed
   Swarm should update the record up/down as well - in progress.
    For a download that has finished we save it in 
      SwarmNodeList::DownloadCompletedSpeed, which can then be retrieved by 
      SwarmThr
    For upload - dont know what to do !
 
   Swarm should update the bytes sent/received as well - in progress
     On every succesfull DP send and receive, we should keep incrementing
     SwarmNodeList::BytesSend, SwarmNodeList::BytesReceived, which can then
     be retrieved by SwarmThr.
   Oct 26

w) Add a link to the demos. Help->Demos. This should open the browser
   to the Demos page. As of now point to 
     http://www.masalaboard.com/masalairc/masalamate.php
   In Linux, we spawn firefox, if failure, then spawn mozilla.
   Oct 26

x) Deny a GET/GETPARTIAL in FileServer if Swarm exists for same file.
   So send a message saying, please issue: /swarm ...
   Nov 6

y) As of now only the person who did /swarmstart responds to the @swarm
   command. So when he is gone, though swarm exists, no one responds. We
   should make all the nicks who have the highest filesize respond
   to @swarm.
   Nov 6

z) Allow only one connection from 1 ip in the release build. For testing
   we will allow multiple as I can test the code when many parallel
   connections are in progress.
   Nov 6

a) Iroffer bots, returning notices wrt q position was not correctly handled.
   Adde LineParse::getIndexOfWordInLine().
   Nov 12

b) The <@Nick>, <%Nick> in lines, to have different colors. 
   This is to highlight these nicks from the regulars and voices.
   <@Nick> to appear in Red, and <%Nick> to appear in Yellow.
   Nov 12

c) Changes to allow compilation in 64 bit.
   Nov 13
 
d) While reading Data (not lines) using readData() in Swarm, use time 
   out of 0, so that it returns immediately.
   Nov 16

e) When receiving a file in DCCServer, check if the file is already in
   swarm. If so reject it. Same with a DCC SEND.
   Changes is DCCServerThr.cpp and DCCThr.cpp
   Nov 16

f) Release Nov 17th version.
   cvs tag Nov17-2005
   RELEASE
   Nov 17

g) Crash in TabBookWindow::restoreSelections().
   Change a few conditions, and added more TRACE()
   Nov 20

h) Bug in Helper::handshakeWriteReadSwarmConnection(), in the case when
   sender sends "AC YS" as he has less filesize, but we send him an "AC NO"
   as the SHA doesnt match. We were falsely setting track_node_state to
   SWARM_NODE_HS_SUCCESS in this case. The other client receives AC NO,
   and hence disconnects. But we, receive AC YS, and so in the end
   mark state as SWARM_NODE_HS_SUCCESS, which was wrong.
   Nov 20

i) When initiating a Swarm Connection, on connecting to the DCCServer and
   negotiating the swarm protocol, we were equating a boolean retvalb to
   writeData(). Hence, connections which connected to DCCServer port on other
   side, but failed on a write, were not attempting to send out the DCC
   SWARM message, so that the other side can try to connect to us.
   OK, got the DCC SWARM going, but still other side is not trying to
   connect -> Test this scenario: 8124 listening by some other program,
   and start swarm in this scenario ...
   Test setup:
   - suriyan.is-a-geek.org Windows - Nick = UpgradeMM
     - Start a program which listens on 8124, and disconnects the connection
     - as soon as a connection is got - Modified TCPConnectTest.cpp
     - /swarmstart
   - T30Linux Linux - Nick = KhelBaccha
     - /swarm UpgradeMM
   Problem is if connection succeeds, the write always succeeds, and hence
   it later fails in the handshake code, and hence we dont issue a DCC SWARM.
   Hence, now, if handshake fails, just try the DCC SWARM anyway.
   So with this in place, on first failure the node will make its entry
   in the TriedAndFailedNodes list. So, ignore them as of now.
   Nov 21

j) Add ->
    This software uses the FOX Toolkit Library (http://www.fox-toolkit.org).
   as mentioned in Fox ToolKit LGPL addendum, which needs to be in Help->About
   Nov 23

k) Have the tray password settable by user. Once set, it needs to be entered.
   Set via, Tools->set Tray Password. Also, make the password entry box, say
   *** when password entered. This password needs to be put in the cfg
   file, under [IRC] -> TrayPassword.
   Nov 23

l) When we issue a /swarm nick file in Linux, we should spot the file in
   Serving/Partial case insensitive.
   Nov 25

m) When swarm is discoed out cause of seek/write failures, we get a blank
   in Swarm TAB which gives no clue as to what happened. We should put 
   a nice message in UI.
   Nov 26

n) Crash when SHA1 functions called in 64 bit build. Upgraded to CSHA1 latest
   version from net. That version too has a bug in which ULONG_MAX is
   not defined (as limits.h is not included in SHA1.hpp). That is corrected.
   Also, remember to remove space when printing SHA in the ReportHash()
   routines.
   Nov 28
   
o) Issuing /join #IndianMasalaMM crashes client.
   Nov 29

p) On getting any kind of CPU load (especially after the swarm now uses
   some CPU - which should be gone in future releases), the up and down
   semaphore dance and adding a timeout in UI etc, we somehow endup losing
   sync. Meaning, some lines exist in the Queue, but we are not adding
   timeout.
   Hence right now calling the TabBookWindow::updateText() directly, by
   just acquiring the AppMutex. The original code is still available under
   #ifdef UI_SEM
   Now, in Windows port if I use the lock/unlock it freezes after some time.
   I think its a FOX bug. In Linux it works perfectly well as mutexes are
   used. In windows port Enter/LeaveCriticalSection() are used. Somehow
   hangs after the initial connect. So as of now, no lock/unlock for Windows.
   So it can lead to potential problems... but lets see.
   As of now the UI seems very responsive with no lag on load.
   Dec 1

q) File Search searching to be made more flexible. One approach is to
   replace all the buttons and tick boxes to a query based one. Example:
   List such that: 
     FileName contains [text]                   (No text => matches any)
     DirName contains [text]                    (No text => matches any)
     NickName contains [text]                   (No text => matches any)
     FileSize is greater than [integer MB]      (0 => Any)
     FileSize is less than [integer MB]         (0 => Any)
     Sends [OPEN|ANY]
     Queues [OPEN|ANY]
     Firewalled [NO|YES|ANY]
     Partial [NO|YES|ANY]
     Client [MM|SYSRESET|IROFFER|ANY]

   If we take above approach, we need to have 5 InputUIs. Now there is only
   one InputUI.
   So we might have to add 4 more called as  FileSearchNickNameInputUI, 
   FileSearchDirNameInputUI, FileSearchFileSizeGTInputUI,
   FileSearchFileSizeLTInputUI and use the InputUI to stand for
   FileSearchFileNameInputUI
   Lets code it under #ifndef OLD_FILESEARCH, so we know how to retract.
   Now the main function which does the search is onTextEntryFileSearch();
   This functions should read in all the values from the UI, and filter
   the actual search. So for initial testing we write a new function called
   doFileSearch(). This gets called when clicking the Search button.
   Dec 2

r) More cleanup and segregation of #ifndef OLD_FILESEARCH.
   Assume FileSize entries are in MB.
   Dec 3

s) TabBookWindow::onTextEntry(), is a big ass funciton with 'i' being used
   as window index. Can create a problem later on, hence changed it to 
   use variable 'windex' instead.
   Dec 3

t) Toolbar->clear is not clearing the list in File Search tab.
   Dec 3

u) Right click -> List files of nick, should clear the FileName Input UI.
   Right click -> Search in Files, should clear the NickName Input UI, 
   before going and calling doFileSearch().
   Both should clear the DirName InputUI, FileSizes InputUIs.
   Dec 3

v) Replace "Search" with just icon with ToolTip in TAB_FILESEARCH
   Dec 3

w) So suggestion is to make two rows for the FileSearch criteria entries.
   To reduce clutter. Also instead of 'Size >' use 'Size atleast'. Instead
   of 'Size <' use 'Size atmost', and add one more ComboBox drop down
   next to Size, which allows user to select B/KB/MB/GB.
   Dec 3

x) Removed code which was for OLD_FILESEARCH.
   Make changes to functions which can be used for FileSearch and other
   TABs as well. Example: makeNewFileSearchBottomFrameInput(),
   makeNewFileSearchBottomFrameLabel() and others.
   So normalised all of them to be makeNewBottomFrameInput() etc ...
   Dec 4

y) One more FileSearch related improvement. Add "File Type" next to File
   Name, as a drop down combo box. It should have the following selections:
   mpg/avi/wmv/rm/3gp/images.
   We should handle mpg as .mpg/.mpeg/.dat/...
   images as jpg/jpeg/png/tiff/bmp/...
   Dec 4
   

z) On private messaging a nick which requires that I be registered, I get
   a message saying "You must identify to a registered nick to private 
   message private message". Somethign wrong.
   So it seems that is what we receive from the server:
   :Paranoia.mo.us.ircsuper.net 486 deDeofdEU :You must identify to a 
     registered nick to private message private message
   So, not MM bug, but ircd bug.
   Dec 6

a) Change the label colors of labels that get created in the bottom frame.
   They were same color as the TAB labels (blue), and possibly cause some
   confusion. Now, changed those to be black. Will do for the moment.
   Dec 7

b) Macintosh PPC port. - Done on the Mac Mini. I installed xcode 2.2 first -
   which has the gcc, ld, cvs, make etc.
   I see that we have __APPLE__ and __MACH__ defined in this port.
   So we will use #if defined (__APPLE) && defined(__MACH__) for Macintosh
   specific code.
   Introduced #define USE_NAMED_SEMPAHORE, as Apple does not support nameless
   semaphores.
   Dec 8
   
c) When Ctrl K is pressed, put \003 in the text input, which is the code
   for color. Change in TabBookWindow::onExpandTextEntry();
   Dec 14

d) Bug Fix in Ctrl K, for cursor positioning.
   Dec 16

e) EndianNess mess up. included <endian.h>, and use if __BYTE_ORDER == 
   __LITTLE_ENDIAN or __BIG_ENDIAN. All code have them in different ifdefs,
    as we will catch it as a compilation error if they dont get defined.
   Dec 18

f) Some more fix in Ctrl K. Sometimes it would interpret it as Ctrl K even
   when it wasnt.
   Dec 25

g) In DCC Chat, when we dont get any text on timeout it prints an empty
   line in the DCC Chat window.
   Dec 25

h) Have to get the name of the UPGRADE files for different OSes and bitmodes,
   in a standardized non confusing way. A good way if file name contains
   OS-Processor-BitMode in it.
   Example: 32 bit windows on x86: MasalaMate.WIN_X86_32
            64 bit windows on x85: MasalaMate.WIN_X86_64
            32 bit Linux on x86:   MasalaMate.LINUX_X86_32
            64 bit Linux on x86:   MasalaMate.LINUX_X86_64
            32 bit Apple on ppc32: MasalaMate.APPLE_PPC_32
            64 bit Apple on ppc64: MasalaMate.APPLE_PPC_64
            32 bit Apple on x86:   MasalaMate.APPLE_X86_32
            64 bit Apple on x86:   MasalaMate.APPLE_X86_64
            32 bit Linux on ppc32: MasalaMate.LINUX_PPC_32
            64 bit Linux on ppc64: MasalaMate.LINUX_PPC_64
   So a client who wants to upgrade sends an upgrade message with file name
   specific to his port. Example MasalaMate.WIN_X86_32, which is same
   as UPGRADE_PROGRAM_NAME. 
   For Reference.
   Upgrade message: /ctcp UpgradeMM UPGRADE sha ip UPGRADE_PROGRAM_NAME
   Dec 25

i) Moving endianness definition to Compatibility.hpp as its trying to be
   defined in many places.
   Dec 25

In Progress:
============

w) Swarm algo improvements.
   - Scenarios in which a slow node can affect the overall speed of the swarm.
   The slow node will be found at the first index of the hole, waiting for a
   DP. If we find all future holes occupied, and a node free to which a
   request could be sent, then we need to mark the slow node as slow, and stop
   requesting from it in future, and also mark the data that will be received
   from that node for the current request to be discarded on receive.

   - Another issue is when a node is actually dead, but we cannot detect, as
   the write will succeed. This scenario will effect the download, only if
   a data piece is being requested from this node, in which case it will get
   caught as a slow node sooner or later and marked as such.

x) When we get disconnected and rejoin, or when we quit and restart, we should
   rejoin all swarms we are participants of.
   We quit to be a participant, only when we Quit that Swarm.

y) We still havent tested the case when two clients have almost same filesize,
   but their filesizes are not in exact difference of SWARM_DATA_PIECE. And
   hence they exchange bytes across buckets. 

z) How about have two threads per swarm. One thread to Read, the other to
   Write. That way we can make it wait for input, and if any input we receive
   which might warrant a write, we wake up the write thread. Think on this.
   This can help in regulating CPU used, as we wont loop trying to do work.

s) Khair again hit the jumping Q syndrome. Where he got queued at position 4
   right after the nicks not in channel position and before other voiced
   users.
   - Open issue, Cant seem to find anything wrong.

t) In Windows 98 SE (telugustud_2005), he crashes in FromServerThr, on a
   IC_QUIT. The last function call on stack is strcasestr(). Am wondering
   if quit_mesg is set to NULL, and sprintf on Win 98 SE causes a crash.
   - Research sprintf behaviour in Win 98. - Ok sprintf is fine.
   - Running debug on Win 98 SE.

y) Recognize generic iroffer bots too. This will be needed as we can now
   join other channels.

z) There does seem to be a crash in UI when saveSelections() and then
   restoreSelections() are called. Added more TRACE()
   An example sequence as obtained from a trace file->
   onUIUpdate()
   updateFXText()
   getWindowIndex()
   displayColorfulText()
   onCmdPanel()
   updateWaiting()
   getWindowIndex()
   saveSelections()
   restoreSelections()
   - Debugging.

x) Disappearing Queues which are going to be converted to Sends, when a
   FileServer access is done at almost same time ?

n) Code cleanup. XChange->get... does a lock of Mutex, and XChange->reset...
   does an unlock. So we should just have a generic ->lockGlobal, and
   ->unlockGlobal so its clean and understandable.
   in fact they can use the same XGlobal->lock() and unlock() to access
   these values.
  
n) Code cleanup. Move repeated code in DCCThr/DCCServerThr for chat/fserv 
   access into Helper.

a) Add an option to ignore user. Additional option to have its join and part
   also ignored.

a) Put in SSL support. (Will have to add a menu to enable it)

b) Extract multiple triggers in sysreset FServ. So change FServParse.cpp
   to return number of triggers and their respective trigger names.


May not Implement list:
-----------------------
w) NOTE: A case when a requeue doesnt happen.
   A DCC send is sent to Chimero, and we wait for a connection to come to us
   , but it never does, and after a timeout that FD entry is cleared by a 
   purge after the set TimeOut time. (DCCAcceptWaiting Queue)
   So, in this case we dont do anything.
   This actually is a BUG.

y) Mirc like feature:
   click masala-chat tab. and click masala-chat tab again, you go back to 
   the previous tab you came from
   (Am not getting message when mouse clicked on active panel)
   - May not implement.


Bugs:
----
a) Sometimes we auto issue the trigger of someone, and it attempt to connect
   after the 3 minute timeout, and hence lands up in our CHAT. 
   Nothing can be done here.

b) On right click release, it doesnt highlight the word under cursor in
   the nick list ui, while popping up the menu.
   The FOX GUI is not reporting the correct position of cursor.

c) With time the text displayed in channels is lagged. I found that it
   was lagged in #IM but not in #MC. Which makes me believe its not the
   fault of the Queueing mechanism (threads/mutex/sema). It seems to 
   be possibly FOX toolkit related. If I issue a /clear in #IM, it behaves
   OK henceforth.
   Increased LINEQUEUE_MAXLINES from 100 to 1000. (affects Win port)
   in Jan 25th version.
   This might fix the issue.

d) We do get occasional memory corruption exits.
   UI.cpp:25 (UI object is created)
   TabBookWindow.cpp:564 (end of destructor of TabBookWindow)
   Dont delete UI, let the App take care of that.
   This might fix the issue.

e) In Feb 17th version. TCPConnect.cpp readLine() gets a SIGSEGV.
   Found a memory corruption happening with DotIp -  March 30, section q
   It might be cause of that.
