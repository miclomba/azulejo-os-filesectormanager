/******************************************************************************
 *   File Sector Manager (FSM)
 *   Author: Michael Lombardi
 ******************************************************************************/
#include "iNode.h"

#include <stdio.h>

#include "config.h"
#include "fsmDefinitions.h"
#include "gDefinitions.h"

/************************** FUNCTION DEFINITIONS *****************************/
/****************************** def beg makeInodes ****************************

   void makeInodes(unsigned int _count, FILE *_fileStream,
                                                           unsigned int _diskOffset)

   Function:
        makeInodes will construct a block of iNodes in a linear fashion,
        initializing all values to 0 for data and -1 for pointers

   Data Environment:
        Function Parameters
                unsigned int _count
                : the number of iNodes to be constructed

                FILE *_fileStream
                := a pointer to the hard drive file to be written to

                unsigned int _diskOffset
                : because of system design, the iNodes will not begin at
                the beginning of the hard drive

        Local Variables
                unsigned int i
                := iterator for moving through a buffer during
                initialization

                unsigned int j
                := iterator for moving through a buffer during
                initialization

                unsigned int buffer[_count][20]
                := an array buffer to hold all iNodes before writing
                to disk

        Struct Variables
                unsigned int fileType
                := struct location for fileType

                unsigned int fileSize
                := struct location for fileSize

                unsigned int permissions
                := struct location for permissions

                unsigned int linkCount
                := struct location for linkCount

                unsigned int dataBlocks
                := struct location for dataBlocks

                unsigned int tModified
                := struct location for tModified

                unsigned int status
                := struct location for status

                unsigned int directPtr[10]
                := struct location for direct pointers

                unsigned int sIndirect
                := struct location for single indirect disk block

                unsigned int dIndirect
                := struct location for double indirect disk block

                unsigned int tIndirect
                := struct location for triple indirect disk block

   Change Record
        4/1/10 makeInodes implemented.

****************************** def end makeInodes ***************************/
void makeInodes(unsigned int _count, FILE *_fileStream, unsigned int _diskOffset) {
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

/****************************** def beg readInode *****************************

   void readInode(Inode *_inode, unsigned int _iNodeNum, FILE *_fileStream)

   Function:
        readInode will calculate and locate the location of a given iNode,
        storing all values into a buffer

   Data Environment:
        Function Parameters
                Inode *_inode
                := a pointer to an iNode struct that serves as a buffer

                unsigned int _inodeNum
                : the number of the iNode to be read

                FILE *_fileStream
                = a pointer to the hard drive file to be read from

        Local Variables
                int j
                := iterator for moving through arrays

                int k
                := iterators for moving through arrays

                int sampleCount
                := store the temporary value of the return value from fread

                int offset
                := will need to determine an offset from the first iNode to
                the iNode we want to read from

                unsigned int buffer[20]
                := array buffer to hold values from the iNode
                buffer passed into the function

        Struct Variables
                unsigned int fileType
                := struct location for fileType

                unsigned int fileSize
                := struct location for fileSize

                unsigned int permissions
                := struct location for permissions

                unsigned int linkCount
                := struct location for linkCount

                unsigned int dataBlocks
                := struct location for dataBlocks

                unsigned int tModified
                := struct location for tModified

                unsigned int status
                := struct location for status

                unsigned int directPtr[10]
                := struct location for direct pointers

                unsigned int sIndirect
                := struct location for single indirect disk block

                unsigned int dIndirect
                := struct location for double indirect disk block

                unsigned int tIndirect
                := struct location for triple indirect disk block

   Change Record
        4/1/10 readInode implemented.

******************************* def end readInode ****************************/
void readInode(Inode *_inode, unsigned int _inodeNum, FILE *_fileStream) {
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
    fread(buffer, sizeof(unsigned int), 20, _fileStream);
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

/***************************** def beg writeInode *****************************

   void writeInode(Inode *_inode, unsigned int _iNodeNum, FILE *_fileStream)

   Function:
        writeInode calculates and locates the location of a given iNode,
        writing all values from the iNode buffer to the respective iNode
        location

   Data Environment:
        Function Parameters
                Inode *_inode
                := a pointer to an iNode struct that serves as a buffer

                unsigned int _inodeNum
                : the number of the iNode to be read

                FILE *_fileStream
                = a pointer to the hard drive file to write to

        Local Variables
                int j
                := iterator for moving through array

                int k
                := iterator for moving through array

                int sampleCount
                = store the temporary value returned from fwrite

                int offset
                := will need to determine an offset from the first iNode

                unsigned int buffer[20]
                := array buffer to hold values from the iNode
                buffer passed into the function

        Struct Variables
                unsigned int fileType
                := struct location for fileType

                unsigned int fileSize
                := struct location for fileSize

                unsigned int permissions
                := struct location for permissions

                unsigned int linkCount
                := struct location for linkCount

                unsigned int dataBlocks
                := struct location for dataBlocks

                unsigned int tModified
                := struct location for tModified

                unsigned int status
                := struct location for status

                unsigned int directPtr[10]
                := struct location for direct pointers

                unsigned int sIndirect
                := struct location for single indirect disk block

                unsigned int dIndirect
                := struct location for double indirect disk block

                unsigned int tIndirect
                := struct location for triple indirect disk block

        Change Record:

***************************** def end writeInode *****************************/
void writeInode(Inode *_inode, unsigned int _inodeNum, FILE *_fileStream) {
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
