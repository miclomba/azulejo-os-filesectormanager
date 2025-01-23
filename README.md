# azulejo-os-filesectormanager

Linux File Sector Manager which can be used for creating local file systems or custom binary formats.

## Introduction

A Unix file can be a directory (containing other files and directories); or a data/binary file (binary bits: text, binary data, image, executable, symbolic link, device special, named pipes, etc.)

A file has exactly one inode, but can have many physical disk blocks (disk sectors).

1. The physical disk is segmented into disk blocks.

- Disk block 0 is the boot block or boot sector. Unix treats the boot block as a file, with an inode (inode 0) associated with it.
- Disk block 1 is the superblock (also called the filesystem header), with information about the filesystem, e.g. disk block size, inode size, number of inodes for the filesystem, etc. Unix treats the superblock as a file, with an inode (inode 1) associated with it.
- Disk block 2 through disk block N make up the inode-block.
- The remaining disk blocks (N+1 through the last block) are data blocks available for use.

2. The inode block has the inodes, numbered 0, 1, 2, ..., with following structure.

- Inode 0 is the inode for the boot block file.
- Inode 1 is the inode for the superblock file.
- Inode 2 is the inode for the root directory (/) file.

3. An inode for a file has the following structure:

- File information: file type (directory, link ...), owner, length, creation date, number of links, etc. However, the file name is not kept in the file’s inode.
- 13 disk block pointers: 10 direct, 1 indirect, 1 double indirect, and 1 triple indirect.

4. A Unix filesystem has a file directory to keep track of files within it.

- A directory is also a file.
- The “data” for a directory entry consists of 2-tuple entries:

* The file name
* The inode for the file

## File Sector Manager (FSM)

The File Sector Manager (FSM) manages the inode block and the inodes. It also uses the SSM to manage the disk blocks.

### Assumptions

Assume that all file names in the filesystem are 8 characters or less. You can decide on reasonable system parameters such as: inode size, number of inodes, file directory size, etc.

## Sector Space Manager (SSM)

The Sector Space Manager (SSM) controls the allocation of physical disk blocks (sectors or clusters). The same manager can be used also to control the allocation of physical blocks of any random access mass storage media (hard disk, DVD, CD, flash memory, main memory...). The disk blocks are called sectors. (Main memory managers use practically identical basic physical space manager for blocks or pages of physical space.)

SSM provides the lowest level of physical space management to track the disk sector availability. Its function is basically a simple 1 column binary score board with values of Free
or Allocated.

A lesser known, essential function is the maintenance of the physical space management integrity. For example, it can be disastrous if a sector is allocated to 2 or more files.

SSM is a software component that is basically independent of other functions that use it. As such, it can be designed and implemented while the file system is being designed.

## System Calls

The deliverables for an OS are the system calls (fd (file descriptor) is a number, beginning with 0 (zero)):

- Disk formatting or initialization utility: mkfs(“disk name”, disk size, number of disk blocks, size of an inode, number of inodes, etc.)
- fd = mkinode(filename, etc.) or fd = create(filename, etc.) to create a new file
- fd =openfile(filename, etc.)
- status = close(fd)
- bytes_read = read(fd, buffer, nbytes, etc.)
- bytes_written = write(fd, buffer, nbytes, etc.)
- status = fileinfo(fd, where_to_return_status, etc.)
- status = rename(oldname, newname)
- status = mkdir(dirname, etc.)
- status = rmdir(dirname, etc.)
