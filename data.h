#ifndef DATA_H
#define DATA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <search.h>

#define HashTable_SIZE 1024

typedef struct hsearch_data HashIndex;
typedef struct _HashData {
    int                 size;
    int                 pos;
    char                *buf;
    struct _HashData   *next;
}HashData;

typedef struct {
    HashIndex          *index;
    HashData           *data;
    HashData           *cur_data;
}HashTable;


typedef struct {
    int                 max_cnt;
    int                 cur_cnt;
    HashIndex          **list;
    HashData           *data;
    HashData           *cur_data;
}HashTableList;

HashTable *hash_table_init();
int hash_table_add_data( HashTable *ht, char *key, char *value );
char *hash_table_get_data( HashTable *ht, char *key );
void hash_table_cleanup( HashTable *ht );


HashTableList *hash_table_list_init();
int hash_table_list_extend( HashTableList *htl, int count );
int hash_table_list_add_data( HashTableList *htl, int index, char *key, char *value );
char *hash_table_list_get_data( HashTableList *htl, int index, char *key );
void hash_table_list_cleanup( HashTableList *htl );


HashData *hash_table_malloc_data( HashData *cur_data, int new_size );

#endif /* !DATA_H */
