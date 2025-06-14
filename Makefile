CC = gcc
CFLAGS = -g -W -Wall -Iinclude -Itest/include

OBJ = test/main.o test/src/commands.o src/fileSectorMgr.o src/fsmDefinitions.o src/iNode.o src/sectorSpaceMgr.o src/logger.o

test/fsm: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) -lm

test/main.o: test/main.c include/ssmDefinitions.h include/gDefinitions.h include/config.h
	$(CC) $(CFLAGS) -c test/main.c -o $@

test/src/commands.o: test/src/commands.c test/include/commands.h
	$(CC) $(CFLAGS) -c test/src/commands.c -o $@

src/fileSectorMgr.o: src/fileSectorMgr.c include/gDefinitions.h include/config.h
	$(CC) $(CFLAGS) -c src/fileSectorMgr.c -o $@

src/fsmDefinitions.o: src/fsmDefinitions.c include/fsmDefinitions.h include/config.h
	$(CC) $(CFLAGS) -c src/fsmDefinitions.c -o $@

src/sectorSpaceMgr.o: src/sectorSpaceMgr.c include/sectorSpaceMgr.h include/gDefinitions.h include/ssmDefinitions.h include/config.h
	$(CC) $(CFLAGS) -c src/sectorSpaceMgr.c -o $@

src/iNode.o: src/iNode.c include/iNode.h include/gDefinitions.h include/ssmDefinitions.h include/config.h
	$(CC) $(CFLAGS) -c src/iNode.c -o $@

src/logger.o: src/logger.c include/logger.h include/gDefinitions.h include/ssmDefinitions.h include/config.h include/sectorSpaceMgr.h
	$(CC) $(CFLAGS) -c src/logger.c -o $@

clean:
	rm -f test/fsm *.o src/*.o test/*.o test/qaOutput.txt

touch_data:
	mkdir -p fs
	touch fs/aMap fs/fMap fs/iMap fs/hardDisk

