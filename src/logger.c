/*********************************
 *	Sector Space Manager (SSM)
 *	Author: Michael Lombardi
 **********************************/
#include "logger.h"

#include <stdio.h>

#include "config.h"
#include "fsm.h"
#include "global_constants.h"
#include "ssm.h"
#include "ssm_constants.h"

enum { ALLOCATED = 9, FREE = 10 };
enum { FIELD_WIDTH = 9 };
static char MESSAGE_BUFFER[1024];

/**
 * @brief Prints INODE and FREE maps 8 bit sections.
 * For example: 00011111
 * @param[in,out] b bit offset within the byte.
 * @return void
 */
static void print_8_bits(int b) {
    printf("\n");
    for (int j = 0; j < 8; ++j) {
        printf("%-*d", FIELD_WIDTH, b++ * 8);
    }  // end for (j = 0; j < 8; j++)
    printf("\n");
}

/**
 * @brief Prints INODE and FREE maps 8 bit sections.
 * For example: 00011111 11111111 11111111 11111111 11111111 11111111 11111111 11111111
 *              00011111 11111111 11111111 11111111 11111111 11111111 11111111 11111111
 * @param[in] map inode or free map.
 * @param[in] _startByte Byte offset at which to begin the debug trace.
 * @param[in] block_count block count
 * @return void
 */
static void print_128_bits(unsigned char* map, unsigned int _startByte, unsigned int block_count) {
    char byte_array[8];
    int count = 0;
    unsigned int k = _startByte;
    for (unsigned int i = _startByte; i < block_count && i < _startByte + 16; i++) {
        unsigned char tmp = map[i];
        for (int j = 0; j < 8; j++) {
            byte_array[j] = tmp % 2 == 1 ? '1' : '0';
            tmp /= 2;
        }  // end for (j = 0; j < 8; j++)
        for (int j = 0; j < 8; j++) printf("%c", byte_array[j]);
        printf(" ");
        if ((count + 1) % 8 == 0 || i + 1 == block_count) {
            print_8_bits(k);
            k += 8;
        }  // end if(count+1) % 8 = 0 | i+1 = block_count)
        count++;
    }
}

/**
 * @brief Prints SSM maps.
 * @param[in] _ssm the Sector Space Manager
 * @param[in] _startByte the start byte of the maps
 * @return void
 */
static void print_ssm_maps(SSM* _ssm, unsigned int _startByte) {
    printf("//Print 128 contiguous Sectors from Free/Aloc Map starting at Sector (%d).\n",
           _startByte * 8);
    // printf("//P:%d\n\n",_startByte);
    printf("=======================================================================\n");
    printf("FREE MAP\n");
    print_128_bits(_ssm->freeMap, _startByte, SECTOR_BYTES);
    printf("\n");
    printf("ALLOCATED MAP\n");
    print_128_bits(_ssm->alocMap, _startByte, SECTOR_BYTES);
    printf("=======================================================================\n\n\n");
}

/**
 * @brief Prints sector allocation failure message.
 * @param[in] badArray sector table
 * @param[in] message the error message
 * @return void
 */
static void print_sector_failure(unsigned int badArray[][2], const char* message) {
    printf("DEBUG_LEVEL > 0:\n");
    for (unsigned int k = 0; k < MAX_INPUT; k++) {
        if (badArray[k][0] == (unsigned int)(-1)) {
            break;
        }
        int sector = 8 * badArray[k][0] + badArray[k][1];
        printf("%s %d\n", message, sector);
    }
    printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
}

/**
 * @brief Gets a sector number from indices.
 * @param[in] index the fsm or ssm index
 * @return the sector number
 */
static int get_sector_number(unsigned int index[2]) { return 8 * index[0] + index[1]; }

/**
 * @brief Prints an inode.
 * @param[in] _fsm the file sector manager
 * @return void
 */
static void print_inode(FSM* _fsm) {
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
    for (int i = 0; i < 10; i++) {
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
}

/**
 * @brief Print general message
 * @param[in] message message
 * @return void
 */
static void print_message(const char* message) {
    printf("DEBUG_LEVEL > 0:\n");
    printf("%s\n", message);
    printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
}

/**
 * @brief Prints SSM maps.
 * @param[in] _ssm the Sector Space Manager
 * @param[in] mode map mode (ALLOCATED, FREE)
 * @return void
 */
static void print_set_map_sector(SSM* _ssm, int mode) {
    snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
             "//Set allocated map sector (%d*8 + %d).\n//%s:%d:%d\n\nSetting %s map sector %d",
             _ssm->index[0], _ssm->index[1], mode == ALLOCATED ? "X" : "Y", _ssm->index[0],
             _ssm->index[1], mode == ALLOCATED ? "allocated" : "free",
             get_sector_number(_ssm->index));
    print_message(MESSAGE_BUFFER);
}

/**
 * @brief Prints Inode map.
 * @param[in] _fsm the File Sector Manager
 * @param[in] _startByte the start byte of the maps
 * @return void
 */
static void print_inode_map(FSM* _fsm, unsigned int _startByte) {
    printf("DEBUG_LEVEL > 0:\n");
    printf("//Print 128 contiguous Inodes from Inode Map ");
    printf("starting at Inode (%d).\n", _startByte * 8);
    printf("//P:%d\n\n", _startByte);
    printf("===================================");
    printf("====================================\n\n");
    printf("INODE MAP\n");
    print_128_bits(_fsm->iMap, _startByte, INODE_BLOCKS);
    printf("\n");
    printf("===================================");
    printf("====================================\n\n");
}

/**
 * @brief Prints making of the file system messages.
 * @return void
 */
static void print_making_fs(void) {
    snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
             "//Making the File System.\n"
             "//Disk = 3000000, Block = 1024, Inode = 128, Inode Block = 32 blocks...\n"
             "//M\n\n"
             "-> Allocating Boot Block, Super Block, 32 Inode Blocks, and Root (Inode 2)...\n"
             "** Expected Result: 3 Inodes allocated in the Inode Map\n"
             "** Expected Result: 35 Blocks allocated in the Aloc/Free Map");
    print_message(MESSAGE_BUFFER);
    printf("\n");
}

void log_fsm(FSM* _fsm, int _case, unsigned int _startByte) {
    switch (_case) {
        // Print Initialization Message
        case 0:
            print_message("Initializing FSM maps...");
            break;
        // Print Inode Map
        case 1:
            print_inode_map(_fsm, _startByte);
            break;
        case 2:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Allocate %d contiguous inodes.\n//A:%d\n\nAllocating %d contiguous inodes "
                     "at sector %d",
                     _fsm->contInodes, _fsm->contInodes, _fsm->contInodes,
                     get_sector_number(_fsm->index));
            print_message(MESSAGE_BUFFER);
            break;
        // Print Initializing FSM message
        case 3:
            print_message("Initializing FSM...");
            break;
        // Print Allocate Failure method
        case 4:
            print_sector_failure(_fsm->badInode, "Failed to allocate inodes at sector\n");
            break;
        case 5:
            break;
        // Print deallocation message
        case 6:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Deallocate %d contiguous inodes starting at sector "
                     "%d\n//D:%d:%d:%d\n\nDeallocating %d contiguous inodes at sector %d",
                     _fsm->contInodes, _fsm->index[0] * 8 + _fsm->index[1], _fsm->contInodes,
                     _fsm->index[0], _fsm->index[1], _fsm->contInodes,
                     get_sector_number(_fsm->index));
            print_message(MESSAGE_BUFFER);
            break;
        // Print deallocation failure message
        case 7:
            print_sector_failure(_fsm->badInode, "Failed to deallocate inode\n");
            break;
        // Print integrity check failure message
        case 8:
            print_sector_failure(_fsm->badInode, "Failed integrity check at sector\n\n");
            break;
        // Print integrity check pass message
        case 9:
            print_message("Passed integrity check.");
            break;
        // Print integrity check message
        case 10:
            print_message("//Check for integrity.\n//I\n\nChecking integrity of inode map...");
            break;
        case 11:
            break;
        // Print Set inode sector message
        case 12:
            snprintf(
                MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                "//Set inode map sector (%d*8 + %d).\n//X:%d:%d\n\nSetting inode map sector %d",
                _fsm->index[0], _fsm->index[1], _fsm->index[0], _fsm->index[1],
                get_sector_number(_fsm->index));
            print_message(MESSAGE_BUFFER);
            break;
        case 13:
            break;
        // End of input
        case 14:
            printf("\nEND");
            break;
        // Print bad input message
        case 15:
            print_message("Bad input...");
            break;
        // Print unable to find contiguous inodes message
        case 16:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER), "Could not find %d contiguous inodes.",
                     _fsm->contInodes);
            print_message(MESSAGE_BUFFER);
            break;
        // Print read input message
        case 17:
            print_message("Reading input file...");
            break;
        // Print create stub output message
        case 18:
            print_message("Creating stub output...");
            break;
        // Print variable information
        case 19:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "Variable information:\ncontInodes = %d\nindex[0] = %d\nindex[1] = %d",
                     _fsm->contInodes, _fsm->index[0], _fsm->index[1]);
            print_message(MESSAGE_BUFFER);
            break;
        // Print getting inodes message
        case 20:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Get %d contiguous inodes\n//G:%d\n\nThere are %d contiguous inodes at "
                     "sector %d.",
                     _fsm->contInodes, _fsm->contInodes, _fsm->contInodes,
                     get_sector_number(_fsm->index));
            print_message(MESSAGE_BUFFER);
            break;
        // Print making file system message
        case 21:
            print_making_fs();
            break;
        // Print created a file
        case 22:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Create a file\n//C:0\n\nCreated a file...");
            print_message(MESSAGE_BUFFER);
            break;
        // Print opened file
        case 23:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Open a file\n//O:%d\n\nOpenned file at inode %d...\n", _fsm->inodeNum,
                     _fsm->inodeNum);
            print_message(MESSAGE_BUFFER);
            break;
        // Print write to file
        case 24:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Write to file\n//W:%d\n\nWrote to file at inode %d...", _fsm->inodeNum,
                     _fsm->inodeNum);
            print_message(MESSAGE_BUFFER);
            break;
        // Print read from file
        case 25:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Read from file\n//R:%d\n\nRead from file at inode %d..", _fsm->inodeNum,
                     _fsm->inodeNum);
            print_message(MESSAGE_BUFFER);
            break;
        // Print create directory
        case 26:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Create a directory\n//C:1\n\nCreated a directory...");
            print_message(MESSAGE_BUFFER);
            break;
        // Print testing root
        case 27:
            print_message("//Testing root directory\n//T:2\n");
            break;
        case 28:
            print_message("//Listing root directory\n//L:2\n");
            break;
        case 29:
            print_inode(_fsm);
            break;
    }  // end switch (_case)
}

void log_ssm(SSM* _ssm, int _case, int _startByte) {
    switch (_case) {
        case 0:
            print_message("Initializing SSM maps...");
            break;
        case 1:
            print_ssm_maps(_ssm, (unsigned int)_startByte);
            break;
        case 2:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Allocate %d contiguous sectors.\n//A:%d\n\nAllocating %d contiguous "
                     "sectors at sector %d...",
                     _ssm->contSectors, _ssm->contSectors, _ssm->contSectors,
                     get_sector_number(_ssm->index));
            print_message(MESSAGE_BUFFER);
            break;
        case 3:
            print_message("Initializing SSM...");
            break;
        case 4:
            print_sector_failure(_ssm->badSector, "Failed to allocate sectors at sector");
            break;
        case 5:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Check for degree of fragmentation.\n//F\n\nThe degree of memory "
                     "fragmentation is %10.10f ",
                     _ssm->fragmented);
            print_message(MESSAGE_BUFFER);
            break;
        case 6:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Deallocate %d contiguous sectors starting at sector %d\n//D:%d:%d:%d\n\n"
                     "Deallocating %d contiguous sectors at sector %d...",
                     _ssm->contSectors, _ssm->index[0] * 8 + _ssm->index[1], _ssm->contSectors,
                     _ssm->index[0], _ssm->index[1], _ssm->contSectors,
                     get_sector_number(_ssm->index));
            print_message(MESSAGE_BUFFER);
            break;
        case 7:
            print_sector_failure(_ssm->badSector, "Failed to deallocate sector");
            break;
        case 8:
            print_sector_failure(_ssm->badSector, "Failed integrity check at sector");
            printf("\n");
            break;
        case 9:
            print_message("Passed integrity check.");
            break;
        case 10:
            print_message("//Check for integrity.\n//I\n\nChecking integrity of the maps...");
            break;
        case 11:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "The memory sectors are %f percent fragmented.\n", _ssm->fragmented);
            print_message(MESSAGE_BUFFER);
            break;
        case 12:
            print_set_map_sector(_ssm, ALLOCATED);
            break;
        case 13:
            print_set_map_sector(_ssm, FREE);
            break;
        case 14:
            printf("\nEND");
            break;
        case 15:
            print_message("Bad input...");
            break;
        case 16:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "Could not find %d contiguous sectors.\n", _ssm->contSectors);
            print_message(MESSAGE_BUFFER);
            break;
        case 17:
            print_message("Reading input file...");
            break;
        case 18:
            print_message("Creating stub output...");
            break;
        case 19:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "Variable information:\ncontSectors = %d\nindex[0] = %d\nindex[1] = %d",
                     _ssm->contSectors, _ssm->index[0], _ssm->index[1]);
            print_message(MESSAGE_BUFFER);
            break;
        case 20:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Get %d contiguous sectors.\n//G:%d\n\nThere are %d contiguous sectors at "
                     "sector %d.",
                     _ssm->contSectors, _ssm->contSectors, _ssm->contSectors,
                     get_sector_number(_ssm->index));
            print_message(MESSAGE_BUFFER);
            break;
    }
}
