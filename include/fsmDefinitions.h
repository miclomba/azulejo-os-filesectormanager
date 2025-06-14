/*******************************************************************************
 * File Sector Manager (FSM)
 * Author: Michael Lombardi
 *******************************************************************************/
#ifndef FSM_DEFINITIONS_H
#define FSM_DEFINITIONS_H

#include "config.h"

#ifndef MAX_DISK_SIZE
#define MAX_DISK_SIZE (3000000)
#endif

#ifndef MAX_INODE_BLOCKS
#define MAX_INODE_BLOCKS (128)
#endif

#ifndef MAX_BLOCK_SIZE
#define MAX_BLOCK_SIZE (1024)
#endif

unsigned int DISK_SIZE;     // 3000000
unsigned int BLOCK_SIZE;    // 1024
unsigned int INODE_SIZE;    //(BLOCK_SIZE / 8)
unsigned int INODE_BLOCKS;  // 32
unsigned int INODE_COUNT;   // 8*INODE_BLOCKS

unsigned int PTRS_PER_BLOCK;
unsigned int S_INDIRECT_BLOCKS;
unsigned int S_INDIRECT_SIZE;
unsigned int D_INDIRECT_BLOCKS;
unsigned int D_INDIRECT_SIZE;
unsigned int T_INDIRECT_BLOCKS;
unsigned int T_INDIRECT_SIZE;

/**
 * @brief Initializes file system constants.
 * Sets global or static constants required for the File Sector Manager,
 * such as disk size, block size, and inode layout.
 * @param[in] _DISK_SIZE Total disk size in bytes.
 * @param[in] _BLOCK_SIZE Block size in bytes.
 * @param[in] _INODE_SIZE Size of a single inode in bytes.
 * @param[in] _INODE_BLOCKS Number of blocks allocated for inodes.
 * @param[in] _INODE_COUNT Total number of inodes.
 * @return void
 */
void initializeFsmConstants(unsigned int _DISK_SIZE, unsigned int _BLOCK_SIZE,
                            unsigned int _INODE_SIZE, unsigned int _INODE_BLOCKS,
                            unsigned int _INODE_COUNT);

#endif  // FSM_DEFINITIONS_H