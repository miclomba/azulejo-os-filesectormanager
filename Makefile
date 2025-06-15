MEM = -O1 -fsanitize=address,undefined
CC = gcc
CFLAGS = -g -W -Wall -Iinclude -Itest/include

OBJ = test/main.o test/src/commands.o test/src/utils.o src/fsm.o src/fsm_constants.o src/inode.o src/ssm.o src/logger.o

test/fsm: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) -lm

test/main.o: test/main.c include/ssm_constants.h include/global_constants.h include/config.h test/include/test_config.h
	$(CC) $(CFLAGS) -c test/main.c -o $@

test/src/commands.o: test/src/commands.c test/include/commands.h
	$(CC) $(CFLAGS) -c test/src/commands.c -o $@

test/src/utils.o: test/src/utils.c test/include/utils.h
	$(CC) $(CFLAGS) -c test/src/utils.c -o $@

src/fsm.o: src/fsm.c include/global_constants.h include/config.h
	$(CC) $(CFLAGS) -c src/fsm.c -o $@

src/fsm_constants.o: src/fsm_constants.c include/fsm_constants.h include/config.h
	$(CC) $(CFLAGS) -c src/fsm_constants.c -o $@

src/ssm.o: src/ssm.c include/ssm.h include/global_constants.h include/ssm_constants.h include/config.h
	$(CC) $(CFLAGS) -c src/ssm.c -o $@

src/inode.o: src/inode.c include/inode.h include/global_constants.h include/ssm_constants.h include/config.h
	$(CC) $(CFLAGS) -c src/inode.c -o $@

src/logger.o: src/logger.c include/logger.h include/global_constants.h include/ssm_constants.h include/config.h include/ssm.h
	$(CC) $(CFLAGS) -c src/logger.c -o $@

clean:
	rm -f test/fsm *.o src/*.o test/*.o test/src/*.o test/qaOutput.txt

touch_data:
	mkdir -p fs
	touch fs/aMap fs/fMap fs/iMap fs/hardDisk

