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

#include <error.h>
#include "dbg.h"
#include "sfs_internal.h"
#include "blockio.h"


int sfs_read(int fd, int start, int length, char *mem_pointer) {

    File *file;
    int err_code;
    file = File_find_by_descriptor(fd);
    check(file!=NULL,SFS_ERR_BAD_FD);
    check (file->type == FTYPE_DATA, SFS_ERR_BAD_FILE_TYPE);
    check (start>=0,SFS_ERR_INVALID_START_LOC);
    int tempVar = start/BLOCK_SIZE;
    check ((tempVar ) == ((start + length) / BLOCK_SIZE), SFS_ERR_INVALID_START_LOC);
// 3.
    check (start + length <= file->size, SFS_ERR_NOT_ENOUGH_DATA);
// 4.
    int index = file->blocks[start / BLOCK_SIZE];
// 5.
    //check(index>=0,S)
    //BlockID block = (short)get_block(index, mem_pointer);
// 6.
    char boofer[BLOCK_SIZE];
    get_block(index,boofer);
    memcpy(mem_pointer,(boofer + start) , (size_t )length);
mem_pointer[length]='\0';

            return 0;
    error:
        return err_code;
}