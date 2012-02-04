/* Libypk
 *
 * Copyright (c) 2011-2012 Ylmf OS
 *
 * Written by: 0o0 <0o0@115.com> <0o0zzyz@gmail.com>
 * Version: 0.1
 * Date: 2012.2.3
 */
#define LIBYPK 1
#include "ypackage.h"


YPackageManager *packages_manager_init()
{
    char *config_file;
    YPackageManager *pm;

    pm = malloc( sizeof(YPackageManager) );

    if( !pm )
        return NULL;

    if( access( CONFIG_FILE, R_OK ) )
        return NULL;

    if( access( DB_NAME, R_OK ) )
        return NULL;

    config_file = CONFIG_FILE;

    pm->source_uri = util_get_config( config_file, "YPPATH_URI" );
    if( !pm->source_uri )
        return NULL;

    pm->accept_repo = util_get_config( config_file, "ACCEPT_REPO" );
    pm->package_dest = util_get_config( config_file, "YPPATH_PKGDEST" );
    pm->db_name = strdup( DB_NAME );
    pm->log = util_get_config( config_file, "LOG" );

    return pm;
}

void packages_manager_cleanup( YPackageManager *pm )
{
    if( !pm )
        return;

    if( pm->source_uri )
        free( pm->source_uri );

    if( pm->accept_repo )
        free( pm->accept_repo );

    if( pm->package_dest )
        free( pm->package_dest );

    if( pm->db_name )
        free( pm->db_name );

    if( pm->log )
        free( pm->log );

    free( pm );
}


int packages_check_update( YPackageManager *pm )
{
    int             timestamp, last_check;
    char            *target_url, *list_date_file;
    DownloadContent  content;

    last_check = packages_get_last_check_timestamp( pm );
    if( last_check == 1 )
        return 1;

    //download updates date
    list_date_file = UPDATE_DIR "/" LIST_DATE_FILE;
    target_url = util_strcat( pm->source_uri, "/", list_date_file, NULL );
    content.text = malloc(1);
    content.size = 0;
    if( get_content( target_url, &content ) != 0 )
    {
        free(content.text);
        free(target_url);
        target_url = NULL;
        return -1; 
    }

    util_rtrim( content.text, 0 );
    timestamp = atoi( content.text );

    //cmp date
    if( timestamp > last_check )
    {
        packages_set_last_check_timestamp( pm, timestamp );
        free(content.text);
        free(target_url);
        target_url = NULL;
        return 1; 
    }

    free(content.text);
    free(target_url);
    target_url = NULL;
    return 0;
}

int packages_import_local_data( YPackageManager *pm )
{
    int                 xml_ret, db_ret, is_desktop, i, list_len;
    char                xml_value, *sql, *sql_history, *sql_data, *sql_history_data, *sql_filelist, *idx, *data_key, *file_path, *file_path_sub, *list_line;
    char                *package_name, *version, *install_time, *install_size, *file_type, *file_file, *file_size, *file_perms, *file_uid, *file_gid, *file_mtime, *file_extra, *last_update;
    char                *xml_attrs[] = {"name", "type", "lang", "id", NULL};
    struct stat         statbuf, statbuf_sub;
    struct dirent       *entry, *entry_sub;
    DIR                 *dir, *dir_sub;
    FILE                *fp;
    XMLReaderHandle   xml_handle;
    DB                  db;

    //init
    db_init( &db, pm->db_name, OPEN_WRITE );
    db_exec( &db, "begin", NULL );  

    printf( "Start ...\n" );


    printf( "Import universe  ...\n" );
    //import universe
    reader_open( LOCAL_UNIVERSE,  &xml_handle );
    sql = "replace into universe (name, yversion, generic_name, is_desktop, category, arch, version, priority, install, license, homepage, repo, size, sha, build_date, packager, uri, description, data_count) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sql_history = "replace into universe_history (name, yversion, generic_name, is_desktop, category, arch, version, priority, install, license, homepage, repo, size, sha, build_date, packager, uri, description, data_count) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    while( ( xml_ret = reader_fetch_a_row( &xml_handle, 1, xml_attrs ) ) == 1 )
    {
        is_desktop = (int)reader_get_value( &xml_handle, "genericname|desktop|keyword|en" );
        package_name = reader_get_value2( &xml_handle, "name" );
        version = reader_get_value2( &xml_handle, "version" );

        //universe
        db_ret = db_exec( &db, sql,  
                package_name, //name
                reader_get_value2( &xml_handle, "yversion" ), //yversion
                is_desktop ? reader_get_value2( &xml_handle, "genericname|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "genericname|keyword|en" ), //generic_name
                is_desktop ? "1" : "0", //desktop
                reader_get_value2( &xml_handle, "category" ), //category
                reader_get_value2( &xml_handle, "arch" ), //arch
                version, //version
                reader_get_value2( &xml_handle, "priority" ), //priority
                reader_get_value2( &xml_handle, "install" ), //install
                reader_get_value2( &xml_handle, "license" ), //license
                reader_get_value2( &xml_handle, "homepage" ), //homepage
                reader_get_value2( &xml_handle, "repo" ), //repo
                reader_get_value2( &xml_handle, "size" ), //size
                reader_get_value2( &xml_handle, "sha" ), //sha
                reader_get_value2( &xml_handle, "build_date" ), //build_date
                reader_get_value2( &xml_handle, "packager" ), //packager
                reader_get_value2( &xml_handle, "uri" ), //uri
                is_desktop ? reader_get_value2( &xml_handle, "description|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "description|keyword|en" ), //description
                reader_get_value2( &xml_handle, "data_count" ), //data_count
                NULL);

        //universe_history
        db_ret = db_exec( &db, sql_history,  
                package_name, //name
                reader_get_value2( &xml_handle, "yversion" ), //yversion
                is_desktop ? reader_get_value2( &xml_handle, "genericname|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "genericname|keyword|en" ), //generic_name
                is_desktop ? "1" : "0", //desktop
                reader_get_value2( &xml_handle, "category" ), //category
                reader_get_value2( &xml_handle, "arch" ), //arch
                version, //version
                reader_get_value2( &xml_handle, "priority" ), //priority
                reader_get_value2( &xml_handle, "install" ), //install
                reader_get_value2( &xml_handle, "license" ), //license
                reader_get_value2( &xml_handle, "homepage" ), //homepage
                reader_get_value2( &xml_handle, "repo" ), //repo
                reader_get_value2( &xml_handle, "size" ), //size
                reader_get_value2( &xml_handle, "sha" ), //sha
                reader_get_value2( &xml_handle, "build_date" ), //build_date
                reader_get_value2( &xml_handle, "packager" ), //packager
                reader_get_value2( &xml_handle, "uri" ), //uri
                is_desktop ? reader_get_value2( &xml_handle, "description|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "description|keyword|en" ), //description
                reader_get_value2( &xml_handle, "data_count" ), //data_count
                NULL);
    
        //universe_data
        db_exec( &db, "delete from universe_data where name=?", package_name, NULL );  

        db_exec( &db, "delete from universe_history_data where name=? and version=?", package_name, version, NULL );  

        sql_data = "insert into universe_data (name, version, data_name, data_format, data_size, data_install_size, data_depend, data_bdepend, data_recommended, data_conflict) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

        sql_history_data = "insert into universe_history_data (name, version, data_name, data_format, data_size, data_install_size, data_depend, data_bdepend, data_recommended, data_conflict) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
        data_key = (char *)malloc( 32 );
        for( i = 0; ; i++ )
        {
            idx = util_int_to_str( i );
            if( !reader_get_value( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|name", NULL ) ) )
            {
                free( idx );
                break;
            }
            db_exec( &db, sql_data,  
                    package_name, //name
                    version, //version
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|name", NULL ) ), //data_name
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|format", NULL ) ), //data_format
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|size", NULL ) ), //data_size
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|install_size", NULL ) ), //data_install_size
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|depend", NULL ) ), //data_depend
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|bdepend", NULL ) ), //data_bdepend
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|recommended", NULL ) ), //data_recommended
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|conflict" ) ), //data_conflict
                    NULL);

            db_exec( &db, sql_history_data,  
                    package_name, //name
                    version, //version
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|name", NULL ) ), //data_name
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|format", NULL ) ), //data_format
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|size", NULL ) ), //data_size
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|install_size", NULL ) ), //data_install_size
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|depend", NULL ) ), //data_depend
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|bdepend", NULL ) ), //data_bdepend
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|recommended", NULL ) ), //data_recommended
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|conflict" ) ), //data_conflict
                    NULL);
            free( idx );
        }
        free( data_key );
    }
    reader_cleanup( &xml_handle );

    //world
    //printf( "Import world  ...\n" );
    reader_open( LOCAL_WORLD,  &xml_handle );
    sql = "replace into world (name, yversion, generic_name, is_desktop, category, arch, version, priority, install, license, homepage, repo, size, sha, build_date, packager, uri, description, data_count) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    while( ( xml_ret = reader_fetch_a_row( &xml_handle, 1, xml_attrs ) ) == 1 )
    {
        is_desktop = (int)reader_get_value( &xml_handle, "genericname|desktop|keyword|en" );
        package_name = reader_get_value2( &xml_handle, "name" );
        version = reader_get_value2( &xml_handle, "version" );

        //world
        db_ret = db_exec( &db, sql,  
                package_name, //name
                reader_get_value2( &xml_handle, "yversion" ), //yversion
                is_desktop ? reader_get_value2( &xml_handle, "genericname|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "genericname|keyword|en" ), //generic_name
                is_desktop ? "1" : "0", //desktop
                reader_get_value2( &xml_handle, "category" ), //category
                reader_get_value2( &xml_handle, "arch" ), //arch
                version, //version
                reader_get_value2( &xml_handle, "priority" ), //priority
                reader_get_value2( &xml_handle, "install" ), //install
                reader_get_value2( &xml_handle, "license" ), //license
                reader_get_value2( &xml_handle, "homepage" ), //homepage
                reader_get_value2( &xml_handle, "repo" ), //repo
                reader_get_value2( &xml_handle, "size" ), //size
                reader_get_value2( &xml_handle, "sha" ), //sha
                reader_get_value2( &xml_handle, "build_date" ), //build_date
                reader_get_value2( &xml_handle, "packager" ), //packager
                reader_get_value2( &xml_handle, "uri" ), //uri
                is_desktop ? reader_get_value2( &xml_handle, "description|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "description|keyword|en" ), //description
                reader_get_value2( &xml_handle, "data_count" ), //data_count
                NULL);
    
        //world_data
        db_exec( &db, "delete from world_data where name=?", package_name, NULL );  

        sql_data = "insert into world_data (name, version, data_name, data_format, data_size, data_install_size, data_depend, data_bdepend, data_recommended, data_conflict) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
        data_key = (char *)malloc( 32 );
        for( i = 0; ; i++ )
        {
            idx = util_int_to_str( i );
            if( !reader_get_value( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|name", NULL ) ) )
            {
                free( idx );
                break;
            }
            db_exec( &db, sql_data,  
                    package_name, //name
                    version, //version
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|name", NULL ) ), //data_name
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|format", NULL ) ), //data_format
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|size", NULL ) ), //data_size
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|install_size", NULL ) ), //data_install_size
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|depend", NULL ) ), //data_depend
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|bdepend", NULL ) ), //data_bdepend
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|recommended", NULL ) ), //data_recommended
                    reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|conflict" ) ), //data_conflict
                    NULL);
            free( idx );
        }
        free( data_key );

        //update universe
        db_exec( &db, "update universe set installed=1  where name=?", package_name, NULL );  
    }
    reader_cleanup( &xml_handle );


    //world_file
    //printf( "Import file list  ...\n" );
    dir = opendir( PACKAGE_DB_DIR );
    if( !dir )
        return -1;

    while( entry = readdir( dir ) )
    {
        if( !strcmp( entry->d_name, "." ) || !strcmp( entry->d_name, ".." ) )
        {
            continue;
        }

        package_name = entry->d_name;
        //printf( "%s\n", package_name );
        file_path = util_strcat( PACKAGE_DB_DIR, "/", entry->d_name, NULL );
        if( !stat( file_path, &statbuf ) && S_ISDIR( statbuf.st_mode ) )
        {
            //sub dir
            dir_sub = opendir( file_path );
            if( dir_sub )
            {
                while( entry_sub = readdir( dir_sub ) )
                {
                    if( strstr(entry_sub->d_name, ".desc") )
                    {
                        file_path_sub = util_strcat( file_path, "/", entry_sub->d_name, NULL );
                        install_time = util_get_config( file_path_sub, "INSTALL_TIME" );
                        install_size = util_get_config( file_path_sub, "INSTALL_SIZE" );
                        if( install_time )
                        {
                            db_exec( &db, "update world set install_time=?, size=? where name=?", install_time, install_size ? install_size : "0", package_name, NULL );  
                        }
                        else
                        {
                            db_exec( &db, "update world set install_time=strftime('%s','now'), size=? where name=?", package_name,  install_size ? install_size : "0", NULL );  
                        }

                        if( install_time )
                            free( install_time );

                        if( install_size )
                            free( install_size );

                        free( file_path_sub );
                    }

                    if( strstr(entry_sub->d_name, ".list") )
                    {
                        file_path_sub = util_strcat( file_path, "/", entry_sub->d_name, NULL );

                        db_exec( &db, "delete from world_file where name=?", package_name, NULL );  
                        sql_filelist = "insert into world_file (name, type, file, size, perms, uid, gid, mtime, extra) values (?, ?, ?, ?, ?, ?, ?, ?, ?)"; 

                        fp = fopen( file_path_sub, "r" );
                        list_line = NULL;
                        while( getline( &list_line, &list_len, fp ) != -1 )
                        {
                            if( list_line[0] != 'I' )
                            {
                                file_type = strtok( list_line, " ,");
                                file_file = strtok( NULL, " ,");
                                file_size = strtok( NULL, " ,");
                                file_perms = strtok( NULL, " ,");
                                file_uid = strtok( NULL, " ,");
                                file_gid = strtok( NULL, " ,");
                                file_mtime = strtok( NULL, " ,");
                                file_extra = strtok( NULL, " ,");

                                db_ret = db_exec( &db, sql_filelist, 
                                        package_name,
                                        file_type ? file_type : "",
                                        file_file ? file_file : "",
                                        file_size ? file_size : "",
                                        file_perms ? file_perms : "",
                                        file_uid ? file_uid : "",
                                        file_gid ? file_gid : "",
                                        file_mtime ? file_mtime : "",
                                        file_extra ? file_extra : "",
                                        NULL );
                            }

                        }
                        if( list_line )
                            free( list_line );

                        fclose( fp );
                        free( file_path_sub );
                    }
                }
                closedir( dir_sub );
            }
        }
        free( file_path );
    }
    closedir( dir );

    //update config
    db_query( &db, "select max(build_date) from universe", NULL );
    db_fetch_num( &db );
    last_update = db_get_value_by_index( &db, 0 );
    db_cleanup( &db );
    db_exec( &db, "update config set last_update=?, last_check=?", last_update, last_update, NULL );  

    db_exec( &db, "end", NULL );  
    printf( "Done!\n" );
    //clean up
    db_close( &db );
    return 0;
}

int packages_update( YPackageManager *pm, ypk_progress_callback cb, void *cb_arg )
{
    int             timestamp, last_update, time_update, cnt;
    char            *sql, *msg, *target_url, *list_file, *list_line, xml_file[32],  sum[48];
    DB              db;
    DownloadContent content;

    if( !pm )
        return -1;

    msg = NULL;

    //download updates list
    if( cb )
    {
        cb( cb_arg, "*", 0, 1, "resynchronize the package information" );
        cb( cb_arg, "*", 1, -1, "initialization" );
    }

    list_file = UPDATE_DIR "/" LIST_FILE;
    target_url = util_strcat( pm->source_uri, "/", list_file, NULL );
    content.text = malloc(1);
    content.size = 0;
    if( get_content(target_url, &content) != 0 )
    {
        free(content.text);
        free(target_url);
        target_url = NULL;
        return -1; 
    }

    if( cb )
    {
        cb( cb_arg, "*", 1, 1, NULL );
    }

    list_line = util_mem_gets( content.text );
    last_update = packages_get_last_update_timestamp( pm );
    cnt = 0;
    while( list_line )
    {
        util_rtrim( list_line, 0 );
        memset( xml_file, '\0', 32 );
        memset( sum, '\0', 48 );
        if( sscanf( list_line, "%s %d %s", xml_file, &timestamp, sum ) == 3 )
        {
            if( timestamp > last_update )
            {
                //printf("xml:%s time:%d sum:%s\n",  xml_file, timestamp, sum);
                //printf("importing: %s %d %s\n",  xml_file, timestamp, sum);
                packages_update_single_xml( pm, xml_file, sum, cb, cb_arg  );
                last_update = timestamp;
                cnt++;
            }
        }
        free( list_line );
        list_line = util_mem_gets( NULL );
    }
    packages_set_last_update_timestamp( pm, last_update );

    free(content.text);
    free(target_url);
    target_url = NULL;

    if( cb )
    {
        cb( cb_arg, "*", 9, 1, NULL );
    }

    return cnt;
}

/*
 * packages_get_upgrade_list
 */
YPackageChangeList *packages_get_upgrade_list( YPackageManager *pm )
{    
    int             len, i;
    char            *package_name;
    YPackageList    *pkg_list;
    YPackageChangeList     *list, *cur_pkg;

    list = NULL;

    pkg_list = packages_get_list( pm, 100, 0, "can_update", "1", 0, 0 );
    if( pkg_list )
    {
        for( i = 0; i < pkg_list->cnt; i++ )
        {
            package_name = packages_get_list_attr( pkg_list, i, "name" );
            cur_pkg =  (YPackageChangeList *)malloc( sizeof( YPackageChangeList ) );
            len = strlen( package_name );
            cur_pkg->name = (char *)malloc( len + 1 );
            strncpy( cur_pkg->name, package_name, len );
            cur_pkg->name[len] = 0;
            cur_pkg->type = 1;
            cur_pkg->prev = list;
            list = cur_pkg;
        }
        packages_free_list( pkg_list );
    }

    return list;

}

int packages_upgrade_list( YPackageManager *pm, YPackageChangeList *list, ypk_progress_callback cb, void *cb_arg   )
{
    YPackageChangeList    *cur_pkg;

    if( !list )
        return;


    while( list )
    {
        cur_pkg = list;
        
        //printf("upgrading %s ...\n", cur_pkg->name );
        packages_install_package( pm, cur_pkg->name, cb, cb_arg  );
        list = list->prev;
    }
}

void packages_free_upgrade_list( YPackageChangeList *list )
{
    YPackageChangeList    *cur_pkg;

    if( !list )
        return;


    while( list )
    {
        cur_pkg = list;
        list = list->prev;
        free( cur_pkg->name );
        free( cur_pkg );
    }
}

static int packages_update_single_xml( YPackageManager *pm, char *xml_file, char *sum, ypk_progress_callback cb, void *cb_arg )
{
    int                 i, xml_ret, db_ret, cmp_ret, is_desktop, do_replace;
    char                *xml_sha, *target_url, *msg, xml_value, *sql, *sql_data, *sql_history, *sql_history_data, *package_name, *version, *delete, *idx, *data_key, *installed, *old_version, *can_update;
    char                tmp_bz2[] = "/tmp/tmp_bz2.XXXXXX";
    char                tmp_xml[] = "/tmp/tmp_xml.XXXXXX";
    char                *xml_attrs[] = {"name", "type", "lang", "id", NULL};
    DownloadFile        file;
    XMLReaderHandle     xml_handle;
    DB                  db;

    if( !pm || !xml_file || !sum )
        return -1;


    //donload xml
    target_url = util_strcat( pm->source_uri, "/", UPDATE_DIR, "/", xml_file, NULL );

    if( cb )
    {
        msg = util_strcat( "get: ", target_url, NULL );

        cb( cb_arg, "*", 2, -1, msg ? msg : NULL );

        if( msg )
        {
            free( msg );
            msg = NULL;
        }
    }

    mkstemp(tmp_bz2);
    file.file = tmp_bz2;
    file.stream = NULL;
    file.cb = NULL;
    file.cb_arg = NULL;
    if( download_file( target_url, &file ) != 0 )
    {
        if( cb )
        {
            cb( cb_arg, "Download failed.", 2, 1, NULL );
        }
        remove(tmp_bz2);
        return -1; 
    }
    fclose(file.stream);
    free(target_url);
    target_url = NULL;


    //compare sum
    xml_sha = util_sha1( tmp_bz2 );
    if( strncmp( xml_sha, sum, 41 ) != 0 )
    {
        if( cb )
        {
            cb( cb_arg, "sha1 checksum does not match.", 2, 1, NULL );
        }
        return -1;
    }

    if( cb )
    {
        cb( cb_arg, "*", 2, 1, NULL );
    }

    //unzip
    if( cb )
    {
        msg = strdup( "extracting information" );

        cb( cb_arg, "*", 4, -1, msg ? msg : NULL );

        if( msg )
        {
            free( msg );
            msg = NULL;
        }
    }

    mkstemp(tmp_xml);
    if( archive_extract_file( file.file, "update.xml", tmp_xml ) == -1 )
    {
        remove(tmp_bz2);
        remove(tmp_xml);
        return -1;
    }

    if( cb )
    {
        cb( cb_arg, "*", 4, 1, NULL );
    }


    if( cb )
    {
        msg = strdup( "updating the database" );

        cb( cb_arg, "*", 8, -1, msg ? msg : NULL );

        if( msg )
        {
            free( msg );
            msg = NULL;
        }
    }

    //parse xml
    reader_open( tmp_xml,  &xml_handle );
    db_init( &db, pm->db_name, OPEN_WRITE );
    db_exec( &db, "begin", NULL );  

    sql = "replace into universe (name, yversion, generic_name, is_desktop, category, arch, version, priority, install, license, homepage, repo, size, sha, build_date, packager, uri, description, data_count, can_update, installed ) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sql_history = "replace into universe_history (name, yversion, generic_name, is_desktop, category, arch, version, priority, install, license, homepage, repo, size, sha, build_date, packager, uri, description, data_count) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sql_data = "insert into universe_data (name, version, data_name, data_format, data_size, data_install_size, data_depend, data_bdepend, data_recommended, data_conflict) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sql_history_data = "insert into universe_history_data (name, version, data_name, data_format, data_size, data_install_size, data_depend, data_bdepend, data_recommended, data_conflict) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    while( ( xml_ret = reader_fetch_a_row( &xml_handle, 1, xml_attrs ) ) == 1 )
    {
        package_name = reader_get_value2( &xml_handle, "name" );
        version = reader_get_value2( &xml_handle, "version" );
        delete = reader_get_value( &xml_handle, "delete" );

        if( delete )
        {
            if( strncmp( delete, "all", 3 ) == 0 )
            {
                db_exec( &db, "delete from universe where name=?", package_name, NULL );  

                db_exec( &db, "delete from universe_data where name=?", package_name, NULL );  

                db_exec( &db, "delete from universe_history where name=?", package_name, NULL );  

                db_exec( &db, "delete from universe_history_data where name=?", package_name, NULL );  
            }
            else if( strncmp( delete, "single", 6 ) == 0 )
            {
                db_exec( &db, "delete from universe where name=? and version=?", package_name, version, NULL );  

                db_exec( &db, "delete from universe_data where name=? and version=?", package_name, version, NULL );  

                db_exec( &db, "delete from universe_history where name=? and version=?", package_name, version, NULL );  

                db_exec( &db, "delete from universe_history_data where name=? and version=?", package_name, version, NULL );  
            }
        }
        else
        {
            is_desktop = (int)reader_get_value( &xml_handle, "genericname|desktop|keyword|en" );

            //get original value
            do_replace = 0;
            can_update = "0";
            installed = "0";
            db_query( &db, "select version, installed from universe where name=?", package_name, NULL);
            if( db_fetch_assoc( &db ) )
            {
                old_version = db_get_value_by_key( &db, "version" );
                installed =  db_get_value_by_key( &db, "installed" );

                if( installed[0] == '1' )
                {
                    if( version && (strlen( version ) > 0) && old_version && (strlen( old_version ) > 0)  )
                    {
                            cmp_ret = packages_compare_version( version, old_version );
                            if( cmp_ret == 1 )
                            {
                                can_update = "1"; //can upgrade
                            }
                            else
                            {
                                can_update = "-1"; //can downgrade
                            }
                    }
                }

                do_replace = 1;
                db_cleanup( &db );
            }
            else
            {
                do_replace = 1;
            }

            //universe
            if( do_replace )
            {
                db_ret = db_exec( &db, sql,  
                        package_name, //name
                        reader_get_value2( &xml_handle, "yversion" ), //yversion
                        is_desktop ? reader_get_value2( &xml_handle, "genericname|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "genericname|keyword|en" ), //generic_name
                        is_desktop ? "1" : "0", //desktop
                        reader_get_value2( &xml_handle, "category" ), //category
                        reader_get_value2( &xml_handle, "arch" ), //arch
                        version, //version
                        reader_get_value2( &xml_handle, "priority" ), //priority
                        reader_get_value2( &xml_handle, "install" ), //install
                        reader_get_value2( &xml_handle, "license" ), //license
                        reader_get_value2( &xml_handle, "homepage" ), //homepage
                        reader_get_value2( &xml_handle, "repo" ), //repo
                        reader_get_value2( &xml_handle, "size" ), //size
                        reader_get_value2( &xml_handle, "sha" ), //sha
                        reader_get_value2( &xml_handle, "build_date" ), //build_date
                        reader_get_value2( &xml_handle, "packager" ), //packager
                        reader_get_value2( &xml_handle, "uri" ), //uri
                        is_desktop ? reader_get_value2( &xml_handle, "description|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "description|keyword|en" ), //description
                        reader_get_value2( &xml_handle, "data_count" ), //data_count
                        can_update,
                        installed,
                        NULL);

                db_ret = db_exec( &db, sql_history,  
                        package_name, //name
                        reader_get_value2( &xml_handle, "yversion" ), //yversion
                        is_desktop ? reader_get_value2( &xml_handle, "genericname|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "genericname|keyword|en" ), //generic_name
                        is_desktop ? "1" : "0", //desktop
                        reader_get_value2( &xml_handle, "category" ), //category
                        reader_get_value2( &xml_handle, "arch" ), //arch
                        version, //version
                        reader_get_value2( &xml_handle, "priority" ), //priority
                        reader_get_value2( &xml_handle, "install" ), //install
                        reader_get_value2( &xml_handle, "license" ), //license
                        reader_get_value2( &xml_handle, "homepage" ), //homepage
                        reader_get_value2( &xml_handle, "repo" ), //repo
                        reader_get_value2( &xml_handle, "size" ), //size
                        reader_get_value2( &xml_handle, "sha" ), //sha
                        reader_get_value2( &xml_handle, "build_date" ), //build_date
                        reader_get_value2( &xml_handle, "packager" ), //packager
                        reader_get_value2( &xml_handle, "uri" ), //uri
                        is_desktop ? reader_get_value2( &xml_handle, "description|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "description|keyword|en" ), //description
                        reader_get_value2( &xml_handle, "data_count" ), //data_count
                        NULL);
            
                //universe_data
                db_exec( &db, "delete from universe_data where name=?", package_name, NULL );  

                db_exec( &db, "delete from universe_history_data where name=? and version=?", package_name, version, NULL );  

                data_key = (char *)malloc( 32 );
                for( i = 0; ; i++ )
                {
                    idx = util_int_to_str( i );
                    if( !reader_get_value( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|name", NULL ) ) )
                    {
                        free( idx );
                        break;
                    }
                    db_exec( &db, sql_data,  
                            package_name, //name
                            version, //version
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|name", NULL ) ), //data_name
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|format", NULL ) ), //data_format
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|size", NULL ) ), //data_size
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|install_size", NULL ) ), //data_install_size
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|depend", NULL ) ), //data_depend
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|bdepend", NULL ) ), //data_bdepend
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|recommended", NULL ) ), //data_recommended
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|conflict" ) ), //data_conflict
                            NULL);

                    db_exec( &db, sql_history_data,  
                            package_name, //name
                            version, //version
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|name", NULL ) ), //data_name
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|format", NULL ) ), //data_format
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|size", NULL ) ), //data_size
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|install_size", NULL ) ), //data_install_size
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|depend", NULL ) ), //data_depend
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|bdepend", NULL ) ), //data_bdepend
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|recommended", NULL ) ), //data_recommended
                            reader_get_value2( &xml_handle, util_strcat2( data_key, 32, "data|", idx, "|conflict" ) ), //data_conflict
                            NULL);

                    free( idx );
                }//for
                free( data_key );
            }//if

        }
    }//while

    db_ret = db_exec( &db, "commit", NULL );  
    if( db_ret == SQLITE_BUSY )
    {
        printf( "db_exec commit:%d\n", db_ret);
        db_ret = db_exec( &db, "commit", NULL );  
        printf( "db_exec commit:%d\n", db_ret);
    }

    if( db_ret == SQLITE_BUSY )
    {
        db_ret = db_exec( &db, "rollback", NULL );  
        printf( "db_exec rollback:%d\n", db_ret);
    }
    //clean up
    db_close( &db );
    reader_cleanup( &xml_handle );

    if( cb )
    {
        cb( cb_arg, "*", 8, 1, NULL );
    }

    remove(tmp_bz2);
    remove(tmp_xml);
}

static int packages_get_last_check_timestamp( YPackageManager *pm )
{
    int     last_check, has_new;
    DB      db;

    if( !pm )
        return -1;

    db_init( &db, pm->db_name, OPEN_READ );
    db_query( &db, "select has_new, last_check from config limit 1", NULL );
    if( db_fetch_row( &db, RESULT_NUM ) == 0 )
    {
        db_close( &db );
        return -1;
    }

    has_new = atoi( db.crow[0] );
    if( has_new )
    {
        db_close( &db );
        return 1;
    }

    last_check = atoi( db.crow[1] );
    db_close( &db );
    return last_check;
}


static int packages_set_last_check_timestamp( YPackageManager *pm, int last_check )
{
    int     ret = -1;
    char    *timestamp;
    DB      db;

    if( !pm || last_check < 0 )
        return ret;

    db_init( &db, pm->db_name, OPEN_WRITE );
    timestamp = util_int_to_str( last_check );
    if( db_exec( &db, "update config set has_new = 1, last_check = ?", timestamp, NULL ) )
    {
        ret = 0;
    }

    free( timestamp );
    db_close( &db );
    return ret;
}

static int packages_get_last_update_timestamp( YPackageManager *pm )
{
    DB      db;
    int     last_update;

    if( !pm )
        return -1;

    db_init( &db, pm->db_name, OPEN_READ );
    db_query( &db, "select last_update from config limit 1", NULL );
    if( db_fetch_row( &db, RESULT_NUM ) == 0 )
    {
        db_close( &db );
        return -1;
    }
    last_update = atoi( db.crow[0] );
    db_close(&db);

    return last_update;
}

static int packages_set_last_update_timestamp( YPackageManager *pm, int last_update )
{
    DB      db;
    char    timestamp[11];

    if( !pm || last_update < 0 )
        return -1;

    db_init( &db, pm->db_name, OPEN_WRITE );
    snprintf( timestamp, 11, "%d", last_update);
    timestamp[10] = '\0';
    if( db_exec( &db, "update config set has_new = 0, last_update = ?", timestamp, NULL ) )
    {
        db_close( &db );
        return 0;
    }

    db_close( &db );
    return -1;
}

int packages_get_count( YPackageManager *pm, char *key, char *keyword, int wildcards, int installed  )
{
    DB      db;
    int     count;
    char    *sql, *table;

    if( !pm )
        return -1;

    table = installed ? "world" : "universe";

    db_init( &db, pm->db_name, OPEN_READ );
    if( !key || !keyword )
    {
        sql = util_strcat( "select count(*) from ", table, NULL );
        db_query( &db, sql, NULL);
        free( sql );
    }
    else if( key[0] == '*' && wildcards )
    {
        sql = util_strcat( "select count(*) from ", table, " where name like '%'||?||'%' or generic_name like  '%'||?||'%'  or description like '%'||?||'%'", NULL );
        db_query( &db, sql, keyword, keyword, keyword, NULL);
        free( sql );
    }
    else if( key[0] == '*' )
    {
        sql = util_strcat( "select count(*) from ", table, " where name = ? or generic_name = ? or description = ?", NULL );
        db_query( &db, sql, keyword, keyword, keyword, NULL);
        free( sql );
    }
    else
    {
        sql = util_strcat( "select count(*) from ", table, " where ", key, wildcards ? " like '%'||?||'%'" : " = ?", NULL );
        db_query( &db, sql, keyword, NULL );
        free( sql );
    }

    db_fetch_num( &db );
    count = atoi( db_get_value_by_index( &db, 0 ) );
    db_close( &db );

    return count;
}

int packages_has_installed( YPackageManager *pm, char *name, char *version )
{
    int     count, ret;
    char    *version_installed;
    DB      db;

    if( !pm || !name )
        return -1;

    db_init( &db, pm->db_name, OPEN_READ );

    if( version )
    {
        version_installed = NULL;

        db_query( &db, "select version from world where name=?", name, NULL);

        if( db_fetch_num( &db ) )
        {
            version_installed = db_get_value_by_index( &db, 0 );
            if( version[0] == '>' || version[0] == '=' || version[0] == '!' || version[0] == '<')
            {
                if( version[1] == '=' )
                {
                    ret = packages_compare_version( version_installed, version + 2 );
                    switch( version[0] )
                    {
                        case '>':
                            ret = ret != -1;
                            break;

                        case '!':
                            ret = ret != 0;
                            break;

                        case '<':
                            ret = ret != 1;
                            break;

                        default:
                            ret = ret == 0;
                    }
                }
                else
                {
                    ret = packages_compare_version( version_installed, version + 1 );
                    switch( version[0] )
                    {
                        case '=':
                            ret = ret == 0;
                            break;

                        case '>':
                            ret = ret == 1;
                            break;

                        case '<':
                            ret = ret == -1;
                            break;

                        default:
                            ret = ret == 0;
                    }
                }
            }
            else
            {
                ret = packages_compare_version( version, version_installed ) == 0;
            }
        }
        else
        {
            ret = 0;
        }

    }
    else
    {
        db_query( &db, "select count(*) from world where name=?", name, NULL);
        db_fetch_num( &db );
        count = atoi( db_get_value_by_index( &db, 0 ) );
        ret = count > 0;
    }


    db_close( &db );

    return ret;
}

int packages_exists( YPackageManager *pm, char *name, char *version )
{
    DB      db;
    int     count;

    if( !pm || !name )
        return -1;

    db_init( &db, pm->db_name, OPEN_READ );
    if( version)
        db_query( &db, "select count(*) from universe where name=? and version=?", name, version, NULL);
    else
        db_query( &db, "select count(*) from universe where name=?", name, NULL);

    db_fetch_num( &db );
    count = atoi( db_get_value_by_index( &db, 0 ) );
    db_close( &db );

    return count > 0;
}

YPackage *packages_get_package( YPackageManager *pm, char *name, int installed )
{
    char                    *sql, *cur_key, *cur_value, **attr_keys_offset;
    char                    *attr_keys[] = { "name", "generic_name", "category", "priority", "version", "license", "homepage", "description", "uri", "sha", "size", "repo", "arch", "install", "data_count", "installed", "install_time", "can_update", "build_date", "packager", NULL  }; 
    DB                      db;
    YPackage                 *pkg = NULL;

    if( !pm || !name )
    {
        return NULL;
    }

    db_init( &db, pm->db_name, OPEN_READ );
    sql = util_strcat( "select * from ", installed ? "world" : "universe"," where name=? limit 1", NULL );
    db_query( &db, sql, name, NULL);
    free( sql );

    if( db_fetch_assoc( &db ) )
    {
        pkg = (YPackage *)malloc( sizeof( YPackage ) );
        pkg->ht = hash_table_init( );

        attr_keys_offset = attr_keys;
        while( cur_key = *attr_keys_offset++ )
        {
            cur_value = db_get_value_by_key( &db, cur_key );
            hash_table_add_data( pkg->ht, cur_key, cur_value );
        }
    }

    db_close( &db );

    return pkg;
}

/*
 * packages_get_package_from_ypk
 */
int packages_get_package_from_ypk( char *ypk_path, YPackage **package, YPackageData **package_data )
{
    int                 i, ret, return_code = 0, cur_data_index, data_count;
    void                *pkginfo = NULL, *control = NULL;
    size_t              pkginfo_len = 0, control_len = 0;
    char                *cur_key, *cur_value, *cur_xpath, **attr_keys_offset, **attr_xpath_offset, *idx, *data_key, *package_name;
    char                *desktop_file; //for ypki2
    char                *attr_keys[] = { "name", "yversion", "generic_name", "category", "arch", "priority", "version", "install", "license", "homepage", "repo", "description", "sha", "size", "build_date", "packager", "uri", "data_count", "is_desktop", NULL  }; 
    char                *attr_xpath[] = { "//Package/@name", "//yversion", "//genericname/keyword", "//category", "//arch", "//priority", "//version", "//install", "//license", "//homepage", "//repo", "//description/keyword", "//sha", "//size", "//build_date", "//packager", "//uri", "//data_count", "//genericname[@type='desktop']", NULL  }; 
    char                *data_attr_keys[] = { "data_name", "data_format", "data_size", "data_install_size", "data_depend", "data_bdepend", "data_recommended", "data_conflict", NULL  }; 
    char                *data_attr_xpath[] = { "name", "format", "size", "install_size", "depend", "bdepend", "recommended", "conflict", NULL  }; 
    char                tmp_ypk_desktop[] = "/tmp/ypkdesktop.XXXXXX"; //for ypki2
    xmlDocPtr           xmldoc = NULL;
    xmlXPathObjectPtr   xpath;
    YPackage             *pkg;
    YPackageData        *pkg_data;


    if( !package && !package_data )
        return -1;

    if( access( ypk_path, R_OK ) )
        return -1;
    
    //unzip info
    ret = archive_extract_file2( ypk_path, "pkginfo", &pkginfo, &pkginfo_len );
    if( ret == -1 || pkginfo_len == 0 )
    {
        return_code = -1;
        goto return_point;
    }

    ret = archive_extract_file4( pkginfo, pkginfo_len, "control.xml", &control, &control_len );
    if( ret == -1 ||  control_len == 0)
    {
        return_code = -2;
        goto return_point;
    }

    
    //parse info
	if( ( xmldoc = xpath_open2( control, control_len ) ) == (xmlDocPtr)-1 )
    {
        return_code = -3;
        goto return_point;
    }
    if( control )
    {
        free( control );
        control = NULL;
    }
    

    if( package )
    {
        pkg = (YPackage *)malloc( sizeof( YPackage ) );
        pkg->ht = hash_table_init( );

        attr_keys_offset = attr_keys;
        attr_xpath_offset = attr_xpath;
        while( cur_key = *attr_keys_offset++ )
        {
            cur_xpath = *attr_xpath_offset++;
            cur_value =  xpath_get_node( xmldoc, cur_xpath );

            hash_table_add_data( pkg->ht, cur_key, cur_value );
            if( cur_value )
                free( cur_value );
        }
        *package = pkg;
    }

    if( package_data )
    {
        if( package && packages_get_package_attr( (*package), "data_count") )
        {
            cur_value = packages_get_package_attr( (*package), "data_count");
            data_count =  cur_value ? atoi( cur_value ) : 1;
        }
        else
        {
            cur_value = xpath_get_node( xmldoc, "//data_count" );
            data_count =  cur_value ? atoi( cur_value ) : 1;
            free( cur_value );
        }
        data_count = data_count ? data_count : 1;

        pkg_data = (YPackageData *)malloc( sizeof( YPackageData ) );
        pkg_data->htl = hash_table_list_init( data_count );
        
        cur_data_index = 0;
        pkg_data->cnt = 0;

        data_key = (char *)malloc( 32 );
        for( i = 0; ; i++ )
        {
            idx = util_int_to_str( i );
            cur_value = xpath_get_node( xmldoc, util_strcat2( data_key, 32, "//data[@id='", idx, "']", NULL ) );
            if( !cur_value )
            {
                free( idx );
                break;
            }
            free( cur_value );

            attr_keys_offset = data_attr_keys;
            attr_xpath_offset = data_attr_xpath;
            while( cur_key = *attr_keys_offset++ )
            {
                cur_xpath = *attr_xpath_offset++;
                cur_value = xpath_get_node( xmldoc, util_strcat2( data_key, 32, "//data[@id='", idx, "']/", cur_xpath, NULL ) );
                hash_table_list_add_data( pkg_data->htl, cur_data_index, cur_key, cur_value );
                if( cur_value  )
                    free( cur_value );
            }
            free( idx );
            cur_data_index++;
        }
        free( data_key );
        pkg_data->cnt = cur_data_index;

        if( pkg_data->cnt == 0 )
        {
            hash_table_list_cleanup( pkg_data->htl );
            free( pkg_data );
            *package_data =  NULL;
        }
        else
        {
            *package_data = pkg_data;
        }
    }

    package_name = packages_get_package_attr( (*package), "name"); //ypki2
    desktop_file = util_strcat( package_name, ".desktop", NULL ); //ypkg2
    ret = archive_extract_file3( pkginfo, pkginfo_len, desktop_file, tmp_ypk_desktop );//ypki2
    free( desktop_file ); //ypki2

    if( pkginfo )
    {
        free( pkginfo );
        pkginfo = NULL;
    }

    if( xmldoc )
        xmlFreeDoc( xmldoc );

	xmlCleanupParser();
    return 0;

return_point:
    if( pkginfo )
        free( pkginfo );

    if( control )
        free( control );

    if( xmldoc )
        xmlFreeDoc( xmldoc );

	xmlCleanupParser();

    if( package )
        *package = NULL;

    if( package_data )
        *package_data = NULL;
    return return_code;
}

/*
 * packages_get_package_attr
 */
char *packages_get_package_attr( YPackage *pkg, char *key )
{
    if( !pkg || !key )
        return NULL;

    return hash_table_get_data( pkg->ht, key );
}

char *packages_get_package_attr2( YPackage *pkg, char *key )
{
    char *result;

    if( !pkg || !key )
        return NULL;

    result = hash_table_get_data( pkg->ht, key );
    return result ? result : "";
}

/*
 * packages_free_package
 */
void packages_free_package( YPackage *pkg )
{
    if( !pkg )
        return;

    hash_table_cleanup( pkg->ht );
    free( pkg );
}

/*
 * packages_get_package_data
 */
YPackageData *packages_get_package_data( YPackageManager *pm, char *name, int installed )
{
    int                     data_count, cur_data_index;
    char                    *sql, *cur_key, *cur_value, **attr_keys_offset;
    char                    *attr_keys[] = { "name", "data_name", "data_format", "data_size", "data_install_size", "data_depend", "data_bdepend", "data_recommended", "data_conflict", NULL  }; 
    DB                      db;
    YPackageData            *pkg_data = NULL;

    if( !pm || !name )
    {
        return NULL;
    }

    db_init( &db, pm->db_name, OPEN_READ );
    //get file count
    sql = util_strcat( "select count(*) from ", installed ? "world_data" : "universe_data"," where name=?", NULL );
    db_query( &db, sql, name, NULL);
    free( sql );
    db_fetch_num( &db );
    data_count = atoi( db_get_value_by_index( &db, 0 ) );
    db_cleanup( &db );

    //get data
    sql = util_strcat( "select * from ", installed ? "world_data" : "universe_data"," where name=?", NULL );
    db_query( &db, sql, name, NULL);
    free( sql );

    pkg_data = (YPackageData *)malloc( sizeof( YPackageData ) );
    pkg_data->htl = hash_table_list_init( data_count );

    cur_data_index = 0;
    pkg_data->cnt = 0;

    while( db_fetch_assoc( &db ) )
    {

        attr_keys_offset = attr_keys;
        while( cur_key = *attr_keys_offset++ )
        {
            cur_value = db_get_value_by_key( &db, cur_key );
            hash_table_list_add_data( pkg_data->htl, cur_data_index, cur_key, cur_value );
        }
        cur_data_index++;
    }
    pkg_data->cnt = cur_data_index;

    db_close( &db );
    if( pkg_data->cnt == 0 )
    {
        hash_table_list_cleanup( pkg_data->htl );
        free( pkg_data );
        return NULL;
    }

    return pkg_data;
}

/*
 * packages_get_package_data_attr
 */
char *packages_get_package_data_attr( YPackageData *pkg_data, int index, char *key )
{
    return hash_table_list_get_data( pkg_data->htl, index, key );
}

char *packages_get_package_data_attr2( YPackageData *pkg_data, int index, char *key )
{
    char *result;

    result = hash_table_list_get_data( pkg_data->htl, index, key );
    return result ? result : "";
}
/*
 * packages_free_package_data
 */
void packages_free_package_data( YPackageData *pkg_data )
{
    if( !pkg_data )
        return;

    hash_table_list_cleanup( pkg_data->htl );
    free( pkg_data );
}

/*
 * packages_get_package_file
 */
YPackageFile *packages_get_package_file( YPackageManager *pm, char *name )
{
    int                     file_count, file_type, buf_size, cur_len, cur_pos, cur_file_index;
    char                    *sql, *cur_key, *cur_value, **attr_keys_offset;
    char                    *attr_keys[] = { "name", "file", "type", "size", "perms", "uid", "gid", "mtime", NULL  }; 
    DB                      db;
    YPackageFile            *pkg_file = NULL;
    HashTable              *cur_file;
    ENTRY                   item, *itemp;

    if( !pm || !name )
    {
        return NULL;
    }


    db_init( &db, pm->db_name, OPEN_READ );

    //get file count
    db_query( &db, "select count(*) from world_file where name=?", name, NULL);
    db_fetch_num( &db );
    file_count = atoi( db_get_value_by_index( &db, 0 ) );
    db_cleanup( &db );

    //get file info
    db_query( &db, "select * from world_file where name=?", name, NULL);

    pkg_file = (YPackageFile *)malloc( sizeof( YPackageFile ) );
    pkg_file->htl = hash_table_list_init( file_count );

    cur_file_index = 0;
    pkg_file->cnt = 0;
    pkg_file->cnt_file = 0;
    pkg_file->cnt_dir = 0;
    pkg_file->cnt_link = 0;
    pkg_file->size = 0;

    while( db_fetch_assoc( &db ) )
    {
        attr_keys_offset = attr_keys;
        while( cur_key = *attr_keys_offset++ )
        {
            cur_value = db_get_value_by_key( &db, cur_key );
            hash_table_list_add_data( pkg_file->htl, cur_file_index, cur_key, cur_value );

            //counter
            if( cur_key[0] == 't' && cur_value ) //type
            {
                switch( cur_value[0] )
                {
                    case 'F':
                        pkg_file->cnt_file++;
                        file_type = 1;
                        break;
                    case 'D':
                        pkg_file->cnt_dir++;
                        file_type = 2;
                        break;
                    case 'L':
                        pkg_file->cnt_link++;
                        file_type = 3;
                        break;
                }
            }
            else if( cur_key[0] == 's' && file_type == 1 ) //size
            {
                pkg_file->size += atoi( cur_value );
            }
        }
        cur_file_index++;
    }
    pkg_file->cnt = cur_file_index;

    db_close( &db );
    if( pkg_file->cnt == 0 )
    {
        hash_table_list_cleanup( pkg_file->htl );
        free( pkg_file );
        return NULL;
    }

    pkg_file->size = pkg_file->size / 1024;
    return pkg_file;
}

/*
 * packages_get_package_file_from_ypk
 */
YPackageFile *packages_get_package_file_from_ypk( char *ypk_path )
{
    int                 ret, file_count, cur_file_index;
    size_t              pkginfo_len = 0, filelist_len = 0;
    void                *pkginfo = NULL, *filelist = NULL;
    char                *package_name, *yversion, *file_type, *file_file, *file_size, *file_perms, *file_uid, *file_gid, *file_mtime, *file_extra;
    char                *list_line, *cur_key, *cur_value, **attr_keys_offset;
    char                *attr_keys[] = { "type", "file", "size", "perms", "uid", "gid", "mtime", "extra", NULL  }; 
    YPackageFile        *pkg_file = NULL;

    if( access( ypk_path, R_OK ) )
        return NULL;


    //unzip info
    ret = archive_extract_file2( ypk_path, "pkginfo", &pkginfo, &pkginfo_len );
    if( ret == -1 || pkginfo_len == 0 )
    {
        goto return_point;
    }

    ret = archive_extract_file4( pkginfo, pkginfo_len, "filelist", &filelist, &filelist_len );
    if( ret == -1 ||  filelist_len == 0)
    {
        goto return_point;
    }

    file_count = 500;
    pkg_file = (YPackageFile *)malloc( sizeof( YPackageFile ) );
    pkg_file->htl = hash_table_list_init( file_count );

    cur_file_index = 0;
    pkg_file->cnt = 0;

    list_line = util_mem_gets( filelist );
    while( list_line )
    {
        if( list_line[0] != 'I' )
        {
            if( cur_file_index + 1 > file_count )
            {
                file_count += 500;
                hash_table_list_extend( pkg_file->htl, file_count );
            }

            attr_keys_offset = attr_keys;
            cur_value = strtok( list_line, " ,");
            while( cur_key = *attr_keys_offset++ )
            {
                hash_table_list_add_data( pkg_file->htl, cur_file_index, cur_key, cur_value );
                cur_value =  strtok( NULL, " ,");
            }
            cur_file_index++;
        }
        else
        {
            strtok( list_line, " ,");
            pkg_file->cnt_file = atoi( strtok( NULL, " ,") );
            pkg_file->cnt_dir = atoi( strtok( NULL, " ,") );
            pkg_file->cnt_link = atoi( strtok( NULL, " ,") );
            strtok( NULL, " ,");
            pkg_file->size = atoi( strtok( NULL, " ,") );
        }
        free( list_line );
        list_line = util_mem_gets( NULL );
    }
    pkg_file->cnt = cur_file_index;

return_point:
    if( pkginfo )
        free( pkginfo );

    if( filelist )
        free( filelist );

    if( !pkg_file )
        return NULL;

    if( pkg_file->cnt == 0 )
    {
        hash_table_list_cleanup( pkg_file->htl );
        free( pkg_file );
        return NULL;
    }

    return pkg_file;
}

/*
 * packages_get_package_file_attr
 */
char *packages_get_package_file_attr( YPackageFile *pkg_file, int index, char *key )
{
    return hash_table_list_get_data( pkg_file->htl, index, key );
}

char *packages_get_package_file_attr2( YPackageFile *pkg_file, int index, char *key )
{
    char *result;

    result = hash_table_list_get_data( pkg_file->htl, index, key );
    return result ? result : "";
}

/*
 * packages_free_package_file
 */
void packages_free_package_file( YPackageFile *pkg_file )
{
    int i;

    if( !pkg_file )
        return;

    hash_table_list_cleanup( pkg_file->htl );
    free( pkg_file );
}

/*
 * packages_get_list
 */
YPackageList *packages_get_list( YPackageManager *pm, int limit, int offset, char *key, char *keyword, int wildcards, int installed )
{
    int                     ret, cur_pkg_index;
    char                    *table,*sql, *offset_str, *limit_str, *cur_key, *cur_value, **attr_keys_offset;
    char                    *attr_keys[] = { "name", "generic_name", "category", "priority", "version", "license", "description", "size", "repo", "install_time", "installed", "can_update", "homepage", "build_date", "packager", NULL  }; 
    DB                      db;
    YPackageList            *pkg_list;


    if( offset < 0 )
        offset = 0;

    if( limit < 1 )
        limit = 5;

    offset_str = util_int_to_str( offset );
    limit_str = util_int_to_str( limit );

    table = installed ? "world" : "universe";

    ret = db_init( &db, pm->db_name, OPEN_READ );
    if( !key || !keyword )
    {
        sql = util_strcat( "select * from ", table, " limit ? offset ?", NULL );
        db_query( &db, sql, limit_str, offset_str, NULL);
        free( sql );
    }
    else if( key[0] == '*' && wildcards )
    {
        sql = util_strcat( "select * from ", table, " where name like '%'||?||'%' or generic_name like  '%'||?||'%'  or description like '%'||?||'%' limit ? offset ?", NULL );
        db_query( &db, sql, keyword, keyword, keyword, limit_str, offset_str, NULL);
        free( sql );
    }
    else if( key[0] == '*' )
    {
        sql = util_strcat( "select * from ", table, " where name = ? or generic_name = ? or description = ? limit ? offset ?", NULL );
        db_query( &db, sql, keyword, keyword, keyword, limit_str, offset_str, NULL);
        free( sql );
    }
    else
    {
        sql = util_strcat( "select * from ", table, " where ", key, wildcards ? " like '%'||?||'%' limit ? offset ?" : " = ? limit ? offset ?", NULL );
        db_query( &db, sql, keyword, limit_str, offset_str, NULL );
        free( sql );
    }
    free( limit_str );
    free( offset_str );

    pkg_list = (YPackageList *)malloc( sizeof( YPackageList ) );
    pkg_list->htl = hash_table_list_init( limit );

    cur_pkg_index = 0;
    pkg_list->cnt = 0;

    while( db_fetch_assoc( &db ) )
    {

        attr_keys_offset = attr_keys;
        while( cur_key = *attr_keys_offset++ )
        {
            cur_value = db_get_value_by_key( &db, cur_key );
            hash_table_list_add_data( pkg_list->htl, cur_pkg_index, cur_key, cur_value );
        }
        cur_pkg_index++;
    }
    pkg_list->cnt = cur_pkg_index;

    db_close( &db );

    if( pkg_list->cnt == 0 )
    {
        hash_table_list_cleanup( pkg_list->htl );
        free( pkg_list );
        return NULL;
    }

    return pkg_list;
}

/*
 * packages_get_list2
 */
YPackageList *packages_get_list2( YPackageManager *pm, int page_size, int page_no, char *key, char *keyword, int wildcards, int installed )
{
    int                     offset;
    YPackageList            *pkg_list;


    if( page_size < 1 )
        page_size = 5;

    if( page_no < 1 )
        page_no = 1;

    offset = ( page_no - 1 ) * page_size;

    pkg_list = packages_get_list( pm, page_size, offset, key, keyword, wildcards, installed );

    return pkg_list;
}

/*
 * packages_get_list_with_data
 */
YPackageList *packages_get_list_with_data( YPackageManager *pm, int limit, int offset, char *key, char *keyword, int installed )
{    
    int                     ret, cur_pkg_index;
    char                    *table, *table_data, *sql, *offset_str, *limit_str, *cur_key, *cur_value, **attr_keys_offset;
    char                    *attr_keys[] = { "name", "generic_name", "category", "priority", "version", "license", "description", "size", NULL  }; 
    DB                      db;
    YPackageList            *pkg_list;

    if( !key || !keyword )
        return NULL;

    if( offset < 0 )
        offset = 0;

    if( limit < 1 )
        limit = 5;

    offset_str = util_int_to_str( offset );
    limit_str = util_int_to_str( limit );

    table = installed ? "world" : "universe";
    table_data = installed ? "world_data" : "universe_data";


    ret = db_init( &db, pm->db_name, OPEN_READ );
    sql = util_strcat( "select ", table, ".* from ", table, ",", table_data, " where ", table, ".name=", table_data, ".name and ", key, " = ? ", "union select ", table, ".* from ", table, ",", table_data, " where ", table, ".name=", table_data, ".name and ", key, " like ?||',%'", "union select ", table, ".* from ", table, ",", table_data, " where ", table, ".name=", table_data, ".name and ", key, " like '%,'||?", "union select ", table, ".* from ", table, ",", table_data, " where ", table, ".name=", table_data, ".name and ", key, " like '%,'||?||',%'", " limit ? offset ?", NULL );
    db_query( &db, sql, keyword, keyword, keyword, keyword, limit_str, offset_str, NULL);
    free( sql );
    free( limit_str );
    free( offset_str );

    pkg_list = (YPackageList *)malloc( sizeof( YPackageList ) );
    pkg_list->htl = hash_table_list_init( limit );

    cur_pkg_index = 0;
    pkg_list->cnt = 0;

    while( db_fetch_assoc( &db ) )
    {
        attr_keys_offset = attr_keys;
        while( cur_key = *attr_keys_offset++ )
        {
            cur_value = db_get_value_by_key( &db, cur_key );
            hash_table_list_add_data( pkg_list->htl, cur_pkg_index, cur_key, cur_value );
        }
        cur_pkg_index++;
    }
    pkg_list->cnt = cur_pkg_index;

    db_close( &db );

    if( pkg_list->cnt == 0 )
    {
        hash_table_list_cleanup( pkg_list->htl );
        free( pkg_list );
        return NULL;
    }

    return pkg_list;

}

/*
 * packages_get_list_by_recommended
 */
YPackageList *packages_get_list_by_recommended( YPackageManager *pm, int limit, int offset, char *recommended, int installed )
{
    return packages_get_list_with_data( pm, limit, offset, "data_recommended", recommended, installed );
}

/*
 * packages_get_list_by_conflict
 */
YPackageList *packages_get_list_by_conflict( YPackageManager *pm, int limit, int offset, char *conflict, int installed )
{
    return packages_get_list_with_data( pm, limit, offset, "data_conflict", conflict,installed );
}

/*
 * packages_get_list_by_depend
 */
YPackageList *packages_get_list_by_depend( YPackageManager *pm, int limit, int offset, char *depend, int installed )
{
    return packages_get_list_with_data( pm, limit, offset, "data_depend", depend,installed );
}

/*
 * packages_get_list_by_bdepend
 */
YPackageList *packages_get_list_by_bdepend( YPackageManager *pm, int limit, int offset, char *bdepend, int installed )
{
    return packages_get_list_with_data( pm, limit, offset, "data_bdepend", bdepend,installed );
}

/*
 * packages_get_list_by_file
 */
YPackageList *packages_get_list_by_file( YPackageManager *pm, int limit, int offset, char *file )
{    
    int                     ret, cur_pkg_index;
    char                    *sql, *offset_str, *limit_str, *cur_key, *cur_value, **attr_keys_offset;
    char                    *attr_keys[] = { "name", "generic_name", "category", "priority", "version", "license", "description", "size", "file", "type", "extra", NULL  }; 
    DB                      db;
    YPackageList            *pkg_list;

    if( !file )
        return NULL;

    if( offset < 0 )
        offset = 0;

    if( limit < 1 )
        limit = 5;

    offset_str = util_int_to_str( offset );
    limit_str = util_int_to_str( limit );

    ret = db_init( &db, pm->db_name, OPEN_READ );
    sql = util_strcat( "select distinct * from world,world_file where world.name=world_file.name  and file like ", file[0] == '/' ? "'%'" : "'%/'", "||? limit ? offset ?", NULL );
    db_query( &db, sql, file, limit_str, offset_str, NULL);
    free( sql );
    free( limit_str );
    free( offset_str );

    pkg_list = (YPackageList *)malloc( sizeof( YPackageList ) );
    pkg_list->htl = hash_table_list_init( limit );

    cur_pkg_index = 0;
    pkg_list->cnt = 0;
    while( db_fetch_assoc( &db ) )
    {
        attr_keys_offset = attr_keys;
        while( cur_key = *attr_keys_offset++ )
        {
            cur_value = db_get_value_by_key( &db, cur_key );
            hash_table_list_add_data( pkg_list->htl, cur_pkg_index, cur_key, cur_value );
        }
        cur_pkg_index++;
    }
    pkg_list->cnt = cur_pkg_index;

    db_close( &db );

    if( pkg_list->cnt == 0 )
    {
        hash_table_list_cleanup( pkg_list->htl );
        free( pkg_list );
        return NULL;
    }

    return pkg_list;
}

/*
 * packages_free_list
 */
void packages_free_list( YPackageList *pkg_list )
{
    if( !pkg_list )
        return;

    hash_table_list_cleanup( pkg_list->htl );
    free( pkg_list );
}

/*
 * packages_get_list_attr
 */
char *packages_get_list_attr( YPackageList *pkg_list, int index, char *key )
{
    return hash_table_list_get_data( pkg_list->htl, index, key );
}

char *packages_get_list_attr2( YPackageList *pkg_list, int index, char *key )
{
    char *result;

    result = hash_table_list_get_data( pkg_list->htl, index, key );
    return result ? result : "";
}


static int packages_download_progress_callback( void *arg,double dltotal, double dlnow, double ultotal, double ulnow )
{
    YPackageDCB         *cb;

    if( !arg )
        return 1;

    cb = (YPackageDCB *)arg;
    if( cb->dcb )
        return cb->dcb( cb->dcb_arg, cb->package_name, dltotal, dlnow );
    else if( cb->pcb )
        return cb->pcb( cb->pcb_arg, cb->package_name, 2, dlnow / dltotal, NULL );
    else
        return 1;

    //return cb->cb( dcb->arg, dltotal, dlnow );
}

/*
 * packages_download_package
 */
int packages_download_package( YPackageManager *pm, char *package_name, char *url, char *dest, int force, ypk_download_callback dcb, void *dcb_arg, ypk_progress_callback pcb, void *pcb_arg  )
{
    int                 return_code;
    char                *target_url, *package_url, *package_path;
    DownloadFile        file;
    YPackage            *pkg;
    YPackageDCB         *cb = NULL;

    if( (!url || !dest) && (!pm || !package_name) )
        return -1;

    return_code = 0;
    if( url && dest )
    {
        target_url = strdup( url );
        package_path = strdup( dest );
        pkg = NULL;
    }
    else
    {
        pkg = packages_get_package( pm, package_name, 0 );
        if( !pkg )
        {
            return -2;
        }


        package_url = packages_get_package_attr( pkg, "uri" );
        if( !package_url )
        {
            return_code = -3;
            goto return_point;
        }

        package_path = util_strcat( pm->package_dest, "/", package_url+2, NULL );
        target_url = util_strcat( pm->source_uri, "/", package_url, NULL );
    }

    if( force || access( package_path, R_OK ) )
    {
        file.file = package_path;
        file.stream = NULL;
        cb = NULL;
        if( dcb || pcb )
        {
            cb = (YPackageDCB *)malloc( sizeof( YPackageDCB ) );
            if( cb )
            {
                memset( cb, 0, sizeof( YPackageDCB ) );

                cb->package_name = package_name;
                if( dcb )
                {
                    cb->dcb = dcb;
                    cb->dcb_arg = dcb_arg;
                }

                if( pcb )
                {
                    cb->pcb = pcb;
                    cb->pcb_arg = pcb_arg;
                }


                file.cb = packages_download_progress_callback;
                file.cb_arg = (void *)cb;
            }
            else
            {
                file.cb = NULL;
                file.cb_arg = NULL;
            }
        }
        else
        {
            file.cb = NULL;
            file.cb_arg = NULL;
        }

        if( download_file( target_url, &file ) != 0 )
        {
            if( file.stream )
                fclose( file.stream );
            return_code = -4;
            goto return_point;
        }
        fclose( file.stream );
        if( cb )
        {
            free( cb );
            cb = NULL;
        }
    }

return_point:
    if( target_url )
        free( target_url );

    if( package_path )
        free( package_path );

    if( pkg )
        packages_free_package( pkg );

    if( cb )
        free( cb );
    return return_code;
}


/*
 * packages_install_package
 */
int packages_install_package( YPackageManager *pm, char *package_name, ypk_progress_callback cb, void *cb_arg  )
{
    int                 return_code;
    char                *target_url = NULL, *package_url = NULL, *package_path = NULL, *pkg_sha = NULL, *ypk_sha = NULL;
    YPackage            *pkg;

    if( !package_name )
        return -1;

    if( cb )
    {
        cb( cb_arg, package_name, 0, 2, "install" );
        cb( cb_arg, package_name, 1, -1, "initialization" );
    }

    return_code = 0;
    pkg = packages_get_package( pm, package_name, 0 );
    if( !pkg )
    {
        return -2;
    }

    package_url = packages_get_package_attr( pkg, "uri" );
    if( !package_url )
    {
        return_code = -3;
        goto return_point;
    }

    if( cb )
    {
        cb( cb_arg, package_name, 1, 1, NULL );
    }

    package_path = util_strcat( pm->package_dest, "/", package_url+2, NULL );
    target_url = util_strcat( pm->source_uri, "/", package_url, NULL );
    pkg_sha = packages_get_package_attr( pkg, "sha" );

    if( !access( package_path, F_OK ) )
    {
        ypk_sha = util_sha1( package_path );

        if( ypk_sha && strncmp( pkg_sha, ypk_sha, 41 ) )
        {
            if( packages_download_package( NULL, package_name, target_url, package_path, 1, NULL, NULL, cb, cb_arg  ) < 0 )
            {
                return -4;
                goto return_point;
            }

            free( ypk_sha );

            ypk_sha = util_sha1( package_path );
            if( ypk_sha && strncmp( pkg_sha, ypk_sha, 41 ) )
            {
                return -4;
                goto return_point;
            }
        }
    }
    else
    {
        if( packages_download_package( NULL, package_name, target_url, package_path, 0, NULL, NULL, cb, cb_arg  ) < 0 )
        {
            return -4;
            goto return_point;
        }

        ypk_sha = util_sha1( package_path );
        if( ypk_sha && strncmp( pkg_sha, ypk_sha, 41 ) )
        {
            return -4;
            goto return_point;
        }
    }

    if( packages_install_local_package( pm, package_path, "/", 0, cb, cb_arg ) )
    {
        return_code = -5;
    }

return_point:
    if( package_path )
        free( package_path );

    if( target_url )
        free( target_url );

    if( ypk_sha )
        free( ypk_sha );

    packages_free_package( pkg );
    return return_code;
}


YPackageChangeList *packages_get_depend_list( YPackageManager *pm, char *package_name )
{
    int             i, len;
    char            *token, *depend;
    YPackageData    *pkg_data;
    YPackageChangeList    *list, *cur_pkg, *sub_list, *tmp;

    if( packages_has_installed( pm, package_name, NULL ) )
    {
        return NULL;
    }
    
    list = NULL;

    if( pkg_data = packages_get_package_data( pm, package_name, 0 ) )
    {
        for( i = 0; i < pkg_data->cnt; i++ )
        {
            depend = packages_get_package_data_attr( pkg_data, i, "data_depend");
            if( depend )
            {
                token = strtok( depend, " ,");
                while( token )
                {
                    if( !packages_has_installed( pm, token, NULL ) )
                    {
                        cur_pkg =  (YPackageChangeList *)malloc( sizeof( YPackageChangeList ) );
                        len = strlen( token );
                        cur_pkg->name = (char *)malloc( len + 1 );
                        strncpy( cur_pkg->name, token, len );
                        cur_pkg->name[len] = 0;
                        cur_pkg->type = 2;
                        cur_pkg->prev = list;
                        list = cur_pkg;

                        sub_list = packages_get_depend_list( pm, cur_pkg->name );
                        if( sub_list )
                        {
                            tmp = sub_list;
                            while( tmp->prev )
                                tmp = tmp->prev;
                            tmp->prev = list;
                            list = sub_list;
                        }
                    }
                    token = strtok( NULL, " ,");
                }
            }

        }
        packages_free_package_data( pkg_data );
    }

    return list;
}

YPackageChangeList *packages_get_recommended_list( YPackageManager *pm, char *package_name )
{
    int             i, len;
    char            *token, *recommended;
    YPackageData    *pkg_data;
    YPackageChangeList    *list, *cur_pkg, *sub_list, *tmp;

    if( packages_has_installed( pm, package_name, NULL ) )
    {
        return NULL;
    }
    
    list = NULL;

    if( pkg_data = packages_get_package_data( pm, package_name, 0 ) )
    {
        for( i = 0; i < pkg_data->cnt; i++ )
        {
            recommended = packages_get_package_data_attr( pkg_data, i, "data_recommended");
            if( recommended )
            {
                token = strtok( recommended, " ,");
                while( token )
                {
                    if( !packages_has_installed( pm, token, NULL ) )
                    {
                        cur_pkg =  (YPackageChangeList *)malloc( sizeof( YPackageChangeList ) );
                        len = strlen( token );
                        cur_pkg->name = (char *)malloc( len + 1 );
                        strncpy( cur_pkg->name, token, len );
                        cur_pkg->name[len] = 0;
                        cur_pkg->type = 3;
                        cur_pkg->prev = list;
                        list = cur_pkg;

                        sub_list = packages_get_recommended_list( pm, cur_pkg->name );
                        if( sub_list )
                        {
                            tmp = sub_list;
                            while( tmp->prev )
                                tmp = tmp->prev;
                            tmp->prev = list;
                            list = sub_list;
                        }
                    }
                    token = strtok( NULL, " ,");
                }
            }
        }
        packages_free_package_data( pkg_data );
    }

    return list;
}

YPackageChangeList *packages_get_bdepend_list( YPackageManager *pm, char *package_name )
{
    int                     i, len;
    char                    *token, *depend, *dev, *bdepend;
    YPackageData            *pkg_data;
    YPackageChangeList      *list, *cur_pkg, *sub_list, *tmp;

    list = NULL;

    if( pkg_data = packages_get_package_data( pm, package_name, 0 ) )
    {
        for( i = 0; i < pkg_data->cnt; i++ )
        {
            depend = packages_get_package_data_attr( pkg_data, i, "data_depend");
            if( depend )
            {
                token = strtok( depend, " ,");
                while( token )
                {
                    dev = util_strcat( token, "-dev", NULL );
                    if( !packages_exists( pm, dev, NULL ) )
                    {
                        free( dev );
                        token = strtok( NULL, " ,");
                        continue;
                    }

                    if( !packages_has_installed( pm, dev, NULL ) )
                    {
                        cur_pkg =  (YPackageChangeList *)malloc( sizeof( YPackageChangeList ) );
                        len = strlen( dev );
                        cur_pkg->name = (char *)malloc( len + 1 );
                        strncpy( cur_pkg->name, dev, len );
                        cur_pkg->name[len] = 0;
                        cur_pkg->type = 2;
                        cur_pkg->prev = list;
                        list = cur_pkg;
                    }
                    free( dev );
                    token = strtok( NULL, " ,");
                }
            }

            bdepend = packages_get_package_data_attr( pkg_data, i, "data_bdepend");
            if( bdepend )
            {
                token = strtok( bdepend, " ,");
                while( token )
                {
                    if( !packages_has_installed( pm, token, NULL ) )
                    {
                        cur_pkg =  (YPackageChangeList *)malloc( sizeof( YPackageChangeList ) );
                        len = strlen( token );
                        cur_pkg->name = (char *)malloc( len + 1 );
                        strncpy( cur_pkg->name, token, len );
                        cur_pkg->name[len] = 0;
                        cur_pkg->type = 2;
                        cur_pkg->prev = list;
                        list = cur_pkg;
                    }
                    token = strtok( NULL, " ,");
                }
            }

        }
        packages_free_package_data( pkg_data );
    }

    return list;
}

static void packages_free_change_list( YPackageChangeList *list )
{
    YPackageChangeList    *cur_pkg;

    if( !list )
        return;


    while( list )
    {
        cur_pkg = list;
        list = list->prev;
        free( cur_pkg->name );
        free( cur_pkg );
    }
}

/*
 * packages_get_install_list
 */
YPackageChangeList *packages_get_install_list( YPackageManager *pm, char *package_name )
{
    int             len;
    YPackageChangeList    *list, *depend, *cur_pkg;

    if( packages_has_installed( pm, package_name, NULL ) )
    {
        return NULL;
    }
    
    list = NULL;

    depend = packages_get_recommended_list( pm, package_name );
    if( depend )
    {
        cur_pkg = depend;
        while( cur_pkg->prev )
            cur_pkg = cur_pkg->prev;
        cur_pkg->prev = list;
        list = depend;
    }
    
    cur_pkg =  (YPackageChangeList *)malloc( sizeof( YPackageChangeList ) );
    len = strlen( package_name );
    cur_pkg->name = (char *)malloc( len + 1 );
    strncpy( cur_pkg->name, package_name, len );
    cur_pkg->name[len] = 0;
    cur_pkg->type = 1;
    cur_pkg->prev = list;
    list = cur_pkg;

    depend = packages_get_depend_list( pm, package_name );
    if( depend )
    {
        cur_pkg = depend;
        while( cur_pkg->prev )
            cur_pkg = cur_pkg->prev;
        cur_pkg->prev = list;
        list = depend;
    }


    return list;
}

void packages_free_install_list( YPackageChangeList *list )
{
    packages_free_change_list( list );
}

/*
 * packages_get_dev_list
 */
YPackageChangeList *packages_get_dev_list( YPackageManager *pm, char *package_name )
{
    return packages_get_bdepend_list( pm, package_name );
}

void packages_free_dev_list( YPackageChangeList *list )
{
    packages_free_change_list( list );
}


/*
 * packages_install_list
 */
int packages_install_list( YPackageManager *pm, YPackageChangeList *list, ypk_progress_callback cb, void *cb_arg )
{
    int                     ret;
    YPackageChangeList      *cur_pkg;

    if( !list )
        return 0;


    while( list )
    {
        cur_pkg = list;
        
        ret = packages_install_package( pm, cur_pkg->name, cb, cb_arg );
        if( ret )
        {
            return -1;
        }
        list = list->prev;
    }

    return 0;
}


/*
 * package_get_remove_list
 */
YPackageChangeList *packages_get_remove_list( YPackageManager *pm, char *package_name )
{
    int             i, len;
    char            *name, *size;
    YPackageList    *pkg_list;
    YPackageChangeList     *list, *cur_pkg;

    list = NULL;

    cur_pkg =  (YPackageChangeList *)malloc( sizeof( YPackageChangeList ) );
    len = strlen( package_name );
    cur_pkg->name = (char *)malloc( len + 1 );
    strncpy( cur_pkg->name, package_name, len );
    cur_pkg->name[len] = 0;
    cur_pkg->type = 1;
    cur_pkg->prev = NULL;
    list = cur_pkg;

    pkg_list = packages_get_list_by_depend( pm, 2000, 0, package_name, 1 );
    if( pkg_list )
    {
        for( i = 0; i < pkg_list->cnt; i++ )
        {
            name =  packages_get_list_attr( pkg_list, i, "name");
            size =  packages_get_list_attr( pkg_list, i, "size");
            cur_pkg =  (YPackageChangeList *)malloc( sizeof( YPackageChangeList ) );
            len = strlen( name );
            cur_pkg->name = (char *)malloc( len + 1 );
            strncpy( cur_pkg->name, name, len );
            cur_pkg->name[len] = 0;
            cur_pkg->type = 1;
            cur_pkg->size = atoi( size );
            cur_pkg->prev = list;
            list = cur_pkg;
        }
        packages_free_list( pkg_list );
    }

    return list;
}


/*
 * packages_remove_list
 */
int packages_remove_list( YPackageManager *pm, YPackageChangeList *list, ypk_progress_callback cb, void *cb_arg  )
{
    YPackageChangeList    *cur_pkg;

    if( !list )
        return;


    while( list )
    {
        cur_pkg = list;
        
        //printf("removing %s ...\n", cur_pkg->name );
        packages_remove_package( pm, cur_pkg->name, cb, cb_arg );
        list = list->prev;
    }
}

void packages_free_remove_list( YPackageChangeList *list )
{
    packages_free_change_list( list );
}


/*
 * packages_check_package
 */
int packages_check_package( YPackageManager *pm, char *ypk_path, char *extra, int extra_max_len )
{
    int                 i, ret, return_code = 0;
    char                *depend, *conflict, *token, *package_name, *arch, *version, *version2;
    struct utsname      buf;
    YPackage            *pkg = NULL, *pkg2 = NULL;
    YPackageData        *pkg_data = NULL;
    YPackageFile        *pkg_file;

    if( !ypk_path || access( ypk_path, R_OK ) )
        return -1;

    //get ypk's infomations
    if( packages_get_package_from_ypk( ypk_path, &pkg, &pkg_data ) < 0 )
    {
        return -1;
    }


    if( !( pkg_file = packages_get_package_file_from_ypk( ypk_path ) ) )
    {
        return -1;
    }
    packages_free_package_file( pkg_file );

    package_name = packages_get_package_attr( pkg, "name" );
    arch = packages_get_package_attr( pkg, "arch" );
    version = packages_get_package_attr( pkg, "version" );

    //check arch
    if( arch && ( arch[0] != 'a' || arch[1] != 'n' || arch[2] != 'y' ) )
    {
        if( !uname( &buf ) )
        {
            if( strcmp( buf.machine, arch ) )
            {
                return_code = -2;

                if( extra && extra_max_len > 0 )
                {
                    strncpy( extra, buf.machine, extra_max_len );
                }

                goto return_point;
            }
        }
    }

    for( i = 0; i < pkg_data->cnt; i++ )
    {
        //check depend
        depend = packages_get_package_data_attr( pkg_data, i, "data_depend");
        if( depend )
        {
            token = strtok( depend, " ,");
            while( token )
            {
                if( !packages_has_installed( pm, token, NULL ) )
                {
                    return_code = -3; 

                    if( extra && extra_max_len > 0 )
                    {
                        strncpy( extra, token, extra_max_len );
                    }

                    goto return_point;
                }
                token = strtok( NULL, " ,");
            }
        }

        //check conflict
        conflict = packages_get_package_data_attr( pkg_data, i, "data_conflict");
        if( conflict )
        {
            token = strtok( conflict, " ,");
            while( token )
            {
                if( packages_has_installed( pm, token, NULL ) )
                {
                    return_code = -4; 

                    if( extra && extra_max_len > 0 )
                    {
                        strncpy( extra, token, extra_max_len );
                    }

                    goto return_point;
                }
                token = strtok( NULL, " ,");
            }
        }
    }

    //check installed
    if( pkg2 = packages_get_package( pm, package_name, 1 )  )
    {
        version2 = packages_get_package_attr( pkg2, "version" );
        if( version && (strlen( version ) > 0) && version2 && (strlen( version2 ) > 0) )
        {
            ret = packages_compare_version( version, version2 );
            if( ret > 0 )
                return_code = 3; 
            else if( ret == 0 )
                return_code = 2; 
            else
                return_code = 1; 

            if( extra && extra_max_len > 0 )
            {
                strncpy( extra, version2, extra_max_len );
            }

            goto return_point;
        }
    }

return_point:
    if( pkg )
        packages_free_package( pkg );

    if( pkg2 )
        packages_free_package( pkg2 );

    if( pkg_data )
        packages_free_package_data( pkg_data );

    return return_code;
}

/*
 * packages_unpack_package
 *
 * params: unzip_info - 0,1,2
 */
int packages_unpack_package( YPackageManager *pm, char *ypk_path, char *dest_dir, int unzip_info )
{
    char                tmp_ypk_data[] = "/tmp/ypkdata.XXXXXX", tmp_ypk_info[] = "/tmp/ypkinfo.XXXXXX";
    char                *info_dest_dir;

    if( !ypk_path )
        return -1;

    if( !dest_dir )
        dest_dir = "./";

    //unzip pkgdata
    if( unzip_info < 2 )
    {
        mkstemp( tmp_ypk_data );
        if( archive_extract_file( ypk_path, "pkgdata", tmp_ypk_data ) == -1 )
        {
            return -2;
        }

        //copy files 
        //printf( "unpacking files ...\n");
        if( archive_extract_all( tmp_ypk_data, dest_dir ) == -1 )
        {
            remove( tmp_ypk_data );
            return -3;
        }
        
        remove( tmp_ypk_data );
    }

    //unzip pkginfo
    if( unzip_info )
    {
        mkstemp( tmp_ypk_info );
        if( archive_extract_file( ypk_path, "pkginfo", tmp_ypk_info ) == -1 )
        {
            return -4;
        }

        util_rtrim( dest_dir, '/' );
        info_dest_dir = util_strcat( dest_dir, "/YLMFOS", NULL );

        if( archive_extract_all( tmp_ypk_info, info_dest_dir ) == -1 )
        {
            free(info_dest_dir);
            remove( tmp_ypk_info );
            return -5;
        }
        
        free(info_dest_dir);
        remove( tmp_ypk_info );
    }

    return 0;
}

/*
 * packages_pack_package
 */
int packages_pack_package( YPackageManager *pm, char *source_dir, char *ypk_path )
{
}

/*
 * compare version
 */
int packages_compare_version( char *version1, char *version2 )
{
    int         buf_size = 16, result;
    char        *pattern = "(?<ver>(.*?)+)(-(?<ver1>.*?))?(-(?<ver2>.*?))?(-(?<ver3>.*?))?$";
    char        buf1[buf_size], buf2[buf_size], *sub_ver1, *sub_ver2;
    PREGInfo    pi1, pi2;

    if( preg_match( &pi1, pattern, version1, 0, 1 ) > 0 &&  preg_match( &pi2, pattern, version2, 0, 1 ) > 0 )
    {
        if( preg_result2(&pi1, "ver", buf1, buf_size) > 0 && preg_result2(&pi2, "ver", buf2, buf_size) > 0 )
        {
            result = packages_compare_main_version( buf1, buf2 );
            if( !result )
            {
                if( preg_result2(&pi1, "ver1", buf1, buf_size) > 0 )
                    sub_ver1 = buf1;
                else
                    sub_ver1 = NULL;


                if( preg_result2(&pi2, "ver1", buf2, buf_size) > 0 )
                    sub_ver2 = buf2;
                else
                    sub_ver2 = NULL;
                

                result = packages_compare_sub_version( sub_ver1, sub_ver2 );
                if( !result )
                {
                    if( preg_result2(&pi1, "ver2", buf1, buf_size) > 0 )
                        sub_ver1 = buf1;
                    else
                        sub_ver1 = NULL;

                    if( preg_result2(&pi2, "ver2", buf2, buf_size) > 0 )
                        sub_ver2 = buf2;
                    else
                        sub_ver2 = NULL;

                    result = packages_compare_sub_version( sub_ver1, sub_ver2 );
                    if( !result )
                    {
                        if( preg_result2(&pi1, "ver3", buf1, buf_size) > 0 )
                            sub_ver1 = buf1;
                        else
                            sub_ver1 = NULL;

                        if( preg_result2(&pi2, "ver3", buf2, buf_size) > 0 )
                            sub_ver2 = buf2;
                        else
                            sub_ver2 = NULL;

                        result = packages_compare_sub_version( sub_ver1, sub_ver2 );
                    }
                }
            }


            if( result && result != -2 )
            {
                result = result > 0 ? 1 : -1;
            }

        }
        else
        {
            result = -2;
        }
    }
    else
    {
        result = -2;
    }
    preg_free( &pi1 );
    preg_free( &pi2 );

    return result;
}

static int packages_compare_main_version( char *version1, char *version2 )
{
    int         ver1, ver2, ret1, ret2, result = 0, buf_size = 16;
    char        *pattern = "(\\d+)";
    char        buf1[buf_size], buf2[buf_size];
    PREGInfo    pi1, pi2;

    ret1 = preg_match( &pi1, pattern, version1, 0, 1 );
    ret2 = preg_match( &pi2, pattern, version2, 0, 1 );

    while( ret1 > 0 &&  ret2 > 0 )
    {
        if( preg_result(&pi1, 0, buf1, buf_size) > 0 && preg_result2(&pi2, 0, buf2, buf_size) > 0 )
        {
            if( buf1 && buf2 )
            {
                ver1 = atoi( buf1 );
                ver2 = atoi( buf2 );
                if( result =  ver1 - ver2 )
                {
                    result = result > 0 ? 1 : -1;
                    break;
                }
            }
            else if( !buf1 && !buf2 )
            {
                result = 0;
                break;
            }
            else if( !buf1 )
            {
                result = 1;
                break;
            }
            else
            {
                result = -1;
                break;
            }
        }
        else
        {
            result = -2;
            break;
        }

        ret1 = preg_match( &pi1, pattern, version1, 0, 0 );
        ret2 = preg_match( &pi2, pattern, version2, 0, 0 );
    }

    if( ret1 > 0 && ret2 < 0 )
        result = 1;
    else if( ret1 < 0 && ret2 > 0 )
        result = -1;

    if( result == 0 )
    {
        result = strcmp( version1, version2 );
    }

    preg_free( &pi1 );
    preg_free( &pi2 );

    return result;
}

static int packages_compare_sub_version( char *version1, char *version2 )
{
    int level1, level2;


    if( version1 == NULL )
        level1 = 2;
    else if( isdigit( version1[0] ) )
        level1 = 4;
    else if( version1[0] == 'y' && version1[1] == 'l' )
        level1 = 3;
    else
        level1 = 1;

    if( version2 == NULL )
        level2 = 2;
    else if( isdigit( version2[0] ) )
        level2 = 4;
    else if( version2[0] == 'y' && version2[1] == 'l' )
        level2 = 3;
    else
        level2 = 1;

    if( level1 == level2 )
    {
        if( level1 == 2 )
            return 0;
        else if( level1 == 4 )
            return atoi( version1 ) - atoi( version2 ); // 2 > 1
        else
            return strcmp( version1, version2 ); //ymlf2 > ylmf1 > rc1 > beta1 > alpha1 
    }
    else
    {
        return level1 - level2; // foo_1.0-1 > foo_1.0 > foo_1.0-ylmf1
    }
}


/*
 * packages_exec_script
 */
int packages_exec_script( char *script, char *package_name, char *version, char *version2, char *action )
{
    int     status;
    char    *cmd;


    if( !script || !package_name || !version || !action )
        return -1;

    if( access( script, R_OK ) != 0 )
        return -1;

    cmd = util_strcat( "source '", script, "'; if type ", action, " >/dev/null 2>&1; then ", action, " ", package_name, " ", version, " ", version2 ? version2 : "", "; fi", NULL );
    //printf( "exec: %s\n", cmd );
    status = system( cmd );
    free( cmd );

    if( WEXITSTATUS( status ) )
    {
        return -1;
    }
    return 0;
}

/*
 * packages_install_local_package
 */
int packages_install_local_package( YPackageManager *pm, char *ypk_path, char *dest_dir, int force, ypk_progress_callback cb, void *cb_arg  )
{
    int                 i, j, ret, installed, upgrade, return_code;
    size_t              pkginfo_len, filelist_len;
    void                *pkginfo, *filelist;
    char                *msg, *sql, *sql_data, *sql_filelist, *sql_filelist2, *install_file, *list_line;
    char                *package_name, *version, *version2, *file_type, *file_file, *file_size, *file_perms, *file_uid, *file_gid, *file_mtime, *file_extra, *can_update;
    char                tmp_ypk_install[] = "/tmp/ypkinstall.XXXXXX";
    char                extra[32];
    YPackage            *pkg, *pkg2;
    YPackageData        *pkg_data;
    YPackageFile        *pkg_file;
    DB                  db;


    installed = 0;
    upgrade = 0;
    return_code = 0;
    msg = NULL;
    pkginfo = NULL; 
    filelist = NULL;
    pkg = NULL; 
    pkg2 = NULL; 
    pkg_data = NULL;

    //check
    if( !ypk_path || access( ypk_path, R_OK ) )
        return -1;


    if( cb )
    {
        cb( cb_arg, ypk_path, 3, -1, "check dependencies of package" );
    }

    memset( extra, 0, 32 );
    ret = packages_check_package( pm, ypk_path, extra, 32 ); //ret = -4 ~ 3

    if( cb )
    {
        switch( ret )
        {
            case 1:
            case 2:
                msg = util_strcat( "The latest version has installed.", NULL );
                break;

            case -1:
                msg = util_strcat( "Error: Invalid format or File not found.", NULL );
                break;

            case -2:
                msg = util_strcat( "Error: Architecture does not match. (",  extra, ")", NULL );
                break;

            case -3:
                msg = util_strcat( "Error: missing runtime deps: ", extra, NULL );
                break;

            case -4:
                msg = util_strcat( "Error: conflicting deps: ", extra, NULL );
                break;

        }

        cb( cb_arg, ypk_path, 3, 1, msg ? msg : "ok" );

        if( msg )
        {
            free( msg );
            msg = NULL;
        }
    }


    if( ret == -1 )
    {
        return -1;
    }
    else if( ret < -1 && !force )
    {
        return ret;
    }
    else if( ret == 1 )
    {
        if( !force )
            return 1;

        installed = 1;
        upgrade = -1;
    }
    else if( ret == 2 )
    {
        if( !force )
            return 1;

        installed = 1;
    }
    else if( ret == 3 )
    {
        installed = 1;
        upgrade = 1;
    }



    if( !dest_dir )
        dest_dir = "/";


    //get package infomations
    if( cb )
    {
        cb( cb_arg, ypk_path, 4, -1, "reading package information" );
    }

    packages_get_package_from_ypk( ypk_path, &pkg, &pkg_data );
    if( !pkg || !pkg_data )
    {
        return_code = -5;
        goto return_point;
    }
    package_name = packages_get_package_attr( pkg, "name" );
    version = packages_get_package_attr( pkg, "version" );

    if( cb )
    {
        cb( cb_arg, ypk_path, 4, 1, NULL );
    }

    //extract install script
    if( cb )
    {
        cb( cb_arg, ypk_path, 5, -1, "executing pre_install script" );
    }

    mkstemp( tmp_ypk_install );
    ret = archive_extract_file2( ypk_path, "pkginfo", &pkginfo, &pkginfo_len );
    if( ret == -1 )
    {
        return_code = -6;
        goto return_point;
    }

    install_file = util_strcat( package_name, ".install", NULL );
    ret = archive_extract_file3( pkginfo, pkginfo_len, install_file, tmp_ypk_install );
    free( install_file );

    //exec pre_x script
    if( ret != -1 )
    {
        if( !upgrade )
        {
            if( packages_exec_script( tmp_ypk_install, package_name, version, NULL, "pre_install" ) == -1 )
            {
                return_code = -7; 
                goto return_point;
            }
        }
        else if( upgrade == 1 )
        {
            version2 = extra;
            if( packages_exec_script( tmp_ypk_install, package_name, version, version2, "pre_upgrade" ) == -1 )
            {
                return_code = -7; 
                goto return_point;
            }
        }
        else //downgrade
        {
            version2 = extra;
            if( packages_exec_script( tmp_ypk_install, package_name, version, version2, "pre_downgrade" ) == -1 )
            {
                return_code = -7; 
                goto return_point;
            }
        }
    }

    if( cb )
    {
        cb( cb_arg, ypk_path, 5, 1, NULL );
    }

    //copy files 
    //printf( "copying files ...\n");
    if( cb )
    {
        cb( cb_arg, ypk_path, 6, -1, "copying files" );
    }

    if( (ret = packages_unpack_package( pm, ypk_path, dest_dir, 0 )) != 0 )
    {
        //printf("unpack ret:%d\n",ret);
        return_code = -8; 
        goto return_point;
    }

    if( cb )
    {
        cb( cb_arg, ypk_path, 6, 1, NULL );
    }

    //exec post_x script
    if( cb )
    {
        cb( cb_arg, ypk_path, 7, -1, "executing post_install script" );
    }

    if( !access( tmp_ypk_install, R_OK ) )
    {
        if( !upgrade )
        {
            if( packages_exec_script( tmp_ypk_install, package_name, version, NULL, "post_install" ) == -1 )
            {
                return_code = -9; 
                goto return_point;
            }
        }
        else if( upgrade == 1 )
        {
            version2 = extra;
            if( packages_exec_script( tmp_ypk_install, package_name, version, version2, "post_upgrade" ) == -1 )
            {
                return_code = -9; 
                goto return_point;
            }
        }
        else //downgrade
        {
            version2 = extra;
            if( packages_exec_script( tmp_ypk_install, package_name, version, version2, "post_downgrade" ) == -1 )
            {
                return_code = -9; 
                goto return_point;
            }
        }
    }

    if( cb )
    {
        cb( cb_arg, ypk_path, 7, 1, NULL );
    }

    //update db
    //printf( "updating database ...\n");
    if( cb )
    {
        cb( cb_arg, ypk_path, 8, -1, "updating database" );
    }

    db_init( &db, pm->db_name, OPEN_WRITE );
    db_cleanup( &db );
    db_exec( &db, "begin", NULL );  

    //update world
    sql = "replace into world (name, generic_name, is_desktop, category, arch, version, priority, install, license, homepage, repo, size, sha, build_date, packager, uri, description, data_count, install_time) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, strftime('%s','now'));";


    ret = db_exec( &db, sql,  
            package_name, //name
            packages_get_package_attr2( pkg, "generic_name"), //generic_name
            packages_get_package_attr2( pkg, "is_desktop"), //desktop
            packages_get_package_attr2( pkg, "category" ), //category
            packages_get_package_attr2( pkg, "arch" ), //arch
            version, //version
            packages_get_package_attr2( pkg, "priority" ), //priority
            packages_get_package_attr2( pkg, "install" ), //install
            packages_get_package_attr2( pkg, "license" ), //license
            packages_get_package_attr2( pkg, "homepage" ), //homepage
            packages_get_package_attr2( pkg, "repo" ), //repo
            packages_get_package_attr2( pkg, "size" ), //size
            packages_get_package_attr2( pkg, "sha" ), //repo
            packages_get_package_attr2( pkg, "build_date" ), //build_date
            packages_get_package_attr2( pkg, "packager" ), //packager
            packages_get_package_attr2( pkg, "uri" ), //uri
            packages_get_package_attr2( pkg, "description" ), //description
            packages_get_package_attr2( pkg, "data_count" ), //data_count
            NULL);
    
    //update world_data
    db_exec( &db, "delete from world_data where name=?", package_name, NULL );  
    sql_data = "insert into world_data (name, version, data_name, data_format, data_size, data_install_size, data_depend, data_bdepend, data_recommended, data_conflict) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    for( i = 0; i < pkg_data->cnt; i++ )
    {
        ret = db_exec( &db, sql_data,  
                package_name, //name
                version, //version
                packages_get_package_data_attr2( pkg_data, i, "data_name"), //data_name
                packages_get_package_data_attr2( pkg_data, i, "data_format" ), //data_format
                packages_get_package_data_attr2( pkg_data, i, "data_size" ), //data_size
                packages_get_package_data_attr2( pkg_data, i, "data_install_size" ), //data_install_size
                packages_get_package_data_attr2( pkg_data, i, "data_depend" ), //data_depend
                packages_get_package_data_attr2( pkg_data, i, "data_bdepend" ), //data_bdepend
                packages_get_package_data_attr2( pkg_data, i, "data_recommended" ), //data_recommended
                packages_get_package_data_attr2( pkg_data, i, "data_conflict" ), //data_conflict
                NULL);
    }

    //extract filelist
    ret = archive_extract_file4( pkginfo, pkginfo_len, "filelist", &filelist, &filelist_len );
    if( ret == -1 )
    {
        return_code = -10; 
        goto return_point;
    }

    //delete the files only in the old version
    if( installed )
    {
        sql_filelist = "delete from world_file where name=? and file=?"; 
        list_line = util_mem_gets( filelist );
        while( list_line )
        {
            if( list_line[0] != 'I' )
            {
                file_type = strtok( list_line, " ,");
                file_file = strtok( NULL, " ,");
                file_size = strtok( NULL, " ,");
                file_perms = strtok( NULL, " ,");
                file_uid = strtok( NULL, " ,");
                file_gid = strtok( NULL, " ,");
                file_mtime = strtok( NULL, " ,");
                file_extra = strtok( NULL, " ,");

                if( file_file )
                {
                    db_exec( &db, sql_filelist, package_name, file_file, NULL );
                }

            }

            free( list_line );
            list_line = util_mem_gets( NULL );
        }
        db_exec( &db, "commit", NULL );  

        pkg_file = packages_get_package_file( pm, package_name );
        if( pkg_file )
        {
            for( j = 0; j < pkg_file->cnt; j++ )
            {
                file_file = packages_get_package_file_attr( pkg_file, j, "file");
                if( file_file )
                {
                    remove( file_file );
                }
            }
            packages_free_package_file( pkg_file );
        }

        db_exec( &db, "begin", NULL );  

    }

    //update file list
    db_exec( &db, "delete from world_file where name=?", package_name, NULL );  
    sql_filelist2 = "insert into world_file (name, version, type, file, size, perms, uid, gid, mtime) values (?, ?, ?, ?, ?, ?, ?, ?, ?)"; 

    list_line = util_mem_gets( filelist );
    while( list_line )
    {
        if( list_line[0] != 'I' )
        {
            file_type = strtok( list_line, " ,");
            file_file = strtok( NULL, " ,");
            file_size = strtok( NULL, " ,");
            file_perms = strtok( NULL, " ,");
            file_uid = strtok( NULL, " ,");
            file_gid = strtok( NULL, " ,");
            file_mtime = strtok( NULL, " ,");
            file_extra = strtok( NULL, " ,");


            ret = db_exec( &db, sql_filelist2, 
                    package_name,
                    version,
                    file_type ? file_type : "",
                    file_file ? file_file : "",
                    file_size ? file_size : "",
                    file_perms ? file_perms : "",
                    file_uid ? file_uid : "",
                    file_gid ? file_gid : "",
                    file_extra ? file_extra : "",
                    NULL );
        }

        free( list_line );
        list_line = util_mem_gets( NULL );
    }

    //update universe
    pkg2 = packages_get_package( pm, package_name, 0 );
    if( pkg2 )
    {
        version2 = packages_get_package_attr( pkg2, "version" );
        if( version && (strlen( version ) > 0) && version2 && (strlen( version2 ) > 0) )
        {
            can_update = packages_compare_version( version, version2 ) < 0 ? "1" : "0";
        }
        else
        {
            can_update = "0";
        }

        db_exec( &db, "update universe set installed='1', can_update=? where name=?", can_update, package_name, NULL );  
    }

    db_exec( &db, "commit", NULL );  
    db_close( &db );

    if( cb )
    {
        cb( cb_arg, ypk_path, 8, 1, NULL );
    }

    if( cb )
    {
        cb( cb_arg, ypk_path, 9, 1, NULL );
    }

return_point:
    if( pkginfo )
        free( pkginfo );

    if( filelist )
        free( filelist );

    if( pkg )
        packages_free_package( pkg );

    if( pkg2 )
    {
        packages_free_package( pkg2 );
    }

    if( pkg_data )
        packages_free_package_data( pkg_data );

    remove( tmp_ypk_install );
    return return_code;
}

int packages_remove_package( YPackageManager *pm, char *package_name, ypk_progress_callback cb, void *cb_arg  )
{
    int             ret, return_code = 0;
    char            *install_file, *version, *install_file_path = NULL, *sql_file, *file_path;
    YPackage         *pkg;
    DB              db;

    if( !package_name )
        return -1;

    if( cb )
    {
        cb( cb_arg, package_name, 0, 3, "remove" );
        cb( cb_arg, package_name, 1, -1, "initialization" );
    }

    //get info from db
    pkg = packages_get_package( pm, package_name, 1 );
    if( !pkg )
    {
        return -2;
    }

    install_file = packages_get_package_attr( pkg, "install" );
    version = packages_get_package_attr( pkg, "version" );

    if( cb )
    {
        cb( cb_arg, package_name, 1, 1, NULL );
    }

    if( cb )
    {
        cb( cb_arg, package_name, 5, -1, "executing pre_remove script" );
    }

    //exec pre_remove script
    if( install_file && strlen( install_file ) > 8 )
    {
        install_file_path = util_strcat( PACKAGE_DB_DIR, "/", package_name, "/", install_file, NULL );
        if( !access( install_file_path, R_OK ) )
        {
            //printf( "running pre remove script ...\n" );
            if( packages_exec_script( install_file_path, package_name, version, NULL, "pre_remove" ) == -1 )
            {
                return_code = -3; 
                goto return_point;
            }
        }
    }

    if( cb )
    {
        cb( cb_arg, package_name, 5, 1, NULL );
    }

    if( cb )
    {
        cb( cb_arg, package_name, 6, -1, "deleting files" );
    }

    //delete files
    db_init( &db, pm->db_name, OPEN_READ );
    sql_file = "select * from world_file where (type='F' or type='S') and name=?";
    db_query( &db, sql_file, package_name, NULL);
    while( db_fetch_assoc( &db ) )
    {
        file_path = db_get_value_by_key( &db, "file" );
        remove( file_path );
        /*
        printf("deleting %s ... ", file_path);
        if( !remove( file_path ) )
            printf("successed.\n" );
        else
            printf("failed.\n" );
            */
    }


    //delete directories
    sql_file = "select * from world_file where type='D' and name=?";
    db_query( &db, sql_file, package_name, NULL);
    while( db_fetch_assoc( &db ) )
    {
        file_path = db_get_value_by_key( &db, "file" );
        remove( file_path );
        /*
        printf("deleting %s ... ", file_path);
        if( !remove( file_path ) )
            printf("successed.\n" );
        else
            printf("failed.\n" );
            */

    }
    db_close( &db );

    if( cb )
    {
        cb( cb_arg, package_name, 6, 1, NULL );
    }

    if( cb )
    {
        cb( cb_arg, package_name, 7, -1, "executing post_remove script" );
    }

    //exec post_remove script
    if( install_file_path && !access( install_file_path, R_OK ) )
    {
        //printf( "running post remove script ...\n" );
        if( packages_exec_script( install_file_path, package_name, version, NULL, "post_remove" ) == -1 )
        {
            return_code = -5; 
            goto return_point;
        }
    }


    //delete /var/ypkg/db/$N
    file_path = util_strcat( PACKAGE_DB_DIR, "/", package_name, NULL );
    //printf( "deleting %s ... \n", file_path );
    util_remove_dir( file_path );
    free( file_path );

    if( cb )
    {
        cb( cb_arg, package_name, 7, 1, NULL );
    }

    if( cb )
    {
        cb( cb_arg, package_name, 8, -1, "updating database" );
    }

    //update db
    db_init( &db, pm->db_name, OPEN_WRITE );
    db_exec( &db, "begin", NULL );  
    db_exec( &db, "delete from world where name=?", package_name, NULL );  
    db_exec( &db, "delete from world_data where name=?", package_name, NULL );  
    db_exec( &db, "delete from world_file where name=?", package_name, NULL );  
    db_exec( &db, "update universe set can_update='0', installed='0' where name=?", package_name, NULL );  
    db_exec( &db, "end", NULL );  
    db_close( &db );

    if( cb )
    {
        cb( cb_arg, package_name, 8, 1, NULL );
    }

    if( cb )
    {
        cb( cb_arg, package_name, 9, 1, NULL );
    }


return_point:
    if( install_file_path )
        free( install_file_path );

    packages_free_package( pkg );
    return return_code;
}


/*
 * clean up cache directory
 */
int packages_cleanup_package( YPackageManager *pm )
{
    return util_remove_files( pm->package_dest, ".ypk" );
}


/*
 * log
 */
int packages_log( YPackageManager *pm, char *package_name, char *msg )
{
    char    *time_str, *log;
    time_t  t; 
    FILE    *log_file;

    if( !msg )
        return -1;

    log = pm->log ? pm->log : LOG_FILE;
    log_file = fopen( log, "a" );

    if( log_file )
    {

        if( ( t = time( NULL ) ) == -1 )
        {
            fclose( log_file );
            return -1;
        }

        time_str = util_time_to_str( t );
        fprintf( log_file, "%s %s : %s\n", time_str, package_name ? package_name : "", msg );

        fflush( log_file );
        fclose( log_file );
        return 0;
    }

    return -1;
}


/**
 * async interface
 */

static YPackageManager *packages_manager_clone(  YPackageManager *pm )
{
    int             len;
    YPackageManager *pm_copy;

    if( !pm )
        return NULL;

    pm_copy = malloc( sizeof( YPackageManager ) );

    if( !pm_copy )
        return NULL;

    if( pm->source_uri )
        pm_copy->source_uri = strdup( pm->source_uri );

    if( pm->accept_repo )
        pm_copy->accept_repo = strdup( pm->accept_repo );

    if( pm->package_dest )
        pm_copy->package_dest = strdup( pm->package_dest );

    if( pm->db_name )
        pm_copy->db_name = strdup( pm->db_name );

    return pm_copy;
}

static void *packages_check_update_backend_thread( void *data )
{
    int                 ret;
    AsyncQueryParams  *params;

    params = (AsyncQueryParams  *)data;

    ret = packages_check_update( params->pm );

    if( params->cb )
        params->cb( (void *)ret );

    packages_manager_cleanup( params->pm );
    free( params );
    return (void *)ret;
}

int packages_check_update_async( YPackageManager *pm, YPackageCB *cb  )
{
    int                 tret;
    pthread_t           tid;
    AsyncQueryParams  *params;

    params = malloc( sizeof( AsyncQueryParams ) );

    params->pm = packages_manager_clone( pm );
    params->cb = cb;

    tret = pthread_create( &tid, NULL, packages_check_update_backend_thread, (void *)params );

    pthread_detach(tid);

    if( !tret )
        return 0;

    return -1;
}

static void *packages_update_backend_thread( void *data )
{
    int                 ret;
    AsyncQueryParams  *params;

    params = (AsyncQueryParams  *)data;

    ret = packages_update( params->pm, NULL, NULL );

    if( params->cb )
        params->cb( (void *)ret );

    packages_manager_cleanup( params->pm );
    free( params );
    return (void *)ret;
}

int packages_update_async( YPackageManager *pm,  YPackageCB *cb )
{
    int                 tret;
    pthread_t           tid;
    AsyncQueryParams  *params;

    params = malloc( sizeof( AsyncQueryParams ) );

    params->pm = packages_manager_clone( pm );
    params->cb = cb;

    tret = pthread_create(&tid, NULL, packages_update_backend_thread, (void *)params );

    //pthread_join(tid, NULL);
    pthread_detach(tid);

    if( !tret )
        return 0;

    return -1;
}

static void *packages_get_list_backend_thread( void *data )
{
    AsyncQueryParams  *params;
    YPackageList        *pkg_list;

    params = (AsyncQueryParams  *)data;

    pkg_list = packages_get_list( params->pm, params->limit, params->offset, params->key, params->keyword, params->wildcards, params->installed );

    if( params->cb )
        params->cb( (void *)pkg_list );

    packages_manager_cleanup( params->pm );
    free( params );
    return (void *)0;
}

int packages_get_list_async( YPackageManager *pm, int limit, int offset, char *key, char *keyword, int wildcards, int installed, YPackageCB *cb )
{
    int                 tret;
    pthread_t           tid;
    AsyncQueryParams  *params;

    params = malloc( sizeof( AsyncQueryParams ) );

    params->pm = packages_manager_clone( pm );
    params->limit = limit;
    params->offset = offset;
    params->key = strdup(key);
    params->keyword = strdup(keyword);
    params->wildcards = wildcards;
    params->wildcards = installed;
    params->cb = cb;

    tret = pthread_create(&tid, NULL, packages_get_list_backend_thread, (void *)params );

    //pthread_join(tid, NULL);
    pthread_detach(tid);

    if( !tret )
        return 0;

    return -1;
}

int packages_get_list_async2( YPackageManager *pm, int page_size, int page_no, char *key, char *keyword, int wildcards, YPackageCB *cb )
{
    int                 tret, offset;
    pthread_t           tid;
    AsyncQueryParams  *params;

    params = malloc( sizeof( AsyncQueryParams ) );

    params->pm = packages_manager_clone( pm );
    params->limit = page_size;
    offset = ( page_no - 1 ) * page_size;
    params->offset = offset;
    params->key = strdup(key);
    params->keyword = strdup(keyword);
    params->wildcards = wildcards;
    params->cb = cb;

    tret = pthread_create(&tid, NULL, packages_get_list_backend_thread, (void *)params );

    //pthread_join(tid, NULL);
    pthread_detach(tid);

    if( !tret )
        return 0;

    return -1;
}
