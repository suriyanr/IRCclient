all: Swarm

INCLUDES = -I../src -I.

ifeq ($(BUILD), WINDOWS)
	LIBS = -lws2_32
else
	LIBS = -lpthread
endif

EXT_OBJECTS = ../src/LineParse.o ../src/LineQueue.o ../src/StackTrace.o ../src/HistoryLines.o ../src/SHA1.o ../src/Utilities.o

OBJECTS = SwarmNode.o Swarm.o LineQueueBW.o

Swarm: ${OBJECTS} ${EXT_OBJECTS}
	g++ $^ -o Swarm ${LIBS}
	#./FileSetup.sh

Swarm.o: Swarm.cpp Swarm.hpp
	g++ -g -c ${INCLUDES} Swarm.cpp

SwarmNode.o: SwarmNode.cpp Swarm.hpp
	g++ -g -c ${INCLUDES} SwarmNode.cpp

LineQueueBW.o: LineQueueBW.cpp LineQueueBW.hpp
	g++ -g -c ${INCLUDES} LineQueueBW.cpp

clean:
	rm -f ${OBJECTS} Swarm Swarm.exe Swarm.dat
	chmod +x FileSetup.sh
	./FileSetup.sh clean
