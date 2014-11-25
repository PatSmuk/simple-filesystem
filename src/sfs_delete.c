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

int sfs_delete(char *pathname) {
    //Variables
    int err_code = 0;
    File *file = NULL;
    File *pFile = NULL;
    int i;
    char zeroBuffer[BLOCK_SIZE];
    //Code
    check(strcmp(pathname,"/")!= 0, SFS_ERR_CANT_DELETE_ROOT);
    check_err(File_find_by_path(&file,pathname));
    check_err(err_code == SFS_ERR_FILE_NOT_FOUND);

    // There must not be any OpenFiles that point to this File.
    for (i = 0; i < MAX_OPEN_FILES; i++) {
        OpenFile *openFile = &openFiles[i];
        check(openFile->file != file, SFS_ERR_FILE_OPEN);
    }

    if(File_is_directory(file))
    {
        check(file->dirContents == NULL,SFS_ERR_DIR_NOT_EMPTY);
    }

    pFile = File_get_parent(file);
    File_remove_file_from_dir(file,pFile);
    memset(zeroBuffer, 0, BLOCK_SIZE);

    for(i = 0; i < MAX_BLOCKS_PER_FILE; i++)
    {
        if(file->blocks[i] == -1)
        {
            break;
        }
        put_block(file->blocks[i], zeroBuffer);
    }

    memset(file, 0, sizeof(*file));
    check_err(File_save(file));

    return 0;
error:
    return err_code;
}
