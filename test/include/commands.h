/*******************************************************************************
 * Filesystem Commands
 * Author: Michael Lombardi
 *******************************************************************************/
#ifndef COMMANDS_H
#define COMMANDS_H

#include "fsm.h"
#include "global_constants.h"

/**
 * @brief Initializes the file system.
 * Parses command-line arguments and sets up the file system accordingly.
 * @param _argc Number of command-line arguments.
 * @param _argv Array of argument strings.
 * @param input Input buffer for commands (max 10,000 characters).
 * @param fsm Pointer to the FSM instance.
 * @param i Current index in the input buffer.
 * @return Updated input buffer index.
 */
int init_command(int _argc, char** _argv, char* input, FSM* fsm, int i);

/**
 * @brief Finalizes the file system and releases resources.
 * @param fsm Pointer to the FSM instance.
 * @return void
 */
void end_command(FSM* fsm);

/**
 * @brief Prints system metadata and file system statistics.
 * @param input Input buffer (max 10,000 characters).
 * @param fsm Pointer to the FSM instance.
 * @param i Current index in the input buffer.
 * @return Updated input buffer index.
 */
int info_command(char* input, FSM* fsm, int i);

/**
 * @brief Prints data blocks or file contents.
 * @param input Input buffer (max 10,000 characters).
 * @param fsm Pointer to the FSM instance.
 * @param i Current index in the input buffer.
 * @return Updated input buffer index.
 */
int print_command(char* input, FSM* fsm, int i);

/**
 * @brief Creates a new file or directory.
 * @param input Input buffer (max 10,000 characters).
 * @param fsm Pointer to the FSM instance.
 * @param name Buffer used to store or assign a file name.
 * @param i Current index in the input buffer.
 * @return Updated input buffer index.
 */
int create_command(char* input, FSM* fsm, char* name, int i);

/**
 * @brief Renames an existing file or directory.
 * @param input Input buffer (max 10,000 characters).
 * @param fsm Pointer to the FSM instance.
 * @param name Buffer containing the new name.
 * @param i Current index in the input buffer.
 * @return Updated input buffer index.
 */
int rename_command(char* input, FSM* fsm, char* name, int i);

/**
 * @brief Writes data to a file.
 * @param input Input buffer (max 10,000 characters).
 * @param fsm Pointer to the FSM instance.
 * @param i Current index in the input buffer.
 * @return Updated input buffer index.
 */
int write_command(char* input, FSM* fsm, int i);

/**
 * @brief Reads data from a file.
 * @param input Input buffer (max 10,000 characters).
 * @param fsm Pointer to the FSM instance.
 * @param i Current index in the input buffer.
 * @return Updated input buffer index.
 */
int read_command(char* input, FSM* fsm, int i);

/**
 * @brief Removes a file or directory.
 * @param input Input buffer (max 10,000 characters).
 * @param fsm Pointer to the FSM instance.
 * @param i Current index in the input buffer.
 * @return Updated input buffer index.
 */
int remove_command(char* input, FSM* fsm, int i);

/**
 * @brief Performs a test removal of a file or directory.
 * Used for development or validation purposes.
 * @param input Input buffer (max 10,000 characters).
 * @param fsm Pointer to the FSM instance.
 * @param i Current index in the input buffer.
 * @return Updated input buffer index.
 */
int remove_test_command(char* input, FSM* fsm, int i);

/**
 * @brief Lists files and directories within a directory.
 * @param input Input buffer (max 10,000 characters).
 * @param fsm Pointer to the FSM instance.
 * @param name Buffer to receive or filter by file name.
 * @param i Current index in the input buffer.
 * @return Updated input buffer index.
 */
int list_command(char* input, FSM* fsm, char* name, int i);

/**
 * @brief Handles unrecognized or default commands.
 * Fallback mechanism for unknown input.
 * @param input Input buffer (max 10,000 characters).
 * @param fsm Pointer to the FSM instance.
 * @param i Current index in the input buffer.
 * @return Updated input buffer index.
 */
int default_command(char* input, FSM* fsm, int i);

#endif  // COMMANDS_H