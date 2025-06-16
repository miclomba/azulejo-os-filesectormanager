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
    size_t res = fread(buffer, sizeof(char), MAX_INPUT, inputFile);
    if (res == 0) {
        printf("Error reading file %s from file stream.\n", file_name);
        return;
    }
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
