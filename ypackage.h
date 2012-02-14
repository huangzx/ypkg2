/* Libypk
 *
 * Copyright (c) 2011-2012 Ylmf OS
 *
 * Written by: 0o0 <0o0@115.com> <0o0zzyz@gmail.com>
 * Version: 0.1
 * Date: 2012.1.11
 */
#ifndef PACKAGE_H
#define PACKAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/utsname.h>

#ifdef LIBYPK 

#include "download.h"
#include "util.h"
#include "db.h"
#include "data.h"
#include "archive.h"
#include "xml.h"
#include "preg.h"

#else

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
#endif

#define CONFIG_FILE "/etc/yget.conf"
#define PACKAGE_DB_DIR  "/var/ypkg/db"
#define DB_NAME "/var/ypkg/db/package.db"
#define LOG_FILE "/var/log/ypkg2.log"
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
    char    *log;
}YPackageManager;

typedef struct {
    HashTable              *ht;
}YPackage;

typedef struct {
    int                     cnt;
    HashTableList         *htl;
}YPackageData;

typedef struct {
    int                     cnt;
    int                     cnt_file;
    int                     cnt_link;
    int                     cnt_dir;
    int                     size;
    HashTableList         *htl;
}YPackageFile;

typedef struct {
    int                     cnt;
    HashTableList         *htl;
}YPackageList;

typedef struct _YPackageChangeList {
    char                    *name;
    char                    *version;
    int                     size;
    int                     type; //self:1 ,depend:2, recommended:3, upgrade: 4, downgrade: 5
    struct _YPackageChangeList    *prev;
}YPackageChangeList;


/*
typedef int (*ypk_download_callback)( void *cb_arg, double dltotal, double dlnow );
typedef struct {
    ypk_download_callback   cb;
    void                    *arg;
}YPackageDCB;
*/

typedef int (*ypk_download_callback)( void *cb_arg, char *package_name, double dltotal, double dlnow );
typedef int (*ypk_progress_callback)( void *cb_arg, char *package_name, int action, double progress, char *msg );
typedef struct {
    char                    *package_name;
    ypk_progress_callback   pcb;
    void                    *pcb_arg;
    ypk_download_callback   dcb;
    void                    *dcb_arg;
}YPackageDCB;




/**********************************/
/* sync interface                 */
/**********************************/

/*
 * init & clean up
 */
YPackageManager *packages_manager_init();
void packages_manager_cleanup( YPackageManager *pm );

/*
 * update database
 */
int packages_check_update( YPackageManager *pm );

int packages_update( YPackageManager *pm, ypk_progress_callback cb, void *cb_arg );

static int packages_update_single_xml( YPackageManager *pm, char *xml_file, char *sum,  ypk_progress_callback cb, void *cb_arg  );

int packages_import_local_data( YPackageManager *pm );


/*
 * get package infomations
 */

/* get a single package infomations */
int packages_get_count( YPackageManager *pm, char *key, char *keyword, int wildcards, int installed );
int packages_has_installed( YPackageManager *pm, char *name, char *version );
int packages_exists( YPackageManager *pm, char *name, char *version );

int packages_get_package_from_ypk( char *ypk_path, YPackage **package, YPackageData **package_data );

YPackage *packages_get_package( YPackageManager *pm, char *name, int installed );
char *packages_get_package_attr( YPackage *pkg, char *key );
char *packages_get_package_attr2( YPackage *pkg, char *key );
void packages_free_package( YPackage *pkg );


YPackageData *packages_get_package_data( YPackageManager *pm, char *name, int installed );
char *packages_get_package_data_attr( YPackageData *pkg_data, int index, char *key );
char *packages_get_package_data_attr2( YPackageData *pkg_data, int index, char *key );
void packages_free_package_data( YPackageData *pkg_data );


/* get package file infomations */
YPackageFile *packages_get_package_file( YPackageManager *pm, char *name );
YPackageFile *packages_get_package_file_from_ypk( char *ypk_path );
char *packages_get_package_file_attr( YPackageFile *pkg_file, int index, char *key );
char *packages_get_package_file_attr2( YPackageFile *pkg_file, int index, char *key );
void packages_free_package_file( YPackageFile *pkg_file );

/* get package list */
YPackageList *packages_get_list( YPackageManager *pm, int limit, int offset, char *key, char *keyword, int wildcards, int installed );

YPackageList *packages_get_list2( YPackageManager *pm, int page_size, int page_no, char *key, char *keyword, int wildcards, int installed );

//YPackageList *packages_get_history_list( YPackageManager *pm, char *name );

YPackageList *packages_get_list_with_data( YPackageManager *pm, int limit, int offset, char *key, char *keyword, int installed );

YPackageList *packages_get_list_by_depend( YPackageManager *pm, int limit, int offset, char *depend, int installed );

YPackageList *packages_get_list_by_bdepend( YPackageManager *pm, int limit, int offset, char *bdepend, int installed );

YPackageList *packages_get_list_by_conflict( YPackageManager *pm, int limit, int offset, char *conflict, int installed );

YPackageList *packages_get_list_by_recommended( YPackageManager *pm, int limit, int offset, char *recommended, int installed );

YPackageList *packages_get_list_by_file( YPackageManager *pm, int limit, int offset, char *file );

char *packages_get_list_attr( YPackageList *pkg_list, int index, char *key );
char *packages_get_list_attr2( YPackageList *pkg_list, int index, char *key );
void packages_free_list( YPackageList *pkg_list );


/*
 * compare version
 */
int packages_compare_version( char *version1, char *version2 );

/*
 * package install & remove & upgrade
 */
int packages_check_package( YPackageManager *pm, char *ypk_path, char *extra, int extra_max_len );

int packages_unpack_package( YPackageManager *pm, char *ypk_path, char *dest_dir, int unzip_info );
int packages_pack_package( YPackageManager *pm, char *source_dir, char *ypk_path );


//int packages_download_package( YPackageManager *pm, char *package_name, char *url, char *dest, int force, ypk_progress_callback cb, void *cb_arg );
int packages_download_package( YPackageManager *pm, char *package_name, char *url, char *dest, int force, ypk_download_callback dcb, void *dcb_arg, ypk_progress_callback pcb, void *pcb_arg  );
int packages_exec_script( char *script, char *package_name, char *version, char *version2, char *action );
int packages_install_local_package( YPackageManager *pm, char *ypk_path, char *dest_dir, int force, ypk_progress_callback cb, void *cb_arg );
int packages_install_package( YPackageManager *pm, char *package_name, char *version, ypk_progress_callback cb, void *cb_arg  );
//int packages_install_history_package( YPackageManager *pm, char *package_name, char *version );

YPackageChangeList *packages_get_install_list( YPackageManager *pm, char *package_name, char *version );

YPackageChangeList *packages_get_depend_list( YPackageManager *pm, char *package_name,char *version );

YPackageChangeList *packages_get_recommended_list( YPackageManager *pm, char *package_name, char *version );

YPackageChangeList *packages_get_bdepend_list( YPackageManager *pm, char *package_name, char *version );

YPackageChangeList *packages_remove_duplicate_item( YPackageChangeList *change_list );

void packages_free_install_list( YPackageChangeList *list );


YPackageChangeList *packages_get_dev_list( YPackageManager *pm, char *package_name, char *version );

void packages_free_dev_list( YPackageChangeList *list );
int packages_install_list( YPackageManager *pm, YPackageChangeList *list, ypk_progress_callback cb, void *cb_arg  );


int packages_remove_package( YPackageManager *pm, char *package_name, ypk_progress_callback cb, void *cb_arg  );
YPackageChangeList *packages_get_remove_list( YPackageManager *pm, char *package_name );
int packages_remove_list( YPackageManager *pm, YPackageChangeList *list, ypk_progress_callback cb, void *cb_arg  );
void packages_free_remove_list( YPackageChangeList *list );

YPackageChangeList *packages_get_upgrade_list( YPackageManager *pm );
int packages_upgrade_list( YPackageManager *pm, YPackageChangeList *list, ypk_progress_callback cb, void *cb_arg   );
void packages_free_upgrade_list( YPackageChangeList *list );

int packages_cleanup_package( YPackageManager *pm );


/*
 * log
 */
int packages_log( YPackageManager *pm, char *package_name, char *msg );

/******************************/
/* async interface            */
/******************************/
typedef void YPackageCB( void *data );
typedef struct {
    int                 limit;
    int                 offset; 
    int                 wildcards; 
    int                 installed; 
    char                *key; 
    char                *keyword; 
    YPackageManager     *pm; 
    YPackageCB          *cb;
}AsyncQueryParams;

int packages_check_update_async( YPackageManager *pm,  YPackageCB *cb );

int packages_update_async( YPackageManager *pm,  YPackageCB *cb );

int packages_get_list_async( YPackageManager *pm, int limit, int offset, char *key, char *keyword, int wildcards, int installed, YPackageCB *cb );

int packages_get_list_async2( YPackageManager *pm, int page_size, int page_no, char *key, char *keyword, int wildcards, YPackageCB *cb );


/**********************************/
/* internal functions             */
/**********************************/
static int packages_compare_main_version( char *version1, char *version2 );
static int packages_compare_sub_version( char *version1, char *version2 );

static YPackageManager *packages_manager_clone(  YPackageManager *pm );


static int packages_get_last_check_timestamp( YPackageManager *pm );
static int packages_set_last_check_timestamp( YPackageManager *pm, int last_check );

static int packages_get_last_update_timestamp( YPackageManager *pm );
static int packages_set_last_update_timestamp( YPackageManager *pm, int last_update );


static void packages_free_change_list( YPackageChangeList *list );

static void *packages_check_update_backend_thread( void *data );
static void *packages_update_backend_thread( void *data );
static void *packages_get_list_backend_thread(void *data);
static int packages_download_progress_callback( void *cb_arg,double dltotal, double dlnow, double ultotal, double ulnow );
#endif /* !PACKAGE_H */
