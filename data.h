#ifndef DATA_H
#define DATA_H

#include <stdlib.h>
#include <string.h>
////#define __USE_GNU
#include <search.h>

#define HASH_TABLE_SIZE 1024

typedef struct hsearch_data HASH_INDEX;
typedef struct _HASH_DATA {
    int                 size;
    int                 pos;
    char                *buf;
    struct _HASH_DATA   *next;
}HASH_DATA;

typedef struct {
    HASH_INDEX          *index;
    HASH_DATA           *data;
    HASH_DATA           *cur_data;
}HASH_TABLE;


typedef struct {
    int                 max_cnt;
    int                 cur_cnt;
    HASH_INDEX          **list;
    HASH_DATA           *data;
    HASH_DATA           *cur_data;
}HASH_TABLE_LIST;

HASH_TABLE *hash_table_init();
int hash_table_add_data( HASH_TABLE *ht, char *key, char *value );
char *hash_table_get_data( HASH_TABLE *ht, char *key );
void hash_table_cleanup( HASH_TABLE *ht );


HASH_TABLE_LIST *hash_table_list_init();
int hash_table_list_extend( HASH_TABLE_LIST *htl, int count );
int hash_table_list_add_data( HASH_TABLE_LIST *htl, int index, char *key, char *value );
char *hash_table_list_get_data( HASH_TABLE_LIST *htl, int index, char *key );
void hash_table_list_cleanup( HASH_TABLE_LIST *htl );


static HASH_DATA *hash_table_malloc_data( HASH_DATA *cur_data, int new_size );

#endif /* !DATA_H */
