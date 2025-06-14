#ifndef COMMANDS_H
#define COMMANDS_H

#include "fileSectorMgr.h"
#include "gDefinitions.h"

int advance_to_char(char* buffer, char c, int i);

/*
 * @brief A filesystem init command
 * @param _argc the number of arguments passed to program
 # @param _argv an array holding the arguments
 * @param driver buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param digit placeholder for any conversions from character to integer
 * @param i the driver buffer current index
 * @return an updated driver buffer index
 */
int init_command(int _argc, char** _argv, char* driver, FileSectorMgr* fsm, int* digit, int i);

/*
 * @brief A filesystem end command
 * @param fsm The FileSectorMgr reference
 * @param loop the driver input loop status
 * @return an updated driver buffer index
 */
void end_command(FileSectorMgr* fsm, Bool* loop);

/*
 * @brief A filesystem info command
 *
 * @param driver buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param success boolean for use with file handles
 * @param inodeNumF buffer for holding an iNode number dealing with files
 * @param digit placeholder for any conversions from character to integer
 * @param i the driver buffer current index
 * @return an updated driver buffer index
 */
int info_command(char* driver, FileSectorMgr* fsm, Bool* success, unsigned int* inodeNumF,
                 int* digit, int i);

/*
 * @brief A filesystem print command
 *
 * @param driver buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param digit placeholder for any conversions from character to integer
 * @param i the driver buffer current index
 * @return an updated driver buffer index
 */
int print_command(char* driver, FileSectorMgr* fsm, int* digit, int i);

/*
 * @brief A filesystem create command
 *
 * @param driver buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param name buffer used when renaming files
 * @param inodeNumD buffer for holding an iNode number dealing with directories
 * @param inodeNumF buffer for holding an iNode number dealing with files
 * @param digit placeholder for any conversions from character to integer
 * @param i the driver buffer current index
 * @return an updated driver buffer index
 */
int create_command(char* driver, FileSectorMgr* fsm, unsigned int* name, unsigned int* inodeNumD,
                   unsigned int* inodeNumF, int* digit, int i);

/*
 * @brief A filesystem rename command
 *
 * @param driver buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param name buffer used when renaming files
 * @param inodeNumD buffer for holding an iNode number dealing with directories
 * @param inodeNumF buffer for holding an iNode number dealing with files
 * @param digit placeholder for any conversions from character to integer
 * @param i the driver buffer current index
 * @return an updated driver buffer index
 */
int rename_command(char* driver, FileSectorMgr* fsm, unsigned int* name, unsigned int* inodeNumD,
                   unsigned int* inodeNumF, int* digit, int i);

/*
 * @brief A filesystem write command
 *
 * @param driver buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param buffer for holding block information
 * @param success boolean for use with file handles
 * @param inodeNumF buffer for holding an iNode number dealing with files
 * @param digit placeholder for any conversions from character to integer
 * @param i the driver buffer current index
 * @return an updated driver buffer index
 */
int write_command(char* driver, FileSectorMgr* fsm, unsigned int* buffer, Bool* success,
                  unsigned int* inodeNumF, int* digit, int i);

/*
 * @brief A filesystem read command
 *
 * @param driver buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param buffer for holding block information
 * @param success boolean for use with file handles
 * @param inodeNumF buffer for holding an iNode number dealing with files
 * @param digit placeholder for any conversions from character to integer
 * @param i the driver buffer current index
 * @return an updated driver buffer index
 */
int read_command(char* driver, FileSectorMgr* fsm, unsigned int* buffer, Bool* success,
                 unsigned int* inodeNumF, int* digit, int i);

/*
 * @brief A filesystem remove command
 *
 * @param driver buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param success boolean for use with file handles
 * @param inodeNumD buffer for holding an iNode number dealing with directories
 * @param inodeNumF buffer for holding an iNode number dealing with files
 * @param digit placeholder for any conversions from character to integer
 * @param i the driver buffer current index
 * @return an updated driver buffer index
 */
int remove_command(char* driver, FileSectorMgr* fsm, Bool* success, unsigned int* inodeNumD,
                   unsigned int* inodeNumF, int* digit, int i);

/*
 * @brief A filesystem remove test command
 *
 * @param driver buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param success boolean for use with file handles
 * @param digit placeholder for any conversions from character to integer
 * @param i the driver buffer current index
 * @return an updated driver buffer index
 */
int remove_test_command(char* driver, FileSectorMgr* fsm, Bool* success, int* digit, int i);

/*
 * @brief A filesystem list command
 *
 * @param driver buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param buffer for holding block information
 * @param name buffer used when renaming files
 * @param success boolean for use with file handles
 * @param inodeNumF buffer for holding an iNode number dealing with files
 * @param digit placeholder for any conversions from character to integer
 * @param i the driver buffer current index
 * @return an updated driver buffer index
 */
int list_command(char* driver, FileSectorMgr* fsm, unsigned int* buffer, unsigned int* name,
                 Bool* success, unsigned int* inodeNumF, int* digit, int i);

#endif  // COMMANDS_H