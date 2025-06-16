# Include debug symbols and disable optimization for easier stepping.
DEBUG = -g -O0

# Turn on almost all sensible warnings (including pedantic ISO-C).
SENSIBLE_W = -W -Wall -Wextra -Wpedantic
# Warn on implicit conversions that may change sign or width.
CONV_W = -Wconversion -Wsign-conversion
# Ensure all functions are properly prototyped, catch old-style (K&R) defs.
PROTO_W = -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition
#Catch shadowed variables, questionable pointer arithmetic, and alignment casts.
PTR_ALIGN_W = -Wshadow -Wpointer-arith #-Wcast-align
# At runtime, detect buffer overflows, use-after-free, integer overflows, invalid casts, etc.
MEM_W = -fsanitize=address,undefined
# Improves backtraces under ASan/UBSan.
BACKTRACE_W = -fno-omit-frame-pointer

CC = gcc
CFLAGS = -g $(SENSIBLE_W) $(MEM_W) $(PROTO_W) $(PTR_ALIGN_W) $(BACKTRACE_W) $(DEBUG) -Iinclude -Itest/include

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

