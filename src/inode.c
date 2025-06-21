/******************************************************************************
 *   File Sector Manager (FSM)
 *   Author: Michael Lombardi
 ******************************************************************************/
#include "inode.h"

#include <math.h>
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

//========================= FSM FUNCTION PROTOTYPES =======================//
static Bool is_not_null(unsigned int _ptr);

//========================= FSM FUNCTION DEFINITIONS =======================//
/**
 * @brief Verify if a pointer is not null.
 * @param[in] _ptr a pointer.
 * @return True if the pointer is not null; false otherwise.
 * @date 2025-06-17 First implementation.
 */
static inline Bool is_not_null(unsigned int _ptr) { return _ptr != (unsigned int)(-1); }

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

Bool allocate_inode(void) {
    if (is_not_null(inode_map.iMapOffset[0])) {
        int bit = inode_map.iMapOffset[1];
        int byte = inode_map.iMapOffset[0];
        int n = 1;
        // Find contiguous sectors
        while (n > 0) {
            for (; bit < BITS_PER_BYTE; bit++) {
                inode_map.iMap[byte] -= pow(2, bit);
                n--;
                if (n == 0) {
                    break;
                }  // end if (n == 0)
            }  // end for (; bit < 8; bit++)
            byte++;
            bit = 0;
        }  // end while (n > 0)
    }
    inode_map.iMapOffset[0] = (unsigned int)(-1);
    inode_map.iMapOffset[1] = (unsigned int)(-1);
    // Write the newly allocated inode to the iMapHandler
    inode_map.iMapHandle = fopen(FSM_INODE_MAP, "r+");
    fwrite(inode_map.iMap, 1, INODE_BLOCKS, inode_map.iMapHandle);
    fclose(inode_map.iMapHandle);
    inode_map.iMapHandle = Null;
    return True;
}

Bool deallocate_inode(unsigned int _inodeNum) {
    inode_map.iMapOffset[0] = _inodeNum / BITS_PER_BYTE;
    inode_map.iMapOffset[1] = _inodeNum % BITS_PER_BYTE;
    // Deallocate iMap at Inode's location
    if (is_not_null(inode_map.iMapOffset[0])) {
        int bit = inode_map.iMapOffset[1];
        int byte = inode_map.iMapOffset[0];
        int n = 1;
        while (n > 0) {
            for (; bit < BITS_PER_BYTE; bit++) {
                inode_map.iMap[byte] += pow(2, bit);
                n--;
                if (n == 0) {
                    break;
                }  // end if (n == 0)
            }  // end for (; bit < 8; bit++)
            byte++;
            bit = 0;
        }  // end while (n > 0)
    }
    inode_map.iMapOffset[0] = (unsigned int)(-1);
    inode_map.iMapOffset[1] = (unsigned int)(-1);
    // Write iMap to its Handler
    inode_map.iMapHandle = fopen(FSM_INODE_MAP, "r+");
    fwrite(inode_map.iMap, 1, INODE_BLOCKS, inode_map.iMapHandle);
    fclose(inode_map.iMapHandle);
    inode_map.iMapHandle = Null;
    return True;
}

Bool get_inode(int _n) {
    int i;
    unsigned int mask, result, buff, byteCount;
    unsigned char *iMap;
    unsigned long long int subsetMap[1];
    Bool found;
    // We assume 0 < _n < 33
    if (_n < 33 && _n > 0) {
        mask = 0;
        for (i = _n - 1; i > -1; i--) {
            mask += (unsigned int)pow(2, i);
        }  // end for (i = _n - 1; i > -1; i--)
        // initially assume false
        found = False;
        result = mask;
        buff = mask;
        subsetMap[0] = 0;
        byteCount = 0;
        iMap = inode_map.iMap;
        inode_map.iMapOffset[0] = -1;
        inode_map.iMapOffset[1] = -1;
        while (byteCount < INODE_BLOCKS) {
            subsetMap[0] = *((unsigned long long int *)iMap);
            for (i = 0; i < 32; i++) {
                buff = (unsigned int)subsetMap[0];
                result = mask & buff;
                if (result == mask) {
                    found = True;
                    break;
                }  // end if (result == mask)
                else {
                    subsetMap[0] >>= 1;
                }  // end else
            }  // end for (i = 0; i < 32; i++)
            // Assign found inode to index
            if (found == True) {
                inode_map.iMapOffset[0] = byteCount + (i / BITS_PER_BYTE);
                inode_map.iMapOffset[1] = i % BITS_PER_BYTE;
                return True;  // break;
            }  // end if (found == True)
            iMap += 4;
            byteCount += 4;
        }  // end while (byteCount < INODE_BLOCKS)
    }  // end if (_n < 33 && _n > 0)
    return False;
}
