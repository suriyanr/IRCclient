ifeq ($(BUILD_MODE), 64)
   BIT_MODE=-m64
endif

ifeq ($(PRODUCTION), YES)
  CCC= g++ ${BIT_MODE} -O2 -fno-exceptions -fno-rtti
  UPX= upx
# No LeakTracer in Production build.
  DEBUG=StackTrace.o HistoryLines.o
else
  ifeq ($(PRODUCTION), PARTIAL)
     CCC= g++ ${BIT_MODE} -fno-exceptions -fno-rtti -DCOUT_OUTPUT
     DEBUG=StackTrace.o HistoryLines.o
  else
     CCC= g++ ${BIT_MODE} -g -fno-exceptions -fno-rtti -DCOUT_OUTPUT
     UPX=
# LeakTracer.o should come in the end of all objects so that its destructor
# is called last.
     DEBUG=StackTrace.o HistoryLines.o LeakTracer.o
  endif
endif

ifeq ($(SERVER), IRCSUPER)
	IRCSERVER = -DIRCSUPER
endif

ifeq ($(BUILD), WINDOWS)
	CCCFLAGS = -mno-cygwin -mwindows $(IRCSERVER)
	ifneq ($(PRODUCTION), YES)
            CCCFLAGS += -mconsole
        endif
	LIBS=$(DEBUG) -lws2_32
	FOXLIB= -L/usr/local/lib -lFOX-1.4
	STACKTRACETEST=StackTraceTest.exe
else
	STACKTRACELIB=-lpthread
	CCCFLAGS=$(IRCSERVER)
	LIBS= $(DEBUG) -lpthread
# -ljpeg and -ltiff were required in Fedora (or mebbe build FOX without it)
#	XLIBS= -L/usr/X11R6/lib -lX11 -lXext -lXrandr -ljpeg -ltiff -lpng
	XLIBS= -L/usr/X11R6/lib${BUILD_MODE} -lX11 -lXext -lXrandr -lXrender
	FOXLIB= -L/home/stealth/FOX/lib -L/usr/local/lib -lFOX-1.4
	STACKTRACETEST=StackTraceTest
endif

INCLUDES= -I/usr/local/include/fox-1.4 -I/usr/local/include -I/home/stealth/FOX/include/fox-1.4

ICONS = clear_gif.gif help_gif.gif portcheck_gif.gif servingdir_gif.gif \
	dccsend_gif.gif options_gif.gif portcheckme_gif.gif \
	font_gif.gif partialdir_gif.gif resource.gif \
	channeltextcurrent_gif.gif channeltextnew_gif.gif \
	TrayIcon.ico

OBJECTS= ConnectionMethod.o TCPConnect.o Base64.o IRCServerList.o \
	IRCChannelList.o LineQueue.o XChange.o IRCClient.o \
	Client.o LineParse.o IRCLineInterpret.o FilesDetailList.o \
	IRCNickLists.o FServParse.o SpamFilter.o \
	DCCServerThr.o ToTriggerThr.o ToServerThr.o ToServerNowThr.o \
	FromServerThr.o ToUIThr.o DCCThr.o FromUIThr.o DCCChatThr.o \
	DwnldInitThr.o FileServerThr.o TimerThr.o FileServer.o \
	TransferThr.o Transfer.o Helper.o SHA1.o SHA1File.o Upnp.o \
	UpnpThr.o ToTriggerNowThr.o DCCChatClient.o Utilities.o \
	SwarmDataPieceList.o SwarmNodeList.o SwarmStream.o SwarmThr.o

NONUIOBJECTS= UI.o

UIOBJECTS=HelpWindow.o with-UI.o Icons.o Help.o \
	MasalaMateAppUI.o ChilliIcon.o TabBookWindow.o \
	FXEmbedApp.o FXEmbeddedWindow.o FXEmbedderWindow.o FXEmbedTopWindow.o \
	FXTray.o

TEST_OBJECTS= ConnectionMethodTest.o TCPConnectTest.o TCPConnectClientTest.o \
	IRCServerListTest.o IRCChannelListTest.o LineQueueTest.o \
	Client.o LineParseTest.o IRCLineInterpretTest.o \
	IRCNickListsTest.o FServParseTest.o \
	SpamFilterTest.o HistoryLinesTest.o FilesDetailListTest.o \
	StackTraceTest.o SHA1FileTest.o UpnpTest.o TrayTest.o \
	UtilitiesTest.o

BINARIES= ConnectionMethodTest TCPConnectTest TCPConnectClientTest \
	IRCServerListTest IRCChannelListTest LineQueueTest \
	LineParseTest IRCLineInterpretTest IRCNickListsTest \
	Client UIClient FServParseTest \
	SpamFilterTest HistoryLinesTest FilesDetailListTest \
	StackTraceTest SHA1FileTest UpnpTest TrayTest UtilitiesTest

# LineParse now needs Utilties and SHA1 when linked into executables.
LINEPARSE_DEP= LineParse.o Utilities.o SHA1.o

ifeq ($(BUILD), APPLE)
  NM=nm -g -n
  STRIP=strip -u -r
else
  NM=nm --numeric-sort --defined-only --demangle
  STRIP=strip -s
endif



all: $(DEBUG) $(BINARIES) $(TEST_OBJECTS) $(OBJECTS) $(UIOBJECTS) $(NONUIOBJECTS) $(UPX)

upx: UIClient Client
ifeq ($(BUILD), WINDOWS)
	rm -f MasalaMate.exe MasalaMate.zip
	$(NM) UIClient.exe > MasalaMate.map
	$(STRIP) UIClient.exe
	upx --best --crp-ms=999999 --nrv2d UIClient.exe -o MasalaMate.exe
#	cp UIClient.exe MasalaMate.exe
#	zip MasalaMate.zip MasalaMate.exe
else
	rm -f MasalaMate MasalaMate.gz
	$(NM) UIClient > MasalaMate.map
	$(STRIP) UIClient
	cp UIClient MasalaMate
	gzip -9 MasalaMate
	rm -f MasalaMate
	~/bin/upx --best --crp-ms=999999 --nrv2d UIClient -o MasalaMate
endif

UIClient: $(OBJECTS) $(UIOBJECTS) $(DEBUG)
	$(CCC) $(CCCFLAGS) $(OBJECTS) $(UIOBJECTS) -o UIClient $(FOXLIB) $(XLIBS) $(LIBS)

Client: $(OBJECTS) $(NONUIOBJECTS)
	$(CCC) $(CCCFLAGS) $(OBJECTS) $(NONUIOBJECTS) -o Client $(FOXLIB) $(LIBS)

ifeq ($(BUILD), WINDOWS)
ChilliIcon.o: ChilliIcon.rc ChilliIcon.ico
	windres ChilliIcon.rc -o ChilliIcon.o
else
ChilliIcon.o:
	touch ChilliIcon-linux.cpp
	$(CCC) -c ChilliIcon-linux.cpp -o ChilliIcon.o
endif

Icons.hpp: $(ICONS)
	reswrap -i -o Icons.hpp $^

Icons.cpp: $(ICONS)
	reswrap -e -o Icons.cpp $^

Icons.o: Icons.hpp Icons.cpp
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c Icons.cpp

Utilities.o: Utilities.cpp Utilities.hpp
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c Utilities.cpp

SwarmStream.o: SwarmStream.cpp SwarmStream.hpp
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c SwarmStream.cpp

SwarmNodeList.o: SwarmNodeList.cpp SwarmNodeList.hpp
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c SwarmNodeList.cpp

SwarmDataPieceList.o: SwarmDataPieceList.cpp SwarmDataPieceList.hpp
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c SwarmDataPieceList.cpp

TrayTest.o: TrayTest.cpp Icons.o
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c TrayTest.cpp

FXTray.o: FXTray.cpp FXTray.h
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c FXTray.cpp

FXEmbeddedWindow.o: FXEmbeddedWindow.cpp FXEmbeddedWindow.h
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c FXEmbeddedWindow.cpp

FXEmbedApp.o: FXEmbedApp.cpp FXEmbedApp.h
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c FXEmbedApp.cpp

FXEmbedderWindow.o: FXEmbedderWindow.cpp FXEmbedderWindow.h
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c FXEmbedderWindow.cpp

FXEmbedTopWindow.o: FXEmbedTopWindow.cpp FXEmbedTopWindow.h
	${CCC} $(CCCFLAGS) $(INCLUDES) -c FXEmbedTopWindow.cpp

MasalaMateAppUI.o: MasalaMateAppUI.cpp MasalaMateAppUI.hpp TabBookWindow.hpp
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c MasalaMateAppUI.cpp

TabBookWindow.o: TabBookWindow.cpp TabBookWindow.hpp Icons.hpp Icons.cpp
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c TabBookWindow.cpp

Help.o: Help.cpp Help.hpp
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c Help.cpp

HelpWindow.o: HelpWindow.cpp HelpWindow.hpp
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c HelpWindow.cpp

Upnp.o: Upnp.cpp Upnp.hpp
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c Upnp.cpp

OptionsDialog.o: OptionsDialog.cpp OptionsDialog.hpp
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c OptionsDialog.cpp

UI.o: UI.cpp UI.hpp TabBookWindow.hpp
	$(CCC) $(CCCFLAGS) $(INCLUDES) -c UI.cpp

with-UI.o: UI.cpp UI.hpp TabBookWindow.hpp
	$(CCC) $(CCCFLAGS) -DUSER_INTERFACE $(INCLUDES) -c UI.cpp -o with-UI.o

UtilitiesTest: UtilitiesTest.o Utilities.o SHA1.o
	$(CCC) $(CCCFLAGS) $^ -o UtilitiesTest $(LIBS)

UtilitiesTest.o: UtilitiesTest.cpp Utilities.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c UtilitiesTest.cpp

IRCLineInterpretTest: IRCLineInterpretTest.o IRCLineInterpret.o $(LINEPARSE_DEP)
	$(CCC) $(CCCFLAGS) $^ -o IRCLineInterpretTest $(LIBS)

IRCLineInterpretTest.o: IRCLineInterpretTest.cpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c IRCLineInterpretTest.cpp

FilesDetailListTest: FilesDetailListTest.o FilesDetailList.o TCPConnect.o ConnectionMethod.o Base64.o $(LINEPARSE_DEP)
	$(CCC) $(CCCFLAGS) $^ -o FilesDetailListTest $(LIBS)

FilesDetailListTest.o: FilesDetailListTest.cpp FilesDetailList.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c FilesDetailListTest.cpp

StackTrace.o: StackTrace.cpp StackTrace.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c StackTrace.cpp

StackTraceTest.o: StackTraceTest.cpp StackTrace.o
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c StackTraceTest.cpp

StackTraceTest: StackTraceTest.o $(LINEPARSE_DEP) StackTrace.o HistoryLines.o
	$(CCC) $(CCCFLAGS) StackTraceTest.o $(LINEPARSE_DEP) -o StackTraceTest $(STACKTRACELIB) $(LIBS)
	$(NM) $(STACKTRACETEST) > StackTraceTest.map
#	strip -s $(STACKTRACETEST)

HistoryLinesTest: HistoryLinesTest.o HistoryLines.o LeakTracer.o
	$(CCC) $(CCCFLAGS) $^ -o HistoryLinesTest

HistoryLinesTest.o: HistoryLinesTest.cpp HistoryLines.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c HistoryLinesTest.cpp

HistoryLines.o: HistoryLines.cpp HistoryLines.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c HistoryLines.cpp

FilesDetailList.o: FilesDetailList.cpp FilesDetailList.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c FilesDetailList.cpp

IRCNickListsTest: IRCNickListsTest.o IRCNickLists.o
	$(CCC) $(CCCFLAGS) IRCNickListsTest.o IRCNickLists.o -o IRCNickListsTest $(LIBS)

IRCNickListsTest.o: IRCNickListsTest.cpp IRCNickLists.hpp IRCNickLists.cpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c IRCNickListsTest.cpp

IRCNickLists.o: IRCNickLists.cpp IRCNickLists.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c IRCNickLists.cpp

IRCLineInterpret.o: IRCLineInterpret.cpp IRCLineInterpret.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c IRCLineInterpret.cpp

LineParseTest: LineParseTest.o $(LINEPARSE_DEP)
	$(CCC) $(CCCFLAGS) $^ -o LineParseTest $(LIBS)

LineParseTest.o: LineParseTest.cpp LineParse.o
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c LineParseTest.cpp

LineParse.o: LineParse.cpp LineParse.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c LineParse.cpp

Client.o: Client.cpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c Client.cpp

Helper.o: Helper.cpp Helper.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c Helper.cpp

FileServer.o: FileServer.cpp FileServer.hpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c FileServer.cpp

Transfer.o: Transfer.cpp Transfer.hpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c Transfer.cpp

DCCServerThr.o: DCCServerThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c DCCServerThr.cpp

ToTriggerThr.o: ToTriggerThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c ToTriggerThr.cpp

ToTriggerNowThr.o: ToTriggerNowThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c ToTriggerNowThr.cpp

ToServerThr.o: ToServerThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c ToServerThr.cpp

ToServerNowThr.o: ToServerNowThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c ToServerNowThr.cpp

FromServerThr.o: FromServerThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c FromServerThr.cpp

ToUIThr.o: ToUIThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c ToUIThr.cpp

DCCThr.o: DCCThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c DCCThr.cpp

FromUIThr.o: FromUIThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c FromUIThr.cpp

DCCChatThr.o: DCCChatThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c DCCChatThr.cpp

DCCChatClient.o: DCCChatClient.cpp DCCChatClient.hpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c DCCChatClient.cpp

DwnldInitThr.o: DwnldInitThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c DwnldInitThr.cpp

FileServerThr.o: FileServerThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c FileServerThr.cpp

TransferThr.o: TransferThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c TransferThr.cpp

TimerThr.o: TimerThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c TimerThr.cpp

UpnpThr.o: UpnpThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c UpnpThr.cpp

SwarmThr.o: SwarmThr.cpp ThreadMain.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c SwarmThr.cpp

IRCClient.o: IRCClient.cpp IRCClient.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c IRCClient.cpp

XChange.o: XChange.cpp XChange.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c $(INCLUDES) XChange.cpp

LeakTracer.o: LeakTracer.cpp LeakTracer.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c LeakTracer.cpp

FServParse.o: FServParse.cpp FServParse.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c FServParse.cpp

FServParseTest: FServParseTest.o FServParse.o $(LINEPARSE_DEP) TCPConnect.o ConnectionMethod.o Base64.o
	$(CCC) $(CCCFLAGS) $^ -o FServParseTest $(LIBS)

FServParseTest.o: FServParseTest.cpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c FServParseTest.cpp

SpamFilterTest: SpamFilterTest.o SpamFilter.o $(LINEPARSE_DEP)
	$(CCC) $(CCCFLAGS) $^ -o SpamFilterTest $(LIBS)

SpamFilterTest.o: SpamFilterTest.cpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c SpamFilterTest.cpp

SpamFilter.o: SpamFilter.cpp SpamFilter.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c SpamFilter.cpp

LineQueueTest: LineQueueTest.o LineQueue.o
	$(CCC) $(CCCFLAGS) LineQueueTest.o LineQueue.o -o LineQueueTest $(LIBS)

LineQueueTest.o: LineQueueTest.cpp LineQueue.hpp LineQueue.cpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c LineQueueTest.cpp

LineQueue.o: LineQueue.cpp LineQueue.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c LineQueue.cpp

IRCChannelListTest: IRCChannelListTest.o IRCChannelList.o
	$(CCC) $(CCCFLAGS) IRCChannelListTest.o IRCChannelList.o -o IRCChannelListTest $(LIBS)

IRCChannelListTest.o: IRCChannelListTest.cpp IRCChannelList.hpp IRCChannelList.cpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c IRCChannelListTest.cpp

IRCChannelList.o: IRCChannelList.cpp IRCChannelList.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c IRCChannelList.cpp

IRCServerListTest: IRCServerListTest.o IRCServerList.o
	$(CCC) $(CCCFLAGS) IRCServerListTest.o IRCServerList.o -o IRCServerListTest $(LIBS)

IRCServerListTest.o: IRCServerListTest.cpp IRCServerList.hpp IRCServerList.cpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c IRCServerListTest.cpp

IRCServerList.o: IRCServerList.cpp IRCServerList.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c IRCServerList.cpp

Base64.o: Base64.cpp Base64.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c Base64.cpp

SHA1.o: SHA1.cpp SHA1.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c SHA1.cpp

SHA1File.o: SHA1File.cpp SHA1File.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c SHA1File.cpp

SHA1FileTest.o: SHA1FileTest.cpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c SHA1FileTest.cpp

SHA1FileTest: SHA1FileTest.o SHA1File.o $(LINEPARSE_DEP)
	$(CCC) $(CCCFLAGS) $^ -o SHA1FileTest $(LIBS)

UpnpTest.o: UpnpTest.cpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c UpnpTest.cpp

UpnpTest: UpnpTest.o Upnp.o TCPConnect.o ConnectionMethod.o Base64.o $(LINEPARSE_DEP)
	$(CCC) $(CCCFLAGS) $^ -o UpnpTest $(LIBS)

TCPConnect.o: TCPConnect.cpp TCPConnect.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c TCPConnect.cpp

TCPConnectTest.o: TCPConnectTest.cpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c TCPConnectTest.cpp

TCPConnectTest: TCPConnectTest.o TCPConnect.o ConnectionMethod.o Base64.o
	$(CCC) $(CCCFLAGS) TCPConnectTest.o TCPConnect.o ConnectionMethod.o Base64.o -o TCPConnectTest $(LIBS)

TCPConnectClientTest: TCPConnectClientTest.o TCPConnect.o ConnectionMethod.o Base64.o
	$(CCC) $(CCCFLAGS) TCPConnectClientTest.o TCPConnect.o ConnectionMethod.o Base64.o -o TCPConnectClientTest $(LIBS)

TCPConnectClientTest.o: TCPConnectClientTest.cpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c TCPConnectClientTest.cpp

ConnectionMethod.o: ConnectionMethod.cpp ConnectionMethod.hpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c ConnectionMethod.cpp

ConnectionMethodTest.o: ConnectionMethodTest.cpp
	$(CCC) $(INCLUDES) $(CCCFLAGS) -c ConnectionMethodTest.cpp

ConnectionMethodTest: ConnectionMethodTest.o ConnectionMethod.o
	$(CCC) $(CCCFLAGS) ConnectionMethodTest.o ConnectionMethod.o -o ConnectionMethodTest $(LIBS)

TrayTest: TrayTest.o FXTray.o FXEmbeddedWindow.o FXEmbedApp.o FXEmbedderWindow.o FXEmbedTopWindow.o Icons.o
	$(CCC) $(CCCFLAGS) $^ -o TrayTest $(FOXLIB) $(XLIBS) $(LIBS)

clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(BINARIES) $(UIOBJECTS) $(NONUIOBJECTS) icon-linux.cpp $(DEBUG) LeakTracer.o 
	rm -f *.o
	rm -f Icons.cpp Icons.hpp
	rm -f UIClient.exe.bak
	rm -f MasalaMate2b MasalaMate2d
	rm -f MasalaMate.memory MasalaMate.*.trc
	rm -f MasalaMate.gz MasalaMate
	rm -f ChilliIcon-linux.cpp
	mv upx.exe upx.exe.org
	rm -f *.exe
	mv upx.exe.org upx.exe
	rm -f MasalaMate.map StackTraceTest.map
	rm -f MasalaMate.zip
	(cd Upgrade; rm -f MasalaMate*; cd ..)

check: all
	./ConnectionMethodTest
#	./TCPConnectTest
