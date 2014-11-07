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

#include "dbg.h"
#include "sfs_internal.h"
#include "blockio.h"

static void free_directory_lists(void) {
    // For each file.
    for (int i = 0; i < MAX_FILES; i++) {
        File *file = &files[i];

        // If it's a directory and has contents.
        if (File_is_directory(file) && file->dirContents != NULL) {
            FileNode *node = file->dirContents, *lastNode;
            file->dirContents = NULL;

            // Go through each node of the list and free them.
            while (node) {
                lastNode = node;
                node = node->next;
                free(lastNode);
            }
        }
    }
}

int sfs_initialize(int erase) {

    int err_code = 0;
    char buffer[BLOCK_SIZE];
    FileSystemHeader header;

    // First, check some assumptions that should hold true.
    // These should ideally be checked at compile-time, but C compilers don't have static assertions.
    {
        const size_t fileSize = sizeof(File),
            filesPerBlock = BLOCK_SIZE/fileSize;

        // Compute the number of blocks needed to store all the File objects.
        size_t fileBlocks = MAX_FILES/filesPerBlock;
        // If there was remainder, add an extra block for them.
        if (MAX_FILES % filesPerBlock != 0) {
            fileBlocks++;
        }

        // All error codes should be negative.
        check(SFS_ERR_MAX <= 0, SFS_ERR_ADJUST_ERROR_CODES);
        // We need enough blocks to store the header and all the Files.
        check(fileBlocks < MAX_BLOCKS-1, SFS_ERR_NOT_ENOUGH_BLOCKS_FOR_FILES);
        // There should be enough room in a block to hold at least one File object.
        check(BLOCK_SIZE >= sizeof(File), SFS_ERR_BLOCKS_TOO_SMALL_FOR_FILE);
    }

    // If initialize is called twice, memory could be leaked.
    // This will clean up any FileNodes that already exist.
    if (initialized) {
        free_directory_lists();
    }
    else {
        // Free directory list memory at exit.
        atexit(free_directory_lists);
    }
    initialized = true;

    // Mark all blocks as free at the start.
    for (int i = 0; i < MAX_BLOCKS; i++) {
        freeBlocks[i] = true;
    }

    // 1. Load the first page (header) of the file system into a buffer.
    check(get_block(0, buffer) == 0, SFS_ERR_BLOCK_IO);
    freeBlocks[0] = false;

    // A filesystem already exists and we don't want to erase it.
    if (buffer[0] > 0 && !erase) {
        // a. Ensure that all the header’s fields are valid.
        memcpy(&header, buffer, sizeof(header));

        check(strcmp(header.magicCode1, MAGIC_CODE_1) == 0, SFS_ERR_INVALID_DATA_FILE);
        check(header.version == SFS_DATA_VERSION, SFS_ERR_INVALID_DATA_FILE);
        check(header.fileControlBlockSize == sizeof(File), SFS_ERR_INVALID_DATA_FILE);
        check(header.blockSize == BLOCK_SIZE, SFS_ERR_INVALID_DATA_FILE);
        check(header.maxBlocks == MAX_BLOCKS, SFS_ERR_INVALID_DATA_FILE);
        check(header.maxFiles == MAX_FILES, SFS_ERR_INVALID_DATA_FILE);
        check(header.maxBlocksPerFile == MAX_BLOCKS_PER_FILE, SFS_ERR_INVALID_DATA_FILE);
        check(header.maxPathComponentLength == MAX_PATH_COMPONENT_LENGTH, SFS_ERR_INVALID_DATA_FILE);
        check(strcmp(header.magicCode2, MAGIC_CODE_2) == 0, SFS_ERR_INVALID_DATA_FILE);

        // b. Load all of the Files into memory from the reserved File blocks.
        BlockID currentBlock = 0;

        for (FileID file_id = 0; file_id < MAX_FILES; file_id++) {
            BlockID block_id = FileID_to_BlockID(file_id);
            unsigned char offset = FileID_to_offset(file_id);

            if (block_id != currentBlock) {
                check(get_block(block_id, buffer) == 0, SFS_ERR_BLOCK_IO);
                freeBlocks[block_id] = false;
                currentBlock = block_id;
            }

            memcpy(&files[file_id], buffer+offset, sizeof(File));
        }

        // d. Ensure that the first File is the root directory.
        File *root = &files[0];
        check(File_is_directory(root), SFS_ERR_INVALID_DATA_FILE);
        check(strcmp(root->name, "/") == 0, SFS_ERR_INVALID_DATA_FILE);
        check(root->parentDirectoryID == -1, SFS_ERR_INVALID_DATA_FILE);

        // e. For each File
        for (FileID file_id = 0; file_id < MAX_FILES; file_id++) {
            File *file = &files[file_id];

            // i. Ensure that the type is valid.
            check(file->type == FTYPE_NONE || File_is_data(file) || File_is_directory(file), SFS_ERR_INVALID_DATA_FILE);

            // ii. Ensure that the parent exists and is a directory.
            File *parent = File_get_parent(file);
            if (parent) {
                // Make sure the parent is a directory.
                check(File_is_directory(parent), SFS_ERR_INVALID_DATA_FILE);
                // Make sure the parent isn't this file.
                check(parent != file, SFS_ERR_INVALID_DATA_FILE);
            }
            else {
                // The only active file without a parent is the root directory.
                check(file_id == 0 || file->type == FTYPE_NONE, SFS_ERR_INVALID_DATA_FILE);
            }

            // iii. If the File is a data file
            if (File_is_data(file)) {
                // 1. Ensure that the File’s size is consistent with the number of blocks it is using.
                unsigned char blocksInUse = 0;
                for (int i = 0; i < MAX_BLOCKS_PER_FILE; i++) {
                    if (file->blocks[i] >= 0) {
                        blocksInUse++;
                    }
                    // As soon as we hit a -1, the rest should also be -1, so break.
                    else {
                        break;
                    }
                }

                if (blocksInUse == 0) {
                    check(file->size == 0, SFS_ERR_INVALID_DATA_FILE);
                }
                else {
                    check((file->size)/BLOCK_SIZE+1 == blocksInUse, SFS_ERR_INVALID_DATA_FILE);
                }

                // 2. For each block, ensure that the block is unused and mark it at used.
                for (int i = 0; i < MAX_BLOCKS_PER_FILE; i++) {
                    BlockID block_id = file->blocks[i];
                    if (block_id >= 0) {
                        check(freeBlocks[block_id], SFS_ERR_INVALID_DATA_FILE);
                        freeBlocks[block_id] = false;
                    }
                }
            }
            // iv. If the File is a directory
            else if (File_is_directory(file)) {
                File *directory = file;
                directory->dirContents = NULL;

                // 1. For each File, if the File’s parent is this File, add that File to this File’s list of contents.
                for (FileID file_id_2 = 0; file_id_2 < MAX_FILES; file_id_2++) {
                    // A directory can't contain itself, so skip the directory.
                    if (file_id_2 == file_id) {
                        continue;
                    }

                    file = &files[file_id_2];

                    if (file->parentDirectoryID == file_id) {
                        File_add_file_to_dir(file, directory);
                    }
                }

                // 2. Ensure that the File’s size is consistent with the number of items in its list of contents.
                if (directory->dirContents == NULL) {
                    check(directory->size == 0, SFS_ERR_INVALID_DATA_FILE);
                }
                else {
                    FileNode *node = directory->dirContents;
                    unsigned char directorySize = 1;

                    while (node->next != NULL) {
                        node = node->next;
                        directorySize++;
                    }

                    check(directory->size == directorySize, SFS_ERR_INVALID_DATA_FILE);
                }
            }
        }
    }
    // The filesystem needs to be created from scratch.
    else {
        // a. Create the root directory file as File 0.
        File *root = &files[0];
        root->type = FTYPE_DIR;
        root->name[0] = '/';
        root->name[1] = '\0';
        root->size = 0;
        root->dirContents = NULL;
        root->parentDirectoryID = -1;
        check_err(File_save(root));

        // b. Save the header to block 0 and the root directory to block 1.
        strcpy(header.magicCode1, MAGIC_CODE_1);
        strcpy(header.magicCode2, MAGIC_CODE_2);
        header.version = SFS_DATA_VERSION;
        header.blockSize = BLOCK_SIZE;
        header.fileControlBlockSize = sizeof(File);
        header.maxBlocks = MAX_BLOCKS;
        header.maxBlocksPerFile = MAX_BLOCKS_PER_FILE;
        header.maxFiles = MAX_FILES;
        header.maxPathComponentLength = MAX_PATH_COMPONENT_LENGTH;

        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &header, sizeof(header));
        check(put_block(0, buffer) == 0, SFS_ERR_BLOCK_IO);

        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, root, sizeof(*root));
        check(put_block(1, buffer) == 0, SFS_ERR_BLOCK_IO);

        // c. If erase is 1, overwrite all the other blocks with a buffer filled with zeros.
        if (erase) {
            memset(buffer, 0, sizeof(buffer));

            for (int i = 2; i < MAX_BLOCKS; i++) {
                check(put_block(i, buffer) == 0, SFS_ERR_BLOCK_IO);
            }
        }

        // Initialize all the other files.
        for (int i = 1; i < MAX_FILES; i++) {
            File *file = &files[i];
            memset(file, 0, sizeof(*file));
            file->parentDirectoryID = -1;
            file->type = FTYPE_NONE;
            check_err(File_save(file));
            freeBlocks[FileID_to_BlockID(i)] = false;
        }
    }

    return 0;

error:
    return err_code;
}
