/*******************************************************************************
 * Sector Space Manager (SSM)
 * Author: Michael Lombardi
 *******************************************************************************/
#ifndef SSM_DEFINITIONS_H
#define SSM_DEFINITIONS_H

#include "config.h"

#ifndef SECTOR_BYTES
#define SECTOR_BYTES (700)
#endif

#ifndef NUM_SECTORS
#define NUM_SECTORS (8 * SECTOR_BYTES)
#endif

#endif  // SSM_DEFINITIONS_H
