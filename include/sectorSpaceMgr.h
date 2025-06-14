/*********************************
 *	Sector Space Manager (SSM)
 *	Author: Michael Lombardi
 **********************************/
#ifndef SECTOR_SPACE_MGR_H
#define SECTOR_SPACE_MGR_H

#include <stdio.h>

#include "config.h"
#include "gDefinitions.h"
#include "ssmDefinitions.h"

//============================== SSM TYPE DEFINITION ==============================//
typedef struct {
    FILE *alocMapHandle;
    FILE *freeMapHandle;
    unsigned int sampleCount;
    unsigned char alocMap[SECTOR_BYTES];
    unsigned char freeMap[SECTOR_BYTES];
    unsigned int contSectors;
    unsigned int index[2];
    unsigned int badSector[MAX_INPUT][2];
    float fragmented;
} SecSpaceMgr;

//============================== SSM FUNCTION PROTOTYPES ==========================//

/**
 * @brief Initializes the Sector Space Manager.
 * Sets default values, resets tracking structures, and loads allocation and free maps
 * from disk into memory. This should be called before any allocation or deallocation.
 * @param[in,out] _ssm Pointer to the SecSpaceMgr structure to initialize.
 * @return void
 */
void ssm_init(SecSpaceMgr *_ssm);

/**
 * @brief Initializes and zeroes out the sector allocation and free maps.
 * Writes a clean slate to both maps on disk: allocation map is zeroed (no sectors allocated),
 * and the free map is filled (all sectors available).
 * @param[in,out] _ssm Pointer to the SecSpaceMgr structure.
 * @return void
 */
void ssm_init_maps(SecSpaceMgr *_ssm);

/**
 * @brief Marks a contiguous range of sectors as allocated.
 * Using internal state (index and count), marks sectors in the free map as allocated
 * and updates both maps on disk. Checks consistency between maps afterward.
 * @param[in,out] _ssm Pointer to the SecSpaceMgr structure.
 * @return True if sectors were allocated and maps remained consistent, False otherwise.
 */
Bool ssm_allocate_sectors(SecSpaceMgr *_ssm);

/**
 * @brief Frees a contiguous range of sectors.
 * Reverses allocation by marking the sectors as free in the maps, then updates the
 * maps on disk. Checks for consistency between maps.
 * @param[in,out] _ssm Pointer to the SecSpaceMgr structure.
 * @return True if deallocation succeeded and maps remained consistent, False otherwise.
 */
Bool ssm_deallocate_sectors(SecSpaceMgr *_ssm);

/**
 * @brief Finds a contiguous block of free sectors.
 * Searches the free map for `_n` contiguous free sectors. If found, stores
 * the byte and bit index in the manager's internal state (`_ssm->index`)
 * and returns success.
 * @param[in] _n Number of contiguous sectors to find (1–32).
 * @param[in,out] _ssm Pointer to the SecSpaceMgr structure.
 * @return True if a suitable block was found, False otherwise.
 */
Bool ssm_get_sector(int _n, SecSpaceMgr *_ssm);

#endif  // SECTOR_SPACE_MGR_H