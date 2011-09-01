#include "archive.h"

/*
 * file to file
 */
int archive_extract_file( char *arch_file, const char *src, char *dest )
{
    int                     ret, flags;     
    const char              *filename;
    struct archive          *arch_r = NULL, *arch_w = NULL;
    struct archive_entry    *entry;

    if( !arch_file || !src || !dest )
        return -1;

    arch_r = archive_read_new();
    archive_read_support_format_all( arch_r );
    archive_read_support_compression_all( arch_r );

    if( archive_read_open_filename( arch_r, arch_file, 10240 ) != ARCHIVE_OK )
        goto errout;

    while( archive_read_next_header( arch_r, &entry ) == ARCHIVE_OK ) 
    {
        filename = archive_entry_pathname( entry );
		if( fnmatch( src, filename, FNM_PATHNAME | FNM_PERIOD ) )
		{
			if( archive_read_data_skip( arch_r ) != ARCHIVE_OK )
            {
                goto errout;
            }
		}
        else
        {
#ifdef DEBUG
            printf("extract:%s\n", filename );
#endif

            flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;
            arch_w = archive_write_disk_new();
            archive_write_disk_set_options( arch_w, flags );
            archive_write_disk_set_standard_lookup( arch_w );
            archive_entry_set_pathname( entry, dest );

            if( archive_read_extract2( arch_r, entry, arch_w ) != ARCHIVE_OK ) 
                goto errout;

            archive_write_finish( arch_w );
        }
    }

    archive_read_finish( arch_r );
    return 0;

errout:
#ifdef DEBUG
    fprintf( stderr, "%s\n", archive_error_string( arch_r ) );
#endif
    if( arch_r )
        archive_read_finish( arch_r );
    if( arch_w )
        archive_write_finish( arch_w );
    return -1;
}

/*
 * file to memory
 */
int archive_extract_file2( char *arch_file, const char *src, void **dest_buff, size_t *dest_len )
{
    int                     ret, flags;     
    const char              *filename;
    struct archive          *arch_r = NULL;
    struct archive_entry    *entry;

    if( !arch_file || !src )
        return -1;

    arch_r = archive_read_new();
    archive_read_support_format_all( arch_r );
    archive_read_support_compression_all( arch_r );

    if( archive_read_open_filename( arch_r, arch_file, 10240 ) != ARCHIVE_OK )
        goto errout;

    while( archive_read_next_header( arch_r, &entry ) == ARCHIVE_OK ) 
    {
        filename = archive_entry_pathname( entry );
		if( fnmatch( src, filename, FNM_PATHNAME | FNM_PERIOD ) )
		{
			if( archive_read_data_skip( arch_r ) != ARCHIVE_OK )
            {
                goto errout;
            }
		}
        else
        {
#ifdef DEBUG
            printf("extract:%s\n", filename );
#endif

            *dest_len = archive_entry_size( entry ); 
            if( *dest_len > 0 )
                *dest_buff = malloc( *dest_len + 10 );

            if( archive_read_data( arch_r, *dest_buff, *dest_len) < 0 ) 
                goto errout;
        }
    }

    archive_read_finish( arch_r );
    return 0;

errout:
#ifdef DEBUG
    fprintf( stderr, "errno:%d, error:%s\n", archive_errno( arch_r ), archive_error_string( arch_r ) );
#endif
    if( arch_r )
        archive_read_finish( arch_r );
    return -1;
}

/*
 * memory to file
 */
int archive_extract_file3( void *arch_buff, size_t arch_size, const char *src, char *dest )
{
    int                     ret, flags;     
    const char              *filename;
    struct archive          *arch_r = NULL, *arch_w = NULL;
    struct archive_entry    *entry;

    if( !src || !dest )
        return -1;

    arch_r = archive_read_new();
    archive_read_support_format_all( arch_r );
    archive_read_support_compression_all( arch_r );

    if( archive_read_open_memory( arch_r, arch_buff, arch_size ) != ARCHIVE_OK )
        goto errout;

    while( archive_read_next_header( arch_r, &entry ) == ARCHIVE_OK ) 
    {
        filename = archive_entry_pathname( entry );
		if( fnmatch( src, filename, FNM_PATHNAME | FNM_PERIOD ) )
		{
			if( archive_read_data_skip( arch_r ) != ARCHIVE_OK )
            {
                goto errout;
            }
		}
        else
        {
#ifdef DEBUG
            printf("extract:%s\n", filename );
#endif

            flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;
            arch_w = archive_write_disk_new();
            archive_write_disk_set_options( arch_w, flags );
            archive_write_disk_set_standard_lookup( arch_w );
            archive_entry_set_pathname( entry, dest );

            if( archive_read_extract2( arch_r, entry, arch_w ) != ARCHIVE_OK ) 
                goto errout;

            archive_write_finish( arch_w );
        }
    }

    archive_read_finish( arch_r );
    return 0;

errout:
#ifdef DEBUG
    fprintf( stderr, "%s\n", archive_error_string( arch_r ) );
#endif
    if( arch_r )
        archive_read_finish( arch_r );
    if( arch_w )
        archive_write_finish( arch_w );
    return -1;
}

/*
 * memory to memory
 */
int archive_extract_file4( void *arch_buff, size_t arch_size, const char *src,  void **dest_buff, size_t *dest_len )
{    
    int                     ret, flags;     
    const char              *filename;
    struct archive          *arch_r = NULL;
    struct archive_entry    *entry;

    if( !src )
        return -1;

    arch_r = archive_read_new();
    archive_read_support_format_all( arch_r );
    archive_read_support_compression_all( arch_r );

    if( archive_read_open_memory( arch_r, arch_buff, arch_size ) != ARCHIVE_OK )
        goto errout;

    while( archive_read_next_header( arch_r, &entry ) == ARCHIVE_OK ) 
    {
        filename = archive_entry_pathname( entry );
		if( fnmatch( src, filename, FNM_PATHNAME | FNM_PERIOD ) )
		{
			if( archive_read_data_skip( arch_r ) != ARCHIVE_OK )
            {
                goto errout;
            }
		}
        else
        {
#ifdef DEBUG
            printf("extract:%s\n", filename );
#endif

            *dest_len = archive_entry_size( entry ); 
            if( *dest_len > 0 )
                *dest_buff = malloc( *dest_len );

            if( archive_read_data( arch_r, *dest_buff, *dest_len) < 0 ) 
                goto errout;
        }
    }

    archive_read_finish( arch_r );
    return 0;

errout:
#ifdef DEBUG
    fprintf( stderr, "%s\n", archive_error_string( arch_r ) );
#endif
    if( arch_r )
        archive_read_finish( arch_r );
    return -1;

}

int archive_extract_all( char *arch_file, char *dest_dir )
{
    int                     ret, flags, path_len;     
    char                    *filename, *dest = NULL;
    struct archive          *arch_r = NULL, *arch_w = NULL;
    struct archive_entry    *entry = NULL;

    if( !arch_file )
        return -1;

    arch_r = archive_read_new();
    archive_read_support_format_all( arch_r );
    archive_read_support_compression_all( arch_r );

    if( archive_read_open_filename( arch_r, arch_file, 10240 ) != ARCHIVE_OK )
        goto errout;

    ;
    if( !( dest = (char *)malloc( MAX_PATH_LEN ) ) )
        goto errout;

    flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;
    arch_w = archive_write_disk_new();
    archive_write_disk_set_options( arch_w, flags );
    archive_write_disk_set_standard_lookup( arch_w );

    while( archive_read_next_header( arch_r, &entry ) == ARCHIVE_OK ) 
    {
        filename = (char *)archive_entry_pathname( entry );
#ifdef DEBUG
        printf("extract:%s\n", filename );
#endif

        if( dest_dir && dest_dir[0] )
        {
            if( dest_dir[0] == '/' && dest_dir[1] == '\0' && filename[0] == '/' )
                dest = filename;
            else
            {
                path_len = snprintf( dest, MAX_PATH_LEN, "%s/%s", dest_dir, filename );
                dest[path_len] = '\0';
            }
        }
        else
        {
            dest = filename;
        }

        archive_entry_set_pathname( entry, dest );
        if( archive_read_extract2( arch_r, entry, arch_w ) != ARCHIVE_OK ) 
            goto errout;
    }

    free( dest );
    archive_read_finish( arch_r );
    archive_write_finish( arch_w );
    return 0;

errout:
    if( arch_r )
        archive_read_finish( arch_r );
    if( arch_w )
        archive_write_finish( arch_w );
    if( dest )
        free( dest );
    return -1;
}
