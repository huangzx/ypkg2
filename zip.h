#ifndef ZIP_H
#define ZIP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <libtar.h>
#include <bzlib.h>
#include <lzma.h>

/*
 * xz
 */
#define IN_BUF_MAX 4096
#define OUT_BUF_MAX 4096

#define XZ_ERROR_INPUT		-1
#define XZ_ERROR_DECOMPRESSION	-2

typedef struct 
{
    int             finished;
    int             buf_len;
    char            *buf;
    FILE            *file;
    lzma_stream     *strm;
} XZFILE;

// mode: "r" for compress or "w" for decompress
XZFILE *XZ_open( const char *path, const char *mode );
void XZ_close( XZFILE *xz );

int XZ_read( XZFILE *xz, void *buf, int len );
int XZ_write( XZFILE *xz, void *buf, int len );


/*
 * tar
 */

// zip_type: "1" bz; "2" xz
int extract_file( char *tarfile, char *src, char *dest, int zip_type );

#endif /* !ZIP_H */
