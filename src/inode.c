/******************************************************************************
 *   File Sector Manager (FSM)
 *   Author: Michael Lombardi
 ******************************************************************************/
#include "inode.h"

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "fsm_constants.h"
#include "global_constants.h"

Inode inode = {.dataBlocks = 0,
               .dIndirect = 0,
               .directPtr = {0},
               .fileSize = 0,
               .fileType = 0,
               .linkCount = 0,
               .owner = 0,
               .permissions = 0,
               .sIndirect = 0,
               .status = 0,
               .tIndirect = 0};

InodeMap inode_map = {.id = 0, .iMap = {0}, .iMapHandle = NULL, .iMapOffset = {0, 0}};

void inode_init_ptrs(Inode *_inode) {
    typedef unsigned int UI;
    char *buffer = (char *)_inode;
    const int offset = 7;
    // initialize all 10 direct pointers, 1 single idirect, 1 double indirect, 1 triple indirect
    memcpy(buffer + offset * sizeof(unsigned int),
           (unsigned int[]){(UI)-1, (UI)-1, (UI)-1, (UI)-1, (UI)-1, (UI)-1, (UI)-1, (UI)-1, (UI)-1,
                            (UI)-1, (UI)-1, (UI)-1, (UI)-1},
           sizeof(unsigned int) * 13);
}

void inode_init(Inode *_inode) {
    char *buffer = (char *)_inode;
    const int offset = 7;
    // value loc for fileType, fileSize, permissions, linkCount, dataBlocks, tModified, status
    memcpy(buffer, (unsigned int[]){0, 0, 0, 0, 0, 0, 0}, sizeof(unsigned int) * offset);
    inode_init_ptrs(_inode);
}

void inode_make(unsigned int _count, FILE *_fileStream, unsigned int _diskOffset) {
    // before writing default iNodes, store values in a buffer
    unsigned int buffer[_count][32];
    typedef unsigned int UI;
    // initialize all struct values to their default values
    // 0 for data, -1 for pointers
    for (unsigned int i = 0; i < _count; i++) {
        Inode *node = (Inode *)&buffer[i][0];
        inode_init(node);
        memcpy(&buffer[i][20],
               (unsigned int[]){(UI)-3, (UI)-3, (UI)-3, (UI)-3, (UI)-3, (UI)-3, (UI)-3, (UI)-3,
                                (UI)-3, (UI)-3, (UI)-3, (UI)-3},
               sizeof(unsigned int) * 12);
    }  // end for (i = 0; i < _count; i++)
    // locate the requested offset within the disk file
    fseek(_fileStream, _diskOffset, SEEK_SET);
    // write the array of iNodes to the disk file
    fwrite(buffer, sizeof(unsigned int), _count * 32, _fileStream);
    // move to the beginning of the disk file
    fseek(_fileStream, 0, SEEK_SET);
}

void inode_read(Inode *_inode, unsigned int _inodeNum, FILE *_fileStream) {
    // array buffer to hold values from the iNode buffer passed into function
    Inode buffer;
    // will need to determine an offset from the first iNode to the iNode we want to read from
    // generate the offset based on the size of the block, size of the iNode
    // and the iNode number in which we will read from
    int offset = (2 * BLOCK_SIZE) + (_inodeNum * INODE_SIZE);
    // move to the offset location of the filestream
    fseek(_fileStream, offset, SEEK_SET);
    // read the offset location in the filestream
    size_t res = fread(&buffer, sizeof(Inode), 1, _fileStream);
    if (res == 0) {
        printf("Error reading inode %d from file stream.\n", _inodeNum);
        return;
    }
    // move to the start of the filestream
    fseek(_fileStream, 0, SEEK_SET);
    // store buffer for:
    // fileType, fileSize, permissions, linkCount, dataBlocks, owner,
    // status, directPtr[10], sIndirect, dIndirect, tIndirect
    memcpy(_inode, &buffer, sizeof(Inode));
}

void inode_write(Inode *_inode, unsigned int _inodeNum, FILE *_fileStream) {
    // will need to determine an offset from the first iNode to the iNode we want to write to
    // generate the offset based on the size of the block, size of the iNode
    // and the iNode number in which we will write to
    int offset = (2 * BLOCK_SIZE) + (_inodeNum * INODE_SIZE);
    // move to the offset location of the filestream
    fseek(_fileStream, offset, SEEK_SET);
    // write to the offset location in the filestream
    fwrite(_inode, sizeof(Inode), 1, _fileStream);
    // move to the start of the filestream
    fseek(_fileStream, 0, SEEK_SET);
}
