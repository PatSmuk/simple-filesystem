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

/*
 * sfs_internal.h - Simple filesystem internals definitions.
 *
 * This file contains all definitions not intended to be used by the user of the filesystem library.
 */

#ifndef __SFS_INTERNAL_H__
#define __SFS_INTERNAL_H__

#include <stdbool.h>
#include <stdlib.h>

// Bump this number whenever a change occurs to the File or FileSystemHeader structs.
#define SFS_DATA_VERSION 1


// What kind of file the File object is.
typedef enum {
    FTYPE_NONE,  // File is not yet in use.
    FTYPE_DATA,  // File is a data file.
    FTYPE_DIR    // File is a directory.
} FileType;

// A page ID is an ID from 0 to 511, so store it in a 16-bit int.
// The only valid negative ID is -1, which means “no page”.
typedef short BlockID;

// A file ID is the i-number of the file, from 0 to 63.
// The only valid negative ID is -1, which means “no file”.
// This is used as the root directory’s parent (i.e. the
//   root directory has no parent).
typedef char FileID;


// The size of each block, in bytes.
#define BLOCK_SIZE 128

// The maximum number of blocks in the file system, including reserved.
#define MAX_BLOCKS 512

// The maximum number of files that can exist in the file system, include root.
#define MAX_FILES 64

// The maximum number of blocks a File can occupy.
#define MAX_BLOCKS_PER_FILE 4

// The maximum length of a path component.
#define MAX_PATH_COMPONENT_LENGTH 6

// The maximum number of OpenFiles that can exist at once.
#define MAX_OPEN_FILES 4

// See magic code fields in FileSystemHeader for more info on these.
#define MAGIC_CODE_1 "CHEEKY "
#define MAGIC_CODE_2 "SNEAKY "


/*
 * FileSystemHeader - Meta-data for file system.
 *
 * An instance of this struct is placed at the start of the
 *   first block and is used to verify consistency between
 *   builds of the file system and open/close cycles.
 */
typedef struct {
    // Must be equal to MAGIC_CODE_1.
    //
    // This is used to check that this is really a RAMP file system data file
    //   and has not been corrupted/compressed/encrypted.
    char magicCode1[sizeof(MAGIC_CODE_1)];

    // Meta-data format version.
    //
    // This is bumped whenever breaking changes are made
    //   to the format of any of these data structures.
    unsigned int version;

    // Size of the File structure.
    //
    // The size of File may change on different platforms,
    //   causing issues.
    //
    // This is written by the library when it creates the
    //   file system and is used to ensure consistency.
    size_t fileControlBlockSize;

    // These fields hold different constants that are assumptions about the
    //   limits of the file system.
    //
    // They must be checked to ensure our assumptions are correct.
    //
    // Must be equal to BLOCK_SIZE.
    unsigned int blockSize;

    // Must be equal to MAX_BLOCKS.
    unsigned int maxBlocks;

    // Must be equal to MAX_FILES.
    unsigned int maxFiles;

    // Must be equal to MAX_BLOCKS_PER_FILE.
    unsigned int maxBlocksPerFile;

    // Must be equal to MAX_PATH_COMPONENT_LENGTH.
    unsigned int maxPathComponentLength;

    // MAX_OPEN_FILES only matters at run-time, so it need not be included.

    // Must be equal to MAGIC_CODE_2.
    //
    // This is to ensure that the field sizes in this build of the file system
    //   are the same as the build that wrote the header.
    //
    // E.g. if size_t is 8 bytes on the build that wrote the header and 4 bytes
    //   in the build that is reading it, the data won't be aligned and this
    //   code will be read incorrectly, so we can alert the user.
    char magicCode2[sizeof(MAGIC_CODE_2)];

} FileSystemHeader;


// Forward declare FileNode because FileNode's definition comes after File's.
struct sFileNode;

/*
 * File - A file system object.
 *
 * Contains all the meta-data for a data file or directory in the
 *   file system.
 * 
 * This is also called the “file control block” or “i-node”.
 */
typedef struct {
    // The type of this File.
    FileType type;

    // The name of this File. 6 character + terminator.
    char name[MAX_PATH_COMPONENT_LENGTH + 1];

    // If the file type is DATA, size is the amount of data
    //   in bytes.
    // Otherwise, if the file type is DIR, size is the amount of
    //   Files in the dir.
    size_t size;

    // A reference to the directory this file is stored in.
    // This is used at init to rebuild the directory lists.
    FileID parentDirectoryID;

    // Only one of these will be in use in a File, never both.
    // Which one depends on the type of the File.
    union {
        // The blocks the DATA file is stored on.
        BlockID blocks[MAX_BLOCKS_PER_FILE];

        // The head of the linked list of Files that make up
        //   the DIR’s contents.
        //
        // This should be NULL when stored on-disk and is
        //   generated when the file system is loaded from disk.
        struct sFileNode *dirContents;
    };

} File;


/*
 * FileNode - A linked list node that contains a File object.
 *
 * Used to link together the files in a directory.
 *
 * These are created at run-time and should not be serialized.
 */
typedef struct sFileNode {
    File *file;

    // The next FileNode in the list, or NULL if this is the tail.
    struct sFileNode *next;

    // The previous FileNode in the list, or NULL if the previous is the directory File.
    struct sFileNode *prev;

} FileNode;


/*
 * OpenFile - The state of an open File.
 *
 * Used to keep a mapping between file descriptors and Files,
 *   and maintain the state between sfs_readdir calls.
 *
 * These are created at run-time and should not be serialized.
 */
typedef struct {
    // The File object the descriptor is used to access.
    File *file;

    // If file’s type is DIR, this is used to track which node
    //   was read by the last call to sfs_readdir.
    //
    // If sfs_readdir was not called yet, or the file is not a DIR,
    //   this should be NULL.
    //
    // This should also be reset to NULL whenever a file is added
    //   or removed from the directory.
    FileNode *lastRead;

} OpenFile;


// ALl the `File` objects, pre-allocated.
extern File files[MAX_FILES];

// All the `OpenFile` objects, pre-allocated.
extern OpenFile openFiles[MAX_OPEN_FILES];

// Keeps track of which blocks are unused.
// `freeBlocks[block]` is true if `block` is unused, otherwise false.
extern bool freeBlocks[MAX_BLOCKS];

// If `false`, the file system has not been initialized, so no memory clean-up is necessary.
extern bool initialized;


/*
 * Finds an empty `File` object.
 *
 * Returns the `File` or `NULL` if they are all in use.
 */
File * File_find_empty();


/*
 * Finds a `File` by its absolute path, or NULL if it does not exist.
 *
 * Possible errors:
 *  - SFS_ERR_OUT_OF_MEMORY
 *  - SFS_ERR_INVALID_PATH
 *  - SFS_ERR_INVALID_NAME
 *  - SFS_ERR_FILE_NOT_FOUND
 *  - SFS_ERR_BAD_FILE_TYPE (if one of of the files in the middle is a data file)
 */
int File_find_by_path(File **file, const char *path);


/*
 * Finds the `File` that the descriptor has open.
 *
 * Return the `File` or `NULL` if that descriptor is invalid.
 */
File * File_find_by_descriptor(int descriptor);


/*
 * Finds the File in `directory` that is named `name`.
 *
 * Returns the `File` or `NULL` if it does not exist.
 */
File * File_find_in_dir(const char *name, const File *directory);


/*
 * Gets the File's parent (i.e. the directory it's contained within).
 *
 * Returns the `File` or `NULL` if `file` is the root directory.
 */
File * File_get_parent(const File *file);


/*
 * Get the ID of a File based on its address.
 */
#define File_get_id(file) (FileID)(((File*)(file)) - files)


/*
 * Saves the File to the disk.
 *
 * Possible errors:
 *  - SFS_ERR_BLOCK_IO
 */
int File_save(const File *file);


/*
 * Adds `file` to `directory's` list of contents.
 *
 * Possible errors:
 *  - SFS_ERR_OUT_OF_MEMORY
 */
int File_add_file_to_dir(File *file, File *directory);


/*
 * Removes `file` from `directory's` list of contents.
 *
 * Only removes the file from the directory in-memory.
 * To remove it from the directory on-disk, the file's parent directory ID should be set to -1 and saved.
 */
void File_remove_file_from_dir(const File *file, File *directory);


/*
 * Returns `true` if `file` is a data file, otherwise `false`.
 */
#define File_is_data(file) (bool)((file)->type == FTYPE_DATA)


/*
 * Returns `true` if `file` is a directory, otherwise `false`.
 */
#define File_is_directory(file) (bool)((file)->type == FTYPE_DIR)


/*
 * Find an empty OpenFile object.
 *
 * Returns the OpenFile or `NULL` if they are all in use.
 */
OpenFile *OpenFile_find_empty();


/*
 * Finds an OpenFile by descriptor.
 *
 * Returns the OpenFile or `NULL` if the descriptor is invalid (out of range or not opened).
 */
OpenFile *OpenFile_find_by_descriptor(int descriptor);


/*
 * Splits an absolute path into a NULL-terminated array of tokens.
 *
 * For example, the path "/foo/bar" would be turned into ["foo", "bar"].
 *
 * Each token and the list must be freed by the caller.
 *
 * Possible errors:
 *  - SFS_ERR_OUT_OF_MEMORY
 *  - SFS_ERR_INVALID_PATH
 *  - SFS_ERR_INVALID_NAME
 */
int path_to_tokens(const char *path, char ***tokens);


/*
 * Calculates the BlockID of the block that File `file_id` is stored in.
 *
 * For example, let's assume that BLOCK_SIZE is 128, sizeof(File) is 40, and file_id is 10:
 *   128 / 40 = 3   3 Files per File block.
 *   10 / 3 = 4     FileID 10 is in the fourth File block.
 *   4 + 1 = 5      The fourth File block is BlockID 5.
 */
#define FileID_to_BlockID(file_id) (BlockID)((file_id) / (BLOCK_SIZE/sizeof(File)) + 1)


/*
 * Calculates the offset to the File's data inside the block where File `file_id` is stored.
 *
 * For example, assuming the same parameters as the last example:
 *   128 / 40 = 3   3 Files per File block.
 *   10 % 3 = 1     The index of this File in the block is 1.
 *   1 * 40 = 40    The File is stored 40 bytes into the block.
 */
#define FileID_to_offset(file_id) (unsigned char)((file_id) % (BLOCK_SIZE/sizeof(File)) * sizeof(File))

#endif