/*
 * This file was originally part of "Learn C the Hard Way" by Zed A. Shaw
 * and is not the original work of the authors of Simple Filesystem.
 *
 * The original work can be found at: http://c.learncodethehardway.org/book/ex20.html
 *
 * The original work has been modified to make it more suitable to library programming.
 * In particular, logging macros have been removed, and the other macros now set an error code rather than log errors.
 */

#ifndef __dbg_h__
#define __dbg_h__

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "../sfs.h"

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define check(A, C) if (!(A)) { err_code=C; goto error; }

#define sentinel(C) { err_code=C; goto error; }

#define check_mem(A) check((A), SFS_ERR_OUT_OF_MEMORY)

#define check_debug(A, C, M, ...) if (!(A)) { err_code=C; debug(M, ##__VA_ARGS__); goto error; }

#define check_err(C) if ((C) < 0) { err_code=(C); goto error; }

#endif