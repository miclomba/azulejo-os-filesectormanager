# Make file for File Sector Manager

fsm : main.o
	gcc -g -W -Wall -o fsm -lm main.o

main.o : main.c fileSectorMgr.h fsmDefinitions.h iNode.h sectorSpaceMgr.h ssmDefinitions.h gDefinitions.h
	gcc -g -W -Wall -c main.c

clean:
	rm -f fsm main.o
