/*******************************************************************************
 * Filesystem Commands
 * Author: Michael Lombardi
 *******************************************************************************/
#ifndef COMMANDS_H
#define COMMANDS_H

#include "fileSectorMgr.h"
#include "gDefinitions.h"

/*
 * @brief A filesystem init command
 * @param _argc the number of arguments passed to program
 # @param _argv an array holding the arguments
 * @param input buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param i the input buffer current index
 * @return an updated input buffer index
 */
int init_command(int _argc, char** _argv, char* input, FileSectorMgr* fsm, int i);

/*
 * @brief A filesystem end command
 * @param fsm The FileSectorMgr reference
 * @return an updated input buffer index
 */
void end_command(FileSectorMgr* fsm);

/*
 * @brief A filesystem info command
 *
 * @param input buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param i the input buffer current index
 * @return an updated input buffer index
 */
int info_command(char* input, FileSectorMgr* fsm, int i);

/*
 * @brief A filesystem print command
 *
 * @param input buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param i the input buffer current index
 * @return an updated input buffer index
 */
int print_command(char* input, FileSectorMgr* fsm, int i);

/*
 * @brief A filesystem create command
 *
 * @param input buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param name buffer used when renaming files
 * @param i the input buffer current index
 * @return an updated input buffer index
 */
int create_command(char* input, FileSectorMgr* fsm, unsigned int* name, int i);

/*
 * @brief A filesystem rename command
 *
 * @param input buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param name buffer used when renaming files
 * @param i the input buffer current index
 * @return an updated input buffer index
 */
int rename_command(char* input, FileSectorMgr* fsm, unsigned int* name, int i);

/*
 * @brief A filesystem write command
 *
 * @param input buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param i the input buffer current index
 * @return an updated input buffer index
 */
int write_command(char* input, FileSectorMgr* fsm, int i);

/*
 * @brief A filesystem read command
 *
 * @param input buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param i the input buffer current index
 * @return an updated input buffer index
 */
int read_command(char* input, FileSectorMgr* fsm, int i);

/*
 * @brief A filesystem remove command
 *
 * @param input buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param i the input buffer current index
 * @return an updated input buffer index
 */
int remove_command(char* input, FileSectorMgr* fsm, int i);

/*
 * @brief A filesystem remove test command
 *
 * @param input buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param i the input buffer current index
 * @return an updated input buffer index
 */
int remove_test_command(char* input, FileSectorMgr* fsm, int i);

/*
 * @brief A filesystem list command
 *
 * @param input buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param name buffer used when renaming files
 * @param i the input buffer current index
 * @return an updated input buffer index
 */
int list_command(char* input, FileSectorMgr* fsm, unsigned int* name, int i);

/*
 * @brief A filesystem default command
 *
 * @param input buffer for input read, has a maximum input of 10,000 characters
 * @param fsm The FileSectorMgr reference
 * @param i the input buffer current index
 * @return an updated input buffer index
 */
int default_command(char* input, FileSectorMgr* fsm, int i);

#endif  // COMMANDS_H