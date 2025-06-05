/******************************************************************************
 *   File Sector Manager (FSM)
 *   Author: Michael Lombardi
 ******************************************************************************/
#ifndef I_NODE_H
#define I_NODE_H
#include <stdio.h>

#include "config.h"

typedef struct {
    unsigned int fileType;
    unsigned int fileSize;
    unsigned int permissions;
    unsigned int linkCount;
    unsigned int dataBlocks;
    unsigned int owner;
    unsigned int status;
    unsigned int directPtr[10];
    unsigned int sIndirect;
    unsigned int dIndirect;
    unsigned int tIndirect;
} Inode;

/************************** FUNCTION PROTOTYPES ******************************/
void makeInodes(unsigned int _count, FILE *_fileStream, unsigned int _diskOffset);
void readInode(Inode *_inode, unsigned int _inodeNum, FILE *_fileStream);
void writeInode(Inode *_inode, unsigned int _inodeNum, FILE *_fileStream);

#endif  // I_NODE_H