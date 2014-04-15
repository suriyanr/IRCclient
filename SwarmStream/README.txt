To setup the files, run: ./FileSetup.sh

This is where we simulate our Swarm Stream protocol.

We create Threads, each representing a Node in the Swarm. Each thread is
spawned with a parameter structure, which identifies its characteristics.
The characteristics currently thought of are:
- Nick
- FileName
- FileSize of FileName
- Max Upload in BPS
- Max Download in BPS
- Firewall State

All the threads have access to RecvMessageQueue[]. This consists of an array
of LineQueue class.
The Threads receive messages from other threads and process it, as part
of the swarm protocol.

For simulation we add a ThreadID which identifies the node. In real life,
it will be the IP address/Nick. We pass the ThreadID too in all the messages
in the sumulation model.

Handshake:
  HS ThrID FWState FileName FileSize SHA

Response:
  AC YS ThrID FWState FileName FileSize
  AC NO ThrID

Am able to get the swarm formed with connected nodes.

Now, will substitute real files and SHA. Each Thread's file resides in
ThrID directory.

Created FileSetup.sh for generating the directories and Files within them.
It should also take a Swarm.avi and cut it with random lengths for a later
simulation in the directories. Swarm.avi will be a file with no byte which is
'\0', as we are currently doing a line based Messaging for simulation.

SwarmNode will be passed directory, and FileName. It should get the
FileSize by stat etc.

OK all works fine till now, as far as the handshake is concerned.
As its so tightly dependent on BW, and we use LineQueue class for message
passing, we need to make a class derived from LineQueue, which supplies
messages and sends messages back taking into consideration its upload
and download BW.
So, I expect a addLine() call to take as much duration as the length of data
that is sent, and the Upload BW that it is set to.
Similarly, a getLineAndDelete() call should take as much duration as the
amount of data that is requested, and the Download BW that it is set to.
It should also have a max buffer, which it can accept for upload.
If the messages sent are backlogged more than this value, it waits till
some buffer is free, before it queues up the data.
Dont seem to need it for Download. Default is 16K buffer.
Class call it LineQueueBW

LineQueueBW is also done.

Now for implementing the data piece request part of the Swarm protocol.
We only request Data Pieces from Nodes which have FileSize greater than
what we have. Here the FileSize is referred to the size on disk => we have
every byte before that offset.

For every node which has FS greater than the size we currently need, we send
a DP request to get a slice that we need. So we have a list which has the 
information of all Nodes.
This might include: NickID, FileName, FileSize, State.
I think State is required to see the last think that happened with this node.
Example, DR_SENT, IDLE

Data Piece Request Message. (FileSize is senders FileSize)
DR ThrID FileOffset FileSize FileName

Data Piece Response Message. (FileSize is senders FileSize)
DP ThrID DataLength FileSize
Data of max 8K

End Game handling to get the last piece which is < 8192 bytes.
--------------------------------------------------------------
Once we find that we cannot send out any request to get new pieces, that is
an indication that we are possibly the server node or need < 8K bytes.
Also we have no pending requests for Data.
When we have reached this state, we go thru the list of connected nodes, and
find the node which has file sizes more than us. We then pick one random node
out of these, and request the piece.

Now for the FS propagation to all equals.
-----------------------------------------
FileSize should be by default exchanged between sender and receiver, so that
it gets updated in the DR/DP interchange. Note that, for a sending node,
the file size that it notes as part of a DR message it receives, will have
filesizes at the point when they issued the DR. (hence will miss the data
that the server is going to send now)

Once we increase our FileSize, we should notify all our equals about our
change in filesize.
FS ThrID FileSize FileName

With above two, all have about a nice FileSize update in place, which can
be utilised to get the apt data pieces.

Problem in simulation as DP is split in two lines. As someone can add to
the Queue, between the the two, breaking the sequence.
Hence combinie it as one:
DP ThrID DataLength FileSize
Data of max 8K
becomes:
DP ThrID DataLength FileSize <Data of DataLength>

Now its all working fine. Lets get some data, like how much bytes went thru
each node etc. Modification in LineQueueBW class.

To send a message to j, we do a putLine on [j].
Hence putLine adds to data coming into Thread j.
Thread i, does getLineAndDelete on [i].
Hence its quite meaningless in LineQueueBW. We need to add fields in
ConnectedNodesInfo.

OK, cursory with nodes 1 to 10 looks good.
The node having the most FileSize, has 0 Bytes In. (1 node)
The nodes having the least FileSize, have 0 Bytes Out. (a few)
Which makes me think if bytes are at all exchanged between like sized 
file sizes.
As per logic, only end pieces are exchanged with like sized file sizes, which
could be the reason why they arent exchanging amongst themselves. Also, if
we start exchanging delta file pieces < 8192 from like file size nodes, then
it might get complicated in aligning the already sent out requests?

example: A (50K) B(25K) C(20K) D(100K)
So B will ask from A and D, and C will ask from A and D too. If B was allowed
to ask C and vice versa, then it would get a piece < chunk size. And this
will create holes in the Request chunk Q. reason being, B would have asked A
for 20K, and D for 28K. If C was allowed to ask B, then it would have asked
B for 20K, A for 28K, C for 36K. If B returned a non chunk data size, we 
will have a hole in that region, till it gets filled.

