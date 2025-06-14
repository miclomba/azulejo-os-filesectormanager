/*******************************************************************************
 * Utilities
 * Author: Michael Lombardi
 *******************************************************************************/
#ifndef UTILS_H
#define UTILS_H

/*
 * @brief read from file into given buffer
 * @param buffer the buffer to load content into
 * @param file_name the fully qualified file path
 */
void read_file(char* buffer, const char* file_name);

/*
 * @brief Helper function to advance an index through a buffer
 * @param buffer a given buffer
 * @param c a char within the given buffer[j] == c where j >= i
 * @param i a starting index of the buffer
 * @return the index j >= i such that buffer[j] == c
 */
int advance_to_char(char* buffer, char c, int i);

#endif  // UTILS_H