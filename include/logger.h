/*********************************
 *	Logger
 *	Author: Michael Lombardi
 **********************************/
#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#include "config.h"
#include "fsm.h"
#include "ssm.h"

/**
 * @brief Prints debug information for the Sector Space Manager (SSM).
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _ssm Pointer to the SSM structure.
 * @param[in] _case Identifier for the type of debug information to print.
 * @param[in] _startByte Byte offset at which to begin the debug trace.
 * @return void
 */
void log_ssm(SSM *_ssm, int _case, int _startByte);

/**
 * @brief Prints debug information for the File Sector Manager (FSM).
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _fsm Pointer to the fsm structure.
 * @param[in] _case Identifier for the type of debug information to print.
 * @param[in] _startByte Byte offset at which to begin the debug trace.
 * @return void
 */
void log_fsm(FSM *_fsm, int _case, unsigned int _startByte);

#endif  // LOGGER_H
