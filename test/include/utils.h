/*******************************************************************************
 * Utilities
 * Author: Michael Lombardi
 *******************************************************************************/
#ifndef UTILS_H
#define UTILS_H

/**
 * @brief Reads the contents of a file into a buffer.
 * Opens the specified file and loads its contents into the provided buffer.
 * @param[out] buffer Buffer to store the file contents.
 * @param[in] file_name Fully qualified path to the file to be read.
 * @return void
 */
void read_file(char* buffer, const char* file_name);

/**
 * @brief Advances through a buffer to the next occurrence of a specified character.
 * Scans forward from index `i` in the buffer until it finds the first occurrence
 * of character `c`.
 * @param[in] buffer The character buffer to search.
 * @param[in] c The character to search for.
 * @param[in] i Starting index within the buffer.
 * @return The index `j >= i` such that `buffer[j] == c`, or the terminating index if not found.
 */
int advance_to_char(char* buffer, char c, int i);

#endif  // UTILS_H