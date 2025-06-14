/*********************************
 *	Logger
 *	Author: Michael Lombardi
 **********************************/
#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#include "config.h"
#include "sectorSpaceMgr.h"

/*
 * @brief Print Sector Space Manager debug messages
 * @param _ssm pointer to the SSM
 * @param _case the debug statement to print
 * @param _startByte the byte to start at
 */
void logSSM(SecSpaceMgr *_ssm, int _case, int _startByte);

/*
 * @brief Print File Sector Manager debug messages
 * @param _fsm pointer to the FSM
 * @param _case the debug statement to print
 * @param _startByte the byte to start at
 */
void logFSM(FileSectorMgr *_fsm, int _case, unsigned int _startByte);

#endif  // LOGGER_H