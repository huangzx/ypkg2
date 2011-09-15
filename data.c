#include "data.h"


HASH_TABLE *hash_table_init()
{
    int                 buf_size;
    HASH_TABLE          *ht;


    ht = (HASH_TABLE *)malloc( sizeof( HASH_TABLE ) );
    if( !ht )
        return NULL;

    ht->index = (HASH_INDEX *)malloc( sizeof( HASH_INDEX ) );
    if( !ht->index )
    {
        free( ht );
        return NULL;
    }
    memset( ht->index, '\0',  sizeof( HASH_INDEX )  );
    hcreate_r( 8, ht->index );

    buf_size = HASH_TABLE_SIZE;
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

int hash_table_add_data( HASH_TABLE *ht, char *key, char *value )
{
    int             len, buf_size;
    ENTRY           item, *itemp;
    HASH_INDEX      *cur_index;
    HASH_DATA       *new_data;

    if( !ht || !key )
        return -1;

    if( value  )
    {
        len = strlen( value );
        if( ht->cur_data->pos + len + 1 > ht->cur_data->size )
        {
            buf_size = HASH_TABLE_SIZE;
            new_data = hash_table_malloc_data( ht->cur_data, buf_size );
            if( !new_data )
                return -1;
            ht->cur_data = new_data;

        }

        memcpy( ht->cur_data->buf + ht->cur_data->pos, value, len );
        ht->cur_data->buf[ht->cur_data->pos + len] = '\0';
        item.key = key;
        item.data = (void *)(ht->cur_data->buf + ht->cur_data->pos);
        hsearch_r( item, ENTER, &itemp, ht->index );
        ht->cur_data->pos += len + 1;
    }
    else
    {
        item.key = key;
        item.data = NULL;
        hsearch_r( item, ENTER, &itemp, ht->index );
    }
    return 0;
}


char *hash_table_get_data( HASH_TABLE *ht, char *key )
{
    ENTRY   item, *itemp;

    if( !ht || !key )
        return NULL;

    item.key = key;
    item.data = NULL;
    hsearch_r( item, FIND, &itemp, ht->index );

    return itemp ? itemp->data : NULL;
}

void hash_table_cleanup( HASH_TABLE *ht )
{
    HASH_DATA   *cur_data, *next_data;

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


HASH_TABLE_LIST *hash_table_list_init( int count )
{
    int                 buf_size;
    HASH_TABLE_LIST     *htl;

    if( count < 1 )
        return NULL;

    htl = (HASH_TABLE_LIST *)malloc( sizeof( HASH_TABLE_LIST ) );
    if( !htl )
        return NULL;

    htl->max_cnt = count;
    htl->cur_cnt = 0;

    htl->list = (HASH_INDEX **)malloc( sizeof( HASH_INDEX * ) * count );
    if( !htl->list )
    {
        free( htl );
        return NULL;
    }
    memset( htl->list, '\0',  sizeof( HASH_INDEX * ) * count );

    buf_size = HASH_TABLE_SIZE * count;
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

int hash_table_list_extend( HASH_TABLE_LIST *htl, int count )
{
    HASH_INDEX      **new_list;

    if( !htl )
        return -1;

    if( htl->max_cnt > count )
        return -1;

    new_list = (HASH_INDEX **)realloc( htl->list, sizeof( HASH_INDEX * ) * count );
    if( !new_list )
    {
        return -1;
    }
    memset( &(new_list[htl->max_cnt]), '\0',  sizeof( HASH_INDEX * ) * ( count - htl->max_cnt ) );
    htl->max_cnt = count;
    htl->list = new_list;

    return 0;
}

int hash_table_list_add_data( HASH_TABLE_LIST *htl, int index, char *key, char *value )
{
    int             len, buf_size;
    ENTRY           item, *itemp;
    HASH_INDEX      *cur_index;
    HASH_DATA       *new_data;

    if( !htl || !key || index < 0 )
        return -1;

    if( index > htl->max_cnt - 1 )
        return -1;

    if( index + 1 > htl->cur_cnt )
        htl->cur_cnt = index + 1;

    if( htl->list[index] == NULL )
    {
        cur_index = (HASH_INDEX *)malloc( sizeof( HASH_INDEX ) );
        if( !cur_index )
            return -1;

        memset( cur_index, '\0',  sizeof( HASH_INDEX )  );
        hcreate_r( 8, cur_index );
        htl->list[index] = cur_index;
    }

    if( value  )
    {
        len = strlen( value );
        if( htl->cur_data->pos + len + 1 > htl->cur_data->size )
        {
            if( htl->cur_cnt > htl->max_cnt * 3 / 4 )
                buf_size = HASH_TABLE_SIZE * htl->max_cnt / 3;
            else if( htl->cur_cnt > htl->max_cnt * 2 / 3 )
                buf_size = HASH_TABLE_SIZE * htl->max_cnt / 2;
            else if( htl->cur_cnt > htl->max_cnt / 2 )
                buf_size = HASH_TABLE_SIZE * htl->max_cnt;
            else
                buf_size = HASH_TABLE_SIZE * htl->max_cnt * 2;

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


char *hash_table_list_get_data( HASH_TABLE_LIST *htl, int index, char *key )
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

void hash_table_list_cleanup( HASH_TABLE_LIST *htl )
{
    int         i;
    HASH_DATA   *cur_data, *next_data;

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


static HASH_DATA *hash_table_malloc_data( HASH_DATA *cur_data, int new_size )
{
    HASH_DATA *new_data;

    if( new_size < 1 )
        return NULL;

    new_data = (HASH_DATA *)malloc( sizeof( HASH_DATA ) );
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

