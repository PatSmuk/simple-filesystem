#ifndef __BLOCKIO_H__
#define __BLOCKIO_H__

/*
 * This file was provided as part of the project's requirements.
 *
 * It was not written by the authors of this project and its license status
 * is unknown.
 */

/************************************************
* get_block(blknum,buf)
*    - retrieves one block from the simulated disk
*      if disk file is not yet open, an attempt is
*      made to open it.
*
*    - blknum is the number of the desired block
*       (zero-based count)
*    - buf should point to a block-sized buffer
*
*    - Returns 0 if successful, -1 otherwise
*************************************************/
int get_block(int blknum, char *buf);

/************************************************
* put_block(blknum,buf)
*    - writes one block to the simulated disk
*      if disk file is not yet open, an attempt is
*      made to open it.
*
*    - blknum is the number of the desired block
*       (zero-based count)
*    - buf should point to a block-sized buffer
*
*    - Returns 0 if successful, -1 otherwise
*************************************************/
int put_block(int blknum, char *buf);

#endif