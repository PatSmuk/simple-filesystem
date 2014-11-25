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
 * sfs.h - Simple filesystem interface.
 *
 * This header includes all definitions intended to be used by the user of the filesystem library.
 */

#ifndef __SFS_H__
#define __SFS_H__

/*
 * All possible error conditions the library can encounter and return.
 */
enum {
    // Out of memory.
    SFS_ERR_OUT_OF_MEMORY = -100,

    // File specified by that path does not exist.
    SFS_ERR_FILE_NOT_FOUND,

    // File was not the correct type for the function.
    SFS_ERR_BAD_FILE_TYPE,

    // Supplied file descriptor was not valid.
    SFS_ERR_BAD_FD,

    // The block I/O library encountered an error.
    SFS_ERR_BLOCK_IO,

    // Specified read or write operation would cross block boundaries.
    SFS_ERR_BLOCK_FAULT,

    // Directory contains files and must be empty.
    SFS_ERR_DIR_NOT_EMPTY,

    // File cannot grow any larger.
    SFS_ERR_FILE_FULL,

    // File doesn't contain enough data to satisfy read request.
    SFS_ERR_NOT_ENOUGH_DATA,

    // File name is too long.
    SFS_ERR_INVALID_NAME,

    // File type is invalid.
    SFS_ERR_INVALID_TYPE,

    // The data file that was loaded could not be validated.
    SFS_ERR_INVALID_DATA_FILE,

    // Another file with that name already exists.
    SFS_ERR_NAME_TAKEN,

    // Starting location when reading or writing is invalid (negative for reading, < -1 for writing).
    SFS_ERR_INVALID_START_LOC,

    // Path is invalid (i.e. doesn't start with a slash or ends with a slash).
    SFS_ERR_INVALID_PATH,

    // The filesystem is full, no more files can be created.
    SFS_ERR_FILE_SYSTEM_FULL,

    // Too many files are currently open
    SFS_ERR_TOO_MANY_OPEN,

    // There are no more empty blocks to write to.
    SFS_ERR_NO_MORE_BLOCKS,

    // There are too many error codes, the first one needs to be re-assignment to a more negative value.
    SFS_ERR_ADJUST_ERROR_CODES,

    // There aren't enough blocks on the device to hold all the Files' metadata.
    SFS_ERR_NOT_ENOUGH_BLOCKS_FOR_FILES,

    // The blocks are not large enough to hold a single File object.
    SFS_ERR_BLOCKS_TOO_SMALL_FOR_FILE,

    // User tried to delete the root directory, lol.
    SFS_ERR_CANT_DELETE_ROOT,

    // User tried to delete an open file.
    SFS_ERR_FILE_OPEN,


    // Used to make sure all errors are negative numbers.
    // New error codes should come before it.
    SFS_ERR_MAX
};


/*
 * Get a human-readable error message for `error_code`.
 *
 * Returns the message, or `NULL` if `error_code` isn't a valid error code.
 */
const char *sfs_error_message(int error_code);


/*
 * Open the file specified by pathname.
 *
 * It is an error for the file not to exist.
 *
 * If a file is successfully opened, a file descriptor will be returned to the caller.
 * Otherwise, the return value is an error code.
 *
 * Possible errors:
 *  - SFS_ERR_FILE_NOT_FOUND
 */
int sfs_open(char *pathname);


/*
 * Copy length bytes of data from a regular file to the memory location specified by mem_pointer.
 *
 * The parameter start gives the offset of the first byte in the file that will be copied.
 * So, if start is 10 and length is 5, then bytes 10 through 14 of the file will be copied into memory.
 *
 * If a read request cannot be fully satisfied because the file is not long enough,
 * no data will be copied from the file, and an error value should be returned.
 *
 * Possible errors:
 *  - SFS_ERR_BAD_FILE_TYPE (must be a data file)
 *  - SFS_ERR_BAD_FD
 *  - SFS_ERR_BLOCK_IO
 *  - SFS_ERR_BLOCK_FAULT (when `start` and `start+length` are on different blocks)
 *  - SFS_ERR_NOT_ENOUGH_DATA
 *  - SFS_ERR_INVALID_START_LOC (when `start` is negative)
 */
int sfs_read(int fd, int start, int length, char *mem_pointer);


/*
 * Copy length bytes of data from the memory location specified by mem_pointer to the specified file.
 *
 * The parameter start gives the offset of the first byte in the file that should be copied to.
 * So, if start is 10 and length is 5, then five bytes from memory should be used to “overwrite” bytes 10 through 14
 * in the file.
 *
 * Alternatively, the value of start may be set to -1.
 * This indicates that the specified number of bytes should be “appended” to (added to the end of) the file.
 *
 * Appending (setting start to -1) is the only allowable way to increase the length of a file.
 * This means that if start is not -1, then it is an error for (start + length -1) to be greater than
 * the current length of the file.
 *
 * Possible errors:
 *  - SFS_ERR_BAD_FILE_TYPE (must be a data file)
 *  - SFS_ERR_BAD_FD
 *  - SFS_ERR_BLOCK_IO
 *  - SFS_ERR_BLOCK_FAULT (when `start` and `start+length` are on different blocks)
 *  - SFS_ERR_NOT_ENOUGH_DATA (when trying to overwrite data that does not exist)
 *  - SFS_ERR_FILE_FULL
 *  - SFS_ERR_INVALID_START_LOC (when `start` is < -1)
 */
int sfs_write(int fd, int start, int length, char *mem_pointer);


/*
 * This call is used to read the file name components from a directory file.
 *
 * The first time sfs_readdir is called, the first file name component (a string) in the directory should be
 * placed into memory at the location pointed to by mem_pointer.
 *
 * The function should return a positive value to indicate that a name component has been retrieved.
 *
 * Each successive call to sfs_readdir should place the next name component from the directory into the buffer,
 * and should return a positive value.
 *
 * When all names have been returned, sfs_readdir should place nothing in the buffer,
 * and should return a value of zero to indicate that the directory has been completely scanned.
 *
 * Possible errors:
 *  - SFS_ERR_BAD_FILE_TYPE (must be a directory file)
 *  - SFS_ERR_BAD_FD
 */
int sfs_readdir(int fd, char *mem_pointer);


/*
 * Indicates that the specified file descriptor is no longer needed.
 *
 * Possible errors:
 *  - SFS_ERR_BAD_FD
 */
int sfs_close(int fd);


/*
 * Delete the specified file or directory, if it exists.
 *
 * Directories must be empty to be deleted, i.e. the total number of files in the file system should decrease by
 * exactly one when this command is executed without error.
 *
 * This call should return an error if pathname specifies a directory that is not empty.
 *
 * Possible errors:
 *  - SFS_ERR_FILE_NOT_FOUND
 *  - SFS_ERR_BLOCK_IO
 *  - SFS_ERR_DIR_NOT_EMPTY
 *  - SFS_ERR_CANT_DELETE_ROOT
 */
int sfs_delete(char *pathname);


/*
 * If there is not already a file with name pathname, create one.
 *
 * If the specified name is already in use, returns an error.
 *
 * The parameter type is an integer which should have value zero or one.
 * If zero, a regular file will be created.
 * If one, a directory file will be created.
 *
 * All components of pathname (except the last) must already exist, i.e. the total number of files in the file system
 * will increase by exactly one when this command is executed without error.
 *
 * Possible errors:
 *  - SFS_ERR_OUT_OF_MEMORY
 *  - SFS_ERR_FILE_NOT_FOUND (when the parent file could not be found)
 *  - SFS_ERR_BAD_FILE_TYPE (when the parent file is not a directory)
 *  - SFS_ERR_BLOCK_IO
 *  - SFS_ERR_INVALID_NAME
 *  - SFS_ERR_INVALID_TYPE (`type` must be either 0 or 1)
 *  - SFS_ERR_NAME_TAKEN
 *  - SFS_ERR_FILE_SYSTEM_FULL
 */
int sfs_create(char *pathname, int type);


/*
 * If the specified file is a regular file, this function should return the number of bytes in the file.
 *
 * If it is a directory file, this function should return the number of directory entries in the file.
 *
 * Possible errors:
 *  - SFS_ERR_FILE_NOT_FOUND
 */
int sfs_getsize(char *pathname);


/*
 * This function should return the value zero if the specified file is a regular file.
 *
 * It should return the value one if the file is a directory.
 *
 * Possible errors:
 *  - SFS_ERR_FILE_NOT_FOUND
 */
int sfs_gettype(char *pathname);


/*
 * The sfs_initialize function must be called before any other file system functions are called.
 *
 * (In addition, it may be called at any other time.)
 *
 * This function may be used to perform any initialization required by your file system.
 *
 * Normally, the value of erase will be zero. If the value of erase is one, this indicates that any existing
 * file system on the simulated disk is to be destroyed, and that a brand new file system should be created.
 * Any existing files on the device are destroyed by this call if the value of erase is set to one.
 *
 * A new file system should consist of a single, empty root directory and no other directories or regular files.
 *
 * Possible errors:
 *  - SFS_ERR_BLOCK_IO
 *  - SFS_ERR_INVALID_DATA_FILE
 */
int sfs_initialize(int erase);

#endif