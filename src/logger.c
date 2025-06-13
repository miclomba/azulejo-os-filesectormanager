/*********************************
 *	Sector Space Manager (SSM)
 *	Author: Michael Lombardi
 **********************************/
#include <stdio.h>

#include "config.h"
#include "gDefinitions.h"
#include "sectorSpaceMgr.h"
#include "ssmDefinitions.h"

void logSSM(SecSpaceMgr *_ssm, int _case, int _startByte) {
    int i, j, k;
    int count;
    int sector;
    unsigned char tmp;
    char byteArray[8];
    switch (_case) {
        case 0:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Initializing SSM maps...\n");
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 1:
            // printf("DEBUG_LEVEL > 0:\n");
            printf("//Print 128 contiguous Sectors from Free/Aloc Map starting at Sector (%d).\n",
                   _startByte * 8);
            // printf("//P:%d\n\n",_startByte);
            printf("=======================================================================\n");
            printf("FREE MAP\n");
            k = _startByte;
            count = 0;
            for (i = _startByte; i < SECTOR_BYTES && i < _startByte + 16; i++) {
                tmp = _ssm->freeMap[i];
                for (j = 0; j < 8; j++) {
                    if (tmp % 2 == 1)
                        byteArray[j] = '1';
                    else
                        byteArray[j] = '0';
                    tmp /= 2;
                }
                for (j = 0; j < 8; j++) printf("%c", byteArray[j]);
                printf(" ");
                if ((count + 1) % 8 == 0 || i + 1 == SECTOR_BYTES) {
                    printf("\n");
                    for (j = 0; j < 8; j++) {
                        if (k * 8 < 10)
                            printf("%d        ", k * 8);
                        else if (k * 8 < 100)
                            printf("%d       ", k * 8);
                        else if (k * 8 < 1000)
                            printf("%d      ", k * 8);
                        else if (k * 8 < 10000)
                            printf("%d     ", k * 8);
                        else if (k * 8 < 100000)
                            printf("%d    ", k * 8);
                        else if (k * 8 < 1000000)
                            printf("%d   ", k * 8);
                        else if (k * 8 < 10000000)
                            printf("%d  ", k * 8);
                        else if (k * 8 < 100000000)
                            printf("%d ", k * 8);
                        k++;
                    }
                    printf("\n");
                }
                count++;
            }
            printf("\n");
            printf("ALLOCATED MAP\n");
            k = _startByte;
            count = 0;
            for (i = _startByte; i < SECTOR_BYTES && i < _startByte + 16; i++) {
                tmp = _ssm->alocMap[i];
                for (j = 0; j < 8; j++) {
                    if (tmp % 2 == 1)
                        byteArray[j] = '1';
                    else
                        byteArray[j] = '0';
                    tmp /= 2;
                }
                for (j = 0; j < 8; j++) printf("%c", byteArray[j]);
                printf(" ");
                if ((count + 1) % 8 == 0 || i + 1 == SECTOR_BYTES) {
                    printf("\n");
                    for (j = 0; j < 8; j++) {
                        if (k * 8 < 10)
                            printf("%d        ", k * 8);
                        else if (k * 8 < 100)
                            printf("%d       ", k * 8);
                        else if (k * 8 < 1000)
                            printf("%d      ", k * 8);
                        else if (k * 8 < 10000)
                            printf("%d     ", k * 8);
                        else if (k * 8 < 100000)
                            printf("%d    ", k * 8);
                        else if (k * 8 < 1000000)
                            printf("%d   ", k * 8);
                        else if (k * 8 < 10000000)
                            printf("%d  ", k * 8);
                        else if (k * 8 < 100000000)
                            printf("%d ", k * 8);
                        k++;
                    }
                    printf("\n");
                }
                count++;
            }
            printf("=======================================================================\n\n\n");
            break;
        case 2:
            printf("DEBUG_LEVEL > 0:\n");
            sector = 8 * _ssm->index[0] + _ssm->index[1];
            printf("//Allocate %d contiguous sectors.\n", _ssm->contSectors);
            printf("//A:%d\n\n", _ssm->contSectors);
            printf("Allocating %d contiguous sectors at sector %d...\n", _ssm->contSectors, sector);
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 3:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Initializing SSM...\n");
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 4:
            printf("DEBUG_LEVEL > 0:\n");
            for (k = 0; k < MAX_INPUT; k++) {
                if (_ssm->badSector[k][0] == (unsigned int)(-1)) {
                    break;
                }
                sector = 8 * _ssm->badSector[k][0] + _ssm->badSector[k][1];
                printf("Failed to allocate sectors at sector %d\n", sector);
            }
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 5:
            printf("DEBUG_LEVEL > 0:\n");
            printf("//Check for degree of fragmentation.\n");
            printf("//F\n\n");
            printf("The degree of memory fragmentation is %10.10f \n", _ssm->fragmented);
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 6:
            printf("DEBUG_LEVEL > 0:\n");
            sector = 8 * _ssm->index[0] + _ssm->index[1];
            printf("//Deallocate %d contiguous sectors starting at sector %d\n", _ssm->contSectors,
                   _ssm->index[0] * 8 + _ssm->index[1]);
            printf("//D:%d:%d:%d\n\n", _ssm->contSectors, _ssm->index[0], _ssm->index[1]);
            printf("Deallocating %d contiguous sectors at sector %d...\n", _ssm->contSectors,
                   sector);
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 7:
            printf("DEBUG_LEVEL > 0:\n");
            for (k = 0; k < MAX_INPUT; k++) {
                if (_ssm->badSector[k][0] == (unsigned int)(-1)) {
                    break;
                }
                sector = 8 * _ssm->badSector[k][0] + _ssm->badSector[k][1];
                printf("Failed to deallocate sector %d\n", sector);
            }
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 8:
            printf("DEBUG_LEVEL > 0:\n");
            for (k = 0; k < MAX_INPUT; k++) {
                if (_ssm->badSector[k][0] == (unsigned int)(-1)) {
                    break;
                }
                sector = 8 * _ssm->badSector[k][0] + _ssm->badSector[k][1];
                printf("Failed integrity check at sector %d\n", sector);
            }
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
            printf("\n");
            break;
        case 9:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Passed integrity check.\n");
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 10:
            printf("DEBUG_LEVEL > 0:\n");
            printf("//Check for integrity.\n");
            printf("//I\n\n");
            printf("Checking integrity of the maps...\n");
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 11:
            printf("DEBUG_LEVEL > 0:\n");
            printf("The memory sectors are %f percent fragmented.\n", _ssm->fragmented);
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 12:
            printf("DEBUG_LEVEL > 0:\n");
            sector = 8 * _ssm->index[0] + _ssm->index[1];
            printf("//Set allocated map sector (%d*8 + %d).\n", _ssm->index[0], _ssm->index[1]);
            printf("//X:%d:%d\n\n", _ssm->index[0], _ssm->index[1]);
            printf("Setting allocated map sector %d\n", sector);
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 13:
            printf("DEBUG_LEVEL > 0:\n");
            sector = 8 * _ssm->index[0] + _ssm->index[1];
            printf("//Set free map sector (%d*8 + %d).\n", _ssm->index[0], _ssm->index[1]);
            printf("//Y:%d:%d\n\n", _ssm->index[0], _ssm->index[1]);
            printf("Setting free map sector %d\n", sector);
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 14:
            printf("\nEND");
            break;
        case 15:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Bad input...\n");
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 16:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Could not find %d contiguous sectors.\n", _ssm->contSectors);
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 17:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Reading input file...\n");
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 18:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Creating stub output...\n");
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 19:
            printf("DEBUG_LEVEL > 1:\n");
            printf("Variable information:\n");
            printf("contSectors = %d\n", _ssm->contSectors);
            printf("index[0] = %d\n", _ssm->index[0]);
            printf("index[1] = %d\n", _ssm->index[1]);
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 20:
            printf("DEBUG_LEVEL > 0:\n");
            sector = 8 * _ssm->index[0] + _ssm->index[1];
            printf("//Get %d contiguous sectors.\n", _ssm->contSectors);
            printf("//G:%d\n\n", _ssm->contSectors);
            printf("There are %d contiguous sectors at sector %d.\n", _ssm->contSectors, sector);
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
            break;
    }
}
