#ifndef XML_H
#define XML_H

#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlreader.h>
#include <libxml/hash.h>

/* #define DEBUG 1 */

/**
 * xPath
 */
xmlDocPtr xpath_open( char *docname );
xmlDocPtr xpath_open2( char *buffer, int size );
xmlXPathObjectPtr xpath_get_nodeset( xmlDocPtr doc, xmlChar *xpath );
char *xpath_get_node( xmlDocPtr doc, xmlChar *xpath );



/**
 * xmlTextReader
 */
#define HASH_TABLE_SIZE 32
#define HASH_FULL_KEY_LEN 64
typedef struct _XML_READER_HANDLE{
    xmlTextReaderPtr    reader;
    xmlHashTablePtr     ht;
}XML_READER_HANDLE;

int reader_open( char *docname,  XML_READER_HANDLE *handle);

int reader_fetch_a_row( XML_READER_HANDLE *handle, int target_depth, char **attr_list );

static int reader_fetch_fields( XML_READER_HANDLE *handle, int node_depth, char *prefix, char **attr_list );

static void hash_data_cleanup( void *data, xmlChar *key );

static void reader_hash_cleanup( XML_READER_HANDLE *handle );

void reader_cleanup( XML_READER_HANDLE *handle );

char *reader_get_value( XML_READER_HANDLE *handle, char *key );

char *reader_get_value2( XML_READER_HANDLE *handle, char *key );

#endif /* !XML_H */
