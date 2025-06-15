/*******************************************************************************
 * File Sector Manager (FSM)
 * Author: Michael Lombardi
 *******************************************************************************/
#include "fsm.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "fsm_constants.h"
#include "global_constants.h"
#include "inode.h"
#include "ssm.h"

//========================= FSM FUNCTION PROTOTYPES =======================//
static void init_file_sector_mgr(FSM *_fsm, int _initSsmMaps);
static void init_fsm_maps(FSM *_fsm);
static Bool allocate_inode(FSM *_fsm);
static Bool deallocate_inode(FSM *_fsm);
static Bool get_inode(int _n, FSM *_fsm);
static Bool add_file_to_dir(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                            unsigned int _inodeNumD);
static Bool add_file_to_single_indirect(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                                        unsigned int _sIndirectOffset, Bool _allocate);
static Bool add_file_to_double_indirect(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                                        unsigned int _dIndirectOffset, Bool _allocate);
static Bool add_file_to_triple_indirect(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                                        unsigned int _tIndirectOffset, Bool _allocate);
static Bool remove_file_from_single_indirect(FSM *_fsm, unsigned int _inodeNumF,
                                             unsigned int _dIndirectOffset,
                                             unsigned int _sIndirectOffset);
static Bool remove_file_from_double_indirect(FSM *_fsm, unsigned int _inodeNumF,
                                             unsigned int _tIndirectOffset,
                                             unsigned int _dIndirectOffset);
static Bool remove_file_from_triple_indirect(FSM *_fsm, unsigned int _inodeNumF,
                                             unsigned int _tIndirectOffset);
static unsigned int aloc_single_indirect(FSM *_fsm, long long int _blockCount);
static unsigned int aloc_double_indirect(FSM *_fsm, long long int _blockCount);
static unsigned int aloc_triple_indirect(FSM *_fsm, long long int _blockCount);
static void write_to_single_indirect_blocks(FSM *_fsm, unsigned int _baseOffset, void *_buffer,
                                            unsigned int _sIndirectPtrs);
static void write_to_double_indirect_blocks(FSM *_fsm, unsigned int _baseOffset, void *_buffer,
                                            unsigned int _dIndirectPtrs);
static void write_to_triple_indirect_blocks(FSM *_fsm, unsigned int _baseOffset, void *_buffer,
                                            unsigned int _tIndirectPtrs);
static void read_from_single_indirect_blocks(FSM *_fsm, void *_buffer, unsigned int _diskOffset);
static void read_from_double_indirect_blocks(FSM *_fsm, void *_buffer, unsigned int _diskOffset);
static void read_from_triple_indirect_blocks(FSM *_fsm, void *_buffer, unsigned int _diskOffset);
static void remove_file_single_indirect_blocks(FSM *_fsm, unsigned int _fileType,
                                               unsigned int _inodeNumD, unsigned int _diskOffset);
static void remove_file_double_indirect_blocks(FSM *_fsm, unsigned int _fileType,
                                               unsigned int _inodeNumD, unsigned int _diskOffset);
static void remove_file_triple_indirect_blocks(FSM *_fsm, unsigned int _fileType,
                                               unsigned int _inodeNumD, unsigned int _diskOffset);
static Bool rename_file_in_single_indirect(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                                           unsigned int _sIndirectOffset);
static Bool rename_file_in_double_indirect(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                                           unsigned int _dIndirectOffset);
static Bool rename_file_in_triple_indirect(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                                           unsigned int _tIndirectOffset);

//========================= FSM FUNCTION DEFINITIONS =======================//
/**
 * @brief Initializes the File Sector Manager.
 * Sets up the File Sector Manager and optionally initializes the SSM maps.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @param[in] _initSsmMaps Flag indicating whether to initialize the SSM maps.
 * @return void
 * @date 2010-04-01 First implementation.
 */
static void init_file_sector_mgr(FSM *_fsm, int _initSsmMaps) {
    int i, j;
    // Initialize SSM Maps
    if (_initSsmMaps == 1) {
        ssm_init_maps(_fsm->ssm);
    }  // end if (_initSsmMaps == 1)
    // Initialize SEctor Space Manager
    ssm_init(_fsm->ssm);
    char dbfile[256];
    // Initialize FSM's variables
    _fsm->contInodes = 0;
    _fsm->index[0] = -1;
    _fsm->index[1] = -1;
    _fsm->sampleCount = 0;
    // Initialize FSM's inode pointer to a blank inode
    _fsm->inode.fileType = 0;
    _fsm->inode.fileSize = 0;
    _fsm->inode.permissions = 0;
    _fsm->inode.linkCount = 0;
    _fsm->inode.dataBlocks = 0;
    _fsm->inode.owner = 0;
    _fsm->inode.status = 0;
    j = 0;
    // Initialize all inode pointers to -1
    for (i = 7; i < 17; i++) {
        _fsm->inode.directPtr[j] = -1;
        j++;
    }  // end for (i = 7; i < 17; i++)
    _fsm->inode.sIndirect = -1;
    _fsm->inode.dIndirect = -1;
    _fsm->inode.tIndirect = -1;
    _fsm->inodeNum = (unsigned int)-1;
    // Open file stream for iMap
    snprintf(dbfile, sizeof(dbfile), "./fs/iMap");
    _fsm->iMapHandle = fopen(dbfile, "r+");
    // Read in INODE_BLOCKS number of items from iMap to iMapHandle
    _fsm->sampleCount = fread(_fsm->iMap, 1, INODE_BLOCKS, _fsm->iMapHandle);
    // Closes the iMapHandle
    fclose(_fsm->iMapHandle);
    _fsm->iMapHandle = Null;
    // Open file stream for the disk
    snprintf(dbfile, sizeof(dbfile), "./fs/hardDisk");
    // Open binary form of file for reading and writing
    _fsm->diskHandle = fopen(dbfile, "rb+");
}

/**
 * @brief Initializes the File Sector Manager maps.
 * Sets up internal maps within the File Sector Manager.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @return void
 * @date 2010-04-01 First implementation.
 */
static void init_fsm_maps(FSM *_fsm) {
    unsigned int i;
    char dbfile[256];
    unsigned char map[INODE_BLOCKS];  // SECTOR_BYTES
    unsigned char disk[DISK_SIZE];
    // Load iMap from file and place in iMapHandle
    snprintf(dbfile, sizeof(dbfile), "./fs/iMap");
    _fsm->iMapHandle = fopen(dbfile, "r+");
    // Initialize all map elements to 255
    for (i = 0; i < INODE_BLOCKS; i++) {
        map[i] = 255;
    }  // end for (i = 0; i < INODE_BLOCKS; i++)
    // Write map back to iMapHandle
    _fsm->sampleCount = fwrite(map, 1, INODE_BLOCKS, _fsm->iMapHandle);
    // close the file
    fclose(_fsm->iMapHandle);
    _fsm->iMapHandle = Null;
    snprintf(dbfile, sizeof(dbfile), "./fs/hardDisk");
    _fsm->diskHandle = fopen(dbfile, "rb+");
    // Initialize all disk elements to 0
    for (i = 0; i < DISK_SIZE; i++) {
        disk[i] = 0;
    }  // end for (i = 0; i < DISK_SIZE; i++)
    // Write contents of disk to diskHandle
    _fsm->sampleCount = fwrite(disk, 1, DISK_SIZE, _fsm->diskHandle);
    fclose(_fsm->diskHandle);
    _fsm->diskHandle = Null;
}

unsigned int fs_create_file(FSM *_fsm, int _isDirectory, unsigned int *_name,
                            unsigned int _inodeNumD) {
    get_inode(1, _fsm);
    if (_fsm->index[0] == (unsigned int)(-1)) {
        return (unsigned int)(-1);
    }  // end if (_fsm->index[0] == (unsigned int)(-1))
    // Create a default file. Will start with all pointers as -1
    unsigned int inodeNum;
    inodeNum = 8 * _fsm->index[0] + _fsm->index[1];
    inode_read(&_fsm->inode, inodeNum, _fsm->diskHandle);
    // initialize inode metadata
    _fsm->inode.fileSize = 0;
    _fsm->inode.permissions = 0;
    _fsm->inode.linkCount = 0;
    _fsm->inode.dataBlocks = 0;
    _fsm->inode.owner = 0;
    _fsm->inode.status = 0;
    int i;
    for (i = 0; i < 10; i++) {
        _fsm->inode.directPtr[i] = (unsigned int)(-1);
    }  // end for (i = 0; i < 10; i++)
    _fsm->inode.sIndirect = (unsigned int)(-1);
    _fsm->inode.dIndirect = (unsigned int)(-1);
    _fsm->inode.tIndirect = (unsigned int)(-1);
    // Assign filetype
    if (_isDirectory == 1) {
        _fsm->inode.fileType = 2;
    } else {
        _fsm->inode.fileType = 1;
    }  // end else (_isDirectory == 1)
    inode_write(&_fsm->inode, inodeNum, _fsm->diskHandle);
    allocate_inode(_fsm);
    unsigned int name[2];
    if (_isDirectory == 1) {
        // set . directory
        strcpy((char *)name, ".");
        add_file_to_dir(_fsm, inodeNum, name, inodeNum);
        // set .. directory
        strcpy((char *)name, "..");
        add_file_to_dir(_fsm, _inodeNumD, name, inodeNum);
    }  // end if (_isDirectory == 1)
    add_file_to_dir(_fsm, inodeNum, _name, _inodeNumD);
    return inodeNum;
}

Bool fs_open_file(FSM *_fsm, unsigned int _inodeNum) {
    int i, j;
    if (_inodeNum == (unsigned int)(-1)) {
        return False;
    }  // end if (_inodeNum == (unsigned int)(-1)) {
    inode_read(&_fsm->inode, _inodeNum, _fsm->diskHandle);
    _fsm->inodeNum = _inodeNum;
    // Return true if file loaded correctly
    if (_fsm->inode.fileType > 0) {
        return True;
    }  // end if (_fsm->inode.fileType > 0)
    else {
        // If file not loaded, create a default inode and return False
        _fsm->inode.fileType = 0;
        _fsm->inode.fileSize = 0;
        _fsm->inode.permissions = 0;
        _fsm->inode.linkCount = 0;
        _fsm->inode.dataBlocks = 0;
        _fsm->inode.owner = 0;
        _fsm->inode.status = 0;
        j = 0;
        for (i = 7; i < 17; i++) {
            _fsm->inode.directPtr[j] = -1;
            j++;
        }  // end for (i = 7; i < 17; i++)
        _fsm->inode.sIndirect = -1;
        _fsm->inode.dIndirect = -1;
        _fsm->inode.tIndirect = -1;
        _fsm->inodeNum = (unsigned int)(-1);
        // Return False if file did not open correctly
        return False;
    }  // end else (_fsm->inode.fileType > 0)
}

void fs_close_file(FSM *_fsm) {
    int i, j;
    // Reset all FSM->Inode variables to defaults
    _fsm->inodeNum = (unsigned int)(-1);
    _fsm->inode.fileType = 0;
    _fsm->inode.fileSize = 0;
    _fsm->inode.permissions = 0;
    _fsm->inode.linkCount = 0;
    _fsm->inode.dataBlocks = 0;
    _fsm->inode.owner = 0;
    _fsm->inode.status = 0;
    j = 0;
    for (i = 7; i < 17; i++) {
        _fsm->inode.directPtr[j] = (unsigned int)(-1);
        j++;
    }  // end for (i = 7; i < 17; i++)
    _fsm->inode.sIndirect = (unsigned int)(-1);
    _fsm->inode.dIndirect = (unsigned int)(-1);
    _fsm->inode.tIndirect = (unsigned int)(-1);
}

Bool fs_read_from_file(FSM *_fsm, unsigned int _inodeNum, void *_buffer) {
    // Open file at Inode _inodeNum for reading
    Bool success;
    success = fs_open_file(_fsm, _inodeNum);
    if (success == False) {
        return False;
    }  // end if (success == False)
    void *buffer;
    buffer = _buffer;
    // Read data from direct pointers into buffer _buffer
    int i;
    unsigned int diskOffset;
    for (i = 0; i < 10; i++) {
        diskOffset = _fsm->inode.directPtr[i];
        if (diskOffset != (unsigned int)(-1)) {
            fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
            _fsm->sampleCount = fread(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);
            buffer += BLOCK_SIZE;
        }  // end if (diskOffset != (unsigned int)(-1))
    }  // end for (i = 0; i < 10; i++)
    // Read data from single indirect pointer into buffer _buffer
    diskOffset = _fsm->inode.sIndirect;
    if (diskOffset != (unsigned int)(-1)) {
        read_from_single_indirect_blocks(_fsm, buffer, diskOffset);
        buffer += BLOCK_SIZE * S_INDIRECT_BLOCKS;
    }  // end if (diskOffset != (unsigned int)(-1))
    diskOffset = _fsm->inode.dIndirect;
    if (diskOffset != (unsigned int)(-1)) {
        read_from_double_indirect_blocks(_fsm, buffer, diskOffset);
        buffer += BLOCK_SIZE * D_INDIRECT_BLOCKS;
    }  // end if (diskOffset != (unsigned int)(-1))
    diskOffset = _fsm->inode.tIndirect;
    if (diskOffset != (unsigned int)(-1)) {
        read_from_triple_indirect_blocks(_fsm, buffer, diskOffset);
    }  // end if (diskOffset != (unsigned int)(-1))
    fseek(_fsm->diskHandle, 0, SEEK_SET);
    return True;
}

Bool fs_write_to_file(FSM *_fsm, unsigned int _inodeNum, void *_buffer, long long int _fileSize) {
    Bool success;
    // return false if openFile fails
    success = fs_open_file(_fsm, _inodeNum);
    if (success == False) {
        return False;
    }  // end if (success == False)
    // set fileSize and inode fileSize
    long long int fileSize = _fileSize;
    _fsm->inode.fileSize = (unsigned int)_fileSize;
    _fsm->inode.dataBlocks = _fsm->inode.fileSize / BLOCK_SIZE;
    unsigned int directPtrs = 0;
    unsigned int i;
    // Calculate how many direct pointers needed
    for (i = 0; i < 10; i++) {
        fileSize -= BLOCK_SIZE;
        if (fileSize > 0) {
            directPtrs++;
        }  // end if (fileSize > 0)
        else {
            directPtrs++;
            break;
        }  // end else (fileSize > 0)
    }  // end for (i = 0; i < 10; i++)
    void *buffer;
    buffer = _buffer;
    // Write to direct pointers
    unsigned int diskOffset;
    for (i = 0; i < directPtrs; i++) {
        diskOffset = _fsm->inode.directPtr[i];
        if (diskOffset == (unsigned int)(-1)) {
            // If direct pointer is empty, get a Sector for it
            ssm_get_sector(1, _fsm->ssm);
            // If pointer invalid, break out of if
            if (_fsm->ssm->index[0] == (unsigned int)(-1)) {
                break;
            }  // end if (_fsm->ssm->index[0] == (unsigned int)(-1))
            else {
                diskOffset = BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));
                _fsm->inode.directPtr[i] = diskOffset;
                fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                _fsm->sampleCount = fwrite(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);
                ssm_allocate_sectors(_fsm->ssm);
                buffer += BLOCK_SIZE;
            }  // end else (_fsm->ssm->index[0] == (unsigned int)(-1))
        }  // end if (diskOffset == (unsigned int)(-1))
        else {
            // Search for diskHandle from beginning of disk
            fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
            // Overwrite the data at that location
            _fsm->sampleCount = fwrite(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);
            buffer += BLOCK_SIZE;
        }  // end else (diskOffset == (unsigned int)(-1))
    }  // end for (i = 0; i < directPtrs; i++)
    unsigned int sIndirectPtrs = 0;
    unsigned int dIndirectPtrs = 0;
    unsigned int tIndirectPtrs = 0;
    unsigned int baseOffset;
    // If file is too big for the direct pointers,
    // begin looking at the indrect pointers
    if (fileSize > 0) {
        // Write to file using triple Indirection if file too big for
        /// double indirection
        if (fileSize + (10 * BLOCK_SIZE) - D_INDIRECT_SIZE > 0) {
            sIndirectPtrs = S_INDIRECT_BLOCKS;
            dIndirectPtrs = D_INDIRECT_BLOCKS;
            // Allocate Single and double indirect pointers
            _fsm->inode.sIndirect = aloc_single_indirect(_fsm, sIndirectPtrs);
            _fsm->inode.dIndirect = aloc_double_indirect(_fsm, dIndirectPtrs);
            // Write to single indirect blocks
            baseOffset = _fsm->inode.sIndirect;
            write_to_single_indirect_blocks(_fsm, baseOffset, buffer, sIndirectPtrs);
            buffer += BLOCK_SIZE * S_INDIRECT_BLOCKS;
            // Write to double indirect blocks
            baseOffset = _fsm->inode.dIndirect;
            write_to_double_indirect_blocks(_fsm, baseOffset, buffer, dIndirectPtrs);
            buffer += BLOCK_SIZE * D_INDIRECT_BLOCKS;
            // Calculate remaining filesize
            fileSize = fileSize + (10 * BLOCK_SIZE) - D_INDIRECT_SIZE;
            // Calculate number of triple Indirect pointers needed
            tIndirectPtrs = fileSize / BLOCK_SIZE;
            if (fileSize % BLOCK_SIZE > 0) {
                tIndirectPtrs += 1;
            }  // end if (fileSize % BLOCK_SIZE > 0)
            // Allocate triple Indirect Pointers
            _fsm->inode.tIndirect = aloc_triple_indirect(_fsm, tIndirectPtrs);
            baseOffset = _fsm->inode.tIndirect;
            // Write to tripleIndirectBlocks
            write_to_triple_indirect_blocks(_fsm, baseOffset, buffer, tIndirectPtrs);
        }  // end if (fileSize + (10 * BLOCK_SIZE) - D_INDIRECT_SIZE > 0)
        // Write to file using double Indirection if too big for
        //   single indirection
        else if (fileSize + (10 * BLOCK_SIZE) - S_INDIRECT_SIZE > 0) {
            // Allocate single Indirect blocks
            sIndirectPtrs = S_INDIRECT_BLOCKS;
            _fsm->inode.sIndirect = aloc_single_indirect(_fsm, sIndirectPtrs);
            // Write to single Indirect pointers
            baseOffset = _fsm->inode.sIndirect;
            write_to_single_indirect_blocks(_fsm, baseOffset, buffer, sIndirectPtrs);
            buffer += BLOCK_SIZE * S_INDIRECT_BLOCKS;
            // Calculate Remaining filesize
            fileSize = fileSize + (10 * BLOCK_SIZE) - S_INDIRECT_SIZE;
            // Calculate number of double Indirect Pointrs needed
            dIndirectPtrs = fileSize / BLOCK_SIZE;
            if (fileSize % BLOCK_SIZE > 0) {
                dIndirectPtrs += 1;
            }  // end if (fileSize % BLOCK_SIZE > 0)
            // Allocate double Indirect Pointers
            _fsm->inode.dIndirect = aloc_double_indirect(_fsm, dIndirectPtrs);
            baseOffset = _fsm->inode.dIndirect;
            // Write to double Indirect Pointers
            write_to_double_indirect_blocks(_fsm, baseOffset, buffer, dIndirectPtrs);
        }  // end else if (fileSize + (10 * BLOCK_SIZE) - S_INDIRECT_SIZE > 0)
        // Write to file using single Indirection only
        else {
            // Calculate number of single indirect pointers needed
            sIndirectPtrs = fileSize / BLOCK_SIZE;
            if (fileSize % BLOCK_SIZE > 0) {
                sIndirectPtrs += 1;
            }  // end if (fileSize % BLOCK_SIZE > 0)
            // Allocate single indirect pointers
            _fsm->inode.sIndirect = aloc_single_indirect(_fsm, sIndirectPtrs);
            // Write to single indirect pointers
            baseOffset = _fsm->inode.sIndirect;
            write_to_single_indirect_blocks(_fsm, baseOffset, buffer, sIndirectPtrs);
        }  // end else
    }  // end if (fileSize > 0)
    // Write created inode to disk
    inode_write(&_fsm->inode, _inodeNum, _fsm->diskHandle);
    fseek(_fsm->diskHandle, 0, SEEK_SET);
    return True;
}

/**
 * @brief Adds a file to a directory.
 * Inserts a file into the specified directory within the File Sector Manager.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @param[in] _inodeNumF Inode number of the file to be added.
 * @param[in] _name Pointer to the name of the file.
 * @param[in] _inodeNumD Inode number of the target directory.
 * @return True if the file was added successfully, false otherwise.
 * @date 2010-04-01 First implementation.
 */
static Bool add_file_to_dir(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                            unsigned int _inodeNumD) {
    Bool success;
    unsigned int i, j;
    unsigned int diskOffset;
    unsigned int buffer[BLOCK_SIZE / 4];
    unsigned int buffer2[BLOCK_SIZE / 4];
    success = fs_open_file(_fsm, _inodeNumD);
    if (success == True) {
        if (_fsm->inode.fileType == 2) {
            // Read file's direct pointers from disk
            for (i = 0; i < 10; i++) {
                if (_fsm->inode.directPtr[i] != (unsigned int)(-1)) {
                    diskOffset = _fsm->inode.directPtr[i];
                    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                    _fsm->sampleCount =
                        fread(buffer, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                    for (j = 0; j < BLOCK_SIZE / 4; j += 4) {
                        if (buffer[j + 3] == 0) {
                            buffer[j + 3] = 1;
                            buffer[j] = _name[0];
                            buffer[j + 1] = _name[1];
                            buffer[j + 2] = _inodeNumF;
                            _fsm->inode.linkCount += 1;
                            fseek(_fsm->diskHandle, 0, SEEK_SET);
                            fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                            _fsm->sampleCount = fwrite(buffer, sizeof(unsigned int), BLOCK_SIZE / 4,
                                                       _fsm->diskHandle);
                            fseek(_fsm->diskHandle, 0, SEEK_SET);
                            fs_close_file(_fsm);
                            return True;
                        }  // end if (buffer[j+3] == 0)
                    }  // end or (j = 0; j < BLOCK_SIZE/4; j += 4)
                }  // end if (_fsm->inode.directPtr[i] != (unsigned int)(-1))
            }  // end for (i = 0; i < 10; i++)
            if (_fsm->inode.sIndirect != (unsigned int)(-1)) {
                success = add_file_to_single_indirect(_fsm, _inodeNumF, _name,
                                                      _fsm->inode.sIndirect, False);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.sIndirect != (unsigned int)(-1))
            if (_fsm->inode.dIndirect != (unsigned int)(-1)) {
                success = add_file_to_double_indirect(_fsm, _inodeNumF, _name,
                                                      _fsm->inode.dIndirect, False);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end _fsm->inode.dIndirect != (unsigned int)(-1))
            if (_fsm->inode.tIndirect != (unsigned int)(-1)) {
                success = add_file_to_triple_indirect(_fsm, _inodeNumF, _name,
                                                      _fsm->inode.tIndirect, False);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.tIndirect != (unsigned int)(-1))
            for (i = 0; i < 10; i++) {
                if (_fsm->inode.directPtr[i] == (unsigned int)(-1)) {
                    // Get sectors for direct pointers
                    ssm_get_sector(1, _fsm->ssm);
                    if (_fsm->ssm->index[0] == (unsigned int)(-1)) {
                        // If sectors can't be retrieved, return false
                        return False;
                    }  // end if (_fsm->ssm->index[0] == (unsigned int)(-1))
                    else {
                        diskOffset =
                            BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));
                        _fsm->inode.directPtr[i] = diskOffset;
                        _fsm->sampleCount = fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                        // Clear buffer2
                        for (j = 0; j < BLOCK_SIZE / 4; j++) {
                            buffer2[j] = 0;
                        }  // end for (j = 0; j < BLOCK_SIZE/4; j++)
                        buffer2[3] = 1;
                        buffer2[0] = _name[0];
                        buffer2[1] = _name[1];
                        buffer2[2] = _inodeNumF;
                        _fsm->inode.linkCount += 1;
                        _fsm->inode.fileSize += BLOCK_SIZE;
                        _fsm->inode.dataBlocks = _fsm->inode.fileSize / BLOCK_SIZE;
                        _fsm->sampleCount =
                            fwrite(buffer2, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                        // Write file to disk
                        fseek(_fsm->diskHandle, 0, SEEK_SET);
                        inode_write(&_fsm->inode, _inodeNumD, _fsm->diskHandle);
                        ssm_allocate_sectors(_fsm->ssm);
                        fs_close_file(_fsm);
                        return True;
                    }  // end else (_fsm->ssm->index[0] == (unsigned int)(-1))
                }  // end if (_fsm->inode.directPtr[i] == (unsigned int)(-1))
            }  // end for (i = 0; i < 10; i++)
            // Add file to existing Single Indirect
            if (_fsm->inode.sIndirect != (unsigned int)(-1)) {
                // Add file to single indirect without allocation
                success = add_file_to_single_indirect(_fsm, _inodeNumF, _name,
                                                      _fsm->inode.sIndirect, False);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.sIndirect != (unsigned int)(-1))
            // Add file to existing Double Indirect
            if (_fsm->inode.dIndirect != (unsigned int)(-1)) {
                // Add file to double indirection without allocation
                success = add_file_to_double_indirect(_fsm, _inodeNumF, _name,
                                                      _fsm->inode.dIndirect, False);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.dIndirect != (unsigned int)(-1))
            // Add file to existing Triple Indirect
            if (_fsm->inode.tIndirect != (unsigned int)(-1)) {
                // Add file to triple indirection without allocation
                success = add_file_to_triple_indirect(_fsm, _inodeNumF, _name,
                                                      _fsm->inode.tIndirect, False);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.tIndirect != (unsigned int)(-1))
            // If can not find an open Single, Double or Triple Indirect pointer,
            // allocate a new one at lowest possible level
            // Add file to new Single Indirect
            if (_fsm->inode.sIndirect != (unsigned int)(-1)) {
                // Add file to single indirection with allocation
                success = add_file_to_single_indirect(_fsm, _inodeNumF, _name,
                                                      _fsm->inode.sIndirect, True);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.sIndirect != (unsigned int)(-1))
            // Add file to new Double Indirect
            if (_fsm->inode.dIndirect != (unsigned int)(-1)) {
                // Add file to double indirection with allocation
                success = add_file_to_double_indirect(_fsm, _inodeNumF, _name,
                                                      _fsm->inode.dIndirect, True);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.dIndirect != (unsigned int)(-1))
            // Add file to new Triple Indirect
            if (_fsm->inode.tIndirect != (unsigned int)(-1)) {
                // Add file to triple indirection with allocation
                success = add_file_to_triple_indirect(_fsm, _inodeNumF, _name,
                                                      _fsm->inode.tIndirect, True);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.tIndirect != (unsigned int)(-1))
            if (_fsm->inode.sIndirect == (unsigned int)(-1)) {
                ssm_get_sector(1, _fsm->ssm);
                if (_fsm->ssm->index[0] == (unsigned int)(-1)) {
                    return False;
                }  // end if (_fsm->ssm->index[0] == (unsigned int)(-1))
                else {
                    _fsm->inode.sIndirect =
                        BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));
                    for (i = 0; i < BLOCK_SIZE / 4; i++) {
                        buffer2[i] = (unsigned int)(-1);
                    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
                    diskOffset = _fsm->inode.sIndirect;
                    fseek(_fsm->diskHandle, 0, SEEK_SET);
                    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                    _fsm->sampleCount =
                        fwrite(buffer2, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                    ssm_allocate_sectors(_fsm->ssm);
                    fseek(_fsm->diskHandle, 0, SEEK_SET);
                    inode_write(&_fsm->inode, _inodeNumD, _fsm->diskHandle);
                }
            }  // end else (_fsm->ssm->index[0] == (unsigned int)(-1))
            success = add_file_to_single_indirect(_fsm, _inodeNumF, _name, diskOffset, True);
            if (success == True) {
                return True;
            }  // end if (success == True)
        }  // end if (_fsm->inode.sIndirect == (unsigned int)(-1))
        if (_fsm->inode.dIndirect == (unsigned int)(-1)) {
            ssm_get_sector(1, _fsm->ssm);
            if (_fsm->ssm->index[0] == (unsigned int)(-1)) {
                return False;
            }  // end if (_fsm->ssm->index[0] == (unsigned int)(-1))
            else {
                _fsm->inode.dIndirect =
                    BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));
                for (i = 0; i < BLOCK_SIZE / 4; i++) {
                    buffer2[i] = (unsigned int)(-1);
                }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
                diskOffset = _fsm->inode.dIndirect;
                fseek(_fsm->diskHandle, 0, SEEK_SET);
                fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                _fsm->sampleCount =
                    fwrite(buffer2, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                ssm_allocate_sectors(_fsm->ssm);
                fseek(_fsm->diskHandle, 0, SEEK_SET);
                inode_write(&_fsm->inode, _inodeNumD, _fsm->diskHandle);
            }  // end else (_fsm->ssm->index[0] == (unsigned int)(-1))
            success =
                add_file_to_double_indirect(_fsm, _inodeNumF, _name, _fsm->inode.dIndirect, True);
            if (success == True) {
                return True;
            }  // end if (success == True)
        }  // end if (_fsm->inode.dIndirect == (unsigned int)(-1))
        if (_fsm->inode.tIndirect == (unsigned int)(-1)) {
            ssm_get_sector(1, _fsm->ssm);
            if (_fsm->ssm->index[0] == (unsigned int)(-1)) {
                return False;
            }  // end if (_fsm->ssm->index[0] == (unsigned int)(-1))
            else {
                _fsm->inode.tIndirect =
                    BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));
                for (i = 0; i < BLOCK_SIZE / 4; i++) {
                    buffer2[i] = (unsigned int)(-1);
                }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
                diskOffset = _fsm->inode.tIndirect;
                fseek(_fsm->diskHandle, 0, SEEK_SET);
                fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                _fsm->sampleCount =
                    fwrite(buffer2, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                ssm_allocate_sectors(_fsm->ssm);
                fseek(_fsm->diskHandle, 0, SEEK_SET);
                inode_write(&_fsm->inode, _inodeNumD, _fsm->diskHandle);
            }  // end else (_fsm->ssm->index[0] == (unsigned int)(-1))
            success =
                add_file_to_triple_indirect(_fsm, _inodeNumF, _name, _fsm->inode.tIndirect, True);
            if (success == True) {
                return True;
            }  // end if (success == True)
        }  // end if (_fsm->inode.tIndirect == (unsigned int)(-1))
        else {
            // If filetype isn't "file" return false
            return False;
        }  // end else
    }  // end if (success == True)
    else {
        // If file didn't open, return false
        return False;
    }  // end else (success == True)
    // Fail case
    return False;
}

/**
 * @brief Adds a file to a triple indirect pointer.
 * Inserts a file into the structure at the specified triple indirect offset,
 * optionally allocating necessary blocks.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @param[in] _inodeNumF Inode number of the file to be added.
 * @param[in] _name Pointer to the name of the file.
 * @param[in] _tIndirectOffset Offset within the triple indirect block.
 * @param[in] _allocate If true, allocate blocks as needed.
 * @return True if the file was added successfully, false otherwise.
 * @date 2010-04-01 First implementation.
 */
static Bool add_file_to_triple_indirect(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                                        unsigned int _tIndirectOffset, Bool _allocate) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int buffer[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    Bool success;
    diskOffset = _tIndirectOffset;
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    unsigned int i;
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            success = add_file_to_double_indirect(_fsm, _inodeNumF, _name, diskOffset, _allocate);
            if (success == True) {
                return True;
            }  // end if (success == True)
        }  // end if (indirectBlock[i] != (unsigned int)(-1))
    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
    // Allocate more sectors to write to
    if (_allocate == True) {
        for (i = 0; i < BLOCK_SIZE / 4; i++) {
            if (indirectBlock[i] == (unsigned int)(-1)) {
                ssm_get_sector(1, _fsm->ssm);
                if (_fsm->ssm->index[0] == (unsigned int)(-1)) {
                    return False;
                }  // end if (_fsm->ssm->index[0] == (unsigned int)(-1))
                else {
                    indirectBlock[i] =
                        BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));
                    fseek(_fsm->diskHandle, _tIndirectOffset, SEEK_SET);
                    _fsm->sampleCount = fwrite(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
                    for (i = 0; i < BLOCK_SIZE / 4; i++) {
                        buffer[i] = (unsigned int)(-1);
                    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
                    fseek(_fsm->diskHandle, indirectBlock[i], SEEK_SET);
                    _fsm->sampleCount =
                        fwrite(buffer, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                    ssm_allocate_sectors(_fsm->ssm);
                }  // end else (_fsm->ssm->index[0] == (unsigned int)(-1))
                diskOffset = indirectBlock[i];
                success =
                    add_file_to_double_indirect(_fsm, _inodeNumF, _name, diskOffset, _allocate);
                if (success == True) {
                    return True;
                }  // end if(success == True)
            }  // end else
        }  // for (i = 0; i < BLOCK_SIZE/4; i++)
    }  // end if (_allocate == True)
    return False;
}

/**
 * @brief Adds a file to a double indirect pointer.
 * Inserts a file into the structure at the specified double indirect offset,
 * optionally allocating space as needed.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @param[in] _inodeNumF Inode number of the file to be added.
 * @param[in] _name Pointer to the name of the file.
 * @param[in] _dIndirectOffset Offset within the double indirect block.
 * @param[in] _allocate If true, allocate additional space as needed.
 * @return True if the file was added successfully, false otherwise.
 * @date 2010-04-01 First implementation.
 */
static Bool add_file_to_double_indirect(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                                        unsigned int _dIndirectOffset, Bool _allocate) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int buffer[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    Bool success;
    diskOffset = _dIndirectOffset;
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    unsigned int i;
    // Add file to a double indirect pointer
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            success = add_file_to_single_indirect(_fsm, _inodeNumF, _name, diskOffset, _allocate);
            if (success == True) {
                return True;
            }  // end if (success == True)
        }  // if (indirectBlock[i] != (unsigned int)(-1))
    }  // for (i = 0; i < BLOCK_SIZE/4; i++)
    if (_allocate == True) {
        for (i = 0; i < BLOCK_SIZE / 4; i++) {
            if (indirectBlock[i] == (unsigned int)(-1)) {
                ssm_get_sector(1, _fsm->ssm);
                if (_fsm->ssm->index[0] == (unsigned int)(-1)) {
                    return False;
                }  // end if (_fsm->ssm->index[0] == (unsigned int)(-1))
                else {
                    indirectBlock[i] =
                        BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));
                    fseek(_fsm->diskHandle, _dIndirectOffset, SEEK_SET);
                    _fsm->sampleCount = fwrite(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
                    for (i = 0; i < BLOCK_SIZE / 4; i++) {
                        buffer[i] = (unsigned int)(-1);
                    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
                    fseek(_fsm->diskHandle, indirectBlock[i], SEEK_SET);
                    _fsm->sampleCount =
                        fwrite(buffer, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                    ssm_allocate_sectors(_fsm->ssm);
                }  // end else (_fsm->ssm->index[0] == (unsigned int)(-1))
                diskOffset = indirectBlock[i];
                success =
                    add_file_to_single_indirect(_fsm, _inodeNumF, _name, diskOffset, _allocate);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (indirectBlock[i] == (unsigned int)(-1))
        }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
    }  // end if (_allocate == True)
    return False;
}

/**
 * @brief Adds a file to a single indirect pointer.
 * Inserts a file at the specified single indirect offset, optionally allocating space if required.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @param[in] _inodeNumF Inode number of the file to be added.
 * @param[in] _name Pointer to the name of the file.
 * @param[in] _sIndirectOffset Offset within the single indirect block.
 * @param[in] _allocate If true, allocate space as needed.
 * @return True if the file was added successfully, false otherwise.
 * @date 2010-04-01 First implementation.
 */
static Bool add_file_to_single_indirect(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                                        unsigned int _sIndirectOffset, Bool _allocate) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    unsigned int buffer[BLOCK_SIZE / 4];
    diskOffset = _sIndirectOffset;  //_fsm->inode.sIndirect;
    // Read indirect block
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    unsigned int i, j;
    if (_allocate == False) {
        for (i = 0; i < BLOCK_SIZE / 4; i++) {
            if (indirectBlock[i] != (unsigned int)(-1)) {
                diskOffset = indirectBlock[i];
                fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                _fsm->sampleCount = fread(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);
                for (j = 0; j < BLOCK_SIZE / 4; j += 4) {
                    if (buffer[j + 3] == 0) {
                        buffer[j + 3] = 1;
                        buffer[j] = _name[0];
                        buffer[j + 1] = _name[1];
                        buffer[j + 2] = _inodeNumF;
                        _fsm->inode.linkCount += 1;
                        fseek(_fsm->diskHandle, 0, SEEK_SET);
                        fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                        _fsm->sampleCount =
                            fwrite(buffer, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                        fseek(_fsm->diskHandle, 0, SEEK_SET);
                        fs_close_file(_fsm);
                        return True;
                    }  // end if (buffer[j+3] == 0)
                }  // for (j = 0; j < BLOCK_SIZE/4; j += 4)
            }  // if (indirectBlock[i] != (unsigned int)(-1))
        }  // for (i = 0; i < BLOCK_SIZE/4; i++)
    }  // if (_allocate == False)
    else {
        // Allocate space then write
        for (i = 0; i < BLOCK_SIZE / 4; i++) {
            if (indirectBlock[i] == (unsigned int)(-1)) {
                ssm_get_sector(1, _fsm->ssm);
                if (_fsm->ssm->index[0] == (unsigned int)(-1)) {
                    return False;
                }  // end if(_fsm->ssm->index[0] == (unsigned int)(-1))
                else {
                    indirectBlock[i] =
                        BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));
                    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                    _fsm->sampleCount = fwrite(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
                    for (j = 0; j < BLOCK_SIZE / 4; j++) {
                        buffer[j] = 0;
                    }  // end for (j = 0; j < BLOCK_SIZE/4; j++)
                    buffer[3] = 1;
                    buffer[0] = _name[0];
                    buffer[1] = _name[1];
                    buffer[2] = _inodeNumF;
                    _fsm->inode.linkCount += 1;
                    _fsm->inode.fileSize += BLOCK_SIZE;
                    _fsm->inode.dataBlocks = _fsm->inode.fileSize / BLOCK_SIZE;
                    fseek(_fsm->diskHandle, indirectBlock[i], SEEK_SET);
                    _fsm->sampleCount =
                        fwrite(buffer, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                    fseek(_fsm->diskHandle, 0, SEEK_SET);
                    ssm_allocate_sectors(_fsm->ssm);
                    fs_close_file(_fsm);
                    return True;
                }  // if (_fsm->ssm->index[0] == (unsigned int)(-1))
            }  // if (indirectBlock[i] == (unsigned int)(-1))
        }  // for (i = 0; i < BLOCK_SIZE/4; i++)
    }  // end else
    return False;
}

Bool fs_remove_file_from_dir(FSM *_fsm, unsigned int _inodeNumF, unsigned int _inodeNumD) {
    Bool success;
    unsigned int i, j, k;
    unsigned int diskOffset;
    unsigned int buffer[BLOCK_SIZE / 4];
    unsigned int sectorNum;
    // open file, return false if access error
    success = fs_open_file(_fsm, _inodeNumD);
    if (success == True) {
        if (_fsm->inode.fileType == 2) {  // type 2 is directory
            for (i = 0; i < 10; i++) {
                if (_fsm->inode.directPtr[i] != (unsigned int)(-1)) {
                    diskOffset = _fsm->inode.directPtr[i];
                    // update sampleCount
                    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                    _fsm->sampleCount =
                        fread(buffer, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                    // clear all 4 bytes on each loop
                    for (j = 0; j < BLOCK_SIZE / 4; j += 4) {
                        if (buffer[j + 3] == 1 && buffer[j + 2] == _inodeNumF) {
                            buffer[j + 3] = 0;
                            buffer[j] = 0;
                            buffer[j + 1] = 0;
                            buffer[j + 2] = 0;
                            _fsm->inode.linkCount -= 1;
                            if (_fsm->inode.linkCount == 0) {
                                // if there is nothing in the directory, set size to 0
                                _fsm->inode.fileSize = 0;
                                // deallocate all associated sectors
                                for (k = 0; k < 10; k++) {
                                    _fsm->inode.directPtr[k] = -1;
                                }  // end for (k = 0; k < 10; k++)
                                _fsm->inode.sIndirect = -1;
                                _fsm->inode.dIndirect = -1;
                                _fsm->inode.tIndirect = -1;
                            }  // end if (_fsm->inode.linkCount == 0)
                            fseek(_fsm->diskHandle, 0, SEEK_SET);
                            fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                            _fsm->sampleCount = fwrite(buffer, sizeof(unsigned int), BLOCK_SIZE / 4,
                                                       _fsm->diskHandle);
                            for (k = 0; k < BLOCK_SIZE / 4; k += 4) {
                                if (buffer[k + 3] == 1) {
                                    break;
                                }  // end if (buffer[k+3] == 1)
                            }  // end for (k = 0; k < BLOCK_SIZE/4; k += 4)
                            if (k == BLOCK_SIZE / 4) {
                                sectorNum = _fsm->inode.directPtr[i] / BLOCK_SIZE;
                                _fsm->ssm->contSectors = 1;
                                _fsm->ssm->index[0] = sectorNum / 8;
                                _fsm->ssm->index[1] = sectorNum % 8;
                                ssm_deallocate_sectors(_fsm->ssm);
                                _fsm->inode.directPtr[i] = (unsigned int)(-1);
                                _fsm->inode.dataBlocks -= 1;
                                inode_write(&_fsm->inode, _fsm->inodeNum, _fsm->diskHandle);
                            }  // end if (k == BLOCK_SIZE/4)
                            return True;
                        }  // end if (buffer[j+3] == 1 && buffer[j+2] == _inodeNumF)
                    }  // end for (j = 0; j < BLOCK_SIZE/4; j += 4)
                }  // end if (_fsm->inode.directPtr[i] != (unsigned int)(-1))
            }  // end for (i = 0; i < 10; i++)
            // if the directory has files in it's sindirect pointer area, remove them
            if (_fsm->inode.sIndirect != (unsigned int)(-1)) {
                success = remove_file_from_single_indirect(_fsm, _inodeNumF, (unsigned int)(-1),
                                                           _fsm->inode.sIndirect);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.sIndirect != (unsigned int)(-1))
            // if the directory has files in it's dindirect pointer area, remove them
            if (_fsm->inode.dIndirect != (unsigned int)(-1)) {
                success = remove_file_from_double_indirect(_fsm, _inodeNumF, (unsigned int)(-1),
                                                           _fsm->inode.dIndirect);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.dIndirect != (unsigned int)(-1))
            // if the directory has files in it's tindirect pointer area, remove them
            if (_fsm->inode.tIndirect != (unsigned int)(-1)) {
                success = remove_file_from_triple_indirect(_fsm, _inodeNumF, _fsm->inode.tIndirect);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.tIndirect != (unsigned int)(-1))
        }  // end if (_fsm->inode.fileType == 2)
    }  // end if (success == True)
    // file failed to open, return false
    return False;
}

/**
 * @brief Removes a file from a triple indirect pointer.
 * Removes the file identified by `_inodeNumF` from the triple indirect block at the specified
 * offset. Internally calls `rmFileFrom_D_Indirect` to complete the operation.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @param[in] _inodeNumF Inode number of the file to be removed.
 * @param[in] _tIndirectOffset Offset to the triple indirect block containing the file.
 * @return True if the file was successfully removed, false if the file could not be accessed.
 * @date 2010-04-01 First implementation.
 */
static Bool remove_file_from_triple_indirect(FSM *_fsm, unsigned int _inodeNumF,
                                             unsigned int _tIndirectOffset) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    unsigned int sectorNum;
    Bool success;
    diskOffset = _tIndirectOffset;
    // set sampleCount
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    unsigned int i, k;
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            success =
                remove_file_from_double_indirect(_fsm, _inodeNumF, _tIndirectOffset, diskOffset);
            if (success == True) {
                diskOffset = _tIndirectOffset;
                fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
                for (k = 0; k < BLOCK_SIZE / 4; k++) {
                    if (indirectBlock[k] != (unsigned int)(-1)) {
                        break;
                    }  // end if (indirectBlock[k] != (unsigned int)(-1))
                }  // end for (k = 0; k < BLOCK_SIZE/4; k++)
                if (k == BLOCK_SIZE / 4) {
                    sectorNum = _tIndirectOffset / BLOCK_SIZE;
                    _fsm->ssm->contSectors = 1;
                    _fsm->ssm->index[0] = sectorNum / 8;
                    _fsm->ssm->index[1] = sectorNum % 8;
                    ssm_deallocate_sectors(_fsm->ssm);
                    _fsm->inode.tIndirect = (unsigned int)(-1);
                    inode_write(&_fsm->inode, _fsm->inodeNum, _fsm->diskHandle);
                }  // end if (k == BLOCK_SIZE/4)
                return True;
            }  // if (success == True)
        }  // if (indirectBlock[i] != (unsigned int)(-1))
    }  // for (i = 0; i < BLOCK_SIZE/4; i++)
    return False;
}

/**
 * @brief Removes a file from a double indirect pointer.
 * Removes the file identified by `_inodeNumF` from the double indirect block at the specified
 * offset. Typically called by higher-level triple indirect removal logic.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @param[in] _inodeNumF Inode number of the file to be removed.
 * @param[in] _tIndirectOffset Offset to the parent triple indirect block.
 * @param[in] _dIndirectOffset Offset to the double indirect block containing the file.
 * @return True if the file was successfully removed, false if the file could not be accessed.
 * @date 2010-04-01 First implementation.
 */
static Bool remove_file_from_double_indirect(FSM *_fsm, unsigned int _inodeNumF,
                                             unsigned int _tIndirectOffset,
                                             unsigned int _dIndirectOffset) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    unsigned int sectorNum;
    Bool success;
    // load next pointer
    diskOffset = _dIndirectOffset;
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    unsigned int i, k;
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            success =
                remove_file_from_single_indirect(_fsm, _inodeNumF, _dIndirectOffset, diskOffset);
            if (success == True) {
                diskOffset = _dIndirectOffset;
                fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
                for (k = 0; k < BLOCK_SIZE / 4; k++) {
                    if (indirectBlock[k] != (unsigned int)(-1)) {
                        break;
                    }  // if (indirectBlock[k] != (unsigned int)(-1))
                }  // end for (k = 0; k < BLOCK_SIZE/4; k++)
                if (k == BLOCK_SIZE / 4) {
                    sectorNum = _dIndirectOffset / BLOCK_SIZE;
                    _fsm->ssm->contSectors = 1;
                    _fsm->ssm->index[0] = sectorNum / 8;
                    _fsm->ssm->index[1] = sectorNum % 8;
                    ssm_deallocate_sectors(_fsm->ssm);
                    if (_tIndirectOffset != (unsigned int)(-1)) {
                        diskOffset = _tIndirectOffset;
                        fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                        _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
                        for (k = 0; k < BLOCK_SIZE / 4; k++) {
                            if (indirectBlock[k] == _dIndirectOffset) {
                                indirectBlock[k] = (unsigned int)(-1);
                                break;
                            }  // end if (indirectBlock[k] == _dIndirectOffset)
                        }  // end for (k = 0; k < BLOCK_SIZE/4; k++)
                        fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                        _fsm->sampleCount = fwrite(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
                    } else {
                        _fsm->inode.dIndirect = (unsigned int)(-1);
                        inode_write(&_fsm->inode, _fsm->inodeNum, _fsm->diskHandle);
                    }  // end else
                }  // if (k == BLOCK_SIZE/4)
                return True;
            }  // end if (success == True)
        }  // end if (indirectBlock[i] != (unsigned int)(-1))
    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
    return False;
}

/**
 * @brief Removes a file from a single indirect pointer.
 * Removes the file identified by `_inodeNumF` from the single indirect block located
 * via the specified double and single indirect offsets.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @param[in] _inodeNumF Inode number of the file to be removed.
 * @param[in] _dIndirectOffset Offset to the parent double indirect block.
 * @param[in] _sIndirectOffset Offset to the single indirect block containing the file.
 * @return True if the file was successfully removed, false if the file could not be accessed.
 * @date 2010-04-01 First implementation.
 */
static Bool remove_file_from_single_indirect(FSM *_fsm, unsigned int _inodeNumF,
                                             unsigned int _dIndirectOffset,
                                             unsigned int _sIndirectOffset) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    unsigned int buffer[BLOCK_SIZE / 4];
    unsigned int sectorNum;
    diskOffset = _sIndirectOffset;
    // set simpleCount
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    unsigned int i, j, k;
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
            _fsm->sampleCount = fread(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);
            // clear all data in block
            for (j = 0; j < BLOCK_SIZE / 4; j += 4) {
                if (buffer[j + 3] == 1 && buffer[j + 2] == _inodeNumF) {
                    buffer[j + 3] = 0;
                    buffer[j] = 0;
                    buffer[j + 1] = 0;
                    buffer[j + 2] = 0;
                    _fsm->inode.linkCount -= 1;
                    fseek(_fsm->diskHandle, 0, SEEK_SET);
                    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                    _fsm->sampleCount =
                        fwrite(buffer, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                    for (k = 0; k < BLOCK_SIZE / 4; k += 4) {
                        if (buffer[k + 3] == 1) {
                            break;
                        }  // end if (buffer[k+3] == 1)
                    }  // end for (k = 0; k < BLOCK_SIZE/4; k += 4)
                    if (k == BLOCK_SIZE / 4) {
                        sectorNum = indirectBlock[i] / BLOCK_SIZE;
                        _fsm->ssm->contSectors = 1;
                        _fsm->ssm->index[0] = sectorNum / 8;
                        _fsm->ssm->index[1] = sectorNum % 8;
                        ssm_deallocate_sectors(_fsm->ssm);
                        _fsm->inode.dataBlocks -= 1;
                        inode_write(&_fsm->inode, _fsm->inodeNum, _fsm->diskHandle);
                        indirectBlock[i] = (unsigned int)(-1);
                        diskOffset = _sIndirectOffset;
                        fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                        _fsm->sampleCount = fwrite(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
                        for (k = 0; k < BLOCK_SIZE / 4; k++) {
                            if (indirectBlock[k] != (unsigned int)(-1)) {
                                break;
                            }  // end if (indirectBlock[k] != (unsigned int)(-1))
                        }  // end for (k = 0; k < BLOCK_SIZE/4; k++)
                        if (k == BLOCK_SIZE / 4) {
                            sectorNum = _sIndirectOffset / BLOCK_SIZE;
                            _fsm->ssm->contSectors = 1;
                            _fsm->ssm->index[0] = sectorNum / 8;
                            _fsm->ssm->index[1] = sectorNum % 8;
                            ssm_deallocate_sectors(_fsm->ssm);
                            if (_dIndirectOffset != (unsigned int)(-1)) {
                                diskOffset = _dIndirectOffset;
                                fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                                _fsm->sampleCount =
                                    fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
                                for (k = 0; k < BLOCK_SIZE / 4; k++) {
                                    if (indirectBlock[k] == _sIndirectOffset) {
                                        indirectBlock[k] = (unsigned int)(-1);
                                        break;
                                    }  // end if (indirectBlock[k] == _sIndirectOffset)
                                }  // end for (k = 0; k < BLOCK_SIZE/4; k++)
                                fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                                _fsm->sampleCount =
                                    fwrite(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
                            }  // end if (_dIndirectOffset != (unsigned int)(-1))
                            else {
                                _fsm->inode.sIndirect = (unsigned int)(-1);
                                inode_write(&_fsm->inode, _fsm->inodeNum, _fsm->diskHandle);
                            }  // end else
                        }  // end if (k == BLOCK_SIZE/4)
                    }  // end if (k == BLOCK_SIZE/4)
                    return True;
                }  // end if (buffer[j+3] == 1 && buffer[j+2] == _inodeNumF)
            }  // end for (j = 0; j < BLOCK_SIZE/4; j += 4)
        }  // end if (indirectBlock[i] != (unsigned int)(-1))
    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
    return False;
}

/**
 * @brief Reads pointers from a triple indirect block into a buffer.
 * Fills the provided buffer with all pointers from a specified triple indirect block,
 * one block at a time. Internally calls `readFrom_D_IndirectBlocks`.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[out] _buffer Pointer to the buffer where the read data will be stored.
 * @param[in] _diskOffset Offset to the first usable block on disk.
 * @return void
 * @date 2010-04-12 First implementation.
 */
static void read_from_triple_indirect_blocks(FSM *_fsm, void *_buffer, unsigned int _diskOffset) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    void *buffer;
    buffer = _buffer;
    diskOffset = _diskOffset;
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    unsigned int i;
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            read_from_double_indirect_blocks(_fsm, buffer, diskOffset);
            buffer += BLOCK_SIZE * D_INDIRECT_BLOCKS;
        }  // end if (indirectBlock[i] != (unsigned int)(-1))
    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
}

/**
 * @brief Reads pointers from a double indirect block into a buffer.
 * Fills the provided buffer with all pointers from a specified double indirect block,
 * one block at a time. Internally calls `readFrom_S_IndirectBlocks`.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[out] _buffer Pointer to the buffer where the read data will be stored.
 * @param[in] _diskOffset Offset to the first usable block on disk.
 * @return void
 * @date 2010-04-12 First implementation.
 */
static void read_from_double_indirect_blocks(FSM *_fsm, void *_buffer, unsigned int _diskOffset) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    void *buffer;
    buffer = _buffer;
    diskOffset = _diskOffset;
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    unsigned int i;
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            read_from_single_indirect_blocks(_fsm, buffer, diskOffset);
            buffer += BLOCK_SIZE * S_INDIRECT_BLOCKS;
        }  // end if (indirectBlock[i] != (unsigned int)(-1))
    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
}

/**
 * @brief Reads pointers from a single indirect block into a buffer.
 * Fills the provided buffer with all pointers from a specified single indirect block,
 * one block at a time.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[out] _buffer Pointer to the buffer where the read data will be stored.
 * @param[in] _diskOffset Offset to the first usable block on disk.
 * @return void
 * @date 2010-04-12 First implementation.
 */
static void read_from_single_indirect_blocks(FSM *_fsm, void *_buffer, unsigned int _diskOffset) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    void *buffer;
    buffer = _buffer;
    diskOffset = _diskOffset;
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    unsigned int i;
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
            _fsm->sampleCount = fread(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);
            buffer += BLOCK_SIZE;
        }  // end if (indirectBlock[i] != (unsigned int)(-1))
    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
}

/**
 * @brief Allocates a triple indirect block and all underlying levels of pointers.
 * Allocates sectors via the SSM to form a triple indirect structure:
 * a block pointing to blocks of pointers, which in turn point to more pointer blocks.
 * Returns the address of the top-level triple indirect block.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _blockCount Number of blocks to allocate.
 * @return The address of the triple indirect block on success, or -1 on failure.
 * @date 2010-04-12 First implementation.
 */
static unsigned int aloc_triple_indirect(FSM *_fsm, long long int _blockCount) {
    unsigned int baseAddress;
    unsigned int address;
    unsigned int diskOffset;
    long long int blockCount;
    ssm_get_sector(1, _fsm->ssm);
    if (_fsm->ssm->index[0] != (unsigned int)(-1)) {
        baseAddress = BLOCK_SIZE * (8 * _fsm->ssm->index[0] + _fsm->ssm->index[1]);
        ssm_allocate_sectors(_fsm->ssm);
        diskOffset = baseAddress;  //_fsm->inode.tIndirect;
        // Initialize the indirect block pointers to -1
        unsigned int i;
        unsigned int base[BLOCK_SIZE / 4];
        for (i = 0; i < BLOCK_SIZE / 4; i++) {
            base[i] = (unsigned int)(-1);
        }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
        fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
        _fsm->sampleCount = fwrite(base, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
        // Allocate _blockCount blocks and store their pointers in
        // the indirect block
        blockCount = _blockCount;
        for (i = 0; i < PTRS_PER_BLOCK; i++) {
            address = aloc_double_indirect(_fsm, _blockCount);
            // write to buffer
            fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
            _fsm->sampleCount = fwrite(&address, sizeof(unsigned int), 1, _fsm->diskHandle);
            diskOffset += 4;
            // update block count
            blockCount -= D_INDIRECT_BLOCKS;
            if (blockCount < 0) {
                break;
            }  // end if (blockCount < 0)
        }  // end for (i = 0; i < PTRS_PER_BLOCK; i++)
        // return the address of the tindirect block
        return baseAddress;
    }  // end if (_fsm->ssm->index[0] != (unsigned int)(-1))
    else {
        return (unsigned int)(-1);
    }  // end else
}

/**
 * @brief Allocates a double indirect block and its underlying pointer blocks.
 * Allocates sectors via the SSM to form a double indirect structure:
 * a block pointing to blocks of pointers, which in turn point to data blocks.
 * Returns the address of the top-level double indirect block.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _blockCount Number of blocks to allocate.
 * @return The address of the double indirect block on success, or -1 on failure.
 * @date 2010-04-12 First implementation.
 */
static unsigned int aloc_double_indirect(FSM *_fsm, long long int _blockCount) {
    unsigned int baseAddress;
    unsigned int address;
    unsigned int diskOffset;
    long long int blockCount;
    ssm_get_sector(1, _fsm->ssm);
    // calculate base address
    if (_fsm->ssm->index[0] != (unsigned int)(-1)) {
        baseAddress = BLOCK_SIZE * (8 * _fsm->ssm->index[0] + _fsm->ssm->index[1]);
        ssm_allocate_sectors(_fsm->ssm);
        diskOffset = baseAddress;  //_fsm->inode.sIndirect;
        // Initialize the indirect block pointers to -1
        unsigned int i;
        unsigned int base[BLOCK_SIZE / 4];
        for (i = 0; i < BLOCK_SIZE / 4; i++) {
            base[i] = (unsigned int)(-1);
        }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
        fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
        _fsm->sampleCount = fwrite(base, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
        // Allocate _blockCount blocks and store their pointers in
        // the indirect block
        blockCount = _blockCount;
        for (i = 0; i < PTRS_PER_BLOCK; i++) {
            address = aloc_single_indirect(_fsm, _blockCount);
            // write to buffer
            fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
            _fsm->sampleCount = fwrite(&address, sizeof(unsigned int), 1, _fsm->diskHandle);
            diskOffset += 4;
            // update block count
            blockCount -= S_INDIRECT_BLOCKS;
            if (blockCount < 0) {
                break;
            }  // end if (blockCount < 0
        }  // end for (i = 0; i < PTRS_PER_BLOCK; i++)
        // return the address of the Dindirect block
        return baseAddress;
    } else {
        return (unsigned int)(-1);
    }  // end else
}

/**
 * @brief Allocates a single indirect block and its associated data blocks.
 * Allocates a sector from the SSM, fills it with pointers to other sectors,
 * and returns the address of the allocated single indirect block.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _blockCount Number of blocks to allocate.
 * @return The address of the single indirect block on success, or -1 on failure.
 * @date 2010-04-12 First implementation.
 */
static unsigned int aloc_single_indirect(FSM *_fsm, long long int _blockCount) {
    unsigned int baseAddress;
    unsigned int address;
    unsigned int diskOffset;
    ssm_get_sector(1, _fsm->ssm);
    // calculate base address
    if (_fsm->ssm->index[0] != (unsigned int)(-1)) {
        baseAddress = BLOCK_SIZE * (8 * _fsm->ssm->index[0] + _fsm->ssm->index[1]);
        ssm_allocate_sectors(_fsm->ssm);
        diskOffset = baseAddress;  //_fsm->inode.sIndirect;
        // Initialize the indirect block pointers to -1
        unsigned int i;
        unsigned int base[BLOCK_SIZE / 4];
        for (i = 0; i < BLOCK_SIZE / 4; i++) {
            base[i] = (unsigned int)(-1);
        }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
        fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
        _fsm->sampleCount = fwrite(base, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
        // Allocate _blockCount blocks and store associated pointers in
        // the indirect block
        for (i = 0; i < _blockCount && i < PTRS_PER_BLOCK; i++) {
            ssm_get_sector(1, _fsm->ssm);
            if (_fsm->ssm->index[0] != (unsigned int)(-1)) {
                address = BLOCK_SIZE * (8 * _fsm->ssm->index[0] + _fsm->ssm->index[1]);
                ssm_allocate_sectors(_fsm->ssm);
                // write to buffer
                fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                _fsm->sampleCount = fwrite(&address, sizeof(unsigned int), 1, _fsm->diskHandle);
                diskOffset += 4;
            }  // end if (_fsm->ssm->index[0] != (unsigned int)(-1))
        }  // end for (i = 0; i < _blockCount && i < PTRS_PER_BLOCK; i++
        // return the address of the sindirect block
        return baseAddress;
    } else {
        return (unsigned int)(-1);
    }  // end else
}

/**
 * @brief Writes data into triple indirect blocks one block at a time.
 * Writes the contents of the given buffer into a triple indirect structure,
 * starting at the specified base offset. Internally calls `writeTo_D_IndirectBlocks`
 * to handle the lower levels.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _baseOffset Offset to the first usable data block on disk.
 * @param[in] _buffer Pointer to the buffer containing data to be written.
 * @param[in] _tIndirectPtrs Number of blocks to allocate through the triple indirect pointer.
 * @return void
 * @date 2010-04-12 First implementation.
 */
static void write_to_triple_indirect_blocks(FSM *_fsm, unsigned int _baseOffset, void *_buffer,
                                            unsigned int _tIndirectPtrs) {
    unsigned int diskOffset;
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    void *buffer;
    unsigned int tIndirectPtrs;
    buffer = _buffer;
    tIndirectPtrs = _tIndirectPtrs;
    fseek(_fsm->diskHandle, _baseOffset, SEEK_SET);
    _fsm->sampleCount = fread(&indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    // write to each of the sIndirectPtrs blocks
    unsigned int i;
    for (i = 0; i < PTRS_PER_BLOCK; i++) {
        diskOffset = indirectBlock[i];
        if (diskOffset == (unsigned int)(-1)) {
            break;
        }  // end if (diskOffset == (unsigned int)(-1))
        else {
            if (tIndirectPtrs - D_INDIRECT_BLOCKS > 0) {
                // write a dindirect worth of blocks then continue looping
                write_to_double_indirect_blocks(_fsm, diskOffset, buffer, D_INDIRECT_BLOCKS);
                buffer += BLOCK_SIZE * D_INDIRECT_BLOCKS;
                tIndirectPtrs -= D_INDIRECT_BLOCKS;
            }  // end if (tIndirectPtrs - D_INDIRECT_BLOCKS > 0)
            else if (tIndirectPtrs - D_INDIRECT_BLOCKS == 0) {
                // write the rest into a dindirect block
                write_to_double_indirect_blocks(_fsm, diskOffset, buffer, D_INDIRECT_BLOCKS);
                buffer += BLOCK_SIZE * D_INDIRECT_BLOCKS;
                tIndirectPtrs -= D_INDIRECT_BLOCKS;
                break;
            } else {
                // write the rest into a dindirect block
                write_to_double_indirect_blocks(_fsm, diskOffset, buffer, tIndirectPtrs);
                buffer += BLOCK_SIZE * tIndirectPtrs;
                tIndirectPtrs = 0;
                break;
            }  // end else
        }  // end else
    }  // end for (i = 0; i < PTRS_PER_BLOCK; i++)
}

/**
 * @brief Writes data into double indirect blocks one block at a time.
 * Writes the contents of the given buffer into a double indirect structure,
 * starting at the specified base offset. Internally calls `writeTo_S_IndirectBlocks`
 * to handle the single indirect level.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _baseOffset Offset to the first usable data block on disk.
 * @param[in] _buffer Pointer to the buffer containing data to be written.
 * @param[in] _dIndirectPtrs Number of blocks to allocate through the double indirect pointer.
 * @return void
 * @date 2010-04-12 First implementation.
 */
static void write_to_double_indirect_blocks(FSM *_fsm, unsigned int _baseOffset, void *_buffer,
                                            unsigned int _dIndirectPtrs) {
    unsigned int diskOffset;
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    void *buffer;
    unsigned int dIndirectPtrs;
    buffer = _buffer;
    dIndirectPtrs = _dIndirectPtrs;
    fseek(_fsm->diskHandle, _baseOffset, SEEK_SET);
    _fsm->sampleCount = fread(&indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    // write to each of the sIndirectPtrs blocks
    unsigned int i;
    for (i = 0; i < PTRS_PER_BLOCK; i++) {
        diskOffset = indirectBlock[i];
        if (diskOffset == (unsigned int)(-1)) {
            break;
        }  // end if (diskOffset == (unsigned int)(-1))
        else {
            // write a sindirect block at a time
            if (dIndirectPtrs - S_INDIRECT_BLOCKS > 0) {
                write_to_single_indirect_blocks(_fsm, diskOffset, buffer, S_INDIRECT_BLOCKS);
                buffer += BLOCK_SIZE * S_INDIRECT_BLOCKS;
                dIndirectPtrs -= S_INDIRECT_BLOCKS;
            }  // end if (dIndirectPtrs - S_INDIRECT_BLOCKS > 0)
            // write the rest into a sindirect block
            else if (dIndirectPtrs - S_INDIRECT_BLOCKS == 0) {
                write_to_single_indirect_blocks(_fsm, diskOffset, buffer, S_INDIRECT_BLOCKS);
                buffer += BLOCK_SIZE * S_INDIRECT_BLOCKS;
                dIndirectPtrs -= S_INDIRECT_BLOCKS;
                break;
            }  // end else if (dIndirectPtrs - S_INDIRECT_BLOCKS == 0)
            // write the rest into a sindirect block
            else {
                write_to_single_indirect_blocks(_fsm, diskOffset, buffer, dIndirectPtrs);
                buffer += BLOCK_SIZE * dIndirectPtrs;
                dIndirectPtrs = 0;
                break;
            }  // end else
        }  // end else
    }  // end for (i = 0; i < PTRS_PER_BLOCK; i++)
}

/**
 * @brief Writes data into single indirect blocks one block at a time.
 * Writes the contents of the provided buffer into a single indirect structure,
 * starting at the specified base offset.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _baseOffset Offset to the first usable data block on disk.
 * @param[in] _buffer Pointer to the buffer containing data to be written.
 * @param[in] _sIndirectPtrs Number of blocks to allocate through the single indirect pointer.
 * @return void
 * @date 2010-04-12 First implementation.
 */
static void write_to_single_indirect_blocks(FSM *_fsm, unsigned int _baseOffset, void *_buffer,
                                            unsigned int _sIndirectPtrs) {
    unsigned int diskOffset;
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    void *buffer;
    buffer = _buffer;
    // read the single indirect pointer
    fseek(_fsm->diskHandle, _baseOffset, SEEK_SET);
    _fsm->sampleCount = fread(&indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    // write to each of the sIndirectPtrs blocks
    unsigned int i;
    for (i = 0; i < _sIndirectPtrs; i++) {
        // get next pointer
        diskOffset = indirectBlock[i];
        // if next pointer is unused, break out of loop
        if (diskOffset == (unsigned int)(-1)) {
            break;
        } else {
            fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
            _fsm->sampleCount = fwrite(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);
            buffer += BLOCK_SIZE;  // increment buffer
        }  // end else
    }  // end for (i = 0; i < _sIndirectPtrs; i++)
}

Bool fs_remove_file(FSM *_fsm, unsigned int _inodeNum, unsigned int _inodeNumD) {
    // Open file at Inode _inodeNum for reading
    Bool success;
    success = fs_open_file(_fsm, _inodeNum);
    if (success == False) {
        return False;
    }  // end if (success == False)
    unsigned int directPtrs[10];
    unsigned int sIndirect;
    unsigned int dIndirect;
    unsigned int tIndirect;
    unsigned int fileType;
    unsigned int buffer[BLOCK_SIZE / 4];
    unsigned int sectorNumber;
    unsigned int byte;
    unsigned int bit;
    unsigned int i, j;
    unsigned int diskOffset;
    fileType = _fsm->inode.fileType;
    sIndirect = _fsm->inode.sIndirect;
    dIndirect = _fsm->inode.dIndirect;
    tIndirect = _fsm->inode.tIndirect;
    for (i = 0; i < 10; i++) {
        directPtrs[i] = _fsm->inode.directPtr[i];
    }  // end for (i = 0; i < 10; i++)
    // Read data from direct pointers into buffer _buffer
    for (i = 0; i < 10; i++) {
        diskOffset = directPtrs[i];
        if (diskOffset != (unsigned int)(-1)) {
            if (fileType == 2) {
                fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                _fsm->sampleCount =
                    fread(buffer, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                for (j = 8; j < BLOCK_SIZE / 4; j += 4) {
                    if (buffer[j + 3] == 1) {
                        fs_remove_file(_fsm, buffer[j + 2], _inodeNum);
                    }  // end if (buffer[j+3] == 1)
                }  // end for (j = 8; j < BLOCK_SIZE/4; j += 4)
            }  // end if (fileType == 2)
            sectorNumber = diskOffset / BLOCK_SIZE;
            byte = sectorNumber / 8;
            bit = sectorNumber % 8;
            _fsm->ssm->contSectors = 1;
            _fsm->ssm->index[0] = byte;
            _fsm->ssm->index[1] = bit;
            ssm_deallocate_sectors(_fsm->ssm);
        }  // end if (diskOffset != (unsigned int)(-1))
    }  // end for (i = 0; i < 10; i++)
    // Read data from single indirect pointer into buffer _buffer
    diskOffset = sIndirect;
    if (diskOffset != (unsigned int)(-1)) {
        if (fileType == 1) {
            // If remove directory, pass inodeNumD
            remove_file_single_indirect_blocks(_fsm, fileType, _inodeNumD, diskOffset);
        }  // end if (fileType == 1)
        else if (fileType == 2) {
            // If remove file, pass inodeNum
            remove_file_single_indirect_blocks(_fsm, fileType, _inodeNum, diskOffset);
        }  // end else if (fileType == 2)
    }  // end if (diskOffset != (unsigned int)(-1))
    // Read data from double indirect pointer into buffer _buffer
    diskOffset = dIndirect;
    if (diskOffset != (unsigned int)(-1)) {
        if (fileType == 1) {
            // If remove directory, pass inodeNumD
            remove_file_double_indirect_blocks(_fsm, fileType, _inodeNumD, diskOffset);
        }  // end if (fileType == 1)
        else if (fileType == 2) {
            // If remove file, pass inodeNum
            remove_file_double_indirect_blocks(_fsm, fileType, _inodeNum, diskOffset);
        }  // end else if (fileType == 2)
    }  // end if (diskOffset != (unsigned int)(-1))
    // Read data from triple indirect pointer into buffer _buffer
    diskOffset = tIndirect;
    if (diskOffset != (unsigned int)(-1)) {
        if (fileType == 1) {
            // If remove directory, pass inodeNumD
            remove_file_triple_indirect_blocks(_fsm, fileType, _inodeNumD, diskOffset);
        }  // end if (fileType == 1)
        else if (fileType == 2) {
            // If remove file, pass inodeNum
            remove_file_triple_indirect_blocks(_fsm, fileType, _inodeNum, diskOffset);
        }  // end else if (fileType == 2)
    }  // end if (diskOffset != (unsigned int)(-1))
    // Open inode, assign to success whether opening worked
    success = fs_open_file(_fsm, _inodeNum);
    // If inode didn't open, return false
    if (success == False) {
        return False;
    }  // end if (success == False)
    // Clear the inode
    _fsm->inode.fileType = 0;
    _fsm->inode.fileSize = 0;
    _fsm->inode.permissions = 0;
    _fsm->inode.linkCount = 0;
    _fsm->inode.dataBlocks = 0;
    _fsm->inode.owner = 0;
    _fsm->inode.status = 0;
    // Clear the direct pointers
    for (i = 0; i < 10; i++) {
        _fsm->inode.directPtr[i] = (unsigned int)(-1);
    }
    // Clear single indirect pointers
    _fsm->inode.sIndirect = (unsigned int)(-1);
    // Clear double indirect pointers
    _fsm->inode.dIndirect = (unsigned int)(-1);
    // Clear triple indirect pointers
    _fsm->inode.tIndirect = (unsigned int)(-1);
    // Write over inode
    fseek(_fsm->diskHandle, 0, SEEK_SET);
    inode_write(&_fsm->inode, _inodeNum, _fsm->diskHandle);
    byte = _inodeNum / 8;
    bit = _inodeNum % 8;
    _fsm->index[0] = byte;
    _fsm->index[1] = bit;
    _fsm->contInodes = 1;
    deallocate_inode(_fsm);
    // Remove the file from its containing directory
    fs_remove_file_from_dir(_fsm, _inodeNum, _inodeNumD);
    // Succeeded so return true
    return True;
}

/**
 * @brief Removes triple indirect pointers associated with a file.
 * Clears triple indirect block references for a file, starting at the specified disk offset.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _fileType Type of the file being removed.
 * @param[in] _inodeNumD Inode number of the directory containing the file.
 * @param[in] _diskOffset Offset to the first usable data block on disk.
 * @return void
 * @date 2010-04-12 First implementation.
 */
static void remove_file_triple_indirect_blocks(FSM *_fsm, unsigned int _fileType,
                                               unsigned int _inodeNumD, unsigned int _diskOffset) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    unsigned int sectorNumber;
    unsigned int byte;
    unsigned int bit;
    diskOffset = _diskOffset;
    // Read disk to indirectBlock
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    // Deallocate Double indirect blocks
    unsigned int i;
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            remove_file_double_indirect_blocks(_fsm, _fileType, _inodeNumD, diskOffset);
        }  // if (indirectBlock[i] != (unsigned int)(-1))
    }  // for (i = 0; i < BLOCK_SIZE/4; i++)
    // Deallocate T indirect block
    diskOffset = _diskOffset;
    sectorNumber = diskOffset / BLOCK_SIZE;
    byte = sectorNumber / 8;
    bit = sectorNumber % 8;
    _fsm->ssm->contSectors = 1;
    _fsm->ssm->index[0] = byte;
    _fsm->ssm->index[1] = bit;
    // Deallocate newly freed sectors
    ssm_deallocate_sectors(_fsm->ssm);
}

/**
 * @brief Removes double indirect pointers associated with a file.
 * Clears double indirect block references for a file, starting at the specified disk offset.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _fileType Type of the file being removed.
 * @param[in] _inodeNumD Inode number of the directory containing the file.
 * @param[in] _diskOffset Offset to the first usable data block on disk.
 * @return void
 * @date 2010-04-12 First implementation.
 */
static void remove_file_double_indirect_blocks(FSM *_fsm, unsigned int _fileType,
                                               unsigned int _inodeNumD, unsigned int _diskOffset) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    unsigned int sectorNumber;
    unsigned int byte;
    unsigned int bit;
    diskOffset = _diskOffset;
    // Read disk to indirectBlock
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    // Remove all the Single indirect pointers
    unsigned int i;
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            remove_file_single_indirect_blocks(_fsm, _fileType, _inodeNumD, diskOffset);
        }  // end if (indirectBlock[i] != (unsigned int)(-1))
    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
    // Deallocate D indirect block
    diskOffset = _diskOffset;
    sectorNumber = diskOffset / BLOCK_SIZE;
    byte = sectorNumber / 8;
    bit = sectorNumber % 8;
    _fsm->ssm->contSectors = 1;
    _fsm->ssm->index[0] = byte;
    _fsm->ssm->index[1] = bit;
    // Deallocate newly freed sectors
    ssm_deallocate_sectors(_fsm->ssm);
}

/**
 * @brief Removes single indirect pointers associated with a file.
 * Clears single indirect block references for a file, starting at the specified disk offset.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _fileType Type of the file being removed.
 * @param[in] _inodeNumD Inode number of the directory containing the file.
 * @param[in] _diskOffset Offset to the first usable data block on disk.
 * @return void
 * @date 2010-04-12 First implementation.
 */
static void remove_file_single_indirect_blocks(FSM *_fsm, unsigned int _fileType,
                                               unsigned int _inodeNumD, unsigned int _diskOffset) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    unsigned int buffer[BLOCK_SIZE / 4];
    unsigned int sectorNumber;
    unsigned int byte;
    unsigned int bit;
    diskOffset = _diskOffset;
    // Read disk to indirectBlock
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    // Deallocate all Single Indirect Blocks
    unsigned int i, j;
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            // Deallocate Data Blocks
            if (_fileType == 2) {
                fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                _fsm->sampleCount =
                    fread(buffer, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                for (j = 8; j < BLOCK_SIZE / 4; j += 4) {
                    if (buffer[j + 3] == 1) {
                        fs_remove_file(_fsm, buffer[j + 2], _inodeNumD);
                    }  // end if (buffer[j+3] == 1)
                }  // end for (j = 8; j < BLOCK_SIZE/4; j += 4)
            }  // end if (_fileType == 2)
            sectorNumber = diskOffset / BLOCK_SIZE;
            byte = sectorNumber / 8;
            bit = sectorNumber % 8;
            _fsm->ssm->contSectors = 1;
            _fsm->ssm->index[0] = byte;
            _fsm->ssm->index[1] = bit;
            ssm_deallocate_sectors(_fsm->ssm);
        }  // end if (indirectBlock[i] != (unsigned int)(-1))
    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
    // Deallocate S indirect block
    diskOffset = _diskOffset;
    sectorNumber = diskOffset / BLOCK_SIZE;
    byte = sectorNumber / 8;
    bit = sectorNumber % 8;
    _fsm->ssm->contSectors = 1;
    _fsm->ssm->index[0] = byte;
    _fsm->ssm->index[1] = bit;
    // Deallocate newly freed sectors
    ssm_deallocate_sectors(_fsm->ssm);
}

Bool fs_rename_file(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                    unsigned int _inodeNumD) {
    Bool success;
    unsigned int i, j;
    unsigned int diskOffset;
    unsigned int buffer[BLOCK_SIZE / 4];
    // Attempt to open directory
    success = fs_open_file(_fsm, _inodeNumD);
    if (success == True) {
        if (_fsm->inode.fileType == 2) {
            for (i = 0; i < 10; i++) {
                if (_fsm->inode.directPtr[i] != (unsigned int)(-1)) {
                    diskOffset = _fsm->inode.directPtr[i];
                    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                    _fsm->sampleCount =
                        fread(buffer, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                    for (j = 0; j < BLOCK_SIZE / 4; j += 4) {
                        if (buffer[j + 3] == 1 && buffer[j + 2] == _inodeNumF) {
                            buffer[j] = _name[0];
                            buffer[j + 1] = _name[1];
                            fseek(_fsm->diskHandle, 0, SEEK_SET);
                            fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                            _fsm->sampleCount = fwrite(buffer, sizeof(unsigned int), BLOCK_SIZE / 4,
                                                       _fsm->diskHandle);
                            return True;
                        }  // end if (buffer[j+3] == 1 && buffer[j+2] == _inodeNumF)
                    }  // end for (j = 0; j < BLOCK_SIZE/4; j += 4)
                }  // end if (_fsm->inode.directPtr[i] != (unsigned int)(-1))
            }  // end for (i = 0; i < 10; i++)
            // Rename all the Single Indirect blocks
            if (_fsm->inode.sIndirect != (unsigned int)(-1)) {
                success =
                    rename_file_in_single_indirect(_fsm, _inodeNumF, _name, _fsm->inode.sIndirect);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.sIndirect != (unsigned int)(-1))
            // Rename all Double Indirect block references
            if (_fsm->inode.dIndirect != (unsigned int)(-1)) {
                success =
                    rename_file_in_double_indirect(_fsm, _inodeNumF, _name, _fsm->inode.dIndirect);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.dIndirect != (unsigned int)(-1))
            // Rename all Triple Indirect block references
            if (_fsm->inode.tIndirect != (unsigned int)(-1)) {
                success =
                    rename_file_in_triple_indirect(_fsm, _inodeNumF, _name, _fsm->inode.tIndirect);
                if (success == True) {
                    return True;
                }  // end if (success == True)
            }  // end if (_fsm->inode.tIndirect != (unsigned int)(-1))
        }  // end if (_fsm->inode.fileType == 2)
    }  // end if (success == True)
    // Returns false if none of the renames successful
    return False;
}

/**
 * @brief Renames a file within a triple indirect block.
 * Renames the file identified by `_inodeNumF` in the structure pointed to
 * by the triple indirect pointer at `_tIndirectOffset`.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _inodeNumF Inode number of the file to rename.
 * @param[in] _name Pointer to the new name for the file.
 * @param[in] _tIndirectOffset Offset of the triple indirect pointer.
 * @return True if the rename was successful, false otherwise.
 * @date 2010-04-12 First implementation.
 */
static Bool rename_file_in_triple_indirect(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                                           unsigned int _tIndirectOffset) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    Bool success;
    diskOffset = _tIndirectOffset;
    // Read disk to indirectBlock
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    unsigned int i;
    // Loop through indirect block, follow the Double indirect pointers
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            success = rename_file_in_double_indirect(_fsm, _inodeNumF, _name, diskOffset);
            if (success == True) {
                return True;
            }  // end if (success == True)
        }  // end if (indirectBlock[i] != (unsigned int)(-1))
    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
    // Return false if not successful in renaming
    return False;
}

/**
 * @brief Renames a file within a double indirect block.
 * Renames the file identified by `_inodeNumF` in the structure pointed to
 * by the double indirect pointer at `_dIndirectOffset`.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _inodeNumF Inode number of the file to rename.
 * @param[in] _name Pointer to the new name for the file.
 * @param[in] _dIndirectOffset Offset of the double indirect pointer.
 * @return True if the rename was successful, false otherwise.
 * @date 2010-04-12 First implementation.
 */
static Bool rename_file_in_double_indirect(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                                           unsigned int _dIndirectOffset) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    Bool success;
    diskOffset = _dIndirectOffset;
    // Read disk to indirectBlock
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    unsigned int i;
    // Loop through indirect block, follow the Single indirect pointers
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            success = rename_file_in_single_indirect(_fsm, _inodeNumF, _name, diskOffset);
            if (success == True) {
                return True;
            }  // end if (success == True)
        }  // end if (indirectBlock[i] != (unsigned int)(-1))
    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
    // Return false if renaming didn't work
    return False;
}

/**
 * @brief Renames a file within a single indirect block.
 * Renames the file identified by `_inodeNumF` in the structure pointed to
 * by the single indirect pointer at `_sIndirectOffset`.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _inodeNumF Inode number of the file to rename.
 * @param[in] _name Pointer to the new name for the file.
 * @param[in] _sIndirectOffset Offset of the single indirect pointer.
 * @return True if the rename was successful, false otherwise.
 * @date 2010-04-12 First implementation.
 */
static Bool rename_file_in_single_indirect(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                                           unsigned int _sIndirectOffset) {
    unsigned int indirectBlock[BLOCK_SIZE / 4];
    unsigned int diskOffset;
    unsigned int buffer[BLOCK_SIZE / 4];
    diskOffset = _sIndirectOffset;
    // Read disk to indirectBlock
    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
    _fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);
    unsigned int i, j;
    // Loop through indirect blocks
    for (i = 0; i < BLOCK_SIZE / 4; i++) {
        if (indirectBlock[i] != (unsigned int)(-1)) {
            diskOffset = indirectBlock[i];
            fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
            _fsm->sampleCount = fread(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);
            // Go through buffer, find name portion
            for (j = 0; j < BLOCK_SIZE / 4; j += 4) {
                if (buffer[j + 3] == 1 && buffer[j + 2] == _inodeNumF) {
                    // Write name to data
                    buffer[j] = _name[0];
                    buffer[j + 1] = _name[1];
                    fseek(_fsm->diskHandle, 0, SEEK_SET);
                    fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
                    _fsm->sampleCount =
                        fwrite(buffer, sizeof(unsigned int), BLOCK_SIZE / 4, _fsm->diskHandle);
                    return True;
                }  // end if (buffer[j+3] == 1 && buffer[j+2] == _inodeNumF)
            }  // end for (j = 0; j < BLOCK_SIZE/4; j += 4)
        }  // end if (indirectBlock[i] != (unsigned int)(-1))
    }  // end for (i = 0; i < BLOCK_SIZE/4; i++)
    // Return false if renaming not successful
    return False;
}

void fs_make(FSM *_fsm, unsigned int _DISK_SIZE, unsigned int _BLOCK_SIZE, unsigned int _INODE_SIZE,
             unsigned int _INODE_BLOCKS, unsigned int _INODE_COUNT, int _initSsmMaps) {
    init_fsm_constants(_DISK_SIZE, _BLOCK_SIZE, _INODE_SIZE, _INODE_BLOCKS, _INODE_COUNT);
    init_fsm_maps(_fsm);
    init_file_sector_mgr(_fsm, _initSsmMaps);
    int i;
    // Allocate the boot and super block sectors on disk
    ssm_get_sector(2, _fsm->ssm);
    ssm_allocate_sectors(_fsm->ssm);
    // Create INODE_COUNT Inodes
    int factorsOf_32 = INODE_BLOCKS / 32;
    // int remainder = INODE_BLOCKS % 32;
    // get 32 sectors at a time and make them inode sectors
    for (i = 0; i < factorsOf_32; i++) {
        ssm_get_sector(32, _fsm->ssm);
        _fsm->diskOffset = BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));
        // take the 32 sectors and make inodes
        inode_make(32, _fsm->diskHandle, _fsm->diskOffset);
        ssm_allocate_sectors(_fsm->ssm);
    }  // end for (i = 0; i < factorsOf_32; i++)
    // ssm_get_sector(remainder, _fsm->ssm);
    // _fsm->diskOffset = BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));
    // // take the remaining sectors and make inodes
    // inode_make(remainder, _fsm->diskHandle, _fsm->diskOffset);
    // ssm_allocate_sectors(_fsm->ssm);
    unsigned int name[2];
    // Set inode 0 for boot sector
    fs_create_file(_fsm, 0, name, (unsigned int)(-1));
    fs_open_file(_fsm, 0);
    _fsm->inode.directPtr[0] = 0;
    inode_write(&_fsm->inode, 0, _fsm->diskHandle);
    // Set inode 1 for super block
    fs_create_file(_fsm, 0, name, (unsigned int)(-1));
    fs_open_file(_fsm, 1);
    _fsm->inode.directPtr[0] = BLOCK_SIZE * (1);
    inode_write(&_fsm->inode, 1, _fsm->diskHandle);
    // make root directory with inode 2
    fs_create_file(_fsm, 1, name, (unsigned int)(-1));
}

void fs_remove(FSM *_fsm) {
    if (_fsm->diskHandle) {
        fclose(_fsm->diskHandle);
        _fsm->diskHandle = Null;
    }
}

/**
 * @brief Allocates a new inode.
 * Searches for a free inode in the inode bitmap and marks it as allocated.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @return True if inode allocation was successful, false otherwise.
 * @date 2010-04-12 First implementation.
 */
static Bool allocate_inode(FSM *_fsm) {
    if (_fsm->index[0] != (unsigned int)(-1)) {
        int n;
        int byte;
        int bit;
        bit = _fsm->index[1];
        byte = _fsm->index[0];
        n = _fsm->contInodes;
        // Find contiguous sectors
        while (n > 0) {
            for (; bit < 8; bit++) {
                _fsm->iMap[byte] -= pow(2, bit);
                n--;
                if (n == 0) {
                    break;
                }  // end if (n == 0)
            }  // end for (; bit < 8; bit++)
            byte++;
            bit = 0;
        }  // end while (n > 0)
    }  // end if (_fsm->index[0] != (unsigned int)(-1))
    _fsm->index[0] = (unsigned int)(-1);
    _fsm->index[1] = (unsigned int)(-1);
    _fsm->contInodes = 0;
    // Write the newly allocated inode to the iMapHandler
    char dbfile[256];
    snprintf(dbfile, sizeof(dbfile), "./fs/iMap");
    _fsm->iMapHandle = fopen(dbfile, "r+");
    _fsm->sampleCount = fwrite(_fsm->iMap, 1, INODE_BLOCKS, _fsm->iMapHandle);
    fclose(_fsm->iMapHandle);
    _fsm->iMapHandle = Null;
    return True;
}

/**
 * @brief Deallocates an inode.
 * Frees an inode previously marked as allocated in the inode bitmap.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @return True if inode deallocation was successful, false otherwise.
 * @date 2010-04-12 First implementation.
 */
static Bool deallocate_inode(FSM *_fsm) {
    // Deallocate iMap at Inode's location
    if (_fsm->index[0] != (unsigned int)(-1)) {
        int n;
        int byte;
        int bit;
        bit = _fsm->index[1];
        byte = _fsm->index[0];
        n = _fsm->contInodes;
        while (n > 0) {
            for (; bit < 8; bit++) {
                _fsm->iMap[byte] += pow(2, bit);
                n--;
                if (n == 0) {
                    break;
                }  // end if (n == 0)
            }  // end for (; bit < 8; bit++)
            byte++;
            bit = 0;
        }  // end while (n > 0)
    }  // end if (_fsm->index[0] != (unsigned int)(-1))
    _fsm->index[0] = (unsigned int)(-1);
    _fsm->index[1] = (unsigned int)(-1);
    _fsm->contInodes = 0;
    // Write iMap to its Handler
    char dbfile[256];
    snprintf(dbfile, sizeof(dbfile), "./fs/iMap");
    _fsm->iMapHandle = fopen(dbfile, "r+");
    _fsm->sampleCount = fwrite(_fsm->iMap, 1, INODE_BLOCKS, _fsm->iMapHandle);
    fclose(_fsm->iMapHandle);
    _fsm->iMapHandle = Null;
    return True;
}

/**
 * @brief Retrieves an inode from the inode map.
 * Searches the inode map for a free inodes.
 * @param[in] _n number of contiguous inodes to get.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @return True if an inode was successfully retrieved, false otherwise.
 * @date 2010-04-12 First implementation.
 */
static Bool get_inode(int _n, FSM *_fsm) {
    int i;
    unsigned int mask;
    unsigned int result;
    unsigned int buff;
    unsigned char *iMap;
    unsigned long long int subsetMap[1];
    unsigned int byteCount;
    Bool found;
    _fsm->contInodes = _n;
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
        iMap = _fsm->iMap;
        _fsm->index[0] = -1;
        _fsm->index[1] = -1;
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
                _fsm->index[0] = byteCount + (i / 8);
                _fsm->index[1] = i % 8;
                return True;  // break;
            }  // end if (found == True)
            iMap += 4;
            byteCount += 4;
        }  // end while (byteCount < INODE_BLOCKS)
    }  // end if (_n < 33 && _n > 0)
    return False;
}
