#include "util.h"

char *util_get_config(char *config_file, char *keyword)
{
    int             n, match, len;
    FILE            *fp;
    char            *pos, *result;
    char            line[MAXCFGLINE], keybuf[MAXKWLEN], pattern[MAXFMTLEN], valbuf[MAXCFGLINE];

    if ((fp = fopen(config_file, "r")) == NULL)
        return NULL;
    match = 0;
    while (fgets(line, MAXCFGLINE, fp) != NULL) 
    {
        pos = strchr(line, '=');
        sprintf(pattern, "%%%ds%%*2s%%%ds", pos-line, MAXCFGLINE-1);
        n = sscanf(line, pattern, keybuf, valbuf);
        if (n == 2 && strcmp(keyword, keybuf) == 0) 
        {
            match = 1;
            break;
        }
    }
    fclose(fp);
    if (match != 0)
    {
        len = strlen(valbuf);
        valbuf[len - 1] = '\0';
        result = malloc(len);
        strncpy(result, valbuf, len);
        return result;
    }
    else 
        return NULL;
}


char *util_rtrim(char *str)
{
    int i;

    for(i = strlen(str)-1;  i >= 0; i--)
    {
        if(str[i] == '\r' || str[i] == '\n' || str[i] == ' ' || str[i] == '\t' || str[i] == '\x0B' )
            str[i] = '\0';
        else
            break;
    }
    return str;
}

char *util_mem_gets( char *mem )
{
    int             len;
    char            *pos, *str;
    static int      cur_len;
    static char     *cur_pos;

    if( mem )
    {
        cur_pos = mem;
        cur_len = strlen( cur_pos );
        if( cur_len < 1 )
        {
            cur_pos = NULL;
            cur_len = 0;
            return NULL;
        }
    }
    else if( !cur_pos || cur_len < 1 )
    {
        return NULL;
    }

    pos = strchr( cur_pos, '\n' );
    if( !pos )
        len = cur_len;
    else
        len = pos - cur_pos;

    str = malloc( len + 1 );
    strncpy( str, cur_pos, len );
    str[len] = '\0';
    cur_pos = pos + 1;
    cur_len -= len + 1;

    return str;
}

char *util_strcpy( char *src )
{
    int     len;
    char    *dest;

    if( !src )
        return NULL;

    len = strlen( src );
    dest = malloc( len + 1 );
    memset( dest, '\0', len + 1);
    strncpy( dest, src, len + 1);
    
    return dest;
}

char *util_strcat(char *first, ...)
{
    va_list ap;
    char    *arg, *result;
    int     len;

    if( !first )
        return NULL;

    len = strlen( first );
    va_start( ap, first );
    while( ( arg = va_arg( ap, char * ) ) != NULL )
    {
        len += strlen( arg );
    }
    va_end(ap);

    result = malloc( len + 1 );
    memset( result, '\0', len + 1);
    strncpy( result, first, strlen( first ) );

    va_start( ap, first );
    while( ( arg = va_arg( ap, char * ) ) != NULL )
    {
        strncat( result, arg, strlen( arg ) );
    }
    va_end(ap);

    result[len] = '\0';

    return result;
}

char *util_strcat2( char *dest, int size, char *first, ...)
{
    va_list ap;
    char    *arg, *result;
    int     len;

    len = strlen( first );
    va_start( ap, first );
    while( ( arg = va_arg( ap, char * ) ) != NULL )
    {
        len += strlen( arg );
    }
    va_end(ap);

    if( len + 1 > size )
        return NULL;

    result = dest;
    memset( result, '\0', len + 1);
    strncpy( result, first, strlen( first ) );

    va_start( ap, first );
    while( ( arg = va_arg( ap, char * ) ) != NULL )
    {
        strncat( result, arg, strlen( arg ) );
    }
    va_end(ap);

    result[len] = '\0';

    return result;
}

char *util_int_to_str( int i )
{
    char *result;

    result = malloc( 11 );
    snprintf( result, 11, "%ld", i );
    result[10] = '\0';

    return result;
}

void util_log( char *log, char *msg )
{
    FILE *fp;

    fp = fopen( log, "a" );
    fprintf( fp, msg );
    fclose( fp );
}

int util_remove_dir( char *dir_path )
{
    int             ret;
    char            *file_path;
    DIR             *dir;
    struct dirent   *entry;
    struct stat     statbuf;


    dir = opendir( dir_path );
    if( !dir )
        return -1;

    while( entry = readdir( dir ) )
    {
        if( !strcmp( entry->d_name, "." ) || !strcmp( entry->d_name, ".." ) )
        {
            continue;
        }

        file_path = util_strcat( dir_path, "/", entry->d_name, NULL );
        if( !stat( file_path, &statbuf ) )
        {
            if( S_ISDIR( statbuf.st_mode ) )
            {
                ret = util_remove_dir( file_path );
            }
            else
            {
                ret = unlink( file_path );
            }
        }
        free( file_path );
        if( ret )
        {
            closedir( dir );
            return -1;
        }
    }
    closedir( dir );

    return rmdir( dir_path );
}
