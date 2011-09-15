#include "package.h"

#define DEBUG 1


PACKAGE_MANAGER *packages_manager_init()
{
    char *config_file;
    PACKAGE_MANAGER *pm;

    pm = malloc( sizeof(PACKAGE_MANAGER) );

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
    pm->db_name = util_strcpy( DB_NAME );

    return pm;
}

void packages_manager_cleanup( PACKAGE_MANAGER *pm )
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

    free( pm );
}

int packages_check_update( PACKAGE_MANAGER *pm )
{
    int             timestamp, last_check;
    char            *target_url, *list_date_file;
    TARGET_CONTENT  content;

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

    util_rtrim( content.text );
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

int packages_import_local_data( PACKAGE_MANAGER *pm )
{
    int                 xml_ret, db_ret, is_desktop, i, list_len;
    char                xml_value, *sql, *sql_history, *sql_data, *sql_history_data, *sql_filelist, *idx, *data_key, *file_path, *file_path_sub, *list_line;
    char                *package_name, *yversion, *install_time, *install_size, *file_type, *file_file, *file_size, *file_perms, *file_uid, *file_gid, *file_mtime, *file_extra;
    char                *xml_attrs[] = {"name", "type", "lang", "id", NULL};
    struct stat         statbuf, statbuf_sub;
    struct dirent       *entry, *entry_sub;
    DIR                 *dir, *dir_sub;
    FILE                *fp;
    XML_READER_HANDLE   xml_handle;
    DB                  db;

    //init
    db_init( &db, pm->db_name, OPEN_WRITE );
    db_exec( &db, "begin", NULL );  

    printf( "Start ...\n" );


    printf( "Import universe  ...\n" );
    //import universe
    reader_open( LOCAL_UNIVERSE,  &xml_handle );
    sql = "replace into universe (name, yversion, generic_name, is_desktop, category, arch, version, priority, install, license, homepage, repo, size, sha, build_date, uri, description, data_count) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sql_history = "replace into universe_history (name, yversion, generic_name, is_desktop, category, arch, version, priority, install, license, homepage, repo, size, sha, build_date, uri, description, data_count) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    while( ( xml_ret = reader_fetch_a_row( &xml_handle, 1, xml_attrs ) ) == 1 )
    {
        is_desktop = (int)reader_get_value( &xml_handle, "genericname|desktop|keyword|en" );
        package_name = reader_get_value2( &xml_handle, "name" );
        yversion = reader_get_value( &xml_handle, "yversion" );
        if( !yversion )
            yversion = "0";

        //universe
        db_ret = db_exec( &db, sql,  
                package_name, //name
                yversion, //yversion
                is_desktop ? reader_get_value2( &xml_handle, "genericname|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "genericname|keyword|en" ), //generic_name
                is_desktop ? "1" : "0", //desktop
                reader_get_value2( &xml_handle, "category" ), //category
                reader_get_value2( &xml_handle, "arch" ), //arch
                reader_get_value2( &xml_handle, "version" ), //version
                reader_get_value2( &xml_handle, "priority" ), //priority
                reader_get_value2( &xml_handle, "install" ), //install
                reader_get_value2( &xml_handle, "license" ), //license
                reader_get_value2( &xml_handle, "homepage" ), //homepage
                reader_get_value2( &xml_handle, "repo" ), //repo
                reader_get_value2( &xml_handle, "size" ), //size
                reader_get_value2( &xml_handle, "sha" ), //sha
                reader_get_value2( &xml_handle, "build_date" ), //build_date
                reader_get_value2( &xml_handle, "uri" ), //uri
                is_desktop ? reader_get_value2( &xml_handle, "description|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "description|keyword|en" ), //description
                reader_get_value2( &xml_handle, "data_count" ), //data_count
                NULL);

        //universe_history
        db_ret = db_exec( &db, sql_history,  
                package_name, //name
                yversion, //yversion
                is_desktop ? reader_get_value2( &xml_handle, "genericname|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "genericname|keyword|en" ), //generic_name
                is_desktop ? "1" : "0", //desktop
                reader_get_value2( &xml_handle, "category" ), //category
                reader_get_value2( &xml_handle, "arch" ), //arch
                reader_get_value2( &xml_handle, "version" ), //version
                reader_get_value2( &xml_handle, "priority" ), //priority
                reader_get_value2( &xml_handle, "install" ), //install
                reader_get_value2( &xml_handle, "license" ), //license
                reader_get_value2( &xml_handle, "homepage" ), //homepage
                reader_get_value2( &xml_handle, "repo" ), //repo
                reader_get_value2( &xml_handle, "size" ), //size
                reader_get_value2( &xml_handle, "sha" ), //sha
                reader_get_value2( &xml_handle, "build_date" ), //build_date
                reader_get_value2( &xml_handle, "uri" ), //uri
                is_desktop ? reader_get_value2( &xml_handle, "description|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "description|keyword|en" ), //description
                reader_get_value2( &xml_handle, "data_count" ), //data_count
                NULL);
    
        //universe_data
        db_exec( &db, "delete from universe_data where name=?", package_name, yversion, NULL );  

        db_exec( &db, "delete from universe_history_data where name=? and yversion=?", package_name, yversion, NULL );  

        sql_data = "insert into universe_data (name, yversion, data_name, data_format, data_size, data_install_size, data_depend, data_bdepend, data_recommended, data_conflict) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

        sql_history_data = "insert into universe_history_data (name, yversion, data_name, data_format, data_size, data_install_size, data_depend, data_bdepend, data_recommended, data_conflict) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
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
                    yversion, //yversion
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
                    yversion, //yversion
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
    printf( "Import world  ...\n" );
    reader_open( LOCAL_WORLD,  &xml_handle );
    sql = "replace into world (name, yversion, generic_name, is_desktop, category, arch, version, priority, install, license, homepage, repo, size, sha, build_date, uri, description, data_count) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    while( ( xml_ret = reader_fetch_a_row( &xml_handle, 1, xml_attrs ) ) == 1 )
    {
        is_desktop = (int)reader_get_value( &xml_handle, "genericname|desktop|keyword|en" );
        package_name = reader_get_value2( &xml_handle, "name" );
        yversion = reader_get_value( &xml_handle, "yversion" );
        if( !yversion )
            yversion = "0";

        //world
        db_ret = db_exec( &db, sql,  
                package_name, //name
                yversion, //yversion
                is_desktop ? reader_get_value2( &xml_handle, "genericname|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "genericname|keyword|en" ), //generic_name
                is_desktop ? "1" : "0", //desktop
                reader_get_value2( &xml_handle, "category" ), //category
                reader_get_value2( &xml_handle, "arch" ), //arch
                reader_get_value2( &xml_handle, "version" ), //version
                reader_get_value2( &xml_handle, "priority" ), //priority
                reader_get_value2( &xml_handle, "install" ), //install
                reader_get_value2( &xml_handle, "license" ), //license
                reader_get_value2( &xml_handle, "homepage" ), //homepage
                reader_get_value2( &xml_handle, "repo" ), //repo
                reader_get_value2( &xml_handle, "size" ), //size
                reader_get_value2( &xml_handle, "sha" ), //sha
                reader_get_value2( &xml_handle, "build_date" ), //build_date
                reader_get_value2( &xml_handle, "uri" ), //uri
                is_desktop ? reader_get_value2( &xml_handle, "description|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "description|keyword|en" ), //description
                reader_get_value2( &xml_handle, "data_count" ), //data_count
                NULL);
    
        //world_data
        db_exec( &db, "delete from world_data where name=?", package_name, NULL );  

        sql_data = "insert into world_data (name, yversion, data_name, data_format, data_size, data_install_size, data_depend, data_bdepend, data_recommended, data_conflict) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
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
                    yversion, //yversion
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

    //world_file
    printf( "Import file list  ...\n" );
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

    db_exec( &db, "end", NULL );  
    printf( "Done!\n" );
    //clean up
    db_close( &db );
    return 0;
}

int packages_update( PACKAGE_MANAGER *pm )
{
    int             timestamp, last_update, cnt;
    char            *target_url, *list_file, *list_line, xml_file[32],  sum[48];
    char            *sql;
    int             time_update;
    DB              db;
    TARGET_CONTENT  content;

    if( !pm )
        return -1;

    //download updates list
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

    list_line = util_mem_gets( content.text );
    last_update = packages_get_last_update_timestamp( pm );
    cnt = 0;
    while( list_line )
    {
        util_rtrim( list_line );
        memset( xml_file, '\0', 32 );
        memset( sum, '\0', 48 );
        if( sscanf( list_line, "%s %d %s", xml_file, &timestamp, sum ) == 3 )
        {
            if( timestamp > last_update )
            {
                printf("xml:%s time:%d sum:%s\n",  xml_file, timestamp, sum);
                packages_update_single_xml( pm, xml_file, sum );
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

    return cnt;
}

static int packages_update_single_xml( PACKAGE_MANAGER *pm, char *xml_file, char *sum )
{
    int                 xml_ret, db_ret, is_desktop, i;
    char                *target_url, xml_value, *sql, *sql_data, *sql_history, *sql_history_data, *package_name, *yversion, *idx, *data_key;
    char                tmp_bz2[] = "/tmp/tmp_bz2.XXXXXX";
    char                tmp_xml[] = "/tmp/tmp_xml.XXXXXX";
    char                *xml_attrs[] = {"name", "type", "lang", "id", NULL};
    TARGET_FILE         file;
    XML_READER_HANDLE   xml_handle;
    DB                  db;

    if( !pm || !xml_file || !sum )
        return -1;

    //donload xml
    target_url = util_strcat( pm->source_uri, "/", UPDATE_DIR, "/", xml_file, NULL );
    mkstemp(tmp_bz2);
    file.file = tmp_bz2;
    file.stream = NULL;
    if( download_file( target_url, &file ) != 0 )
    {
        remove(tmp_bz2);
        return -1; 
    }
    fclose(file.stream);
    free(target_url);
    target_url = NULL;

    //compare sum
    /*
    db_init(&db, pm->db_name);
    db_query(&db, "select sha1sum, time_update from config", NULL);
    if(db_fetch_row(&db) == 0)
    {
        return 1;
    }
    memset(sha1sum_old, '\0', 64);
    strncpy(sha1sum_old, db.crow[0], strlen(db.crow[0]));
    time_update = atoi(db.crow[1]);

    if( strncmp( sha1sum_new, sha1sum_old, strlen(sha1sum_old) ) == 0 )
    {
        return 1;
    }
    */

    //unzip
    mkstemp(tmp_xml);
    if( archive_extract_file( file.file, "update.xml", tmp_xml ) == -1 )
    {
        remove(tmp_bz2);
        remove(tmp_xml);
        return -1;
    }


    //parse xml
    reader_open( tmp_xml,  &xml_handle );
    db_init( &db, pm->db_name, OPEN_WRITE );

    sql = "replace into universe (name, yversion, generic_name, is_desktop, category, arch, version, priority, install, license, homepage, repo, size, sha, build_date, uri, description, data_count) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sql_history = "replace into universe_history (name, yversion, generic_name, is_desktop, category, arch, version, priority, install, license, homepage, repo, size, sha, build_date, uri, description, data_count) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sql_data = "insert into universe_data (name, yversion, data_name, data_format, data_size, data_install_size, data_depend, data_bdepend, data_recommended, data_conflict) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sql_history_data = "insert into universe_history_data (name, yversion, data_name, data_format, data_size, data_install_size, data_depend, data_bdepend, data_recommended, data_conflict) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    while( ( xml_ret = reader_fetch_a_row( &xml_handle, 1, xml_attrs ) ) == 1 )
    {
        is_desktop = (int)reader_get_value( &xml_handle, "genericname|desktop|keyword|en" );
        package_name = reader_get_value2( &xml_handle, "name" );
        yversion = reader_get_value( &xml_handle, "yversion" );
        if( !yversion )
            yversion = "0";

        //universe
        db_ret = db_exec( &db, sql,  
                package_name, //name
                yversion, //yversion
                is_desktop ? reader_get_value2( &xml_handle, "genericname|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "genericname|keyword|en" ), //generic_name
                is_desktop ? "1" : "0", //desktop
                reader_get_value2( &xml_handle, "category" ), //category
                reader_get_value2( &xml_handle, "arch" ), //arch
                reader_get_value2( &xml_handle, "version" ), //version
                reader_get_value2( &xml_handle, "priority" ), //priority
                reader_get_value2( &xml_handle, "install" ), //install
                reader_get_value2( &xml_handle, "license" ), //license
                reader_get_value2( &xml_handle, "homepage" ), //homepage
                reader_get_value2( &xml_handle, "repo" ), //repo
                reader_get_value2( &xml_handle, "size" ), //size
                reader_get_value2( &xml_handle, "sha" ), //sha
                reader_get_value2( &xml_handle, "build_date" ), //build_date
                reader_get_value2( &xml_handle, "uri" ), //uri
                is_desktop ? reader_get_value2( &xml_handle, "description|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "description|keyword|en" ), //description
                reader_get_value2( &xml_handle, "data_count" ), //data_count
                NULL);

        db_ret = db_exec( &db, sql_history,  
                package_name, //name
                yversion, //yversion
                is_desktop ? reader_get_value2( &xml_handle, "genericname|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "genericname|keyword|en" ), //generic_name
                is_desktop ? "1" : "0", //desktop
                reader_get_value2( &xml_handle, "category" ), //category
                reader_get_value2( &xml_handle, "arch" ), //arch
                reader_get_value2( &xml_handle, "version" ), //version
                reader_get_value2( &xml_handle, "priority" ), //priority
                reader_get_value2( &xml_handle, "install" ), //install
                reader_get_value2( &xml_handle, "license" ), //license
                reader_get_value2( &xml_handle, "homepage" ), //homepage
                reader_get_value2( &xml_handle, "repo" ), //repo
                reader_get_value2( &xml_handle, "size" ), //size
                reader_get_value2( &xml_handle, "sha" ), //sha
                reader_get_value2( &xml_handle, "build_date" ), //build_date
                reader_get_value2( &xml_handle, "uri" ), //uri
                is_desktop ? reader_get_value2( &xml_handle, "description|desktop|keyword|en" ) : reader_get_value2( &xml_handle, "description|keyword|en" ), //description
                reader_get_value2( &xml_handle, "data_count" ), //data_count
                NULL);
    
        //universe_data
        db_exec( &db, "delete from universe_data where name=?", package_name, yversion, NULL );  

        db_exec( &db, "delete from universe_history_data where name=? and yversion=?", package_name, yversion, NULL );  

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
                    yversion, //yversion
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
                    yversion, //yversion
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

    //clean up
    db_close( &db );
    reader_cleanup( &xml_handle );
    remove(tmp_bz2);
    remove(tmp_xml);
}

static int packages_get_last_check_timestamp( PACKAGE_MANAGER *pm )
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


static int packages_set_last_check_timestamp( PACKAGE_MANAGER *pm, int last_check )
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

static int packages_get_last_update_timestamp( PACKAGE_MANAGER *pm )
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

static int packages_set_last_update_timestamp( PACKAGE_MANAGER *pm, int last_update )
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

int packages_get_count( PACKAGE_MANAGER *pm, char *key, char *keyword, int wildcards, int installed  )
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

int packages_has_installed( PACKAGE_MANAGER *pm, char *name )
{
    DB      db;
    int     count;

    if( !pm || !name )
        return -1;

    db_init( &db, pm->db_name, OPEN_READ );
    db_query( &db, "select count(*) from world where name=?", name, NULL);

    db_fetch_num( &db );
    count = atoi( db_get_value_by_index( &db, 0 ) );
    db_close( &db );

    return count > 0;
}

PACKAGE *packages_get_package( PACKAGE_MANAGER *pm, char *name, int installed )
{
    char                    *sql, *cur_key, *cur_value, **attr_keys_offset;
    char                    *attr_keys[] = { "name", "generic_name", "category", "priority", "version", "license", "description", "uri", "size", "install", "data_count", NULL  }; 
    DB                      db;
    PACKAGE                 *pkg = NULL;

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
        pkg = (PACKAGE *)malloc( sizeof( PACKAGE ) );
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
int packages_get_package_from_ypk( char *ypk_path, PACKAGE **package, PACKAGE_DATA **package_data )
{
    int                 i, ret, return_code = 0, cur_data_index, data_count;
    void                *pkginfo = NULL, *control = NULL;
    size_t              pkginfo_len = 0, control_len = 0;
    char                *cur_key, *cur_value, *cur_xpath, **attr_keys_offset, **attr_xpath_offset, *idx, *data_key;
    char                *attr_keys[] = { "name", "yversion", "generic_name", "category", "arch", "priority", "version", "install", "license", "homepage", "repo", "description", "sha", "size", "build_date", "uri", "data_count", "is_desktop", NULL  }; 
    char                *attr_xpath[] = { "//Package/@name", "//yversion", "//genericname/keyword", "//category", "//arch", "//priority", "//version", "//install", "//license", "//homepage", "//repo", "//description/keyword", "//sha", "//size", "//build_date", "//uri", "//data_count", "//genericname[@type='desktop']", NULL  }; 
    char                *data_attr_keys[] = { "data_name", "data_format", "data_size", "data_install_size", "data_depend", "data_bdepend", "data_recommended", "data_conflict", NULL  }; 
    char                *data_attr_xpath[] = { "name", "format", "size", "install_size", "depend", "bdepend", "recommended", "conflict", NULL  }; 
    xmlDocPtr           xmldoc = NULL;
    xmlXPathObjectPtr   xpath;
    PACKAGE             *pkg;
    PACKAGE_DATA        *pkg_data;


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

    if( pkginfo )
    {
        free( pkginfo );
        pkginfo = NULL;
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
        pkg = (PACKAGE *)malloc( sizeof( PACKAGE ) );
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

        pkg_data = (PACKAGE_DATA *)malloc( sizeof( PACKAGE_DATA ) );
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
char *packages_get_package_attr( PACKAGE *pkg, char *key )
{
    if( !pkg || !key )
        return NULL;

    return hash_table_get_data( pkg->ht, key );
}

char *packages_get_package_attr2( PACKAGE *pkg, char *key )
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
void packages_free_package( PACKAGE *pkg )
{
    if( !pkg )
        return;

    hash_table_cleanup( pkg->ht );
    free( pkg );
}

/*
 * packages_get_package_data
 */
PACKAGE_DATA *packages_get_package_data( PACKAGE_MANAGER *pm, char *name, int installed )
{
    int                     data_count, cur_data_index;
    char                    *sql, *cur_key, *cur_value, **attr_keys_offset;
    char                    *attr_keys[] = { "name", "data_name", "data_format", "data_size", "data_install_size", "data_depend", "data_bdepend", "data_recommended", "data_conflict", NULL  }; 
    DB                      db;
    PACKAGE_DATA            *pkg_data = NULL;

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

    //get data
    sql = util_strcat( "select * from ", installed ? "world_data" : "universe_data"," where name=?", NULL );
    db_query( &db, sql, name, NULL);
    free( sql );

    pkg_data = (PACKAGE_DATA *)malloc( sizeof( PACKAGE_DATA ) );
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
char *packages_get_package_data_attr( PACKAGE_DATA *pkg_data, int index, char *key )
{
    return hash_table_list_get_data( pkg_data->htl, index, key );
}

char *packages_get_package_data_attr2( PACKAGE_DATA *pkg_data, int index, char *key )
{
    char *result;

    result = hash_table_list_get_data( pkg_data->htl, index, key );
    return result ? result : "";
}
/*
 * packages_free_package_data
 */
void packages_free_package_data( PACKAGE_DATA *pkg_data )
{
    if( !pkg_data )
        return;

    hash_table_list_cleanup( pkg_data->htl );
    free( pkg_data );
}

/*
 * packages_get_package_file
 */
PACKAGE_FILE *packages_get_package_file( PACKAGE_MANAGER *pm, char *name )
{
    int                     file_count, file_type, buf_size, cur_len, cur_pos, cur_file_index;
    char                    *sql, *cur_key, *cur_value, **attr_keys_offset;
    char                    *attr_keys[] = { "name", "file", "type", "size", "perms", "uid", "gid", "mtime", NULL  }; 
    DB                      db;
    PACKAGE_FILE            *pkg_file = NULL;
    HASH_TABLE              *cur_file;
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

    //get file info
    db_query( &db, "select * from world_file where name=?", name, NULL);

    pkg_file = (PACKAGE_FILE *)malloc( sizeof( PACKAGE_FILE ) );
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
PACKAGE_FILE *packages_get_package_file_from_ypk( char *ypk_path )
{
    int                 ret, file_count, cur_file_index;
    size_t              pkginfo_len = 0, filelist_len = 0;
    void                *pkginfo = NULL, *filelist = NULL;
    char                *package_name, *yversion, *file_type, *file_file, *file_size, *file_perms, *file_uid, *file_gid, *file_mtime, *file_extra;
    char                *list_line, *cur_key, *cur_value, **attr_keys_offset;
    char                *attr_keys[] = { "type", "file", "size", "perms", "uid", "gid", "mtime", "extra", NULL  }; 
    PACKAGE_FILE        *pkg_file = NULL;

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
    pkg_file = (PACKAGE_FILE *)malloc( sizeof( PACKAGE_FILE ) );
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
char *packages_get_package_file_attr( PACKAGE_FILE *pkg_file, int index, char *key )
{
    return hash_table_list_get_data( pkg_file->htl, index, key );
}

char *packages_get_package_file_attr2( PACKAGE_FILE *pkg_file, int index, char *key )
{
    char *result;

    result = hash_table_list_get_data( pkg_file->htl, index, key );
    return result ? result : "";
}

/*
 * packages_free_package_file
 */
void packages_free_package_file( PACKAGE_FILE *pkg_file )
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
PACKAGE_LIST *packages_get_list( PACKAGE_MANAGER *pm, int limit, int offset, char *key, char *keyword, int wildcards, int installed )
{
    int                     ret, cur_pkg_index;
    char                    *table,*sql, *offset_str, *limit_str, *cur_key, *cur_value, **attr_keys_offset;
    char                    *attr_keys[] = { "name", "generic_name", "category", "priority", "version", "license", "description", "size", "install_time", NULL  }; 
    DB                      db;
    PACKAGE_LIST            *pkg_list;


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

    pkg_list = (PACKAGE_LIST *)malloc( sizeof( PACKAGE_LIST ) );
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
PACKAGE_LIST *packages_get_list2( PACKAGE_MANAGER *pm, int page_size, int page_no, char *key, char *keyword, int wildcards, int installed )
{
    int                     offset;
    PACKAGE_LIST            *pkg_list;


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
PACKAGE_LIST *packages_get_list_with_data( PACKAGE_MANAGER *pm, int limit, int offset, char *key, char *keyword, int installed )
{    
    int                     ret, cur_pkg_index;
    char                    *table, *table_data, *sql, *offset_str, *limit_str, *cur_key, *cur_value, **attr_keys_offset;
    char                    *attr_keys[] = { "name", "generic_name", "category", "priority", "version", "license", "description", "size", NULL  }; 
    DB                      db;
    PACKAGE_LIST            *pkg_list;

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

    pkg_list = (PACKAGE_LIST *)malloc( sizeof( PACKAGE_LIST ) );
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
PACKAGE_LIST *packages_get_list_by_recommended( PACKAGE_MANAGER *pm, int limit, int offset, char *recommended, int installed )
{
    return packages_get_list_with_data( pm, limit, offset, "data_recommended", recommended, installed );
}

/*
 * packages_get_list_by_conflict
 */
PACKAGE_LIST *packages_get_list_by_conflict( PACKAGE_MANAGER *pm, int limit, int offset, char *conflict, int installed )
{
    return packages_get_list_with_data( pm, limit, offset, "data_conflict", conflict,installed );
}

/*
 * packages_get_list_by_depend
 */
PACKAGE_LIST *packages_get_list_by_depend( PACKAGE_MANAGER *pm, int limit, int offset, char *depend, int installed )
{
    return packages_get_list_with_data( pm, limit, offset, "data_depend", depend,installed );
}

/*
 * packages_get_list_by_bdepend
 */
PACKAGE_LIST *packages_get_list_by_bdepend( PACKAGE_MANAGER *pm, int limit, int offset, char *bdepend, int installed )
{
    return packages_get_list_with_data( pm, limit, offset, "data_bdepend", bdepend,installed );
}

/*
 * packages_get_list_by_file
 */
PACKAGE_LIST *packages_get_list_by_file( PACKAGE_MANAGER *pm, int limit, int offset, char *file )
{    
    int                     ret, cur_pkg_index;
    char                    *sql, *offset_str, *limit_str, *cur_key, *cur_value, **attr_keys_offset;
    char                    *attr_keys[] = { "name", "generic_name", "category", "priority", "version", "license", "description", "size", "file", "type", "extra", NULL  }; 
    DB                      db;
    PACKAGE_LIST            *pkg_list;

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

    pkg_list = (PACKAGE_LIST *)malloc( sizeof( PACKAGE_LIST ) );
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
void packages_free_list( PACKAGE_LIST *pkg_list )
{
    if( !pkg_list )
        return;

    hash_table_list_cleanup( pkg_list->htl );
    free( pkg_list );
}

/*
 * packages_get_list_attr
 */
char *packages_get_list_attr( PACKAGE_LIST *pkg_list, int index, char *key )
{
    return hash_table_list_get_data( pkg_list->htl, index, key );
}

char *packages_get_list_attr2( PACKAGE_LIST *pkg_list, int index, char *key )
{
    char *result;

    result = hash_table_list_get_data( pkg_list->htl, index, key );
    return result ? result : "";
}

/*
 * packages_install_package
 */
int packages_install_package( PACKAGE_MANAGER *pm, char *package_name )
{
    int                 return_code = 0;
    char                *target_url = NULL, *package_url = NULL, *package_path = NULL;
    TARGET_FILE         file;
    PACKAGE             *pkg;

    if( !package_name )
        return -1;

    //get info from db
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
    if( !access( package_path, R_OK ) )
    {
        //goto return_point;
    }

    target_url = util_strcat( pm->source_uri, "/", package_url, NULL );

    file.file = package_path;
    file.stream = NULL;
    printf( "downloading %s to %s\n", target_url, package_path );
    if( download_file( target_url, &file ) != 0 )
    {
        fclose( file.stream );
        return_code = -4;
        goto return_point;
    }
    fclose( file.stream );

    printf( "installing %s\n", package_path );
    if( return_code = packages_install_local_package( pm, package_path, "/" ) )
    {
        printf("packages_install_local_package() return code: %d\n", return_code );
        return_code = -5;
    }

return_point:

    if( target_url )
        free( target_url );
    if( package_path )
        free( package_path );

    packages_free_package( pkg );
    return return_code;
}

/*
 * packages_check_package
 */
int packages_check_package( PACKAGE_MANAGER *pm, char *ypk_path )
{
    int                 i, return_code = 0;
    char                *depend, *conflict, *token, *package_name;
    PACKAGE             *pkg = NULL;
    PACKAGE_DATA        *pkg_data = NULL;

    if( !ypk_path || access( ypk_path, R_OK ) )
        return -1;

    //get ypk's infomations
    if(packages_get_package_from_ypk( ypk_path, &pkg, &pkg_data ) < 0 )
    {
        return -1;
    }

    package_name = packages_get_package_attr( pkg, "name" );

    //check installed
    if( packages_has_installed( pm, package_name ) )
    {
        return_code = -2;
        goto return_point;
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
                if( !packages_has_installed( pm, packages_get_package_data_attr( pkg_data, i, token ) ) )
                {
                    return_code = -3; 
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
                if( packages_has_installed( pm, packages_get_package_data_attr( pkg_data, i, token ) ) )
                {
                    return_code = -4; 
                    goto return_point;
                }
                token = strtok( NULL, " ,");
            }
        }
    }

return_point:
    if( pkg )
        packages_free_package( pkg );

    if( pkg_data )
        packages_free_package_data( pkg_data );

    return return_code;
}

/*
 * packages_unpack_package
 */
int packages_unpack_package( PACKAGE_MANAGER *pm, char *ypk_path, char *dest_dir )
{
    char                tmp_ypk_data[] = "/tmp/ypkdata.XXXXXX";

    if( !ypk_path )
        return -1;

    if( !dest_dir )
        dest_dir = "./";

    //unzip data
    mkstemp( tmp_ypk_data );
    if( archive_extract_file( ypk_path, "pkgdata", tmp_ypk_data ) == -1 )
    {
        return -2;
    }

    //copy files 
    printf( "unpacking files ...\n");
    if( archive_extract_all( tmp_ypk_data, dest_dir ) == -1 )
    {
        remove( tmp_ypk_data );
        return -3;
    }
    
    remove( tmp_ypk_data );
    return 0;
}

/*
 * packages_pack_package
 */
int packages_pack_package( PACKAGE_MANAGER *pm, char *source_dir, char *ypk_path )
{
}

/*
 * packages_install_local_package
 */
int packages_install_local_package( PACKAGE_MANAGER *pm, char *ypk_path, char *dest_dir )
{
    int                 i, ret, status, return_code = 0;
    size_t              pkginfo_len, filelist_len;
    void                *pkginfo = NULL, *filelist = NULL;
    char                *sql, *sql_data, *sql_filelist, *install_file, *cmd, *list_line;
    char                *package_name, *yversion, *file_type, *file_file, *file_size, *file_perms, *file_uid, *file_gid, *file_mtime, *file_extra;
    char                tmp_ypk_install[] = "/tmp/ypkinstall.XXXXXX";
    PACKAGE             *pkg = NULL;
    PACKAGE_DATA        *pkg_data = NULL;
    DB                  db;

    //check
    if( !ypk_path )
        return -1;

    if( packages_check_package( pm, ypk_path ) < 0 )
        return -1;

    if( !dest_dir )
        dest_dir = "/";

    packages_get_package_from_ypk( ypk_path, &pkg, &pkg_data );
    if( !pkg || !pkg_data )
    {
        return_code = -1;
        goto return_point;
    }
    package_name = packages_get_package_attr( pkg, "name" );
    yversion = packages_get_package_attr( pkg, "yversion" );
    if( !yversion )
        yversion = "0";

    //pre install
    mkstemp( tmp_ypk_install );
    ret = archive_extract_file2( ypk_path, "pkginfo", &pkginfo, &pkginfo_len );
    if( ret == -1 )
    {
        return_code = -2;
        goto return_point;
    }

    install_file = util_strcat( package_name, ".install", NULL );
    ret = archive_extract_file3( pkginfo, pkginfo_len, install_file, tmp_ypk_install );
    free( install_file );

    if( ret != -1 )
    {
        printf( "running pre_install script ...\n" );
        cmd = util_strcat( "source '", tmp_ypk_install, "'; if type pre_install >/dev/null 2>1; then pre_install; fi", NULL );
        status = system( cmd );
        free( cmd );
        if( WEXITSTATUS( status ) )
        {
            return_code = -3; 
            //goto return_point;
        }
    }

    //copy files 
    printf( "copying files ...\n");
    if( packages_unpack_package( pm, ypk_path, dest_dir ) != 0 )
    {
        return_code = -4; 
        goto return_point;
    }

    //post install
    if( !access( tmp_ypk_install, R_OK ) )
    {
        printf( "running post_install script ...\n" );
        cmd = util_strcat( "source '", tmp_ypk_install, "'; if type post_install >/dev/null 2>1; then post_install; fi", NULL );
        status = system( cmd );
        free( cmd );
        if( WEXITSTATUS( status ) )
        {
            return_code = -5; 
            //goto return_point;
        }
    }

    //update db
    printf( "updating database ...\n");
    db_init( &db, pm->db_name, OPEN_WRITE );
    //world
    sql = "replace into world (name, yversion, generic_name, is_desktop, category, arch, version, priority, install, license, homepage, repo, size, sha, build_date, uri, description, data_count, install_time) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, strftime('%s','now'));";


    ret = db_exec( &db, sql,  
            package_name, //name
            yversion, //yversion
            packages_get_package_attr2( pkg, "generic_name"), //generic_name
            packages_get_package_attr2( pkg, "is_desktop"), //desktop
            packages_get_package_attr2( pkg, "category" ), //category
            packages_get_package_attr2( pkg, "arch" ), //arch
            packages_get_package_attr2( pkg, "version" ), //version
            packages_get_package_attr2( pkg, "priority" ), //priority
            packages_get_package_attr2( pkg, "install" ), //install
            packages_get_package_attr2( pkg, "license" ), //license
            packages_get_package_attr2( pkg, "homepage" ), //homepage
            packages_get_package_attr2( pkg, "repo" ), //repo
            packages_get_package_attr2( pkg, "size" ), //size
            packages_get_package_attr2( pkg, "sha" ), //repo
            packages_get_package_attr2( pkg, "build_date" ), //build_date
            packages_get_package_attr2( pkg, "uri" ), //uri
            packages_get_package_attr2( pkg, "description" ), //description
            packages_get_package_attr2( pkg, "data_count" ), //data_count
            NULL);
    
    //world_data
    db_exec( &db, "delete from world_data where name=?", package_name, NULL );  
    sql_data = "insert into world_data (name, yversion, data_name, data_format, data_size, data_install_size, data_depend, data_bdepend, data_recommended, data_conflict) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    for( i = 0; i < pkg_data->cnt; i++ )
    {
        ret = db_exec( &db, sql_data,  
                package_name, //name
                yversion, //yversion
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

    //file list
    ret = archive_extract_file4( pkginfo, pkginfo_len, "filelist", &filelist, &filelist_len );
    if( ret == -1 )
    {
        return_code = -7; 
        goto return_point;
    }

    db_exec( &db, "delete from world_file where name=?", package_name, NULL );  
    sql_filelist = "insert into world_file (name, yversion, type, file, size, perms, uid, gid, mtime) values (?, ?, ?, ?, ?, ?, ?, ?, ?)"; 

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


            ret = db_exec( &db, sql_filelist, 
                    package_name,
                    yversion,
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

    db_close( &db );

return_point:
    if( pkginfo )
        free( pkginfo );
    if( filelist )
        free( filelist );
    if( pkg )
        packages_free_package( pkg );
    if( pkg_data )
        packages_free_package_data( pkg_data );
    remove( tmp_ypk_install );
    return return_code;
}

int packages_remove_package( PACKAGE_MANAGER *pm, char *package_name )
{
    int             status, ret, return_code = 0;
    char            *install_file, *install_file_path = NULL, *sql_file, *cmd, *file_path;
    PACKAGE         *pkg;
    DB              db;

    if( !package_name )
        return -1;

    //get info from db
    pkg = packages_get_package( pm, package_name, 1 );
    if( !pkg )
    {
        return -2;
    }

    install_file = packages_get_package_attr( pkg, "install" );
    //pre remove
    if( install_file && strlen( install_file ) > 8 )
    {
        install_file_path = util_strcat( PACKAGE_DB_DIR, "/", package_name, "/", install_file, NULL );
        if( !access( install_file_path, R_OK ) )
        {
            printf( "running pre remove script ...\n" );
            cmd = util_strcat( "source '", install_file_path, "'; if type pre_remove >/dev/null 2>1; then pre_remove; fi", NULL );
            status = system( cmd );
            free( cmd );
            if( WEXITSTATUS( status ) )
            {
                return_code = -3; 
                //goto return_point;
            }
        }
    }
    //remove
    db_init( &db, pm->db_name, OPEN_READ );
    sql_file = "select * from world_file where (type='F' or type='S') and name=?";
    db_query( &db, sql_file, package_name, NULL);
    while( db_fetch_assoc( &db ) )
    {
        file_path = db_get_value_by_key( &db, "file" );
        printf("deleting %s ... ", file_path);
        if( !remove( file_path ) )
            printf("successed.\n" );
        else
            printf("failed.\n" );
    }

    sql_file = "select * from world_file where type='D' and name=?";
    db_query( &db, sql_file, package_name, NULL);
    while( db_fetch_assoc( &db ) )
    {
        file_path = db_get_value_by_key( &db, "file" );
        printf("deleting %s ... ", file_path);
        if( !remove( file_path ) )
            printf("successed.\n" );
        else
            printf("failed.\n" );

    }
    db_close( &db );

    //post remove
    if( install_file_path && !access( install_file_path, R_OK ) )
    {
        printf( "running post remove script ...\n" );
        cmd = util_strcat( "source '", install_file_path, "'; if type post_remove >/dev/null 2>1; then post_remove; fi", NULL );
        status = system( cmd );
        free( cmd );
        if( WEXITSTATUS( status ) )
        {
            return_code = -5; 
            //goto return_point;
        }
    }

    //delete /var/ypkg/db/$N
    file_path = util_strcat( PACKAGE_DB_DIR, "/", package_name, NULL );
    printf( "deleting %s ... \n", file_path );
    util_remove_dir( file_path );
    free( file_path );

    //update db
    db_init( &db, pm->db_name, OPEN_WRITE );
    db_exec( &db, "delete from world where name=?", package_name, NULL );  
    db_exec( &db, "delete from world_data where name=?", package_name, NULL );  
    db_exec( &db, "delete from world_file where name=?", package_name, NULL );  
    db_close( &db );



return_point:
    if( install_file_path )
        free( install_file_path );
    packages_free_package( pkg );
    return return_code;
}


/**
 * async interface
 */

static PACKAGE_MANAGER *packages_manager_clone(  PACKAGE_MANAGER *pm )
{
    int             len;
    PACKAGE_MANAGER *pm_copy;

    if( !pm )
        return NULL;

    pm_copy = malloc( sizeof( PACKAGE_MANAGER ) );

    if( !pm_copy )
        return NULL;

    if( pm->source_uri )
        pm_copy->source_uri = util_strcpy( pm->source_uri );

    if( pm->accept_repo )
        pm_copy->accept_repo = util_strcpy( pm->accept_repo );

    if( pm->package_dest )
        pm_copy->package_dest = util_strcpy( pm->package_dest );

    if( pm->db_name )
        pm_copy->db_name = util_strcpy( pm->db_name );

    return pm_copy;
}

static void *packages_check_update_backend_thread( void *data )
{
    int                 ret;
    ASYNC_QUERY_PARAMS  *params;

    params = (ASYNC_QUERY_PARAMS  *)data;

    ret = packages_check_update( params->pm );

    if( params->cb )
        params->cb( (void *)ret );

    packages_manager_cleanup( params->pm );
    free( params );
    return (void *)ret;
}

int packages_check_update_async( PACKAGE_MANAGER *pm, PACKAGE_CB *cb  )
{
    int                 tret;
    pthread_t           tid;
    ASYNC_QUERY_PARAMS  *params;

    params = malloc( sizeof( ASYNC_QUERY_PARAMS ) );

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
    ASYNC_QUERY_PARAMS  *params;

    params = (ASYNC_QUERY_PARAMS  *)data;

    ret = packages_update( params->pm );

    if( params->cb )
        params->cb( (void *)ret );

    packages_manager_cleanup( params->pm );
    free( params );
    return (void *)ret;
}

int packages_update_async( PACKAGE_MANAGER *pm,  PACKAGE_CB *cb )
{
    int                 tret;
    pthread_t           tid;
    ASYNC_QUERY_PARAMS  *params;

    params = malloc( sizeof( ASYNC_QUERY_PARAMS ) );

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
    ASYNC_QUERY_PARAMS  *params;
    PACKAGE_LIST        *pkg_list;

    params = (ASYNC_QUERY_PARAMS  *)data;

    pkg_list = packages_get_list( params->pm, params->limit, params->offset, params->key, params->keyword, params->wildcards, params->installed );

    if( params->cb )
        params->cb( (void *)pkg_list );

    packages_manager_cleanup( params->pm );
    free( params );
    return (void *)0;
}

int packages_get_list_async( PACKAGE_MANAGER *pm, int limit, int offset, char *key, char *keyword, int wildcards, int installed, PACKAGE_CB *cb )
{
    int                 tret;
    pthread_t           tid;
    ASYNC_QUERY_PARAMS  *params;

    params = malloc( sizeof( ASYNC_QUERY_PARAMS ) );

    params->pm = packages_manager_clone( pm );
    params->limit = limit;
    params->offset = offset;
    params->key = util_strcpy(key);
    params->keyword = util_strcpy(keyword);
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

int packages_get_list_async2( PACKAGE_MANAGER *pm, int page_size, int page_no, char *key, char *keyword, int wildcards, PACKAGE_CB *cb )
{
    int                 tret, offset;
    pthread_t           tid;
    ASYNC_QUERY_PARAMS  *params;

    params = malloc( sizeof( ASYNC_QUERY_PARAMS ) );

    params->pm = packages_manager_clone( pm );
    params->limit = page_size;
    offset = ( page_no - 1 ) * page_size;
    params->offset = offset;
    params->key = util_strcpy(key);
    params->keyword = util_strcpy(keyword);
    params->wildcards = wildcards;
    params->cb = cb;

    tret = pthread_create(&tid, NULL, packages_get_list_backend_thread, (void *)params );

    //pthread_join(tid, NULL);
    pthread_detach(tid);

    if( !tret )
        return 0;

    return -1;
}












