/*******************************************************************************
 * File Sector Manager (FSM)
 * Author: Michael Lombardi
 *******************************************************************************/
#include "fsmDefinitions.h"

#include "config.h"

void initializeFsmConstants(unsigned int _DISK_SIZE, unsigned int _BLOCK_SIZE,
                            unsigned int _INODE_SIZE, unsigned int _INODE_BLOCKS,
                            unsigned int _INODE_COUNT) {
    DISK_SIZE = _DISK_SIZE;
    BLOCK_SIZE = _BLOCK_SIZE;
    INODE_SIZE = _INODE_SIZE;
    INODE_BLOCKS = _INODE_BLOCKS;
    INODE_COUNT = _INODE_COUNT;

    PTRS_PER_BLOCK = (BLOCK_SIZE / 4);
    S_INDIRECT_BLOCKS = (BLOCK_SIZE / 4);
    S_INDIRECT_SIZE = (S_INDIRECT_BLOCKS * BLOCK_SIZE + 10 * BLOCK_SIZE);
    D_INDIRECT_BLOCKS = ((BLOCK_SIZE / 4) * (BLOCK_SIZE / 4));
    D_INDIRECT_SIZE = (D_INDIRECT_BLOCKS * BLOCK_SIZE + S_INDIRECT_SIZE);
    T_INDIRECT_BLOCKS = ((BLOCK_SIZE / 4) * (BLOCK_SIZE / 4) * (BLOCK_SIZE / 4));
    T_INDIRECT_SIZE = (T_INDIRECT_BLOCKS * BLOCK_SIZE + D_INDIRECT_SIZE);
}