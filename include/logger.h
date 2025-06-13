/*********************************
 *	Logger
 *	Author: Michael Lombardi
 **********************************/
#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#include "config.h"
#include "sectorSpaceMgr.h"

//============================== LOGGER FUNCTION PROTOTYPES ==========================//
void logSSM(SecSpaceMgr *_ssm, int _case, int _startByte);
void logFSM(FileSectorMgr *_fsm, int _case, unsigned int _startByte);

#endif  // LOGGER_H