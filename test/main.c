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

                unsigned int i
                := buffer variable

                char dbfile[256]
                := buffer for the location of the file to be used

                char driver[MAX_INPUT]
                := buffer for input read, has a maximum input of
                10,000 characters

                unsigned int name[2]
                := buffer for holding temporary values

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
int main(int _argc, char* _argv[]) {
    // FileSectorMgr array used for pointer simplicity
    FileSectorMgr fsm[1];
    // boolean for state of reading input
    Bool loop;
    // buffer variable i
    unsigned int i;
    // buffer for the location of the file to be used
    char dbfile[256];
    // buffer for input read, has a maximum input of 10,000 characters
    char driver[MAX_INPUT];
    // buffer used when renaming files
    unsigned int name[2];
    // buffer for holding block information
    unsigned int buffer[600 * (MAX_BLOCK_SIZE / 4)];
    // buffer for holding block information
    unsigned int buffer2[600 * (MAX_BLOCK_SIZE / 4)];
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
        FILE* driverHandle = fopen(dbfile, "r+");
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
        FILE* driverHandle = fopen(dbfile, "r+");
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
            // determine what the program should do based on input
            switch (driver[i]) {
                // case 'I' displays the information of a file
                case 'I':
                    i = info_command(driver, fsm, i);
                    break;
                // case 'N' renames a file
                case 'N':
                    i = rename_command(driver, fsm, name, i);
                    break;
                // case 'W' will write data to a file
                case 'W':
                    i = write_command(driver, fsm, buffer, i);
                    break;
                // case 'R' will read a file
                case 'R':
                    i = read_command(driver, fsm, buffer2, i);
                    break;
                // case 'V' removes a file from a folder
                case 'V':
                    i = remove_command(driver, fsm, i);
                    break;
                // case 'T' tests the removal of a file or directory
                case 'T':
                    i = remove_test_command(driver, fsm, i);
                    break;
                // case 'L' lists the contents of a directory
                case 'L':
                    i = list_command(driver, fsm, buffer2, name, i);
                    break;
                // case 'C' should create either a file or directory
                case 'C':
                    i = create_command(driver, fsm, name, i);
                    break;
                // case 'M' creates the filesystem
                case 'M':
                    i = init_command(_argc, _argv, driver, fsm, i);
                    break;
                // case 'P' should print the maps
                case 'P':
                    i = print_command(driver, fsm, i);
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
                    end_command(fsm, &loop);
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
