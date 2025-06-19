/*********************************
 *	Logger
 *	Author: Michael Lombardi
 **********************************/
#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#include "config.h"

/*
 * @brief FSM logger values
 */
typedef enum LoggerFSMOption {
    FSM_INIT_MAPS = 0,
    FSM_INODE_ARRAY = 1,
    FSM_ALLOC_INODES = 2,
    FSM_INIT = 3,
    FSM_ALLOC_FAIL = 4,
    FSM_DEALLOC_INODES = 6,
    FSM_DEALLOC_FAIL = 7,
    FSM_INTEG_FAIL = 8,
    FSM_INTEG_PASS = 9,
    FSM_INTEG_CHECK = 10,
    FSM_INODE_SET = 12,
    FSM_END = 14,
    FSM_INVALID_INPUT = 15,
    FSM_INODE_NOT_FOUND = 16,
    FSM_INPUT = 17,
    FSM_OUTPUT_STUB = 18,
    FSM_INFO = 19,
    FSM_INODE_GET = 20,
    FSM_MAKE = 21,
    FSM_FILE_CREATE = 22,
    FSM_FILE_OPEN = 23,
    FSM_FILE_WRITE = 24,
    FSM_FILE_READ = 25,
    FSM_FILE_MKDIR = 26,
    FSM_FILE_ROOT_TEST = 27,
    FSM_FILE_ROOT_LS = 28,
    FSM_INODE_PRINT = 29
} LoggerFSMOption;

/*
 * @brief SSM logger values
 */
typedef enum LoggerSSMOption {
    SSM_INIT_MAPS = 0,
    SSM_MAPS_PRINT = 1,
    SSM_MAPS_ALLOC = 2,
    SSM_INIT = 3,
    SSM_MAPS_ALLOC_FAIL = 4,
    SSM_FRAGMENTATION = 5,
    SSM_MAPS_DEALLOC = 6,
    SSM_MAPS_DEALLOC_FAIL = 7,
    SSM_MAPS_INTEGRITY_FAIL = 8,
    SSM_MAPS_INTEGRITY_PASS = 9,
    SSM_MAPS_INTEGRITY = 10,
    SSM_FRAGMENTATION_MSG = 11,
    SSM_MAPS_SET = 12,
    SSM_MAPS_UNSET = 13,
    SSM_END = 14,
    SSM_INVALID_INPUT = 15,
    SSM_MAPS_NOT_FOUND = 16,
    SSM_FILE_OPEN = 17,
    SSM_FILE_STUB_OUTPUT = 18,
    SSM_INFO = 19,
    SSM_MAPS_GET = 20
} LoggerSSMOption;

/**
 * @brief Prints debug information for the Sector Space Manager (SSM).
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _case Identifier for the type of debug information to print.
 * @param[in] _startByte Byte offset at which to begin the debug trace.
 * @return void
 */
void log_ssm(LoggerSSMOption _case, int _startByte);

/**
 * @brief Prints debug information for the File Sector Manager (FSM).
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _case Identifier for the type of debug information to print.
 * @param[in] _startByte Byte offset at which to begin the debug trace.
 * @return void
 */
void log_fsm(LoggerFSMOption _case, unsigned int _startByte);

#endif  // LOGGER_H
