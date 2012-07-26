/* ypkg2
 *
 * Copyright (c) 2011-2012 StartOS
 *
 * Written by: 0o0<0o0zzyz@gmail.com> ChenYu_Xiao<yunsn0303@gmail.com>
 * Version: 0.1
 * Date: 2012.3.30
 */
#include <stdio.h>
#include <getopt.h>
#include <time.h>
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

struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "remove", no_argument, NULL, 'C' },
    { "install", no_argument, NULL, 'I' },
    { "check", no_argument, NULL, 'c' },
    { "list-files", no_argument, NULL, 'l' },
    { "depend", no_argument, NULL, 'k' },
    { "unpack-binary", no_argument, NULL, 'x' },
    { "unpack-info", no_argument, NULL, 'X' },
    { "pack-directory", no_argument, NULL, 'b' },
    { "list-installed", no_argument, NULL, 'L' },
    { "whatrequires", no_argument, NULL, 's' },
    { "whatprovides", no_argument, NULL, 'S' },
    { "compare-version", no_argument, NULL, 'm' },
    { "force", no_argument, NULL, 'f' },
    { 0, 0, 0, 0}
};

void usage()
{
    char *usage = "\
Usage: ypkg command pkg1 [pkg2 ...]\n\n\
ypkg is a simple command line for dealing with ypk package.\n\n\
Commands:\n\
  -h|--help                   Show usage\n\
  -C|--remove                 Remove package\n\
  -I|--install                Install package (pkg is leafpa.ypk not leafpad)\n\
  -c|--check                  Check dependencies of package (pkg is leafpad.ypk not leafpad)\n\
  -l|--list-files             List all files of installed package\n\
  -k|--depend                 Show dependency of package\n\
  -x|--unpack-binary          Unpack ypk package \n\
  -X|--unpack-info            Unpack ypkinfo \n\
  -b|--pack-directory         Pack directory to package\n\
  -L|--list-installed         List all installed packages\n\
  -s|--whatrequires           Show which package needs package\n\
  -S|--whatprovides           Search which package provide this file\n\
  --compare-version           Comprare two version strings \n\n\
Options:\n\
  -f|--force                  Override problems, Only work with install\n\
       ";

    printf( "%s\n", usage );
}

int ypkg_pack_callback( void *cb_arg, char *package_name, int action, double progress, char *msg )
{
    printf( "ypkg: %s\n", msg );
    return 0;
}

int ypkg_progress_callback( void *cb_arg, char *package_name, int action, double progress, char *msg )
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
        packages_log( pm, package_name, "Finish" );
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

int main( int argc, char **argv )
{
    int             c, force, i, j, action, ret, err, flag, len, exit_code;
    char            *tmp, *package_name, *ypk_path, *pack_path, *unpack_path, *file_name, *install_time, *version, *version2, *depend, *bdepend, *recommended, *conflict, *file_type;
    YPackageManager *pm;
    YPackage        *pkg, *pkg2;
    YPackageData    *pkg_data;
    YPackageFile    *pkg_file;
    YPackageList    *pkg_list;
        
    if( argc == 1 )
    {
        usage();
        return 1;
    }

    action = 0;
    err = 0;
    flag = 0;
    force = 0;
    exit_code = 0;
    version = NULL;
    pm = NULL;

    while( ( c = getopt_long( argc, argv, ":hCIclkxXbLsSmi:o:f", longopts, NULL ) ) != -1 )
    {
        switch( c )
        {
            case 'C': //remove package
            case 'I': //install package

            case 'c': //check dependencies, conlicts and so on of package
            case 'l': //list all files of installed package
            case 'k': //show dependency of package
            case 'L': //list all installed packages  
            case 's': //show which package needs package
            case 'S': //search which package provide this file

            case 'x': //unpack ypk package
            case 'X': //unpack ypkinfo
            case 'b': //pack directory to package
            case 'm': //comprare two version strings
            case 'h': //show usage
                if( !action )
                    action = c;
                break;

            case 'f':
                force = 1;
                break;

            case '?':
                break;
        }
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
        case 'C':
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
                pm = packages_manager_init2( 2 );
                if( !pm )
                {
                    switch( libypk_errno )
                    {
                        case MISSING_DB:
                            err = 4;
                        break;

                        case LOCK_ERROR:
                            err = 5;
                        break;

                        default:
                            err = 3;
                    }
                }
                else
                {
                    for( i = optind; i < argc; i++)
                    {
                        packages_log( pm, argv[i], "Remove" );
                        ret = packages_remove_package( pm, argv[i], ypkg_progress_callback, pm );
                        printf( "%s\n", ret < 0 ? "failed" : "successed" );
                    }
                    packages_manager_cleanup2( pm );
                }
            }
            break;

        /*
         * install package
         */
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
                pm = packages_manager_init2( 2 );
                if( !pm )
                {
                    switch( libypk_errno )
                    {
                        case MISSING_DB:
                            err = 4;
                        break;

                        case LOCK_ERROR:
                            err = 5;
                        break;

                        default:
                            err = 3;
                    }
                }
                else
                {
                    for( i = optind; i < argc; i++)
                    {
                        package_name = argv[i];
                        packages_log( pm, package_name, "Install" );
                        printf( "Installing " COLOR_WHILE "%s" COLOR_RESET " ...\n", package_name );

                        ret = packages_install_local_package( pm, package_name, "/", force, ypkg_progress_callback, pm );

                        switch( ret )
                        {
                            case 1:
                            case 2:
                                printf( COLOR_YELLO "Warning: Newer or same version installed, skipped.\n" COLOR_RESET );
                                break;
                            case 0:
                                printf( COLOR_GREEN "Installation successful.\n" COLOR_RESET );
                                break;
                            case -1:
                                printf( COLOR_RED "Error: invalid format or file not found.\n" COLOR_RESET );
                                break;
                            case -2:
                                printf( COLOR_RED "Error: architecture mismatched.\n" COLOR_RESET );
                                break;
                            case -3:
                                printf( COLOR_RED "Error: missing runtime dependencies.\n" COLOR_RESET );
                                break;
                            case -4:
                                printf( COLOR_RED "Error: conflicting dependencies found.\n" COLOR_RESET );
                                break;
                            case -5:
                            case -6:
                                printf( COLOR_RED "Error: reading state infomation failed.\n" COLOR_RESET );
                                break;
                            case -7:
                                printf( COLOR_RED "Error: an error occurred while executing the pre_install script.\n" COLOR_RESET );
                                break;
                            case -8:
                                printf( COLOR_RED "Error: an error occurred while copy files.\n" COLOR_RESET );
                                break;
                            case -9:
                                printf( COLOR_RED "Error: an error occurred while executing the post_install script.\n" COLOR_RESET );
                                break;
                            case -10:
                                printf( COLOR_RED "Error: an error occurred while updating database.\n" COLOR_RESET );
                                break;
                        }
                    }
                        packages_manager_cleanup2( pm );
                }
            }
            break;

        /*
         * check dependencies, conlicts and so on of package
         */
        case 'c':
            if( argc < 3 )
            {
                err = 1;
            }
            else
            {
                pm = packages_manager_init2( 1 );
                if( !pm )
                {
                    switch( libypk_errno )
                    {
                        case MISSING_DB:
                            err = 4;
                        break;

                        case LOCK_ERROR:
                            err = 5;
                        break;

                        default:
                            err = 3;
                    }
                }
                else
                {
                    for( i = optind; i < argc; i++)
                    {
                        package_name = argv[i];
                        printf( "Checking for " COLOR_WHILE "%s" COLOR_RESET " ...\n",  package_name );
                        ret = packages_check_package( pm, package_name, NULL, 0 );
                        switch( ret )
                        {
                            case 3:
                                printf( COLOR_GREEN "Can be upgraded.\n" COLOR_RESET );
                                break;
                            case 1:
                            case 2:
                                printf( COLOR_WHILE "The latest version has installed.\n" COLOR_RESET );
                                break;
                            case 0:
                                printf( COLOR_GREEN "Ready.\n" COLOR_RESET );
                                break;
                            case -1:
                                printf( COLOR_RED "Error: invalid format or file not found.\n" COLOR_RESET );
                                break;
                            case -2:
                                printf( COLOR_RED "Error: architecture mismatched.\n" COLOR_RESET );
                                break;
                            case -3:
                                printf( COLOR_RED "Error: missing runtime dependency.\n" COLOR_RESET );
                                break;
                            case -4:
                                printf( COLOR_RED "Error: conflicting dependency.\n" COLOR_RESET );
                                break;
                            default:
                                printf( COLOR_RED "Error: unknown error.\n" COLOR_RESET );
                        }


                    }
                }
            }

            break;
        /*
         * list all files of installed package
         */
        case 'l':
            if( argc < 3 )
            {
                err = 1;
            }
            else
            {
                for( i = optind; i < argc; i++)
                {
                    pkg_file = NULL;
                    package_name = argv[i];
                    len = strlen( package_name );
                    if( strncmp( package_name+len-4, ".ypk", 4 ) || access( package_name, R_OK ) )
                    {
                        if( !pm )
                            pm = packages_manager_init2( 1 );

                        if( !pm )
                        {
                            switch( libypk_errno )
                            {
                                case MISSING_DB:
                                    err = 4;
                                break;

                                case LOCK_ERROR:
                                    err = 5;
                                break;

                                default:
                                    err = 3;
                            }
                        }
                        else
                        {
                            pkg_file = packages_get_package_file( pm, package_name );
                            if( (pkg = packages_get_package( pm, package_name, 1 )) )
                            {
                                version = packages_get_package_attr( pkg, "version" );
                            }
                            else
                            {
                                version = "Unknown";
                            }
                        }
                    }
                    else
                    {
                        pkg_file =  packages_get_package_file_from_ypk( package_name );
                        if( !packages_get_package_from_ypk( package_name, &pkg, NULL ) )
                        {
                            version = packages_get_package_attr( pkg, "version" );
                        }
                        else
                        {
                            version = "Unknown";
                        }
                    }


                    if( pkg_file )
                    {
                        printf( COLOR_YELLO "Contents of %s %s:\n" COLOR_RESET, package_name, version );
                        for( j = 0; j < pkg_file->cnt; j++ )
                        {
                            file_type = packages_get_package_file_attr( pkg_file, j, "type");
                            if( file_type[0] == 'F' ||  file_type[0] == 'D' || file_type[0] == 'S' )
                                printf( "%s|%10s| %s\n",  file_type, packages_get_package_file_attr( pkg_file, j, "size"), packages_get_package_file_attr( pkg_file, j, "file") );
                        }
                        packages_free_package_file( pkg_file );

                        printf( "\nFile: %d, Dir: %d, Link: %d, Size: %dK\n", pkg_file->cnt_file,  pkg_file->cnt_dir, pkg_file->cnt_link, pkg_file->size );
                        printf( COLOR_YELLO "Contents of %s %s\n" COLOR_RESET, package_name, version );
                    }
                    else if( !err )
                    {
                        printf( COLOR_RED "* %s not found\n" COLOR_RESET,  package_name );
                    }
                }

                if( pm )
                    packages_manager_cleanup2( pm );
            }

            break;

        /*
         * show dependency of package
         */
        case 'k':
            if( argc != 3 )
            {
                err = 1;
            }
            else
            {
                package_name = argv[optind];

                pm = packages_manager_init2( 1 );
                if( !pm )
                {
                    switch( libypk_errno )
                    {
                        case MISSING_DB:
                            err = 4;
                        break;

                        case LOCK_ERROR:
                            err = 5;
                        break;

                        default:
                            err = 3;
                    }
                }
                else
                {

                    if( (pkg = packages_get_package( pm, package_name, 1 )) )
                    {
                        version = packages_get_package_attr( pkg, "version");
                    }
                    else
                    {
                        printf( COLOR_RED "* %s not found\n" COLOR_RESET,  package_name );
                        break;
                    }

                    if( (pkg_data = packages_get_package_data( pm, package_name, 1 )) )
                    {

                        for( i = 0; i < pkg_data->cnt; i++ )
                        {
                            printf( COLOR_WHILE "%s_%s" COLOR_RESET ":\n",  package_name, version );
                            bdepend = packages_get_package_data_attr( pkg_data, i, "data_bdepend");
                            if( bdepend )
                            {
                                printf( COLOR_GREEN "Build-dependencies:"  COLOR_RESET  "\n%s\n",  util_chr_replace( bdepend, ',', ' ' ) );
                            }

                            depend = packages_get_package_data_attr( pkg_data, i, "data_depend");
                            if( depend )
                            {
                                printf( COLOR_GREEN "Run-dependencies:"  COLOR_RESET  "\n%s\n",  util_chr_replace( depend, ',', ' ' ) );
                            }

                            recommended = packages_get_package_data_attr( pkg_data, i, "data_recommended");
                            if( recommended )
                            {
                                printf( COLOR_GREEN "Recommended:"  COLOR_RESET  "\n%s\n",  util_chr_replace( recommended, ',', ' ' ) );
                            }

                            conflict = packages_get_package_data_attr( pkg_data, i, "data_conflict");
                            if( conflict )
                            {
                                printf( COLOR_GREEN "Conflict:"  COLOR_RESET  "\n%s\n",  conflict );
                            }
                        }
                        packages_free_package_data( pkg_data );
                    }
                    packages_free_package( pkg );

                    packages_manager_cleanup2( pm );
                }
            }

            break;

        /*
         * list all installed packages  
         */
        case 'L':
            pm = packages_manager_init2( 1 );
            if( !pm )
            {
                switch( libypk_errno )
                {
                    case MISSING_DB:
                        err = 4;
                    break;

                    case LOCK_ERROR:
                        err = 5;
                    break;

                    default:
                        err = 3;
                }
            }
            else
            {
                pkg_list = packages_get_list( pm, 50000, 0, NULL, NULL, 0, 1 );
                if( pkg_list )
                {
                    for( i = 0; i < pkg_list->cnt; i++ )
                    {
                        tmp = packages_get_list_attr( pkg_list, i, "install_time" );
                        if( tmp )
                            install_time = util_time_to_str( atoi( tmp ) );
                        else
                            install_time = NULL;

                        printf( COLOR_GREEN "[I] "  COLOR_RESET  "%s\t%s\t%s\t%s\n", packages_get_list_attr( pkg_list, i, "name"), packages_get_list_attr( pkg_list, i, "version"), install_time ? install_time : "0", packages_get_list_attr( pkg_list, i, "description") );

                        if( install_time )
                            free( install_time );
                    }
                    packages_free_list( pkg_list );
                    packages_manager_cleanup2( pm );
                }
            }
            break;

        /*
         * show which package needs package
         */
        case 's':
            if( argc != 3 )
            {
                err = 1;
            }
            else
            {
                pm = packages_manager_init2( 1 );
                if( !pm )
                {
                    switch( libypk_errno )
                    {
                        case MISSING_DB:
                            err = 4;
                        break;

                        case LOCK_ERROR:
                            err = 5;
                        break;

                        default:
                            err = 3;
                    }
                }
                else
                {
                    package_name = argv[optind];
                    //depend
                    printf( "[R] stand for runtime depend, [B] for build time, [A] for recommoneded, [C] for conflict.\n" );
                    pkg_list = packages_get_list_by_depend( pm, 2000, 0, package_name, 1 );
                    if( pkg_list )
                    {
                        if( !flag )
                        {
                            flag = 1;
                            printf( COLOR_YELLO "%s is related with:\n" COLOR_RESET,  package_name );
                        }
                        for( i = 0; i < pkg_list->cnt; i++ )
                        {
                            printf( COLOR_BLUE "[R]" COLOR_RESET " %s\n",  packages_get_list_attr( pkg_list, i, "name") );
                        }
                        packages_free_list( pkg_list );
                    }

                    //bdepend
                    pkg_list = packages_get_list_by_bdepend( pm, 2000, 0, package_name, 1 );
                    if( pkg_list )
                    {
                        if( !flag )
                        {
                            flag = 1;
                            printf( COLOR_YELLO "%s is related with:\n" COLOR_RESET,  package_name );
                        }
                        for( i = 0; i < pkg_list->cnt; i++ )
                        {
                            printf( COLOR_BLUE "[B]" COLOR_RESET " %s\n",  packages_get_list_attr( pkg_list, i, "name") );
                        }
                        packages_free_list( pkg_list );
                    }

                    //recommoneded
                    pkg_list = packages_get_list_by_recommended( pm, 2000, 0, package_name, 1 );
                    if( pkg_list )
                    {
                        if( !flag )
                        {
                            flag = 1;
                            printf( COLOR_YELLO "%s is related with:\n" COLOR_RESET,  package_name );
                        }
                        for( i = 0; i < pkg_list->cnt; i++ )
                        {
                            printf( COLOR_BLUE "[A]" COLOR_RESET " %s\n",  packages_get_list_attr( pkg_list, i, "name") );
                        }
                        packages_free_list( pkg_list );
                    }

                    //conflict
                    pkg_list = packages_get_list_by_conflict( pm, 2000, 0, package_name, 1 );
                    if( pkg_list )
                    {
                        if( !flag )
                        {
                            flag = 1;
                            printf( COLOR_YELLO "%s is related with:\n" COLOR_RESET,  package_name );
                        }
                        for( i = 0; i < pkg_list->cnt; i++ )
                        {
                            printf( COLOR_BLUE "[C]" COLOR_RESET " %s\n",  packages_get_list_attr( pkg_list, i, "name") );
                        }
                        packages_free_list( pkg_list );
                    }

                    if( !flag )
                    {
                        printf( COLOR_RED "%s not found\n" COLOR_RESET,  package_name );
                    }

                    packages_manager_cleanup2( pm );
                }
            }

            break;

        /*
         * search which package provide this file
         */
        case 'S':
            if( argc < 3 )
            {
                err = 1;
            }
            else
            {
                for( i = optind; i < argc; i++)
                {
                    file_name = argv[i];
                    len = strlen( file_name );
                    if( file_name[len - 1] == '/' )
                        file_name[len -1] = '\0';

                    
                    if( !pm )
                        pm = packages_manager_init2( 1 );

                    if( !pm )
                    {
                        switch( libypk_errno )
                        {
                            case MISSING_DB:
                                err = 4;
                            break;

                            case LOCK_ERROR:
                                err = 5;
                            break;

                            default:
                                err = 3;
                        }
                    }
                    else
                    {

                        printf( "Searching for " COLOR_WHILE "%s" COLOR_RESET " ...\n",  file_name );
                        pkg_list = packages_get_list_by_file( pm, 2000, 0, file_name );
                        if( pkg_list )
                        {
                            for( j = 0; j < pkg_list->cnt; j++ )
                            {
                                file_type = packages_get_list_attr( pkg_list, j, "type");
                                printf( "%s_%s: %s, " COLOR_WHILE "%s" COLOR_RESET,  packages_get_list_attr( pkg_list, j, "name"), packages_get_list_attr( pkg_list, j, "version"), file_type, packages_get_list_attr( pkg_list, j, "file") );
                                if( file_type[0] == 'S' )
                                    printf( " -> %s", packages_get_list_attr( pkg_list, j, "extra") );
                                printf( "\n" );
                            }
                            packages_free_list( pkg_list );
                        }
                        else
                        {
                            printf( COLOR_RED "%s not owned by any packages.\n" COLOR_RESET,  file_name );
                        }
                        printf( "\n" );
                    }
                }

                if( pm )
                    packages_manager_cleanup2( pm );
            }
            break;

        /*
         * unpack ypk package
         */
        case 'x':
            if( argc < 3 && argc > 4 )
            {
                err = 1;
            }
            else
            {
                ypk_path = argv[2];
                tmp = argc == 4 ? argv[3] : NULL;
                unpack_path = NULL;
                pkg = NULL;

                if( access( ypk_path, R_OK ) )
                {
                    printf( COLOR_RED "%s not found\n" COLOR_RESET,  ypk_path );
                    break;
                }

                if( packages_check_package( pm, ypk_path, NULL, 0 ) == -1 )
                {
                    printf( COLOR_RED "Error: invalid format[%s]\n" COLOR_RESET, ypk_path );
                    break;
                }

                if( !tmp )
                {
                    if( packages_get_package_from_ypk( ypk_path, &pkg, NULL ) )
                    {
                        printf( COLOR_RED "Error: Invalid format[%s]\n" COLOR_RESET, ypk_path );
                        break;
                    }
                    else
                    {
                        package_name = packages_get_package_attr( pkg, "name" );
                        version = packages_get_package_attr( pkg, "version" );
                        unpack_path = util_strcat( package_name, "_", version, NULL );
                    }
                }

                packages_unpack_package( ypk_path, tmp ? tmp : unpack_path , 1, NULL );

                if( pkg )
                    packages_free_package( pkg );

                if( unpack_path )
                    free( unpack_path );
            }
            break;

        /*
         * unpack ypkinfo
         */
        case 'X':
            if( argc < 3 && argc > 4 )
            {
                err = 1;
            }
            else
            {
                ypk_path = argv[2];
                tmp = argc == 4 ? argv[3] : NULL;
                unpack_path = NULL;
                pkg = NULL;

                if( access( ypk_path, R_OK ) )
                {
                    printf( COLOR_RED "%s not found\n" COLOR_RESET,  ypk_path );
                    break;
                }

                if( packages_check_package( pm, ypk_path, NULL, 0 ) == -1 )
                {
                    printf( COLOR_RED "Error: invalid format[%s]\n" COLOR_RESET, ypk_path );
                    break;
                }

                if( !tmp )
                {
                    if( packages_get_package_from_ypk( ypk_path, &pkg, NULL ) )
                    {
                        printf( COLOR_RED "Error: invalid format[%s]\n" COLOR_RESET, ypk_path );
                        break;
                    }
                    else
                    {
                        package_name = packages_get_package_attr( pkg, "name" );
                        version = packages_get_package_attr( pkg, "version" );
                        unpack_path = util_strcat( package_name, "_", version, NULL );
                    }
                }

                packages_unpack_package( ypk_path, tmp ? tmp : unpack_path , 2, NULL );

                if( pkg )
                    packages_free_package( pkg );

                if( unpack_path )
                    free( unpack_path );
            }
            break;

        /*
         * pack directory to package
         */
        case 'b':
            if( argc < 3 && argc > 4 )
            {
                err = 1;
            }
            else
            {
                pack_path = argv[2];
                ypk_path = argc == 4 ? argv[3] : NULL;

                ret = packages_pack_package( pack_path, ypk_path, ypkg_pack_callback, NULL );
                if( !ret )
                {
                    printf( COLOR_GREEN "Package successful.\n" COLOR_RESET );
                }
                else if( ret == -1 )
                {
                    printf( COLOR_RED "Error: cannot access control.xml in the %s directory.\n" COLOR_RESET, pack_path );
                    err = 3;
                }
                else if( ret == -2 )
                {
                    printf( COLOR_RED "Error: missing some required configuration in control.xml.\n" COLOR_RESET );
                    err = 3;
                }
                else if( ret == -3 )
                {
                    printf( COLOR_RED "Error: cannot access filelist in the %s directory.\n" COLOR_RESET, pack_path );
                    err = 3;
                }
                else if( ret < 0 )
                {
                    printf( COLOR_RED "Error: an error occurred while packaging.\n" COLOR_RESET );
                    err = 3;
                }
            }
            break;

        /*
         * comprare two version strings
         */
        case 'm':
            if( argc != 4 )
            {
                err = 1;
            }
            else if( access( argv[optind], R_OK ) || access( argv[optind+1], R_OK ) )
            {
                version = argv[optind];
                version2 = argv[optind+1];

                ret = packages_compare_version( version, version2 );
                if( ret > 0 )
                {
                    printf( "%s > %s\n", version,  version2 );
                    exit_code = 1;
                }
                else if( ret < 0 )
                {
                    printf( "%s < %s\n", version,  version2  );
                    exit_code = 2;
                }
                else
                {
                    printf( "%s = %s\n",  version, version2  );
                    exit_code = 0;
                }
            }
            else
            {
                version = NULL;
                version2 = NULL;
                
                if( !packages_get_package_from_ypk( argv[optind], &pkg, NULL ) )
                {
                    version = packages_get_package_attr( pkg, "version" );

                    if( !packages_get_package_from_ypk( argv[optind + 1], &pkg2, NULL ) )
                    {
                        version2 = packages_get_package_attr( pkg2, "version" );

                        ret = packages_compare_version( version, version2 );
                        if( ret > 0 )
                        {
                            printf( "%s[version:%s] is newer than %s[version:%s]\n", argv[optind], version, argv[optind+1], version2 );
                            exit_code = 1;
                        }
                        else if( ret < 0 )
                        {
                            printf( "%s[version:%s] is older than %s[version:%s]\n", argv[optind], version, argv[optind+1], version2  );
                            exit_code = 2;
                        }
                        else
                        {
                            printf( "%s[version:%s] and %s[version:%s] has the same version\n", argv[optind], version, argv[optind+1], version2  );
                            exit_code = 0;
                        }

                    }
                    else
                    {
                        printf( COLOR_RED "Error: invalid format or file not found[%s]\n" COLOR_RESET, argv[optind + 1] );
                    }
                }
                else
                {
                    printf( COLOR_RED "Error: invalid format or file not found[%s]\n" COLOR_RESET, argv[optind] );
                }


                if( pkg )
                    packages_free_package( pkg );

                if( pkg2 )
                    packages_free_package( pkg2 );
            }
            break;
        default:
            usage();
    }

    if( err == 1 )
        usage();
    else if( err == 2 )
        fprintf( stderr, "Permission Denied!\n" );
    else if( err == 3 )
        fprintf( stderr, "Failed!\n" );
    else if( err == 4 )
        fprintf( stderr, "Error: open database failed.\n" );
    else if( err == 5 )
        fprintf( stderr, "Error: database is locked.\n" );

    if( err )
        exit_code = err;

    return exit_code;
}
