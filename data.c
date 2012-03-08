#include "data.h"


HashTable *hash_table_init()
{
    int                 buf_size;
    HashTable          *ht;


    ht = (HashTable *)malloc( sizeof( HashTable ) );
    if( !ht )
        return NULL;

    ht->index = (HashIndex *)malloc( sizeof( HashIndex ) );
    if( !ht->index )
    {
        free( ht );
        return NULL;
    }
    memset( ht->index, '\0',  sizeof( HashIndex )  );
    hcreate_r( 24, ht->index );

    buf_size = HashTable_SIZE;
    ht->data = hash_table_malloc_data( NULL, buf_size );
    if( !ht->data )
    {
        free( ht->index );
        free( ht );
        return NULL;
    }
    ht->cur_data = ht->data;

    return ht;
}

int hash_table_add_data( HashTable *ht, char *key, char *value )
{
    int             len, buf_size, ret;
    ENTRY           item, *itemp;
    HashData       *new_data;

    if( !ht || !key )
        return -1;

    if( value  )
    {
        len = strlen( value );
        if( ht->cur_data->pos + len + 1 > ht->cur_data->size )
        {
            buf_size = HashTable_SIZE;
            new_data = hash_table_malloc_data( ht->cur_data, buf_size );
            if( !new_data )
                return -1;
            ht->cur_data = new_data;

        }

        memcpy( ht->cur_data->buf + ht->cur_data->pos, value, len );
        ht->cur_data->buf[ht->cur_data->pos + len] = '\0';
        item.key = key;
        item.data = (void *)(ht->cur_data->buf + ht->cur_data->pos);
        ret = hsearch_r( item, ENTER, &itemp, ht->index );
        ht->cur_data->pos += len + 1;
    }
    else
    {
        item.key = key;
        item.data = NULL;
        ret = hsearch_r( item, ENTER, &itemp, ht->index );
    }
    return 0;
}


char *hash_table_get_data( HashTable *ht, char *key )
{
    int     ret;
    ENTRY   item, *itemp;

    if( !ht || !key )
        return NULL;

    item.key = key;
    item.data = NULL;
    ret = hsearch_r( item, FIND, &itemp, ht->index );

    return itemp ? itemp->data : NULL;
}

void hash_table_cleanup( HashTable *ht )
{
    HashData   *cur_data, *next_data;

    if( !ht )
        return;

    if( ht->index )
    {
        hdestroy_r( ht->index );
        free( ht->index );
    }

    if( ht->data );
    {
        cur_data = ht->data;

        while( cur_data )
        {
            if( cur_data->buf )
                free( cur_data->buf );

            next_data = cur_data->next; 
            free( cur_data );
            cur_data = next_data;
        }
    }

    free( ht );
}


HashTableList *hash_table_list_init( int count )
{
    int                 buf_size;
    HashTableList     *htl;

    if( count < 1 )
        return NULL;

    htl = (HashTableList *)malloc( sizeof( HashTableList ) );
    if( !htl )
        return NULL;

    htl->max_cnt = count;
    htl->cur_cnt = 0;

    htl->list = (HashIndex **)malloc( sizeof( HashIndex * ) * count );
    if( !htl->list )
    {
        free( htl );
        return NULL;
    }
    memset( htl->list, '\0',  sizeof( HashIndex * ) * count );

    buf_size = HashTable_SIZE * count;
    htl->data = hash_table_malloc_data( NULL, buf_size );
    if( !htl->data )
    {
        free( htl->list );
        free( htl );
        return NULL;
    }
    htl->cur_data = htl->data;

    return htl;
}

int hash_table_list_extend( HashTableList *htl, int count )
{
    HashIndex      **new_list;

    if( !htl )
        return -1;

    if( htl->max_cnt > count )
        return -1;

    new_list = (HashIndex **)realloc( htl->list, sizeof( HashIndex * ) * count );
    if( !new_list )
    {
        return -1;
    }
    memset( &(new_list[htl->max_cnt]), '\0',  sizeof( HashIndex * ) * ( count - htl->max_cnt ) );
    htl->max_cnt = count;
    htl->list = new_list;

    return 0;
}

int hash_table_list_add_data( HashTableList *htl, int index, char *key, char *value )
{
    int             len, buf_size;
    ENTRY           item, *itemp;
    HashIndex      *cur_index;
    HashData       *new_data;

    if( !htl || !key || index < 0 )
        return -1;

    if( index > htl->max_cnt - 1 )
        return -1;

    if( index + 1 > htl->cur_cnt )
        htl->cur_cnt = index + 1;

    if( htl->list[index] == NULL )
    {
        cur_index = (HashIndex *)malloc( sizeof( HashIndex ) );
        if( !cur_index )
            return -1;

        memset( cur_index, '\0',  sizeof( HashIndex )  );
        hcreate_r( 24, cur_index );
        htl->list[index] = cur_index;
    }

    if( value  )
    {
        len = strlen( value );
        if( htl->cur_data->pos + len + 1 > htl->cur_data->size )
        {
            if( htl->cur_cnt > htl->max_cnt * 3 / 4 )
                buf_size = HashTable_SIZE * htl->max_cnt / 3;
            else if( htl->cur_cnt > htl->max_cnt * 2 / 3 )
                buf_size = HashTable_SIZE * htl->max_cnt / 2;
            else if( htl->cur_cnt > htl->max_cnt / 2 )
                buf_size = HashTable_SIZE * htl->max_cnt;
            else
                buf_size = HashTable_SIZE * htl->max_cnt * 2;

            new_data = hash_table_malloc_data( htl->cur_data, buf_size );
            if( !new_data )
                return -1;
            htl->cur_data = new_data;

        }

        memcpy( htl->cur_data->buf + htl->cur_data->pos, value, len );
        htl->cur_data->buf[htl->cur_data->pos + len] = '\0';
        item.key = key;
        item.data = (void *)(htl->cur_data->buf + htl->cur_data->pos);
        hsearch_r( item, ENTER, &itemp, htl->list[index] );
        htl->cur_data->pos += len + 1;
    }
    else
    {
        item.key = key;
        item.data = NULL;
        hsearch_r( item, ENTER, &itemp, htl->list[index] );
    }
    return 0;
}


char *hash_table_list_get_data( HashTableList *htl, int index, char *key )
{
    ENTRY   item, *itemp;

    if( !htl || !key || index < 0 )
        return NULL;

    if( index > htl->cur_cnt - 1 )
        return NULL;

    item.key = key;
    item.data = NULL;
    hsearch_r( item, FIND, &itemp, htl->list[index] );

    return itemp ? itemp->data : NULL;
}

void hash_table_list_cleanup( HashTableList *htl )
{
    int         i;
    HashData   *cur_data, *next_data;

    if( !htl )
        return;

    if( htl->list )
    {
        for( i = 0; i < htl->cur_cnt; i++ )
        {
            if( htl->list[i] )
            {
                hdestroy_r( htl->list[i] );
                free( htl->list[i] );
            }
        }
        free( htl->list );
    }

    if( htl->data );
    {
        cur_data = htl->data;

        while( cur_data )
        {
            if( cur_data->buf )
                free( cur_data->buf );

            next_data = cur_data->next; 
            free( cur_data );
            cur_data = next_data;
        }
    }

    free( htl );
}


HashData *hash_table_malloc_data( HashData *cur_data, int new_size )
{
    HashData *new_data;

    if( new_size < 1 )
        return NULL;

    new_data = (HashData *)malloc( sizeof( HashData ) );
    if( !new_data )
    {
        return NULL;
    }

    new_data->next = NULL;
    new_data->size = new_size;
    new_data->pos = 0;
    new_data->buf = (char *)malloc( new_size );
    if( !new_data->buf )
    {
        free( new_data );
        return NULL;
    }
    memset( new_data->buf, '\0',  new_size  );
    if( cur_data )
        cur_data->next = new_data;

    return new_data;
}

