/*******************************************************************************
 * Sector Space Manager (SSM)
 * Author: Michael Lombardi
 *******************************************************************************/

#define _GNU_SOURCE

#ifndef SECTOR_BYTES
#define SECTOR_BYTES (700)
#endif

#ifndef NUM_SECTORS
#define NUM_SECTORS (8 * SECTOR_BYTES)
#endif
