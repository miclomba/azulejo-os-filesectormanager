# Make file for File Sector Manager

fsm : fsm.o
	gcc -g -W -Wall -o fsm -lm fsm.o

fsm.o : fsm.c fileSectorMgr.h fsmDefinitions.h iNode.h sectorSpaceMgr.h ssmDefinitions.h gDefinitions.h
	gcc -g -W -Wall -c fsm.c

clean:
	rm -f fsm fsm.o
