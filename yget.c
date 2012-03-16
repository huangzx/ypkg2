/* yget2
 *
 * Copyright (c) 2011-2012 Ylmf OS
 *
 * Written by: 0o0<0o0zzyz@gmail.com> ChenYu_Xiao<yunsn0303@gmail.com>
 * Version: 0.1
 * Date: 2012.2.20
 */
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ypackage.h"
#include "util.h"

#define COLOR_RED "\e[31;49m"
#define COLOR_GREEN "\e[32;49m"
#define COLOR_YELLO "\e[33;49m"
#define COLOR_BLUE "\e[34;49m"
#define COLOR_MAGENTA "\e[35;49m"
#define COLOR_CYAN "\e[36;49m"
#define COLOR_WHILE "\033[1m"
#define COLOR_RESET "\e[0m"

typedef struct {
    int             progress;
    double          cnt;
    struct timeval  st;
}DownloadStat;

struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "install", no_argument, NULL, 'I' },
    { "reinstall", no_argument, NULL, 'r' },
    { "install-dev", no_argument, NULL, 'i' },
    { "remove", no_argument, NULL, 'R' },
    { "autoremove", no_argument, NULL, 'A' },
    { "search", no_argument, NULL, 'S' },
    { "show", no_argument, NULL, 's' },
    { "status", no_argument, NULL, 't' },
    { "clean", no_argument, NULL, 'C' },
    { "update", no_argument, NULL, 'u' },
    { "upgrade", no_argument, NULL, 'U' },
    { "verbose", no_argument, NULL, 'v' },
    { 0, 0, 0, 0}
};

void usage()
{
    char *usage = "\
        Usage: yget command [options] pkg1 [pkg2 ...]\n\n\
        yget is a simple command line interface for downloading and\n\
        installing packages. The most frequently used commands are update\n\
        and install.\n\n\
        Commands:\n\
            --install                 install packages and dependencies (pkg is leafpad not leafpad_0.8.17.ypk)\n\
            --reinstall               reinstall packages and dependencies (pkg is leafpad not leafpad_0.8.17.ypk)\n\
            --install-dev             install build-dependencies for packages (pkg is leafpad not leafpad_0.8.17.ypk)\n\
            --remove                  remove package and orphaned dependencies\n\
            --search                  search packages\n\
            --show                    show package's infomations\n\
            --clean                   remove all downloaded packages\n\
            --upgrade                 upgrade system\n\
            --update                  retrieve new lists of packages\n\
\n\
        Options:\n\
            -p                      instead of actually install, simply display what to do\n\
            -y                      assume Yes to all queries\n\
            -d		                download only - do NOT install\n\
            -f | --force            force\n\
       ";

    printf( "%s\n", usage );
}

int yget_progress_callback( void *cb_arg, char *package_name, int action, double progress, char *msg )
{
    YPackageManager *pm;

    pm = (YPackageManager *)cb_arg;

    if( !action )
    {
        printf( "%s %s \n", msg, package_name );
        packages_log( pm, package_name, msg );

    }
    else if( action == 9 )
    {
        packages_log( pm, package_name, "finish" );
    }
    else
    {
        if( progress == 0 || progress == -1 )
        {
            printf( "%s... ", msg );
            packages_log( pm, package_name, msg );
        }

        if( progress == 1 )
        {
            if( action == 3)
            {
                printf( "\n%s\n", msg );
                packages_log( pm, package_name, msg );
            }
            else 
            {
                printf( "%s\n", "done" );
            }
        }
    }

    fflush( stdout );
    return 0;
}


int yget_download_progress_callback( void *cb_arg, char *package_name, double dltotal, double dlnow )
{
    int                 progress, bar_len;
    long                speed;
    char                *bar = "====================";
    char                *space = "                    ";
    struct timeval      now;
    DownloadStat        *statp;

    statp = (DownloadStat *)cb_arg;

    progress = (int)(dlnow * 100 / dltotal);
    if( progress < 0 || progress == statp->progress )
        return 0;


    speed = 0;
    statp->progress = progress;
    if( !gettimeofday( &now, NULL ) )
    {
        if( statp->st.tv_sec != now.tv_sec && statp->cnt != dlnow )
        {
            speed = (dlnow - statp->cnt) * 1000000 /  ( (now.tv_sec - statp->st.tv_sec) * 1000000 + (now.tv_usec - statp->st.tv_usec) );
            speed = speed < 0 ? 0 : speed;

            statp->st.tv_sec = now.tv_sec;
            statp->st.tv_usec = now.tv_usec; 
            statp->cnt = dlnow;
        }
    }


    bar_len = progress / 5;
    bar_len = bar_len > 0 ? bar_len : 1;

    printf( 
            "[%.*s>%.*s]   %.2f%s/%.2f%s[%3d%%]  %ld%s/s%s", 
            bar_len, 
            bar, 
            20 - bar_len, 
            space, 
            dlnow > 1048576 ? dlnow/1048576 : (dlnow > 1024 ? dlnow/1024 : dlnow),
            dlnow > 1048576 ? "MB" : (dlnow > 1024 ? "KB" : "B"),
            dltotal > 1048576 ? dltotal/1048576 : (dltotal > 1024 ? dltotal/1024 : dltotal),
            dltotal > 1048576 ? "MB" : (dltotal > 1024 ? "KB" : "B"),
            progress, 
            speed > 1024 ? speed/1024 : speed, 
            speed > 1024 ? "KB" : "B", 
            space 
            );

    if( progress == 100 )
        putchar( '\n' );
    else
        putchar( '\r' );

    fflush( stdout );

    return 0;
}

int yget_upgrade_self( char *package_path )
{
    pid_t           pid;
    struct stat     sb;
    char            lib_path[32];

    if( !package_path )
        return -1;

    if( util_mkdir( "/tmp/ypkg2_backup" ) != 0 )
        return -1;

    lstat( "/usr/lib/libypk.so", &sb );
    if( sb.st_mode & S_IFLNK )
    {
        memset( lib_path, 0, 32 );
        strncpy( lib_path, "/usr/lib/", 9 );
        if( readlink( "/usr/lib/libypk.so", lib_path+9, 23 ) == -1 )
            goto failed;

        if( util_copy_file( lib_path, "/tmp/ypkg2_backup/libypk.so" ) != 0 )
            goto failed;
        else
            chmod( "/tmp/ypkg2_backup/libypk.so", 0755 );
    }
    else
    {
        if( util_copy_file( "/usr/lib/libypk.so", "/tmp/ypkg2_backup/libypk.so" ) != 0 )
            goto failed;
        else
            chmod( "/tmp/ypkg2_backup/libypk.so", 0755 );
    }

    if( util_copy_file( "/usr/bin/ypkg2", "/tmp/ypkg2_backup/ypkg2" ) != 0 )
        goto failed;
    else
        chmod( "/tmp/ypkg2_backup/ypkg2", 0755 );

    if( util_copy_file( "/usr/bin/yget2", "/tmp/ypkg2_backup/yget2" ) != 0 )
        goto failed;
    else
        chmod( "/tmp/ypkg2_backup/yget2", 0755 );

    if( util_copy_file( "/usr/bin/ypkg2-upgrade", "/tmp/ypkg2_backup/ypkg2-upgrade" ) != 0 )
        goto failed;
    else
        chmod( "/tmp/ypkg2_backup/ypkg2-upgrade", 0755 );

    if( (pid = fork()) < 0 )
    {
        goto failed;
    }
    else if( pid == 0 )
    {
        if( execl( "/tmp/ypkg2_backup/ypkg2-upgrade", "ypkg2-upgrade", "/tmp/ypkg2_backup", package_path, NULL ) < 0 )
            goto failed;
    }

    return 0;

failed:
    util_remove_dir( "/tmp/ypkg2_backup" );
    return -1;
}


int yget_install_package( YPackageManager *pm, char *package_name, char *version, int download_only, int upgrade_self )
{
    int                 ret, return_code;
    char                *target_url = NULL, *package_url = NULL, *package_path = NULL, *pkg_sha = NULL, *ypk_sha = NULL;
    YPackage            *pkg;
    //YPackageDCB         dcb;
    DownloadStat        dl_stat;

    if( !package_name )
        return -1;

    packages_log( pm, package_name, "install" );
    packages_log( pm, package_name, "initialization" );

    return_code = 0;
    if( !packages_exists( pm, package_name, version ) )
    {
        printf( COLOR_RED "Error: Can't find the package %s(%s).\n" COLOR_RESET, package_name, version );
        return -2;
    }

    pkg = packages_get_package( pm, package_name, 0 );
    if( !pkg )
    {
        printf( COLOR_RED "Error: Can't find the package %s(%s).\n" COLOR_RESET, package_name, version );
        return -2;
    }

    package_url = packages_get_package_attr( pkg, "uri" );
    if( !package_url )
    {
        printf( COLOR_RED "Error: Can't get the download url of the package.\n" COLOR_RESET );
        return_code = -3;
        goto return_point;
    }

    package_path = util_strcat( pm->package_dest, "/", package_url+2, NULL );
    target_url = util_strcat( pm->source_uri, "/", package_url, NULL );

    packages_log( pm, package_name, "downloading" );
    printf( "Downloading %s to %s\n", target_url, package_path  );
    dl_stat.progress = 0;
    dl_stat.cnt = 0;
    dl_stat.st.tv_sec = 0;
    dl_stat.st.tv_usec = 0;
    //dcb.cb = yget_download_progress_callback;
    //dcb.arg = &dl_stat;

    pkg_sha = packages_get_package_attr( pkg, "sha" );

    if( !access( package_path, F_OK ) )
    {
        ypk_sha = util_sha1( package_path );

        if( strncmp( pkg_sha, ypk_sha, 41 ) )
        {
            if( packages_download_package( NULL, package_name, target_url, package_path, 1, yget_download_progress_callback,&dl_stat, yget_progress_callback, pm ) < 0 )
            {
                printf( COLOR_RED "Error: Can't download the package %s from %s.\n" COLOR_RESET, package_name, target_url );
                return -4;
                goto return_point;
            }

            free( ypk_sha );

            ypk_sha = util_sha1( package_path );
            if( ypk_sha && strncmp( pkg_sha, ypk_sha, 41 ) )
            {
                printf( COLOR_RED "Error: Download failed, sha1 checksum does not match. [%s sha1sum:%s]\n" COLOR_RESET, package_path, ypk_sha );
                return -4;
                goto return_point;
            }
        }
    }
    else
    {
        if( packages_download_package( NULL, package_name, target_url, package_path, 0, yget_download_progress_callback,&dl_stat, yget_progress_callback, pm ) < 0 )
        {
            printf( COLOR_RED "Error: Can't download the package %s from %s.\n" COLOR_RESET, package_name, target_url );
            return -4;
            goto return_point;
        }

        ypk_sha = util_sha1( package_path );
        if( ypk_sha && strncmp( pkg_sha, ypk_sha, 41 ) )
        {
            printf( COLOR_RED "Error: Download failed, sha1 checksum does not match. [%s sha1sum:%s]\n" COLOR_RESET, package_path, ypk_sha );
            return -4;
            goto return_point;
        }
    }

    if( !download_only )
    {
        printf( "Installing %s\n", package_path );
        if( upgrade_self )
        {
            yget_upgrade_self( package_path );
        }
        else
        {
            if( (ret = packages_install_local_package( pm, package_path, "/", 1, yget_progress_callback, pm )) )
            {
                switch( ret )
                {
                    case 1:
                    case 2:
                        printf( COLOR_YELLO "The latest version has installed.\n" COLOR_RESET );
                        break;
                    case 0:
                        printf( COLOR_GREEN "Installation successful.\n" COLOR_RESET );
                        break;
                    case -1:
                        printf( COLOR_RED "Error: Invalid format or File not found.\n" COLOR_RESET );
                        break;
                    case -2:
                        printf( COLOR_RED "Error: Architecture does not match.\n" COLOR_RESET );
                        break;
                    case -3:
                        printf( COLOR_RED "Error: missing runtime deps.\n" COLOR_RESET );
                        break;
                    case -4:
                        printf( COLOR_RED "Error: conflicting deps.\n" COLOR_RESET );
                        break;
                    case -5:
                    case -6:
                        printf( COLOR_RED "Error: Can not get package's infomation.\n" COLOR_RESET );
                        break;
                    case -7:
                        printf( COLOR_RED "Error: An error occurred while executing the pre_install script.\n" COLOR_RESET );
                        break;
                    case -8:
                        printf( COLOR_RED "Error: An error occurred while copy files.\n" COLOR_RESET );
                        break;
                    case -9:
                        printf( COLOR_RED "Error: An error occurred while executing the post_install script.\n" COLOR_RESET );
                    case -10:
                        printf( COLOR_RED "Error: An error occurred while updating database.\n" COLOR_RESET );
                        break;
                }

                return_code = -5;
            }
        }
    }

return_point:
    if( package_path )
        free( package_path );

    if( target_url )
        free( target_url );

    packages_free_package( pkg );
    return return_code;
}

int yget_install_list( YPackageManager *pm, YPackageChangeList *list, int download_only )
{
    int                     ret;
    YPackageChangeList      *cur_pkg;

    if( !list )
        return 0;


    while( list )
    {
        cur_pkg = list;
        
        printf( "Installing " COLOR_WHILE "%s" COLOR_RESET " ...\n", cur_pkg->name );
        ret = yget_install_package( pm, cur_pkg->name, cur_pkg->version, download_only, 0 );
        if( !ret )
        {
            printf( COLOR_GREEN "Installation successful.\n" COLOR_RESET );
        }
        else
        {
            printf( COLOR_RED "Error: Installation failed.\n" COLOR_RESET );
            return ret;
        }
        list = list->prev;
    }

    return 0;
}


int main( int argc, char **argv )
{
    int             c, force, download_only, simulation, reinstall, yes, unknown_arg, verbose, i, j, action, ret, err, flag, len, size, install_size, pkg_count, upgrade_ypkg;
    char            confirm, *tmp, *package_name, *install_date, *build_date, *version,  *installed, *can_update, *repo;

    YPackageManager *pm;
    YPackage        *pkg, *pkg2;
    YPackageData    *pkg_data;
    YPackageList    *pkg_list;
    YPackageChangeList     *depend_list, *recommended_list, *sub_list, *install_list, *remove_list, *upgrade_list, *cur_package, *cur_package2;       

    if( argc == 1 )
    {

        usage();
        return 1;
    }

    action = 0;
    err = 0;
    flag = 0;
    force = 0;
    download_only = 0;
    simulation = 0;
    reinstall = 0;
    yes = 0;
    unknown_arg = 0;
    verbose = 0;
    upgrade_ypkg = 0;


    while( ( c = getopt_long( argc, argv, ":hIiRASstCuUpydfrv", longopts, NULL ) ) != -1 )
    {
        switch( c )
        {
            case 'h': //show usage
            case 'I': //install
            case 'i': //install-dev
            case 'r': //reinstall
            case 'R': //remove
            case 'A': //autoremove
            case 'S': //search
            case 's': //search
            case 't': //status
            case 'C': //clean
            case 'u': //update
            case 'U': //upgrade
                if( !action )
                    action = c;
                break;

            case 'y': //non-interactive
                yes = 1;
                break;

            case 'f': //force
                force = 1;
                break;

            case 'd':
                download_only = 1;
                break;

            case 'p': 
                simulation = 1;
                break;

            case 'v': //verbose
                verbose = 1;
                break;

            case '?':
                unknown_arg = 1;
                break;
        }
    }

    if( unknown_arg )
    {
        usage();
        return 1;
    }

    pm = packages_manager_init();
    if( !pm )
    {
        fprintf( stderr, "Error: Can not open database.\n" );
        return 1;
    }

    switch( action )
    {
        /*
         * show usage
         */
        case 'h':
            usage();
            break;

        /*
         * remove package
         */
        case 'R':
            if( argc < 3 )
            {
                err = 1;
            }
            else if( geteuid() )
            {
                err = 2;
            }
            else
            {
                remove_list = NULL;
                for( i = optind; i < argc; i++)
                {
                    package_name = argv[i];
                    sub_list = packages_get_remove_list( pm, package_name, 0 );

                    if( sub_list )
                    {
                        cur_package = sub_list;
                        while( cur_package->prev )
                            cur_package = cur_package->prev;

                        cur_package->prev = remove_list;
                        remove_list = sub_list;
                    }
                    else
                    {
                        printf( "%s has no installed.\n", package_name );
                    }
                }

                packages_clist_remove_duplicate_item( remove_list );

                confirm = 'N';

                if( remove_list )
                {
                    printf( "Remove:" );
                    cur_package = remove_list;
                    while( cur_package )
                    {
                        if( cur_package->type == 1 )
                            printf(" %s", cur_package->name );

                        cur_package = cur_package->prev;
                    }

                    printf( "\nAuto-remove:" );
                    cur_package = remove_list;
                    while( cur_package )
                    {
                        if( cur_package->type == 2 )
                            printf(" %s", cur_package->name );

                        cur_package = cur_package->prev;
                    }

                    if( yes )
                    {
                        confirm = 'Y';
                    }
                    else
                    {
                        printf( "\nDo you want to continue [y/N]?" );
                        confirm = getchar();
                    }

                    if( confirm == 'Y' || confirm == 'y' )
                    {
                        packages_remove_list( pm, remove_list, yget_progress_callback, pm );
                    }
                    packages_free_remove_list( remove_list );
                }
            }
            break;

        /*
         * autoremove
         */
        case 'A':
            break;

        /*
         * install package
         */
        case 'r':
            reinstall = 1;

        case 'I':
            if( argc < 3 )
            {
                err = 1;
            }
            else if( geteuid() )
            {
                err = 2;
            }
            else
            {
                install_list = NULL;
                depend_list = NULL;
                recommended_list = NULL;
                for( i = optind; i < argc; i++)
                {
                    package_name = argv[i];

                    /*
                    install_list = packages_get_install_list( pm, package_name );
                    if(install_list)
                    {
                        printf( "Install: %s", install_list->name );
                        cur_package = install_list->prev;
                        while( cur_package )
                        {
                            printf(" %s ", cur_package->name );
                            cur_package = cur_package->prev;
                        }
                        continue;
                    }
                    */

                    if( !packages_exists( pm, package_name, NULL ) )
                    {
                        printf( "Error: %s not found.\n",  package_name );
                        continue;
                    }

                    if( (pkg = packages_get_package( pm, package_name, 0 )) )
                    {
                        version = packages_get_package_attr( pkg, "version" );
                        if( !force && !reinstall )
                        {
                            if( packages_has_installed( pm, package_name, version ) )
                            {
                                printf( "%s_%s has installed.\n", package_name, version );
                                continue;
                            }
                        }
                    }
                    else
                    {
                        printf( "Error: %s not found.\n",  package_name );
                        continue;
                    }


                    cur_package =  (YPackageChangeList *)malloc( sizeof( YPackageChangeList ) );
                    len = strlen( package_name );
                    cur_package->name = (char *)malloc( len + 1 );
                    strncpy( cur_package->name, package_name, len );
                    cur_package->name[len] = 0;
                    cur_package->version = util_strcat( version, NULL );
                    cur_package->type = 1;
                    cur_package->prev = install_list;
                    install_list = cur_package;


                    sub_list = packages_get_depend_list( pm, package_name, version );
                    if( sub_list )
                    {
                        cur_package = sub_list;
                        while( cur_package->prev )
                            cur_package = cur_package->prev;
                        cur_package->prev = depend_list;
                        depend_list = sub_list;
                    }


                    sub_list = packages_get_recommended_list( pm, package_name, version );
                    if( sub_list )
                    {
                        cur_package = sub_list;
                        while( cur_package->prev )
                            cur_package = cur_package->prev;
                        cur_package->prev = recommended_list;
                        recommended_list = sub_list;
                    }

                }

                confirm = 'N';

                if( install_list )
                {
                    printf( "Install: %s", install_list->name );
                    cur_package = install_list->prev;
                    while( cur_package )
                    {
                        printf(" %s ", cur_package->name );
                        cur_package = cur_package->prev;
                    }

                    if( depend_list )
                    {
                        printf( "\nAuto-install: %s", depend_list->name );
                        cur_package = depend_list->prev;
                        while( cur_package )
                        {
                            printf(" %s ", cur_package->name );
                            cur_package = cur_package->prev;
                        }
                    }

                    if( recommended_list )
                    {
                        printf( "\nRecommended-install: %s", recommended_list->name );
                        cur_package = recommended_list->prev;
                        while( cur_package )
                        {
                            printf(" %s ", cur_package->name );
                            cur_package = cur_package->prev;
                        }
                    }
                    putchar( '\n' );

                    if( !simulation )
                    {
                        if( yes )
                        {
                            confirm = 'Y';
                        }
                        else
                        {
                            printf( "Do you want to continue [Y/n]?" );
                            confirm = getchar();
                        }
                        if( confirm != 'n' && confirm != 'N' )
                        {
                            if( !yget_install_list( pm, depend_list, download_only ) || force )
                            {
                                if( !yget_install_list( pm, install_list, download_only ) )
                                {
                                    yget_install_list( pm, recommended_list, download_only );
                                }
                            }
                            else
                                err = 3;
                        }
                        else
                            err = 3;
                    }

                    packages_free_install_list( install_list );
                    packages_free_install_list( depend_list );
                    packages_free_install_list( recommended_list );
                }
            }
            break;

        /*
         * install dev
         */
        case 'i':
            if( argc < 3 )
            {
                err = 1;
            }
            else if( geteuid() )
            {
                err = 2;
            }
            else
            {
                for( i = optind; i < argc; i++)
                {
                    package_name = argv[i];
                    install_list = packages_get_dev_list( pm, package_name, NULL );
                    confirm = 'N';

                    if( install_list )
                    {
                        printf( "Install list: %s", install_list->name );
                        cur_package = install_list->prev;
                        while( cur_package )
                        {
                            printf(" %s ", cur_package->name );
                            cur_package = cur_package->prev;
                        }

                        if( yes )
                        {
                            confirm = 'Y';
                        }
                        else
                        {
                            printf( "\nDo you want to continue [y/N]?" );
                            confirm = getchar();
                        }

                        if( confirm == 'Y' || confirm == 'y' )
                        {
                            yget_install_list( pm, install_list, 0 );
                        }

                        packages_free_dev_list( install_list );
                    }
                    else
                    {
                        printf( "%s's development packages have installed.\n", package_name );
                    }
                }
            }
            break;

        /*
         * Clean
         */
        case 'C':
            if( yes )
            {
                confirm = 'Y';
            }
            else
            {
                printf( "Do you want to remove all downloaded packages? [y/N]?" );
                confirm = getchar();
            }
            if( confirm == 'Y' || confirm == 'y' )
            {
                packages_cleanup_package( pm );
            }
            break;
        /*
         * Search
         */
        case 'S':
            if( argc < 3 )
            {
                err = 1;
            }
            else
            {
                printf( "status \tname \tversion    \tchannel  \tdescription\n====================================================================\n" );

                for( i = optind; i < argc; i++)
                {
                    package_name = argv[i];

                    pkg_count = packages_get_count( pm, "name", package_name, 1, 0 );
                    if( pkg_count > 0 )
                    {
                        pkg_list = packages_get_list( pm, pkg_count, 0, "name", package_name, 1, 0 );
                        if( pkg_list )
                        {
                            for( j = 0; j < pkg_list->cnt; j++ )
                            {
                                installed = packages_get_list_attr( pkg_list, j, "installed" );
                                can_update = packages_get_list_attr( pkg_list, j, "can_update" );
                                if( installed[0] == '0' )
                                {
                                    installed = "[*]";
                                }
                                else if( can_update[0] == '1' )
                                {
                                    installed = "[U]";
                                }
                                else if( can_update[0] == '-' &&  can_update[1] == '1' )
                                {
                                    installed = "[D]";
                                }
                                else
                                {
                                    installed = "[I]";
                                }

                                repo = packages_get_list_attr( pkg_list, j, "repo" );

                                printf( 
                                        COLOR_GREEN "%-s "  COLOR_RESET  "\t%-s \t%-10s \t%-8s \t%-s\n",
                                        installed, 
                                        packages_get_list_attr( pkg_list, j, "name"), 
                                        packages_get_list_attr( pkg_list, j, "version"), 
                                        repo, 
                                        packages_get_list_attr( pkg_list, j, "description") 
                                        );

                            }
                            packages_free_list( pkg_list );
                        }
                        else
                        {
                            printf( COLOR_RED "* %s not found\n" COLOR_RESET,  package_name );
                        }
                    }
                    else
                    {
                        printf( COLOR_RED "* %s not found\n" COLOR_RESET,  package_name );
                    }
                }
            }

            break;

        /*
         * Show
         */
        case 's':
            if( argc != 3 )
            {
                err = 1;
            }
            else
            {
                package_name = argv[optind];
                pkg = NULL;
                pkg2 = NULL;
                pkg_data = NULL;
                install_date = NULL;
                build_date = NULL;
                if( (pkg = packages_get_package( pm, package_name, 0 )) )
                {
                    installed = packages_get_package_attr( pkg, "installed" );
                    can_update = packages_get_package_attr( pkg, "can_update" );
                    if( installed[0] != '0' )
                    {
                        pkg2 = packages_get_package( pm, package_name, 1 );
                    }

                    if( installed[0] == '0' )
                    {
                        installed = "*";
                    }
                    else if( can_update[0] == '1' )
                    {
                        installed = "U";
                    }
                    else if( can_update[0] == '-' &&  can_update[1] == '1' )
                    {
                        installed = "D";
                    }
                    else
                    {
                        installed = "I";
                    }

                    tmp = packages_get_package_attr( pkg, "build_date");
                    if( tmp )
                        build_date = util_time_to_str( atoi( tmp ) );
                    else
                        build_date = NULL;

                    if( pkg2 )
                    {
                        tmp = packages_get_package_attr( pkg2, "install_time");
                        if( tmp )
                            install_date = util_time_to_str( atoi( tmp ) );
                        else
                            install_date = NULL;
                    }

                    tmp = packages_get_package_attr( pkg, "size");
                    size = tmp ? atoi( tmp ) : 0;

                    pkg_data = packages_get_package_data( pm, package_name, 0 );
                    if( pkg_data )
                    {
                        tmp = packages_get_package_data_attr( pkg_data, 0, "data_install_size");
                        install_size = tmp ? atoi( tmp ) : 0;

                        repo = packages_get_package_attr( pkg, "repo");
                        if(repo == NULL)
                        {
                            repo = "stable";
                        }

                        printf( 
                                "Name: %s\nVersion: %s\nArch: %s\nRepo:%s\nCategory: %s\nPriority: %s\nStatus: %s\nInstall_date: %s\nAvailable: %s\nLicense: %s\nPackager: %s\nInstall Script: %s\nSize: %d%c\nSha: %s\nBuild_date: %s\nUri: %s\nInstall_size: %d%c\nDepend: %s\nBdepend: %s\nRecommended: %s\nConflict: %s\nDescription: %s\nHomepage: %s\n", 
                                package_name,
                                pkg2 ? packages_get_package_attr( pkg2, "version") : packages_get_package_attr( pkg, "version"), 
                                packages_get_package_attr( pkg, "arch"), 
                                repo,
                                packages_get_package_attr( pkg, "category"), 
                                packages_get_package_attr( pkg, "priority"), 
                                installed,
                                install_date ? install_date : "",  //install date
                                packages_get_package_attr( pkg, "version"),  //available
                                packages_get_package_attr( pkg, "license"), 
                                packages_get_package_attr( pkg, "packager"), 
                                packages_get_package_attr( pkg, "install"), 
                                size > 1000000 ? size / 1000000 : (size > 1000 ? size / 1000 : size), 
                                size > 1000000 ? 'M' : (size > 1000 ? 'K' : 'B'), 
                                packages_get_package_attr( pkg, "sha"), 
                                build_date ? build_date : "",
                                packages_get_package_attr( pkg, "uri"), 
                                install_size > 1000000 ? install_size / 1000000 : (install_size > 1000 ? install_size / 1000 : install_size), 
                                install_size > 1000000 ? 'M' : (install_size > 1000 ? 'K' : 'B'), 
                                util_chr_replace( packages_get_package_data_attr( pkg_data, 0, "data_depend"), ',', ' ' ),  //depend
                                util_chr_replace( packages_get_package_data_attr( pkg_data, 0, "data_bdepend"), ',', ' ' ),  //bdepend
                                util_chr_replace( packages_get_package_data_attr( pkg_data, 0, "data_recommended"), ',', ' ' ),  //recommended
                                packages_get_package_data_attr( pkg_data, 0, "data_conflict"),  //conflict
                                packages_get_package_attr( pkg, "description"),
                                packages_get_package_attr( pkg, "homepage")
                                );
                    }

                    if( build_date )
                        free( build_date );

                    if( install_date )
                        free( install_date );

                    packages_free_package_data( pkg_data );
                    packages_free_package( pkg );
                    packages_free_package( pkg2 );
                }
                else
                {
                    printf( COLOR_RED "* %s not found\n" COLOR_RESET,  package_name );
                }

            }

            break;

        /*
         * status
         */
        case 't':
            if( argc != 3 )
            {
                err = 1;
            }
            else
            {
                package_name = argv[optind];
                if( (pkg = packages_get_package( pm, package_name, 0 )) )
                {
                    installed = packages_get_package_attr( pkg, "installed" );
                    can_update = packages_get_package_attr( pkg, "can_update" );
                    if( installed[0] != '0' )
                    {
                        pkg2 = packages_get_package( pm, package_name, 1 );
                    }

                    if( installed[0] == '0' )
                    {
                        installed = "*";
                    }
                    else if( can_update[0] == '1' )
                    {
                        installed = "U";
                    }
                    else if( can_update[0] == '-' &&  can_update[1] == '1' )
                    {
                        installed = "D";
                    }
                    else
                    {
                        installed = "I";
                    }
                    printf( 
                            "Name: %s\nStatus: %s\nVersion: %s\nDescription: %s\n", 
                            package_name,
                            installed,
                            packages_get_package_attr( pkg, "version"), 
                            packages_get_package_attr( pkg, "description") 
                            );
                }
                else
                {
                    printf( COLOR_RED "* %s not found\n" COLOR_RESET,  package_name );
                }
            }

            break;

        /*
         * Upgrade
         */
        case 'U':
            if( geteuid() )
            {
                err = 2;
            }
            else
            {
                    depend_list = NULL;
                    recommended_list = NULL;
                    confirm = 'N';

                    upgrade_list = packages_get_upgrade_list( pm );

                    if( upgrade_list )
                    {
                        cur_package = upgrade_list;
                        while( cur_package )
                        {
                            if( !strcmp( cur_package->name, "ypkg2" ) )
                            {
                                upgrade_ypkg = 1;
                            }

                            sub_list = packages_get_depend_list( pm, cur_package->name, cur_package->version );
                            if( sub_list )
                            {
                                cur_package2 = sub_list;
                                while( cur_package2->prev )
                                    cur_package2 = cur_package2->prev;
                                cur_package2->prev = depend_list;
                                depend_list = sub_list;
                            }

                            sub_list = packages_get_recommended_list( pm, cur_package->name, cur_package->version );
                            if( sub_list )
                            {
                                cur_package2 = sub_list;
                                while( cur_package2->prev )
                                    cur_package2 = cur_package2->prev;
                                cur_package2->prev = recommended_list;
                                recommended_list = sub_list;
                            }

                            cur_package = cur_package->prev;
                        }
                        packages_clist_remove_duplicate_item( depend_list );
                        packages_clist_remove_duplicate_item( recommended_list );


                        printf( "Upgrade:" );
                        cur_package = upgrade_list;
                        while( cur_package )
                        {
                            if( cur_package->type == 4 )
                                printf(" %s", cur_package->name );
                            cur_package = cur_package->prev;
                        }

                        printf( "\nDowngrade:" );
                        cur_package = upgrade_list;
                        while( cur_package )
                        {
                            if( cur_package->type == 5 )
                                printf(" %s", cur_package->name );
                            cur_package = cur_package->prev;
                        }
                        putchar( '\n' );


                        if( depend_list )
                        {
                            printf( "\nAuto-install: %s", depend_list->name );
                            cur_package = depend_list->prev;
                            while( cur_package )
                            {
                                printf(" %s ", cur_package->name );
                                cur_package = cur_package->prev;
                            }
                        }

                        if( recommended_list )
                        {
                            printf( "\nRecommended-install: %s", recommended_list->name );
                            cur_package = recommended_list->prev;
                            while( cur_package )
                            {
                                printf(" %s ", cur_package->name );
                                cur_package = cur_package->prev;
                            }
                        }
                        putchar( '\n' );

                        if( yes )
                        {
                            confirm = 'Y';
                        }
                        else
                        {
                            printf( "Do you want to continue [y/N]?" );
                            confirm = getchar();
                        }

                        if( confirm == 'Y' || confirm == 'y' )
                        {
                            if( upgrade_ypkg ) //upgrade self
                            {
                                if( yget_install_package( pm, "ypkg2", NULL, 0, 1 ) != 0 )
                                {
                                    err = 3;
                                }
                            }
                            else //normal upgrade
                            {
                                if( !yget_install_list( pm, depend_list, download_only ) || force )
                                {
                                    if( !yget_install_list( pm, upgrade_list, download_only ) )
                                    {
                                        yget_install_list( pm, recommended_list, download_only );
                                    }
                                }
                            }
                        }

                        packages_free_upgrade_list( upgrade_list );
                    }
                    else
                    {
                        printf( "Done.\n" );
                    }
            }

            break;

        /*
         * update
         */
        case 'u':
            if( geteuid() )
            {
                err = 2;
            }
            else
            {
                ret = packages_check_update( pm );
                if(ret)
                {
                    if( packages_update( pm, yget_progress_callback, pm ) == -1 )
                        printf("Failed!\n");
                    else
                        printf("Done!\n");

                }
                else
                {
                    printf("Done!\n");
                }
            }
            break;

        default:
            usage();
    }
    packages_manager_cleanup( pm );

    if( err == 1 )
        usage();
    else if( err == 2 )
        fprintf( stderr, "Permission Denied!\n" );
    else if( err == 3 )
        fprintf( stderr, "Failed!\n" );

    return err;
}
