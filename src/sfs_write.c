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

int sfs_write(int fd, int start, int length, char *mem_pointer) {
    File *file;
    int err_code;
    int index;
    BlockID block;
    file = File_find_by_descriptor(fd);
    check(file!=NULL,SFS_ERR_BAD_FD);
    check(file->type==1,SFS_ERR_BAD_FILE_TYPE);
    if (start == -1) {

        start = length;
int tempVar;

        check ((start/BLOCK_SIZE) == (start+length)/BLOCK_SIZE ,SFS_ERR_BLOCK_FAULT );
        check(mem_pointer>0,SFS_ERR_BLOCK_IO);
        index = file->blocks[start / BLOCK_SIZE];
        if (index==-1) {
            for (int i = 0; i < MAX_BLOCKS; i++) {
                if (freeBlocks[i]) {
                    index = i;
                    break;
                }
            }
            check(index != -1,SFS_ERR_NO_MORE_BLOCKS);
        }
        block = (short)get_block(index, mem_pointer);
// d.
        file->size = file->size + length;
}
    // 3.
    else {
        check(start >= 0, SFS_ERR_INVALID_START_LOC);
        check (start + length <= file->size, SFS_ERR_NOT_ENOUGH_DATA);
// b.
        check ((start / BLOCK_SIZE) == ((start + length) / BLOCK_SIZE), SFS_ERR_BLOCK_FAULT);

// 4.
        index = file->blocks[start / BLOCK_SIZE];
// 5.
       block = (short) get_block(index, mem_pointer);

    }
    char boofer[BLOCK_SIZE];
    get_block(block,boofer);
    memcpy(mem_pointer,(boofer + start) , (size_t )length);
    put_block(block, boofer);

    return 0;
    error:
        return err_code;
}
