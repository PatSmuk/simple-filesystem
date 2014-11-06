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

#include <stdlib.h>
#include <string.h>

#include "cheat.h"
#include "../sfs.h"
#include "../src/sfs_internal.h"
#include "../src/blockio.h"

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test"
#define TEST_FILE_PATH "/" TEST_FILE_NAME
#define TEST_FILE_DATA "This is some random data."
#endif

CHEAT_DECLARE(
        int test_fd; // File descriptor for the test data file.
        int root_fd; // File descriptor for the root directory.
)

CHEAT_SET_UP(
        // Create a brand new file system.
        sfs_initialize(1);

        // Create the test file.
        File *testFile = File_find_empty(),
            *root = &files[0];

        strcpy(testFile->name, "test");
        testFile->type = FTYPE_DATA;
        for (int i = 0; i < MAX_BLOCKS_PER_FILE; i++) {
            testFile->blocks[i] = -1;
        }

        // Add the test file to the root directory.
        // This memory will be freed by the clean-up routine at exit.
        FileNode *node = malloc(sizeof(FileNode));

        node->file = testFile;
        node->next = NULL;
        node->prev = NULL;
        root->dirContents = node;
        root->size += 1;
        testFile->parentDirectoryID = 0;

        // Open the root directory and test files as FDs 0 and 1 respectively.
        OpenFile *rootOpenFile = &openFiles[0],
        *testOpenFile = &openFiles[1];

        rootOpenFile->file = root;
        rootOpenFile->lastRead = NULL;
        testOpenFile->file = testFile;

        // Write some data to the test file.
        char buffer[BLOCK_SIZE];
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, TEST_FILE_DATA);

        put_block(MAX_BLOCKS-1, buffer);
        testFile->blocks[0] = MAX_BLOCKS-1;
        testFile->size = sizeof(TEST_FILE_DATA);

        root_fd = 0;
        test_fd = 1;
)

CHEAT_TEST(File_find_by_path,
        File *file;

        cheat_assert(File_find_by_path(&file, "/") == 0);
        cheat_assert(File_find_by_path(&file, "/foo") == SFS_ERR_FILE_NOT_FOUND);
        cheat_assert(File_find_by_path(&file, TEST_FILE_PATH) == 0);
)

CHEAT_SKIP(File_find_by_descriptor,
        ; // TODO: Expand once `sfs_open` is implemented.
)

CHEAT_TEST(File_find_in_dir,
        cheat_assert(File_find_in_dir(TEST_FILE_NAME, &files[0]) != NULL);
        cheat_assert(File_find_in_dir(TEST_FILE_NAME "2", &files[0]) == NULL);
)

CHEAT_TEST(File_save,
        cheat_assert(File_save(&files[0]) == 0);
)

CHEAT_TEST(path_to_tokens,
        char **tokens;

        // Empty path should fail.
        cheat_assert(path_to_tokens("", &tokens) == SFS_ERR_INVALID_PATH);

        // "/" should result in [].
        cheat_assert(path_to_tokens("/", &tokens) == 0);
        cheat_assert(tokens[0] == NULL);
        free(tokens);

        // "/foo" should result in ["foo"].
        cheat_assert(path_to_tokens("/foo", &tokens) == 0);
        cheat_assert(strcmp(tokens[0], "foo") == 0);
        cheat_assert(tokens[1] == NULL);
        free(tokens[0]);
        free(tokens);

        // "/foo/bar" should result in ["foo", "bar"].
        cheat_assert(path_to_tokens("/foo/bar", &tokens) == 0);
        cheat_assert(strcmp(tokens[0], "foo") == 0);
        cheat_assert(strcmp(tokens[1], "bar") == 0);
        free(tokens[0]);
        free(tokens[1]);
        free(tokens);

        // A path that ends with a '/' should fail.
        cheat_assert(path_to_tokens("/foo/", &tokens) == SFS_ERR_INVALID_PATH);

        // A path with a token that's length is > MAX_PATH_COMPONENT_LENGTH should fail.
        char buffer[1+MAX_PATH_COMPONENT_LENGTH+1+1];
        buffer[0] = '/';
        buffer[MAX_PATH_COMPONENT_LENGTH+2] = '\0';
        memset(buffer+1, 'A', MAX_PATH_COMPONENT_LENGTH+1);
        cheat_assert(path_to_tokens(buffer, &tokens) == SFS_ERR_INVALID_NAME);

        // A path with a token that's length is MAX_PATH_COMPONENT_LENGTH should succeed.
        buffer[MAX_PATH_COMPONENT_LENGTH+1] = '\0';
        cheat_assert(path_to_tokens(buffer, &tokens) == 0);
        cheat_assert(strncmp(tokens[0], buffer+1, MAX_PATH_COMPONENT_LENGTH) == 0);
        free(tokens[0]);
        free(tokens);
)

CHEAT_TEST(sfs_initialize,
        // Initialize should not fail.
        cheat_assert(sfs_initialize(0) == 0);

        // Root file should be created with initialize.
        cheat_assert(files[0].type == FTYPE_DIR);
        cheat_assert(strcmp(files[0].name, "/") == 0);
)

CHEAT_TEST(sfs_getsize,
        // The size of the root directory should be 1.
        cheat_assert(sfs_getsize("/") == 1);

        /* TODO: Uncomment this once `sfs_write` is implemented.
        // The size of the data file should be the same as the length of the data string.
        cheat_assert(sfs_getsize(TEST_FILE_PATH) == strlen(TEST_FILE_DATA));
        */

        // Try to read from a non-existent file.
        cheat_assert(sfs_getsize(TEST_FILE_PATH "2") == SFS_ERR_FILE_NOT_FOUND);
)

CHEAT_TEST(sfs_gettype,
        // The type of the root directory should be 1.
        cheat_assert(sfs_gettype("/") == 1);

        // The type of the test file should be 0.
        cheat_assert(sfs_gettype(TEST_FILE_PATH) == 0);

        // Try to get the type of a non-existent file.
        cheat_assert(sfs_gettype(TEST_FILE_PATH "2") == SFS_ERR_FILE_NOT_FOUND);
)

CHEAT_TEST(sfs_readdir,
        char nameBuffer[MAX_PATH_COMPONENT_LENGTH + 1];

        // Read the name of the test file.
        cheat_assert(sfs_readdir(root_fd, nameBuffer) > 0);
        cheat_assert(cheat_compare(nameBuffer, TEST_FILE_NAME));

        // All the file names have been read.
        cheat_assert(sfs_readdir(root_fd, nameBuffer) == 0);
        cheat_assert(cheat_compare(nameBuffer, ""));

        // Try to read the contents of a file.
        cheat_assert(sfs_readdir(test_fd, nameBuffer) == SFS_ERR_BAD_FILE_TYPE);

        // Try to read the contents of a non-existent file descriptor.
        cheat_assert(sfs_readdir(MAX_OPEN_FILES, nameBuffer) == SFS_ERR_BAD_FD);
)

CHEAT_TEST(sfs_open,
        // Opening the test file should succeed.
        int fd1 = sfs_open(TEST_FILE_PATH);
        cheat_assert(fd1 >= 0);

        // Opening the test file again should succeed.
        int fd2 = sfs_open(TEST_FILE_PATH);
        cheat_assert(fd2 >= 0);

        // The new file descriptor should not be the same as the last.
        cheat_assert(fd1 != fd2);

        // Open a file that doesn't exist, this should fail.
        cheat_assert(sfs_open(TEST_FILE_PATH "2") == SFS_ERR_FILE_NOT_FOUND);
)

CHEAT_TEST(sfs_close,
        // Closing the file should succeed.
        cheat_assert(sfs_close(test_fd) == 0);

        // Closing the file again should fail.
        cheat_assert(sfs_close(test_fd) == SFS_ERR_BAD_FD);
)

CHEAT_TEST(sfs_create,
        // Create a new file by appending 2 to TEST_FILE_NAME, this should succeed.
        cheat_assert(sfs_create(TEST_FILE_PATH "2", 0) == 0);

        // Try to create a file using the test file as the parent directory, this should fail.
        cheat_assert(sfs_create(TEST_FILE_PATH "/foo", 0) == SFS_ERR_BAD_FILE_TYPE);

        // Try to create a file in a directory that doesn't exist, this should fail.
        cheat_assert(sfs_create(TEST_FILE_PATH "3/foo", 0) == SFS_ERR_FILE_NOT_FOUND);

        // Create TEST_FILE_NAME again, this should fail.
        cheat_assert(sfs_create(TEST_FILE_PATH, 0) == SFS_ERR_NAME_TAKEN);
)

CHEAT_TEST(sfs_delete,
        // Deleting a file that doesn't exist should fail.
        cheat_assert(sfs_delete(TEST_FILE_PATH "2") == SFS_ERR_FILE_NOT_FOUND);

        // Deleting the root directory should always fail.
        cheat_assert(sfs_delete("/") == SFS_ERR_INVALID_NAME);

        sfs_create(TEST_FILE_PATH "2", 1);
        sfs_create(TEST_FILE_PATH "2/file", 0);

        // Deleting a directory with files should fail.
        cheat_assert(sfs_delete(TEST_FILE_PATH "2") == SFS_ERR_DIR_NOT_EMPTY);

        // Deleting the file then the directory should succeed.
        cheat_assert(sfs_delete(TEST_FILE_PATH "2/file") == 0);
        cheat_assert(sfs_delete(TEST_FILE_PATH "2") == 0);
)

CHEAT_TEST(sfs_read,
        char buffer[BLOCK_SIZE];
        char referenceBuffer[BLOCK_SIZE];

        // Reading from a directory should fail.
        cheat_assert(sfs_read(root_fd, 0, 1, buffer) == SFS_ERR_BAD_FILE_TYPE);

        // Try to start at a negative index should fail.
        cheat_assert(sfs_read(test_fd, -1, 1, buffer) == SFS_ERR_INVALID_START_LOC);

        // Reading from the data file should succeed.
        cheat_assert(sfs_read(test_fd, 0, (int)sizeof(TEST_FILE_DATA), buffer) == 0);

        // Ensure that the correct data was written to the buffer.
        cheat_assert(cheat_compare(buffer, TEST_FILE_DATA));

        // Reading past the end of the data file should fail.
        cheat_assert(sfs_read(test_fd, 0, (int)sizeof(TEST_FILE_DATA)+1, buffer) == SFS_ERR_NOT_ENOUGH_DATA);




        // Reading from a non-existent file should fail.
        cheat_assert(sfs_read(MAX_OPEN_FILES, 0, 1, buffer) == SFS_ERR_BAD_FD);

        /* TODO: Uncomment these when `sfs_delete`, `sfs_create`, `sfs_open`, and `sfs_write` are implemented.
        // Erase the test file.
        sfs_delete(TEST_FILE_PATH);
        sfs_create(TEST_FILE_PATH, 0);
        test_fd = sfs_open(TEST_FILE_PATH);

        // Fill the test file with dollar signs, as a manifestation of how dope the authors are.
        memset(buffer, '$', BLOCK_SIZE);
        memset(referenceBuffer, '$', BLOCK_SIZE);
        sfs_write(test_fd, -1, BLOCK_SIZE, buffer); // Fill first block.
        sfs_write(test_fd, -1, BLOCK_SIZE, buffer); // Fill second block.

        // Reading the entire first block should succeed.
        memset(buffer, 0, BLOCK_SIZE);
        cheat_assert(sfs_read(test_fd, 0, BLOCK_SIZE, buffer) == 0);
        cheat_assert(strncmp(buffer, referenceBuffer, BLOCK_SIZE) == 0);

        // Reading the entire second block should succeed.
        memset(buffer, 0, BLOCK_SIZE);
        cheat_assert(sfs_read(test_fd, BLOCK_SIZE, BLOCK_SIZE, buffer) == 0);
        cheat_assert(strncmp(buffer, referenceBuffer, BLOCK_SIZE) == 0);

        // Reading across blocks should fail.
        cheat_assert(sfs_read(test_fd, BLOCK_SIZE-1, 2, buffer) == SFS_ERR_BLOCK_FAULT);
        */
)

CHEAT_TEST(sfs_write,
        char buffer[BLOCK_SIZE];
        char referenceBuffer[BLOCK_SIZE];
        memset(buffer, 'A', sizeof(buffer));
        memset(referenceBuffer, 'A', sizeof(referenceBuffer));

        // Writing to a directory should fail.
        cheat_assert(sfs_write(root_fd, -1, 1, buffer) == SFS_ERR_BAD_FILE_TYPE);

        // Writing to an invalid file descriptor should fail.
        cheat_assert(sfs_write(MAX_OPEN_FILES, -1, 1, buffer) == SFS_ERR_BAD_FD);

        // Trying to write over data beyond the end of the file should fail.
        cheat_assert(sfs_write(test_fd, 0, sizeof(TEST_FILE_DATA)+1, buffer) == SFS_ERR_NOT_ENOUGH_DATA);

        // Trying to starting writing at -2 or lower should fail.
        cheat_assert(sfs_write(test_fd, -2, 1, buffer) == SFS_ERR_INVALID_START_LOC);

        // Writing over all the data should succeed.
        cheat_assert(sfs_write(test_fd, 0, sizeof(TEST_FILE_DATA), buffer) == 0);

        /* TODO: Uncomment this once `sfs_read` is implemented.
        // Make sure what's read back is correct.
        memset(buffer, 0, BLOCK_SIZE);
        sfs_read(test_fd, 0, sizeof(TEST_FILE_DATA), buffer);
        cheat_assert(strncmp(buffer, referenceBuffer, sizeof(TEST_FILE_DATA)) == 0);
        */

        /* TODO: Uncomment this once `sfs_delete`, `sfs_create`, `sfs_open`, and `sfs_read` are implemented.
        // Erase the file.
        sfs_delete(TEST_FILE_PATH);
        sfs_create(TEST_FILE_PATH, 0);
        test_fd = sfs_open(TEST_FILE_PATH);

        // Writing up to MAX_BLOCKS_PER_FILE blocks should succeed.
        for (char i = 0; i < MAX_BLOCKS_PER_FILE; i++) {
            memset(buffer, 'A'+i, BLOCK_SIZE);
            cheat_assert(sfs_write(test_fd, -1, BLOCK_SIZE, buffer) == 0);
        }

        // Make sure the writes succeeded.
        for (char i = 0; i < MAX_BLOCKS_PER_FILE; i++) {
            memset(referenceBuffer, 'A'+i, BLOCK_SIZE);
            memset(buffer, 0, BLOCK_SIZE);
            sfs_read(test_fd, 0, BLOCK_SIZE, buffer);
            cheat_assert(strncmp(buffer, referenceBuffer, BLOCK_SIZE) == 0);
        }

        // Writing across the block boundary should fail.
        cheat_assert(sfs_write(test_fd, BLOCK_SIZE-1, 2, buffer) == SFS_ERR_BLOCK_FAULT);

        // Writing too much data should fail.
        cheat_assert(sfs_write(test_fd, -1, 1, buffer) == SFS_ERR_FILE_FULL);
        */
)