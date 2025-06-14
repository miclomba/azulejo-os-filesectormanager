#include "commands.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "fileSectorMgr.h"
#include "gDefinitions.h"
#include "logger.h"

// Helper function to advance an index through a buffer
int advance_to_char(char* buffer, char c, int i) {
    char* p = strchr(&buffer[i], c);
    if (p != NULL) {
        i = p - buffer;  // i now points to the '\n'
    }
    return i;
}

int init_command(int _argc, char** _argv, char* driver, FileSectorMgr* fsm, int* digit, int i) {
    // vars for holding the disk, block, iNode, iNode-block, iNode-count sizes for the file system
    unsigned int _DISK_SIZE, _BLOCK_SIZE, _INODE_SIZE, _INODE_BLOCKS, _INODE_COUNT;

    // if debug, print the creation of the file system
    if (DEBUG_LEVEL > 0) {
        // call to logFSM, print creation of the file system
        logFSM(fsm, 21, 0);
    }  // end if (DEBUG_LEVEL > 0)
    // move to retrieve disk size
    i += 2;
    // check to see that the character is a digit
    *digit = isdigit(driver[i]);
    // if the character is a digit, proceed
    if (*digit > 0) {
        // first parameter is disk size, store value
        _DISK_SIZE = atoi(&driver[i]);
    }  // end if (digit > 0)
    // find next input
    i = advance_to_char(driver, ':', i);
    // move to retrieve block size
    i += 1;
    // check to see that the character is a digit
    *digit = isdigit(driver[i]);
    // if the character is a digit, proceed
    if (*digit > 0) {
        // second parameter is block size, store value
        _BLOCK_SIZE = atoi(&driver[i]);
    }  // end if digit (digit > 0)
    // find next input
    i = advance_to_char(driver, ':', i);
    // move to retrieve iNode size
    i += 1;
    // check to see that the character is a digit
    *digit = isdigit(driver[i]);
    // if the character is a digit, proceed
    if (*digit > 0) {
        // third parameter is iNode size, store value
        _INODE_SIZE = atoi(&driver[i]);
    }  // end if (digit > 0)
    // find next input
    i = advance_to_char(driver, ':', i);
    // move to retrieve number of iNode blocks
    i += 1;
    // check to see that the character is a digit
    *digit = isdigit(driver[i]);
    // if the character is a digit, proceed
    if (*digit > 0) {
        // fourth parameter is number iNode blocks, store value
        _INODE_BLOCKS = atoi(&driver[i]);
    }  // end if (digit > 0)
    // find next input
    i = advance_to_char(driver, ':', i);
    // move to retrieve iNodes per block
    i += 1;
    // check to see that the character is a digit
    *digit = isdigit(driver[i]);
    // if the character is a digit, proceed
    if (*digit > 0) {
        // fifth parameter is iNodes per block, store value
        _INODE_COUNT = atoi(&driver[i]);
    }  // end if (digit > 0)
    // if correct parameters, create the file system
    if (_argc > 1 && atoi(_argv[1]) == 1) {
        // call to mkfs, initializing the SSM values
        mkfs(fsm, _DISK_SIZE, _BLOCK_SIZE, _INODE_SIZE, _INODE_BLOCKS, _INODE_COUNT, 1);
    }  // end if
    else {
        // call to mkfs, without initializing SSM values
        mkfs(fsm, _DISK_SIZE, _BLOCK_SIZE, _INODE_SIZE, _INODE_BLOCKS, _INODE_COUNT, 0);
    }  // end else
    // find next line of input
    i = advance_to_char(driver, '\n', i);
    return i;
}

int info_command(char* driver, FileSectorMgr* fsm, Bool* success, unsigned int* inodeNumF,
                 int* digit, int i) {
    // move to retrieve the iNode number
    i += 2;
    // ensure that the character is a digit
    *digit = isdigit(driver[i]);
    // if the character is a digit, attempt to locate iNode
    if (*digit > 0) {
        // convert the character to a digit
        *inodeNumF = atoi(&driver[i]);
        // attempt to open the file located at inodeNumF
        *success = openFile(fsm, *inodeNumF);
        // if the iNode was successfully opened, print the
        // appropriate message
        if (*success == True) {
            // call to logFSM, print the iNode information
            logFSM(fsm, 29, 0);
        }  // if (*success == True)
        // close the iNode by flushing the iNode buffer inside FSM
        closeFile(fsm);
    }  // end if (*digit > 0)
    // find the next line of input
    i = advance_to_char(driver, '\n', i);
    return i;
}

int print_command(char* driver, FileSectorMgr* fsm, int* digit, int i) {
    // get the starting point, (for our input, a number)
    i += 2;
    // check to see that the character is a digit
    *digit = isdigit(driver[i]);
    // if the character is a digit, print both FSM and SSM maps
    if (*digit > 0) {
        // if debug, print the call to both logFSM and logSSM
        if (DEBUG_LEVEL > 0) {
            // call to logFSM
            logFSM(fsm, 1, 0);
            // call to logSSM
            logSSM(fsm->ssm, 1, atoi(&driver[i]));
        }  // end if (DEBUG_LEVEL > 0)
    }  // end if (digit > 0)
    // discard input until new line
    i = advance_to_char(driver, '\n', i);
    return i;
}

int create_command(char* driver, FileSectorMgr* fsm, unsigned int* name, unsigned int* inodeNumD,
                   unsigned int* inodeNumF, int* digit, int i) {
    // move to retrieve character
    i += 2;
    // store character value
    char c = driver[i];
    // find next input
    i = advance_to_char(driver, ':', i);
    // move to after semi-colon
    i += 1;
    // check to see that the character is a digit
    *digit = isdigit(driver[i]);
    // if the character is a digit, proceed
    if (*digit > 0) {
        // convert the character to a digit
        *inodeNumD = atoi(&driver[i]);
        // for debugging purposes, string of characters in buffer
        strcpy((char*)name, "my name");
        // if character is 'F', create a file
        if (c == 'F') {
            // call to createFile with parameter for file
            *inodeNumF = createFile(fsm, 0, name, *inodeNumD);
        }  // end if (c == 'F')
        // if character is 'D', create a directory
        else if (c == 'D') {
            // call to createFile with parameter for directory
            *inodeNumF = createFile(fsm, 1, name, *inodeNumD);
        }  // end if (c == 'D')
        // find next input
        i = advance_to_char(driver, ':', i);
        // move to after the semi-colon
        i += 1;
        // get the name for the new file or directory
        for (int k = 0; k < 8; k++) {
            // copy values from the input buffer
            *((char*)name + k) = driver[i];
            // proceed through input buffer
            i++;
        }  // end for (k = 0; k < 8; k++)
        // call to renameFile
        renameFile(fsm, *inodeNumF, name, *inodeNumD);
    }  // end if (*digit > 0)
    // print debug information
    printf("DEBUG_LEVEL > 0:\n");
    // if a folder was created, print respective output
    if (c == 'F') {
        // print input information
        printf("//Create a File (\'%s\') in Folder (Inode %d)\n", (char*)name, *inodeNumD);
    }  // end if (c == 'F')
    // if a directory was created, print respective output
    else if (c == 'D') {
        // print input information
        printf("//Create a Directory ");
        printf("(\'%s\') in Folder (Inode %d)\n", (char*)name, *inodeNumD);
    }  // end else if (c == 'D')
    // print input information
    printf("//C:%c:%d:%s\n\n", c, *inodeNumD, (char*)name);
    // print more information based on type
    if (c == 'D') {
        printf("-> Used (Inode %d) to create a Directory.\n", *inodeNumF);
        printf("** Expected Result: 1 Inode allocated in ");
        printf("the Inode Map\n");
        printf("** Expected Result: 1 Block allocated in the ");
        printf("Aloc/Free Map\n");
    }  // end if (c == 'D')
    else if (c == 'F') {
        printf("-> Used (Inode %d) to create a File.\n", *inodeNumF);
        printf("** Expected Result: 1 Inode allocated in ");
        printf("the Inode Map\n");
        printf("** Expected Result: 0 Blocks allocated in the ");
        printf("Aloc/Free Map\n");
    }  // end if (c == 'F')
    // print section break
    printf("- - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf(" - - - - - - - - - -\n\n\n");
    // find next line of input
    i = advance_to_char(driver, '\n', i);
    return i;
}

int rename_command(char* driver, FileSectorMgr* fsm, unsigned int* name, unsigned int* inodeNumD,
                   unsigned int* inodeNumF, int* digit, int i) {
    // move to retrieve the file's iNode
    i += 2;
    // ensure that the character is a digit
    *digit = isdigit(driver[i]);
    // if the character is a digit, proceed
    if (*digit > 0) {
        // convert the character to a digit
        *inodeNumF = atoi(&driver[i]);
        // find next input
        i = advance_to_char(driver, ':', i);
        // move to after semi-colon
        i += 1;
        // ensure that the character is a digit
        *digit = isdigit(driver[i]);
        // if the character is a digit, proceed
        if (*digit > 0) {
            // convert the character to a digit
            *inodeNumD = atoi(&driver[i]);
            // find next input
            i = advance_to_char(driver, ':', i);
            // move to after semi-colon
            i += 1;
            // read the characters for the new file name
            for (int k = 0; k < 8; k++) {
                // store the character from the buffer
                *((char*)name + k) = driver[i];
                // proceed through input buffer
                i++;
            }  // end for (k = 0; k < 8; k++)
            // print debug information
            printf("\nDEBUG_LEVEL > 0:\n");
            // print that the file will be renamed
            printf("//Renaming File (Inode ");
            printf("%d), in Folder (Inode %d), to \"%s\"\n", *inodeNumF, *inodeNumD, (char*)name);
            // print input information
            printf("//N:%d:%d:%s\n\n", *inodeNumF, *inodeNumD, (char*)name);
            // call renameFile
            renameFile(fsm, *inodeNumF, name, *inodeNumD);
            // print success in renaming file
            printf("-> Renamed File (Inode %d) to \'%s\'\n", *inodeNumF, (char*)name);
            // print section break
            printf("- - - - - - - - - - - - - - - - - - - - - - -");
            printf(" - - - - - - - - - - - - -\n\n");
        }  // end if (*digit > 0)
    }  // end if (*digit > 0)
    // find the next line of input
    i = advance_to_char(driver, '\n', i);
    return i;
}

int write_command(char* driver, FileSectorMgr* fsm, unsigned int* buffer, Bool* success,
                  unsigned int* inodeNumF, int* digit, int i) {
    // move to retrieve the iNode number
    i += 2;
    // ensure that the character is a digit
    *digit = isdigit(driver[i]);
    // if the digit is a character, proceed
    if (*digit > 0) {
        // convert the character to a digit
        *inodeNumF = atoi(&driver[i]);
        // find next input
        i = advance_to_char(driver, ':', i);
        // move to after semi-colon
        i += 1;
        // ensure that the character is a digit
        *digit = isdigit(driver[i]);
        // if the character is a digit, proceed
        if (*digit > 0) {
            // print debug information
            printf("\nDEBUG_LEVEL > 0:\n");
            // print that file will be written to
            printf("//Writing %d bytes to File (Inode %d)\n", atoi(&driver[i]), *inodeNumF);
            // print input information
            printf("//W:%d:%d\n\n", *inodeNumF, atoi(&driver[i]));
            // initialize input buffer to 0
            for (unsigned int j = 0; j < 600 * (BLOCK_SIZE / 4); j++) {
                // array slot will have value of 0
                buffer[j] = 0;
            }  // end for (unsigned int j = 0; j < 600*(BLOCK_SIZE/4);j++)
            // create test data in a double indirect data block
            buffer[14 * (BLOCK_SIZE / 4) + 23] = 77;
            buffer[266 * (BLOCK_SIZE / 4) + 56] = 113;
            // call to writeToFile
            *success = writeToFile(fsm, *inodeNumF, buffer, atoi(&driver[i]));
            // print file had been written to
            printf("-> Wrote %d bytes to File (Inode %d)\n", atoi(&driver[i]), *inodeNumF);
            printf("** Expected Result: 0 Inodes allocated in the");
            printf(" Inode Map\n");
            printf("** Expected Result: ");
            printf("%d Blocks allocated in the Aloc/Free Map\n", atoi(&driver[i]) / BLOCK_SIZE);
            printf("** Note: value of WRITE buffer at byte ");
            printf("(272608) = %d\n\n", buffer[266 * (BLOCK_SIZE / 4) + 56]);
        }  // end if (*digit > 0)
        // print section break
        printf("- - - - - - - - - - - - - - - - - - - - - - - - ");
        printf("- - - - - - - - - - - -\n\n");
    }  // end if (*digit > 0)
    // find next line of input
    i = advance_to_char(driver, '\n', i);
    return i;
}

int read_command(char* driver, FileSectorMgr* fsm, unsigned int* buffer, Bool* success,
                 unsigned int* inodeNumF, int* digit, int i) {
    // move to retrieve the iNode number of the file to be read
    i += 2;
    // check to see if character is a digit
    *digit = isdigit(driver[i]);
    // if the character is a digit, proceed
    if (*digit > 0) {
        // convert the character to a number
        *inodeNumF = atoi(&driver[i]);
        // find the next input
        i = advance_to_char(driver, ':', i);
        // move to after the semi-colon, size of bytes to be read
        i += 1;
        // check to see if character is a digit
        *digit = isdigit(driver[i]);
        // if the character is a digit, proceed
        if (*digit > 0) {
            // print debug information
            printf("\nDEBUG_LEVEL > 0:\n");
            // print that file will be read
            printf("//Reading %d bytes from file at Inode %d\n", atoi(&driver[i]), *inodeNumF);
            // print input information
            printf("//R:%d:%d\n\n", *inodeNumF, atoi(&driver[i]));
            // call to readFromFile
            *success = readFromFile(fsm, *inodeNumF, buffer);
            // print that the file had been read
            printf("-> Read %d bytes from File (Inode %d)\n", atoi(&driver[i]), *inodeNumF);
            printf("** Expected Result: 0 Inodes allocated in the");
            printf(" Inode Map\n");
            printf("** Expected Result: 0 Blocks allocated in the");
            printf(" Aloc/Free Map\n");
            printf("** Note: value of READ buffer at byte ");
            printf("(272608) = %d\n\n", buffer[266 * (BLOCK_SIZE / 4) + 56]);
        }  // end if (*digit > 0)
        // print section break
        printf("- - - - - - - - - - - - - - - - - - - - - - - - ");
        printf("- - - - - - - - - - - -\n\n\n");
    }  // end if (*digit > 0)
    // discard input until new line
    i = advance_to_char(driver, '\n', i);
    return i;
}

int remove_command(char* driver, FileSectorMgr* fsm, Bool* success, unsigned int* inodeNumD,
                   unsigned int* inodeNumF, int* digit, int i) {
    // move to retrieve the file's iNode number
    i += 2;
    // check to see that the character is a digit
    *digit = isdigit(driver[i]);
    // if the character is a digit, proceed
    if (*digit > 0) {
        // convert the character to a digit
        *inodeNumF = atoi(&driver[i]);
        // find next input
        i = advance_to_char(driver, ':', i);
        // move to after semi-colon, folder's iNode number
        i += 1;
        // check to see that the character is a digit
        *digit = isdigit(driver[i]);
        // if the character is a digit, proceed
        if (*digit > 0) {
            // convert the character to a digit
            *inodeNumD = atoi(&driver[i]);
            // print debug information
            printf("\nDEBUG_LEVEL > 0:\n");
            // print that file will be removed
            printf("//Removing File (Inode ");
            printf("%d) from Folder (Inode %d)\n", *inodeNumF, *inodeNumD);
            // print input information
            printf("//V:%d:%d\n\n", *inodeNumF, *inodeNumD);
            // print that file has been removed
            printf("-> Removed File (Inode ");
            printf("%d) from Folder (Inode %d).\n", *inodeNumF, *inodeNumD);
            // call to openFile to ensure file has been removed
            openFile(fsm, *inodeNumF);
            // if removing a folder, recursively remove all
            // subdirectories and files
            if (fsm->inode.fileType == 2) {
                // print all subdirectories and files will be deleted
                printf("** Expected Result: Recursively removing ");
                printf("all Files in Folder (Inode %d)\n", *inodeNumD);
            }  // end if (fsm->inode.fileType == 2)
            // print expected results from rmFile
            printf("** Expected Result: 1 Inode deallocated in ");
            printf("the Inode Map\n");
            printf("** Expected Result: ");
            printf("%d Blocks deallocated in the Aloc/Free Map\n",
                   fsm->inode.fileSize / BLOCK_SIZE);
            // call to rmFile
            *success = rmFile(fsm, *inodeNumF, *inodeNumD);
        }  // end if (digit > 0)
        // print section break
        printf("- - - - - - - - - - - - - - - - - - - - - - - - ");
        printf("- - - - - - - - - - - -\n\n");
    }  // end if (digit > 0)
    // find the next line of input
    i = advance_to_char(driver, '\n', i);
    return i;
}

int list_command(char* driver, FileSectorMgr* fsm, unsigned int* buffer, unsigned int* name,
                 Bool* success, unsigned int* inodeNumF, int* digit, int i) {
    // move to retrieve the iNode number
    i += 2;
    // check to see that the character is a digit
    *digit = isdigit(driver[i]);
    // if the character is a digit
    if (*digit > 0) {
        // convert the character into a digit
        *inodeNumF = atoi(&driver[i]);
        // find next input
        i = advance_to_char(driver, ':', i);
        // move to after semi-colon
        i += 1;
        // store value in buffer
        for (int k = 0; k < 8; k++) {
            // read characters from input buffer
            *((char*)name + k) = driver[i];
            // proceed through input buffer
            i++;
        }  // end for (k = 0; k < 8; k++)
        // print debug information
        if (DEBUG_LEVEL > 0) {
            // print debug information
            printf("DEBUG_LEVEL > 0:\n");
            // print that contents of a directory will be listed
            printf("//Listing contents of Directory ('");
            printf("%s') at (Inode %d)\n", (char*)name, *inodeNumF);
            // print input values
            printf("//L:%d:%s\n", *inodeNumF, (char*)name);
        }  // end if (DEBUG_LEVEL > 0)
        // call to readFromFile
        *success = readFromFile(fsm, *inodeNumF, buffer);
        // store values from readFromFile call into a local buffer
        char* charBuffer = (char*)buffer;
        // print all items of the iNode
        for (unsigned int j = 0; j < fsm->inode.fileSize; j += 16) {
            // if the values are not blank, print them
            if (*((unsigned int*)(&charBuffer[j + 12])) != 0) {
                // read value from buffer
                for (int k = 0; k < 8; k++) {
                    // copy value over from block
                    *((char*)name + k) = charBuffer[j + k];
                }  // end for (k = 0; k < 8; k++)
                // print the tuple values
                printf("\n-> Tuple name is: \"%s\"\n", (char*)name);
                printf("-> Inode number is %d\n", *((unsigned int*)(&charBuffer[j + 8])));
            }  // end if (*((unsigned int *)(&charBuffer[j+12])) != 0)
        }  // end for (unsigned int j = 0; j < fsm->inode.fileSize; j += 16)
        // print section break
        printf("- - - - - - - - - - - - - - - - - - - - - - - - ");
        printf("- - - - - - - - - - - -\n\n\n");
    }  // end if (digit > 0)
    // find next line of input
    i = advance_to_char(driver, '\n', i);
    return i;
}