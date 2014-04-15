Done:
=====

a) In Linux as a failed connect can take 3 or more minutes to fail, we
   need to use alarm() to work around it. - check Iroffer code.
   issues a alarm(TCPCONNECT_FAIL_TIMEOUT), call connect(), and reset alarm
   by alarm(0)
   This should be handled right in the TCPConnect class.
   SIGALRM to be caught before issuing a connect().
   This above method is wrong. SIGALRMs are not stackable, as a result
   when a couple of threads issues the connect(), the last one is the only
   one valid. Hence we shouldnt be using this alarm() method for multi
   threaded connect() calls. (unless we can stack them)
   For Linux, set /proc/sys/net/ipv4/tcp_syn_retries to 1 from 5.
   That will solve the issue. Or for a more generic solution, make the
   socket non blocking, and use select to figure out if we are connected.
   So now revert back FileServerWaiting timeout to 3 minutes.
   May 13th

b) In DCC Send only use DCC method if DCCServer doesnt return 151 in
   previous method. Change in Helper::dccSend()
   May 14th

c) For a Send if server rejects transfer if receiver requests a resume
   which is greater than the file length that server has, server should 
   NOTICE receiver appropriately.
   Analysis:
   Server side all originates in Helper::dccSend()
   Tracing the DCCServer transaction.
   Server sends -> 120 nick filesize filename
   Clients DCCServer catches this, and should reject if server filesize
   is <= what client has. (ie 151, send Notice, and info in UI). Change in
   DCCServerThr.cpp - done.
   Tracing the DCC transaction.
   Server sends -> DCC SEND filename filesize etc.
   Client receives it, and should reject is server filesize is <= what
   client has. (ie do not respond, but send Notice, and info in UI). Change
   in DCCThr.cpp - done.
   May 14th

d) Auto Update feature.
   If an MM client exists in im and mc, and is op in both the channels,  
   then he can be assumed to run the latest MM. So for such MMs, we have a
   menu item called File->Upgrade Enable. Its tickable. Default starts up 
   unticked. On ticking it we help upgrade. so that is from the server side. 
   This Check item should come up only if we are OP in both channels.
   The Enable will work if the directory Upgrade exists, containing
   MasalaMate.OS (OS = WIN32X86 or LINUX32X86SLES9 as of now) - done.

   How do clients interact ? Note that the client could be already banned
   in main. Hence wont know who the other MMs are.
   One way is user initiated, Tools -> Check and Upgrade. The other is
   triggered on being kicked in main.
   The process is same.

   Two new ICs, IC_CTCPUPGRADE = ctcp, IC_CTCPUPGRADEREPLY = its reply =
   NOTICE. - done.
   Client sends /ctcp UpgradeNick UPGRADE SHA FileNameto all ops in chat.
   OS currently can be WIN32X86 or LINUX32X86SLES9 - done.
   Trigger on being kicked in main channel. - added one more message to be 
   handled by UI, called *UPGRADE*. On receiving it we initiate an upgrade
   process. - done

   The Server on getting that ctcp replies to the CTCP only if that SHA
   for that OS is different and hence needs an upgrade. The reply is in the
   form of a NOTICE. It will have the SHA of the file that will be sent.
   - done.

   All these requests are put in a seperate Q called the Upgrade Q in server.
   They are picked up every 3 minutes in TimerThr, and a dccSend initiated.
   Server, then,  dcc sends the MasalaMate.OS file to requestor - done.

   The client on receiving the MasalaMate.OS, checks if sender is OP in chat,
   is same nick as noted, same SHA as noted. same IP as noted, and within 
   Upgrade_Time.
   If so, it moves over currently running binary as .bak. Then moves the
   got one in its place. - done.
   May 15th

e) Add a Tools->Request UNBAN. It should attempt an unban, and then
   try to join all Channels.
   May 15th

f) In "Waitings" TAB, if Q number is 0, show it as UNKNOWN.
   May 15th

g) When we issue a Update Queue information on an Iroffer bot entry, present
   in the Waiting TAB: If its already in Q, it sends a message as below:
   *** All Slots Full, Denied, You already have that item queued.
   In FromServerThr, we just catch the Denied, part and remove it from
   DwnldWaiting Q. Hence it disappears. Need to correct it.
   May 15th

h) BUG: In DwnldInitThr. Get/Cancel/GetPartial are not initialised every loop.
        This is Major. Hence erratic behavior.
   May 15th

i) BUG: Requesting a download. quitting MM -> saved in config file. Restart
   MM. Entries added with partial truth (Trigger type etc are wrong). Now when
   we try to rerequest or cancel them, we should remove old entry, and populate
   with entry form FilesDB as it will have rest of pertinent information.
   Done in DwnldInitThr().
   May 15th

j) In MyFilesDB and MyPartialFilesDB we convert all path seperators to 
   FS_DIR_SEP. That helps in having same interface in the File Server.
   Also helps in FFLC, as we transfer same FS_DIR_SEP.
   Now, when actually sending a file, where we actually need to open the file
   and send, we need to convert the FileName and DirNames to DIR_SEP, so
   it can actally open. Add in Utilities.cpp -> 
   convertToFSDIRSEP(char *), will convert inplace, DIR_SEP to FS_DIR_SEP.
   convertToDIRSEP(char *), will convert inplace, FS_DIR_SEP to DIR_SEP.
   Convert them in File Generation, and Convert back in dccSend
   Correct File Size for all files in dccSend()
   May 16

k) A Queued File, when pushed, is made to take the state of ManualSend = 'M'
   and a dcc send also sets ManualSend = 'M'. Both have differnt ways to arrive
   at full file path. Hence a conflict. So, a DCC Send should be differentiated
   from a Pushed Send. Pushed Sends should take state 'M' in ManualSend.
   DCC Sends should take state 'D' in ManualSend. TimerThr, should push out
   Queues at index 1, which have ManualSend 'D' or 'M'.
   Path corrected properly, in dccSend and DCCServerThr 0 generating full
   path considering ManualSend, DownloadState.
   May 16

l) Because of above, Make an UPGRADE send, ManualSend = 'D', DownloadState =
   'S'.
   May 17

m) Put the fully qualified file name generation, given an FD in a Helper
   function, so it can be used by workaround send and DCC Send.
   The FS_DIR_SEP char is converted to native DIR_SEP in Helper::dccSend().
   which then resides in DCCSendWaiting, if failed to do a workaround send.
   May 17

n) Quit messages if they have MasalaMate in them, put them on. Use the "To"
   field to contain the Quit Message.
   May 17

o) After sorting, highlight the same line that was selected before the sort.
   Change in onSearchHeaderClick(). so getCurrentItem() before search, and
   identify its FD pointer. And when populating list with search, on hitting
   that FD, setCurrentItem() and selectItem() and makeItemVisible()
   May 17

p) Unban should just print a UI message if already present in MAIN, instead
   of issuing a blind !unban.
   May 17

q) Obsolete FFLC version 1. Can make code cleaner.
   May 17

r) FFLC Version 3. No other versions supported.
   For FFLC, as partial files are very dynamic, they should be seperately
   exchanged. Currently all Serving and Partial are exchanged together.
   so possibly have a seperate transaction for Partial files.
   In hash generation, I think we shouldnt take into consideration file 
   sizes for FFLC. All we care about is file list in Serving/Partial and 
   their approximate sizes.
   So for checksum, this is what is considered: 
     length of FileName, length of DirName, Total # of Files
     fileinfos exchanges only Serving files from MyFilesDB
     fileinfop exhanges only Partial files from MyPartialFilesDB.
   Change in FilesDetailList::getCheckSums, getCheckSumString - done.
             Helper::helperFServNicklist - done
             Helper::helperFServStartEndlist - done
             Helper::helperFServFilelist - done
   Add FilesDetailList::delFilesOfNickByDownloadState(nick, dstate); - done
   Add FilesDetailList::getFilesOfNickByDownloadState(nick, dstate) - done
   FileServer to respond to filelists and filelistp - done
   May 17

s) When uploading a file we are downloading. Transfer.upload() keeps uploading
   till it cant read any more from the open file. It doesnt stop when the
   initial negotiated filesize is reached. Because of this the percentages
   and time left information on both the server and client are messed up
   as they go by initial negotiated file size. It is good that it goes on
   as much as it can. Hence we only fix the UI part.
   Change in TabBookWindow::updateDownloads(), updateFileServer()
   May 18

t) Add function private in TabBookWindow::saveSelection() and 
   restoreSelection(). These will save the highlighted items, and restore them
   back, while entering and exiting the updateDownloads(), updateFileServer(),
   updateWaiting() calls.
   May 18

u) In sorting wrt Directory. The Partial Files (who have NULL directory),
   should appear before NULL Directories. Change in FilesDetailList::
   compareFilesDetail() for FD_COMPARE_DIRNAME.
   May 18
   
v) As FD->FileSize is more than resume + bytes received -> Handled in upload.
   Make it behave same way in download. So we can download more of the file
   than we thought its size was and not terminate when initial negotiated
   size is reached.
   Change in Transfer::download(). We keep reading till socket is closed on
   other end (returns -1). (return 0 => DCC timeout). So if readData returns
   -1, we check how much we have downloaded. If downloaded is >= filesize
   its a positve note -> download complete. If not its a failed download.
   May 18 - Have to test upload/download extensively.

w) We can have one more button called: List Partials. It will list only the
   partials. And we add one more check box, Ignore Partials, which can be
   used to ignore the partials in listing.
   ID_BUTTON_LIST_PARTIALS (button), ID_BUTTON_IGNORE_PARTIALS (Check box)
   onSearchListPartials(), onSearchIgnorePartialsCheckButton()
   May 18

x) Transfer.run() was closing file descriptors only on a Successul upload/
   download. Later it got closed only after TransferThr exited. So moving
   file in TransferThr() gave errors as file was still open. Made .run()
   close it on return.
   May 19th

y) After a Download/Upload run gets over, print the Upload/Download BPS at
   which it was done.
   Add Transfer::getAvgUploadBps(), Transfer::getAvgDownloadBps()
   May 19th

z) Added a LEFT Search too.
   ID_FIND_TEXT_WRAP_LEFT, ID_FIND_TEXT_WRAP_RIGHT: both handled by:
   TabBookWindow::onFindNextPrevious()
   May 19th

a) When cancelling a download from a non Iroffer server, currently we sleep
   10 seconds in the UI. Try to remove that.
   In TabBookWindow::cancelDownloadSelected(), on cancelling a Download
   (non Iroffer), now we put it in the UI_ToDwnldInit Q as a CANCELD
   request. Changed previous CANCEL to a CANCELQ request.
   Let DwnldInitThr deal with this.
   May 19th

b) On clicking on text, we do get that text up in the search UI. so if we add
   a right click menu item named "Search in Files", which acts on that text
   and switches to "File Search" tab. Have it in the pop up menu for the 
   Channel TABS.
   - ID_POPUP_SEARCHTEXTINFILES
   May 19th

c) Added NETWORK_NAME as "IRCSuper". In Quit messages, if NETWORK_NAME is
   found then do display it.
   Changed CLIENT_NAME as CLIENT_NAME_FULL
   Add CLIENT_NAME as "MasalaMate"
   May 20th

d) Right click -> ban  incorporated as /op +b.
   Manual unban = /op -b channel string
   Manual ban = /op +b channel string.
   May 20th

e) Change Right click -> Ban as Ban/Kick. That makes more sense.
   So change it to do a ban followed by a kick.
   May 20th

f) Added COUTs for file open/close - to cursorily check if all opens
   are closed.
   May 20th

g) Do not allow a "dcc send" to ourselves. Its stupid, and gives us a wrong
   information about us being not firewalled.
   May 20th

h) Add a comedy option in File Search right click popup. Call it,
   "FedEx File to Home". ID_FILESEARCH_FEDEX
   May 20th

i) If we get a change nick, from Server, but we have a password for the nick.
   Issue a ghost command, and then attempt to take the nick again.
   May 20th

j) Design Problem: On getting an Ad in main, all MMs who dont have that 
   nicks info will jump on it. Ex, superdude, joins and does /ad. All MMs
   will issue his trigger, which is about 40 or 50 of them. This is not
   right. Think on it.
   Say m1 m2 m3 m4 m5 ... m40 are in channel. They are all MMs.
   Say s1 enters channel and puts an ad out (say s1 is sysreset)
   Assume n1 n2 n3 n4 n5 are new MMs in channel and they do not know any
   other MMs.
   Current algo, will make m1 thru m40, and n1 thru n5 to access s1's trigger.

   Each MM has a view of the channel, with their own lists of people who are
   FW or NF.
   When I first join channel, I have no idea of my FW state. Nor, do I have
   a list of other MMs. In this state, I should only access MM ads as they
   are displayed and not Ads of other servers.
   Once I have a MM list of at least 10, I can assume that I am in the group.
   By this time, I am known to be NF of FW.
   Once in the group, the Ad access follows the propagation algorithm.
   It is still being thought off - hence ramblings -
   We will distribute it as a binary propagation.
   It makes sense to arrange the logical tree such that if a node is NF,
   then both its children are FW preferably, unless we run out of FWs.
   In the same note, if a node is FW, then both its children are NF,
   compulsorily.
   This above logical tree formation given the rules looks good.
   The head of the tree, and another NF node are the ones responsible for
   accesing all the triggers that are displayed in channel.
   Once they get the file information, they send out a "propagate AdNick"
   CTCP to both its children. The children on receiving the propagate
   CTCP, check if they have "AdNick"'s filelist. If they do they just
   keep quiet. If they dont, they issue parent's trigger and FFLC sync.
   On syncing, they now in turn send out a "propagate AdNick" to their
   children. This process continues, till it reaches Nick's who have the
   "AdNick"'s info and stop the propagation.
   Also, note that there are two parallel propagations in progress. That 
   will take care of failures.
   This sounds good so far.

   How to construct the logical tree fitting the criteria. The key is that
   all MMs in the group come up with the same logical tree.
     Ah !!!!! Another thought. We dont really need to construct a logical tree.
   Its enough if each MM chooses its two siblings without contention and
   satisfying the tree criteria. (One way is for each to construct that logical
   tree)

   Algorithm:
     Make two arrays. ArrayNF, ArrayFW. (ascending order of Nick Names)
     Go thru CHANNEL_MAIN list of Nicks. If Nick is MM and NF append it in
     ArrayNF. If Nick is MM and FW append it in ArrayFW.
     Note all MMs have that list in ascending order. Hence will form
     the same arrays within the group.
     Now we build FinalArray as below:
     Pick 1st from ArrayNF, put in FinalArray[1]
     Pick 1st and 2nd from ArrayFW, put in FinalArray[2], FinalArray[3].
     while picking and placing in Final Aray, just follow the two rules:
     Rule 1: If node is NF, 1st priority -> get FW children. If FW children
             not available, then get NF children.
     Rule 2: If node is FW, only NF children. If NF children not available
             then no children.

     This forms the first tree.
     For the second tree. Make two arrays ArrayNF and ArrayFW with
     descending order of Nick Names. Generate tree using same procedure as
     above.
     Hence we have two trees.
     The head of these two trees are the actual trigger accessers in
     CHANNEL_MAIN.

   PROBLEM: As tree gets bigger, it needs considerable more number of
   NF clients for it to be effective. Basically all odd levels in the
   tree consist of entirely NF MM clients. Hence to fill the 5th level
   we need about 32 NF clients, etc. Hence a different strategy.

   STRATEGY 2:
   -----------
   Assume the same NFArray and FWArray.
   The FW Array consists of FWs and UNs too.
   Two such arrays, one in ascending order, and one in descending order,
   to get the two trees for redundancy.
   Now the NFArray guys are a binary tree in themselves.
   So NF1 is parent of NF2 and NF3 etc.
   Hence NF1 gets trigger, propagates to NF2, NF3, who then propagate to
   NF4, NF5, NF6, NF7, NF8 etc.
   Parallelly, the same thing holds for the descending order list. NFlast
   gets the trigger, propagates to its children etc.
   During propagation, each NF node also propagates to 2 of its corresponding
   FW children. Hence each NF node tries to propagate to 4 MM clients.
   2 of which are MM NF clients. The other 2 are MM FW clients.
   A MM client who is FW, does not propagate further.
 
   This algorithm looks simpler and cleaner.

   IMPORTANT: if this algo is in place, FFLC should not increment the update
              count during exchanges.

   Implementation: (Propagation Algorithm)
   ===============
   IRCNickLists class additions:
     - Note FW => FW or UN or MB in the Firewall state.
     IRCNickLists.getFWMMcount(chan) // FW MM count (includes UN/MB/FW) - done
     IRCNickLists.getNFMMcount(chan) // NF/UN MM count. - done
     IRCNickLists.getNickFWMMindex(chan) // My FW MM index. - done
     IRCNickLists.getNickNFMMindex(chan) // My NF MM index. - done
     We already have functions to get our FW state ::getNickFirewall(chan,nick)
     Once we know our index and state, if we are NF category,
     We need to propagate to 2 NFs and 2 FWs. We use below to get them.
     IRCNickLists.getNickInChannelAtIndexFWMM(chan, index, nick) - done
     IRCNickLists.getNickInChannelAtIndexNFMM(chan, index, nick) - done
     Write test cases for the above functions - done.
     Algo:
        FWcount = getFWMMcount(), NFcount = getNFMMcount()
        MyFWIndex = getMyFWMMindex(), MyNFIndex = getMyNFMMindex()
        returns 0 if not fitting NF or FW.
        so one of them is valid.

        ToTriggerThr algo:
        For an Ad seen in main ->
        If I am NF and first or last guy in list.
        if (MyNFIndex == 1) || (MyNFIndex == NFcount) catch triggers.
        If NFcount == 0, catch triggers. // no trigger catcher in group.
        if (NFCount == 0) catch triggers.
        // End of trigger catching for an ad seen in main - done.

        On receiving a Propagate CTCP.
        Do I have the PropagateNick's files in FilesDB with update count = 0
        if yes, ignore.
        if no, issue trigger of the guy who sent us the Propagate CTCP.

        All triggers issued automatically (not manual triggers)
        now need to be propagated, when the listing has been got.
        ToTriggerThr -> when it issues the trigger, does add the
        FileList in the FServPending Q. We could add a marker there
        to identify it as a list to be propagated, on Dir listing
        being obtained. so its called PropagationNick. We note down
        in that field, which Nick's dir is trying to be propagated.
        New field added called PropagatingNick in FilesDetail.
        It is used in the FServPending Queue. So in ToTriggerThr()
        if we issue a trigger (cause of ad in main), we mark that nick's
        Ad that we see in PropagatingNick. DCCChatThr, when it has
        finished getting the listing, will check if PropagatingNick was
        set for this Fserv dir listing access. If it was, it will generate
        4 propagation ctcps as described below.
        Implementation:
        - Add PropagatingNick in FilesDetail - done.
        - Change copy/freeFilesDetail to take care of it - done.
        - set PropagatingNick in ToTriggerThr() (Q'd in FServPending) - done.

        Once DCCChatClient gets the listing and its marked as a propagate
        listing. PropagationNick = Nick whose listing is got, we do ->
        MyNFIndex = getMyNFMMindex()
        if we are not NF then do nothing. end of story.

        My NF children are 2 * MyNFIndex, 2 * MyNFIndex + 1
        MyNFChild1 = getNickInChannelAtIndexNFMM(chan, 2 * MyNFIndex);
        MyNFChild2 = getNickInChannelAtIndexNFMM(chan, 2 * MyNFIndex + 1);

        My second tree (backward) children are:
        SecondMyNFIndex = NFCount - MyNFIndex + 1;
        SecondMyNFChild1 = NFCount - (SecondMyNFIndex * 2) + 1;
        SecondMyNFChild2 = NFCount - (SecondMyNFIndex * 2 + 1) + 1;

        My FW children are 2 * MyNFIndex - 1, 2 * MyNFIndex.
        MyFWChild1 = getNickInChannelAtIndexFWMM(chan, 2 * MyNFIndex - 1)
        MyFWChild2 = getNickInChannelAtIndexFWMM(chan, 2 * MyNFIndex)

        /ctcp MyNFChild1 PROPAGATION PropagatedNick
        /ctcp MyNFChild2 PROPAGATION PropagatedNick
        /ctcp MyFWChild1 PROPAGATION PropagatedNick
        /ctcp MyFWChild1 PROPAGATION PropagatedNick
        (if the nicks are valid, that is)            - done.

        Now its left to the children to do whatever.
        NOTE: The Propagation ctcp should also send our current sends/queue
        information, as the child on receiving it will issue our trigger,
        and ToTriggerThr(), needs to put it in its TriggerTemplate.
        As ToTrigerThr() takes the help of TriggerParse class, we need to
        make TriggerParse class handle the propagation ctcp.

        Action on receiving a /ctcp PROPAGATION PropagatedNick (handled 
         by ToTriggerThr)
        Check if we already have that Nicks info with update count 0.
        If we dont, We issue the trigger, and attach PropagatedNick
        That covers everything !
        Implementation:
          Add IC_CTCP_PROPAGATE for recognising a PROPAGATION ctcp. - done.
          FromServerThr() to feed it to ToTriggerThr() - done.
          ToTriggerThr() to ask TriggerParse class to decipher it. - done
          So make TriggerParse class to recognize the PROPAGATION line.
          "FromNick PROPAGATION PropagatedNick Sends TotSends Qs TotQs"
            - done.
          Make ToTriggerThr() to handle the PROPAGATECTCP along with FSERVCTCP
            - done.
   So in FFLC UpdateCount is transferred as is - no more increment by 1 - done
   In ToTriggerThr(), if CTCPPROPAGATION, we do not issue trigger if
     UpdateCount <= 1 of the nick which is trying to be propagated => we 
   just got it and possibly just propagated.
   May 21st

k) Linux upgrade needs to set S_IRWXU | S_IRWXG on the upgraded binary.
   May 22nd

l) In quit message for "Be Right Back", remove ':' if present as MM gives till
   first received :
   May 22nd

m) DCC sends do not succeed if the Nick is not present in CHANNEL_MAIN. Moved
   this check to Helper::dccSend(), from TransferThr(). Added a UI message
   in server tab, to let the sender know the reason for the failure.
   May 22nd

n) Code walk thru - setNickFirewall() for our own Nick.
   setNickFirewall(), called in many places, redundantly. Its called
   erroneously in DCCThr.cpp
   setNickFirewall() should be called for our own nick in DCCServerThr()
   only. - checked and this was correct.
   May 22nd

o) In DCCThr() after connecting to users dccserver, we do not check if
   it connected and returned 151, in which case we shouldnt try the 
   dcc method. (Like Helper:dccSend())
   May 22nd

q) Code walk thru - setNickFirewall() for remote Nick
   setNickFirewall() should be called for remote nick in DCCThr().
    when it connects to remote nicks DCCServer - done.
   In DCCChatThr, we mark remote nick as not firewalled, on a successful 
   transaction with DCCCHatClient class. Hence change DCCChatClient 
   class to return bool indicating success/failure - done.
   Helper:dccSend(), when we successfully connect to remote nicks
   dccserver. - this wasnt in place - done.
   May 22nd

r) Added temp code to print Propagation algo decisions in channel.
   Code under #define DEBUG in ToTriggerThr() and DCCChatThr()
   May 22nd

s) Propagation algo Tweak:
   If PropagatedNick is our own nick - dont do it. (receiver - ToTriggerThr)
     - done
   Dont send a propagation to a nick for its own nick. (sender - DCCChatThr)
     happens when I get listing of that nick and that nick is my child - done
   May 22nd

t) In FFLC, we update fw_state only if we have the state as UNKNOWN.
   But with chimero, someone is finding him to be NF when he infact is
   firewalled. Debug ...
   OK bug in DCCChatThr, marking a success chat as NF. It should mark if succes
   and an outgoing (no ndccserver) connection.
   May 22nd

u) If I change a nick and I have a password, it refuses to change it.
   Bug introduced. - fixed.
   May 22nd

v) The Upgrade algo messages all ops in Chat. This generates lof of messages,
   which the serverblocks from the sender. Hence upgrade ctcp never reaches
   the Upgrade server for some users. Hence differnt strategy.
   We assign a nick, say "UpgradeMM" which will be the upgrade sever.
   To get upgraded, we check if UpgradeMM is present in CHANNEL_CHAT and
   is an op. We message it for an upgrade request. The Upgrade Server,
   should also respond to indicate that no upgrades are required.
   May 22nd

w) Changed Ad to have CLIENT_NAME_FULL.
   May 23rd

x) Rollback Truncate File crashes.
   May 23rd

y) UI to handle these *UPGRADE* messages:
     *UPGRADE* TRIGGER <- handled by triggerUpgrade(bool);
     *UPGRADE* DONE    <- handled by upgradeDone()
     *UPGRADE* NOTREQUIRED
     *UPGRADE* NOUPGRADER
   So we change upgradeDone() to upgradeNotify(), which then handles all
   the 3 notification messages.
   May 23rd
   
z) DCCServerThr(), we forgot to mark ourselves as not firewalled in getting
   an incoming DCCServer connect for CHAT and SEND.
   So the marking ourselves as not firewalled should also be moved to the
   DCCChatThr() if CHAT, using our dccserver.
   We accept all SENDS so can do that in DCCServerThr itself
   Corrected it.
   May 23rd

a) In Helper::dccSend(), we stop sends if Nick is not in Channel Main,
   we should allow sends of the files which start with CLIENT_NAME though,
   as it could be a trace file or Upgrade file.
   May 23rd

b) Multiple items selected in SearchUIs crashes Client.
   saveSelections had sel_number declared inside for loop, and initialised
   to 0. -> for (...) { int sel_number = 0; ... }
   Hence sel_number remained 0 in the loop each time it looped. Saving
   all values only in the first entry. All the rest remained as garbage.
   May 24th

c) filelist followed by small 'p' was returning serving list.
   Changed to make it case insensitive.
   May 24th

d) Put Left arrow before Search Text
   May 24th

e) Rename "Search for Text in Files" to "Search in Files"
   May 24th

f) Quit message can have ':' in it. Changed IC_QUIT processing.
   May 24th

g) Moved deletion of OnlyOneSem to end of IRCClient.cpp, after endit(),
   so that it is deleted only when client ends fully.
   May 24th

h) Memory leak of FD->PropagatedNick fixed, which were not getting deleted.
   Changed FilesDetail to call freeFilesDetailList() instead of deleting
   the individual FD elements in exception conditions.
   Removed Mutex protection inside freeFilesDetailList() as it wasnt
   required.
   May 24th

i) Got a SIGABRT crash which makes me think it was from FOX. Last FOX function
   called was addToNickList(). so added a TRACE before and after it accesses
   NickList->insertItem(). And looking at channel log, he crashed immediately
   after mickey_mouse entered channel:
[15:53] * mickey_mouse (mickey_mou@59.92.32.4) has joined #MasalaMate
[15:53] * h_u_l_L_a (h_u_l_L_a@202.177.185.5) Quit (Read error: Connection reset by peer)
   Note: The last call before crash was TCPConnect::readData()
   May 24th - open debug.
   --- Released --- May 25th Version.

j) Propagation CTCP flood: If I see the Ad of Nick. It gets kicked. I access it
   and send propagation to my children. They access my nick to get the 
   propagated nick's list. But I dont have any as the nick quit. So they send
   out propagation in turns, and no one has it. And it comes back to me, and
   I still dont have it. Endless loop. Hence, change as follows:
   Firstly, chekc if nick is in channel before issuing trigger.
   Secondly, if propagated nick is not in channel, dont issue trigger.
   Thirdly, if a propagation CTCP, then at end of transaction, propagate
   only if we indeed got the listing of the Propagated Nick.
   First, Second taken care of in ToTriggerThr()
   Third, taken care of in DCCChatThr()
   --- Released --- May 26th Version - CVS Release tag = Beta26May2005

k) In Linux, alphakaya's server stops responding to new CHAT/FSERV/SEND etc
   after a couple of hours. Basically new threads stop being spawned.
   Possibly pthread_create() failing. We are creating threads, but not
   pthread_join(), and the threads are non detached. Hence suspected thread
   leakage in linux build. Have to explicity set pthread_attr and set the
   state to detached_state before pthread_create for DCCChatThr, FileServerThr,
   TransferThr. Changes in files: DCCServerThr.cpp, DCCThr.cpp, Helper.cpp.
   Jul 13
   --- Linux patch Release --- May 26th version

l) In above build, DCCServThr.cpp, had a misplaced '{' in the #ifdef
   for the pthread_create() modification. Corrected that.
   Jul 16

m) In Makefile added -lpthread for building StackTraceTest else it wasnt 
   working properly for Linux build.
   Jul 16

n) Use backtrace() from execinfo.h for Linux port in StackTrace class,
   to print the stack trace.
   Jul 16

o) Change Makefile to generate map file for PRODUCTION build for the
   UIClient, which can be used to get function name with the trace file.
   Jul 16

p) Create a simple stacktrace() in MINGW version too. Added getfp() and
   mingw32BackTrace(), in StackTrace.cpp. This doesnt work correctly as
   of now. Before the signal, the stack is nice. Looking at the stack
   in the signal, it just ha scurrent signal handler information alone.
   This possibly could be a bug in gcc 3.4.2 for mingw port. As of now,
   I do not see any way around it. Has to be revisited later.
   Jul 16

q) In the Trace output, TRACE() macro, include the thread id too, so its
   easy to find which line belongs to which thread. Hence for now, we
   just call the function to get thread id, and hope not much of a performance
   hit.
   Jul 17

r) Some people have problem connecting out to port 6667. So we replicate
   same server, one with port 6667 and the other with port 6665.
   Jul 17

s) We respond to @find in chat channel - bug, we shouldnt.
   Jul 17

t) In LINUX, spawn konqueror or nautilus to open the Partial Dir and
   Serving Dir.
   Jul 17

u) Make traces be saved in Serving Directory/Crash. We modify class
   StackTrace to have a member -> setTraceDir(char *), which can be
   set at application start, and whenever "set Serving Dir", is invoked.
   We choose Serving Dir, as it is accessible from all clients.
   Jul 17

v) Added Thread ID prints with the type of signal in the header of the
   Trace file.
   Jul 18

w) SEGV on exit in Linux, in WaitForThread(DCCChatThrH) in DCCServerThr().
   Dont know why ?, but on Linux, threads are created detached, so that
   pthread_join(), is errorneous anyway. So possibly put those waits, 
   under an #ifdef __MINGW32__. But nonetheless, they shouldnt core dump.
   So possibly do this, for the non-detached threads created in
   DCCServerThr.cpp, DCCThr.cpp, TimerThr.cpp (Helper.cpp)
   Jul 19

x) DEBUG build in DCCChatThr.cpp, the NF tree and FW tree is not printed
   at all - BUG
   Jul 19

y) To not hog all bw, force socket buffer to 8192. We used to have it at
   65535. Testing with alpha. - No good. So will take it off, and revert
   it to 65535.
   Jul 20

z) When we restart MM, it tries to send files as soon as we connect to IRC
   server, but we havent yet got the nicklist. Hence the first nick which
   should have got the send is disqualified, saying not in channel.
   We need to delay more before attempting the first send as soon as we
   join channel.
   Changed TimerThr.cpp, so that it attemtps to send only if its been
   at least 10 seconds since CHANNEL_MAIN was joined.
   Jul 20

a) Record Upload/Download should be reset each time we restart MM.
   This should not be saved in the Config File. As of now, it still saves
   and reads from the config file. But we reset it to 0, as soon as we
   read it from the config file.
   Jul 20

b) Problem of TCPConnect class being piggy backed in FilesDetail, for the
   sole purpose of getting its Download/Upload BPS. We introduced some
   clumsy global locks and TCPConnect class pointers to keep track of
   validity etc, which still had room for errors.
   So a rework. We remove all the above. Make the individual threads
   responsible for updating the FilesDetail for the corresponding
   upload/download to reflect the correct data.
   TransferThr is the thread we are talking about. It uses the Transfer
   class to do the actual transfer. Transfer class also has access to
   XGlobal and Nick/FileName, so it can directly update the correct
   FilesDetail once in two seconds, which will be ideal.
   New function to handle this Transfer::updateFilesDetail(bool dwnld);
   Added one more function in FilesDetailList class called 
   FilesDetailList::updateFilesDetailNickFileConnectionDetails()
   which will be used by Transfer::updateFilesDetail();
   Remove the Connection related dirty copy in FilesDetailList::copyFiles...()

   Once we are done with this, remove all the TCPConnect class static ptr
   storage related code and mutex out.

   Also remove the IRCClient, calling Connection->closeSocket() etc.
   We have to change Transfer:: class, such that it gracefully exits
   on checking XGlobal->isIRCQuit().

   We still need the Connection in the FilesDetail structure.
   This is used in disconnecting transfers if a nick leaves the channel
   etc. Also for disconnecting the FileServer in progress, when exiting.
   July 20

c) In Linux, issuing a get from a subdirectory of another server doesnt
   update the q information correctly. This is cause the fully qualified
   name of the file is in WINDOWS format, and we use getFileName() to 
   extract the filename. So add one more function in Utilities.cpp called
   getWindowsFileName(), and use that call instead.
   Jul 20

d) Downloads are always showing 100 %, when still in progress.
   FilesDetailList::copyFiles...(), should still copy the Connection
   pointer over.
   Jul 20

e) An @find, !list did not have the preceding Nick Mode of the Nick
   which was saying it.
   Jul 21

f) Implement MaxUploadBps and MaxDwnldBps. These are per class CAPS.
   Changes in class TCPConnect for there. For this to interact with
   the application, we can have a XGlobal->... under the same names,
   and let the TransferThr, update the TCPConnect classes at initial
   and intermediate changes.
   For per socket cap, we do the actual write call in sizes of the actual
   cap/2 if write size is greater than cap/2
   Same holds true for the read calls.
   Tested the sends/gets.

   Note - If I am downloading, then, the last packet loops in the select
          call, and returns only after timeout is reached. Hence the end
          game takes 3 minutes, from 99 % to COMPLETE. So, in the select
          call we wait 1 second, and return if no data within that time,
          only for readData()

   Added TCPConnect::maxUploadBpsSleepTillOKToSend(), and
         TCPConnect::maxDwnldBpsSleepTillOKToSend().
     We can possibly add another check in these functions to make sure
   that they dont sleep for more than a second.
   July 22

h) Implement OverallMaxUploadBps and OverallMaxDwnldBps in TCPConnect class.
   Made functions common to both per object CAP and class wide CAP.
   July 22

i) CAP testing.
   Now have to add commands for testing, like:
   /cap upload each <value>
   /cap upload all <value>
   /cap download each <value>
   /cap download all <value>
   For these, changes in TabBookWindow.cpp
   Also we update the cap values in XGlobal. Hence it too needs those
   variables in there.
   Also, Transfer.cpp, in upload() and download(), periodically scans the
   XGlobal CAP variables and updates its settings. New function for that
   called Transfer::updateCAPFromGlobals(). We call this in upload()
   and download().
   July 22

j) For IRCClient::endit() to end cleanly, rather than leaving the download
   and upload hanging -> We keep monitoring the corresponding FilesDetail
   till it gets to be empty. As the Transfer, does check for IRCQuit() and
   exits.
   July 22

k) For the CAP, have another variable which controls what all are CAPed.
   For example, if I set a download CAP of 40000. Then an upload that is
   going on, and if the download is at max, then it will get delayed in
   trying to send the Ack, for waht it has received. Hence affecting
   it.
   Hence, by default, we do not monitor CAPing. We have to explicitly
   do a call to TCPConnect::monitorForDwnldCap() and
   TCPConnect::monitorForUploadCap(). With this we selectively
   cap the corresponding down/up objects.
   July 23

l) Make CAPs be 0 or > 511 for sanity. Corrected in both Global interface
   thru /cap command and in TCPConnect class.
   #define MIN_CAP_VALUE_BPS as 511
   July 23

m) BUG: In the FileServer, when a nick queued for a file, and it was a nick
   which had priority, we would enqueue before that nick. We should have
   added ourselves, after the last of the priority holding nicks.
   Made such that op gets earlier q position than voice etc.
   July 23

n) BUG: FileServer::requeueTransfer() had a bug in which a file from a sub
   directory would fail to requeue.
   July 23

o) BUG: FileServer::fservDir(), lists only the first Directory, if
   more than one directory exist like: Vision1, Vision2. In this case
   it lists only Vision2. Note that CD to Vision1 does work.
   Change in FileServer::fservCD(), str_index was 1 short.
   July 23

p) People want to add Serving Directories across drives etc.
   Hence suggestion is, to have a Dir -> Add Serving Dir.
   Each time we Add, it will appear in the Dir menu, as:
   Dir -> Open Serving Dir<n> etc.
   To keep it simple we can have a fixed max of 4 dirs.
   #define FSERV_MAX_SERVING_DIRECTORIES
   Added char *XChange::ServingDir[4], for this purpose.
   Change Helper::readConfigFile(), to read in ServingDir<1..4>
   Change Helper::writeFServeConfig, to write in ServingDir<1..4>
   So while generating the filelist, in Helper::recurGenerateMyFilesDB,
   we need to remember which ServingDir the filename is associated with. 
   So have to add a field called ServingDirIndex in FilesDetailList 
   to hold 0 ... 3. Changed also FilesDetail::printDebug and 
   FilesDetail::copyFilesDetail()
   change in Helper::generateMyFilesDB() to call the recur guy with
   the index number too.
   Hence if DownloadState = 'S' then use the ServingDirIndex to get 
   base dir in MyFilesDB entries.
   Now hunt for all other occurances of ServingDir, to use the index.

   GUI visual change:
   DIR ->
     Update Server
     Open Partial Dir
     Set Partial Dir
     Open Serving Dir ->
       Dir 1
       dynamic Dir 2
       dynamic Dir 3
       dynamic Dir 4
     Set Serving Dir ->
       dynamic Dir 2
       dynamic Dir 3
       dynamic Dir 4
     UnSet Serving Dir ->
       dynamic Dir 2
       dynamic Dir 3
       dynamic Dir 4
   July 24

q) Record BPS should be when a download/upload completes and more than
   1 MB is transferred. Hence the XGlobal->RecordUploadBPS and
   XGlobal->RecordDownloadBPS, should be updated in TransferThr and Transfer
   once it finishes the transfer. Remove the 5 second call to 
   Helper::updateRecordBPS from TimerThr. Also get rid of 
   Helper::updateRecordBPS.
   #define MIN_BYTES_TRANSFERRED_FOR_RECORD     1048576
   July 24

r) Manual sending file to Chimero, on DCC timeout, its not requeueing.
   That file does exist in the serving folders.
   Ok, so we get File and Dir information from SendsInProgress, and use that
   to get the FD from FilesDB. As its manual send, the Dir doesnt match,
   and hence it returns NULL. What we should do is, use the FD we get
   from SendsInProgress and use that to add it in QueuesInProgress, 
   initialising values like Connection etc.
   Also, if the FD is a Manual Send, we should queue it in Q 1 so that it
   auto resends.
   July 25

s) File Server should have a configurable retry option on a send failure.
   As of now we put it in q 2 if there is 1 member in q. But if there is 
   no one in q, we just discard the send. Someway to make it send again
   without making it send indefinitely. (Example, rollback error related)
   We can add a SendRetry field in FilesDetail for this purpose. Each time
   it resends/requeues we keep increasing this.
   Change in TimerThr, in which it delays 1 minute before resending if its
   RetryCount is non zero.
   July 25

t) When a DCC Send File gets in Q cause of a retry, and we select it and
   try to Push it, in TabBookWindow, we mark it as of type 'M', and hence
   the fullyqualified name gets messed up as the Serving Directory is 
   also prepended to get full file name. Hence, before we mark a file push
   as 'M', check if its of type 'D'. If it is, leave type as is.
   July 26

u) Trying to get file from arun200401. We get "Error writing to Socket."
   message from him in a few seconds. Download goes nowhere.
   TCPConnect::writeData(), possibly has to check for failure on the 
   select() call and the send() call. Check for EAGAIN is not being
   done.
   Added private function TCPConnect::errorNotOK()
   this will return true if the failure is not OK to be retried.
   July 26

v) Sends that are in progress, when we quit are lost and not saved in the
   config file to be restarted the next time we restart.
   This is because, on seeing isIRC_QUIT() (in Transfer.cpp), we return 
   without requeuing it in QueuesInProgress. On return, TransferThr.cpp
   removes it from the SendsInProgress Queue. Hence lost.
   July 26

w) CRASH: FromServerThr, closes the FD->Connection. Some other thread using that
   same Connection, is inside writeData or any other functions, which expects
   sane values for Sock. But already Connection is disconnected, and hence
   Sock = SOCK_UNUSED = ~0. It then calls the select call with fd, which
   crashes.
   Hence, we should seriously consider removing the "Connection" from 
   FD, and instead have something called TCPConnectMessage which is char.
   In Transfer::updateFilesDetail, as we update FD, we also check if the
   FD is telling us to shut down the transfer. In this case, we close socket.
   So we make updateFilesDetail return false, when connection closed, and
   true otherwise, so that the caller is warned not to use Sock etc.
   Note that we need to implement same mechanism in discoing for FileServer
   too.
   DwnldInitThr uses Nick, File to get FD. (cancelling a download)
   FromServerThr uses Nick to get FD. (cancelling all sends to nick)
   TabBookWindow can use Nick, File to get FD. (cancelling an xdcc download)

   So for the above, we need to add functions in FilesDetail class:
   updateFilesDetailAllNickConnectionMessage(Nick);
   updateFilesDetailNickFileConnectionMessage(Nick, File);
   These return true if update performed, else return false.

   FileServer will need to have a function where it monitors the IRCQuit()
   message and disconnects.
   Changed FileServer.run(), FileServer.fservDir(), FileServer.Sends(),
   FileServer.Queues()
   Also have to change Helper::fservRecvMessage(), so that it returns
   false on QUITing client, as its used by Helper::helperFServFilelist() 
   and Helper::helperFServNicklist()
   July 27

x) Have to serialise MM to MM Serving list exchanges. This will make them
   have better list maintenance, and will prevent stepping on each other.
   This is part of the Propagation Algo tweak.
   This involves Helper::helperFServFilelist() and 
   Helper::helperFServNicklist().
   Case study: Say there are more than one FFLCs in progress. Lets consider
   the filelist of Nick1.
   say A, B, C have Nick1's list and each have a different checksum =>
   Nick1's file list will be exchanged. Now say A has the least update count,
   => B and C want A's copy. While A's info is being update by B and C, say
   D and E are transacting with B, then B will propagate 1/2 ass file list
   information to D and E, each being different, depending on where the
   transaction with A was at that time.
   Hence a NickList exchange is OK => need not be serialised.
   We need to serialise the FileList exchange in FFLC. Hence as of now,
   seems like we need to serialise the functions which respond to a
   FILELIST command and the function which receives the response for
   FILELIST command.
   We create a MUTEX in XChange class, and call it FFLC_Mutex, and use
   that to serialise. Functions in XChange, to lock and unlock it.
   XChange::lockFFLC(), XChange::unlockFFLC()

   Lets try by locking the full nicklist/filelist transaction.
   Also make no TimeOut for FilesDB
   FileServer::fservNicklist(), FileServer::fservFilelist(),
   Helper::helperFServStartEndlist() have been locked.
   July 27

y) BUG: memory freed twice error in Helper::helperFServStartEndlist()
   tmp_str was nont NULLed and in case where it broke of first do {, before
   tmp_str got allocated, it would free that pointer. Hence LeakTracer
   aborted.
   Jul 30

z) We should possibly remove the "time to live" for FilesDB, and
   make it 0. Flush possibly occurring only cause of UpdateCount.
   This is part of the Propagation Algo tweak.
   Jul 30

a) The FServ Ad doesnt prepend our Nick Mode, while displaying in UI.
   Its generated by Helper::generateFServAd(), called from TimerThr.cpp
   and FromServerThr.cpp. The FromServerThr() one is basically a NOTICE
   on a !list, and hence no change in that one.
   Have to change TimerThr.cpp
   Also same case in TabBookWindow for Request unban option.
   Jul 30

b) When we copy paste multiple lines, we are currently pasting it as 
   one line. We should break down the line into multiple lines, and
   let it pass through ToServer Queue (slow one).
   Jul 30

c) Change FileServer::fservSends(), to reply like SysReset.
   Send 1: FileName 4MB is 0% done at 0 cps. Eta: 0 secs. Sending to: Sur4802.
   Added function Utilities::convertTimeToString(time_t, char[64]);
   as it will be used in both TabBookWindow, and FileServer.
   Jul 31

d) FSERV improvement related to CAP.
   Add an automatic send slot if speed < MINUPLOADOVERALLCPS
   If MINOVERALLCPS is 0, then its not in force. 
   This should be initiated in TimeThr, and it should monitor this Overall
   CPS, over a period of 10 minutes, and the OverallCPS should have been
   less than MINOVERALLCPS for this whole period, for it to push a send out.
   /cap upload minoverall MINUPLOADOVERALLCPS
   XGlobal->OverallMinUploadBPS is born.
   Also, change TCPConnect, such that when the OverallMaxCPS is initialized to
   0, its value is dumped in LastClassUploadTime, and LastClassUploadBps.
   These are the values, which will be monitored for bw, for the decision.
   We use these as they will be the actual bytes sent out in the past second,
   rather than using the bytes sent out this second, which is not yet
   complete.
      size_t TCPConnect::getLastUploadBpsTime(time_t *time_ptr)
   #define MIN_UPLOAD_CPS_TIMEPERIOD_FOR_SEND   300 // 5 minutes.
   July 31

e) BUG: in FilesDetailList::addFilesDetailAtIndex(). It was not adding
   to the list in the cases when # of elements in list is say 5. And a call
   comes in to add to list at index say, 10. In such cases we were not adding
   to the list at all.
   We should add that as the last entry.
   Note that this would have caused memory leaks as well as the whole FD
   entry is not queued and hence not freed as well.
   Aug 1

f) DCC -> Add to Queue.
   Can use this for adding a file in a Queue position.
   Only a file being served can be added into the Q. Once file is selected,
   we find what Serving Dir base and subdir this file is in. Once its
   determined, we prompt the user for the Q num, and insert the guy at
   Q position
   So we add a "/dcc queue nick qnum", along the same lines of "/dcc send nick"
   Now, when invoked from menu, we will put a default qnum = 0 => add to 
   last pos in queue. If we want more precise q num control, invoke from
   command line as "/dcc queue nick qnum" in the input text field.
   Change in TabBookWindow.
   - add ID_TOOLBAR_DCCQUEUE for menu.
   Aug 1

g) Improve the Waiting TAB.
   When a Update Q information is chosen, we should access file server,
   if its a SysReset File Server and get the sends and queues information.
   If Iroffer, then cant do much.
   If MM, we issue a /ctcp MMnick FileServerDetails.
   This will prompt MM to respond if MMnick is present in Sends/Queues.
   This will not work as in the case when we have 50 queues, we cant send
   all that information on a CTCP response.
   Hence for both sysreset and MM, same procedure of actually accessing the
   Server in question and issuing, "sends" and "queues" and presenting it
   in the Waiting TAB.

   Implementation:
    - TabBookWindow -> update Queue Information
      Basically the function TabBookWindow::onDownloadSearchUISelected()
   handles the ReRequest. Either from the Download or Waiting UI or the 
   File Search tab. So we do the same actions for all these, that is issue
   a sends and queues (only if its not sending immediately, so that there will
   exist an entry in the Waiting Queue to be populated relevantly).
   Now onDownloadSearchUISelected() just populates UI_ToDwnldInit, which will
   then be picked up by the DwnldInitThr().
   Hence change needs to be done to it.
   It issues the trigger, and adds entry in FServPending, so that is also OK.
   Work is done in the DCCChatThr(), which calls DCCChatClient.getFile()
   Hence change needs to be done to DCCChatClient.getFile(); It is called
   with DCC_Container, so we have access to everything.
   After the GET is issued, the response is analysed in analyseGetResponse()
   It updates the Q position as well. So we should add our new function
   called if the q_pos is non zero. Hence first, we change
   analyseGetResponse(), to also return the q_pos, as one of its parameters.
   Next add analyseSendsResponse() and analyseQueuesResponse() and call 
   them if q_pos != 0.
   They will update DwnldWaiting Q entry with the data got.
   Added Utilities::convertFileSizeToString(size, s_size); useful in many
   places to convert file size to human readable form.
   TabBookWindow, for Waiting TAB, we make the Waiting TAB of type 
   FXFoldingList. Apart from listing the FD, it lists the send/queue info
   present in Data of the FilesDetail structure. This Data is and array
   of strings, last entry is NULL.
   Data[0] = send info if any.
   ...
   Data[i] = queue info if any.
   ...
   Data[n] = NULL.
   This is tagged on to DwnldWaiting Queue, by DCCChatClient.cpp in
   analyseSendsResponse() and analyseQueuesResponse().
   Added FilesDetailList::updateFilesDetailNickFileData(char *nick, 
     char *file, void *data);
   Make all the IconList used in SearchUI as FoldingList.

   Now if server has block detailed sends and block detailed queues,
   then we kind of hang till the fserv connection is timed out.
   We get the message "Closing Idle connection in ..."

   In DCCChatClient, while filling up sends_info and queues_info, dont
   assume the Queue/Send number, extract it from the line as that is the
   correct way.

   On a Nick Change, have to propagate it into the FD->Data Nick fields
   in DwnldWaiting. - Not required as its old data taken at time of access.
   User can request an update Q information and update it.

   Have to delete the FD->Data, when its entry is deleted/moved from 
   DwnldWaiting. This happens, only when we start receiving a file which
   has an entry in DwnldWaiting. So one change is in TransferThr.cpp

   Cause of changing from FXIconList to FXFoldingList, have to check on
   crash in TabBookWindow::onFindNextPrevious() and other possibly changes.
   Aug 5

h) When attempting to update Q info or trying any downloads etc, as handled
   in TabBookWindow::onDownloadSearchUISelected(), we first check if we still
   have an entry in FServPending Queue, => that a previous FServ access is
   still pending, and hence we prompt user as such and request him to try
   again later.
   Aug 5
 
i) BUG: For completed downloads which happen over resends, the Current size 
   of file is always shown to be much larger than the actual current size. 
   FD->BytesReceived is not set once the download completes or partials.
   Set this up in TransferThr.cpp
   Aug 5

j) BUG: New Sends should be added at end of list in SendsInProgress.
   Aug 5

k) BUG: scenario: nick batata_wada exists in channel
   I issue a: /nick batata_wada
   I get a IC_NICKINUSE from server and I then change my nick to some random
   nick.
   We should generate a random nick, only if we are not yet fully registered
   to the IRC server, and not in other cases.
   XGlobal IRCNick is set on a CONNECT in FromServerThr, on a succesful
   nick registration. Hence if we get XGlobal IRCNick and see that its been
   set, we need not change our nick, on getting the IC_NICKINUSE.
   So, a code walk through for when XGlobal->IRCNick to be set.
   It should be set when IRC server has registered the nick we gave, or
   has acknowledged the Nick Change that we gave.
   Currently its being changed in 
   - TabBookWindow, on issuing a /nick
   - TabBookWindow, on issuing a ToolBar -> Nick change.
   - Helper, on initial if config file has no nickname.
   - FromServerThr, in IC_NICKCHANGE.
                    in IC_CONNECT
                    in IC_NICKINUSE
   So we can have one more Global called ProposedNick, which will have the
   nick as read from Config file or /nick command. Then we try to take
   the nick as in ProposedNick. On success, it should be noted down in
   global IRCNick.
   Scenario 1: Nick hanging in channel.  Join IRC, ProposedNick in config file
   is same. Global IRCNick is same. (config file sets it on startup)
   So sends ProposedNick to server to register.
   We get IC_NICKINUSE. In this scenario we generate random nick, and
   register. If NickPassword is set, we send a ghost on Global nick and
   pipeline a NICK command.
   Scenario 2: /nick ExistingNick. We get a IC_NICKINUSE. and confuses
   everyone. We should do recover etc only for Scenario 1. In Scenario2
   we should keep the previous nick we had, and not do anything more.
   So in scenario 2, proposed nick and Global nick will be different.
   
   In all proposed nick changes we set ProposedNick, and change Nick only
   when IRC says its accepted. At this point, update UI with NICK_MY message.
   Aug 6

l) BUG: Sometime we loop indefinitely, when file is sent and we are awaiting
   the DCC ack bytes. This is cause, in the end, we call readAckBytes()
   with DCCSEND_TIMEOUT, so we get all of it. readAckBytes would
   return true, if readData() got 0 bytes -> timeout and no data available.
   Hence we keep looping trying to get the last acks.
   So, readAckBytes() when timeout is DCCSEND_TIMEOUT, and we get 0 bytes
   in readData(), we should return false. 
   Aug 10

m) FSERV:
   - configurable # of overall queues. (minimum 10)
   - configurable # of overall sends. (minimum 2)
   - configurable # of sends per user. (minimum 1)
   - configurable # of queues per user. (minimum 1)
   Note that sends always count towards queues.
   Commands:
     /fserv queues overall <num>
     /fserv queues user    <num>
     /fserv sends overall  <num>
     /fserv sends user     <num>
     /fserv print
   So we add the respective globals in XChange.cpp, which should be accessed
   by calling lock() etc.
   We also need to save these values in the config file ->
     Helper::readFServParamsConfig()
   and read them on startup -> Helper::readConfigFile()
   writeFServConfig() will call writeFServParamsConfig() too.
   writeFServParamsConfig() can be called to just update the Params.
   send/queue determination logic changed in FileServer.cpp
   clr_queues should clear all queues.
   Add clr_queue <num> command, which clears that nick from that q position.
   Research how sysreset handles this command and implement it.
    get fc3-i386-dvd.iso
    Adding your file to queue slot 1. The file will send when ...
    get suse-linux-9.2-ftp-dvd.iso
    Adding your file to queue slot 2. The file will send when ...
    clr_queues
    Removing fc3-i386-dvd.iso from slot 1.
    Removing suse-linux-9.2-ftp-dvd.iso from slot 2.
    get fc3-i386-dvd.iso
    Adding your file to queue slot 1. The file will send when ...
    get suse-linux-9.2-ftp-dvd.iso
    Adding your file to queue slot 2. The file will send when ...
    clr_queue 2
    Removing suse-linux-9.2-ftp-dvd.iso from slot 2.
    clr_queue 10
    I can't remove queues that don't exist!
   Also in welcomeMessage(), mentions its CAP like sysreset - done.
   Aug 11

n) Possibly save and restore the cap information in cfg file.
   change in Helper::readConfigFile(), and added Helper::writeCapConfig();
   Aug 11

o) FilesDetail *Helper::getSuitableQueueForSend(), for returning an FD
   from QueuesInProgress. This chooses the best FD from the Queues, for a
   send.
   Gets a Nick who has the least number of sends in progress.
   Hence we have NickS1 and NickS2 having sends, and NickS1 in Q1, and NickS2
   in Q2, and NickQ1 in Q3. We will pick NickQ1 for a send.
   In a scenario, when NickS1 is in send 1, and in queue 1. It will be picked
   from queue 1 as the prospective send. This is OK, as we are utilising the
   send slots which are free, to maximise bw.
   Further on when we introduce the preempt algorithm of preempting a send
   as a more deserving Q is present, it shoudl take care of fiar sharing
   the send slots.
   Aug 11

p) If RetryCount > TRANSFER_MAX_RETRY do not requeue. Change put in
   Transfer.cpp. Scenario: Sysreset is already downloading a file I am try
   ing to send. So it doesnt accept, but doesnt send a noresend. Hence when
   MM restarts, and first q entry is such a send, it keeps trying to send,
   and doesnt pick up the other entries in q at all. Indefinite starvation.
   Aug 11

q) FSERV:
   - small files sending sooner.
   possible logic being of FServOverallSends, 1/2 of them should be for
   small files. Size of file is determined at the time of issuing a GET,
   in terms of resumes, or actual filesize. MM to MM should now use
   GETFROM resumeposition filename. We note down resume position in the FD.
   When its time to actually send the file, we check that the resume position
   is greater than or equal to what was hinted. If not, we do not start
   the upload, and send a message why we didnt send.
   We should possibly have another Q for accomodating the files of smaller
   size. OverallSends/2 # of sends is reserved for small file sends.
   If small file sends are not being utilised, they will be passed on to the
   big file sends. In such a scenario where a small file q gets occupied,
   and there exists one send which belongs to small file, but is used by
   big file, we cancel that big file send, and put it back in big file Q.
   Send a notice informing the same. Then, push the small file in Q.
   The scenario other way around is true as well (small file sends occupying
   a big file send)
   XChange->FServSmallFileSize -> size <= this, are small files.
   add it to /fserv smallfilesize <size>
   #define FSERV_SMALL_FILE_MAX_SIZE   200*1024*1024      // 200MB
   #define FSERV_SMALL_FILE_DEFAULT_SIZE 60*1024*1024     // 60 MB
   Change TabBookWindow, Helper, to update with /fserv and to read and write
   in config file.

   Add a new Q for just small files: XChange->SmallQueuesInProgress
   Add the GETFROM command in FileServer
   Both GET/GETPARTIAL and GETFROM/GETFROMPARTIAL, should intelligently 
   add the file to the correct queue.
   Manual Sends/DCC Sends go in the appropriate Queue or SmallQueue
   depending on the filesize. 
   TimerThr, should monitor both Qs for pushing manual sends out. It checks
   BigQueue, and if no such manual sends, checks on SmallQueue.
   Modify TabBookWindow to display both the Big and Small Queues.
   FileServer in welcome message should display InstaSends like sysreset.
   Instant Send is at: 1MB
   Helper::writeFServConfig() to save both Big and Small Q. It will have
   to save the resume_pos too, so that while reading Config file we can
   update the ResumePosition in the FD and pull it into the correct Q.
   While adding to Q, we need to update ResumePosition in FD, so that it
   can be verified while the actual send/resume takes place.
   Helper::writeFServConfig(), writes with ResumePosition saved. Does not
   differentiate between small or big Q. Just writes out sends and big q
   entries and small entries in that order.
   Helper::readFServConfig() will load the entries and will put them in the
   correct q depending on the resumeposition, filesize and the smallfilesize
   value defined.

   TimerThr is the one who monitors sends, and decides. It should now
   also decide to terminate a send, and make way for a send from the big
   Q or the small Q.
   Algo is -> We check all the sends, and see if its imbalanced.
      Imbalanced, means, all sends are occupied, and big sends
      are more than allowed ceil(TotalSends/2) and a small Q has an entry
      or small sends are more than allowed floor(TotalSends/2) and
      a big Q has an entry. In such a case, we get the suitable Send FD to
      be stopped.
      Example: TotalSends = 2 => 1 send for big files, 1 for small files.
            TotalSends = 3 => 2 sends for big files, 1 for small files.
            ...
      Hence we create int Helper::stopImbalancedSends();
      which does the above and cancels the appropriate Send.
      It returns -1 => nothing cancelled.      SEND_FROM_NO_QUEUES
      It returns  0 => get a send from SmallQ. SEND_FROM_SMALL_QUEUE
      It returns  1 => get a send from BigQ.   SEND_FROM_BIG_QUEUE

   Note that we will try to utilise all sends possible, hence small files
   will occupy send slots of big files, if no big files present to be sent,
   and vice versa. Hence TimerThr() will have to routinely monitor the sends
   in progress, and decide if it needs to terminate a send in progress (will
   be requeued appropriately by Transfer::requeueTransfer), to make way for
   a Q element.

   Modify Helper::getSuitableQueueForSend(bool SmallQ), modified to take in
   parameter SmallQ. It uses the appropriate Q, BigQ or SmallQ, to select
   the appropriate FD.
   Aug 13

r) imbalance wrongly looks at XChange->MaxOverallSends, to calculate
   the proper send slots for Big and Small Files. Now when a MinCPS
   send is pushed out, the imbalance algo will spot it as an imbalance
   and disconnect it. Hence the proper way to calculate total sends for
   imbalance calculations is:
   Total sends currently in progress - DCC Sends - File Pushes.
   Also when going through the Sends and Init Sends, we should ignore
   DCC Sends and File Pushes.
   Also in disconnecting an imbalance send, we ignore DCC Sends and File
   Pushes.
   Aug 14

s) When a send is discoed and queued as part of imbalance algorithm, we 
   should send a message to that nick with the Q information. 
   BUG: zero out the ConnectionMessage on Requeue. Else it will again and
   again get disconnected.
   Aug 14

t) The FServ Overall Queue length stands for each of the queues.
   Hence total Queues = n of Small and n of Big => 2*n.
   We should use that guideline to determine total queues.
   Hence totalqueues = 2 * overall queues.
   1/2 are for Big and 1/2 are for Small files.
   Corrected the Queues Response. Corrected FileServer to accept small
   files and big files seperately as per their individal capacity.
   Aug 14

u) BUG: OverallMin sends were not working properly. We monitor min upload bps
   by calling T->getLastUploadBpsTime(&time_bps); And we see there are
   sporadic spikes which look unreal, when comparing them to the BPS of the
   sends as seen in the File Server tab. Hence, looks like it makes sense
   to add up the UploadBPS as seen in SendsInProgress, to decide if we
   are making the cap, rather than the previous method.
   So, added size_t Helper::getCurrentOverallUploadBPSFromSends();
   Aug 14

v) BUG: When all sends are correctly sending as per ratio of SMALL/BIG.
   And when minuploadcps kicks in another send, at that state, when
   stopSends is called, it points to getting small files. which can create
   a disconnection frenzy, as it will imbalance the sends. Hence it has to 
   be more intelligent.
   Aug 14

w) Remove ArtDvdHike from the Messages TAB, and put in NickServ instead.
   Aug 18

x) Avoid an FServ access in the below scenarios:
   - !list <nick>, not to be issued, if already there is fserv pending
     or in progress.
   - An FServ access not to be issued, if already there is a fserv pending
     or in progress, when we issue a download, update queue information,
     rerequent a partial, rerequest a waiting, cancel selected download etc.
   Currently we have only FServPending, which holds the nick information
   of a possible impending File Server access that we initiated. We do not
   explicity touch it, and only the timeout clears its entries - timeout =
   180 seconds currently.
   In days to come, we might want to have one more Queue which has the ones
   that are currently in progress.

   Implementation:
   TabBookWindow::onDownloadSearchUISelected() handles all Downloads.
   TabBookWindow::cancelDownloadSelected() handles a cancel - the file server
     of server nick is not accessed - so nothing to do.
   TabBookWindow::cancelWaitingSelected() - handles a cancel of waiting q.

   Now for !list <nick>, we put that check in onTextEntry() as it handles
   all the text entered. It puts up a FXMessageBox there.
   Aug 18

y) BUG: In case when a nick is ghosted and recovered, our mode is not
   -v. So we do a mode change also after we ghost.
   Aug 18

z) A possible way to move upload cap around, is to allow it to be set.
   When a download record is updated, we make sure that if upload cap is set,
   its greater than 1/4th of that record download. And at that same time,
   make sure that upload per user cap is > overall upload cap / # of sends.
   Also at the same time, if upload cap comes out to be > 20, then set
   overallmincps = overall upload cap - 10
   Implementation:
   - XGlobal->RecordDownloadBPS, is updated in Transfer.cpp (4 places),
     TransferThr.cpp (2 places)
     So we can add Helper::updateRecordDownloadAndAdjustUploadCaps(transferbps).
   Aug 19
  
a) Add the CAP values in a clientinfo reply.
   Aug 19

b) Make the header for the SearchUI type tabs, be resizable, like it was
   when we used FXIconList.
   Aug 19

c) BUG: We have sends and queues. We get disconnected from server, and
   rejoin. We do not send any more files.
   As part of solving this, move some more TimerThr code into Helper.
   Also streamline the code, in sections, ex. first section -> duties
   to be done even if we are disconnected from IRC server, second section
   -> duties to be done if we are connected to server but not joined
   CHANNEL_MAIN, third section -> duties to be done if we are connected to
   server and joined in CHANNEL_MAIN.
   Aug 19

d) When issuing a PORTCHECK, we connect to the nick's ip, and also make
   sure that the socket is listened to by the same nick.
   Same logic for PORTCHECKME as well. Change in DCCThr.cpp
   Aug 19

e) Checking where all ToServerNow() is used.
   - FromServerThr -> responding to server PING - OK
   - FromServerThr -> Retrying different nick, when nick in use - OK
   - TransferThr -> To send a NORESEND - OK.
   - TimerThr -> PING the server - OK.
   - TabBookWindow -> To send a NORESEND - OK.
   - TabBookWindow -> Typed Text - OK.
   - TabBookWindow -> Manual DCC Send, USERHOST for IP - OK.
     etc, all OK.
   So lets increase the seconds before ToServer() pushes a line out.
   It used to be 2 seconds. Now we make it 3 seconds.
   Aug 19
 
f) Change all MM to MM gets to GETFROM or GETFROMPARTIAL
   should be turned on just before release, with slight testing.
   Change will be in DCCChatClient.cpp, when it issues the GET/GETPARTIAL.
   Need to change it to form: GETFROM/GETPARTIALFROM, with the additional
   parameter of resume position.
   Aug 19

g) Sanitized overallmincps wrt Record Download, when user changes it
   using /cap
   Aug 20

h) hulla got a crash in restoreSelections()
   Possibly for loop problem in expression -> (i < NumberItems, selection)
   Changed it to ((i < NumberItems) && (selection != NULL))
   Aug 20

i) BUG: in imbalance.
   Scenario 1: 2 total sends configured. Say all 2 sends are in progress
   with big files sends, and no small files in Q. Say, a small file is 
   queued. Imbalance algo should have stopped the last big send, and pushed 
   the small send. But it didnt.
   This is true also, when all 2 are occupied by small sends, and a big
   file is queued.
   Aug 21

j) BUG: Small file queueing seems to work erratically. Alpha configure with
   total 3 sends, 1 Each. total 10 queues, 1 each. small file = 60 mb.
   current sends = 3, current queues = 10. Cant q a small or big file.
   Added a different message in FileServer response, to see if it atleast
   identifies the Q correctly.
   - NOT RESOLVED -
   Aug 21

k) Scrolling seems to work in FOX GUI now. So have put the scroll enable/
   disable code under #ifdef FOX_SCROLLTEXT_BUG
   Aug 21

l) generateMyFilesDB() and generateMyPartialFilesDB(), should be run
   serially, so that two threads dont run over each other updating the list.
   In fact, if one is updating, the other thread should just wait for its
   completion and return, as the work is already being done, by means of
   some other threads call. Well, as of now we just serialize it, using
   lockFFLC().
   Aug 21

m) On cancelling a q -> I got a message in server window with ^C09 instead
   of it showing up as a color. Also the message: "^C09Cancel Queue: 
    Clearing (null). alphakaya Responds: Removing home.mpg from slot 1."
   Aug 22

n) While queueing and deleteing from q multiple times from alpha's server,
   we crash. SEGV points to onWaitingPopUP(), but looks like the last function
   called is restoreSelections(). Looks exactly like what hulla crashed the
   other day. Here its possible that, while we right click and get the pop
   up window, the updateWaiting gets called, and it updates the list. In fact,
   it deletes all list entries and repopulates it afresh. Hence if in the pop
   up we are using the selection, it might be invalid once updateWaiting()
   is done refreshing the window. This is true for all windows.
   Hence, once we popup the FXMessageBox(), from that moment on we shouldnt
   use the "FXFoldingItem *selection" entry.
   This needs to be put in place for the tabs which are updated every 5
   seconds. -> Downloads, Waiting, File Server
   What we could do is, as the update using the semaphore, we could down
   that semaphore on entry into the function, and up it as we leave it.
   That will provide mutual exclusion wrt the update functions.
   Semaphore in question => XGlobal->UpdateUI_Sem.
   That is not good, as it halts all UI update activity.
   We need exclusiveness wrt ID_PERIODIC_UI_UPDATE which is initiated by FOX.
   So we just introduce a bool variable private to TabBookWindow called
   AllowUpdates. We initialize it to true. We set it to false in the
   Popups if FXMessageBox is invoked. Change the update function to return
   without doing anything if AllowUpdates is false.
   Changed in ->
   - onDownloadsPopUp()
   - onWaitingPopUp()
   - onFileServerPopUp()
   Aug 22

o) Helper::fservGenerateAd(), was not generating the correct number of 
   queues currently occupied. Helper::getTotalQueues() corrected.
   Aug 22

p) BUG: FileServer::fservClearQueues(), not clearing queue(s) if they
   existed in SmallQueue.
   Aug 22

q) When people queue files from our server, we do see some guys queue in with
   a filename, whose filesize is somehow 0 bytes. This is wrong in the first
   place, and in the second place, they get queued into small file queue
   as well.
   Ah ! So this happens when a firewalled user tries to get a file, and as a
   result gets requeued. DCCServerThr, for such connections wasnt initialising
   DCC_Container.FileSize, before spawning TransferThr.
   Aug 22

r) imbalance alogrithm bug - When all big sends in progress and only big
   queues present or when all small sends in progress and only small queues
   present, it still tries to disconnect and balance.
   Aug 23

s) Remove using the lockFFLC, for generation of MyFielsDB. It results in
   quite a starvation as it never gets the lock as there are many FFLC
   transactions waiting to happen, and TimerThr hangs, waiting for the
   lock.
   Aug 23

t) HANG on exit: Also the process was takign 99 % CPU. TimerThr only thread
   not exiting. A while loop had ScanFD = ScanFD->Next in the wrong place.
   Aug 23

u) An entry remains in SendsInProgress even after its disconnected, in the
   below scenario.
   - A send is initiated.
   - Receiver changes Nick. (SendsInProgress has the new nick)
   - Send gets over or disconnected.
   We use the nick which was before the transfer is initiated, hence do not
   successfully delete the nick.
   This happens also when we try to requeue and dont get the RemoteNick entry.
   So instead of using the RemoteNick entry, we should get the matching FD
   based on IP and filename. So in Transfer and TransferThr, we should obtain
   the actual current nick from getting Nick based on DottedIP and FileName.
   New function in FilesDetail called getFilesDetailFileNameDottedIP()
   returns the first entry that matches.
   Same is true for DwnldsInProgress.
   Also add delFilesDetailFileNameDottedIP()
   - Changes in TransferThr. - Done.
   - Changes in Transfer - requeueTransfer(), updateFilesDetail()
   - Changes in FileServerThr.
   In all places where RemoteNick is used.
   Aug 23

v) As part of above change, a leak introduced in Transfer::updateFilesDetail()
   Not freeing up FD.
   Aug 24

w) In exit we loop for all the SendsInProgress and FileServersInProgress
   and DwnldsInProgress to end. There are cases when a nick change can
   escape being disconnected. So, we loop like 10 times max waiting for
   it to end, and get out of loop. We do not try to call ->disConnect(),
   cause the Conneciton pointer might be invalid. The actual transfer would
   have really ended way back, and this is just a stray FD.
   Aug 24

x) In FileServer::fservGet(), qnum is not set if file being downloaded
   is MasalaMate*, set it correctly to 1.
   Aug 24

y) Another hang on exit cause of UpdateUI_Sem being destroyed in UI::fromUI()
   when UI.cpp::toUI() is still waiting on that semaphore.
   Hence move the creation and destruction of the UpdateUI_Sem into 
   XChange.cpp, so that it exists till those two threads die out.
   Aug 25

z) The auto scroll stops working in Windows, once the window is minimised.
   Hence for convenience, adding the ScrollCheck Box, default will remain
   unticked. So we force scroll if ticked and auto scroll if unticked.
   Aug 25

a) Increase StackTrace to hold 1024 lines. Current 400 doesnt seem enough.
   Aug 25

b) In the Title of the FileSearch TAB, apart from the current selected FD's
   total files, total size, we also add the overall Total Files and overall
   Total Size. And this information should be updated as part of the
   periodic UI update done every 5 seconds.
   Aug 25

c) In the FFLC algo, we delete the nick in FilesDB in the nicklist interchange
   stage. This will cause us to lose the files of all nicks we will get
   in the filelist part of the transaction. In case the nick we are 
   transacting quits or discoes, we lose all that information. Hence we
   should actually obtain the FileList of the nick, and then delete the info
   from our DB and add the new info got.
   Also removed lot of unnecessary lockFFLC, and got it in where really
   needed.
   Also reintroduced the lockFFLC in MyFilesDB generation.
   Aug 25

d) FilesDetail::addFilesDetail() was not adding if a list was presented
   to it.
   Aug 25

e) Bug when adding XDCC FDs to FilesDB. DownloadState is not set to 'S'.
   Hence the checksum is always calculated 0. Further its not transferred
   via FFLC as it uses getFilesDetailByDownloadState().
   Another thing to trace is if "No Files Present" is propagated. Its a valid
   File, and should be present in both DownloadState(s) (for MM, but only S,
   if its XDCC/SysReset), when no files are being served. Should get 
   calculated in the checksum too. Hence it should
   have a valid DownloadState, and should be propagated in FFLC exchanges.
   Also when noting FileLists as S or P, for XDCC/sysreset we should
   add a "No Files Present" entry for nicks whose file entries are got,
   but are not serving any file.
   MM - done. (No Files Present if necessary in S or P)
   XDCC - to be done in IC_PRIVMSG and IC_NOTICE.
        code is repeated for IC_PRIVMSG and IC_NOTICE. So moving code to
        Helper. Code which deals with XDCC updates. - done.
   SysReset - Added DownloadState = 'S' for No Files Present - done.
   BUG: checkSumString() returning different values for Linux and
        Windows - corrected. Introduced: MM_LITTLE_ENDIAN - done.

   To test this - setup scorpion. Have one iroffer server, and two MMs.
   See how their FFLC update works - done.
   Then, have one sysreset server, and two MMs. See how their FFLC update
   works - done.
   Aug 26

f) StackTrace improvements. All Threads which are important shall register
   with it, so that StackTrace will crash only those threads. Hence the
   threads like FileServerThr, TransferThr should not register with 
   StackTrace, so that their crashes are ignored, but stack trace generated.
   Hence we have two forms of the TRACE_INIT() macro.
   TRACE_INIT_CRASH() => which registers the calling thread as which should
   crash on signal. TRACE_INIT_NOCRASH() is the other form.
   Threads which are registered to crash:
   - DCCServerThr
   - ToTriggerThr
   - ToServerThr
   - ToServerNowThr
   - FromServerThr
   - ToUIThr
   - DCCThr
   - FromUIThr
   - DwnldInitThr
   - TimerThr
   - ToTriggerNowThr
   - Client

   Threads which are not registered to crash:
   - DCCChatThr
   - FileServerThr
   - TransferThr
   - UpnpThr
   Aug 27

g) Change in some UI. Let people join more channels. A /join channel, should
   let them join that channel. To reduce clutter, We move the channels we are
   joined in, in a seperate UI below the NickList UI. Only one Channel Window
   will remain in the TAB. It will take the channel name of whatever we click.
   By default we join 3 channels, these cannot be parted from as well.
   - #IndianMasala
   - #Masala-Chat
   - #MasalaMate  (Only MM clients allowed in this channel).
   In DEBUG build we can choose to see CHANNEL_MM - Done.

   Which nicks are MM ? All nicks in Channel #MasalaMate will be taken
   as MasalaMate clients. - FromServerThr - Done.

   MasalaMate channel thoughts.
   If we get op in CHANNEL_MM, we set the key and make it private and secret.
   - FromServerThr. - Done.
   On each JOIN/QUIT in CHANNEL_MM, if we are op, we make sure that first 
   and last in list are ops. And then, if we are not first or last, we 
   deop ourselves.
   - FromServerThr - Done.

   Implement the /join command. Only allowed in a Channel TAB => starts with #
   - TabBookWindow (issue the join, and update CL) - Done.
   - Move TAB to new Channel TAB - Done.

   Implement the /part command. Only allowed in a Channel TAB => starts with
   #. Not allowed to part from CHANNEL_MAIN, CHANNEL_CHAT and CHANNEL_MM
   All others part, remove from ChannelListUI.
   - TabBookWindow - Done. 
   - Move TAB to Channel 1 = CHANNEL_MAIN - Done.
   
   Try to have only one object ChannelListUI. Hook that same UI across
   Channel UIs, so that it has same size on different channels, possibly
   using reparent(). 
   - TabBookWindow - Done.

   When there is new Text in the Channels, somehow make it visible in the
   ChannelListUI. Possibly add an icon to it, so that its visible.
   ChannelListUI->setItemIcon(index, FXicon *, owned = FALSE);
   so have to first get an icon.
   - Done.

   Save the joined channels in the Config File, so its remembered on 
   restart. New function Helper:: writeChannelConfigFile(), and update
   readConfigFile()
   - Helper.cpp - Done.

   Do not allow OP commands to be executed in CHANNEL_MM.
   - TabBookWindow - Done.
   Aug 29

h) FFLC problems. The way it works is once I get a listing, and if I am
   not firewalled, I try to propagate. On seeing a trigger in main, I access
   it only if I am not firewalled and I am head or tail of the MM group.
   This works well if all have the same MM list. Looks like all do not.
   So, new strategy, All MMs join channel #MasalaMate. I have registered it
   and so Stealth/KhelBaccha etc can join and be op there (set key/private/
   secret), and make apt MMs (first and last) as ops. This was done as
   part of previous action item.
   (Side INot:: No need to register #MasalaMate, as long as an MM becomes OP)
   Now, with this premise we deal with actual FFLC and Ad sync algoes.

   On Joining CHANNEL_MM, I need to mark all the nicks in that channel
   as MM clients. A Op Nick in CHANNEL_MM does the following:
   Makes sure that first and last nicks are ops, and deops itself after that.
   The common things done in IC_MODE, IC_JOIN, IC_NICKLIST is as below:
   1. Mark nick as MM client in all the basic 3 channels.
      Helper::markAsMMClient(char *nick);
   2. Check if we are op is CHANNEL_MM, make 1st and last nick as op. And then
      if we are not the first or the last, we deop ourselves.
      Helper::doOpDutiesIfOpInChannelMM()
   Do the above 2 points intelligently in FromServerThr(), IC_JOIN,
   IC_NICKLIST, IC_PART, IC_QUIT, IC_KICK, IC_NICKCHANGE
   - Done

   Now this affects the ADSYNC algorithm. Cause of this change (in knowing
   MM nicks immediately upon joining channel), we can sync the ADs
   beautifully. We just need to set initial Ad time to some seconds
   after CHANNEL_MAIN join. And set the next Ad Time, each time we see
   an MM Ad in CHANNEL_MAIN.
   We now longer have to see FSERV_MM_ADS_SEEN number of MM Ads to be
   eligible to Ad.
   Helper::calculateFServAdTime(ad_nick, our_nick);
   - Done.

   Make FSERV_INITIAL_AD_TIME to be FSERV_RELATIVE_AD_TIME * 5
   and FSERV_RELATIVE_AD_TIME to be 300 seconds. (5 minutes)
   and PINPONG_TIMEOUT 120 (2 minutes)
   - Done.
   Aug 30
   
i) Instead of having a constant Channel CHANNEL_MM = "MasalaMate" for
   AdSync and other stuff, we should use CHANNEL_MM = CHANNEL_MAIN + "MM"
   This will be helpful when we allow CHANNEL_MAIN to be changed.
   Aug 30

j) On Choose update files of nick, we no longer switch to CHANNEL_MAIN.
   This is true when invoked from the Nick Popup.
   And we disallow it from happening in the CHANNEL_CHAT channel.
   for update files of nick being issued from the non Channel TABs, 
   we issue it in CHANNEL_MAIN, and do not switch the current TAB.
   Aug 30

k) Choosing update files from Downloads/Waiting folder crashes.
   This is casue we no longer switch the TAB to a channel and the 
   text Entry processing function adds the line to History. There is no
   History for those tabs. Add a check.
   Aug 31

l) When in CHANNEL_CHAT and a !list is issued, it should be issued
   in CHANNEL_MAIN
   Aug 31

m) An update files on a nick is forced to CHANNEL_MAIN, if its issued from
   a non CHANNEL window, like Downloads/Waiting/FileSearch.
   In such scenarios we need to do pick the most apt channel.
   First check for nick in CHANEL_MAIN, if present, its apt.
   Next, loop thru NL, and check if Nick is present in any of them, skipping
   CHANNEL_MAIN and CHANNEL_CHAT in the process.
   Helper::getAptChannelForNickToIssueList(char *nick, char *apt_channel);
     apt_channel is allocated by caller.
  Aug 31

n) Say I have an oudated file listing, and server at that time has file in 
   Partial. By the time I request for the file, it has moved to Serving
   folder, or possibly moved to a subdir in Serving Folder, to categorise
   it nicely. Hence, instead of GETFROMPARTIAL, only trying MyPartialDB,
   if it cant find it there, we should check MyFilesDB on failure.
   Change in FileServer::fservGet() - Done.
   Sep 1

o) Make the nick to message, selected in Nick List - Done.
   Also a stray event is generated. When I double click on nick in channel
   windows, it first generates two onDC events. The first DC event is
   correct (window index/item). The second DC event is generated in the
   messages windows (with messages index and previous item as before).
   A third onNickSelectPM is also generated in the Messages window with the
   previous item as before - this is the one which changes the label.
   Looks like those could be generated from ID_MOUSE_RELEASE event, as
   a double click does generate to mouse releases - investigate
   onMouseRelease() - its not this one.
   OK, its like this. When we double click the sequence of events generated
   is as follows -> 
   - physically first click -> generate one Single Click Event
   - physically second click -> generate one Double Click Event
                             -> generate one Single Click Event.
   Now when we get the Double click event, we do the need ful, and change
   TAB to messages tab (which changes FocusIndex through onCmdPanel). Our
   function which handles the single click, comes in when FocusIndex has
   already changed, and goes by FocusIndex, rather than using the object
   passed. So, change onNickSelectPM() to use the object passed, rather
   than relying on FocusIndex.
   DOUBLE CLICK - SINGLE CLICK - BUG
   Sep 1

p) Left/Right arrow when in Channel to rotate through channels.
   TabBookWindow::onExpandTextEntry()
   Sep 1

q) recognize /list.
   NUMERIC 321 = Start of List
   Matrix.mo.us.ircsuper.net 321 Sur4802 Channel User: Name
   NUMERIC 322 = List entry.
   Matrix.mo.us.ircsuper.net 322 Sur4802 #whatever 21: Junk description
   NUMERIC 323 = End of List
   Matrix.mo.us.ircsuper.net 323 Sur4802 :End of /LIST
   It gives the details in the Messages tab.
   Sep 1

r) Response to a PORTCHECK, send NOTICE with colors. green for good, and
   red for bad.
   Sep 1

s) PROCESS the end of channel list messages list like:
   #Masala-Chat shyimp 1124821417
   #Masala-Chat :End of /NAMES list.
   So, that they can be removed from SERVER TAB.
   Added IC_TOPIC_END and IC_NAMES_END
   Sep 2

t) Do not have Key Left/Right to switch windows, as it is used in correcting
   text while typing. Use some other key combination like PgUp/PgDown.
   Sep 2

u) TCPConnect.readLine(), always terminate the line returned.
   Usually helpful when timeout reached and 0 returned. or partial line
   received and error condition hit.
   Sep 4

v) /cap, /fserv, /list in messages window need to pass * as second word, so
   that updateFXText() handles it appropriately. Check all of onTextEntry()
   so that its aptly set.
   Sep 4

w) When people are in our Q with files in our Partial folder and we restart
   our client, all those partial files in Q are lost.
   Sep 4

x) Change FClient and FClientInit to FClnt and FClntInit, so they are same
   size as FServ and FServInit.
   Sep 5

y) /list was sending message as if it was cominf from NickServ.
   It should just have second word as *
   Sep 5

z) Force UpdateCount to 0, on a trigger access to get listing of PropagatedNick
   Sep 5
   
a) When we join we are not adding ourselves to the NickList ? We are saved
   by that mistake as the IC_NICKLIST, gives a list which does include
   our nick. But, add it nonetheless on an IC_JOIN. 
   Note that IRCNickLists::addNick() doesnt update the mode, and just returns 
   if its an existing nick. So, we modify it to change the nickmode at 
   least if an entry already exists.
   Sep 6

b) File Server Queueing BUG. Sometimes it Queues people in earlier Qs.
   This is cause in IC_NICKLIST, we are not adding IC_NICKMODE_REGULAR
   when doing an addNick() call.
   Sep 6

c) File Server Another Queueing BUG. Possibility of getting Queued earlier
   out of priority can happen. Scenario: A ordinary user is getting a send,
   and lets say there are 5 Queues. This users send say gets terminated for
   whatever reason. So he is put in Q 2. Now say a priorty user tries to Q
   a file. He will see that ordinary user at Q 2, and hence get Queued in
   Q 1.
   So, when trying to get an apt Q to q ourselves in, we skip over Queues
   which have a ReSend count.
   Sep 6

d) Do not allow sending exe files.
   - TabBookWindow
   Do not allow receiving exe files.
   - DCCServerThr, DCCThr
   Skip putting files which contain ".exe", in MyfilesDB or MyPartialFilesDB
   - Helper
   Sep 6

e) On receiving a propagation trigger, we check if we have an already issued
   trigger to some nick for the same propagated nick. In that case we do not
   issue the propagation trigger.
   Added FilesDetailList::isPresentMatchingPropagatedNick()
   - ToTriggerThr
   Sep 6

f) Just like there is: FileServerWaiting -> FileServerInProgress, for
   people accessing our fileservers. There should be one
   FServPending -> FServInProgress. As its confusing lets rename it to
   FServClientPending -> FServClientInProgress.
   One part, is on exit, we can terminate the Connection as it will be
   piggy backed in the FD. Also, we will know the servers we are accessing
   currently, so that for PROPAGATION algo, we can check both
   FServClientPending and FServClientInProgress, if already we are
   in progress or about to be in progres with the prospective nick to
   which we are trying to propagate.
   - Done.

   We use isFilesOfNickPresent() to check if Nick's entry is present.
   That mainly goes by the FileName entry, and on NULL entries it
   returns false => not present. So we need to create a new one function
   called isNickPresent(), and this should be used instead as Trigger
   accesses have a NULL filename entry.
   - Done.
   
   Also add the FServClientPending/InProgress info in the File Server TAB.
   - Done.

   Have to pass DottedIP to DCCChatThr, so that it can correctly get nick
   even if its changed.
   - DCCServerThr (already does it), DCCThr - Done.
   Change DCCChatThr, so it looks up FDs via DottedIP.
   - Done.

   FromServerThr needs to feed in the hostname to toTrigger and ToTriggerNow
   so that they can set up the ip address in FServClientInProgress. Also,
   in TabBookWindow, same scenario holds.
   Modify the lines sent to ToTrigger/Now. Currently, its: Nick IRCLine.
   Change it to: HostName Nick IRCLine, so that it can populate IP fields
   in FD, before Queueing it in FServClientInProgress.
   Modify FServParse to understand this new format. Also pass dotted ip
   in the host field whenever possible to save on dns accesses.
   Make ToTriggerThr/ToTriggerNowThr update LongIP of Nick.
   FServParse, should convert ip to host for FSERVCTCP and PROPAGATION only.
   ignore for all rest.
   - Done.
   Sep 7

g) DCCChatClient (which does FServ access and CHATs), now monitors
   isIRC_QUIT(), and terminates the chat accordingly.
   Sep 7

h) After above update a Manual CTCP issued will not work. Ideally we
   should put DCC Chats which dont fit as an issued auto FServ in 
   a DCC Chat window. Hence we dont have to do anything in TabBookWindow
   except issue the CTCP. The incoming DCC CHAT wont have entries in 
   FServClientPending/InProgress and hence will make it in our new
   Queue called DCCChatInProgress.
   DCCChatThr should update DCCChatInProgress.
   - Done.
   TabBookWindow to have a "DCC Chat" window before Messages. The layout
   should consist of ScrollUI, Text Entry with label (like Messages TAB).
   - Done.
   The Server window should also have a Text Entry UI. Once that is done,
   we create TAB_DCC_CHAT just like a server window.
   - Done.
   change Label when new chat established - *DCC_CHAT_NICK* message
   inform in UI when CHAT gets over.
   - Done.
   DCCChatClient will do the readLine(), and TabBookWindow will do the
   writeData(). As DCCChatClient owns the connection, we will have a 
   XChange::lockDCCChat() and XChange::unlockDCCChat(), which will be
   used to make sure that the Connection object is not destroyed during
   the time that TabBookWindow attempts to write. DCCChatClient, will wait
   to acquire the lock, change the Connection which is piggied back in
   the FD to NULL, before it will exit out of the justChat() function.
   - Done.
   Sep 7

i) Add a command to initiate a DCC CHAT with a nick. UI and /dcc chat nick
   Sep 7

j) The CHAT interface is not fully complete. have to jot down the
   connection possibilities and sort it out.
   Scenarios when I initiate a chat through /dcc chat nick.
   - DCCThr gets a USERHOST response, check if no other is in DCCChatInProgress
     and if not, try to connect to ip:8124 as chat. If successful, add in
     DCCChatInProgress.
   - If unsuccessful, add entry in DCCChatPending, and issue a DCC CHAT
     (A DCCServerThr connect incoming, should look at DCCChat Pending, to
      identify that its an incoming CHAT request (non DCCServer) )
   - Typing /quit or /exit in the DCC CHAT window should end the CHAT.
   Sep 8

k) IC_PRIVMSG and IC_NOTICE, on getting a XDCC trigger, we forgot to
   pass hostname/dot ip as first word, as per the change to FServParse.
   Sep 9

l) Minor correction in Helper::getAptChannelForNickToIssueList()
   if Channel window and not Chat channel, then if nick is present, then
   issue it there.
   if couldnt pass test above, then see if nick is in CHANNEL_MAIN, and
   issue it there.
   if couldnt pass test above, go through all channels (other than MAIN and
   CHAT), and see if nick is present there, and issue it there.
   Sep 9

m) Add a Thread Name in StackTrace class. Will be more useful in the stack
   trace, as we will know which thread crashed.
   Sep 11

n) When a nick Quits Channel, we send a cancel to the transfer. Transfer,
   thinks its an imbalance one, and requeues it. Shouldnt requeue in such
   cases. Introduced CONNECTION_MESSAGE_DISCONNECT_ON_QUIT
   Sep 11

o) Tick the "Scroll Enable" as default.
   Sep 12

p) Do not allow a CTCP other than PING/TIME/VERSION/CLIENTINFO to our own
   nick. This is so that we dont issue our own trigger and as it will
   connect successfully, mark us as non firewalled.
   Sep 12.

q) When we fail to get the IP of a nick on USERHOST, there is no point in
   issuing a DCC CHAT, as when the client connects to us, it doesnt get
   matched in DCCChatPending, as DotIP is an empty string.
   Sep 12

r) When we requeue even cause of imbalance algorithm, we update RetryCount.
   This is so that thi scould be a normal nick and has come in Q from a
   Send slot. A FServ Access by a priviliged nick, could put the privileged
   nick ahead of this nick, inappropriately if it issues a get.
   Sep 14

s) Release Sep 14th version.
   cvs tag Sep14-2005
   Sep 14
