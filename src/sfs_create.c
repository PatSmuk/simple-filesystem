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

int sfs_create(char *pathname, int type) {
    int err_code;
    char **tokens;
    File *file = NULL;
    File *pFile = &files[0];
    int i;
    FileID parentID;
    err_code = (File_find_by_path(&file, pathname));
    //check if file is null
    check(err_code == SFS_ERR_FILE_NOT_FOUND, err_code ? err_code : SFS_ERR_NAME_TAKEN);

    file = File_find_empty();

    check_err(path_to_tokens(pathname, &tokens));
    char parentPath[MAX_FILES * MAX_PATH_COMPONENT_LENGTH] = "";
    for (i = 0; tokens[i+1] != NULL; i++) {
        strcat(parentPath, "/");
        strcat(parentPath, tokens[i]);
        check_err(File_find_by_path(&pFile, parentPath));
        check(File_is_directory(pFile), SFS_ERR_BAD_FILE_TYPE);
    }

    parentID = File_get_id(pFile);
    file->type = type == 0 ? FTYPE_DATA : FTYPE_DIR;
    file->parentDirectoryID = parentID;
    strcpy(file->name,tokens[i]);

    if (File_is_data(file))
    {
        for(i = 0; i < MAX_BLOCKS_PER_FILE; i++)
        {
            file->blocks[i] = -1;
        }
    }
    check_err(File_add_file_to_dir(file, pFile));

    File_save(file);
    return 0;

error:
    return err_code;
}
