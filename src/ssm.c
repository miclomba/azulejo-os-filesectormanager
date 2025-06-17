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
#include "global_constants.h"
#include "ssm_constants.h"

//============================== SSM FUNCTION PROTOTYPES =========================//
static Bool check_integrity(SSM *_ssm);
static void is_fragmented(SSM *_ssm) __attribute__((unused));
static void set_aloc_sector(SSM *_ssm, int _byte, int _bit) __attribute__((unused));
static void set_free_sector(SSM *_ssm, int _byte, int _bit) __attribute__((unused));

//============================== SSM FUNCTION DEFINITIONS =========================//
void ssm_init(SSM *_ssm) {
    _ssm->contSectors = 0;
    _ssm->index[0] = (unsigned int)(-1);
    _ssm->index[1] = (unsigned int)(-1);
    // assign -1 to all unsigned int in badSector
    memset(_ssm->badSector, 0xFF, sizeof(_ssm->badSector));
    _ssm->fragmented = 0;
    _ssm->alocMapHandle = fopen(SSM_ALLOCATE_MAP, "r+");
    _ssm->freeMapHandle = fopen(SSM_FREE_MAP, "r+");
    _ssm->sampleCount = fread(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
    _ssm->sampleCount = fread(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);
    fclose(_ssm->alocMapHandle);
    fclose(_ssm->freeMapHandle);
    _ssm->alocMapHandle = Null;
    _ssm->freeMapHandle = Null;
}

void ssm_init_maps(SSM *_ssm) {
    unsigned char map[SECTOR_BYTES];
    memset(map, 0, SECTOR_BYTES);
    _ssm->alocMapHandle = fopen(SSM_ALLOCATE_MAP, "r+");
    _ssm->sampleCount = fwrite(map, 1, SECTOR_BYTES, _ssm->alocMapHandle);
    memset(map, UINT8_MAX, SECTOR_BYTES);
    _ssm->freeMapHandle = fopen(SSM_FREE_MAP, "r+");
    _ssm->sampleCount = fwrite(map, 1, SECTOR_BYTES, _ssm->freeMapHandle);
    fclose(_ssm->alocMapHandle);
    fclose(_ssm->freeMapHandle);
    _ssm->alocMapHandle = Null;
    _ssm->freeMapHandle = Null;
}

Bool ssm_allocate_sectors(SSM *_ssm) {
    if (_ssm->index[0] != (unsigned int)(-1)) {
        int n;
        unsigned int byte;
        int bit;
        bit = _ssm->index[1];
        byte = _ssm->index[0];
        n = _ssm->contSectors;
        while (n > 0) {
            for (; bit < BITS_PER_BYTE; bit++) {
                _ssm->freeMap[byte] -= pow(2, bit);
                _ssm->alocMap[byte] += pow(2, bit);
                n--;
                if (n == 0) {
                    break;
                }
            }
            byte++;
            bit = 0;
        }
    }
    Bool integrity = check_integrity(_ssm);
    if (integrity == False) return False;
    _ssm->index[0] = (unsigned int)(-1);
    _ssm->index[1] = (unsigned int)(-1);
    _ssm->contSectors = 0;
    _ssm->alocMapHandle = fopen(SSM_ALLOCATE_MAP, "r+");
    _ssm->freeMapHandle = fopen(SSM_FREE_MAP, "r+");
    _ssm->sampleCount = fwrite(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
    _ssm->sampleCount = fwrite(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);
    fclose(_ssm->alocMapHandle);
    fclose(_ssm->freeMapHandle);
    _ssm->alocMapHandle = Null;
    _ssm->freeMapHandle = Null;
    return True;
}

Bool ssm_deallocate_sectors(SSM *_ssm) {
    if (_ssm->index[0] != (unsigned int)(-1)) {
        int n;
        unsigned int byte;
        int bit;
        bit = _ssm->index[1];
        byte = _ssm->index[0];
        n = _ssm->contSectors;
        while (n > 0) {
            for (; bit < BITS_PER_BYTE; bit++) {
                _ssm->freeMap[byte] += pow(2, bit);
                _ssm->alocMap[byte] -= pow(2, bit);
                n--;
                if (n == 0) {
                    break;
                }
            }
            byte++;
            bit = 0;
        }
    }
    Bool integrity = check_integrity(_ssm);
    if (integrity == False) return False;
    _ssm->index[0] = (unsigned int)(-1);
    _ssm->index[1] = (unsigned int)(-1);
    _ssm->contSectors = 0;
    _ssm->alocMapHandle = fopen(SSM_ALLOCATE_MAP, "r+");
    _ssm->freeMapHandle = fopen(SSM_FREE_MAP, "r+");
    _ssm->sampleCount = fwrite(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
    _ssm->sampleCount = fwrite(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);
    fclose(_ssm->alocMapHandle);
    fclose(_ssm->freeMapHandle);
    _ssm->alocMapHandle = Null;
    _ssm->freeMapHandle = Null;
    return True;
}

Bool ssm_get_sector(int _n, SSM *_ssm) {
    _ssm->contSectors = _n;
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
        unsigned char *freeMap = _ssm->freeMap;
        _ssm->index[0] = -1;
        _ssm->index[1] = -1;
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
                _ssm->index[0] = byteCount + (i / BITS_PER_BYTE);
                _ssm->index[1] = i % BITS_PER_BYTE;
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
 * `_ssm->badSector[]`.
 * @param[in,out] _ssm Pointer to the SSM structure.
 * @return True if all sectors are consistent, False if corruption is detected.
 */
static Bool check_integrity(SSM *_ssm) {
    int bitShift;
    unsigned char result;
    unsigned int j = 0;

    memset(_ssm->badSector, -1, MAX_INPUT * 2 * sizeof(unsigned int));
    for (unsigned int i = 0; i < SECTOR_BYTES; i++) {
        result = _ssm->freeMap[i] ^ _ssm->alocMap[i];
        if (result < UINT8_MAX) {  // 255
            bitShift = 0;
            while (result % 2 == 1) {
                bitShift += 1;
                result >>= 1;
            }
            _ssm->badSector[j][0] = i;
            _ssm->badSector[j][1] = bitShift;
            j++;
        }
    }
    if (_ssm->badSector[0][0] != (unsigned int)(-1)) {
        return False;
    }
    return True;
}

/**
 * @brief Estimates fragmentation of the free space.
 * Scans the free map and counts transitions between free and used bits to estimate
 * fragmentation. Stores the result in `_ssm->fragmented` as a normalized ratio.
 * @param[in,out] _ssm Pointer to the SSM structure.
 * @return void
 */
static void is_fragmented(SSM *_ssm) {
    unsigned char map[SECTOR_BYTES];
    for (int i = 0; i < SECTOR_BYTES; i++) {
        map[i] = _ssm->freeMap[i];
    }
    int result = map[0] % 2;
    int tmp = result;
    int fragment = 0;
    int j = 0;
    _ssm->fragmented = 0;
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
    _ssm->fragmented = (float)((float)fragment / (float)NUM_SECTORS);
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
static void set_aloc_sector(SSM *_ssm, int _byte, int _bit) {
    unsigned char mapByte = _ssm->alocMap[_byte];
    mapByte >>= (BITS_PER_BYTE - 1 - _bit);
    if (mapByte % 2 == 0)
        _ssm->alocMap[_byte] += pow(2, _bit);
    else
        _ssm->alocMap[_byte] -= pow(2, _bit);
    _ssm->alocMapHandle = fopen(SSM_ALLOCATE_MAP, "r+");
    _ssm->freeMapHandle = fopen(SSM_FREE_MAP, "r+");
    _ssm->sampleCount = fwrite(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
    _ssm->sampleCount = fwrite(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);
    fclose(_ssm->alocMapHandle);
    fclose(_ssm->freeMapHandle);
    _ssm->alocMapHandle = Null;
    _ssm->freeMapHandle = Null;
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
static void set_free_sector(SSM *_ssm, int _byte, int _bit) {
    unsigned char mapByte = _ssm->freeMap[_byte];
    mapByte >>= (BITS_PER_BYTE - 1 - _bit);
    if (mapByte % 2 == 0)
        _ssm->freeMap[_byte] += pow(2, _bit);
    else
        _ssm->freeMap[_byte] -= pow(2, _bit);
    _ssm->alocMapHandle = fopen(SSM_ALLOCATE_MAP, "r+");
    _ssm->freeMapHandle = fopen(SSM_FREE_MAP, "r+");
    _ssm->sampleCount = fwrite(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
    _ssm->sampleCount = fwrite(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);
    fclose(_ssm->alocMapHandle);
    fclose(_ssm->freeMapHandle);
    _ssm->alocMapHandle = Null;
    _ssm->freeMapHandle = Null;
}
