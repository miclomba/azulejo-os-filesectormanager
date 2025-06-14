/******************************************************************************
 * Test Application
 * Author: Michael Lombardi
 ******************************************************************************/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "config.h"
#include "fileSectorMgr.h"
#include "logger.h"

/********************************* def beg main *******************************

   int main(int _argc, char *_argv[])

   Function
        This is the main for our FSM project. Opens either a stub
        or input file, creates the file system and processes input.

   Data Environment
        Parameters
                int _argc
                := the number of arguments passed to program

                char *_argv[]
                := an array holding the arguments

        Local Variables
                FileSectorMgr fsm[1]
                := FSM object

                Bool loop
                := boolean for state of reading input

                char action
                := character for current action of the program

                unsigned int i
                := buffer variable

                unsigned int j
                := buffer variable

                unsigned int k
                := buffer variable

                unsigned int m
                := buffer variable

                char c
                := buffer variable

                int digit
                := placeholder for any conversions from character
                to integer

                Bool success
                := boolean for use with file handles

                FILE * driverHandle
                := handle to a file to be used

                char dbfile[256]
                := buffer for the location of the file to be used

                char driver[MAX_INPUT]
                := buffer for input read, has a maximum input of
                10,000 characters

                unsigned int name[2]
                := buffer for holding temporary values

                char *charBuffer;
                := buffer for holding contents from blocks

                unsigned int inodeNumF
                := buffer for holding an iNode number dealing with files

                unsigned int inodeNumD
                := buffer for holding an iNode number dealing with directories

                unsigned int _DISK_SIZE
                := buffer for holding disk size parameter for file system

                unsigned int _BLOCK_SIZE
                := buffer for holding block size parameter for file system

                unsigned int _INODE_SIZE
                := buffer for holding iNode size parameter for file system

                unsigned int _INODE_BLOCKS
                := buffer for holding iNode blocks parameter for file system

                unsigned int _INODE_COUNT
                := buffer for holding iNode count parameter for file system

                unsigned int buffer[600*(MAX_BLOCK_SIZE/4)]
                := buffer for holding block information

                unsigned int buffer2[600*(MAX_BLOCK_SIZE/4)]
                := buffer for holding block information

                unsigned int index[6]
                := buffer for holding temporary values

                unsigned int block[MAX_BLOCK_SIZE/4]
                := buffer for holding block information

   int main
        = returns 0 upon completing

   Change Record
        5/2/10 - Function implemented.

*********************************** def end main ******************************/
int main(int _argc, char *_argv[]) {
    // FileSectorMgr array used for pointer simplicity
    FileSectorMgr fsm[1];
    // boolean for state of reading input
    Bool loop;
    // character for current action of the program
    char action;
    // buffer variable i
    unsigned int i;
    // buffer variable m
    unsigned int m;
    // placeholder for any conversions from character to integer
    int digit;
    // boolean for use with file handles
    Bool success;
    // handle to a file to be used
    FILE *driverHandle;
    // buffer for the location of the file to be used
    char dbfile[256];
    // buffer for input read, has a maximum input of 10,000 characters
    char driver[MAX_INPUT];
    // buffer used when renaming files
    unsigned int name[2];
    // buffer for holding an iNode number dealing with files
    unsigned int inodeNumF;
    // buffer for holding an iNode number dealing with directories
    unsigned int inodeNumD;
    // buffer for holding block information
    unsigned int buffer[600 * (MAX_BLOCK_SIZE / 4)];
    // buffer for holding block information
    unsigned int buffer2[600 * (MAX_BLOCK_SIZE / 4)];
    // buffer for holding temporary values
    unsigned int index[6];
    // buffer for holding block information
    unsigned int block[MAX_BLOCK_SIZE / 4];
    // if designated by the parameters, run program with stub output
    if (_argc > 2 && atoi(_argv[2]) == 1) {
        // if the debug level is greater than 0, generate stub output
        if (DEBUG_LEVEL > 0) {
            // call logFSM to print that stub output is being created
            logFSM(fsm, 18, 0);
        }  // end if(DEBUG_LEVEL > 0)
        // write the stub input location into buffer
        sprintf(dbfile, "./test/qaInputStub.txt");
        // open the stub input file, file should already exist
        driverHandle = fopen(dbfile, "r+");
        // read the driverHandle
        fread(driver, sizeof(char), MAX_INPUT, driverHandle);
        fclose(driverHandle);
        driverHandle = Null;
        // print all lines of the driver buffer
        for (i = 0; i < MAX_INPUT; i++) {
            // look for the end of the input file, start with character 'E'
            if (i <= 253 && strncmp(&driver[i], "END", 3) == 0) {
                logFSM(fsm, 14, 0);
                // no need to continue reading input
                break;
            }  // end if 'END'
            // print the input read from the buffer
            printf("%c", driver[i]);
        }  // end for (i = 0; i < MAX_INPUT; i++)
    }  // end if (_argc > 2 && atoi(_argv[2]) == 1)
    // if DEBUG_LEVEL is not set to read stub input, read legitimate input
    else {
        // write the qa input location into buffer
        sprintf(dbfile, "./test/qaInput.txt");
        // open the qa input file, file should already exist
        driverHandle = fopen(dbfile, "r+");
        // read the driverHandle
        fread(driver, sizeof(char), MAX_INPUT, driverHandle);
        fclose(driverHandle);
        driverHandle = Null;
        // placeholder in the buffer array should start at the beginning
        i = 0;
        // set loop to true to ensure that all input is read
        loop = True;
        // while there is input, process the commands and operate accordingly
        while (loop == True) {
            // read the command from the driver buffer
            action = driver[i];
            // determine what the program should do based on input
            switch (action) {
                // case 'I' displays the information of a file
                case 'I':
                    i = info_command(driver, fsm, &success, &inodeNumF, &digit, i);
                    break;
                // case 'N' renames a file
                case 'N':
                    i = rename_command(driver, fsm, name, &inodeNumD, &inodeNumF, &digit, i);
                    break;
                // case 'W' will write data to a file
                case 'W':
                    i = write_command(driver, fsm, buffer, &success, &inodeNumF, &digit, i);
                    break;
                // case 'R' will read a file
                case 'R':
                    i = read_command(driver, fsm, buffer2, &success, &inodeNumF, &digit, i);
                    break;
                // case 'V' removes a file from a folder
                case 'V':
                    i = remove_command(driver, fsm, &success, &inodeNumD, &inodeNumF, &digit, i);
                    break;
                // case 'T' tests the removal of a file or directory
                case 'T':
                    // move to retrieve the iNode number
                    i += 2;
                    // check to see that character is a digit
                    digit = isdigit(driver[i]);
                    // if the character is a digit, proceed
                    if (digit > 0) {
                        // print debug information
                        printf("\nDEBUG_LEVEL > 0:\n");
                        // print that iNode values will be tested
                        printf("//TESTING: Writing File-Tuple (Inode 25) into ");
                        printf("Double Indirect Data Block of Folder ");
                        printf("(Inode %d)\n", atoi(&driver[i]));
                        // print input information
                        printf("//T:%d\n\n", atoi(&driver[i]));
                        // call to openFile
                        success = openFile(fsm, atoi(&driver[i]));
                        // print that iNode has been opened
                        printf("Opened Folder (Inode %d)\n", atoi(&driver[i]));
                        // print that the required data blocks are being allocated
                        printf("Allocating required data Blocks:\n\n");
                        // call to getSector
                        getSector(1, fsm->ssm);
                        // retrieve index values from SSM
                        index[0] = fsm->ssm->index[0];
                        index[1] = fsm->ssm->index[1];
                        // locate double indirect
                        fsm->inode.dIndirect =
                            BLOCK_SIZE * (8 * fsm->ssm->index[0] + fsm->ssm->index[1]);
                        // call to writeInode
                        writeInode(&fsm->inode, atoi(&driver[i]), fsm->diskHandle);
                        // call to allocateSectors
                        allocateSectors(fsm->ssm);
                        // print the block size
                        printf("Double indirect Ptr --> %d\n", fsm->inode.dIndirect / BLOCK_SIZE);
                        // call to getSector
                        getSector(1, fsm->ssm);
                        // retrieve index values from SSM
                        index[2] = fsm->ssm->index[0];
                        index[3] = fsm->ssm->index[1];
                        // clear the block
                        for (m = 0; m < BLOCK_SIZE / 4; m++) {
                            // block values set to -1
                            block[m] = (unsigned int)(-1);
                        }  // end for (m = 0; m < BLOCK_SIZE/4; m
                        // retrieve the block
                        block[0] = BLOCK_SIZE * (8 * fsm->ssm->index[0] + fsm->ssm->index[1]);
                        // move to the beginning of the file
                        fseek(fsm->diskHandle, 0, SEEK_SET);
                        // move to the double indirect block
                        fseek(fsm->diskHandle, fsm->inode.dIndirect, SEEK_SET);
                        // write block to the file
                        fwrite(block, BLOCK_SIZE, 1, fsm->diskHandle);
                        // call to allocateSectors
                        allocateSectors(fsm->ssm);
                        // print that iNode values will be tested
                        printf("Double indirect Ptr --> Double Indirect Block ");
                        printf("--> %d\n", block[0] / BLOCK_SIZE);
                        // call to getSector
                        getSector(1, fsm->ssm);
                        // retrieve index values from SSM
                        index[4] = fsm->ssm->index[0];
                        index[5] = fsm->ssm->index[1];
                        // clear the block
                        for (m = 0; m < BLOCK_SIZE / 4; m++) {
                            // block values set to -1
                            block[m] = (unsigned int)(-1);
                        }  // end for (m = 0; m < BLOCK_SIZE/4; m++)
                        // retrieve the block
                        block[0] = BLOCK_SIZE * (8 * fsm->ssm->index[0] + fsm->ssm->index[1]);
                        // move to the beginning of the file
                        fseek(fsm->diskHandle, 0, SEEK_SET);
                        // move to the double indirect block
                        fseek(fsm->diskHandle, BLOCK_SIZE * (8 * index[2] + index[3]), SEEK_SET);
                        // write block to file
                        fwrite(block, BLOCK_SIZE, 1, fsm->diskHandle);
                        // call to allocateSectors
                        allocateSectors(fsm->ssm);
                        // print block value
                        printf("Double indirect Ptr --> Double Indirect Block ");
                        printf("--> Single Indirect Block -> %d\n", block[0] / BLOCK_SIZE);
                        // move value of block to index
                        index[0] = block[0];
                        // clear the block
                        for (m = 0; m < BLOCK_SIZE / 4; m++) {
                            // block value set to 0
                            block[m] = 0;
                        }  // end for (m = 0; m < BLOCK_SIZE/4; m++)
                        // for debugging purposes, assign values in buffer
                        block[0] = 1;
                        block[1] = 1;
                        block[2] = 1;
                        block[3] = 0;
                        block[4] = 1;
                        block[5] = 1;
                        block[6] = 25;
                        block[7] = 1;
                        block[8] = 3;
                        block[9] = 3;
                        block[10] = 1;
                        block[11] = 0;
                        // move to the beginning of the file
                        fseek(fsm->diskHandle, 0, SEEK_SET);
                        // move to the location stored in index
                        fseek(fsm->diskHandle, index[0], SEEK_SET);
                        // write value of block to disk
                        fwrite(block, sizeof(unsigned int), BLOCK_SIZE / 4, fsm->diskHandle);
                        // print the addition of tuple
                        printf("Added File-Tuple (Inode 25) to data block at ");
                        printf("sector %d\n", index[0] / BLOCK_SIZE);
                        // call to rmFileFromDir
                        success = rmFileFromDir(fsm, 25, atoi(&driver[i]));
                        // print respective functions
                        if (success == True) {
                            // print remove was successful
                            printf("\nTRY FUNCTION CALL: rmFileFromDir(fsm,25,");
                            printf("%d);\n\nReturned TRUE\n", atoi(&driver[i]));
                        }  // end if (success == True)
                        else {
                            // print remove was unsuccessful
                            printf("\nTRY FUNCTION CALL: rmFileFromDir(fsm,25,");
                            printf("%d);\n\nReturned FALSE\n", atoi(&driver[i]));
                        }  // end else
                        // call to openFile
                        success = openFile(fsm, atoi(&driver[i]));
                        // set single indirect to -1
                        fsm->inode.dIndirect = -1;
                        // write iNode to file
                        writeInode(&fsm->inode, atoi(&driver[i]), fsm->diskHandle);
                        // print section break
                        printf("- - - - - - - - - - - - - - - - - - - - - - - - ");
                        printf("- - - - - - - - - - - -\n\n\n");
                    }  // end if (digit > 0)
                    // find the next line of input
                    i = advance_to_char(driver, '\n', i);
                    break;
                // case 'L' lists the contents of a directory
                case 'L':
                    i = list_command(driver, fsm, buffer2, name, &success, &inodeNumF, &digit, i);
                    break;
                // case 'C' should create either a file or directory
                case 'C':
                    i = create_command(driver, fsm, name, &inodeNumD, &inodeNumF, &digit, i);
                    break;
                // case 'M' creates the filesystem
                case 'M':
                    i = init_command(_argc, _argv, driver, fsm, &digit, i);
                    break;
                // case 'P' should print the maps
                case 'P':
                    i = print_command(driver, fsm, &digit, i);
                    break;
                // case '/' should ignore all lines with comments
                case '/':
                    // discard input until new line
                    i = advance_to_char(driver, '\n', i);
                    break;
                // case '\n' should ignore blank lines with new lines
                case '\n':
                    break;
                // case 'E' should respond to the end of input
                case 'E':
                    // if debug, print the end has been reached
                    if (DEBUG_LEVEL > 0) {
                        // call to logFSM, print the end has been reached
                        logFSM(fsm, 14, 0);
                    }  // end if (DEBUG_LEVEL > 0)
                    // no more input, stop reading loop
                    loop = False;
                    rmfs(fsm);
                    break;
                // default case should ignore any other form of input
                default:
                    // discard input until new line
                    i = advance_to_char(driver, '\n', i);
                    // if debug, print bad input
                    if (DEBUG_LEVEL > 0) {
                        // call to logFSM, print bad input
                        logFSM(fsm, 15, 0);
                    }  // end if (DEBUG_LEVEL > 0)
                    break;
            }  // end switch (action)
            // increment to the next character of the input buffer
            i++;
        }  // end while (loop == True)
    }  // end else
    // program successfully completed, return 0
    return 0;
}
