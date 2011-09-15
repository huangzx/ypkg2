#ifndef PACKAGE_H
#define PACKAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include "download.h"
#include "util.h"
#include "db.h"
#include "data.h"
#include "archive.h"
#include "xml.h"

#define CONFIG_FILE "/etc/yget.conf"
#define PACKAGE_DB_DIR  "/var/ypkg/db"
#define DB_NAME "/var/ypkg/db/package.db"
//#define DB_NAME "/tmp/soft_manager.db"
#define UPDATE_DIR "updates"
#define LIST_FILE "updates.list"
#define LIST_DATE_FILE "updates.date"
#define PACKAGE_ATTRS_LINE_LEN 1024

#define LOCAL_UNIVERSE "/var/ypkg/packages/universe"
#define LOCAL_WORLD "/var/ypkg/db/world"

typedef struct {
    char    *source_uri;
    char    *accept_repo;
    char    *package_dest;
    char    *db_name;
}PACKAGE_MANAGER;

typedef struct {
    HASH_TABLE              *ht;
}PACKAGE;

typedef struct {
    int                     cnt;
    HASH_TABLE_LIST         *htl;
}PACKAGE_DATA;

typedef struct {
    int                     cnt;
    int                     cnt_file;
    int                     cnt_link;
    int                     cnt_dir;
    int                     size;
    HASH_TABLE_LIST         *htl;
}PACKAGE_FILE;

typedef struct {
    int                     cnt;
    HASH_TABLE_LIST         *htl;
}PACKAGE_LIST;


/**********************************/
/* sync interface                 */
/**********************************/

/*
 * init & clean up
 */
PACKAGE_MANAGER *packages_manager_init();
void packages_manager_cleanup( PACKAGE_MANAGER *pm );

/*
 * update database
 */
int packages_check_update( PACKAGE_MANAGER *pm );
int packages_update( PACKAGE_MANAGER *pm );
int packages_import_local_data( PACKAGE_MANAGER *pm );

/*
 * get package infomations
 */
int packages_get_count( PACKAGE_MANAGER *pm, char *key, char *keyword, int wildcards, int installed );
int packages_has_installed( PACKAGE_MANAGER *pm, char *name );

int packages_get_package_from_ypk( char *ypk_path, PACKAGE **package, PACKAGE_DATA **package_data );

PACKAGE *packages_get_package( PACKAGE_MANAGER *pm, char *name, int installed );
char *packages_get_package_attr( PACKAGE *ht, char *key );
char *packages_get_package_attr2( PACKAGE *ht, char *key );
void packages_free_package( PACKAGE *pkg );


PACKAGE_DATA *packages_get_package_data( PACKAGE_MANAGER *pm, char *name, int installed );
char *packages_get_package_data_attr( PACKAGE_DATA *pkg_data, int index, char *key );
char *packages_get_package_data_attr2( PACKAGE_DATA *pkg_data, int index, char *key );
void packages_free_package_data( PACKAGE_DATA *pkg_data );


/* file info */
PACKAGE_FILE *packages_get_package_file( PACKAGE_MANAGER *pm, char *name );
PACKAGE_FILE *packages_get_package_file_from_ypk( char *ypk_path );
char *packages_get_package_file_attr( PACKAGE_FILE *pkg_file, int index, char *key );
char *packages_get_package_file_attr2( PACKAGE_FILE *pkg_file, int index, char *key );
void packages_free_package_file( PACKAGE_FILE *pkg_file );

PACKAGE_LIST *packages_get_list( PACKAGE_MANAGER *pm, int limit, int offset, char *key, char *keyword, int wildcards, int installed );

PACKAGE_LIST *packages_get_list2( PACKAGE_MANAGER *pm, int page_size, int page_no, char *key, char *keyword, int wildcards, int installed );

//PACKAGE_LIST *packages_get_history_list( PACKAGE_MANAGER *pm, char *name );

PACKAGE_LIST *packages_get_list_with_data( PACKAGE_MANAGER *pm, int limit, int offset, char *key, char *keyword, int installed );

PACKAGE_LIST *packages_get_list_by_depend( PACKAGE_MANAGER *pm, int limit, int offset, char *depend, int installed );

PACKAGE_LIST *packages_get_list_by_bdepend( PACKAGE_MANAGER *pm, int limit, int offset, char *bdepend, int installed );

PACKAGE_LIST *packages_get_list_by_conflict( PACKAGE_MANAGER *pm, int limit, int offset, char *conflict, int installed );

PACKAGE_LIST *packages_get_list_by_recommended( PACKAGE_MANAGER *pm, int limit, int offset, char *recommended, int installed );

PACKAGE_LIST *packages_get_list_by_file( PACKAGE_MANAGER *pm, int limit, int offset, char *file );

char *packages_get_list_attr( PACKAGE_LIST *pkg_list, int index, char *key );
char *packages_get_list_attr2( PACKAGE_LIST *pkg_list, int index, char *key );
void packages_free_list( PACKAGE_LIST *pkg_list );


/*
 * package install & remove & update
 */
int packages_check_package( PACKAGE_MANAGER *pm, char *ypk_path );
int packages_unpack_package( PACKAGE_MANAGER *pm, char *ypk_path, char *dest_dir );
int packages_pack_package( PACKAGE_MANAGER *pm, char *source_dir, char *ypk_path );
int packages_install_package( PACKAGE_MANAGER *pm, char *package_name );
//int packages_install_history_package( PACKAGE_MANAGER *pm, char *package_name, char *yversion );
int packages_install_local_package( PACKAGE_MANAGER *pm, char *ypk_path, char *dest_dir );
int packages_remove_package( PACKAGE_MANAGER *pm, char *package_name );
//int packages_update_package( PACKAGE_MANAGER *pm );

/******************************/
/* async interface            */
/******************************/
typedef void PACKAGE_CB( void *data );
typedef struct _ASYNC_QUERY_PARAMS{
    int                 limit;
    int                 offset; 
    int                 wildcards; 
    int                 installed; 
    char                *key; 
    char                *keyword; 
    PACKAGE_MANAGER     *pm; 
    PACKAGE_CB          *cb;
}ASYNC_QUERY_PARAMS;

int packages_check_update_async( PACKAGE_MANAGER *pm,  PACKAGE_CB *cb );

int packages_update_async( PACKAGE_MANAGER *pm,  PACKAGE_CB *cb );

int packages_get_list_async( PACKAGE_MANAGER *pm, int limit, int offset, char *key, char *keyword, int wildcards, int installed, PACKAGE_CB *cb );

int packages_get_list_async2( PACKAGE_MANAGER *pm, int page_size, int page_no, char *key, char *keyword, int wildcards, PACKAGE_CB *cb );


/**********************************/
/* internal functions             */
/**********************************/
static PACKAGE_MANAGER *packages_manager_clone(  PACKAGE_MANAGER *pm );

static int packages_update_single_xml( PACKAGE_MANAGER *pm, char *xml_file, char *sum );

static int packages_get_last_check_timestamp( PACKAGE_MANAGER *pm );
static int packages_set_last_check_timestamp( PACKAGE_MANAGER *pm, int last_check );

static int packages_get_last_update_timestamp( PACKAGE_MANAGER *pm );
static int packages_set_last_update_timestamp( PACKAGE_MANAGER *pm, int last_update );

static void *packages_check_update_backend_thread( void *data );
static void *packages_update_backend_thread( void *data );
static void *packages_get_list_backend_thread(void *data);
#endif /* !PACKAGE_H */
