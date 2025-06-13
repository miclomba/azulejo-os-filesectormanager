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
void initFileSectorMgr(FileSectorMgr *_fsm, int _initSsmMaps);
void initFsmMaps(FileSectorMgr *_fsm);
void mkfs(FileSectorMgr *_fsm, unsigned int _DISK_SIZE, unsigned int _BLOCK_SIZE,
          unsigned int _INODE_SIZE, unsigned int _INODE_BLOCKS, unsigned int _INODE_COUNT,
          int _initSsmMaps);
Bool allocateInode(FileSectorMgr *_fsm);
Bool deallocateInode(FileSectorMgr *_fsm);
Bool getInode(int _n, FileSectorMgr *_fsm);
unsigned int createFile(FileSectorMgr *_fsm, int _isDirectory, unsigned int *_name,
                        unsigned int _inodeNumD);
Bool openFile(FileSectorMgr *_fsm, unsigned int _inodeNum);
void closeFile(FileSectorMgr *_fsm);
Bool addFileToDir(FileSectorMgr *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                  unsigned int _inodeNumD);
Bool addFileTo_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                          unsigned int _sIndirectOffset, Bool _allocate);
Bool addFileTo_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                          unsigned int _dIndirectOffset, Bool _allocate);
Bool addFileTo_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                          unsigned int _tIndirectOffset, Bool _allocate);
Bool rmFileFromDir(FileSectorMgr *_fsm, unsigned int _inodeNumF, unsigned int _inodeNumD);
Bool rmFileFrom_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
                           unsigned int _dIndirectOffset, unsigned int _sIndirectOffset);
Bool rmFileFrom_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
                           unsigned int _tIndirectOffset, unsigned int _dIndirectOffset);
Bool rmFileFrom_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
                           unsigned int _tIndirectOffset);
unsigned int aloc_S_Indirect(FileSectorMgr *_fsm, long long int _blockCount);
unsigned int aloc_D_Indirect(FileSectorMgr *_fsm, long long int _blockCount);
unsigned int aloc_T_Indirect(FileSectorMgr *_fsm, long long int _blockCount);
Bool writeToFile(FileSectorMgr *_fsm, unsigned int _inodeNum, void *_buffer,
                 long long int _fileSize);
void writeTo_S_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset, void *_buffer,
                              unsigned int _sIndirectPtrs);
void writeTo_D_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset, void *_buffer,
                              unsigned int _dIndirectPtrs);
void writeTo_T_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset, void *_buffer,
                              unsigned int _tIndirectPtrs);
Bool readFromFile(FileSectorMgr *_fsm, unsigned int _inodeNum, void *_buffer);
void readFrom_S_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer, unsigned int _diskOffset);
void readFrom_D_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer, unsigned int _diskOffset);
void readFrom_T_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer, unsigned int _diskOffset);
Bool rmFile(FileSectorMgr *_fsm, unsigned int _inodeNum, unsigned int _inodeNumD);
void rmFile_S_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType, unsigned int _inodeNumD,
                             unsigned int _diskOffset);
void rmFile_D_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType, unsigned int _inodeNumD,
                             unsigned int _diskOffset);
void rmFile_T_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType, unsigned int _inodeNumD,
                             unsigned int _diskOffset);
Bool renameFile(FileSectorMgr *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                unsigned int _inodeNumD);
Bool renameFileIn_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                             unsigned int _sIndirectOffset);
Bool renameFileIn_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                             unsigned int _dIndirectOffset);
Bool renameFileIn_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                             unsigned int _tIndirectOffset);

#endif  // FILE_SECTOR_MGR