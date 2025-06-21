/******************************************************************************
 *   File Sector Manager (FSM)
 *   Author: Michael Lombardi
 ******************************************************************************/
#ifndef I_NODE_H
#define I_NODE_H
#include <stdio.h>

#include "config.h"
#include "fsm_constants.h"

typedef struct Inode {
    // struct location for fileType
    unsigned int fileType;
    // struct location for fileSize
    unsigned int fileSize;
    // struct location for permissions
    unsigned int permissions;
    // struct location for linkCount
    unsigned int linkCount;
    // struct location for dataBlocks
    unsigned int dataBlocks;
    unsigned int owner;
    // struct location for status
    unsigned int status;
    // struct location for direct pointers
    unsigned int directPtr[10];
    // struct location for single indirect disk block
    unsigned int sIndirect;
    // struct location for double indirect disk block
    unsigned int dIndirect;
    // struct location for triple indirect disk block
    unsigned int tIndirect;
} Inode;

typedef struct InodeMetadata {
    unsigned int map_offset[2];
    FILE *iMapHandle;
    unsigned char iMap[MAX_INODE_BLOCKS];
    unsigned int id;
} InodeMetadata;

extern Inode inode;
extern InodeMetadata inode_meta;

/**
 * @brief Initializes the inode memory pointers.
 * @param[out] _inode Pointer to an Inode structure to store the result.
 * @return void
 */
void inode_init_ptrs(Inode *_inode);

/**
 * @brief Initializes the inode memory.
 * @param[out] _inode Pointer to an Inode structure to store the result.
 * @return void
 */
void inode_init(Inode *_inode);

/**
 * @brief Constructs a block of inodes and initializes their fields.
 * Creates a sequence of `_count` inodes starting at the specified disk offset.
 * Each inode is initialized with zeroed data fields and -1 in all pointer fields.
 * @param[in] _count Number of inodes to construct.
 * @param[in,out] _fileStream Pointer to the file representing the hard drive.
 * @param[in] _diskOffset Offset on disk where the inode block begins.
 * @return void
 */
void inode_make(unsigned int _count, FILE *_fileStream, unsigned int _diskOffset);

/**
 * @brief Reads an inode from disk and loads it into the provided buffer.
 * Calculates the disk location of the inode specified by `_inodeNum` and
 * reads its contents into the `_inode` buffer.
 * @param[out] _inode Pointer to an Inode structure to store the result.
 * @param[in] _inodeNum The index of the inode to read.
 * @param[in] _fileStream Pointer to the file representing the hard drive.
 * @return void
 */
void inode_read(Inode *_inode, unsigned int _inodeNum, FILE *_fileStream);

/**
 * @brief Writes an inode to its corresponding location on disk.
 * Calculates the disk location of the inode specified by `_inodeNum` and
 * writes the contents of the `_inode` buffer to that location.
 * @param[in] _inode Pointer to the Inode structure containing the data to write.
 * @param[in] _inodeNum The index of the inode to write.
 * @param[in,out] _fileStream Pointer to the file representing the hard drive.
 * @return void
 */
void inode_write(Inode *_inode, unsigned int _inodeNum, FILE *_fileStream);

#endif  // I_NODE_H
