INCLUDE = -I/usr/local/include/fox-1.4 -I.

OBJECTS = FXTray.o FXEmbeddedWindow.o FXEmbedApp.o FXEmbedderWindow.o \
	FXEmbedTopWindow.o Icons.o MyTray.o

ICONS = TrayIcon.ico

CCC = g++ -g

all: ${OBJECTS} MyTray

Icons.hpp: $(ICONS)
	reswrap -i -o Icons.hpp $^

Icons.cpp: $(ICONS)
	reswrap -e -o Icons.cpp $^

Icons.o: Icons.hpp Icons.cpp
	${CCC} ${INCLUDE} -c Icons.cpp


MyTray: ${OBJECTS}
	${CCC} -mno-cygwin -mwindows -mconsole -L/usr/local/lib ${OBJECTS} -o MyTray -Wl,-R/usr/local/lib -lFOX-1.4

FXTray.o: FXTray.cpp FXTray.h
	${CCC} ${INCLUDE} -c FXTray.cpp

MyTray.o: MyTray.cpp
	${CCC} ${INCLUDE} -c MyTray.cpp

FXEmbeddedWindow.o: FXEmbeddedWindow.cpp FXEmbeddedWindow.h
	${CCC} ${INCLUDE} -c FXEmbeddedWindow.cpp

FXEmbedApp.o: FXEmbedApp.cpp FXEmbedApp.h
	${CCC} ${INCLUDE} -c FXEmbedApp.cpp

FXEmbedderWindow.o: FXEmbedderWindow.cpp FXEmbedderWindow.h
	${CCC} ${INCLUDE} -c FXEmbedderWindow.cpp

FXEmbedTopWindow.o: FXEmbedTopWindow.cpp FXEmbedTopWindow.h
	${CCC} ${INCLUDE} -c FXEmbedTopWindow.cpp

clean:
	rm -f ${OBJECTS} Icons.hpp Icons.cpp MyTray MyTray.exe
