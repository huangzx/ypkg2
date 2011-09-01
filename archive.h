#ifndef ZIP_H
#define ZIP_H

#define MAX_PATH_LEN 1024

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <archive.h>
#include <archive_entry.h>
#include <fnmatch.h>


/*
 * extract a single file (file to file)
 */
int archive_extract_file( char *arch_file, const char *src, char *dest );

/*
 * extract a single file (file to memory)
 */
int archive_extract_file2( char *arch_file, const char *src, void **dest_buff, size_t *dest_len );

/*
 * extract a single file (memory to file)
 */
int archive_extract_file3( void *arch_buff, size_t arch_size, const char *src, char *dest );

/*
 * extract a single file (memory to memory)
 */
int archive_extract_file4( void *arch_buff, size_t arch_size, const char *src,  void **dest_buff, size_t *dest_len );


/*
 * extract all files of the archive
 */
int archive_extract_all( char *filename, char *dest_dir );

/*
 * pack a dir to a archive
 */
int archive_extract_all( char *filename, char *dest_dir );

#endif /* !ZIP_H */
