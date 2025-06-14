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