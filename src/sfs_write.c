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

int sfs_write(int fd, int start, int length, char *mem_pointer) {
    File *file;
    int err_code;
    BlockID blockID;
    char boofer[BLOCK_SIZE];
    file = File_find_by_descriptor(fd);
    check(file!=NULL,SFS_ERR_BAD_FD);
    check(file->type==1,SFS_ERR_BAD_FILE_TYPE);
    if (start == -1) {

        start = (int)file->size;

        check((start/BLOCK_SIZE) == (start+length-1)/BLOCK_SIZE ,SFS_ERR_BLOCK_FAULT );
        check(start < (BLOCK_SIZE * MAX_BLOCKS_PER_FILE), SFS_ERR_FILE_FULL);
        blockID = file->blocks[start / BLOCK_SIZE];

        if (blockID==-1) {
            check(file->blocks[MAX_BLOCKS_PER_FILE-1] == -1, SFS_ERR_FILE_FULL);
            for (BlockID i = 0; i < MAX_BLOCKS; i++) {
                if (freeBlocks[i]) {
                    blockID = i;
                    break;
                }
            }
            check(blockID != -1,SFS_ERR_NO_MORE_BLOCKS);
        }
        check(get_block(blockID, boofer) == 0, SFS_ERR_BLOCK_IO);

        // Mark the block as not free.
        freeBlocks[blockID] = false;
        // Add the block ID to the File.
        for (int i = 0; i < MAX_BLOCKS_PER_FILE; i++) {
            if (file->blocks[i] == -1) {
                file->blocks[i] = blockID;
                break;
            }
        }
        // Update the File's size.
        file->size = file->size + length;
}
    // 3.
    else {
        check(start >= 0, SFS_ERR_INVALID_START_LOC);
        check (start + length <= file->size, SFS_ERR_NOT_ENOUGH_DATA);

        check ((start / BLOCK_SIZE) == ((start + length) / BLOCK_SIZE), SFS_ERR_BLOCK_FAULT);


        blockID = file->blocks[start / BLOCK_SIZE];

        check(get_block(blockID, boofer) == 0, SFS_ERR_BLOCK_IO);
    }

    memcpy(boofer + (start % BLOCK_SIZE), mem_pointer, (size_t )length);
    put_block(blockID, boofer);

    return 0;
error:
    return err_code;
}
