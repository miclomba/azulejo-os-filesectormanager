/*********************************
 *	Sector Space Manager (SSM)
 *	Author: Michael Lombardi
 **********************************/
#include "ssm.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "fsm_constants.h"
#include "global_constants.h"
#include "ssm_constants.h"

//============================== SSM STRUCT =====================================//
static SSM ssm_instance = {
    .alocMapHandle = NULL,
    .freeMapHandle = NULL,
    .alocMap = {0},  // initialize first element with 0 and the rest are implicitly initialized to 0
    .freeMap = {0},  // initialize first element with 0 and the rest are implicitly initialized to 0
    .sampleCount = 0,
    .contSectors = 0,
    .index = {0, 0},
    .badSector = {{0}},  // The first inner array (badSector[0]) is initialized to {0, 0}. All other
                         // entries are implicitly initialized to zero
    .fragmented = 0.0f};

SSM *ssm = &ssm_instance;

//============================== SSM FUNCTION PROTOTYPES =========================//
static Bool check_integrity(void);
static void is_fragmented(void) __attribute__((unused));
static void set_aloc_sector(int _byte, int _bit) __attribute__((unused));
static void set_free_sector(int _byte, int _bit) __attribute__((unused));
static Bool ssm_get_sector(int _n);
static unsigned int ssm_get_sector_offset(void);

//============================== SSM FUNCTION DEFINITIONS =========================//
void ssm_init(void) {
    ssm->contSectors = 0;
    ssm->index[0] = (unsigned int)(-1);
    ssm->index[1] = (unsigned int)(-1);
    // assign -1 to all unsigned int in badSector
    memset(ssm->badSector, 0xFF, sizeof(ssm->badSector));
    ssm->fragmented = 0;
    ssm->alocMapHandle = fopen(SSM_ALLOCATE_MAP, "r+");
    ssm->freeMapHandle = fopen(SSM_FREE_MAP, "r+");
    ssm->sampleCount = fread(ssm->alocMap, 1, SECTOR_BYTES, ssm->alocMapHandle);
    ssm->sampleCount = fread(ssm->freeMap, 1, SECTOR_BYTES, ssm->freeMapHandle);
    fclose(ssm->alocMapHandle);
    fclose(ssm->freeMapHandle);
    ssm->alocMapHandle = Null;
    ssm->freeMapHandle = Null;
}

void ssm_init_maps(void) {
    unsigned char map[SECTOR_BYTES];
    memset(map, 0, SECTOR_BYTES);
    ssm->alocMapHandle = fopen(SSM_ALLOCATE_MAP, "r+");
    ssm->sampleCount = fwrite(map, 1, SECTOR_BYTES, ssm->alocMapHandle);
    memset(map, UINT8_MAX, SECTOR_BYTES);
    ssm->freeMapHandle = fopen(SSM_FREE_MAP, "r+");
    ssm->sampleCount = fwrite(map, 1, SECTOR_BYTES, ssm->freeMapHandle);
    fclose(ssm->alocMapHandle);
    fclose(ssm->freeMapHandle);
    ssm->alocMapHandle = Null;
    ssm->freeMapHandle = Null;
}

unsigned int ssm_allocate_sectors(int _n) {
    if (!ssm_get_sector(_n)) {
        return -1;
    }

    unsigned int bit = ssm->index[1];
    unsigned int byte = ssm->index[0];
    int n = ssm->contSectors;
    while (n > 0) {
        for (; bit < BITS_PER_BYTE; bit++) {
            ssm->freeMap[byte] -= pow(2, bit);
            ssm->alocMap[byte] += pow(2, bit);
            n--;
            if (n == 0) {
                break;
            }
        }
        byte++;
        bit = 0;
    }

    Bool integrity = check_integrity();
    if (integrity == False) return -1;
    ssm->alocMapHandle = fopen(SSM_ALLOCATE_MAP, "r+");
    ssm->freeMapHandle = fopen(SSM_FREE_MAP, "r+");
    ssm->sampleCount = fwrite(ssm->alocMap, 1, SECTOR_BYTES, ssm->alocMapHandle);
    ssm->sampleCount = fwrite(ssm->freeMap, 1, SECTOR_BYTES, ssm->freeMapHandle);
    fclose(ssm->alocMapHandle);
    fclose(ssm->freeMapHandle);
    ssm->alocMapHandle = Null;
    ssm->freeMapHandle = Null;
    return ssm_get_sector_offset();
}

Bool ssm_deallocate_sectors(int _sectorNum) {
    ssm->contSectors = 1;
    ssm->index[0] = _sectorNum / BITS_PER_BYTE;
    ssm->index[1] = _sectorNum % BITS_PER_BYTE;

    if (ssm->index[0] != (unsigned int)(-1)) {
        int n;
        unsigned int byte;
        int bit;
        bit = ssm->index[1];
        byte = ssm->index[0];
        n = ssm->contSectors;
        while (n > 0) {
            for (; bit < BITS_PER_BYTE; bit++) {
                ssm->freeMap[byte] += pow(2, bit);
                ssm->alocMap[byte] -= pow(2, bit);
                n--;
                if (n == 0) {
                    break;
                }
            }
            byte++;
            bit = 0;
        }
    }
    Bool integrity = check_integrity();
    if (integrity == False) return False;
    ssm->index[0] = (unsigned int)(-1);
    ssm->index[1] = (unsigned int)(-1);
    ssm->contSectors = 0;
    ssm->alocMapHandle = fopen(SSM_ALLOCATE_MAP, "r+");
    ssm->freeMapHandle = fopen(SSM_FREE_MAP, "r+");
    ssm->sampleCount = fwrite(ssm->alocMap, 1, SECTOR_BYTES, ssm->alocMapHandle);
    ssm->sampleCount = fwrite(ssm->freeMap, 1, SECTOR_BYTES, ssm->freeMapHandle);
    fclose(ssm->alocMapHandle);
    fclose(ssm->freeMapHandle);
    ssm->alocMapHandle = Null;
    ssm->freeMapHandle = Null;
    return True;
}

/**
 * @brief Gets the sector offset of the last allocated sector.
 * @return The disk byte offset to the current sector.
 */
static inline unsigned int ssm_get_sector_offset(void) {
    return BLOCK_SIZE * ((BITS_PER_BYTE * ssm->index[0]) + (ssm->index[1]));
}

/**
 * @brief Finds a contiguous block of free sectors.
 * Searches the free map for `_n` contiguous free sectors. If found, stores
 * the byte and bit index in the manager's internal state (`_ssm->index`)
 * and returns success.
 * @param[in] _n Number of contiguous sectors to find (1–32).
 * @return True if a suitable block was found, False otherwise.
 */
static Bool ssm_get_sector(int _n) {
    ssm->contSectors = _n;
    // We assume 0 < _n < 33
    if (_n < 33 && _n > 0) {
        unsigned int mask = 0;
        int i;
        for (i = _n - 1; i > -1; i--) {
            mask += (unsigned int)pow(2, i);
        }
        Bool found = False;
        unsigned int result = mask;
        unsigned int buff = mask;
        unsigned long long int subsetMap[1];
        unsigned int byteCount = 0;
        unsigned char *freeMap = ssm->freeMap;
        ssm->index[0] = -1;
        ssm->index[1] = -1;
        unsigned long long value;
        while (byteCount < SECTOR_BYTES) {
            memcpy(&value, freeMap, sizeof(value));
            subsetMap[0] = value;
            for (i = 0; i < 32; i++) {
                buff = (unsigned int)subsetMap[0];
                result = mask & buff;
                if (result == mask) {
                    found = True;
                    break;
                } else
                    subsetMap[0] >>= 1;
            }
            if (found == True) {
                ssm->index[0] = byteCount + (i / BITS_PER_BYTE);
                ssm->index[1] = i % BITS_PER_BYTE;
                return True;  // break;
            }
            freeMap += 4;
            byteCount += 4;
        }
    }
    return False;
}

/**
 * @brief Verifies consistency between the free map and allocation map.
 * Compares each byte in the free and allocation maps using XOR to identify
 * overlapping or missing sector status entries. Records problematic sectors in
 * `ssm->badSector[]`.
 * @param[in,out] _ssm Pointer to the SSM structure.
 * @return True if all sectors are consistent, False if corruption is detected.
 */
static Bool check_integrity(void) {
    int bitShift;
    unsigned char result;
    unsigned int j = 0;

    memset(ssm->badSector, -1, MAX_INPUT * 2 * sizeof(unsigned int));
    for (unsigned int i = 0; i < SECTOR_BYTES; i++) {
        result = ssm->freeMap[i] ^ ssm->alocMap[i];
        if (result < UINT8_MAX) {  // 255
            bitShift = 0;
            while (result % 2 == 1) {
                bitShift += 1;
                result >>= 1;
            }
            ssm->badSector[j][0] = i;
            ssm->badSector[j][1] = bitShift;
            j++;
        }
    }
    if (ssm->badSector[0][0] != (unsigned int)(-1)) {
        return False;
    }
    return True;
}

/**
 * @brief Estimates fragmentation of the free space.
 * Scans the free map and counts transitions between free and used bits to estimate
 * fragmentation. Stores the result in `ssm->fragmented` as a normalized ratio.
 * @param[in,out] _ssm Pointer to the SSM structure.
 * @return void
 */
static void is_fragmented(void) {
    unsigned char map[SECTOR_BYTES];
    for (int i = 0; i < SECTOR_BYTES; i++) {
        map[i] = ssm->freeMap[i];
    }
    int result = map[0] % 2;
    int tmp = result;
    int fragment = 0;
    int j = 0;
    ssm->fragmented = 0;
    for (int i = 0; i < SECTOR_BYTES; i++) {
        for (j = 0; j < BITS_PER_BYTE; j++) {
            map[i] >>= 1;
            result = map[i] % 2;
            if (result != tmp) {
                tmp = result;
                fragment++;
            }
        }
    }
    ssm->fragmented = (float)((float)fragment / (float)NUM_SECTORS);
}

/**
 * @brief Flips the allocation bit for a specific sector.
 * Toggles the allocation status of a given sector in the allocation map and persists
 * the updated maps to disk.
 * @param[in,out] _ssm Pointer to the SSM structure.
 * @param[in] _byte Byte index of the sector in the map.
 * @param[in] _bit Bit index (0–7) within the byte.
 * @return void
 */
static void set_aloc_sector(int _byte, int _bit) {
    unsigned char mapByte = ssm->alocMap[_byte];
    mapByte >>= (BITS_PER_BYTE - 1 - _bit);
    if (mapByte % 2 == 0)
        ssm->alocMap[_byte] += pow(2, _bit);
    else
        ssm->alocMap[_byte] -= pow(2, _bit);
    ssm->alocMapHandle = fopen(SSM_ALLOCATE_MAP, "r+");
    ssm->freeMapHandle = fopen(SSM_FREE_MAP, "r+");
    ssm->sampleCount = fwrite(ssm->alocMap, 1, SECTOR_BYTES, ssm->alocMapHandle);
    ssm->sampleCount = fwrite(ssm->freeMap, 1, SECTOR_BYTES, ssm->freeMapHandle);
    fclose(ssm->alocMapHandle);
    fclose(ssm->freeMapHandle);
    ssm->alocMapHandle = Null;
    ssm->freeMapHandle = Null;
}

/**
 * @brief Flips the free bit for a specific sector.
 * Toggles the free status of a given sector in the free map and writes the
 * updated maps to disk.
 * @param[in,out] _ssm Pointer to the SSM structure.
 * @param[in] _byte Byte index of the sector in the map.
 * @param[in] _bit Bit index (0–7) within the byte.
 * @return void
 */
static void set_free_sector(int _byte, int _bit) {
    unsigned char mapByte = ssm->freeMap[_byte];
    mapByte >>= (BITS_PER_BYTE - 1 - _bit);
    if (mapByte % 2 == 0)
        ssm->freeMap[_byte] += pow(2, _bit);
    else
        ssm->freeMap[_byte] -= pow(2, _bit);
    ssm->alocMapHandle = fopen(SSM_ALLOCATE_MAP, "r+");
    ssm->freeMapHandle = fopen(SSM_FREE_MAP, "r+");
    ssm->sampleCount = fwrite(ssm->alocMap, 1, SECTOR_BYTES, ssm->alocMapHandle);
    ssm->sampleCount = fwrite(ssm->freeMap, 1, SECTOR_BYTES, ssm->freeMapHandle);
    fclose(ssm->alocMapHandle);
    fclose(ssm->freeMapHandle);
    ssm->alocMapHandle = Null;
    ssm->freeMapHandle = Null;
}
