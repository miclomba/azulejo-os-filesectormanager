/*********************************
 *	Sector Space Manager (SSM)
 *	Author: Michael Lombardi
 **********************************/
#include "sectorSpaceMgr.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "gDefinitions.h"
#include "ssmDefinitions.h"

//============================== SSM FUNCTION PROTOTYPES =========================//
static Bool check_integrity(SecSpaceMgr *_ssm);
static void is_fragmented(SecSpaceMgr *_ssm) __attribute__((unused));
static void set_aloc_sector(SecSpaceMgr *_ssm, int _byte, int _bit) __attribute__((unused));
static void set_free_sector(SecSpaceMgr *_ssm, int _byte, int _bit) __attribute__((unused));

//============================== SSM FUNCTION DEFINITIONS =========================//
void ssm_init(SecSpaceMgr *_ssm) {
    char dbfile[256];
    unsigned int i;
    _ssm->contSectors = 0;
    _ssm->index[0] = (unsigned int)(-1);
    _ssm->index[1] = (unsigned int)(-1);
    for (i = 0; i < MAX_INPUT; i++) {
        _ssm->badSector[i][0] = (unsigned int)(-1);
        _ssm->badSector[i][1] = (unsigned int)(-1);
    }
    _ssm->fragmented = 0;
    sprintf(dbfile, "./fs/aMap");
    _ssm->alocMapHandle = fopen(dbfile, "r+");
    sprintf(dbfile, "./fs/fMap");
    _ssm->freeMapHandle = fopen(dbfile, "r+");
    _ssm->sampleCount = fread(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
    _ssm->sampleCount = fread(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);
    fclose(_ssm->alocMapHandle);
    fclose(_ssm->freeMapHandle);
    _ssm->alocMapHandle = Null;
    _ssm->freeMapHandle = Null;
}

void ssm_init_maps(SecSpaceMgr *_ssm) {
    unsigned int i;
    char dbfile[256];
    unsigned char map[SECTOR_BYTES];
    sprintf(dbfile, "./fs/aMap");
    _ssm->alocMapHandle = fopen(dbfile, "r+");
    sprintf(dbfile, "./fs/fMap");
    _ssm->freeMapHandle = fopen(dbfile, "r+");
    for (i = 0; i < SECTOR_BYTES; i++) {
        map[i] = 0;
    }
    _ssm->sampleCount = fwrite(map, 1, SECTOR_BYTES, _ssm->alocMapHandle);
    for (i = 0; i < SECTOR_BYTES; i++) {
        map[i] = 255;
    }
    _ssm->sampleCount = fwrite(map, 1, SECTOR_BYTES, _ssm->freeMapHandle);
    fclose(_ssm->alocMapHandle);
    fclose(_ssm->freeMapHandle);
    _ssm->alocMapHandle = Null;
    _ssm->freeMapHandle = Null;
}

Bool ssm_allocate_sectors(SecSpaceMgr *_ssm) {
    if (_ssm->index[0] != (unsigned int)(-1)) {
        int n;
        unsigned int byte;
        int bit;
        bit = _ssm->index[1];
        byte = _ssm->index[0];
        n = _ssm->contSectors;
        while (n > 0) {
            for (; bit < 8; bit++) {
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
    Bool integrity;
    integrity = check_integrity(_ssm);
    if (integrity == False) return False;
    _ssm->index[0] = (unsigned int)(-1);
    _ssm->index[1] = (unsigned int)(-1);
    _ssm->contSectors = 0;
    char dbfile[256];
    sprintf(dbfile, "./fs/aMap");
    _ssm->alocMapHandle = fopen(dbfile, "r+");
    sprintf(dbfile, "./fs/fMap");
    _ssm->freeMapHandle = fopen(dbfile, "r+");
    _ssm->sampleCount = fwrite(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
    _ssm->sampleCount = fwrite(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);
    fclose(_ssm->alocMapHandle);
    fclose(_ssm->freeMapHandle);
    _ssm->alocMapHandle = Null;
    _ssm->freeMapHandle = Null;
    return True;
}

Bool ssm_deallocate_sectors(SecSpaceMgr *_ssm) {
    if (_ssm->index[0] != (unsigned int)(-1)) {
        int n;
        unsigned int byte;
        int bit;
        bit = _ssm->index[1];
        byte = _ssm->index[0];
        n = _ssm->contSectors;
        while (n > 0) {
            for (; bit < 8; bit++) {
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
    Bool integrity;
    integrity = check_integrity(_ssm);
    if (integrity == False) return False;
    _ssm->index[0] = (unsigned int)(-1);
    _ssm->index[1] = (unsigned int)(-1);
    _ssm->contSectors = 0;
    char dbfile[256];
    sprintf(dbfile, "./fs/aMap");
    _ssm->alocMapHandle = fopen(dbfile, "r+");
    sprintf(dbfile, "./fs/fMap");
    _ssm->freeMapHandle = fopen(dbfile, "r+");
    _ssm->sampleCount = fwrite(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
    _ssm->sampleCount = fwrite(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);
    fclose(_ssm->alocMapHandle);
    fclose(_ssm->freeMapHandle);
    _ssm->alocMapHandle = Null;
    _ssm->freeMapHandle = Null;
    return True;
}

Bool ssm_get_sector(int _n, SecSpaceMgr *_ssm) {
    int i;
    unsigned int mask;
    unsigned int result;
    unsigned int buff;
    unsigned char *freeMap;
    unsigned long long int subsetMap[1];
    unsigned int byteCount;
    Bool found;
    _ssm->contSectors = _n;
    // We assume 0 < _n < 33
    if (_n < 33 && _n > 0) {
        mask = 0;
        for (i = _n - 1; i > -1; i--) {
            mask += (unsigned int)pow(2, i);
        }
        found = False;
        result = mask;
        buff = mask;
        subsetMap[0] = 0;
        byteCount = 0;
        freeMap = _ssm->freeMap;
        _ssm->index[0] = -1;
        _ssm->index[1] = -1;
        while (byteCount < SECTOR_BYTES) {
            subsetMap[0] = *((unsigned long long int *)freeMap);
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
                _ssm->index[0] = byteCount + (i / 8);
                _ssm->index[1] = i % 8;
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
 * @param[in,out] _ssm Pointer to the SecSpaceMgr structure.
 * @return True if all sectors are consistent, False if corruption is detected.
 */
static Bool check_integrity(SecSpaceMgr *_ssm) {
    unsigned int i, j;
    int bitShift;
    unsigned char result;
    for (i = 0; i < MAX_INPUT; i++) {
        _ssm->badSector[i][0] = -1;
        _ssm->badSector[i][1] = -1;
    }
    j = 0;
    for (i = 0; i < SECTOR_BYTES; i++) {
        result = _ssm->freeMap[i] ^ _ssm->alocMap[i];
        if (result < 255) {
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
 * @param[in,out] _ssm Pointer to the SecSpaceMgr structure.
 * @return void
 */
static void is_fragmented(SecSpaceMgr *_ssm) {
    int i, j;
    int tmp;
    int result;
    int fragment;
    unsigned char map[SECTOR_BYTES];
    for (i = 0; i < SECTOR_BYTES; i++) {
        map[i] = _ssm->freeMap[i];
    }
    result = map[0] % 2;
    tmp = result;
    fragment = 0;
    _ssm->fragmented = 0;
    for (i = 0; i < SECTOR_BYTES; i++) {
        for (j = 0; j < 8; j++) {
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
 * @param[in,out] _ssm Pointer to the SecSpaceMgr structure.
 * @param[in] _byte Byte index of the sector in the map.
 * @param[in] _bit Bit index (0–7) within the byte.
 * @return void
 */
static void set_aloc_sector(SecSpaceMgr *_ssm, int _byte, int _bit) {
    unsigned char mapByte;
    mapByte = _ssm->alocMap[_byte];
    mapByte >>= (7 - _bit);
    if (mapByte % 2 == 0)
        _ssm->alocMap[_byte] += pow(2, _bit);
    else
        _ssm->alocMap[_byte] -= pow(2, _bit);
    char dbfile[256];
    sprintf(dbfile, "./fs/aMap");
    _ssm->alocMapHandle = fopen(dbfile, "r+");
    sprintf(dbfile, "./fs/fMap");
    _ssm->freeMapHandle = fopen(dbfile, "r+");
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
 * @param[in,out] _ssm Pointer to the SecSpaceMgr structure.
 * @param[in] _byte Byte index of the sector in the map.
 * @param[in] _bit Bit index (0–7) within the byte.
 * @return void
 */
static void set_free_sector(SecSpaceMgr *_ssm, int _byte, int _bit) {
    unsigned char mapByte;
    mapByte = _ssm->freeMap[_byte];
    mapByte >>= (7 - _bit);
    if (mapByte % 2 == 0)
        _ssm->freeMap[_byte] += pow(2, _bit);
    else
        _ssm->freeMap[_byte] -= pow(2, _bit);
    char dbfile[256];
    sprintf(dbfile, "./fs/aMap");
    _ssm->alocMapHandle = fopen(dbfile, "r+");
    sprintf(dbfile, "./fs/fMap");
    _ssm->freeMapHandle = fopen(dbfile, "r+");
    _ssm->sampleCount = fwrite(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
    _ssm->sampleCount = fwrite(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);
    fclose(_ssm->alocMapHandle);
    fclose(_ssm->freeMapHandle);
    _ssm->alocMapHandle = Null;
    _ssm->freeMapHandle = Null;
}
