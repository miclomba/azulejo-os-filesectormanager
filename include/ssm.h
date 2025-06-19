/*********************************
 *	Sector Space Manager (SSM)
 *	Author: Michael Lombardi
 **********************************/
#ifndef SECTOR_SPACE_MGR_H
#define SECTOR_SPACE_MGR_H

#include <stdio.h>

#include "config.h"
#include "global_constants.h"
#include "ssm_constants.h"

//============================== SSM TYPE DEFINITION ==============================//
/**
 * @brief Sector State Manager structure.
 *
 * Holds metadata and runtime data for managing sector allocation and fragmentation
 * within a file system or block storage interface.
 */
typedef struct SSM {
    /** File handle for the allocation map. */
    FILE *alocMapHandle;
    /** File handle for the free sector map. */
    FILE *freeMapHandle;
    /** Total number of samples/sectors tracked. */
    unsigned int sampleCount;
    /** Allocation bitmap per sector. */
    unsigned char alocMap[SECTOR_BYTES];
    /** Free sector bitmap. */
    unsigned char freeMap[SECTOR_BYTES];
    /** Count of contiguous sectors found. */
    unsigned int contSectors;
    /** Index used for internal iteration or sector marking (e.g., byte/bit). */
    unsigned int index[2];
    /** List of bad sectors with their corresponding coordinates/indexes. */
    unsigned int badSector[MAX_INPUT][2];
    /** Fragmentation percentage as a floating-point value (0.0 to 100.0). */
    float fragmented;
} SSM;

// Create
extern SSM *ssm;

//============================== SSM FUNCTION PROTOTYPES ==========================//

/**
 * @brief Initializes the Sector Space Manager.
 * Sets default values, resets tracking structures, and loads allocation and free maps
 * from disk into memory. This should be called before any allocation or deallocation.
 * @return void
 */
void ssm_init(void);

/**
 * @brief Initializes and zeroes out the sector allocation and free maps.
 * Writes a clean slate to both maps on disk: allocation map is zeroed (no sectors allocated),
 * and the free map is filled (all sectors available).
 * @return void
 */
void ssm_init_maps(void);

/**
 * @brief Marks a contiguous range of sectors as allocated.
 * Using internal state (index and count), marks sectors in the free map as allocated
 * and updates both maps on disk. Checks consistency between maps afterward.
 * @return True if sectors were allocated and maps remained consistent, False otherwise.
 */
Bool ssm_allocate_sectors(void);

/**
 * @brief Frees a contiguous range of sectors.
 * Reverses allocation by marking the sectors as free in the maps, then updates the
 * maps on disk. Checks for consistency between maps.
 * @return True if deallocation succeeded and maps remained consistent, False otherwise.
 */
Bool ssm_deallocate_sectors(void);

/**
 * @brief Finds a contiguous block of free sectors.
 * Searches the free map for `_n` contiguous free sectors. If found, stores
 * the byte and bit index in the manager's internal state (`_ssm->index`)
 * and returns success.
 * @param[in] _n Number of contiguous sectors to find (1–32).
 * @return True if a suitable block was found, False otherwise.
 */
Bool ssm_get_sector(int _n);

#endif  // SECTOR_SPACE_MGR_H
