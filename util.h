#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>



#define MAXFMTLEN 16 
#define MAXKWLEN 32 
#define MAXCFGLINE 128



/*
 * config
 */
char *util_get_config(char *config_file, char *keyword);

/*
 * string
 */
char    *util_rtrim( char *str, char c );
char    *util_mem_gets( char *mem );
char    *util_strcpy( char *src );
char    *util_strcat(char *first, ...);
char    *util_strcat2( char *dest, int size, char *first, ...);
char    *util_int_to_str( int i );
char    *util_time_to_str( time_t time );
int     util_ends_with( char *str, char *suffix );


/*
 * log
 */
void util_log( char *log, char *msg );


/*
 * file
 */
int util_remove_dir( char *dir_path );
int util_remove_files( char *dir_path, char *suffix );

#endif /* !UTIL_H */
