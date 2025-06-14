/*******************************************************************************
 * File Sector Manager (FSM)
 * Author: Michael Lombardi
 *******************************************************************************/
#ifndef FILE_SECTOR_MGR_H
#define FILE_SECTOR_MGR_H
#include <stdio.h>

#include "config.h"
#include "fsmDefinitions.h"
#include "gDefinitions.h"
#include "iNode.h"
#include "sectorSpaceMgr.h"

//======================== FSM TYPE DEFINITION ==============================//
typedef struct {
    // pointer to SSM
    SecSpaceMgr ssm[1];
    // pointers used for disk access
    FILE *iMapHandle;
    FILE *diskHandle;
    unsigned int diskOffset;
    unsigned int sampleCount;
    unsigned char iMap[MAX_INODE_BLOCKS];
    Inode inode;
    unsigned int inodeNum;
    unsigned int contInodes;
    unsigned int index[2];
    unsigned int badInode[MAX_INPUT][2];
} FileSectorMgr;

//======================== FSM FUNCTION PROTOTYPES ==========================//
void mkfs(FileSectorMgr *_fsm, unsigned int _DISK_SIZE, unsigned int _BLOCK_SIZE,
          unsigned int _INODE_SIZE, unsigned int _INODE_BLOCKS, unsigned int _INODE_COUNT,
          int _initSsmMaps);
void rmfs(FileSectorMgr *_fsm);
unsigned int createFile(FileSectorMgr *_fsm, int _isDirectory, unsigned int *_name,
                        unsigned int _inodeNumD);
Bool openFile(FileSectorMgr *_fsm, unsigned int _inodeNum);
void closeFile(FileSectorMgr *_fsm);
Bool rmFileFromDir(FileSectorMgr *_fsm, unsigned int _inodeNumF, unsigned int _inodeNumD);
Bool writeToFile(FileSectorMgr *_fsm, unsigned int _inodeNum, void *_buffer,
                 long long int _fileSize);
Bool readFromFile(FileSectorMgr *_fsm, unsigned int _inodeNum, void *_buffer);
Bool rmFile(FileSectorMgr *_fsm, unsigned int _inodeNum, unsigned int _inodeNumD);
Bool renameFile(FileSectorMgr *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                unsigned int _inodeNumD);

#endif  // FILE_SECTOR_MGR