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