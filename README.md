# azulejo-os-filesystem

A Linux Filesystem implementing a File Sector Manager and Sector Space Manager in the user space.

## Testing

Compile and run the driver application (`fsm`) by running `qaStart.sh`. The driver application loads `test/qaInput.txt` which will create a populated file system in the file called `fs/hardDisk` and dump logs to `test/qaOutput.txt`.

## Building

Run `make`.

## Cleaning

Run `make clean`.

## Introduction

A Unix file can be a directory (containing other files and directories); or a data/binary file (binary bits: text, binary data, image, executable, symbolic link, device special, named pipes, etc.)

A file has exactly one inode, but can have many physical disk blocks (disk sectors).

### The physical disk is segmented into disk blocks.

- Disk block 0 is the boot block or boot sector. Unix treats the boot block as a file, with an inode (inode 0) associated with it.
- Disk block 1 is the superblock (also called the filesystem header), with information about the filesystem (e.g. disk block size, inode size, number of inodes for the filesystem, etc). Unix treats the superblock as a file, with an inode (inode 1) associated with it.
- Disk block 2 through disk block N make up the inode-block.
- The remaining disk blocks (N+1 through the last block) are data blocks available for use.

### The inode block has the inodes, numbered 0, 1, 2, ..., with following structure.

- Inode 0 is the inode for the boot block file.
- Inode 1 is the inode for the superblock file.
- Inode 2 is the inode for the root directory (/) file.

### An inode for a file has the following structure:

#### File information:

- file type (directory, link ...)
- owner
- length
- creation date
- number of links,
- etc.

#### 13 disk block pointers:

- 10 direct
- 1 indirect
- 1 double indirect
- 1 triple indirect.

#### Note: However, the file name is not kept in the file’s inode.

### A Unix filesystem has a file directory to keep track of files within it.

#### A directory is also a file.

#### The “data” for a directory entry consists of 2-tuple entries:

- The file name
- The inode for the file

## File Sector Manager (FSM)

The File Sector Manager (FSM) manages the inode block and the inodes. It also uses the SSM to manage the disk blocks.

### Assumptions

- Assume that all file names in the filesystem are 8 characters or less.
- You can decide on reasonable system parameters such as: inode size, number of inodes, file directory size, etc.

## Sector Space Manager (SSM)

The Sector Space Manager (SSM) controls the allocation of physical disk blocks (sectors or clusters). The same manager can be used also to control the allocation of physical blocks of any random access mass storage media (hard disk, DVD, CD, flash memory, main memory...). The disk blocks are called sectors. (Main memory managers use practically identical basic physical space manager for blocks or pages of physical space.)

SSM provides the lowest level of physical space management to track the disk sector availability. Its function is basically a simple 1 column binary score board with values of Free
or Allocated. A lesser known, essential function is the maintenance of the physical space management integrity.

## System Calls

- Bool fs_make(FSM\* \_fsm, unsigned int \_DISK_SIZE, unsigned int \_BLOCK_SIZE, unsigned int \_INODE_SIZE, unsigned int \_INODE_BLOCKS, unsigned int \_INODE_COUNT, int \_init_ssm_maps);
- Bool fs_remove(FSM\* \_fsm);
- Bool fs_create_file(FSM\* \_fsm, int \_is_dir, unsigned int\* \_file_name, unsigned int \_dir_inode_num);
- Inode\* fs_open_file(FSM\* \_fsm, unsigned int \_file_inode_num);
- void fs_close_file(FSM\* \_fsm);
- Bool fs_remove_file_from_dir(FSM\* \_fsm, unsigned int \_file_inode_num, unsigned int \_dir_inode_num);
- Bool fs_write_to_file(FSM\* \_fsm, unsigned int \_file_inode_num, void\* \_write_buffer, long long int \_file_size);
- Bool fs_read_from_file(FSM\* \_fsm, unsigned int \_file_inode_num, void\* \_read_buffer);
- Bool fs_remove_file(FSM\* \_fsm, unsigned int \_file_inode_num, unsigned int \_dir_inode_num);
- Bool fs_rename_file(FSM\* \_fsm, unsigned int \_file_inode_num, unsigned int\* \_file_name, unsigned int \_dir_inode_num);

## Sample Input

Example input file with `//` as comments:

```
//Make the file system
//Disk = 3000000, Block = 1024, Inode = 128, Inode Block = 32 blocks,
//Inodes per Block = 256
M:3000000:1024:128:32:256
//List contents of Folder (Inode 2)
L:2:ROOT_DIR
//Print 128 contiguous Inodes/Sectors from Inode Map and
//Aloc/Free Map starting at (Sector 0).
P:0
//Create a Directory ('DirL1-01') in Folder (Inode 2)
C:D:2:DirL1-01
//Create a Directory ('DirL1-01') in Folder (Inode 3)
C:D:3:DirL2-01
//Create a Directory ('DirL1-01') in Folder (Inode 4)
C:D:4:DirL3-01
//Create a Directory ('DirL1-01') in Folder (Inode 5)
C:D:5:DirL4-01
//Create a File ('Fi_L2-01') in Folder (Inode 2)
C:F:2:Fi_L2-01
//Print 128 contiguous Inodes/Sectors from Inode Map and
//Aloc/Free Map starting at (Sector 0).
P:0
//List contents of Folder (Inode 2)
L:2:ROOT_DIR
//List contents of Folder (Inode 3)
L:3:DirL1-01
//List contents of Folder (Inode 4)
L:4:DirL2-01
//List contents of Folder (Inode 5)
L:5:DirL3-01
//List contents of Folder (Inode 6)
L:6:DirL4-01
//Displaying Information of File (Inode 2)
I:2
//Write 273412 bytes to File (Inode 7)
W:7:273412
//Read 273412 bytes from File (Inode 7)
R:7:273412
//Displaying Information of File (Inode 7)
I:7
//Print 128 contiguous Inodes/Sectors from Inode Map and
//Aloc/Free Map starting at (Sector 256).
P:32
//Renaming File (Inode 7) in Folder (Inode 2) to "NEW_NAME"
N:7:2:NEW_NAME
//List contents of Folder (Inode 2)
L:2:ROOT_DIR
//Print 128 contiguous Inodes/Sectors from Inode Map and
//Aloc/Free Map starting at (Sector 256).
P:32
//Removing File (Inode 7) from Folder (Inode 2)
V:7:2
//List contents of Folder (Inode 2)
L:2:ROOT_DIR
//Print 128 contiguous Inodes/Sectors from Inode Map and
//Aloc/Free Map starting at (Sector 0).
P:0
//Removing File (Inode 4) from Folder (Inode 3)
V:4:3
//List contents of Folder (Inode 3)
L:3:DirL1-01
//Print 128 contiguous Inodes/Sectors from Inode Map and
//Aloc/Free Map starting at (Sector 0).
P:0
////Writing File-Tuple (Inode 25) into Double Indirect
//Data Block of Folder (Inode 3)
T:3
//Print 128 contiguous Inodes/Sectors from Inode Map and
//Aloc/Free Map starting at (Sector 0).
P:0
//Displaying Information of File (Inode 3)
I:3
//End of input.
E
```

## Sample Output

```
DEBUG_LEVEL > 0:
//Making the File System.
//Disk = 3000000, Block = 1024, Inode = 128, Inode Block = 32 blocks...
//M

-> Allocating Boot Block, Super Block, 32 Inode Blocks, and Root (Inode 2)...
** Expected Result: 3 Inodes allocated in the Inode Map
** Expected Result: 35 Blocks allocated in the Aloc/Free Map
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


DEBUG_LEVEL > 0:
//Listing contents of Directory ('ROOT_DIR') at (Inode 2)
//L:2:ROOT_DIR

-> Tuple name is: "."
-> Inode number is 2

-> Tuple name is: ".."
-> Inode number is -1
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


DEBUG_LEVEL > 0:
//Print 128 contiguous Inodes from Inode Map starting at Inode (0).
//P:0

=======================================================================

INODE MAP
00011111 11111111 11111111 11111111 11111111 11111111 11111111 11111111
0        8        16       24       32       40       48       56
11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111
64       72       80       88       96       104      112      120

=======================================================================

//Print 128 contiguous Sectors from Free/Aloc Map starting at Sector (0).
=======================================================================
FREE MAP
00000000 00000000 00000000 00000000 00011111 11111111 11111111 11111111
0        8        16       24       32       40       48       56
11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111
64       72       80       88       96       104      112      120

ALLOCATED MAP
11111111 11111111 11111111 11111111 11100000 00000000 00000000 00000000
0        8        16       24       32       40       48       56
00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
64       72       80       88       96       104      112      120
=======================================================================


DEBUG_LEVEL > 0:
//Create a Directory ('DirL1-01') in Folder (Inode 2)
//C:D:2:DirL1-01

-> Used (Inode 3) to create a Directory.
** Expected Result: 1 Inode allocated in the Inode Map
** Expected Result: 1 Block allocated in the Aloc/Free Map

...
```
