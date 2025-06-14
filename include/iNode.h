/******************************************************************************
 *   File Sector Manager (FSM)
 *   Author: Michael Lombardi
 ******************************************************************************/
#ifndef I_NODE_H
#define I_NODE_H
#include <stdio.h>

#include "config.h"

typedef struct {
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

/*
 * @brief Will construct a block of iNodes in a linear fashion, initializing all values to 0 for
 data and -1 for pointers
 * @param _count the number of iNodes to be constructed
 * @param _fileStream a pointer to the hard drive file to be written to
 * @param _diskOffset a hard drive offset because due to system design, the iNodes will not begin at
 the beginning of the hard drive
 */
void makeInodes(unsigned int _count, FILE *_fileStream, unsigned int _diskOffset);

/*
 * @brief Will calculate and locate the location of a given iNode, storing all values into a buffer
 * @param _inode a pointer to an iNode struct that serves as a buffer
 * @param _inodeNum the number of the iNode to be read
 * @param _fileStream a pointer to the hard drive file to read from
 */
void readInode(Inode *_inode, unsigned int _inodeNum, FILE *_fileStream);

/*
 * @brief Calculates and locates the location of a given iNode, writing all values from the iNode
 * buffer to the respective iNode location
 * @param _inode a pointer to an iNode struct that serves as a buffer
 * @param _inodeNum the number of the iNode to be read
 * @param _fileStream a pointer to the hard drive file to write to
 */
void writeInode(Inode *_inode, unsigned int _inodeNum, FILE *_fileStream);

#endif  // I_NODE_H