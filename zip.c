#include "zip.h"
#define DEBUG 1

/**
 * BZ2 module
 */
typedef void BZFILE;

typedef struct 
{
    BZFILE *BZFile;
} BZStruct;

BZStruct *bz;
static int libtar_bzopen(const char *pathname, int oflags, mode_t mode)
{
    bz = (BZStruct *)malloc( sizeof( BZStruct ) );
    
    if((bz->BZFile = BZ2_bzopen(pathname, "rb")) == NULL)
    {
        errno = ENOMEM;
        return -1;
    }
    return 0;
}

static int libtar_bzclose(int fd)
{
    BZ2_bzclose( bz->BZFile );
    free( bz );
    return 0; 
}

static ssize_t libtar_bzread(int fd, void *buf, size_t buf_len)
{
    return BZ2_bzread(bz->BZFile, buf, buf_len);
}

static ssize_t libtar_bzwrite(int fd, const void* buf, size_t buf_len)
{
    return BZ2_bzwrite(bz->BZFile, (void *)buf, buf_len);
}
/* * BZ2 module end * */


/**
 * XZ module
 */
XZFILE *XZ_open( const char *path, const char *mode )
{
    lzma_stream     tmp =  LZMA_STREAM_INIT;
    XZFILE          *xz;


    if( !path || !mode || (mode[0] != 'r' && mode[0] != 'w') )
        return NULL;

    xz = (XZFILE *)malloc( sizeof( XZFILE ) );
    xz->strm = (lzma_stream *)malloc( sizeof( lzma_stream ) );
    *(xz->strm) = tmp;

    
    if( ( xz->file = fopen( path, mode ) ) == NULL )
    {
        free( xz->strm );
        free( xz );
        return NULL;
    }

    if( mode[0] == 'r' )
    {

        if( lzma_stream_decoder( xz->strm, UINT64_MAX, LZMA_UNSUPPORTED_CHECK | LZMA_CONCATENATED ) != LZMA_OK  )
        {
            fclose( xz->file );
            free( xz->strm );
            free( xz );
            fprintf( stderr, "lzma stream decoder init error\n" );
            return NULL;
        }
    }
    else 
    {
        if( lzma_easy_encoder( xz->strm, LZMA_PRESET_EXTREME | 6, LZMA_CHECK_CRC64 ) != LZMA_OK )
        {
            fclose( xz->file );
            free( xz->strm );
            free( xz );
            fprintf( stderr, "lzma easy encoder init error\n" );
            return NULL;
        }
    }


    xz->buf = (char *)malloc( IN_BUF_MAX * sizeof( char ) );
    memset( xz->buf, '\0', IN_BUF_MAX );
    xz->buf_len = 0;
    xz->finished = 0;

    return xz;
}

void XZ_close( XZFILE *xz )
{
    fclose( xz->file );
    free( xz->strm );
    free( xz->buf );
    free( xz );
}

int XZ_read( XZFILE *xz, void *buf, int len )
{
	int             ret;
	size_t          in_len;
	lzma_action     action;
	lzma_ret        ret_xz;


    if( !xz->finished && xz->strm->avail_in == 0 )
    {

        /* read incoming data */
        in_len = fread( xz->buf, 1, IN_BUF_MAX, xz->file );

        if( feof( xz->file ) ) 
        {
            xz->finished = 1;
        }

        if( ferror( xz->file ) ) 
        {
            xz->finished = 1;
            ret = XZ_ERROR_INPUT;
        }

        xz->strm->next_in = xz->buf;
        xz->strm->avail_in = in_len;
        xz->strm->avail_out = 0;
    }

    if( xz->strm->avail_in > 0 && xz->strm->avail_out == 0 )
    {
        memset( buf, '\0', len );
        xz->strm->next_out = buf;
        xz->strm->avail_out = len;

        /* decompress data */
        ret_xz = lzma_code( xz->strm, LZMA_RUN );

        if( ( ret_xz != LZMA_OK ) && ( ret_xz != LZMA_STREAM_END ) ) 
        {
            fprintf( stderr, "lzma_code error: %d\n", (int)ret_xz );
            ret = XZ_ERROR_DECOMPRESSION;
        } 
        else 
        {
            ret = len - xz->strm->avail_out;
        }
    }
    else if( xz->finished )
    {
        ret_xz = lzma_code( xz->strm, LZMA_FINISH );
        if( ( ret_xz != LZMA_OK ) && ( ret_xz != LZMA_STREAM_END ) ) 
        {
            fprintf( stderr, "lzma_code error: %d\n", (int)ret_xz );
            ret = XZ_ERROR_DECOMPRESSION;
        }
        lzma_end( xz->strm );
        ret = 0;
    }

    return ret;
}

int XZ_write( XZFILE *xz, void *buf, int len )
{
    return 0;
}


XZFILE *xz;
static int libtar_xzopen( const char *pathname, int oflags, mode_t mode )
{
    xz = (XZFILE *)malloc( sizeof( XZFILE ) );
    
    if( ( xz = XZ_open( pathname, "r" ) ) == NULL )
    {
        errno = ENOMEM;
        return -1;
    }
    return 0;
}

static int libtar_xzclose( int fd )
{
    XZ_close( xz );
    return 0; 
}

static ssize_t libtar_xzread( int fd, void *buf, size_t buf_len )
{
    int len = 0, len2 = 0;

    len = XZ_read( xz, buf, buf_len );

    if( len > 0 && len < buf_len )
        len2 = XZ_read( xz, buf + len, buf_len - len );

    return len + len2;
}

static ssize_t libtar_xzwrite( int fd, const void* buf, size_t buf_len )
{
    return 0;
}
/* * XZ module end * */


/**
 * tar archive  module
 */
tartype_t bztype = { 
  (openfunc_t)libtar_bzopen,
  libtar_bzclose,
  libtar_bzread,
  libtar_bzwrite,
};

tartype_t xztype = { 
  (openfunc_t)libtar_xzopen,
  libtar_xzclose,
  libtar_xzread,
  NULL,
};

int extract_file( char *tarfile, char *src, char *dest, int zip_type )
{
    char        *filename;
    TAR         *t;
    tartype_t   *zip_func;

#ifdef DEBUG
        //fprintf( stderr, "-->extract_file(\"%s\", \"%s\", \"%s\")\n", tarfile, src, dest );
#endif

    switch( zip_type )
    {
        case 1:
            zip_func = &bztype;
            break;

        case 2:
            zip_func = &xztype;
            break;
        default:
            zip_func = NULL;
    }

    if( tar_open( &t, tarfile, zip_func, O_RDONLY, 0, 0 ) == -1 )
    {
#ifdef DEBUG
        fprintf( stderr, "tar_open(): %s\n", strerror(errno) );
#endif
        return -1;
    }


	while( th_read(t) == 0 )
	{
        filename =  th_get_pathname(t);
		if( fnmatch(src, filename, FNM_PATHNAME | FNM_PERIOD) )
		{
			if (TH_ISREG(t) && tar_skip_regfile(t))
            {
#ifdef DEBUG
                fprintf(stderr, "tar_skip_regfile(): %s\n", strerror(errno));
#endif
                tar_close(t);
				return -1;
            }
		}
        else
        {
            if(tar_extract_regfile(t, dest) != 0)
            {
#ifdef DEBUG
                fprintf(stderr, "tar_extract_file(): %s\n", strerror(errno));
#endif
                tar_close(t);
				return -1;
            }
        }
    }

    tar_close(t);
    return 0;
}
/* * tar archive module * */
