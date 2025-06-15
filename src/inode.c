/******************************************************************************
 *   File Sector Manager (FSM)
 *   Author: Michael Lombardi
 ******************************************************************************/
#include "inode.h"

#include <stdio.h>

#include "config.h"
#include "fsm_constants.h"
#include "global_constants.h"

void inode_make(unsigned int _count, FILE *_fileStream, unsigned int _diskOffset) {
    // iterators for moving through a buffer during initialization
    unsigned int i, j;
    // store temporarily the return value from fwrite
    // unsigned int sampleCount;
    // before writing default iNodes, store values in a buffer
    unsigned int buffer[_count][32];
    // initialize all struct values to their default values
    // 0 for data, -1 for pointers
    for (i = 0; i < _count; i++) {
        // value location for fileType
        buffer[i][0] = 0;
        // value location for fileSize
        buffer[i][1] = 0;
        // value location for permissions
        buffer[i][2] = 0;
        // value location for linkCount
        buffer[i][3] = 0;
        // value location for dataBlocks
        buffer[i][4] = 0;
        // value location for tModified
        buffer[i][5] = 0;
        // value location for status
        buffer[i][6] = 0;
        // initialize all direct pointers to values of -1
        for (j = 7; j < 17; j++) {
            // direct pointer to -1
            buffer[i][j] = -1;
        }  // end for (k = 7; j < 17; j++)
        // single indirect pointer to -1
        buffer[i][17] = -1;
        // double indirect pointer to -1
        buffer[i][18] = -1;
        // triple indirect pointer to -1
        buffer[i][19] = -1;
        // add padding to the buffer
        for (j = 20; j < 32; j++) {
            // set to -3 to differentiate
            buffer[i][j] = -3;
        }  // end for (j = 20; j < 32; j++)
    }  // end for (i = 0; i < _count; i++)
    // locate the requested offset within the disk file
    fseek(_fileStream, _diskOffset, SEEK_SET);
    // write the array of iNodes to the disk file
    fwrite(buffer, sizeof(unsigned int), _count * 32, _fileStream);
    // move to the beginning of the disk file
    fseek(_fileStream, 0, SEEK_SET);
}

void inode_read(Inode *_inode, unsigned int _inodeNum, FILE *_fileStream) {
    // iterators for moving through arrays
    int j, k;
    // store the temporary value of the return value from fread
    // int sampleCount;
    // will need to determine an offset from the first iNode to
    // the iNode we want to read from
    int offset;
    // array buffer to hold values from the iNode buffer passed into function
    unsigned int buffer[20];
    // generate the offset based on the size of the block, size of the iNode
    // and the iNode number in which we will read from
    offset = (2 * BLOCK_SIZE) + (_inodeNum * INODE_SIZE);
    // move to the offset location of the filestream
    fseek(_fileStream, offset, SEEK_SET);
    // read the offset location in the filestream
    size_t res = fread(buffer, sizeof(unsigned int), 20, _fileStream);
    if (res != 0) return;
    // move to the start of the filestream
    fseek(_fileStream, 0, SEEK_SET);
    // store buffer for fileType
    _inode->fileType = buffer[0];
    // store buffer for fileSize
    _inode->fileSize = buffer[1];
    // store buffer for permissions
    _inode->permissions = buffer[2];
    // store buffer for linkCount
    _inode->linkCount = buffer[3];
    // store buffer for dataBlocks
    _inode->dataBlocks = buffer[4];
    // store buffer for tModified
    _inode->owner = buffer[5];
    // store buffer for status
    _inode->status = buffer[6];
    // set the starting point to retrieve directPtr values from buffer
    k = 0;
    // retrieve directPtrs from the read buffer
    for (j = 7; j < 17; j++) {
        // iNode directPtr value stores value from buffer
        _inode->directPtr[k] = buffer[j];
        // move to next directPtr
        k++;
    }  // end for (j = 7; j < 17; j++)
    // get the indirect pointers
    _inode->sIndirect = buffer[17];
    _inode->dIndirect = buffer[18];
    _inode->tIndirect = buffer[19];
}

void inode_write(Inode *_inode, unsigned int _inodeNum, FILE *_fileStream) {
    // iterators for moving through arrays
    int j, k;
    // store the temporary value returned from fwrite
    // int sampleCount;
    // will need to determine an offset from the first iNode to
    // the iNode we want to write to
    int offset;
    // array buffer to hold values from the iNode buffer passed into function
    unsigned int buffer[20];
    // generate the offset based on the size of the block, size of the iNode
    // and the iNode number in which we will write to
    offset = (2 * BLOCK_SIZE) + (_inodeNum * INODE_SIZE);
    // store buffer for fileType
    buffer[0] = _inode->fileType;
    // store buffer for fileSize
    buffer[1] = _inode->fileSize;
    // store buffer for permissions
    buffer[2] = _inode->permissions;
    // store buffer for linkCount
    buffer[3] = _inode->linkCount;
    // store buffer for dataBlocks
    buffer[4] = _inode->dataBlocks;
    // store buffer for tModified
    buffer[5] = _inode->owner;
    // store buffer for status
    buffer[6] = _inode->status;
    // set the starting point to retrieve directPtr values from buffer
    k = 0;
    // retrieve directPtr values from iNod buffer
    for (j = 7; j < 17; j++) {
        // buffer value for directPtr will have value from iNode buffer
        buffer[j] = _inode->directPtr[k];
        // move to next directPtr in iNode buffer
        k++;
    }  // end for (j = 7; j < 17; j++)
    // get the indirect pointers
    buffer[17] = _inode->sIndirect;
    buffer[18] = _inode->dIndirect;
    buffer[19] = _inode->tIndirect;
    // move to the offset location of the filestream
    fseek(_fileStream, offset, SEEK_SET);
    // write to the offset location in the filestream
    fwrite(buffer, sizeof(unsigned int), 20, _fileStream);
    // move to the start of the filestream
    fseek(_fileStream, 0, SEEK_SET);
}
