/*******************************************************************************
 * File Sector Manager (FSM)
 * Author: Michael Lombardi
 *******************************************************************************/
#ifndef FILE_SECTOR_MGR_H
#define FILE_SECTOR_MGR_H
#include <stdio.h>

#include "config.h"
#include "fsm_constants.h"
#include "global_constants.h"
#include "inode.h"
#include "ssm.h"

//======================== FSM TYPE DEFINITION ==============================//
typedef struct {
    // pointer to SSM
    SSM ssm[1];
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
} FSM;

//======================== FSM FUNCTION PROTOTYPES ==========================//

/**
 * @brief Creates and initializes the entire file system.
 * Allocates boot and superblocks, initializes all inode blocks, and sets up
 * inodes for the boot, super, and root directories.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _DISK_SIZE Total size of the disk in bytes.
 * @param[in] _BLOCK_SIZE Block size in bytes.
 * @param[in] _INODE_SIZE Size of a single inode in bytes.
 * @param[in] _INODE_BLOCKS Number of blocks reserved for inodes.
 * @param[in] _INODE_COUNT Total number of inodes to initialize.
 * @param[in] _initSsmMaps Flag indicating whether to initialize the SSM maps.
 * @return True if the FSM was successfully created, false otherwise.
 * @date 2010-04-12 First implementation.
 */
Bool fs_make(FSM *_fsm, unsigned int _DISK_SIZE, unsigned int _BLOCK_SIZE, unsigned int _INODE_SIZE,
             unsigned int _INODE_BLOCKS, unsigned int _INODE_COUNT, int _initSsmMaps);

/**
 * @brief Closes the file system and releases associated resources.
 * Finalizes the file system by closing the disk handle if it is open.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @return True if the FSM was successfully removed, false otherwise.
 * @date 2025-06-13 First implementation.
 */
Bool fs_remove(FSM *_fsm);

/**
 * @brief Creates a file or directory.
 * This function creates a new file or directory in the file sector manager.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @param[in] _isDirectory Non-zero if the new file is a directory, 0 otherwise.
 * @param[out] _name Pointer to where the generated name will be stored.
 * @param[in] _inodeNumD Inode number of the directory in which to create the file.
 * @return Non-zero (true) if successful, 0 (false) otherwise.
 * @date 2010-04-01 First implementation.
 */
unsigned int fs_create_file(FSM *_fsm, int _isDirectory, unsigned int *_name,
                            unsigned int _inodeNumD);

/**
 * @brief Opens a file.
 * Opens a file identified by the given inode number within the File Sector Manager.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @param[in] _inodeNum Inode number of the file to open.
 * @return The inode of the file, NULL otherwise.
 * @date 2010-04-01 First implementation.
 */
const Inode *fs_open_file(FSM *_fsm, unsigned int _inodeNum);

/**
 * @brief Closes the currently opened file.
 * Closes the file currently open in the File Sector Manager.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @return return True if the file was successfully closed, false otherwise.
 * @date 2010-04-01 First implementation.
 */
Bool fs_close_file(FSM *_fsm);

/**
 * @brief Removes a file from a directory.
 * Removes the file identified by `_inodeNumF` from the directory specified by `_inodeNumD`.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @param[in] _inodeNumF Inode number of the file to be removed.
 * @param[in] _inodeNumD Inode number of the directory containing the file.
 * @return True if the file was successfully removed, false if the file could not be accessed.
 * @date 2010-04-01 First implementation.
 */
Bool fs_remove_file_from_dir(FSM *_fsm, unsigned int _inodeNumF, unsigned int _inodeNumD);

/**
 * @brief Writes data to a file.
 * Writes the contents of the provided buffer to the file identified by the given inode number.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @param[in] _inodeNum Inode number of the file to write to.
 * @param[in] _buffer Pointer to the data to be written.
 * @param[in] _fileSize Size of the data to write, in bytes.
 * @return True if the write was successful, false otherwise.
 * @date 2010-04-01 First implementation.
 */
Bool fs_write_to_file(FSM *_fsm, unsigned int _inodeNum, void *_buffer, long long int _fileSize);

/**
 * @brief Reads a file.
 * Reads the contents of the file associated with the given inode number
 * into the provided buffer.
 * @param[in,out] _fsm Pointer to the FSM instance.
 * @param[in] _inodeNum Inode number of the file to read.
 * @param[out] _buffer Pointer to a buffer where the file contents will be stored.
 * @return True if the file was read successfully, false otherwise.
 * @date 2010-04-01 First implementation.
 */
Bool fs_read_from_file(FSM *_fsm, unsigned int _inodeNum, void *_buffer);

/**
 * @brief Removes a file from the filesystem.
 * Removes the file identified by `_inodeNum` from the directory specified by `_inodeNumD`.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _inodeNum Inode number of the file to be removed.
 * @param[in] _inodeNumD Inode number of the directory containing the file.
 * @return True if the file was removed successfully, false otherwise.
 * @date 2010-04-12 First implementation.
 */
Bool fs_remove_file(FSM *_fsm, unsigned int _inodeNum, unsigned int _inodeNumD);

/**
 * @brief Renames a file within a directory.
 * Updates the name of the file identified by `_inodeNumF` in the directory
 * specified by `_inodeNumD`.
 * @param[in,out] _fsm Pointer to the FSM structure.
 * @param[in] _inodeNumF Inode number of the file to rename.
 * @param[in] _name Pointer to the new name for the file.
 * @param[in] _inodeNumD Inode number of the directory containing the file.
 * @return True if the rename was successful, false otherwise.
 * @date 2010-04-12 First implementation.
 */
Bool fs_rename_file(FSM *_fsm, unsigned int _inodeNumF, unsigned int *_name,
                    unsigned int _inodeNumD);

#endif  // FILE_SECTOR_MGR
