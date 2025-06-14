/*******************************************************************************
 * Utilities
 * Author: Michael Lombardi
 *******************************************************************************/
#include "utils.h"

#include <stdio.h>
#include <string.h>

#include "global_constants.h"

void read_file(char* buffer, const char* file_name) {
    // open the input file; file should already exist
    FILE* inputFile = fopen(file_name, "r+");
    // read the input file
    fread(buffer, sizeof(char), MAX_INPUT, inputFile);
    fclose(inputFile);
    inputFile = Null;
}

int advance_to_char(char* buffer, char c, int i) {
    char* p = strchr(&buffer[i], c);
    if (p != NULL) {
        i = p - buffer;  // i now points to the '\n'
    }
    return i;
}