/*******************************************************************************
 * File Sector Manager (FSM)
 * Author: Michael Lombardi
 *******************************************************************************/

#define _GNU_SOURCE

#ifndef MAX_DISK_SIZE
#define MAX_DISK_SIZE (3000000)
#endif

#ifndef MAX_INODE_BLOCKS
#define MAX_INODE_BLOCKS (128)
#endif

#ifndef MAX_BLOCK_SIZE
#define MAX_BLOCK_SIZE (1024)
#endif

unsigned int DISK_SIZE;    // 3000000
unsigned int BLOCK_SIZE;   // 1024
unsigned int INODE_SIZE;   //(BLOCK_SIZE / 8)
unsigned int INODE_BLOCKS; // 32
unsigned int INODE_COUNT;  // 8*INODE_BLOCKS

unsigned int PTRS_PER_BLOCK;
unsigned int S_INDIRECT_BLOCKS;
unsigned int S_INDIRECT_SIZE;
unsigned int D_INDIRECT_BLOCKS;
unsigned int D_INDIRECT_SIZE;
unsigned int T_INDIRECT_BLOCKS;
unsigned int T_INDIRECT_SIZE;

void initializeFsmConstants(unsigned int _DISK_SIZE,
                            unsigned int _BLOCK_SIZE,
                            unsigned int _INODE_SIZE,
                            unsigned int _INODE_BLOCKS,
                            unsigned int _INODE_COUNT)
{

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
