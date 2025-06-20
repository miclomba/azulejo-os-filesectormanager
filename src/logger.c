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

static Inode inode;

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
 * @param[in] _startByte the start byte of the maps
 * @return void
 */
static void print_ssm_maps(unsigned int _startByte) {
    printf("//Print 128 contiguous Sectors from Free/Aloc Map starting at Sector (%d).\n",
           _startByte * 8);
    // printf("//P:%d\n\n",_startByte);
    printf("=======================================================================\n");
    printf("FREE MAP\n");
    print_128_bits(ssm->freeMap, _startByte, SECTOR_BYTES);
    printf("\n");
    printf("ALLOCATED MAP\n");
    print_128_bits(ssm->alocMap, _startByte, SECTOR_BYTES);
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
static int get_sector_number(unsigned int index[2]) { return BITS_PER_BYTE * index[0] + index[1]; }

/**
 * @brief Prints an inode.
 * @return void
 */
static void print_inode(void) {
    printf("DEBUG_LEVEL > 0:\n");
    printf("//Display Info of File (Inode %d)\n", fsm->inodeNum);
    printf("//I:%d\n\n", fsm->inodeNum);

    fs_open_file(fsm->inodeNum, &inode);

    if (inode.fileType == 1) {
        printf("-> fileType = FILE\n");
    }  // end if (inode.fileType == 1)
    else if (inode.fileType == 2) {
        printf("-> fileType = DIRECTORY\n");
    }  // end else
    printf("-> fileSize = %d\n", inode.fileSize);
    printf("-> permissions = %d\n", inode.permissions);
    printf("-> linkCount = %d\n", inode.linkCount);
    printf("-> dataBlocks = %d\n", inode.dataBlocks);
    printf("-> owner = %d\n", inode.owner);
    printf("-> status = %d\n\n", inode.status);
    for (int i = 0; i < 10; i++) {
        if (inode.directPtr[i] == (unsigned int)(-1)) {
            printf("-> directPtr[%d] = %d\n", i, inode.directPtr[i]);
        }  // end if(inode.directPtr[i] = (unsigned int))
        else {
            printf("-> directPtr[%d] = %d\n", i, inode.directPtr[i] / BLOCK_SIZE);
        }  // end else
    }  // end for (i = 0; i < 10; i++)
    if (inode.sIndirect == (unsigned int)(-1)) {
        printf("\n-> sIndirect = %d\n", inode.sIndirect);
    }  // end if if(inode.sIndirect == (unsigned int)(-1)
    else {
        printf("\n-> sIndirect = %d\n", inode.sIndirect / BLOCK_SIZE);
    }  // end elsee
    if (inode.dIndirect == (unsigned int)(-1)) {
        printf("-> dIndirect = %d\n", inode.dIndirect);
    }  // end if (inode.dIndirect == (unsigned int)(-1))
    else {
        printf("-> dIndirect = %d\n", inode.dIndirect / BLOCK_SIZE);
    }  // end else
    if (inode.tIndirect == (unsigned int)(-1)) {
        printf("-> tIndirect = %d\n", inode.tIndirect);
    }  // end if (inode.tIndirect == (unsigned int)(-1))
    else {
        printf("-> tIndirect = %d\n", inode.tIndirect / BLOCK_SIZE);
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
 * @param[in] mode map mode (ALLOCATED, FREE)
 * @return void
 */
static void print_set_map_sector(int mode) {
    snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
             "//Set allocated map sector (%d*8 + %d).\n//%s:%d:%d\n\nSetting %s map sector %d",
             ssm_get_sector_offset_byte_index(), ssm_get_sector_offset_bit_index(),
             mode == ALLOCATED ? "X" : "Y", ssm_get_sector_offset_byte_index(),
             ssm_get_sector_offset_bit_index(), mode == ALLOCATED ? "allocated" : "free",
             get_sector_number(ssm->index));
    print_message(MESSAGE_BUFFER);
}

/**
 * @brief Prints Inode map.
 * @param[in] _startByte the start byte of the maps
 * @return void
 */
static void print_inode_map(unsigned int _startByte) {
    printf("DEBUG_LEVEL > 0:\n");
    printf("//Print 128 contiguous Inodes from Inode Map ");
    printf("starting at Inode (%d).\n", _startByte * 8);
    printf("//P:%d\n\n", _startByte);
    printf("===================================");
    printf("====================================\n\n");
    printf("INODE MAP\n");
    print_128_bits(fsm->iMap, _startByte, INODE_BLOCKS);
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

/**
 * @brief Prints debug information for the File Sector Manager (FSM) file operations.
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _case Identifier for the type of debug information to print.
 * @return void
 */
static void log_fsm_file(LoggerFSMOption _case) {
    switch (_case) {
        case FSM_FILE_CREATE:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Create a file\n//C:0\n\nCreated a file...");
            print_message(MESSAGE_BUFFER);
            break;
        // Print opened file
        case FSM_FILE_OPEN:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Open a file\n//O:%d\n\nOpenned file at inode %d...\n", fsm->inodeNum,
                     fsm->inodeNum);
            print_message(MESSAGE_BUFFER);
            break;
        // Print write to file
        case FSM_FILE_WRITE:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Write to file\n//W:%d\n\nWrote to file at inode %d...", fsm->inodeNum,
                     fsm->inodeNum);
            print_message(MESSAGE_BUFFER);
            break;
        // Print read from file
        case FSM_FILE_READ:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Read from file\n//R:%d\n\nRead from file at inode %d..", fsm->inodeNum,
                     fsm->inodeNum);
            print_message(MESSAGE_BUFFER);
            break;
        // Print create directory
        case FSM_FILE_MKDIR:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Create a directory\n//C:1\n\nCreated a directory...");
            print_message(MESSAGE_BUFFER);
            break;
        // Print testing root
        case FSM_FILE_ROOT_TEST:
            print_message("//Testing root directory\n//T:2\n");
            break;
        case FSM_FILE_ROOT_LS:
            print_message("//Listing root directory\n//L:2\n");
            break;
        default:
            break;
    }
}

/**
 * @brief Prints debug information for the File Sector Manager (FSM) inode operations.
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _case Identifier for the type of debug information to print.
 * @return void
 */
static void log_fsm_inode(LoggerFSMOption _case, unsigned int _startByte) {
    switch (_case) {
        // Print Inode Map
        case FSM_INODE_ARRAY:
            print_inode_map(_startByte);
            break;
        // Print Set inode sector message
        case FSM_INODE_SET:
            snprintf(
                MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                "//Set inode map sector (%d*8 + %d).\n//X:%d:%d\n\nSetting inode map sector %d",
                fsm->index[0], fsm->index[1], fsm->index[0], fsm->index[1],
                get_sector_number(fsm->index));
            print_message(MESSAGE_BUFFER);
            break;
        // Print unable to find contiguous inodes message
        case FSM_INODE_NOT_FOUND:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER), "Could not find %d contiguous inodes.",
                     fsm->contInodes);
            print_message(MESSAGE_BUFFER);
            break;
        // Print getting inodes message
        case FSM_INODE_GET:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Get %d contiguous inodes\n//G:%d\n\nThere are %d contiguous inodes at "
                     "sector %d.",
                     fsm->contInodes, fsm->contInodes, fsm->contInodes,
                     get_sector_number(fsm->index));
            print_message(MESSAGE_BUFFER);
            break;
        // Print created a file
        case FSM_INODE_PRINT:
            print_inode();
            break;
        // Print variable information
        case FSM_INFO:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "Variable information:\ncontInodes = %d\nindex[0] = %d\nindex[1] = %d",
                     fsm->contInodes, fsm->index[0], fsm->index[1]);
            print_message(MESSAGE_BUFFER);
            break;
        default:
            break;
    }
}

/**
 * @brief Prints debug information for the File Sector Manager (FSM) alloc operations.
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _case Identifier for the type of debug information to print.
 * @return void
 */
static void log_fsm_alloc(LoggerFSMOption _case) {
    switch (_case) {
        case FSM_ALLOC_INODES:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Allocate %d contiguous inodes.\n//A:%d\n\nAllocating %d contiguous inodes "
                     "at sector %d",
                     fsm->contInodes, fsm->contInodes, fsm->contInodes,
                     get_sector_number(fsm->index));
            print_message(MESSAGE_BUFFER);
            break;
        // Print Allocate Failure method
        case FSM_ALLOC_FAIL:
            print_sector_failure(fsm->badInode, "Failed to allocate inodes at sector\n");
            break;
        // Print deallocation message
        case FSM_DEALLOC_INODES:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Deallocate %d contiguous inodes starting at sector "
                     "%d\n//D:%d:%d:%d\n\nDeallocating %d contiguous inodes at sector %d",
                     fsm->contInodes, fsm->index[0] * 8 + fsm->index[1], fsm->contInodes,
                     fsm->index[0], fsm->index[1], fsm->contInodes, get_sector_number(fsm->index));
            print_message(MESSAGE_BUFFER);
            break;
        // Print deallocation failure message
        case FSM_DEALLOC_FAIL:
            print_sector_failure(fsm->badInode, "Failed to deallocate inode\n");
            break;
        default:
            break;
    }
}

/**
 * @brief Prints debug information for the File Sector Manager (FSM) integrity operations.
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _case Identifier for the type of debug information to print.
 * @return void
 */
static void log_fsm_integrity(LoggerFSMOption _case) {
    switch (_case) {
        // Print integrity check failure message
        case FSM_INTEG_FAIL:
            print_sector_failure(fsm->badInode, "Failed integrity check at sector\n\n");
            break;
        // Print integrity check pass message
        case FSM_INTEG_PASS:
            print_message("Passed integrity check.");
            break;
        // Print integrity check message
        case FSM_INTEG_CHECK:
            print_message("//Check for integrity.\n//I\n\nChecking integrity of inode map...");
            break;
        default:
            break;
    }
}

/**
 * @brief Prints debug information for the File Sector Manager (FSM) init operations.
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _case Identifier for the type of debug information to print.
 * @return void
 */
static void log_fsm_init(LoggerFSMOption _case) {
    switch (_case) {
        // Print Initialization Message
        case FSM_INIT_MAPS:
            print_message("Initializing FSM maps...");
            break;
        // Print Initializing FSM message
        case FSM_INIT:
            print_message("Initializing FSM...");
            break;
        // Print making file system message
        case FSM_MAKE:
            print_making_fs();
            break;
        default:
            break;
    }
}

/**
 * @brief Prints debug information for the File Sector Manager (FSM) io operations.
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _case Identifier for the type of debug information to print.
 * @return void
 */
static void log_fsm_io(LoggerFSMOption _case) {
    switch (_case) {
        // End of input
        case FSM_END:
            printf("\nEND");
            break;
        // Print bad input message
        case FSM_INVALID_INPUT:
            print_message("Bad input...");
            break;
        // Print read input message
        case FSM_INPUT:
            print_message("Reading input file...");
            break;
        // Print create stub output message
        case FSM_OUTPUT_STUB:
            print_message("Creating stub output...");
            break;
        default:
            break;
    }
}

void log_fsm(LoggerFSMOption _case, unsigned int _startByte) {
    log_fsm_file(_case);
    log_fsm_inode(_case, _startByte);
    log_fsm_alloc(_case);
    log_fsm_integrity(_case);
    log_fsm_init(_case);
    log_fsm_io(_case);
}

/**
 * @brief Prints debug information for the Sector Space Manager (SSM) file operations.
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _case Identifier for the type of debug information to print.
 * @return void
 */
static void log_ssm_file(LoggerSSMOption _case) {
    switch (_case) {
        case SSM_FILE_OPEN:
            print_message("Reading input file...");
            break;
        case SSM_FILE_STUB_OUTPUT:
            print_message("Creating stub output...");
            break;
        default:
            break;
    }
}

/**
 * @brief Prints debug information for the Sector Space Manager (SSM) map operations.
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _case Identifier for the type of debug information to print.
 * @param[in] _startByte Byte offset at which to begin the debug trace.
 * @return void
 */
static void log_ssm_maps(LoggerSSMOption _case, int _startByte) {
    switch (_case) {
        case SSM_MAPS_PRINT:
            print_ssm_maps((unsigned int)_startByte);
            break;
        case SSM_MAPS_ALLOC:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Allocate %d contiguous sectors.\n//A:%d\n\nAllocating %d contiguous "
                     "sectors at sector %d...",
                     ssm->contSectors, ssm->contSectors, ssm->contSectors,
                     get_sector_number(ssm->index));
            print_message(MESSAGE_BUFFER);
            break;
        case SSM_MAPS_ALLOC_FAIL:
            print_sector_failure(ssm->badSector, "Failed to allocate sectors at sector");
            break;
        case SSM_MAPS_DEALLOC:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Deallocate %d contiguous sectors starting at sector %d\n//D:%d:%d:%d\n\n"
                     "Deallocating %d contiguous sectors at sector %d...",
                     ssm->contSectors,
                     ssm_get_sector_offset_byte_index() * BITS_PER_BYTE +
                         ssm_get_sector_offset_bit_index(),
                     ssm->contSectors, ssm_get_sector_offset_byte_index(),
                     ssm_get_sector_offset_bit_index(), ssm->contSectors,
                     get_sector_number(ssm->index));
            print_message(MESSAGE_BUFFER);
            break;
        case SSM_MAPS_DEALLOC_FAIL:
            print_sector_failure(ssm->badSector, "Failed to deallocate sector");
            break;
        case SSM_MAPS_SET:
            print_set_map_sector(ALLOCATED);
            break;
        case SSM_MAPS_UNSET:
            print_set_map_sector(FREE);
            break;
        case SSM_MAPS_GET:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Get %d contiguous sectors.\n//G:%d\n\nThere are %d contiguous sectors at "
                     "sector %d.",
                     ssm->contSectors, ssm->contSectors, ssm->contSectors,
                     get_sector_number(ssm->index));
            print_message(MESSAGE_BUFFER);
            break;
        case SSM_MAPS_NOT_FOUND:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "Could not find %d contiguous sectors.\n", ssm->contSectors);
            print_message(MESSAGE_BUFFER);
            break;
        case SSM_INFO:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "Variable information:\ncontSectors = %d\nindex[0] = %d\nindex[1] = %d",
                     ssm->contSectors, ssm_get_sector_offset_byte_index(),
                     ssm_get_sector_offset_bit_index());
            print_message(MESSAGE_BUFFER);
            break;
        default:
            break;
    }
}

/**
 * @brief Prints debug information for the Sector Space Manager (SSM) init operations.
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _case Identifier for the type of debug information to print.
 * @return void
 */
static void log_ssm_init(LoggerSSMOption _case) {
    switch (_case) {
        case SSM_INIT_MAPS:
            print_message("Initializing SSM maps...");
            break;
        case SSM_INIT:
            print_message("Initializing SSM...");
            break;
        default:
            break;
    }
}

/**
 * @brief Prints debug information for the Sector Space Manager (SSM) integrity operations.
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _case Identifier for the type of debug information to print.
 * @return void
 */
static void log_ssm_integrity(LoggerSSMOption _case) {
    switch (_case) {
        case SSM_FRAGMENTATION:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "//Check for degree of fragmentation.\n//F\n\nThe degree of memory "
                     "fragmentation is %10.10f ",
                     ssm->fragmented);
            print_message(MESSAGE_BUFFER);
            break;
        case SSM_FRAGMENTATION_MSG:
            snprintf(MESSAGE_BUFFER, sizeof(MESSAGE_BUFFER),
                     "The memory sectors are %f percent fragmented.\n", ssm->fragmented);
            print_message(MESSAGE_BUFFER);
            break;
        case SSM_MAPS_INTEGRITY_FAIL:
            print_sector_failure(ssm->badSector, "Failed integrity check at sector");
            printf("\n");
            break;
        case SSM_MAPS_INTEGRITY_PASS:
            print_message("Passed integrity check.");
            break;
        case SSM_MAPS_INTEGRITY:
            print_message("//Check for integrity.\n//I\n\nChecking integrity of the maps...");
            break;
        default:
            break;
    }
}

/**
 * @brief Prints debug information for the Sector Space Manager (SSM) io operations.
 * Logs diagnostic output based on the specified debug case and byte offset.
 * @param[in] _case Identifier for the type of debug information to print.
 * @return void
 */
static void log_ssm_io(LoggerSSMOption _case) {
    switch (_case) {
        case SSM_END:
            printf("\nEND");
            break;
        case SSM_INVALID_INPUT:
            print_message("Bad input...");
            break;
        default:
            break;
    }
}

void log_ssm(LoggerSSMOption _case, int _startByte) {
    log_ssm_file(_case);
    log_ssm_maps(_case, _startByte);
    log_ssm_init(_case);
    log_ssm_integrity(_case);
    log_ssm_io(_case);
}
