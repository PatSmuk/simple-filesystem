set(SOURCE_FILES
    blockio.c
    sfs_internal.c
    sfs_close.c
    sfs_create.c
    sfs_delete.c
    sfs_getsize.c
    sfs_gettype.c
    sfs_initialize.c
    sfs_open.c
    sfs_read.c
    sfs_readdir.c
    sfs_write.c)

add_library(sfs ${SOURCE_FILES})