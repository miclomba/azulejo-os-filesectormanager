/*********************************
 *	Sector Space Manager (SSM)
 *	Author: Michael Lombardi
 **********************************/
#include <stdio.h>

#include "config.h"
#include "fileSectorMgr.h"
#include "gDefinitions.h"
#include "sectorSpaceMgr.h"
#include "ssmDefinitions.h"

void log_fsm(FileSectorMgr *_fsm, int _case, unsigned int _startByte) {
    unsigned int i, j, k;
    int count;
    int sector;
    unsigned char tmp;
    char byteArray[8];
    switch (_case) {
        // Print Initialization Message
        case 0:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Initializing FSM maps...\n");
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print Inode Map
        case 1:
            printf("DEBUG_LEVEL > 0:\n");
            printf("//Print 128 contiguous Inodes from Inode Map ");
            printf("starting at Inode (%d).\n", _startByte * 8);
            printf("//P:%d\n\n", _startByte);
            printf("===================================");
            printf("====================================\n\n");
            printf("INODE MAP\n");
            k = _startByte;
            count = 0;
            for (i = _startByte; i < INODE_BLOCKS && i < _startByte + 16; i++) {
                tmp = _fsm->iMap[i];
                for (j = 0; j < 8; j++) {
                    if (tmp % 2 == 1)
                        byteArray[j] = '1';
                    else
                        byteArray[j] = '0';
                    tmp /= 2;
                }  // end for (j = 0; j < 8; j++)
                for (j = 0; j < 8; j++) printf("%c", byteArray[j]);
                printf(" ");
                if ((count + 1) % 8 == 0 || i + 1 == INODE_BLOCKS) {
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
                    }  // end for (j = 0; j < 8; j++)
                    printf("\n");
                }  // end if(count+1) % 8 = 0 | i+1 = INODE_BLOCKS)
                count++;
            } /*for (i = _startByte; i < INODE_BLOCKS &&
                                              i < _startByte + 16; i++)*/
            printf("\n");
            printf("===================================");
            printf("====================================\n\n");
            break;
        case 2:
            printf("DEBUG_LEVEL > 0:\n");
            sector = 8 * _fsm->index[0] + _fsm->index[1];
            printf("//Allocate %d contiguous inodes.\n", _fsm->contInodes);
            printf("//A:%d\n\n", _fsm->contInodes);
            printf("Allocating %d contiguous inodes at sector %d\n", _fsm->contInodes, sector);
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print Initializing FSM message
        case 3:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Initializing FSM...\n");
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print Allocate Failure method
        case 4:
            printf("DEBUG_LEVEL > 0:\n");
            for (k = 0; k < MAX_INPUT; k++) {
                if (_fsm->badInode[k][0] == (unsigned int)(-1)) {
                    break;
                }  // end if(_fsm->badInode[k][0] = (unsigned int)
                sector = 8 * _fsm->badInode[k][0] + _fsm->badInode[k][1];
                printf("Failed to allocate inodes at ");
                printf("sector %d\n", sector);
            }  // end for (k = 0; k < MAX_INPUT; k++)
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 5:
            break;
        // Print deallocation message
        case 6:
            printf("DEBUG_LEVEL > 0:\n");
            sector = 8 * _fsm->index[0] + _fsm->index[1];
            printf("//Deallocate ");
            printf("%d contiguous inodes starting at sector %d\n", _fsm->contInodes,
                   _fsm->index[0] * 8 + _fsm->index[1]);
            printf("//D:%d:%d:%d\n\n", _fsm->contInodes, _fsm->index[0], _fsm->index[1]);
            printf("Deallocating %d contiguous inodes at sector %d", _fsm->contInodes, sector);
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print deallocation failure message
        case 7:
            printf("DEBUG_LEVEL > 0:\n");
            for (k = 0; k < MAX_INPUT; k++) {
                if (_fsm->badInode[k][0] == (unsigned int)(-1)) {
                    break;
                }  // end if (_fsm->badInode[k][0] = (unsigned int)
                sector = 8 * _fsm->badInode[k][0] + _fsm->badInode[k][1];
                printf("Failed to deallocate ");
                printf("inode %d\n", sector);
            }  // end for (k = 0; k < MAX_INPUT; k++)
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print integrity check failure message
        case 8:
            printf("DEBUG_LEVEL > 0:\n");
            for (k = 0; k < MAX_INPUT; k++) {
                if (_fsm->badInode[k][0] == (unsigned int)(-1)) {
                    break;
                }  // end if (_fsm->badInode[k][0] = (unsigned int)
                sector = 8 * _fsm->badInode[k][0] + _fsm->badInode[k][1];
                printf("Failed integrity check at sector ");
                printf("%d\n", sector);
            }  // end for (k = 0; k < MAX_INPUT; k++)
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            printf("\n");
            break;
        // Print integrity check pass message
        case 9:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Passed integrity check.\n");
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print integrity check message
        case 10:
            printf("DEBUG_LEVEL > 0:\n");
            printf("//Check for integrity.\n");
            printf("//I\n\n");
            printf("Checking integrity of inode map...\n");
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 11:
            break;
        // Print Set inode sector message
        case 12:
            printf("DEBUG_LEVEL > 0:\n");
            sector = 8 * _fsm->index[0] + _fsm->index[1];
            printf("//Set inode map sector (%d*8 + %d).\n", _fsm->index[0], _fsm->index[1]);
            printf("//X:%d:%d\n\n", _fsm->index[0], _fsm->index[1]);
            printf("Setting inode map sector %d\n", sector);
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        case 13:
            break;
        case 14:
            printf("\nEND");
            break;
        // Print bad input message
        case 15:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Bad input...\n");
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print unable to find contiguous inodes message
        case 16:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Could not find %d contiguous inodes.\n", _fsm->contInodes);
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print read input message
        case 17:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Reading input file...\n");
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print create stub output message
        case 18:
            printf("DEBUG_LEVEL > 0:\n");
            printf("Creating stub output...\n");
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print variable information
        case 19:
            printf("DEBUG_LEVEL > 1:\n");
            printf("Variable information:\n");
            printf("contInodes = %d\n", _fsm->contInodes);
            printf("index[0] = %d\n", _fsm->index[0]);
            printf("index[1] = %d\n", _fsm->index[1]);
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print getting inodes message
        case 20:
            printf("DEBUG_LEVEL > 0:\n");
            sector = 8 * _fsm->index[0] + _fsm->index[1];
            printf("//Get %d contiguous inodes\n", _fsm->contInodes);
            printf("//G:%d\n\n", _fsm->contInodes);
            printf("There are %d contiguous inodes at sector %d.\n", _fsm->contInodes, sector);
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print making file system message
        case 21:
            printf("DEBUG_LEVEL > 0:\n");
            printf("//Making the File System.\n");
            printf("//Disk = 3000000, Block = 1024, Inode = 128, ");
            printf("Inode Block = 32 blocks...\n");
            printf("//M\n\n");
            printf("-> Allocating Boot Block, Super Block, 32 ");
            printf("Inode Blocks, and Root (Inode 2)...\n");
            printf("** Expected Result: 3 Inodes allocated in the");
            printf(" Inode Map\n");
            printf("** Expected Result: 35 Blocks allocated in ");
            printf("the Aloc/Free Map\n");
            printf("- - - - - - - - - - - - - - - - - - - - - - -");
            printf(" - - - - - - - - - - - - -\n\n\n");
            break;
        // Print created a file
        case 22:
            printf("DEBUG_LEVEL > 0:\n");
            printf("//Create a file\n");
            printf("//C:0\n\n");
            printf("Created a file...\n");
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print opened file
        case 23:
            printf("DEBUG_LEVEL > 0:\n");
            printf("//Open a file\n");
            printf("//O:%d\n\n", _fsm->inodeNum);
            printf("Openned file at inode %d...\n", _fsm->inodeNum);
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print write to file
        case 24:
            printf("DEBUG_LEVEL > 0:\n");
            printf("//Write to file\n");
            printf("//W:%d\n\n", _fsm->inodeNum);
            printf("Wrote to file at inode %d...\n", _fsm->inodeNum);
            break;
        // Print read from file
        case 25:
            printf("DEBUG_LEVEL > 0:\n");
            printf("//Read from file\n");
            printf("//R:%d\n\n", _fsm->inodeNum);
            printf("Read from file at inode %d..\n", _fsm->inodeNum);
            break;
        // Print create directory
        case 26:
            printf("DEBUG_LEVEL > 0:\n");
            printf("//Create a directory\n");
            printf("//C:1\n\n");
            printf("Created a directory...\n");
            printf("- - - - - - - - - - - - - - - - - - ");
            printf("- - - - - - - - - - - - - - - - - -\n\n");
            break;
        // Print testing root
        case 27:
            printf("DEBUG_LEVEL > 0:\n");
            printf("//Testing root directory\n");
            printf("//T:2\n\n");
            break;
        case 28:
            printf("DEBUG_LEVEL > 0:\n");
            printf("//Listing root directory\n");
            printf("//L:2\n\n");
            break;
        case 29:
            printf("DEBUG_LEVEL > 0:\n");
            printf("//Display Info of File (Inode %d)\n", _fsm->inodeNum);
            printf("//I:%d\n\n", _fsm->inodeNum);
            if (_fsm->inode.fileType == 1) {
                printf("-> fileType = FILE\n");
            }  // end if (_fsm->inode.fileType == 1)
            else if (_fsm->inode.fileType == 2) {
                printf("-> fileType = DIRECTORY\n");
            }  // end else
            printf("-> fileSize = %d\n", _fsm->inode.fileSize);
            printf("-> permissions = %d\n", _fsm->inode.permissions);
            printf("-> linkCount = %d\n", _fsm->inode.linkCount);
            printf("-> dataBlocks = %d\n", _fsm->inode.dataBlocks);
            printf("-> owner = %d\n", _fsm->inode.owner);
            printf("-> status = %d\n\n", _fsm->inode.status);
            for (i = 0; i < 10; i++) {
                if (_fsm->inode.directPtr[i] == (unsigned int)(-1)) {
                    printf("-> directPtr[%d] = %d\n", i, _fsm->inode.directPtr[i]);
                }  // end if(_fsm->inode.directPtr[i] = (unsigned int))
                else {
                    printf("-> directPtr[%d] = %d\n", i, _fsm->inode.directPtr[i] / BLOCK_SIZE);
                }  // end else
            }  // end for (i = 0; i < 10; i++)
            if (_fsm->inode.sIndirect == (unsigned int)(-1)) {
                printf("\n-> sIndirect = %d\n", _fsm->inode.sIndirect);
            }  // end if if(_fsm->inode.sIndirect == (unsigned int)(-1)
            else {
                printf("\n-> sIndirect = %d\n", _fsm->inode.sIndirect / BLOCK_SIZE);
            }  // end elsee
            if (_fsm->inode.dIndirect == (unsigned int)(-1)) {
                printf("-> dIndirect = %d\n", _fsm->inode.dIndirect);
            }  // end if (_fsm->inode.dIndirect == (unsigned int)(-1))
            else {
                printf("-> dIndirect = %d\n", _fsm->inode.dIndirect / BLOCK_SIZE);
            }  // end else
            if (_fsm->inode.tIndirect == (unsigned int)(-1)) {
                printf("-> tIndirect = %d\n", _fsm->inode.tIndirect);
            }  // end if (_fsm->inode.tIndirect == (unsigned int)(-1))
            else {
                printf("-> tIndirect = %d\n", _fsm->inode.tIndirect / BLOCK_SIZE);
            }  // end else
            printf("- - - - - - - - - - - - - - - - - - - - - - -");
            printf(" - - - - - - - - - - - - -\n\n\n");
            break;
    }  // end switch (_case)
}

void log_ssm(SecSpaceMgr *_ssm, int _case, int _startByte) {
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
