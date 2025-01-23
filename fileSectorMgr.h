/*******************************************************************************
 * File Sector Manager (FSM)
 * Author: Michael Lombardi
 *******************************************************************************/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "gDefinitions.h"
#include "fsmDefinitions.h"
#include "sectorSpaceMgr.h"
#include "iNode.h"

//======================== FSM TYPE DEFINITION ==============================//

typedef struct
{

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
		  unsigned int _INODE_SIZE, unsigned int _INODE_BLOCKS,
		  unsigned int _INODE_COUNT, int _initSsmMaps);

Bool allocateInode(FileSectorMgr *_fsm);

Bool deallocateInode(FileSectorMgr *_fsm);

Bool getInode(int _n, FileSectorMgr *_fsm);

unsigned int createFile(FileSectorMgr *_fsm, int _isDirectory,
						unsigned int *_name, unsigned int _inodeNumD);

Bool openFile(FileSectorMgr *_fsm, unsigned int _inodeNum);

void closeFile(FileSectorMgr *_fsm);

Bool addFileToDir(FileSectorMgr *_fsm, unsigned int _inodeNumF,
				  unsigned int *_name, unsigned int _inodeNumD);

Bool addFileTo_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
						  unsigned int *_name, unsigned int _sIndirectOffset, Bool _allocate);

Bool addFileTo_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
						  unsigned int *_name, unsigned int _dIndirectOffset, Bool _allocate);

Bool addFileTo_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
						  unsigned int *_name, unsigned int _tIndirectOffset, Bool _allocate);

Bool rmFileFromDir(FileSectorMgr *_fsm, unsigned int _inodeNumF,
				   unsigned int _inodeNumD);

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

void writeTo_S_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset,
							  void *_buffer, unsigned int _sIndirectPtrs);

void writeTo_D_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset,
							  void *_buffer, unsigned int _dIndirectPtrs);

void writeTo_T_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset,
							  void *_buffer, unsigned int _tIndirectPtrs);

Bool readFromFile(FileSectorMgr *_fsm, unsigned int _inodeNum, void *_buffer);

void readFrom_S_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer,
							   unsigned int _diskOffset);

void readFrom_D_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer,
							   unsigned int _diskOffset);

void readFrom_T_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer,
							   unsigned int _diskOffset);

Bool rmFile(FileSectorMgr *_fsm, unsigned int _inodeNum,
			unsigned int _inodeNumD);

void rmFile_S_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType,
							 unsigned int _inodeNumD, unsigned int _diskOffset);

void rmFile_D_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType,
							 unsigned int _inodeNumD, unsigned int _diskOffset);

void rmFile_T_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType,
							 unsigned int _inodeNumD, unsigned int _diskOffset);

Bool renameFile(FileSectorMgr *_fsm, unsigned int _inodeNumF,
				unsigned int *_name, unsigned int _inodeNumD);

Bool renameFileIn_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
							 unsigned int *_name, unsigned int _sIndirectOffset);

Bool renameFileIn_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
							 unsigned int *_name, unsigned int _dIndirectOffset);

Bool renameFileIn_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
							 unsigned int *_name, unsigned int _tIndirectOffset);

void fsmPrint(FileSectorMgr *_fsm, int _case, unsigned int _startByte);

//========================= FSM FUNCTION DEFINITIONS =======================//

/************************* def beg initFileSectorMgr *************************

	void initFileSectorMgr(FileSectorMgr *_fsm, int _initSsmMaps)

	Function
		Initializes the File Sector Manager

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _initSsmMaps
			:= variable that tells program whether to initialize
			the SSM maps

		Local Variables
			int i, j
			:= loop variables

	initFileSectorMgr
		= returns void for all calls to this function


	Change Record: 4/1/10 first implementation

************************* def end initFileSectorMgr ************************/
void initFileSectorMgr(FileSectorMgr *_fsm, int _initSsmMaps)
{

	int i, j;

	// Initialize SSM Maps
	if (_initSsmMaps == 1)
	{
		initSsmMaps(_fsm->ssm);
	} // end if (_initSsmMaps == 1)

	// Initialize SEctor Space Manager
	initSecSpaceMgr(_fsm->ssm);

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
	for (i = 7; i < 17; i++)
	{
		_fsm->inode.directPtr[j] = -1;
		j++;
	} // end for (i = 7; i < 17; i++)

	_fsm->inode.sIndirect = -1;
	_fsm->inode.dIndirect = -1;
	_fsm->inode.tIndirect = -1;
	_fsm->inodeNum = (unsigned int)-1;

	_fsm->iMapHandle = Null;

	// Open file stream for iMap
	sprintf(dbfile, "./iMap");
	_fsm->iMapHandle = fopen(dbfile, "r+");

	// Read in INODE_BLOCKS number of items from iMap to iMapHandle
	_fsm->sampleCount = fread(_fsm->iMap, 1, INODE_BLOCKS, _fsm->iMapHandle);

	// Closes the iMapHandle
	fclose(_fsm->iMapHandle);

	// Open file stream for the disk
	_fsm->diskHandle = Null;
	sprintf(dbfile, "./hardDisk");

	// Open binary form of file for reading and writing
	_fsm->diskHandle = fopen(dbfile, "rb+");

} // end void initFileSectorMgr(FileSectorMgr *_fsm, int _initSsmMaps)

/**************************** def beg initFsmMaps *************************

	void initFsmMaps(FileSectorMgr *_fsm)

	Function
		Initializes the File Sector Manager Maps

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

		Local Variables
			int i
			:= loop variables

			char dbfile
			:= character array to hold data

			char map[]
			:= array to store FSM maps

			char disk[]
			:= map to hold free and allocated disk sectors
	initFsmMaps
		= returns void for all calls to this function

	Change Record: 4/1/10 first implementation

***************************** def end initFsmMaps **********************/
void initFsmMaps(FileSectorMgr *_fsm)
{

	unsigned int i;

	char dbfile[256];

	unsigned char map[INODE_BLOCKS]; // SECTOR_BYTES

	unsigned char disk[DISK_SIZE];

	_fsm->iMapHandle = Null;

	// Load iMap from file and place in iMapHandle
	sprintf(dbfile, "./iMap");
	_fsm->iMapHandle = fopen(dbfile, "r+");

	// Initialize all map elements to 255
	for (i = 0; i < INODE_BLOCKS; i++)
	{
		map[i] = 255;
	} // end for (i = 0; i < INODE_BLOCKS; i++)

	// Write map back to iMapHandle
	_fsm->sampleCount = fwrite(map, 1, INODE_BLOCKS, _fsm->iMapHandle);

	// close the file
	fclose(_fsm->iMapHandle);

	_fsm->diskHandle = Null;

	sprintf(dbfile, "./hardDisk");
	_fsm->diskHandle = fopen(dbfile, "rb+");

	// Initialize all disk elements to 0
	for (i = 0; i < DISK_SIZE; i++)
	{
		disk[i] = 0;
	} // end for (i = 0; i < DISK_SIZE; i++)

	// Write contents of disk to diskHandle
	_fsm->sampleCount = fwrite(disk, 1, DISK_SIZE, _fsm->diskHandle);

	fclose(_fsm->diskHandle);

} // end void initFsmMaps(FileSectorMgr *_fsm)

/**************************** def beg createFile *************************

	Bool createFile(FileSectorMgr *_fsm, int _isDirectory)

	Function
		Creates a file or directory

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _isDirectory
			:= variable that determines if the file is a directory

		Local Variables
			int i, j
			:= loop variables

	createFile
		= Returns true if successful, false otherwise


	Change Record: 4/1/10 first implementation

***************************** def end createFile ************************/
unsigned int createFile(FileSectorMgr *_fsm, int _isDirectory,
						unsigned int *_name, unsigned int _inodeNumD)
{

	getInode(1, _fsm);

	if (_fsm->index[0] == (unsigned int)(-1))
	{
		return (unsigned int)(-1);
	} // end if (_fsm->index[0] == (unsigned int)(-1))

	// Create a default file. Will start with all pointers as -1
	unsigned int inodeNum;

	inodeNum = 8 * _fsm->index[0] + _fsm->index[1];
	readInode(&_fsm->inode, inodeNum, _fsm->diskHandle);

	// initialize inode metadata
	_fsm->inode.fileSize = 0;
	_fsm->inode.permissions = 0;
	_fsm->inode.linkCount = 0;
	_fsm->inode.dataBlocks = 0;
	_fsm->inode.owner = 0;
	_fsm->inode.status = 0;

	int i;

	for (i = 0; i < 10; i++)
	{

		_fsm->inode.directPtr[i] = (unsigned int)(-1);

	} // end for (i = 0; i < 10; i++)

	_fsm->inode.sIndirect = (unsigned int)(-1);
	_fsm->inode.dIndirect = (unsigned int)(-1);
	_fsm->inode.tIndirect = (unsigned int)(-1);

	// Assign filetype
	if (_isDirectory == 1)
	{
		_fsm->inode.fileType = 2;
	}
	else
	{
		_fsm->inode.fileType = 1;
	} // end else (_isDirectory == 1)

	writeInode(&_fsm->inode, inodeNum, _fsm->diskHandle);

	allocateInode(_fsm);

	// Bool success;

	unsigned int name[2];

	if (_isDirectory == 1)
	{

		// set . directory
		strcpy((char *)name, ".");
		/*success =*/addFileToDir(_fsm, inodeNum, name, inodeNum);

		// set .. directory
		strcpy((char *)name, "..");
		/*success =*/addFileToDir(_fsm, _inodeNumD, name, inodeNum);

	} // end if (_isDirectory == 1)

	/*success =*/addFileToDir(_fsm, inodeNum, _name, _inodeNumD);

	return inodeNum;

} // end Bool createFile(FileSectorMgr *_fsm, int _isDirectory)

/*********************** def beg openFile ******************************

	Bool openFile(FileSectorMgr *_fsm, unsigned int _inodeNum)

	Function
		Opens a file

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _inodeNum
			:= inode to open file from

		Local Variables
			int i, j
			:= loop variables

	openFile
		= returns True if file is opened, false otherwise


	Change Record: 4/1/10 first implementation

************************* def end openFile ****************************/
Bool openFile(FileSectorMgr *_fsm, unsigned int _inodeNum)
{

	int i, j;

	if (_inodeNum == (unsigned int)(-1))
	{
		return False;
	} // end if (_inodeNum == (unsigned int)(-1)) {

	readInode(&_fsm->inode, _inodeNum, _fsm->diskHandle);
	_fsm->inodeNum = _inodeNum;

	// Return true if file loaded correctly
	if (_fsm->inode.fileType > 0)
	{
		return True;
	} // end if (_fsm->inode.fileType > 0)
	else
	{

		// If file not loaded, create a default inode and return False
		_fsm->inode.fileType = 0;
		_fsm->inode.fileSize = 0;
		_fsm->inode.permissions = 0;
		_fsm->inode.linkCount = 0;
		_fsm->inode.dataBlocks = 0;
		_fsm->inode.owner = 0;
		_fsm->inode.status = 0;

		j = 0;

		for (i = 7; i < 17; i++)
		{
			_fsm->inode.directPtr[j] = -1;
			j++;
		} // end for (i = 7; i < 17; i++)

		_fsm->inode.sIndirect = -1;
		_fsm->inode.dIndirect = -1;
		_fsm->inode.tIndirect = -1;

		_fsm->inodeNum = (unsigned int)(-1);

		// Return False if file did not open correctly
		return False;

	} // end else (_fsm->inode.fileType > 0)

} // end Bool openFile(FileSectorMgr *_fsm, unsigned int _inodeNum)

/************************ def beg closeFile *****************************

	void closeFile(FileSectorMgr *_fsm)

	Function
		Closes the currently opened file

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

		Local Variables
			int i, j
			:= loop variables

	closeFile
		= returns void for all calls to this function


	Change Record: 4/1/10 first implementation

************************* def end closeFile ****************************/
void closeFile(FileSectorMgr *_fsm)
{

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

	for (i = 7; i < 17; i++)
	{
		_fsm->inode.directPtr[j] = (unsigned int)(-1);
		j++;
	} // end for (i = 7; i < 17; i++)

	_fsm->inode.sIndirect = (unsigned int)(-1);
	_fsm->inode.dIndirect = (unsigned int)(-1);
	_fsm->inode.tIndirect = (unsigned int)(-1);

} // end void closeFile(FileSectorMgr *_fsm)

/**************************** def beg readFromFile *************************

	Bool readFromFile(FileSectorMgr *_fsm, unsigned int _inodeNum,
					 void *_buffer)

	Function
		Reads a file

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _inodeNum
			:= the inode to read from

			void *_buffer
			:= a buffer to place read values

		Local Variables
			Bool success
			:= Holds whether the read was successful or failed

	readFromFile
		= Returns true if file read correctly, false otherwise


	Change Record: 4/1/10 first implementation

***************************** def end readFromFile ************************/
Bool readFromFile(FileSectorMgr *_fsm, unsigned int _inodeNum, void *_buffer)
{

	// Open file at Inode _inodeNum for reading
	Bool success;

	success = openFile(_fsm, _inodeNum);

	if (success == False)
	{
		return False;
	} // end if (success == False)

	void *buffer;

	buffer = _buffer;

	// Read data from direct pointers into buffer _buffer
	int i;

	unsigned int diskOffset;

	for (i = 0; i < 10; i++)
	{

		diskOffset = _fsm->inode.directPtr[i];

		if (diskOffset != (unsigned int)(-1))
		{
			fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
			_fsm->sampleCount = fread(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);
			buffer += BLOCK_SIZE;
		} // end if (diskOffset != (unsigned int)(-1))

	} // end for (i = 0; i < 10; i++)

	// Read data from single indirect pointer into buffer _buffer
	diskOffset = _fsm->inode.sIndirect;

	if (diskOffset != (unsigned int)(-1))
	{
		readFrom_S_IndirectBlocks(_fsm, buffer, diskOffset);
		buffer += BLOCK_SIZE * S_INDIRECT_BLOCKS;
	} // end if (diskOffset != (unsigned int)(-1))

	diskOffset = _fsm->inode.dIndirect;

	if (diskOffset != (unsigned int)(-1))
	{
		readFrom_D_IndirectBlocks(_fsm, buffer, diskOffset);
		buffer += BLOCK_SIZE * D_INDIRECT_BLOCKS;
	} // end if (diskOffset != (unsigned int)(-1))

	diskOffset = _fsm->inode.tIndirect;

	if (diskOffset != (unsigned int)(-1))
	{
		readFrom_T_IndirectBlocks(_fsm, buffer, diskOffset);
	} // end if (diskOffset != (unsigned int)(-1))

	fseek(_fsm->diskHandle, 0, SEEK_SET);

	return True;

} // end readFromFile(FileSectorMgr *_fsm, unsigned int _inodeNum, void *_buffer)

/**************************** def beg writeToFile *************************

	Bool writeToFile(FileSectorMgr *_fsm, unsigned int _inodeNum,
					 void *_buffer, long long int _fileSize)

	Function
		Writes

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _initSsmMaps
			:= variable that tells program whether to initialize
			the SSM maps

		Local Variables
			int i, j
			:= loop variables

	writeToFile
		= returns void for all calls to this function


	Change Record: 4/1/10 first implementation

***************************** def end writeToFile ************************/
Bool writeToFile(FileSectorMgr *_fsm, unsigned int _inodeNum,
				 void *_buffer, long long int _fileSize)
{

	Bool success;

	// return false if openFile fails
	success = openFile(_fsm, _inodeNum);

	if (success == False)
	{
		return False;
	} // end if (success == False)

	// set fileSize and inode fileSize
	long long int fileSize = _fileSize;

	_fsm->inode.fileSize = (unsigned int)_fileSize;
	_fsm->inode.dataBlocks = _fsm->inode.fileSize / BLOCK_SIZE;

	unsigned int directPtrs = 0;

	unsigned int i;

	// Calculate how many direct pointers needed
	for (i = 0; i < 10; i++)
	{

		fileSize -= BLOCK_SIZE;

		if (fileSize > 0)
		{
			directPtrs++;
		} // end if (fileSize > 0)
		else
		{
			directPtrs++;
			break;
		} // end else (fileSize > 0)

	} // end for (i = 0; i < 10; i++)

	void *buffer;
	buffer = _buffer;

	// Write to direct pointers
	unsigned int diskOffset;

	for (i = 0; i < directPtrs; i++)
	{

		diskOffset = _fsm->inode.directPtr[i];

		if (diskOffset == (unsigned int)(-1))
		{

			// If direct pointer is empty, get a Sector for it
			getSector(1, _fsm->ssm);

			// If pointer invalid, break out of if
			if (_fsm->ssm->index[0] == (unsigned int)(-1))
			{
				break;
			} // end if (_fsm->ssm->index[0] == (unsigned int)(-1))
			else
			{

				diskOffset = BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));
				_fsm->inode.directPtr[i] = diskOffset;

				fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
				_fsm->sampleCount = fwrite(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);

				allocateSectors(_fsm->ssm);
				buffer += BLOCK_SIZE;

			} // end else (_fsm->ssm->index[0] == (unsigned int)(-1))

		} // end if (diskOffset == (unsigned int)(-1))
		else
		{

			// Search for diskHandle from beginning of disk
			fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

			// Overwrite the data at that location
			_fsm->sampleCount = fwrite(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);
			buffer += BLOCK_SIZE;

		} // end else (diskOffset == (unsigned int)(-1))

	} // end for (i = 0; i < directPtrs; i++)

	unsigned int sIndirectPtrs = 0;
	unsigned int dIndirectPtrs = 0;
	unsigned int tIndirectPtrs = 0;
	unsigned int baseOffset;

	// If file is too big for the direct pointers,
	// begin looking at the indrect pointers
	if (fileSize > 0)
	{

		// Write to file using triple Indirection if file too big for
		/// double indirection
		if (fileSize + (10 * BLOCK_SIZE) - D_INDIRECT_SIZE > 0)
		{

			sIndirectPtrs = S_INDIRECT_BLOCKS;
			dIndirectPtrs = D_INDIRECT_BLOCKS;

			// Allocate Single and double indirect pointers
			_fsm->inode.sIndirect = aloc_S_Indirect(_fsm, sIndirectPtrs);
			_fsm->inode.dIndirect = aloc_D_Indirect(_fsm, dIndirectPtrs);

			// Write to single indirect blocks
			baseOffset = _fsm->inode.sIndirect;
			writeTo_S_IndirectBlocks(_fsm, baseOffset, buffer, sIndirectPtrs);
			buffer += BLOCK_SIZE * S_INDIRECT_BLOCKS;

			// Write to double indirect blocks
			baseOffset = _fsm->inode.dIndirect;
			writeTo_D_IndirectBlocks(_fsm, baseOffset, buffer, dIndirectPtrs);
			buffer += BLOCK_SIZE * D_INDIRECT_BLOCKS;

			// Calculate remaining filesize
			fileSize = fileSize + (10 * BLOCK_SIZE) - D_INDIRECT_SIZE;

			// Calculate number of triple Indirect pointers needed
			tIndirectPtrs = fileSize / BLOCK_SIZE;
			if (fileSize % BLOCK_SIZE > 0)
			{
				tIndirectPtrs += 1;
			} // end if (fileSize % BLOCK_SIZE > 0)

			// Allocate triple Indirect Pointers
			_fsm->inode.tIndirect = aloc_T_Indirect(_fsm, tIndirectPtrs);
			baseOffset = _fsm->inode.tIndirect;

			// Write to tripleIndirectBlocks
			writeTo_T_IndirectBlocks(_fsm, baseOffset, buffer, tIndirectPtrs);

		} // end if (fileSize + (10 * BLOCK_SIZE) - D_INDIRECT_SIZE > 0)

		// Write to file using double Indirection if too big for
		//   single indirection
		else if (fileSize + (10 * BLOCK_SIZE) - S_INDIRECT_SIZE > 0)
		{

			// Allocate single Indirect blocks
			sIndirectPtrs = S_INDIRECT_BLOCKS;
			_fsm->inode.sIndirect = aloc_S_Indirect(_fsm, sIndirectPtrs);

			// Write to single Indirect pointers
			baseOffset = _fsm->inode.sIndirect;
			writeTo_S_IndirectBlocks(_fsm, baseOffset, buffer, sIndirectPtrs);
			buffer += BLOCK_SIZE * S_INDIRECT_BLOCKS;

			// Calculate Remaining filesize
			fileSize = fileSize + (10 * BLOCK_SIZE) - S_INDIRECT_SIZE;

			// Calculate number of double Indirect Pointrs needed
			dIndirectPtrs = fileSize / BLOCK_SIZE;
			if (fileSize % BLOCK_SIZE > 0)
			{
				dIndirectPtrs += 1;
			} // end if (fileSize % BLOCK_SIZE > 0)

			// Allocate double Indirect Pointers
			_fsm->inode.dIndirect = aloc_D_Indirect(_fsm, dIndirectPtrs);
			baseOffset = _fsm->inode.dIndirect;

			// Write to double Indirect Pointers
			writeTo_D_IndirectBlocks(_fsm, baseOffset, buffer, dIndirectPtrs);

		} // end else if (fileSize + (10 * BLOCK_SIZE) - S_INDIRECT_SIZE > 0)

		// Write to file using single Indirection only
		else
		{

			// Calculate number of single indirect pointers needed
			sIndirectPtrs = fileSize / BLOCK_SIZE;
			if (fileSize % BLOCK_SIZE > 0)
			{
				sIndirectPtrs += 1;
			} // end if (fileSize % BLOCK_SIZE > 0)

			// Allocate single indirect pointers
			_fsm->inode.sIndirect = aloc_S_Indirect(_fsm, sIndirectPtrs);

			// Write to single indirect pointers
			baseOffset = _fsm->inode.sIndirect;
			writeTo_S_IndirectBlocks(_fsm, baseOffset, buffer, sIndirectPtrs);

		} // end else

	} // end if (fileSize > 0)

	// Write created inode to disk
	writeInode(&_fsm->inode, _inodeNum, _fsm->diskHandle);
	fseek(_fsm->diskHandle, 0, SEEK_SET);

	return True;
} /* end writeToFile(FileSectorMgr *_fsm, unsigned int _inodeNum,
						void *_buffer, long long int _fileSize)*/

/**************************** def beg addFileToDir *************************

	Bool addFileToDir(FileSectorMgr *_fsm, unsigned int _inodeNumF,
					  unsigned int *_name, unsigned int _inodeNumD)

	Function
		Adds a file to directory

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _inodeNumF
			:= Inode number of file

			int *_name
			:= Pointer to file's name

			int _inodeNumD
			:= Inode number of directory

		Local Variables
			int i, j
			:= loop variables

			bool success
			:= Holds whether execution was successful

			int diskOffset
			:= The offset for blocks

			int buffer
			:= Stores the file read from disk

			int buffer2
			:= Stores the directory from disk

	addFileToDir
		= Returns true if file added successfully


	Change Record: 4/1/10 first implementation

***************************** def end addFileToDir ************************/
Bool addFileToDir(FileSectorMgr *_fsm, unsigned int _inodeNumF,
				  unsigned int *_name, unsigned int _inodeNumD)
{

	Bool success;

	unsigned int i, j;

	unsigned int diskOffset;

	unsigned int buffer[BLOCK_SIZE / 4];

	unsigned int buffer2[BLOCK_SIZE / 4];

	success = openFile(_fsm, _inodeNumD);

	if (success == True)
	{

		if (_fsm->inode.fileType == 2)
		{

			// Read file's direct pointers from disk
			for (i = 0; i < 10; i++)
			{

				if (_fsm->inode.directPtr[i] != (unsigned int)(-1))
				{

					diskOffset = _fsm->inode.directPtr[i];

					fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
					_fsm->sampleCount = fread(buffer, sizeof(unsigned int),
											  BLOCK_SIZE / 4, _fsm->diskHandle);

					for (j = 0; j < BLOCK_SIZE / 4; j += 4)
					{

						if (buffer[j + 3] == 0)
						{

							buffer[j + 3] = 1;
							buffer[j] = _name[0];
							buffer[j + 1] = _name[1];
							buffer[j + 2] = _inodeNumF;

							_fsm->inode.linkCount += 1;

							fseek(_fsm->diskHandle, 0, SEEK_SET);
							fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
							_fsm->sampleCount = fwrite(buffer, sizeof(unsigned int),
													   BLOCK_SIZE / 4, _fsm->diskHandle);

							fseek(_fsm->diskHandle, 0, SEEK_SET);

							closeFile(_fsm);

							return True;

						} // end if (buffer[j+3] == 0)

					} // end or (j = 0; j < BLOCK_SIZE/4; j += 4)

				} // end if (_fsm->inode.directPtr[i] != (unsigned int)(-1))

			} // end for (i = 0; i < 10; i++)

			if (_fsm->inode.sIndirect != (unsigned int)(-1))
			{
				success = addFileTo_S_Indirect(_fsm, _inodeNumF, _name,
											   _fsm->inode.sIndirect, False);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.sIndirect != (unsigned int)(-1))

			if (_fsm->inode.dIndirect != (unsigned int)(-1))
			{
				success = addFileTo_D_Indirect(_fsm, _inodeNumF, _name,
											   _fsm->inode.dIndirect, False);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end _fsm->inode.dIndirect != (unsigned int)(-1))

			if (_fsm->inode.tIndirect != (unsigned int)(-1))
			{
				success = addFileTo_T_Indirect(_fsm, _inodeNumF, _name,
											   _fsm->inode.tIndirect, False);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.tIndirect != (unsigned int)(-1))

			for (i = 0; i < 10; i++)
			{

				if (_fsm->inode.directPtr[i] == (unsigned int)(-1))
				{

					// Get sectors for direct pointers
					getSector(1, _fsm->ssm);

					if (_fsm->ssm->index[0] == (unsigned int)(-1))
					{
						// If sectors can't be retrieved, return false
						return False;
					} // end if (_fsm->ssm->index[0] == (unsigned int)(-1))
					else
					{

						diskOffset = BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));
						_fsm->inode.directPtr[i] = diskOffset;

						_fsm->sampleCount = fseek(_fsm->diskHandle,
												  diskOffset, SEEK_SET);

						// Clear buffer2
						for (j = 0; j < BLOCK_SIZE / 4; j++)
						{
							buffer2[j] = 0;
						} // end for (j = 0; j < BLOCK_SIZE/4; j++)

						buffer2[3] = 1;
						buffer2[0] = _name[0];
						buffer2[1] = _name[1];
						buffer2[2] = _inodeNumF;

						_fsm->inode.linkCount += 1;
						_fsm->inode.fileSize += BLOCK_SIZE;
						_fsm->inode.dataBlocks = _fsm->inode.fileSize / BLOCK_SIZE;

						_fsm->sampleCount = fwrite(buffer2, sizeof(unsigned int),
												   BLOCK_SIZE / 4, _fsm->diskHandle);

						// Write file to disk
						fseek(_fsm->diskHandle, 0, SEEK_SET);

						writeInode(&_fsm->inode, _inodeNumD, _fsm->diskHandle);

						allocateSectors(_fsm->ssm);

						closeFile(_fsm);

						return True;
					} // end else (_fsm->ssm->index[0] == (unsigned int)(-1))

				} // end if (_fsm->inode.directPtr[i] == (unsigned int)(-1))

			} // end for (i = 0; i < 10; i++)

			// Add file to existing Single Indirect
			if (_fsm->inode.sIndirect != (unsigned int)(-1))
			{

				// Add file to single indirect without allocation
				success = addFileTo_S_Indirect(_fsm, _inodeNumF, _name,
											   _fsm->inode.sIndirect, False);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.sIndirect != (unsigned int)(-1))

			// Add file to existing Double Indirect
			if (_fsm->inode.dIndirect != (unsigned int)(-1))
			{

				// Add file to double indirection without allocation
				success = addFileTo_D_Indirect(_fsm, _inodeNumF, _name,
											   _fsm->inode.dIndirect, False);
				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.dIndirect != (unsigned int)(-1))

			// Add file to existing Triple Indirect
			if (_fsm->inode.tIndirect != (unsigned int)(-1))
			{

				// Add file to triple indirection without allocation
				success = addFileTo_T_Indirect(_fsm, _inodeNumF, _name,
											   _fsm->inode.tIndirect, False);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.tIndirect != (unsigned int)(-1))

			// If can not find an open Single, Double or Triple Indirect pointer,
			// allocate a new one at lowest possible level

			// Add file to new Single Indirect
			if (_fsm->inode.sIndirect != (unsigned int)(-1))
			{

				// Add file to single indirection with allocation
				success = addFileTo_S_Indirect(_fsm, _inodeNumF, _name,
											   _fsm->inode.sIndirect, True);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.sIndirect != (unsigned int)(-1))

			// Add file to new Double Indirect
			if (_fsm->inode.dIndirect != (unsigned int)(-1))
			{

				// Add file to double indirection with allocation
				success = addFileTo_D_Indirect(_fsm, _inodeNumF, _name,
											   _fsm->inode.dIndirect, True);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.dIndirect != (unsigned int)(-1))

			// Add file to new Triple Indirect
			if (_fsm->inode.tIndirect != (unsigned int)(-1))
			{

				// Add file to triple indirection with allocation
				success = addFileTo_T_Indirect(_fsm, _inodeNumF, _name,
											   _fsm->inode.tIndirect, True);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.tIndirect != (unsigned int)(-1))

			if (_fsm->inode.sIndirect == (unsigned int)(-1))
			{

				getSector(1, _fsm->ssm);

				if (_fsm->ssm->index[0] == (unsigned int)(-1))
				{
					return False;
				} // end if (_fsm->ssm->index[0] == (unsigned int)(-1))

				else
				{

					_fsm->inode.sIndirect = BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));

					for (i = 0; i < BLOCK_SIZE / 4; i++)
					{
						buffer2[i] = (unsigned int)(-1);
					} // end for (i = 0; i < BLOCK_SIZE/4; i++)

					diskOffset = _fsm->inode.sIndirect;

					fseek(_fsm->diskHandle, 0, SEEK_SET);

					fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

					_fsm->sampleCount = fwrite(buffer2, sizeof(unsigned int),
											   BLOCK_SIZE / 4, _fsm->diskHandle);

					allocateSectors(_fsm->ssm);

					fseek(_fsm->diskHandle, 0, SEEK_SET);

					writeInode(&_fsm->inode, _inodeNumD, _fsm->diskHandle);
				}
			} // end else (_fsm->ssm->index[0] == (unsigned int)(-1))

			success = addFileTo_S_Indirect(_fsm,
										   _inodeNumF, _name, diskOffset, True);

			if (success == True)
			{
				return True;
			} // end if (success == True)

		} // end if (_fsm->inode.sIndirect == (unsigned int)(-1))

		if (_fsm->inode.dIndirect == (unsigned int)(-1))
		{

			getSector(1, _fsm->ssm);

			if (_fsm->ssm->index[0] == (unsigned int)(-1))
			{
				return False;
			} // end if (_fsm->ssm->index[0] == (unsigned int)(-1))
			else
			{

				_fsm->inode.dIndirect = BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));

				for (i = 0; i < BLOCK_SIZE / 4; i++)
				{
					buffer2[i] = (unsigned int)(-1);
				} // end for (i = 0; i < BLOCK_SIZE/4; i++)

				diskOffset = _fsm->inode.dIndirect;

				fseek(_fsm->diskHandle, 0, SEEK_SET);

				fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

				_fsm->sampleCount = fwrite(buffer2, sizeof(unsigned int),
										   BLOCK_SIZE / 4, _fsm->diskHandle);

				allocateSectors(_fsm->ssm);

				fseek(_fsm->diskHandle, 0, SEEK_SET);

				writeInode(&_fsm->inode, _inodeNumD, _fsm->diskHandle);

			} // end else (_fsm->ssm->index[0] == (unsigned int)(-1))

			success = addFileTo_D_Indirect(_fsm, _inodeNumF, _name,
										   _fsm->inode.dIndirect, True);

			if (success == True)
			{
				return True;
			} // end if (success == True)

		} // end if (_fsm->inode.dIndirect == (unsigned int)(-1))

		if (_fsm->inode.tIndirect == (unsigned int)(-1))
		{

			getSector(1, _fsm->ssm);

			if (_fsm->ssm->index[0] == (unsigned int)(-1))
			{
				return False;
			} // end if (_fsm->ssm->index[0] == (unsigned int)(-1))
			else
			{
				_fsm->inode.tIndirect = BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));

				for (i = 0; i < BLOCK_SIZE / 4; i++)
				{
					buffer2[i] = (unsigned int)(-1);
				} // end for (i = 0; i < BLOCK_SIZE/4; i++)

				diskOffset = _fsm->inode.tIndirect;

				fseek(_fsm->diskHandle, 0, SEEK_SET);

				fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
				_fsm->sampleCount = fwrite(buffer2, sizeof(unsigned int),
										   BLOCK_SIZE / 4, _fsm->diskHandle);

				allocateSectors(_fsm->ssm);

				fseek(_fsm->diskHandle, 0, SEEK_SET);

				writeInode(&_fsm->inode, _inodeNumD, _fsm->diskHandle);

			} // end else (_fsm->ssm->index[0] == (unsigned int)(-1))

			success = addFileTo_T_Indirect(_fsm, _inodeNumF, _name,
										   _fsm->inode.tIndirect, True);

			if (success == True)
			{
				return True;
			} // end if (success == True)

		} // end if (_fsm->inode.tIndirect == (unsigned int)(-1))
		else
		{

			// If filetype isn't "file" return false
			return False;

		} // end else

	} // end if (success == True)
	else
	{

		// If file didn't open, return false
		return False;

	} // end else (success == True)

	// Fail case
	return False;

} /*end addFileToDir(FileSectorMgr *_fsm, unsigned int _inodeNumF,
				  unsigned int *_name, unsigned int _inodeNumD)*/

/********************** def beg addFileTo_T_Indirect *************************

	Bool addFileTo_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,

		 unsigned int *_name, unsigned int _tIndirectOffset, Bool _allocate)

	Function
		Adds a file to a triple indirect pointer

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _inodeNumF
			:= inode number of file

			int *_name
			:= Pointer to file's name

			int _tIndirectOffset
			:= Offset to triple indirect

			Bool _allocate
			:= Tells if blocks need to be allocated

		Local Variables
			int indirectBlock
			:= Stores information read from disk

			int diskOffset
			:= holds the triple indirect disk offset

			Bool success
			:= Holds whether operations were successful

	addFileTo_T_Indirect
		= Returns true if file is added, false otherwise


	Change Record: 4/1/10 first implementation

************************** def end addFileTo_T_Indirect *******************/
Bool addFileTo_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
						  unsigned int *_name, unsigned int _tIndirectOffset, Bool _allocate)
{

	unsigned int indirectBlock[BLOCK_SIZE / 4];

	unsigned int buffer[BLOCK_SIZE / 4];

	unsigned int diskOffset;

	Bool success;

	diskOffset = _tIndirectOffset;

	fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

	_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);

	unsigned int i;

	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];

			success = addFileTo_D_Indirect(_fsm, _inodeNumF, _name,
										   diskOffset, _allocate);

			if (success == True)
			{
				return True;
			} // end if (success == True)

		} // end if (indirectBlock[i] != (unsigned int)(-1))

	} // end for (i = 0; i < BLOCK_SIZE/4; i++)

	// Allocate more sectors to write to
	if (_allocate == True)
	{

		for (i = 0; i < BLOCK_SIZE / 4; i++)
		{

			if (indirectBlock[i] == (unsigned int)(-1))
			{

				getSector(1, _fsm->ssm);

				if (_fsm->ssm->index[0] == (unsigned int)(-1))
				{
					return False;
				} // end if (_fsm->ssm->index[0] == (unsigned int)(-1))
				else
				{

					indirectBlock[i] = BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));

					fseek(_fsm->diskHandle, _tIndirectOffset, SEEK_SET);

					_fsm->sampleCount = fwrite(indirectBlock, BLOCK_SIZE,
											   1, _fsm->diskHandle);

					for (i = 0; i < BLOCK_SIZE / 4; i++)
					{
						buffer[i] = (unsigned int)(-1);
					} // end for (i = 0; i < BLOCK_SIZE/4; i++)

					fseek(_fsm->diskHandle, indirectBlock[i], SEEK_SET);

					_fsm->sampleCount = fwrite(buffer, sizeof(unsigned int),
											   BLOCK_SIZE / 4, _fsm->diskHandle);

					allocateSectors(_fsm->ssm);

				} // end else (_fsm->ssm->index[0] == (unsigned int)(-1))

				diskOffset = indirectBlock[i];

				success = addFileTo_D_Indirect(_fsm, _inodeNumF, _name,
											   diskOffset, _allocate);
				if (success == True)
				{
					return True;
				} // end if(success == True)

			} // end else

		} // for (i = 0; i < BLOCK_SIZE/4; i++)

	} // end if (_allocate == True)

	return False;

} /*end addFileTo_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF
		unsigned int *_name, unsigned int _tIndirectOffset, Bool _allocate)*/

/*********************** def beg addFileTo_D_Indirect *************************

	Bool addFileTo_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,

		 unsigned int *_name, unsigned int _dIndirectOffset, Bool _allocate)

	Function
		Adds a file to a double indirect pointer

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _inodeNumF
			:= Inode number of the file

			int *_name
			:= Pointer to file's name

			int _dIndirectOffset
			:= Offset for double indirect pointer

			Bool _allocate
			:= Tells whether more space should be allocated

		Local Variables
			int indirectBlock
			:=

			int diskOffset
			:= Offset of the disk

			Bool success
			:= Holds whether operations are successful

	addFileTo_D_Indirect
		= Returns true if file successfully added, false otherwise


	Change Record: 4/1/10 first implementation

************************ def end addFileTo_D_Indirect ************************/
Bool addFileTo_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
						  unsigned int *_name, unsigned int _dIndirectOffset, Bool _allocate)
{

	unsigned int indirectBlock[BLOCK_SIZE / 4];

	unsigned int buffer[BLOCK_SIZE / 4];

	unsigned int diskOffset;

	Bool success;

	diskOffset = _dIndirectOffset;

	fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

	_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);

	unsigned int i;

	// Add file to a double indirect pointer
	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];

			success = addFileTo_S_Indirect(_fsm, _inodeNumF, _name,
										   diskOffset, _allocate);

			if (success == True)
			{
				return True;
			} // end if (success == True)

		} // if (indirectBlock[i] != (unsigned int)(-1))

	} // for (i = 0; i < BLOCK_SIZE/4; i++)

	if (_allocate == True)
	{

		for (i = 0; i < BLOCK_SIZE / 4; i++)
		{

			if (indirectBlock[i] == (unsigned int)(-1))
			{

				getSector(1, _fsm->ssm);

				if (_fsm->ssm->index[0] == (unsigned int)(-1))
				{
					return False;
				} // end if (_fsm->ssm->index[0] == (unsigned int)(-1))
				else
				{

					indirectBlock[i] = BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));
					fseek(_fsm->diskHandle, _dIndirectOffset, SEEK_SET);

					_fsm->sampleCount = fwrite(indirectBlock, BLOCK_SIZE, 1,
											   _fsm->diskHandle);

					for (i = 0; i < BLOCK_SIZE / 4; i++)
					{
						buffer[i] = (unsigned int)(-1);
					} // end for (i = 0; i < BLOCK_SIZE/4; i++)

					fseek(_fsm->diskHandle, indirectBlock[i], SEEK_SET);

					_fsm->sampleCount = fwrite(buffer, sizeof(unsigned int),
											   BLOCK_SIZE / 4, _fsm->diskHandle);

					allocateSectors(_fsm->ssm);

				} // end else (_fsm->ssm->index[0] == (unsigned int)(-1))

				diskOffset = indirectBlock[i];

				success = addFileTo_S_Indirect(_fsm, _inodeNumF,
											   _name, diskOffset, _allocate);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (indirectBlock[i] == (unsigned int)(-1))

		} // end for (i = 0; i < BLOCK_SIZE/4; i++)

	} // end if (_allocate == True)

	return False;

} /*end addFileTo_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
		unsigned int *_name, unsigned int _dIndirectOffset, Bool _allocate)*/

/************************ def beg addFileTo_S_Indirect ************************

	Bool addFileTo_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,

		 unsigned int *_name, unsigned int _sIndirectOffset, Bool _allocate)

	Function
		Adds a file to a single indirect pointer

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _inodeNumF
			:= Inode number of the file

			int *_name
			:= Pointer to file's name

			int _sIndirectOffset
			:= Single indirect offset

			Bool _allocate
			:= Holds whether to allocate

		Local Variables
			int indirectBlock
			:= Holds the indirect block

			int diskOffset
			:= Offset of disk

			int buffer
			:= buffer to hold file

	addFileTo_S_Indirect
		= returns void for all calls to this function


	Change Record: 4/1/10 first implementation

************************ def end addFileTo_S_Indirect ***********************/
Bool addFileTo_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
						  unsigned int *_name, unsigned int _sIndirectOffset, Bool _allocate)
{

	unsigned int indirectBlock[BLOCK_SIZE / 4];

	unsigned int diskOffset;

	unsigned int buffer[BLOCK_SIZE / 4];

	diskOffset = _sIndirectOffset; //_fsm->inode.sIndirect;

	// Read indirect block
	fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

	_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);

	unsigned int i, j;

	if (_allocate == False)
	{

		for (i = 0; i < BLOCK_SIZE / 4; i++)
		{

			if (indirectBlock[i] != (unsigned int)(-1))
			{

				diskOffset = indirectBlock[i];

				fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

				_fsm->sampleCount = fread(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);

				for (j = 0; j < BLOCK_SIZE / 4; j += 4)
				{

					if (buffer[j + 3] == 0)
					{

						buffer[j + 3] = 1;
						buffer[j] = _name[0];
						buffer[j + 1] = _name[1];
						buffer[j + 2] = _inodeNumF;

						_fsm->inode.linkCount += 1;

						fseek(_fsm->diskHandle, 0, SEEK_SET);
						fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
						_fsm->sampleCount = fwrite(buffer, sizeof(unsigned int),
												   BLOCK_SIZE / 4, _fsm->diskHandle);

						fseek(_fsm->diskHandle, 0, SEEK_SET);

						closeFile(_fsm);

						return True;

					} // end if (buffer[j+3] == 0)

				} // for (j = 0; j < BLOCK_SIZE/4; j += 4)

			} // if (indirectBlock[i] != (unsigned int)(-1))

		} // for (i = 0; i < BLOCK_SIZE/4; i++)

	} // if (_allocate == False)
	else
	{

		// Allocate space then write
		for (i = 0; i < BLOCK_SIZE / 4; i++)
		{

			if (indirectBlock[i] == (unsigned int)(-1))
			{

				getSector(1, _fsm->ssm);

				if (_fsm->ssm->index[0] == (unsigned int)(-1))
				{
					return False;
				} // end if(_fsm->ssm->index[0] == (unsigned int)(-1))
				else
				{

					indirectBlock[i] = BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));

					fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

					_fsm->sampleCount = fwrite(indirectBlock, BLOCK_SIZE, 1,
											   _fsm->diskHandle);

					for (j = 0; j < BLOCK_SIZE / 4; j++)
					{
						buffer[j] = 0;
					} // end for (j = 0; j < BLOCK_SIZE/4; j++)

					buffer[3] = 1;
					buffer[0] = _name[0];
					buffer[1] = _name[1];
					buffer[2] = _inodeNumF;

					_fsm->inode.linkCount += 1;
					_fsm->inode.fileSize += BLOCK_SIZE;
					_fsm->inode.dataBlocks = _fsm->inode.fileSize / BLOCK_SIZE;

					fseek(_fsm->diskHandle, indirectBlock[i], SEEK_SET);
					_fsm->sampleCount = fwrite(buffer, sizeof(unsigned int),
											   BLOCK_SIZE / 4, _fsm->diskHandle);

					fseek(_fsm->diskHandle, 0, SEEK_SET);

					allocateSectors(_fsm->ssm);
					closeFile(_fsm);

					return True;

				} // if (_fsm->ssm->index[0] == (unsigned int)(-1))

			} // if (indirectBlock[i] == (unsigned int)(-1))

		} // for (i = 0; i < BLOCK_SIZE/4; i++)

	} // end else

	return False;

} /*end addFileTo_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
		unsigned int *_name, unsigned int _sIndirectOffset, Bool _allocate)*/

/************************ def beg rmFileFromDir ************************

	rmFileFromDir(FileSectorMgr *_fsm, unsigned int _inodeNumF,
				   unsigned int _inodeNumD) {

	Function
		removes a file from a directory

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _inodeNumF
			:= Inode number of the file

			Unsigned int inodeNumD
			:= Inode number of the directory

		Local Variables
			unsigned int i,j,k
			:= temp variables used for looping

			unsigned int diskOffset
			:= Offset from disk to block in question

			unsigned int buffer[]
			:= buffer used to store pointers in the block

	rmFileFromDir
			= returns true on successful file removal, false if file could
			  be accessed


	Change Record: 4/1/10 first implementation

************************ def end rmFileFromDir ***********************/
Bool rmFileFromDir(FileSectorMgr *_fsm, unsigned int _inodeNumF,
				   unsigned int _inodeNumD)
{

	Bool success;

	unsigned int i, j, k;

	unsigned int diskOffset;

	unsigned int buffer[BLOCK_SIZE / 4];

	unsigned int sectorNum;

	// open file, return false if access error
	success = openFile(_fsm, _inodeNumD);

	if (success == True)
	{

		if (_fsm->inode.fileType == 2)
		{ // type 2 is directory

			for (i = 0; i < 10; i++)
			{

				if (_fsm->inode.directPtr[i] != (unsigned int)(-1))
				{

					diskOffset = _fsm->inode.directPtr[i];

					// update sampleCount
					fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

					_fsm->sampleCount = fread(buffer, sizeof(unsigned int),
											  BLOCK_SIZE / 4, _fsm->diskHandle);

					// clear all 4 bytes on each loop
					for (j = 0; j < BLOCK_SIZE / 4; j += 4)
					{

						if (buffer[j + 3] == 1 && buffer[j + 2] == _inodeNumF)
						{

							buffer[j + 3] = 0;
							buffer[j] = 0;
							buffer[j + 1] = 0;
							buffer[j + 2] = 0;

							_fsm->inode.linkCount -= 1;

							if (_fsm->inode.linkCount == 0)
							{

								// if there is nothing in the directory, set size to 0
								_fsm->inode.fileSize = 0;

								// deallocate all associated sectors

								for (k = 0; k < 10; k++)
								{
									_fsm->inode.directPtr[k] = -1;
								} // end for (k = 0; k < 10; k++)

								_fsm->inode.sIndirect = -1;
								_fsm->inode.dIndirect = -1;
								_fsm->inode.tIndirect = -1;

							} // end if (_fsm->inode.linkCount == 0)

							fseek(_fsm->diskHandle, 0, SEEK_SET);
							fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
							_fsm->sampleCount = fwrite(buffer, sizeof(unsigned int),
													   BLOCK_SIZE / 4, _fsm->diskHandle);

							for (k = 0; k < BLOCK_SIZE / 4; k += 4)
							{

								if (buffer[k + 3] == 1)
								{
									break;
								} // end if (buffer[k+3] == 1)

							} // end for (k = 0; k < BLOCK_SIZE/4; k += 4)

							if (k == BLOCK_SIZE / 4)
							{

								sectorNum = _fsm->inode.directPtr[i] / BLOCK_SIZE;
								_fsm->ssm->contSectors = 1;
								_fsm->ssm->index[0] = sectorNum / 8;
								_fsm->ssm->index[1] = sectorNum % 8;
								deallocateSectors(_fsm->ssm);
								_fsm->inode.directPtr[i] = (unsigned int)(-1);
								_fsm->inode.dataBlocks -= 1;

								writeInode(&_fsm->inode, _fsm->inodeNum,
										   _fsm->diskHandle);

							} // end if (k == BLOCK_SIZE/4)

							return True;

						} // end if (buffer[j+3] == 1 && buffer[j+2] == _inodeNumF)

					} // end for (j = 0; j < BLOCK_SIZE/4; j += 4)

				} // end if (_fsm->inode.directPtr[i] != (unsigned int)(-1))

			} // end for (i = 0; i < 10; i++)

			// if the directory has files in it's sindirect pointer area, remove them
			if (_fsm->inode.sIndirect != (unsigned int)(-1))
			{

				success = rmFileFrom_S_Indirect(_fsm, _inodeNumF, (unsigned int)(-1),
												_fsm->inode.sIndirect);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.sIndirect != (unsigned int)(-1))

			// if the directory has files in it's dindirect pointer area, remove them
			if (_fsm->inode.dIndirect != (unsigned int)(-1))
			{

				success = rmFileFrom_D_Indirect(_fsm, _inodeNumF, (unsigned int)(-1),
												_fsm->inode.dIndirect);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.dIndirect != (unsigned int)(-1))

			// if the directory has files in it's tindirect pointer area, remove them
			if (_fsm->inode.tIndirect != (unsigned int)(-1))
			{

				success = rmFileFrom_T_Indirect(_fsm, _inodeNumF,
												_fsm->inode.tIndirect);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.tIndirect != (unsigned int)(-1))

		} // end if (_fsm->inode.fileType == 2)

	} // end if (success == True)

	// file failed to open, return false
	return False;

} /*rmFileFromDir(FileSectorMgr *_fsm, unsigned int _inodeNumF,
									   unsigned int _inodeNumD)*/

/*********************** def beg rmFileFrom_T_Indirect ************************

	Bool rmFileFrom_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,

						   unsigned int _dIndirectOffset)

	Function
		removes a file from a directory's T_Indirect pointer,
		calls rmFileFrom_D_Indirect

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _inodeNumF
			:= Inode number of the file to be removed

			Unsigned int _tIndirectOffset
			:= Offset to the triple indirect block containing the file to be
			   removed

		Local Variables
			unsigned int i,j,k
			:= temp variables used for looping

			unsigned int diskOffset
			:= Offset from disk to block in question

			unsigned int buffer[]
			:= buffer used to store pointers in the block

			unsigned int indirectBlock[]
			:= block filled with pointers to be searched through

	rmFileFrom_T_Indirect
			= returns true on successful file removal, false if file could
			  be accessed


	Change Record: 4/1/10 first implementation

************************ def end rmFileFrom_T_Indirect ***********************/
Bool rmFileFrom_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
						   unsigned int _tIndirectOffset)
{

	unsigned int indirectBlock[BLOCK_SIZE / 4];

	unsigned int diskOffset;

	unsigned int sectorNum;

	Bool success;

	diskOffset = _tIndirectOffset;

	// set sampleCount
	fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

	_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);

	unsigned int i, k;

	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];

			success = rmFileFrom_D_Indirect(_fsm, _inodeNumF,
											_tIndirectOffset, diskOffset);

			if (success == True)
			{

				diskOffset = _tIndirectOffset;

				fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

				_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE,
										  1, _fsm->diskHandle);

				for (k = 0; k < BLOCK_SIZE / 4; k++)
				{

					if (indirectBlock[k] != (unsigned int)(-1))
					{
						break;
					} // end if (indirectBlock[k] != (unsigned int)(-1))

				} // end for (k = 0; k < BLOCK_SIZE/4; k++)

				if (k == BLOCK_SIZE / 4)
				{

					sectorNum = _tIndirectOffset / BLOCK_SIZE;
					_fsm->ssm->contSectors = 1;
					_fsm->ssm->index[0] = sectorNum / 8;
					_fsm->ssm->index[1] = sectorNum % 8;

					deallocateSectors(_fsm->ssm);

					_fsm->inode.tIndirect = (unsigned int)(-1);
					writeInode(&_fsm->inode, _fsm->inodeNum, _fsm->diskHandle);

				} // end if (k == BLOCK_SIZE/4)

				return True;

			} // if (success == True)

		} // if (indirectBlock[i] != (unsigned int)(-1))

	} // for (i = 0; i < BLOCK_SIZE/4; i++)

	return False;

} /*rmFileFrom_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
							unsigned int _tIndirectOffset){*/

/*********************** def beg rmFileFrom_D_Indirect ************************

	Bool rmFileFrom_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,

						   unsigned int _dIndirectOffset)

	Function
		removes a file from a directory's D_Indirect pointer,
		calls rmFileFrom_D_Indirect

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _inodeNumF
			:= Inode number of the file to be removed

			Unsigned int _dIndirectOffset
			:= Offset to the double indirect block containing the file to be
			   removed

		Local Variables
			unsigned int i,j,k
			:= temp variables used for looping

			unsigned int diskOffset
			:= Offset from disk to block in question

			unsigned int buffer[]
			:= buffer used to store pointers in the block

			unsigned int indirectBlock[]
			:= block filled with pointers to be searched through

	rmFileFrom_D_Indirect
			= returns true on successful file removal, false if file could
			  be accessed


	Change Record: 4/1/10 first implementation

************************ def end rmFileFrom_D_Indirect ***********************/
Bool rmFileFrom_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
						   unsigned int _tIndirectOffset, unsigned int _dIndirectOffset)
{

	unsigned int indirectBlock[BLOCK_SIZE / 4];

	unsigned int diskOffset;

	unsigned int sectorNum;

	Bool success;

	// load next pointer
	diskOffset = _dIndirectOffset;

	fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

	_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);

	unsigned int i, k;

	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];

			success = rmFileFrom_S_Indirect(_fsm, _inodeNumF, _dIndirectOffset,
											diskOffset);

			if (success == True)
			{

				diskOffset = _dIndirectOffset;

				fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

				_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1,
										  _fsm->diskHandle);

				for (k = 0; k < BLOCK_SIZE / 4; k++)
				{

					if (indirectBlock[k] != (unsigned int)(-1))
					{
						break;
					} // if (indirectBlock[k] != (unsigned int)(-1))

				} // end for (k = 0; k < BLOCK_SIZE/4; k++)

				if (k == BLOCK_SIZE / 4)
				{

					sectorNum = _dIndirectOffset / BLOCK_SIZE;
					_fsm->ssm->contSectors = 1;
					_fsm->ssm->index[0] = sectorNum / 8;
					_fsm->ssm->index[1] = sectorNum % 8;

					deallocateSectors(_fsm->ssm);

					if (_tIndirectOffset != (unsigned int)(-1))
					{

						diskOffset = _tIndirectOffset;

						fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

						_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE,
												  1, _fsm->diskHandle);

						for (k = 0; k < BLOCK_SIZE / 4; k++)
						{

							if (indirectBlock[k] == _dIndirectOffset)
							{
								indirectBlock[k] = (unsigned int)(-1);
								break;
							} // end if (indirectBlock[k] == _dIndirectOffset)

						} // end for (k = 0; k < BLOCK_SIZE/4; k++)

						fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

						_fsm->sampleCount = fwrite(indirectBlock, BLOCK_SIZE,
												   1, _fsm->diskHandle);
					}
					else
					{

						_fsm->inode.dIndirect = (unsigned int)(-1);

						writeInode(&_fsm->inode, _fsm->inodeNum, _fsm->diskHandle);

					} // end else

				} // if (k == BLOCK_SIZE/4)

				return True;

			} // end if (success == True)

		} // end if (indirectBlock[i] != (unsigned int)(-1))

	} // end for (i = 0; i < BLOCK_SIZE/4; i++)

	return False;

} /*rmFileFrom_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
		 unsigned int _tIndirectOffset, unsigned int _dIndirectOffset){*/

/*********************** def beg rmFileFrom_S_Indirect ************************

	Bool rmFileFrom_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
	 unsigned int _dIndirectOffset, unsigned int _sIndirectOffset) {

	Function
		removes a file from a directory's S_Indirect

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _inodeNumF
			:= Inode number of the file to be removed

			Unsigned int _sIndirectOffset
			:= Offset to the single indirect block containing the file to be
			   removed

		Local Variables
			unsigned int i,j,k
			:= temp variables used for looping

			unsigned int diskOffset
			:= Offset from disk to block in question

			unsigned int buffer[]
			:= buffer used to store pointers in the block

			unsigned int indirectBlock[]
			:= block filled with pointers to be searched through

	rmFileFrom_S_Indirect
			= returns true on successful file removal, false if file could
			  be accessed


	Change Record: 4/1/10 first implementation

************************ def end rmFileFrom_S_Indirect ***********************/
Bool rmFileFrom_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
						   unsigned int _dIndirectOffset, unsigned int _sIndirectOffset)
{

	unsigned int indirectBlock[BLOCK_SIZE / 4];

	unsigned int diskOffset;

	unsigned int buffer[BLOCK_SIZE / 4];

	unsigned int sectorNum;

	diskOffset = _sIndirectOffset;

	// set simpleCount
	fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

	_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);

	unsigned int i, j, k;

	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];

			fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

			_fsm->sampleCount = fread(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);

			// clear all data in block
			for (j = 0; j < BLOCK_SIZE / 4; j += 4)
			{

				if (buffer[j + 3] == 1 && buffer[j + 2] == _inodeNumF)
				{

					buffer[j + 3] = 0;
					buffer[j] = 0;
					buffer[j + 1] = 0;
					buffer[j + 2] = 0;

					_fsm->inode.linkCount -= 1;

					fseek(_fsm->diskHandle, 0, SEEK_SET);

					fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

					_fsm->sampleCount = fwrite(buffer, sizeof(unsigned int),
											   BLOCK_SIZE / 4, _fsm->diskHandle);

					for (k = 0; k < BLOCK_SIZE / 4; k += 4)
					{

						if (buffer[k + 3] == 1)
						{
							break;
						} // end if (buffer[k+3] == 1)

					} // end for (k = 0; k < BLOCK_SIZE/4; k += 4)

					if (k == BLOCK_SIZE / 4)
					{

						sectorNum = indirectBlock[i] / BLOCK_SIZE;
						_fsm->ssm->contSectors = 1;
						_fsm->ssm->index[0] = sectorNum / 8;
						_fsm->ssm->index[1] = sectorNum % 8;

						deallocateSectors(_fsm->ssm);

						_fsm->inode.dataBlocks -= 1;

						writeInode(&_fsm->inode, _fsm->inodeNum, _fsm->diskHandle);

						indirectBlock[i] = (unsigned int)(-1);

						diskOffset = _sIndirectOffset;
						fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
						_fsm->sampleCount = fwrite(indirectBlock, BLOCK_SIZE,
												   1, _fsm->diskHandle);

						for (k = 0; k < BLOCK_SIZE / 4; k++)
						{

							if (indirectBlock[k] != (unsigned int)(-1))
							{
								break;
							} // end if (indirectBlock[k] != (unsigned int)(-1))

						} // end for (k = 0; k < BLOCK_SIZE/4; k++)

						if (k == BLOCK_SIZE / 4)
						{

							sectorNum = _sIndirectOffset / BLOCK_SIZE;
							_fsm->ssm->contSectors = 1;
							_fsm->ssm->index[0] = sectorNum / 8;
							_fsm->ssm->index[1] = sectorNum % 8;

							deallocateSectors(_fsm->ssm);

							if (_dIndirectOffset != (unsigned int)(-1))
							{

								diskOffset = _dIndirectOffset;

								fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

								_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE,
														  1, _fsm->diskHandle);

								for (k = 0; k < BLOCK_SIZE / 4; k++)
								{

									if (indirectBlock[k] == _sIndirectOffset)
									{
										indirectBlock[k] = (unsigned int)(-1);
										break;
									} // end if (indirectBlock[k] == _sIndirectOffset)

								} // end for (k = 0; k < BLOCK_SIZE/4; k++)

								fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
								_fsm->sampleCount = fwrite(indirectBlock, BLOCK_SIZE,
														   1, _fsm->diskHandle);

							} // end if (_dIndirectOffset != (unsigned int)(-1))
							else
							{

								_fsm->inode.sIndirect = (unsigned int)(-1);
								writeInode(&_fsm->inode, _fsm->inodeNum,
										   _fsm->diskHandle);
							} // end else

						} // end if (k == BLOCK_SIZE/4)

					} // end if (k == BLOCK_SIZE/4)

					return True;

				} // end if (buffer[j+3] == 1 && buffer[j+2] == _inodeNumF)

			} // end for (j = 0; j < BLOCK_SIZE/4; j += 4)

		} // end if (indirectBlock[i] != (unsigned int)(-1))

	} // end for (i = 0; i < BLOCK_SIZE/4; i++)

	return False;

} /*rmFileFrom_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
			 unsigned int _dIndirectOffset, unsigned int _sIndirectOffset){*/

/********************* def beg readFrom_T_IndirectBlocks **********************

   void readFrom_T_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer,
								  unsigned int _diskOffset)

	Function:
			fills buffer with all pointers from a specified tindirect
			one block a block at a time.
			Calls readFrom_D_IndirectBlocks

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			void *_buffer
			:= pointer to the buffer used to store the value as it's written

			unsigned int _diskOffset
			:= offset to first usable block on disk

		Local Variables
			unsigned int indirectBlock[BLOCK_SIZE / 4]
			:= array to store the new block that will be written

			 unsigned int diskOffset
			:= offset to first usable sector on disk

			void* buffer
			:= pointer to data yet to be read

			unsigned int i
			:= variable used for looping

		Struct Variables (from FileSectorMgr struct)
			FILE *diskHandle
			:= pointer to the hard disk

			unsigned int diskOffset
			:= offset from inode area to data area of disk

			unsigned int sampleCount
			:= value used to keep track of the number of samples

	void readFrom_T_IndirectBlocks
	   = returns void for all calls to this function

	Change Record: 4/12/10 first implemented

************************def end readFrom_T_IndirectBlocks ********************/
void readFrom_T_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer,
							   unsigned int _diskOffset)
{

	unsigned int indirectBlock[BLOCK_SIZE / 4];

	unsigned int diskOffset;

	void *buffer;

	buffer = _buffer;

	diskOffset = _diskOffset;

	fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

	_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);

	unsigned int i;

	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];
			readFrom_D_IndirectBlocks(_fsm, buffer, diskOffset);
			buffer += BLOCK_SIZE * D_INDIRECT_BLOCKS;

		} // end if (indirectBlock[i] != (unsigned int)(-1))

	} // end for (i = 0; i < BLOCK_SIZE/4; i++)

} /*readFrom_T_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer,
											 unsigned int _diskOffset){*/

/*****************def beg readFrom_D_IndirectBlocks **************************
   void readFrom_D_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer,
								  unsigned int _diskOffset)

	Function:
			fills buffer with all pointers from a specified dindirect block
			one block at a time.
			Calls readFrom_S_IndirectBlocks

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			void *_buffer
			:= pointer to the buffer used to store the value as it's written

			unsigned int _diskOffset
			:= offset to first usable block on disk

		Local Variables
			unsigned int indirectBlock[BLOCK_SIZE / 4]
			:= array to store the new block that will be written

			 unsigned int diskOffset
			:= offset to first usable sector on disk

			void* buffer
			:= pointer to data yet to be read

			unsigned int i
			:= variable used for looping

		Struct Variables (from FileSectorMgr struct)
			FILE *diskHandle
			:= pointer to the hard disk

			unsigned int diskOffset
			:= offset from inode area to data area of disk

			unsigned int sampleCount
			:= value used to keep track of the number of samples

	void readFrom_D_IndirectBlocks
	   = returns void for all calls to this function

	Change Record: 4/12/10 first implemented

*****************def end readFrom_D_IndirectBlocks **************************/

void readFrom_D_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer,
							   unsigned int _diskOffset)
{

	unsigned int indirectBlock[BLOCK_SIZE / 4];

	unsigned int diskOffset;

	void *buffer;

	buffer = _buffer;

	diskOffset = _diskOffset;

	fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

	_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);

	unsigned int i;

	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];
			readFrom_S_IndirectBlocks(_fsm, buffer, diskOffset);
			buffer += BLOCK_SIZE * S_INDIRECT_BLOCKS;

		} // end if (indirectBlock[i] != (unsigned int)(-1))

	} // end for (i = 0; i < BLOCK_SIZE/4; i++)

} /*readFrom_D_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer,
											 unsigned int _diskOffset){*/

/********************* def beg readFrom_S_IndirectBlocks **********************

	  void readFrom_S_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer,

									 unsigned int _diskOffset)

	Function:
			fills buffer with all pointers from a specified dindirect block
			one block at a time.
			Calls readFrom_S_IndirectBlocks

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			void *_buffer
			:= pointer to the buffer used to store the value as it's written

			unsigned int _diskOffset
			:= offset to first usable block on disk

		Local Variables
			unsigned int indirectBlock[BLOCK_SIZE / 4]
			:= array to store the new block that will be written

			 unsigned int diskOffset
			:= offset to first usable sector on disk

			void* buffer
			:= pointer to data yet to be read

			unsigned int i
			:= variable used for looping

		Struct Variables (from FileSectorMgr struct)
			FILE *diskHandle
			:= pointer to the hard disk

			unsigned int diskOffset
			:= offset from inode area to data area of disk

			unsigned int sampleCount
			:= value used to keep track of the number of samples

	void readFrom_S_IndirectBlocks
	   = returns void for all calls to this function

	Change Record: 4/12/10 first implemented

******************def end readFrom_S_IndirectBlocks **************************/

void readFrom_S_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer,
							   unsigned int _diskOffset)
{

	unsigned int indirectBlock[BLOCK_SIZE / 4];

	unsigned int diskOffset;

	void *buffer;

	buffer = _buffer;

	diskOffset = _diskOffset;

	fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

	_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);

	unsigned int i;

	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];
			fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
			_fsm->sampleCount = fread(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);
			buffer += BLOCK_SIZE;

		} // end if (indirectBlock[i] != (unsigned int)(-1))

	} // end for (i = 0; i < BLOCK_SIZE/4; i++)

} /*end readFrom_S_IndirectBlocks(FileSectorMgr *_fsm, void *_buffer,
										  unsigned int _diskOffset)*/

/************************** def beg aloc_T_Indirect **************************

	unsigned int aloc_T_Indirect(FileSectorMgr *_fsm,
								 long long int _blockCount)

	Function:
			aloc_T_Indirect gets a sector from the SSM, fills it with pointers
			to other sectors which get filled with pointers to more sectors
			which are filled with pointers to yet more sectors,
			then returns the address for the Tindirect pointer

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct
			long long int _blockCount
			:= the number of blocks to be allocated

		Local Variables
			unsigned int baseAddress
			:= address to newly allocated sindirect to be filled with pointers

			unsigned int address
			:= used to fill the sindirect block with pointers

			unsigned int diskOffset
			:= offset to first sector allocated

			unsigned int i
			:= variable used for looping

		Struct Variables (from FileSectorMgr struct)
			SecSpaceMgr ssm[1]
			:= SSM used to manage disk blocks

			FILE *iMapHandle
			:= pointer to the inode map

			FILE *diskHandle
			:= pointer to the hard disk

			unsigned int diskOffset
			:= offset from inode area to data area of disk

			unsigned int sampleCount
			:= used to keep track of the number of samples

			unsigned int index[2]
			:= used to access bit and byte offset to blocks

	unsigned int aloc_T_Indirect
	   = returns the address of the Tindirect block in an
		 unsigned int on success, returns -1 on failure


	Change Record: 4/12/10 first implemented


************************** def end aloc_T_Indirect **************************/
unsigned int aloc_T_Indirect(FileSectorMgr *_fsm, long long int _blockCount)
{

	unsigned int baseAddress;

	unsigned int address;

	unsigned int diskOffset;

	long long int blockCount;

	getSector(1, _fsm->ssm);

	if (_fsm->ssm->index[0] != (unsigned int)(-1))
	{

		baseAddress = BLOCK_SIZE * (8 * _fsm->ssm->index[0] + _fsm->ssm->index[1]);

		allocateSectors(_fsm->ssm);

		diskOffset = baseAddress; //_fsm->inode.tIndirect;

		// Initialize the indirect block pointers to -1
		unsigned int i;

		unsigned int base[BLOCK_SIZE / 4];

		for (i = 0; i < BLOCK_SIZE / 4; i++)
		{
			base[i] = (unsigned int)(-1);
		} // end for (i = 0; i < BLOCK_SIZE/4; i++)

		fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

		_fsm->sampleCount = fwrite(base, sizeof(unsigned int),
								   BLOCK_SIZE / 4, _fsm->diskHandle);

		// Allocate _blockCount blocks and store their pointers in
		// the indirect block
		blockCount = _blockCount;

		for (i = 0; i < PTRS_PER_BLOCK; i++)
		{

			address = aloc_D_Indirect(_fsm, _blockCount);

			// write to buffer
			fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
			_fsm->sampleCount = fwrite(&address, sizeof(unsigned int),
									   1, _fsm->diskHandle);
			diskOffset += 4;

			// update block count
			blockCount -= D_INDIRECT_BLOCKS;

			if (blockCount < 0)
			{
				break;
			} // end if (blockCount < 0)

		} // end for (i = 0; i < PTRS_PER_BLOCK; i++)

		// return the address of the tindirect block
		return baseAddress;

	} // end if (_fsm->ssm->index[0] != (unsigned int)(-1))
	else
	{

		return (unsigned int)(-1);

	} // end else

} // end aloc_T_Indirect(FileSectorMgr *_fsm, long long int _blockCount)

/************************** def beg aloc_D_Indirect **************************

	unsigned int aloc_D_Indirect(FileSectorMgr *_fsm,
								 long long int _blockCount)

	Function:
			aloc_D_Indirect gets a sector from the SSM, fills it with pointers
			to other sectors which get filled with pointers to more sectors,

			then returns the address for the Dindirect pointer

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct
			long long int _blockCount
			:= the number of blocks to be allocated

		Local Variables
			unsigned int baseAddress
			:= address to newly allocated sindirect to be filled with pointers

			unsigned int address
			:= used to fill the sindirect block with pointers

			unsigned int diskOffset
			:= offset to first sector allocated

			unsigned int i
			:= variable used for looping

		Struct Variables (from FileSectorMgr struct)
			SecSpaceMgr ssm[1]
			:= SSM used to manage disk blocks

			FILE *iMapHandle
			:= pointer to the inode map

			FILE *diskHandle
			:= pointer to the hard disk

			unsigned int diskOffset
			:= offset from inode area to data area of disk

			unsigned int sampleCount
			:= used to keep track of the number of samples

			unsigned int index[2]
			:= used to access bit and byte offset to blocks

	unsigned int aloc_D_Indirect
	   = returns the address of the Dindirect block in an
		 unsigned int on success, returns -1 on failure


	Change Record: 4/12/10 first implemented

************************** def end aloc_D_Indirect **************************/
unsigned int aloc_D_Indirect(FileSectorMgr *_fsm, long long int _blockCount)
{

	unsigned int baseAddress;

	unsigned int address;

	unsigned int diskOffset;

	long long int blockCount;

	getSector(1, _fsm->ssm);

	// calculate base address
	if (_fsm->ssm->index[0] != (unsigned int)(-1))
	{

		baseAddress = BLOCK_SIZE * (8 * _fsm->ssm->index[0] + _fsm->ssm->index[1]);

		allocateSectors(_fsm->ssm);

		diskOffset = baseAddress; //_fsm->inode.sIndirect;

		// Initialize the indirect block pointers to -1
		unsigned int i;

		unsigned int base[BLOCK_SIZE / 4];

		for (i = 0; i < BLOCK_SIZE / 4; i++)
		{
			base[i] = (unsigned int)(-1);
		} // end for (i = 0; i < BLOCK_SIZE/4; i++)

		fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

		_fsm->sampleCount = fwrite(base, sizeof(unsigned int),
								   BLOCK_SIZE / 4, _fsm->diskHandle);

		// Allocate _blockCount blocks and store their pointers in
		// the indirect block
		blockCount = _blockCount;

		for (i = 0; i < PTRS_PER_BLOCK; i++)
		{

			address = aloc_S_Indirect(_fsm, _blockCount);

			// write to buffer
			fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

			_fsm->sampleCount = fwrite(&address, sizeof(unsigned int),
									   1, _fsm->diskHandle);
			diskOffset += 4;

			// update block count
			blockCount -= S_INDIRECT_BLOCKS;

			if (blockCount < 0)
			{
				break;
			} // end if (blockCount < 0

		} // end for (i = 0; i < PTRS_PER_BLOCK; i++)

		// return the address of the Dindirect block
		return baseAddress;
	}
	else
	{

		return (unsigned int)(-1);

	} // end else

} // end aloc_D_Indirect(FileSectorMgr *_fsm, long long int _blockCount

/************************** def beg aloc_S_Indirect **************************

	unsigned int aloc_S_Indirect(FileSectorMgr *_fsm,
								 long long int _blockCount)

	Function:
			aloc_S_Indirect gets a sector from the SSM, fills it with pointers
			to other sectors and returns the address for the Sindirect pointer

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct
			long long int _blockCount
			:= the number of blocks to be allocated

		Local Variables
			unsigned int baseAddress
			:= address to newly allocated sindirect to be filled with pointers

			unsigned int address
			:= used to fill the sindirect block with pointers

			unsigned int diskOffset
			:= offset to first sector allocated

			unsigned int i
			:= variable used for looping

		Struct Variables (from FileSectorMgr struct)
			SecSpaceMgr ssm[1]
			:= SSM used to manage disk blocks

			FILE *iMapHandle
			:= pointer to the inode map

			FILE *diskHandle
			:= pointer to the hard disk

			unsigned int diskOffset
			:= offset from inode area to data area of disk

			unsigned int sampleCount
			:= used to keep track of the number of samples

			unsigned int index[2]
			:= used to access bit and byte offset to blocks

	unsigned int aloc_S_Indirect
	   = returns the address of the Sindirect block in an
		 unsigned int on success, returns -1 on failure


	Change Record: 4/12/10 first implemented

************************** def end aloc_S_Indirect **************************/
unsigned int aloc_S_Indirect(FileSectorMgr *_fsm, long long int _blockCount)
{

	unsigned int baseAddress;

	unsigned int address;

	unsigned int diskOffset;

	getSector(1, _fsm->ssm);

	// calculate base address
	if (_fsm->ssm->index[0] != (unsigned int)(-1))
	{

		baseAddress = BLOCK_SIZE * (8 * _fsm->ssm->index[0] + _fsm->ssm->index[1]);

		allocateSectors(_fsm->ssm);

		diskOffset = baseAddress; //_fsm->inode.sIndirect;

		// Initialize the indirect block pointers to -1
		unsigned int i;

		unsigned int base[BLOCK_SIZE / 4];

		for (i = 0; i < BLOCK_SIZE / 4; i++)
		{
			base[i] = (unsigned int)(-1);
		} // end for (i = 0; i < BLOCK_SIZE/4; i++)

		fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

		_fsm->sampleCount = fwrite(base, sizeof(unsigned int),
								   BLOCK_SIZE / 4, _fsm->diskHandle);

		// Allocate _blockCount blocks and store associated pointers in
		// the indirect block
		for (i = 0; i < _blockCount && i < PTRS_PER_BLOCK; i++)
		{

			getSector(1, _fsm->ssm);

			if (_fsm->ssm->index[0] != (unsigned int)(-1))
			{

				address = BLOCK_SIZE * (8 * _fsm->ssm->index[0] + _fsm->ssm->index[1]);
				allocateSectors(_fsm->ssm);

				// write to buffer
				fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
				_fsm->sampleCount = fwrite(&address, sizeof(unsigned int),
										   1, _fsm->diskHandle);
				diskOffset += 4;
			} // end if (_fsm->ssm->index[0] != (unsigned int)(-1))

		} // end for (i = 0; i < _blockCount && i < PTRS_PER_BLOCK; i++

		// return the address of the sindirect block
		return baseAddress;
	}
	else
	{
		return (unsigned int)(-1);
	} // end else

} // end aloc_S_Indirect(FileSectorMgr *_fsm, long long int _blockCount)

/********************* def beg writeTo_T_IndirectBlocks **********************

   void writeTo_T_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset,

								 void *_buffer, unsigned int _tIndirectPtrs) {

	Function:
			Writes triple indirect blocks to be written into a buffer 1 block
			at a time. Calls writeTo_D_IndirectBlocks to write
			double indirect blocks

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			unsigned int _baseOffset
			:= the offset to the first usable data on disk

			void *_buffer
			:= pointer to the buffer used to store the value as it's written

			unsigned int _dIndirectPtrs
			:= the number of blocks to allocate through the dIndirect block

		Local Variables
			 unsigned int diskOffset
			:= offset to first usable sector on disk

			unsigned int indirectBlock[BLOCK_SIZE / 4]
			:= array to store the new block that will be written

			void* buffer
			:= pointer to data yet to be written

			unsigned int tIndirectPtrs
			:= the amount of sectors to be allocated through the tIndirect ptr

			unsigned int i
			:= variable used for looping

		Struct Variables (from FileSectorMgr struct)
			FILE *diskHandle
			:= pointer to the hard disk

			unsigned int diskOffset
			:= offset from inode area to data area of disk


	void writeTo_T_IndirectBlocks
	   = returns void for all calls to this function

	Change Record: 4/12/10 first implemented

****************** def end writeTo_T_IndirectBlocks **************************/

void writeTo_T_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset,
							  void *_buffer, unsigned int _tIndirectPtrs)
{

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

	for (i = 0; i < PTRS_PER_BLOCK; i++)
	{

		diskOffset = indirectBlock[i];

		if (diskOffset == (unsigned int)(-1))
		{
			break;
		} // end if (diskOffset == (unsigned int)(-1))
		else
		{

			if (tIndirectPtrs - D_INDIRECT_BLOCKS > 0)
			{

				// write a dindirect worth of blocks then continue looping
				writeTo_D_IndirectBlocks(_fsm, diskOffset, buffer, D_INDIRECT_BLOCKS);
				buffer += BLOCK_SIZE * D_INDIRECT_BLOCKS;
				tIndirectPtrs -= D_INDIRECT_BLOCKS;

			} // end if (tIndirectPtrs - D_INDIRECT_BLOCKS > 0)
			else if (tIndirectPtrs - D_INDIRECT_BLOCKS == 0)
			{

				// write the rest into a dindirect block
				writeTo_D_IndirectBlocks(_fsm, diskOffset, buffer, D_INDIRECT_BLOCKS);
				buffer += BLOCK_SIZE * D_INDIRECT_BLOCKS;
				tIndirectPtrs -= D_INDIRECT_BLOCKS;
				break;
			}
			else
			{
				// write the rest into a dindirect block
				writeTo_D_IndirectBlocks(_fsm, diskOffset, buffer, tIndirectPtrs);
				buffer += BLOCK_SIZE * tIndirectPtrs;
				tIndirectPtrs = 0;
				break;
			} // end else

		} // end else

	} // end for (i = 0; i < PTRS_PER_BLOCK; i++)

} /*end writeTo_D_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset,
									void *_buffer, unsigned int _dIndirectPtrs)*/

/********************* def beg writeTo_D_IndirectBlocks **********************

   void writeTo_D_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset,

   void *_buffer, unsigned int _sIndirectPtrs) {

	Function:
			Writes double indirect blocks to be written into a buffer 1 block
			at a time. Calls writeTo_S_IndirectBlocks

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			unsigned int _baseOffset
			:= the offset to the first usable data on disk

			void *_buffer
			:= pointer to the buffer used to store the value as it's written

			unsigned int _dIndirectPtrs
			:= the number of blocks to allocate through the dIndirect block

		Local Variables
			 unsigned int diskOffset
			:= offset to first usable sector on disk

			unsigned int indirectBlock[BLOCK_SIZE / 4]
			:= array to store the new block that will be written

			void* buffer
			:= pointer to data yet to be written

			unsigned int dIndirectPtrs
			:= the amount of sectors to be allocated through the dIndirect ptr

			unsigned int i
			:= variable used for looping

		Struct Variables (from FileSectorMgr struct)
			FILE *diskHandle
			:= pointer to the hard disk

			unsigned int diskOffset
			:= offset from inode area to data area of disk


	void writeTo_d_IndirectBlocks
	   = returns void for all calls to this function

	Change Record: 4/12/10 first implemented

*******************def end writeTo_D_IndirectBlocks **************************/
void writeTo_D_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset,
							  void *_buffer, unsigned int _dIndirectPtrs)
{

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

	for (i = 0; i < PTRS_PER_BLOCK; i++)
	{

		diskOffset = indirectBlock[i];

		if (diskOffset == (unsigned int)(-1))
		{
			break;
		} // end if (diskOffset == (unsigned int)(-1))
		else
		{

			// write a sindirect block at a time
			if (dIndirectPtrs - S_INDIRECT_BLOCKS > 0)
			{

				writeTo_S_IndirectBlocks(_fsm, diskOffset, buffer, S_INDIRECT_BLOCKS);
				buffer += BLOCK_SIZE * S_INDIRECT_BLOCKS;
				dIndirectPtrs -= S_INDIRECT_BLOCKS;

			} // end if (dIndirectPtrs - S_INDIRECT_BLOCKS > 0)
			// write the rest into a sindirect block
			else if (dIndirectPtrs - S_INDIRECT_BLOCKS == 0)
			{

				writeTo_S_IndirectBlocks(_fsm, diskOffset, buffer, S_INDIRECT_BLOCKS);
				buffer += BLOCK_SIZE * S_INDIRECT_BLOCKS;
				dIndirectPtrs -= S_INDIRECT_BLOCKS;
				break;

			} // end else if (dIndirectPtrs - S_INDIRECT_BLOCKS == 0)

			// write the rest into a sindirect block
			else
			{

				writeTo_S_IndirectBlocks(_fsm, diskOffset, buffer, dIndirectPtrs);
				buffer += BLOCK_SIZE * dIndirectPtrs;
				dIndirectPtrs = 0;
				break;

			} // end else

		} // end else

	} // end for (i = 0; i < PTRS_PER_BLOCK; i++)

} /*end writeTo_D_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset,
								void *_buffer, unsigned int _dIndirectPtrs)*/

/********************* def beg writeTo_S_IndirectBlocks **********************

   void writeTo_S_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset,

   void *_buffer, unsigned int _sIndirectPtrs) {

	Function:
			Writes single indirect blocks to be written into a buffer a block
			at a time.

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			unsigned int _baseOffset
			:= the offset to the first usable data on disk

			void *_buffer
			:= pointer to the buffer used to store the value as it's written

			unsigned int _sIndirectPtrs
			:= the number of blocks to allocate through the sIndirect block

		Local Variables
			 unsigned int diskOffset
			:= offset to first usable sector on disk

			unsigned int indirectBlock[BLOCK_SIZE / 4]
			:= array to store the new block that will be written

			void* buffer
			:= pointer to data yet to be written

			unsigned int sIndirectPtrs
			:= the amount of sectors to be allocated through the sIndirect ptr

			unsigned int i
			:= variable used for looping

		Struct Variables (from FileSectorMgr struct)
			FILE *diskHandle
			:= pointer to the hard disk

			unsigned int diskOffset
			:= offset from inode area to data area of disk


	void writeTo_S_IndirectBlocks
	   = returns void for all calls to this function

	Change Record: 4/12/10 first implemented

********************def end writeTo_S_IndirectBlocks *************************/
void writeTo_S_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _baseOffset,
							  void *_buffer, unsigned int _sIndirectPtrs)
{

	unsigned int diskOffset;

	unsigned int indirectBlock[BLOCK_SIZE / 4];

	void *buffer;

	buffer = _buffer;

	// read the single indirect pointer
	fseek(_fsm->diskHandle, _baseOffset, SEEK_SET);

	_fsm->sampleCount = fread(&indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);

	// write to each of the sIndirectPtrs blocks
	unsigned int i;

	for (i = 0; i < _sIndirectPtrs; i++)
	{

		// get next pointer
		diskOffset = indirectBlock[i];

		// if next pointer is unused, break out of loop
		if (diskOffset == (unsigned int)(-1))
		{
			break;
		}
		else
		{

			fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
			_fsm->sampleCount = fwrite(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);
			buffer += BLOCK_SIZE; // increment buffer

		} // end else

	} // end for (i = 0; i < _sIndirectPtrs; i++)

} /*end void writeTo_S_IndirectBlocks(FileSectorMgr *_fsm,
		  unsigned int _baseOffset, void *_buffer, unsigned int _sIndirectPtrs)*/

/******************************* def beg rmFile ********************************

   Bool rmFile(FileSectorMgr *_fsm, unsigned int _inodeNum,
	unsigned int _inodeNumD)

	Function:
			Removes a file

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			unsigned int _inodeNum
			:= inode number of file

			unsigned int _inodeNumD
			:= inode number of directory

		Local Variables
			Bool success
			:= holds whether opening file was successful

	Bool rmFile
	   = returns true if file removed succesfully,
		false otherwise

	Change Record: 4/12/10 first implemented

********************def end rmFile *************************/
Bool rmFile(FileSectorMgr *_fsm, unsigned int _inodeNum,
			unsigned int _inodeNumD)
{

	// Open file at Inode _inodeNum for reading
	Bool success;

	success = openFile(_fsm, _inodeNum);

	if (success == False)
	{
		return False;
	} // end if (success == False)

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

	for (i = 0; i < 10; i++)
	{
		directPtrs[i] = _fsm->inode.directPtr[i];
	} // end for (i = 0; i < 10; i++)

	// Read data from direct pointers into buffer _buffer
	for (i = 0; i < 10; i++)
	{

		diskOffset = directPtrs[i];

		if (diskOffset != (unsigned int)(-1))
		{

			if (fileType == 2)
			{

				fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
				_fsm->sampleCount = fread(buffer, sizeof(unsigned int), BLOCK_SIZE / 4,
										  _fsm->diskHandle);

				for (j = 8; j < BLOCK_SIZE / 4; j += 4)
				{

					if (buffer[j + 3] == 1)
					{
						rmFile(_fsm, buffer[j + 2], _inodeNum);
					} // end if (buffer[j+3] == 1)

				} // end for (j = 8; j < BLOCK_SIZE/4; j += 4)

			} // end if (fileType == 2)

			sectorNumber = diskOffset / BLOCK_SIZE;
			byte = sectorNumber / 8;
			bit = sectorNumber % 8;

			_fsm->ssm->contSectors = 1;
			_fsm->ssm->index[0] = byte;
			_fsm->ssm->index[1] = bit;
			deallocateSectors(_fsm->ssm);

		} // end if (diskOffset != (unsigned int)(-1))

	} // end for (i = 0; i < 10; i++)

	// Read data from single indirect pointer into buffer _buffer
	diskOffset = sIndirect;

	if (diskOffset != (unsigned int)(-1))
	{

		if (fileType == 1)
		{
			// If remove directory, pass inodeNumD
			rmFile_S_IndirectBlocks(_fsm, fileType, _inodeNumD, diskOffset);
		} // end if (fileType == 1)
		else if (fileType == 2)
		{
			// If remove file, pass inodeNum
			rmFile_S_IndirectBlocks(_fsm, fileType, _inodeNum, diskOffset);
		} // end else if (fileType == 2)

	} // end if (diskOffset != (unsigned int)(-1))

	// Read data from double indirect pointer into buffer _buffer
	diskOffset = dIndirect;

	if (diskOffset != (unsigned int)(-1))
	{

		if (fileType == 1)
		{
			// If remove directory, pass inodeNumD
			rmFile_D_IndirectBlocks(_fsm, fileType, _inodeNumD, diskOffset);
		} // end if (fileType == 1)
		else if (fileType == 2)
		{
			// If remove file, pass inodeNum
			rmFile_D_IndirectBlocks(_fsm, fileType, _inodeNum, diskOffset);
		} // end else if (fileType == 2)

	} // end if (diskOffset != (unsigned int)(-1))

	// Read data from triple indirect pointer into buffer _buffer
	diskOffset = tIndirect;

	if (diskOffset != (unsigned int)(-1))
	{

		if (fileType == 1)
		{
			// If remove directory, pass inodeNumD
			rmFile_T_IndirectBlocks(_fsm, fileType, _inodeNumD, diskOffset);
		} // end if (fileType == 1)
		else if (fileType == 2)
		{
			// If remove file, pass inodeNum
			rmFile_T_IndirectBlocks(_fsm, fileType, _inodeNum, diskOffset);
		} // end else if (fileType == 2)

	} // end if (diskOffset != (unsigned int)(-1))

	// Open inode, assign to success whether opening worked
	success = openFile(_fsm, _inodeNum);

	// If inode didn't open, return false
	if (success == False)
	{
		return False;
	} // end if (success == False)

	// Clear the inode
	_fsm->inode.fileType = 0;
	_fsm->inode.fileSize = 0;
	_fsm->inode.permissions = 0;
	_fsm->inode.linkCount = 0;
	_fsm->inode.dataBlocks = 0;
	_fsm->inode.owner = 0;
	_fsm->inode.status = 0;

	// Clear the direct pointers
	for (i = 0; i < 10; i++)
	{
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
	writeInode(&_fsm->inode, _inodeNum, _fsm->diskHandle);

	byte = _inodeNum / 8;
	bit = _inodeNum % 8;
	_fsm->index[0] = byte;
	_fsm->index[1] = bit;
	_fsm->contInodes = 1;

	deallocateInode(_fsm);

	// Remove the file from its containing directory
	rmFileFromDir(_fsm, _inodeNum, _inodeNumD);

	// Succeeded so return true
	return True;

} /*end rmFile(FileSectorMgr *_fsm, unsigned int _inodeNum,
							   unsigned int _inodeNumD)*/

/********************* def beg rmFile_T_IndirectBlocks **********************

   void rmFile_T_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType,

		unsigned int _inodeNumD, unsigned int _diskOffset)

	Function:
			Removes Triple Indirect pointers from a file

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			unsigned int _fileType
			:= holds the type of file

		unsigned int _inodeNumD
		:= inode number of directory

		unsigned int _diskOffset
		:= offset to first usable block on disk

		Local Variables
		unsigned int indirectBlock
		:= holds information of the indirect block

		unsigned int diskOffset
		:= Offset to data to be removed

		unsigned int sectorNumber
		:= sector number of data to be removed

		unsigned int byte
		:= byte number of data to be removed

		unsigned int bit
		:= bit number of data to be removed

	void rmFile_T_IndirectBlocks
	   = Does not return a value

	Change Record: 4/12/10 first implemented

********************def end rmFile_T_IndirectBlocks *************************/
void rmFile_T_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType,
							 unsigned int _inodeNumD, unsigned int _diskOffset)
{

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

	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];
			rmFile_D_IndirectBlocks(_fsm, _fileType, _inodeNumD, diskOffset);

		} // if (indirectBlock[i] != (unsigned int)(-1))

	} // for (i = 0; i < BLOCK_SIZE/4; i++)

	// Deallocate T indirect block
	diskOffset = _diskOffset;
	sectorNumber = diskOffset / BLOCK_SIZE;
	byte = sectorNumber / 8;
	bit = sectorNumber % 8;

	_fsm->ssm->contSectors = 1;
	_fsm->ssm->index[0] = byte;
	_fsm->ssm->index[1] = bit;

	// Deallocate newly freed sectors
	deallocateSectors(_fsm->ssm);

} /*end rmFile_T_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType,
					 unsigned int _inodeNumD, unsigned int _diskOffset)*/

/********************* def beg rmFile_D_IndirectBlocks **********************

   void rmFile_D_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType,

		unsigned int _inodeNumD, unsigned int _diskOffset)

	Function:
			Removes Double Indirect pointers from a file

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			unsigned int _fileType
			:= holds the type of file

		unsigned int _inodeNumD
		:= inode number of directory

		unsigned int _diskOffset
		:= offset to first usable block on disk

		Local Variables
		unsigned int indirectBlock
		:= holds information of the indirect block

		unsigned int diskOffset
		:= Offset to data to be removed

		unsigned int sectorNumber
		:= sector number of data to be removed

		unsigned int byte
		:= byte number of data to be removed

		unsigned int bit
		:= bit number of data to be removed

	void rmFile_D_IndirectBlocks
	   = Does not return a value

	Change Record: 4/12/10 first implemented

********************def end rmFile_D_IndirectBlocks *************************/
void rmFile_D_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType,
							 unsigned int _inodeNumD, unsigned int _diskOffset)
{

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

	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];
			rmFile_S_IndirectBlocks(_fsm, _fileType, _inodeNumD, diskOffset);

		} // end if (indirectBlock[i] != (unsigned int)(-1))

	} // end for (i = 0; i < BLOCK_SIZE/4; i++)

	// Deallocate D indirect block
	diskOffset = _diskOffset;
	sectorNumber = diskOffset / BLOCK_SIZE;
	byte = sectorNumber / 8;
	bit = sectorNumber % 8;

	_fsm->ssm->contSectors = 1;
	_fsm->ssm->index[0] = byte;
	_fsm->ssm->index[1] = bit;

	// Deallocate newly freed sectors
	deallocateSectors(_fsm->ssm);

} /* end rmFile_D_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType,
				unsigned int _inodeNumD, unsigned int _diskOffset){*/

/********************* def beg rmFile_S_IndirectBlocks **********************

   void rmFile_S_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType,
		unsigned int _inodeNumD, unsigned int _diskOffset)

	Function:
			Removes Single Indirect pointers from a file

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			unsigned int _fileType
			:= holds the type of file

		unsigned int _inodeNumD
		:= inode number of directory

		unsigned int _diskOffset
		:= offset to first usable block on disk

		Local Variables
		unsigned int indirectBlock
		:= holds information of the indirect block

		unsigned int diskOffset
		:= Offset to data to be removed

		unsigned int sectorNumber
		:= sector number of data to be removed

		unsigned int byte
		:= byte number of data to be removed

		unsigned int bit
		:= bit number of data to be removed

	void rmFile_S_IndirectBlocks
	   = Does not return a value

	Change Record: 4/12/10 first implemented

********************def end rmFile_S_IndirectBlocks *************************/
void rmFile_S_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType,
							 unsigned int _inodeNumD, unsigned int _diskOffset)
{

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

	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];

			// Deallocate Data Blocks
			if (_fileType == 2)
			{

				fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
				_fsm->sampleCount = fread(buffer, sizeof(unsigned int),
										  BLOCK_SIZE / 4, _fsm->diskHandle);

				for (j = 8; j < BLOCK_SIZE / 4; j += 4)
				{

					if (buffer[j + 3] == 1)
					{
						rmFile(_fsm, buffer[j + 2], _inodeNumD);
					} // end if (buffer[j+3] == 1)

				} // end for (j = 8; j < BLOCK_SIZE/4; j += 4)

			} // end if (_fileType == 2)

			sectorNumber = diskOffset / BLOCK_SIZE;
			byte = sectorNumber / 8;
			bit = sectorNumber % 8;

			_fsm->ssm->contSectors = 1;
			_fsm->ssm->index[0] = byte;
			_fsm->ssm->index[1] = bit;
			deallocateSectors(_fsm->ssm);

		} // end if (indirectBlock[i] != (unsigned int)(-1))

	} // end for (i = 0; i < BLOCK_SIZE/4; i++)

	// Deallocate S indirect block
	diskOffset = _diskOffset;
	sectorNumber = diskOffset / BLOCK_SIZE;
	byte = sectorNumber / 8;
	bit = sectorNumber % 8;

	_fsm->ssm->contSectors = 1;
	_fsm->ssm->index[0] = byte;
	_fsm->ssm->index[1] = bit;
	// Deallocate newly freed sectors
	deallocateSectors(_fsm->ssm);
} /*end rmFile_S_IndirectBlocks(FileSectorMgr *_fsm, unsigned int _fileType,
				unsigned int _inodeNumD, unsigned int _diskOffset){*/

/********************* def beg renameFile **********************

   Bool renameFile(FileSectorMgr *_fsm, unsigned int _inodeNumF,
	unsigned int *_name, unsigned int _inodeNumD)

	Function:
			Renames a file

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			unsigned int _inodeNumF
			:= Inode number of file

		unsigned int *_name
		:= Name to rename file to

		unsigned int _inodeNumD
		:= inode number of directory

		Local Variables
		Bool success
		:= Holds whether operations were successful

		unsigned int i,j
		:= loop variables

		unsigned int buffer
		:= buffer to hold data

		unsigned int diskOffset
		:= Offset to data to be removed

	void rmFile_S_IndirectBlocks
	   = Does not return a value

	Change Record: 4/12/10 first implemented

********************def end renameFile *************************/
Bool renameFile(FileSectorMgr *_fsm, unsigned int _inodeNumF,
				unsigned int *_name, unsigned int _inodeNumD)
{

	Bool success;

	unsigned int i, j;

	unsigned int diskOffset;

	unsigned int buffer[BLOCK_SIZE / 4];

	// Attempt to open directory
	success = openFile(_fsm, _inodeNumD);

	if (success == True)
	{

		if (_fsm->inode.fileType == 2)
		{

			for (i = 0; i < 10; i++)
			{

				if (_fsm->inode.directPtr[i] != (unsigned int)(-1))
				{

					diskOffset = _fsm->inode.directPtr[i];

					fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
					_fsm->sampleCount = fread(buffer, sizeof(unsigned int),
											  BLOCK_SIZE / 4, _fsm->diskHandle);

					for (j = 0; j < BLOCK_SIZE / 4; j += 4)
					{

						if (buffer[j + 3] == 1 && buffer[j + 2] == _inodeNumF)
						{

							buffer[j] = _name[0];
							buffer[j + 1] = _name[1];

							fseek(_fsm->diskHandle, 0, SEEK_SET);
							fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
							_fsm->sampleCount = fwrite(buffer, sizeof(unsigned int),
													   BLOCK_SIZE / 4, _fsm->diskHandle);

							return True;

						} // end if (buffer[j+3] == 1 && buffer[j+2] == _inodeNumF)

					} // end for (j = 0; j < BLOCK_SIZE/4; j += 4)

				} // end if (_fsm->inode.directPtr[i] != (unsigned int)(-1))

			} // end for (i = 0; i < 10; i++)

			// Rename all the Single Indirect blocks
			if (_fsm->inode.sIndirect != (unsigned int)(-1))
			{

				success = renameFileIn_S_Indirect(_fsm, _inodeNumF, _name,
												  _fsm->inode.sIndirect);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.sIndirect != (unsigned int)(-1))

			// Rename all Double Indirect block references
			if (_fsm->inode.dIndirect != (unsigned int)(-1))
			{

				success = renameFileIn_D_Indirect(_fsm, _inodeNumF, _name,
												  _fsm->inode.dIndirect);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.dIndirect != (unsigned int)(-1))

			// Rename all Triple Indirect block references
			if (_fsm->inode.tIndirect != (unsigned int)(-1))
			{

				success = renameFileIn_T_Indirect(_fsm, _inodeNumF, _name,
												  _fsm->inode.tIndirect);

				if (success == True)
				{
					return True;
				} // end if (success == True)

			} // end if (_fsm->inode.tIndirect != (unsigned int)(-1))

		} // end if (_fsm->inode.fileType == 2)

	} // end if (success == True)

	// Returns false if none of the renames successful
	return False;

} /* end Bool renameFile(FileSectorMgr *_fsm, unsigned int _inodeNumF,
				  unsigned int *_name, unsigned int _inodeNumD)*/

/********************* def beg renameFileIn_T_Indirect **********************

   Bool renameFileIn_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
	 unsigned int *_name, unsigned int _tIndirectOffset)

	Function:
			Renames a file

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			unsigned int _inodeNumF
			:= Inode number of file

		unsigned int *_name
		:= Name to rename file to

		unsigned int _tIndirectOffset
		:= offset of triple indirect pointer

		Local Variables
		Bool success
		:= Holds whether operations were successful

		unsigned int diskOffset
		:= Offset to data to be removed

		unsigned int indirectBlock
		:= holds the block containing triple indirect pointer

	Bool renameFileIn_T_Indirect
	   = Returns True if rename successful, false otherwise

	Change Record: 4/12/10 first implemented

********************def end renameFileIn_T_Indirect *************************/
Bool renameFileIn_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
							 unsigned int *_name, unsigned int _tIndirectOffset)
{

	unsigned int indirectBlock[BLOCK_SIZE / 4];

	unsigned int diskOffset;

	Bool success;

	diskOffset = _tIndirectOffset;

	// Read disk to indirectBlock
	fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

	_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);

	unsigned int i;

	// Loop through indirect block, follow the Double indirect pointers
	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];

			success = renameFileIn_D_Indirect(_fsm, _inodeNumF, _name, diskOffset);

			if (success == True)
			{
				return True;
			} // end if (success == True)

		} // end if (indirectBlock[i] != (unsigned int)(-1))

	} // end for (i = 0; i < BLOCK_SIZE/4; i++)

	// Return false if not successful in renaming
	return False;

} /*end renameFileIn_T_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
				unsigned int *_name, unsigned int _tIndirectOffset)*/

/********************* def beg renameFileIn_D_Indirect **********************

   Bool renameFileIn_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
	 unsigned int *_name, unsigned int _dIndirectOffset)

	Function:
			Renames a file

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			unsigned int _inodeNumF
			:= Inode number of file

		unsigned int *_name
		:= Name to rename file to

		unsigned int _dIndirectOffset
		:= offset of double indirect pointer

		Local Variables
		Bool success
		:= Holds whether operations were successful

		unsigned int diskOffset
		:= Offset to data to be removed

		unsigned int indirectBlock
		:= holds the block containing triple indirect pointer

	Bool renameFileIn_D_Indirect
		:= Returns true if rename successful, false otherwise

	Change Record: 4/12/10 first implemented

********************def end renameFileIn_D_Indirect *************************/
Bool renameFileIn_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
							 unsigned int *_name, unsigned int _dIndirectOffset)
{

	unsigned int indirectBlock[BLOCK_SIZE / 4];

	unsigned int diskOffset;

	Bool success;

	diskOffset = _dIndirectOffset;

	// Read disk to indirectBlock
	fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

	_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);

	unsigned int i;

	// Loop through indirect block, follow the Single indirect pointers
	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];

			success = renameFileIn_S_Indirect(_fsm, _inodeNumF, _name, diskOffset);

			if (success == True)
			{
				return True;
			} // end if (success == True)

		} // end if (indirectBlock[i] != (unsigned int)(-1))

	} // end for (i = 0; i < BLOCK_SIZE/4; i++)

	// Return false if renaming didn't work
	return False;

} /*end renameFileIn_D_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
				unsigned int *_name, unsigned int _dIndirectOffset)*/

/********************* def beg renameFileIn_S_Indirect **********************

   Bool renameFileIn_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
	 unsigned int *_name, unsigned int _sIndirectOffset)

	Function:
			Renames a file

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

			unsigned int _inodeNumF
			:= Inode number of file

		unsigned int *_name
		:= Name to rename file to

		unsigned int _sIndirectOffset
		:= offset of single indirect pointer

		Local Variables
		Bool success
		:= Holds whether operations were successful

		unsigned int diskOffset
		:= Offset to data to be removed

		unsigned int indirectBlock
		:= holds the block containing triple indirect pointer

	Bool renameFileIn_S_Indirect
		:= Returns true if rename successful, false otherwise

	Change Record: 4/12/10 first implemented

********************def end renameFileIn_S_Indirect *************************/
Bool renameFileIn_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
							 unsigned int *_name, unsigned int _sIndirectOffset)
{

	unsigned int indirectBlock[BLOCK_SIZE / 4];

	unsigned int diskOffset;

	unsigned int buffer[BLOCK_SIZE / 4];

	diskOffset = _sIndirectOffset;

	// Read disk to indirectBlock
	fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

	_fsm->sampleCount = fread(indirectBlock, BLOCK_SIZE, 1, _fsm->diskHandle);

	unsigned int i, j;

	// Loop through indirect blocks
	for (i = 0; i < BLOCK_SIZE / 4; i++)
	{

		if (indirectBlock[i] != (unsigned int)(-1))
		{

			diskOffset = indirectBlock[i];

			fseek(_fsm->diskHandle, diskOffset, SEEK_SET);

			_fsm->sampleCount = fread(buffer, BLOCK_SIZE, 1, _fsm->diskHandle);

			// Go through buffer, find name portion
			for (j = 0; j < BLOCK_SIZE / 4; j += 4)
			{

				if (buffer[j + 3] == 1 && buffer[j + 2] == _inodeNumF)
				{

					// Write name to data
					buffer[j] = _name[0];
					buffer[j + 1] = _name[1];

					fseek(_fsm->diskHandle, 0, SEEK_SET);
					fseek(_fsm->diskHandle, diskOffset, SEEK_SET);
					_fsm->sampleCount = fwrite(buffer, sizeof(unsigned int),
											   BLOCK_SIZE / 4, _fsm->diskHandle);

					return True;

				} // end if (buffer[j+3] == 1 && buffer[j+2] == _inodeNumF)

			} // end for (j = 0; j < BLOCK_SIZE/4; j += 4)

		} // end if (indirectBlock[i] != (unsigned int)(-1))

	} // end for (i = 0; i < BLOCK_SIZE/4; i++)

	// Return false if renaming not successful
	return False;

} /*end renameFileIn_S_Indirect(FileSectorMgr *_fsm, unsigned int _inodeNumF,
				unsigned int *_name, unsigned int _sIndirectOffset)*/

/*********************** def beg mkfs ***************************************

   void mkfs(FileSectorMgr *_fsm)

	Function:
			creates the entire file system:
			Allocates boot and superblocks
			allocates and initializes all inode blocks.
			allocates inodes for boot, super and root

	Data Environment:
		Function Parameters
			FileSectorMgr *_fsm
			:= pointer to the FileSectorMgr struct

		Local Variables
			int i
			:= variable used for looping

			int factorsOf_32
			:= number of groups of 32 inodes to allocate

			int remainder
			:= number of inodes to allocate less than 32

			bool success
			:= variable used to check addFileToDir success or failure

			unsigned int name[2]
			:= 2-tuple used to store name

			unsigned int inodeNumF
			:= the inode number of the file to be added

		Struct Variables (from FileSectorMgr struct)
			FILE *diskHandle
			:= pointer to the hard disk

			unsigned int diskOffset
			:= offset from inode area to data area of disk

			_fsm->ssm
			:= used to allocate sectors on disk

	void mkfs
	   = returns void for all calls to this function

	Change Record: 4/12/10 first implemented

*************************** def end mkfs ******************************/
void mkfs(FileSectorMgr *_fsm, unsigned int _DISK_SIZE, unsigned int _BLOCK_SIZE,
		  unsigned int _INODE_SIZE, unsigned int _INODE_BLOCKS, unsigned int _INODE_COUNT,
		  int _initSsmMaps)
{

	initializeFsmConstants(_DISK_SIZE, _BLOCK_SIZE, _INODE_SIZE, _INODE_BLOCKS,
						   _INODE_COUNT);

	initFsmMaps(_fsm);

	initFileSectorMgr(_fsm, _initSsmMaps);

	int i;

	// Allocate the boot and super block sectors on disk
	getSector(2, _fsm->ssm);

	allocateSectors(_fsm->ssm);

	// Create INODE_COUNT Inodes
	int factorsOf_32 = INODE_BLOCKS / 32;

	int remainder = INODE_BLOCKS % 32;

	// get 32 sectors at a time and make them inode sectors
	for (i = 0; i < factorsOf_32; i++)
	{

		getSector(32, _fsm->ssm);

		_fsm->diskOffset = BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));

		// take the 32 sectors and make inodes
		makeInodes(32, _fsm->diskHandle, _fsm->diskOffset);
		allocateSectors(_fsm->ssm);

	} // end for (i = 0; i < factorsOf_32; i++)

	getSector(remainder, _fsm->ssm);

	_fsm->diskOffset = BLOCK_SIZE * ((8 * _fsm->ssm->index[0]) + (_fsm->ssm->index[1]));

	// take the remaining sectors and make inodes
	makeInodes(remainder, _fsm->diskHandle, _fsm->diskOffset);

	allocateSectors(_fsm->ssm);

	unsigned int name[2];

	// Set inode 0 for boot sector

	createFile(_fsm, 0, name, (unsigned int)(-1));

	openFile(_fsm, 0);

	_fsm->inode.directPtr[0] = 0;

	writeInode(&_fsm->inode, 0, _fsm->diskHandle);

	// Set inode 1 for super block
	createFile(_fsm, 0, name, (unsigned int)(-1));

	openFile(_fsm, 1);

	_fsm->inode.directPtr[0] = BLOCK_SIZE * (1);

	writeInode(&_fsm->inode, 1, _fsm->diskHandle);

	// make root directory with inode 2
	createFile(_fsm, 1, name, (unsigned int)(-1));

} /*end mkfs(FileSectorMgr *_fsm,unsigned int _DISK_SIZE,
 unsigned int _BLOCK_SIZE, unsigned int _INODE_SIZE, unsigned int _INODE_BLOCKS,
				   unsigned int _INODE_COUNT, int _initSsmMaps)*/

/**************************** def beg allocateInode *************************

	Bool allocateInode (FileSectorMgr *_fsm)

	Function
		Initializes the File Sector Manager

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

		Local Variables
			int n
			:= Holds number of contiguous inodes

			int byte
			:= holds the byte number

			int bit
			:= holds the bit number

	allocateInode
		= Returns true if allocation successful, false otherwise


	Change record: 4/12/10 first implemented

***************************** def end allocateInode ************************/
Bool allocateInode(FileSectorMgr *_fsm)
{

	if (_fsm->index[0] != (unsigned int)(-1))
	{

		int n;
		int byte;
		int bit;

		bit = _fsm->index[1];
		byte = _fsm->index[0];
		n = _fsm->contInodes;

		// Find contiguous sectors
		while (n > 0)
		{

			for (; bit < 8; bit++)
			{
				_fsm->iMap[byte] -= pow(2, bit);

				n--;

				if (n == 0)
				{
					break;
				} // end if (n == 0)

			} // end for (; bit < 8; bit++)

			byte++;
			bit = 0;

		} // end while (n > 0)

	} // end if (_fsm->index[0] != (unsigned int)(-1))

	_fsm->index[0] = (unsigned int)(-1);
	_fsm->index[1] = (unsigned int)(-1);
	_fsm->contInodes = 0;

	// Write the newly allocated inode to the iMapHandler
	char dbfile[256];

	_fsm->iMapHandle = Null;

	sprintf(dbfile, "./iMap");
	_fsm->iMapHandle = fopen(dbfile, "r+");

	_fsm->sampleCount = fwrite(_fsm->iMap, 1, INODE_BLOCKS, _fsm->iMapHandle);

	fclose(_fsm->iMapHandle);

	return True;

} // end Bool allocateInode (FileSectorMgr *_fsm)

/**************************** def beg deallocateInode *************************

	void initFileSectorMgr(FileSectorMgr *_fsm, int _initSsmMaps)

	Function
		Deallocates inode

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _initSsmMaps
			:= variable that tells program whether to initialize
			the SSM maps

		Local Variables
			int n
			:= number of contiguous sectors

			int byte
			:= byte number

			int bit
			:= bit number

	deallocateInode
		= Returns true if deallocation successful, false otherwise


	Change record: 4/12/10 first implemented

***************************** def end deallocateInode ************************/
Bool deallocateInode(FileSectorMgr *_fsm)
{

	// Deallocate iMap at Inode's location
	if (_fsm->index[0] != (unsigned int)(-1))
	{

		int n;
		int byte;
		int bit;

		bit = _fsm->index[1];
		byte = _fsm->index[0];
		n = _fsm->contInodes;

		while (n > 0)
		{

			for (; bit < 8; bit++)
			{
				_fsm->iMap[byte] += pow(2, bit);

				n--;

				if (n == 0)
				{
					break;
				} // end if (n == 0)

			} // end for (; bit < 8; bit++)

			byte++;
			bit = 0;

		} // end while (n > 0)

	} // end if (_fsm->index[0] != (unsigned int)(-1))

	_fsm->index[0] = (unsigned int)(-1);
	_fsm->index[1] = (unsigned int)(-1);
	_fsm->contInodes = 0;

	// Write iMap to its Handler
	char dbfile[256];

	_fsm->iMapHandle = Null;

	sprintf(dbfile, "./iMap");
	_fsm->iMapHandle = fopen(dbfile, "r+");

	_fsm->sampleCount = fwrite(_fsm->iMap, 1, INODE_BLOCKS, _fsm->iMapHandle);

	fclose(_fsm->iMapHandle);

	return True;

} // end Bool deallocateInode (FileSectorMgr *_fsm)

/**************************** def beg getInode *************************

	Bool getInode (int _n, FileSectorMgr *_fsm)

	Function
		Gets an inode

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _n
			:= Number of contiguous inodes to get

		Local Variables
			int i
			:= loop variables

			int mask
			:= mask for bit wise compare operations

			int result
			:= holds the result of applying the AND operation to
			the bitmaps

			int buff
			:= inode searching buffer

			char *iMap
			:= inode map pointer

			long long int subsetMap
			:= inode map

			int byteCount
			:= variable to loop through inodes

			Bool found
			:= Holds whether an inode was found

	getInode
		= Returns true if inode is retrieved, false otherwise


	Change record: 4/12/10 first implemented

***************************** def end getInode ************************/
Bool getInode(int _n, FileSectorMgr *_fsm)
{

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
	if (_n < 33 && _n > 0)
	{

		mask = 0;

		for (i = _n - 1; i > -1; i--)
		{
			mask += (unsigned int)pow(2, i);
		} // end for (i = _n - 1; i > -1; i--)

		// initially assume false
		found = False;
		result = mask;
		buff = mask;
		subsetMap[0] = 0;
		byteCount = 0;
		iMap = _fsm->iMap;

		_fsm->index[0] = -1;
		_fsm->index[1] = -1;

		while (byteCount < INODE_BLOCKS)
		{

			subsetMap[0] = *((unsigned long long int *)iMap);

			for (i = 0; i < 32; i++)
			{

				buff = (unsigned int)subsetMap[0];

				result = mask & buff;

				if (result == mask)
				{
					found = True;
					break;
				} // end if (result == mask)
				else
				{
					subsetMap[0] >>= 1;

				} // end else

			} // end for (i = 0; i < 32; i++)

			// Assign found inode to index
			if (found == True)
			{

				_fsm->index[0] = byteCount + (i / 8);
				_fsm->index[1] = i % 8;
				return True; // break;

			} // end if (found == True)

			iMap += 4;
			byteCount += 4;

		} // end while (byteCount < INODE_BLOCKS)

	} // end if (_n < 33 && _n > 0)

	return False;

} // end getInode (int _n, FileSectorMgr *_fsm)

/**************************** def beg fsmPrint *************************

	void fsmPrint(FileSectorMgr *_fsm, int _case, unsigned int _startByte)

	Function
		Prints debug messages

	Data Environment
		Parameters
			FileSectorMgr *_fsm
			:= pointer to the fsm

			int _case
			:= the debug statement to print

			int _startByte
			:= the byte to start at

		Local Variables
			int i, j, k
			:= loop variables

			int count
			:= count of a value

			int sector
			:= current sector

			char tmp
			:= temporary character

			char byteArray
			:= Array to temporary hold a byte

	fsmPrint
		= returns void for all calls to this function


	Change record: 4/12/10 first implemented

***************************** def end fsmPrint ************************/
void fsmPrint(FileSectorMgr *_fsm, int _case, unsigned int _startByte)
{

	unsigned int i, j, k;

	int count;

	int sector;

	unsigned char tmp;

	char byteArray[8];

	switch (_case)
	{

	// Print Initialization Message
	case 0:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Initializing FSM maps...\n");
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print Inode Map
	case 1:
		printf("DEBUG_LEVEL > 0:\n");
		printf("//Print 128 contiguous Inodes from Inode Map ");
		printf("starting at Inode (%d).\n", _startByte * 8);
		printf("//P:%d\n\n", _startByte);
		printf("===================================");
		printf("====================================\n\n");
		printf("INODE MAP\n");

		k = _startByte;

		count = 0;

		for (i = _startByte; i < INODE_BLOCKS &&
							 i < _startByte + 16;
			 i++)
		{

			tmp = _fsm->iMap[i];

			for (j = 0; j < 8; j++)
			{
				if (tmp % 2 == 1)
					byteArray[j] = '1';
				else
					byteArray[j] = '0';
				tmp /= 2;
			} // end for (j = 0; j < 8; j++)

			for (j = 0; j < 8; j++)
				printf("%c", byteArray[j]);

			printf(" ");

			if ((count + 1) % 8 == 0 || i + 1 == INODE_BLOCKS)
			{

				printf("\n");

				for (j = 0; j < 8; j++)
				{
					if (k * 8 < 10)
						printf("%d        ", k * 8);
					else if (k * 8 < 100)
						printf("%d       ", k * 8);
					else if (k * 8 < 1000)
						printf("%d      ", k * 8);
					else if (k * 8 < 10000)
						printf("%d     ", k * 8);
					else if (k * 8 < 100000)
						printf("%d    ", k * 8);
					else if (k * 8 < 1000000)
						printf("%d   ", k * 8);
					else if (k * 8 < 10000000)
						printf("%d  ", k * 8);
					else if (k * 8 < 100000000)
						printf("%d ", k * 8);
					k++;

				} // end for (j = 0; j < 8; j++)

				printf("\n");

			} // end if(count+1) % 8 = 0 | i+1 = INODE_BLOCKS)

			count++;

		} /*for (i = _startByte; i < INODE_BLOCKS &&
						  i < _startByte + 16; i++)*/

		printf("\n");
		printf("===================================");
		printf("====================================\n\n");
		break;
	case 2:
		printf("DEBUG_LEVEL > 0:\n");
		sector = 8 * _fsm->index[0] + _fsm->index[1];
		printf("//Allocate %d contiguous inodes.\n",
			   _fsm->contInodes);
		printf("//A:%d\n\n", _fsm->contInodes);
		printf("Allocating %d contiguous inodes at sector %d\n",
			   _fsm->contInodes, sector);
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print Initializing FSM message
	case 3:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Initializing FSM...\n");
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print Allocate Failure method
	case 4:
		printf("DEBUG_LEVEL > 0:\n");

		for (k = 0; k < MAX_INPUT; k++)
		{

			if (_fsm->badInode[k][0] == (unsigned int)(-1))
			{
				break;
			} // end if(_fsm->badInode[k][0] = (unsigned int)

			sector = 8 * _fsm->badInode[k][0] + _fsm->badInode[k][1];
			printf("Failed to allocate inodes at ");
			printf("sector %d\n", sector);

		} // end for (k = 0; k < MAX_INPUT; k++)

		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 5:
		break;

	// Print deallocation message
	case 6:
		printf("DEBUG_LEVEL > 0:\n");
		sector = 8 * _fsm->index[0] + _fsm->index[1];
		printf("//Deallocate ");
		printf("%d contiguous inodes starting at sector %d\n",
			   _fsm->contInodes, _fsm->index[0] * 8 + _fsm->index[1]);
		printf("//D:%d:%d:%d\n\n", _fsm->contInodes,
			   _fsm->index[0], _fsm->index[1]);
		printf("Deallocating %d contiguous inodes at sector %d",
			   _fsm->contInodes, sector);
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print deallocation failure message
	case 7:
		printf("DEBUG_LEVEL > 0:\n");

		for (k = 0; k < MAX_INPUT; k++)
		{

			if (_fsm->badInode[k][0] == (unsigned int)(-1))
			{
				break;
			} // end if (_fsm->badInode[k][0] = (unsigned int)

			sector = 8 * _fsm->badInode[k][0] + _fsm->badInode[k][1];
			printf("Failed to deallocate ");
			printf("inode %d\n", sector);

		} // end for (k = 0; k < MAX_INPUT; k++)

		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print integrity check failure message
	case 8:
		printf("DEBUG_LEVEL > 0:\n");

		for (k = 0; k < MAX_INPUT; k++)
		{

			if (_fsm->badInode[k][0] == (unsigned int)(-1))
			{
				break;
			} // end if (_fsm->badInode[k][0] = (unsigned int)

			sector = 8 * _fsm->badInode[k][0] + _fsm->badInode[k][1];
			printf("Failed integrity check at sector ");
			printf("%d\n", sector);

		} // end for (k = 0; k < MAX_INPUT; k++)

		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		printf("\n");
		break;

	// Print integrity check pass message
	case 9:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Passed integrity check.\n");
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print integrity check message
	case 10:
		printf("DEBUG_LEVEL > 0:\n");
		printf("//Check for integrity.\n");
		printf("//I\n\n");
		printf("Checking integrity of inode map...\n");
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 11:
		break;

	// Print Set inode sector message
	case 12:
		printf("DEBUG_LEVEL > 0:\n");
		sector = 8 * _fsm->index[0] + _fsm->index[1];
		printf("//Set inode map sector (%d*8 + %d).\n",
			   _fsm->index[0], _fsm->index[1]);
		printf("//X:%d:%d\n\n", _fsm->index[0], _fsm->index[1]);
		printf("Setting inode map sector %d\n", sector);
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 13:
		break;
	case 14:
		printf("\nEND");
		break;

	// Print bad input message
	case 15:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Bad input...\n");
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print unable to find contiguous inodes message
	case 16:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Could not find %d contiguous inodes.\n",
			   _fsm->contInodes);
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print read input message
	case 17:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Reading input file...\n");
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print create stub output message
	case 18:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Creating stub output...\n");
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print variable information
	case 19:
		printf("DEBUG_LEVEL > 1:\n");
		printf("Variable information:\n");
		printf("contInodes = %d\n", _fsm->contInodes);
		printf("index[0] = %d\n", _fsm->index[0]);
		printf("index[1] = %d\n", _fsm->index[1]);
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print getting inodes message
	case 20:
		printf("DEBUG_LEVEL > 0:\n");
		sector = 8 * _fsm->index[0] + _fsm->index[1];
		printf("//Get %d contiguous inodes\n", _fsm->contInodes);
		printf("//G:%d\n\n", _fsm->contInodes);
		printf("There are %d contiguous inodes at sector %d.\n",
			   _fsm->contInodes, sector);
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print making file system message
	case 21:
		printf("DEBUG_LEVEL > 0:\n");
		printf("//Making the File System.\n");
		printf("//Disk = 3000000, Block = 1024, Inode = 128, ");
		printf("Inode Block = 32 blocks...\n");
		printf("//M\n\n");
		printf("-> Allocating Boot Block, Super Block, 32 ");
		printf("Inode Blocks, and Root (Inode 2)...\n");
		printf("** Expected Result: 3 Inodes allocated in the");
		printf(" Inode Map\n");
		printf("** Expected Result: 35 Blocks allocated in ");
		printf("the Aloc/Free Map\n");
		printf("- - - - - - - - - - - - - - - - - - - - - - -");
		printf(" - - - - - - - - - - - - -\n\n\n");
		break;

	// Print created a file
	case 22:
		printf("DEBUG_LEVEL > 0:\n");
		printf("//Create a file\n");
		printf("//C:0\n\n");
		printf("Created a file...\n");
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print opened file
	case 23:
		printf("DEBUG_LEVEL > 0:\n");
		printf("//Open a file\n");
		printf("//O:%d\n\n", _fsm->inodeNum);
		printf("Openned file at inode %d...\n", _fsm->inodeNum);
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print write to file
	case 24:
		printf("DEBUG_LEVEL > 0:\n");
		printf("//Write to file\n");
		printf("//W:%d\n\n", _fsm->inodeNum);
		printf("Wrote to file at inode %d...\n", _fsm->inodeNum);
		break;

	// Print read from file
	case 25:
		printf("DEBUG_LEVEL > 0:\n");
		printf("//Read from file\n");
		printf("//R:%d\n\n", _fsm->inodeNum);
		printf("Read from file at inode %d..\n", _fsm->inodeNum);
		break;

	// Print create directory
	case 26:
		printf("DEBUG_LEVEL > 0:\n");
		printf("//Create a directory\n");
		printf("//C:1\n\n");
		printf("Created a directory...\n");
		printf("- - - - - - - - - - - - - - - - - - ");
		printf("- - - - - - - - - - - - - - - - - -\n\n");
		break;

	// Print testing root
	case 27:
		printf("DEBUG_LEVEL > 0:\n");
		printf("//Testing root directory\n");
		printf("//T:2\n\n");
		break;

	case 28:
		printf("DEBUG_LEVEL > 0:\n");
		printf("//Listing root directory\n");
		printf("//L:2\n\n");
		break;

	case 29:
		printf("DEBUG_LEVEL > 0:\n");
		printf("//Display Info of File (Inode %d)\n",
			   _fsm->inodeNum);
		printf("//I:%d\n\n", _fsm->inodeNum);

		if (_fsm->inode.fileType == 1)
		{
			printf("-> fileType = FILE\n");
		} // end if (_fsm->inode.fileType == 1)
		else if (_fsm->inode.fileType == 2)
		{
			printf("-> fileType = DIRECTORY\n");
		} // end else

		printf("-> fileSize = %d\n", _fsm->inode.fileSize);
		printf("-> permissions = %d\n", _fsm->inode.permissions);
		printf("-> linkCount = %d\n", _fsm->inode.linkCount);
		printf("-> dataBlocks = %d\n", _fsm->inode.dataBlocks);
		printf("-> owner = %d\n", _fsm->inode.owner);
		printf("-> status = %d\n\n", _fsm->inode.status);

		for (i = 0; i < 10; i++)
		{

			if (_fsm->inode.directPtr[i] == (unsigned int)(-1))
			{
				printf("-> directPtr[%d] = %d\n",
					   i, _fsm->inode.directPtr[i]);
			} // end if(_fsm->inode.directPtr[i] = (unsigned int))
			else
			{
				printf("-> directPtr[%d] = %d\n",
					   i, _fsm->inode.directPtr[i] / BLOCK_SIZE);
			} // end else

		} // end for (i = 0; i < 10; i++)

		if (_fsm->inode.sIndirect == (unsigned int)(-1))
		{
			printf("\n-> sIndirect = %d\n",
				   _fsm->inode.sIndirect);
		} // end if if(_fsm->inode.sIndirect == (unsigned int)(-1)
		else
		{
			printf("\n-> sIndirect = %d\n",
				   _fsm->inode.sIndirect / BLOCK_SIZE);
		} // end elsee

		if (_fsm->inode.dIndirect == (unsigned int)(-1))
		{
			printf("-> dIndirect = %d\n", _fsm->inode.dIndirect);
		} // end if (_fsm->inode.dIndirect == (unsigned int)(-1))
		else
		{
			printf("-> dIndirect = %d\n",
				   _fsm->inode.dIndirect / BLOCK_SIZE);
		} // end else

		if (_fsm->inode.tIndirect == (unsigned int)(-1))
		{
			printf("-> tIndirect = %d\n", _fsm->inode.tIndirect);
		} // end if (_fsm->inode.tIndirect == (unsigned int)(-1))
		else
		{
			printf("-> tIndirect = %d\n",
				   _fsm->inode.tIndirect / BLOCK_SIZE);
		} // end else

		printf("- - - - - - - - - - - - - - - - - - - - - - -");
		printf(" - - - - - - - - - - - - -\n\n\n");
		break;

	} // end switch (_case)

} // end void fsmPrint(FileSectorMgr *_fsm, int _case, int _startByte)
