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
#include "test_config.h"
#include "utils.h"

/*
 * @brief Opens an input file, creates the file system and processes input.
 * @param argc the number of args
 * @param argv the args list
 * @return void
 */
void process_input(int argc, char** argv) {
    // FileSectorMgr array used for pointer simplicity
    FileSectorMgr fsm[1];
    // boolean for state of reading input
    Bool loop = True;
    // placeholder in the buffer array should start at the beginning
    unsigned int i = 0;
    // buffer for input read, has a maximum input of 10,000 characters
    char input[MAX_INPUT];
    // buffer used when renaming files
    unsigned int name[2];

    read_file(input, INPUT_FILE);
    // while there is input, process the commands and operate accordingly
    while (loop == True) {
        // determine what the program should do based on input
        switch (input[i]) {
            // case 'I' displays the information of a file
            case 'I':
                i = info_command(input, fsm, i);
                break;
            // case 'N' renames a file
            case 'N':
                i = rename_command(input, fsm, name, i);
                break;
            // case 'W' will write data to a file
            case 'W':
                i = write_command(input, fsm, i);
                break;
            // case 'R' will read a file
            case 'R':
                i = read_command(input, fsm, i);
                break;
            // case 'V' removes a file from a folder
            case 'V':
                i = remove_command(input, fsm, i);
                break;
            // case 'T' tests the removal of a file or directory
            case 'T':
                i = remove_test_command(input, fsm, i);
                break;
            // case 'L' lists the contents of a directory
            case 'L':
                i = list_command(input, fsm, name, i);
                break;
            // case 'C' should create either a file or directory
            case 'C':
                i = create_command(input, fsm, name, i);
                break;
            // case 'M' creates the filesystem
            case 'M':
                i = init_command(argc, argv, input, fsm, i);
                break;
            // case 'P' should print the maps
            case 'P':
                i = print_command(input, fsm, i);
                break;
            // case '/' should ignore all lines with comments
            case '/':
                // discard input until new line
                i = advance_to_char(input, '\n', i);
                break;
            // case '\n' should ignore blank lines with new lines
            case '\n':
                break;
            // case 'E' should respond to the end of input
            case 'E':
                end_command(fsm);
                loop = False;
                break;
            // default case should ignore any other form of input
            default:
                i = default_command(input, fsm, i);
                break;
        }  // end switch (action)
        // increment to the next character of the input buffer
        i++;
    }  // end while (loop == True)
}

/*
 * @brief Opens a stub file, creates the file system and processes input stub.
 * @return void
 */
void process_input_stub() {
    // FileSectorMgr array used for pointer simplicity
    FileSectorMgr fsm[1];
    // buffer for input read, has a maximum input of 10,000 characters
    char input[MAX_INPUT];
    // if the debug level is greater than 0, generate stub output
    if (DEBUG_LEVEL > 0) {
        // call logFSM to print that stub output is being created
        logFSM(fsm, 18, 0);
    }  // end if(DEBUG_LEVEL > 0)
    // open the stub input file, file should already exist
    read_file(input, INPUT_STUB_FILE);

    // print all lines of the driver buffer
    for (unsigned int i = 0; i < MAX_INPUT; i++) {
        // look for the end of the input file, start with character 'E'
        if (i <= 253 && strncmp(&input[i], "END", 3) == 0) {
            logFSM(fsm, 14, 0);
            // no need to continue reading input
            break;
        }  // end if 'END'
        // print the input read from the buffer
        printf("%c", input[i]);
    }  // end for (i = 0; i < MAX_INPUT; i++)
}

/*
 * @brief This is the main for our FSM project. Opens either a stub or input file, creates the file
 * system and processes input.
 * @return 0 for success; failure otherwise.
 */
int main(int _argc, char* _argv[]) {
    // if designated by the parameters, run program with stub output
    if (_argc > 2 && atoi(_argv[2]) == 1) {
        process_input_stub();
    }  // end if (_argc > 2 && atoi(_argv[2]) == 1)
    // if DEBUG_LEVEL is not set to read stub input, read legitimate input
    else {
        process_input(_argc, _argv);
    }  // end else
    // program successfully completed, return 0
    return 0;
}
