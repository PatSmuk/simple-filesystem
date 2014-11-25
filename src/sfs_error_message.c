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
#include "../sfs.h"

static const char *errorMessages[] = {
    "Could not allocate memory.",                                               // SFS_ERR_OUT_OF_MEMORY
    "The file specified by the path could not be found.",                       // SFS_ERR_FILE_NOT_FOUND
    "File was not the correct type for the function.",                          // SFS_ERR_BAD_FILE_TYPE
    "Supplied file descriptor was not valid.",                                  // SFS_ERR_BAD_FD
    "The block I/O library encountered an error.",                              // SFS_ERR_BLOCK_IO
    "Specified read or write operation would cross block boundaries.",          // SFS_ERR_BLOCK_FAULT
    "Directory contains files and must be empty.",                              // SFS_ERR_DIR_NOT_EMPTY
    "File cannot grow any larger.",                                             // SFS_ERR_FILE_FULL
    "File doesn't contain enough data to satisfy read/write request.",          // SFS_ERR_NOT_ENOUGH_DATA
    "File name is too long.",                                                   // SFS_ERR_INVALID_NAME
    "File type is invalid.",                                                    // SFS_ERR_INVALID_TYPE
    "The data file that was loaded could not be validated.",                    // SFS_ERR_INVALID_DATA_FILE
    "Another file with that name already exists.",                              // SFS_ERR_NAME_TAKEN
    "Starting location when reading or writing is invalid.",                    // SFS_ERR_INVALID_START_LOC
    "Path is invalid (i.e. doesn't start with a slash or ends with a slash).",  // SFS_ERR_INVALID_PATH
    "The filesystem is full, no more files can be created.",                    // SFS_ERR_FILE_SYSTEM_FULL
    "Too many files are currently open",                                        // SFS_ERR_TOO_MANY_OPEN
    "There are no more empty blocks to write to.",                              // SFS_ERR_NO_MORE_BLOCKS
    "There are too many error codes, "
    "the first one needs to be re-assignment to a more negative value.",        // SFS_ERR_ADJUST_ERROR_CODES
    "There aren't enough blocks on the device to hold all the Files' metadata.", // SFS_ERR_NOT_ENOUGH_BLOCKS_FOR_FILES
    "The blocks are not large enough to hold a single File object.",            // SFS_ERR_BLOCKS_TOO_SMALL_FOR_FILE
    "Deleting the root directory is not permitted.",                            // SFS_ERR_CANT_DELETE_ROOT
};

const char *sfs_error_message(int error_code) {
    if (error_code < SFS_ERR_OUT_OF_MEMORY || error_code >= SFS_ERR_MAX) {
        return NULL;
    }
    error_code -= SFS_ERR_OUT_OF_MEMORY;
    return errorMessages[error_code];
}
