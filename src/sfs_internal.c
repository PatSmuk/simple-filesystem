/*
 *  Simple Filesystem
 *  Copyright (C) 2014 Patrick S., Robert C., Aaron P., Matthew R.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <string.h>
#include <stdlib.h>

#include "../sfs.h"
#include "blockio.h"
#include "dbg.h"
#include "sfs_internal.h"


File files[MAX_FILES];
OpenFile openFiles[MAX_OPEN_FILES];
bool freeBlocks[MAX_BLOCKS];
bool initialized = false;


File * File_find_empty() {

    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].type == FTYPE_NONE) {
            return &files[i];
        }
    }

    return NULL;
}


int File_find_by_path(File **_file, const char *path) {

    int err_code = 0;
    char **tokens = NULL;
    File *file = NULL;
    *_file = NULL;

    // Split the path into tokens.
    check_err(path_to_tokens(path, &tokens));

    // Start at the root directory;
    File *directory = &files[0];
    file = directory;

    // For each token, find the file in `directory`, updating `directory` as we traverse the path.
    for (int i = 0; tokens[i] != NULL; i++) {
        char *token = tokens[i];

        File *next = File_find_in_dir(token, directory);
        check(next != NULL, SFS_ERR_FILE_NOT_FOUND);

        // If next token is NULL, this is the last token.
        if (tokens[i+1] == NULL) {
            file = next;
        }
        else {
            check(File_is_directory(next), SFS_ERR_BAD_FILE_TYPE);
            directory = next;
        }
    }

    // Prevent memory leaks.
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
    *_file = file;
    return 0;

error:
    if (tokens) {
        for (int i = 0; tokens[i] != NULL; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }
    return err_code;
}


File * File_find_by_descriptor(int descriptor) {

    OpenFile *openFile = OpenFile_find_by_descriptor(descriptor);

    if (openFile) {
        return openFile->file;
    }

    return NULL;
}


File * File_find_in_dir(const char *name, const File *directory) {

    if (directory->dirContents == NULL) {
        return NULL;
    }

    for (FileNode *node = directory->dirContents; node != NULL; node = node->next) {
        if (strncmp(node->file->name, name, MAX_PATH_COMPONENT_LENGTH) == 0) {
            return node->file;
        }
    }

    return NULL;
}


File * File_get_parent(const File *file) {

    if (file->parentDirectoryID >= 0) {
        return File_find_by_descriptor(file->parentDirectoryID);
    }

    return NULL;
}


int File_save(const File *file) {

    int err_code = 0;
    char buffer[BLOCK_SIZE];

    // Calculate the block that `file` is stored in and the offset where its stored.
    FileID fileID = File_get_id(file);
    BlockID actualBlock = FileID_to_BlockID(fileID);
    unsigned char offset = FileID_to_offset(fileID);

    // Get the block's data from block I/O.
    check(get_block(actualBlock, buffer) == 0, SFS_ERR_BLOCK_IO);

    // Copy `file` into buffer at offset.
    memcpy(buffer+offset, file, sizeof(File));

    // Write back the block to block I/O.
    check(put_block(actualBlock, buffer) == 0, SFS_ERR_BLOCK_IO);

    return 0;

error:
    return err_code;
}


int File_add_file_to_dir(File *file, File *directory) {

    int err_code = 0;

    FileNode *newNode = malloc(sizeof(FileNode));
    check_mem(newNode);

    newNode->file = file;
    newNode->next = NULL;
    newNode->prev = NULL;

    if (!directory->dirContents) {
        // Directory is empty, just make this the first entry.
        directory->dirContents = newNode;
    }
    else {
        // Traverse the list to find the last node.
        FileNode *lastNode = directory->dirContents;

        while (lastNode->next) {
            lastNode = lastNode->next;
        }

        // Append the new node onto the end of the list.
        lastNode->next = newNode;
        newNode->prev = lastNode;
    }

    return 0;

error:
    return err_code;
}


void File_remove_file_from_dir(const File *file, File *directory) {

    FileNode *node = directory->dirContents;

    // This shouldn't actually happen.
    if (!node) {
        debug("Tried to a remove file from an empty directory.");
        return;
    }

    // Find the node that points to `file` in the list.
    while (node != NULL && node->file != file) {
        node = node->next;
    }

    // Again, shouldn't happen.
    if (!node) {
        debug("Tried to a remove file from a directory that it is not a part of.");
        return;
    }

    // If the node is the first node, just update the directory.
    if (node->prev == NULL) {
        directory->dirContents = node->next;

        // Update the next node if it exists.
        if (node->next) {
            node->next->prev = NULL;
        }
    }
    else {
        // Update the previous node to skip this one.
        node->prev->next = node->next;

        // Update the next node if it exists.
        if (node->next) {
            node->next->prev = node->prev;
        }
    }

    // Free it to prevent a memory leak.
    free(node);
}


OpenFile * OpenFile_find_empty() {

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        OpenFile *openFile = &openFiles[i];

        if (openFile->file == NULL) {
            return openFile;
        }
    }

    return NULL;
}


OpenFile * OpenFile_find_by_descriptor(int descriptor) {

    if (descriptor < 0 || descriptor >= MAX_OPEN_FILES) {
        return NULL;
    }

    OpenFile *openFile = &openFiles[descriptor];
    if (openFile->file == NULL) {
        return NULL;
    }

    return openFile;
}


int OpenFile_find_by_file(const File *file, OpenFile ***openFilesArray) {

    int err_code = 0;
    unsigned int count = 0;
    int arrayIndex = 0;
    OpenFile **array = NULL;
    *openFilesArray = NULL;

    // Count the number of matching OpenFiles so we know how much to allocate.
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        OpenFile *openFile = &openFiles[i];

        if (openFile->file == file) {
            count++;
        }
    }

    // Allocate the array.
    array = calloc(count, sizeof(OpenFile*));
    check_mem(array);

    // Put matching OpenFiles in the array.
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        OpenFile *openFile = &openFiles[i];

        if (openFile->file == file) {
            array[arrayIndex] = openFile;
            arrayIndex++;
        }
    }

    *openFilesArray = array;

error:
    free(array);
    *openFilesArray = NULL;
    return err_code;
}


int path_to_tokens(const char *path, char ***_tokens) {

    int err_code = 0;
    char **tokens = NULL;
    *_tokens = NULL;

    // `path` must start with a '/'.
    check(path[0] == '/', SFS_ERR_INVALID_PATH);

    // If `path` is "/", make `_tokens` an empty array and return success.
    if (strcmp(path, "/") == 0) {
        *_tokens = calloc(1, sizeof(char*));
        return 0;
    }

    // Otherwise, make sure that `path` doesn't end with a '/'.
    check(path[strlen(path)-1] != '/', SFS_ERR_INVALID_PATH);

    // Count the number of slashes in `path`.
    unsigned int slashCount = 0;
    for (int i = 0; i < strlen(path); i++) {
        if (path[i] == '/') {
            slashCount++;
        }
    }

    // Allocate enough room for slashCount-many pointers and the NULL terminator.
    tokens = calloc(slashCount+1, sizeof(char*));
    check_mem(tokens);
    unsigned int tokenIndex = 0;

    unsigned int lastSlash = 0;

    for (unsigned int i = 1; i <= strlen(path); i++) {
        if (path[i] == '/' || path[i] == '\0') {
            // Calculate the length of the token and check that it is within range.
            size_t length = i - lastSlash - 1;
            check(length <= MAX_PATH_COMPONENT_LENGTH, SFS_ERR_INVALID_NAME);

            // Allocate enough memory to store it + '\0'.
            char *token = calloc(length+1, sizeof(char));
            check_mem(token);

            // Copy the token into the buffer.
            strncpy(token, path+lastSlash+1, length);

            // Add it to the array.
            tokens[tokenIndex] = token;
            tokenIndex++;

            // Note the location of this slash.
            lastSlash = i;
        }
    }

    *_tokens = tokens;
    return 0;

error:
    if (tokens) {
        for (int i = 0; tokens[i] != NULL; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }
    return err_code;
}
