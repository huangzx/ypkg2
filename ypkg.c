/* ypkg2
 *
 * Copyright (c) 2011-2012 Ylmf OS
 *
 * Written by: 0o0 <0o0@115.com> <0o0zzyz@gmail.com>
 * Version: 0.1
 * Date: 2012.1.11
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
    { "in", no_argument, NULL, 'i' },
    { "out", no_argument, NULL, 'o' },
    { "force", no_argument, NULL, 'f' },
    { 0, 0, 0, 0}
};

void usage()
{
    char *usage = "\
ypkg is a simple command line for install ypk package.\n\n\
Usage: /bin/ypkg command pkg1 [pkg2 ...]\n\n\
Commands:\n\
        -h|--help                               show usage\n\
        -C|--remove                             remove package\n\
        -I|--install  [*.ypk]                   install package (pkg is leafpad_0.8.17.ypk not leafpad)\n\
        -c|--check    [*.ypk]                   check dependencies of package (pkg is leafpad_0.8.17.ypk not leafpad)\n\
        -l|--list-files                         list all files of installed package\n\
        -k|--depend                             show dependency of package\n\
        -x|--unpack-binary [*.ypk]              unpack ypk package \n\
        -X|--unpack-info [*.ypk]                unpack ypkinfo \n\
        -b|--pack-directory                     pack directory to package\n\
        -L|--list-installed                     list all installed packages\n\
        -s|--whatrequires                       show which package needs package\n\
        -S|--whatprovides [file]                search which package provide this file\n\
        --compare-version old new               comprare two version strings \n\
                                               return 0 if same. \n\
                                               return 1 if old is grater than new \n\
                                               return 2 if old is lesser then new\n\
\n\
Options:\n\
        -i|--in infile\n\
        -o|--out outfile\n\
        -f|--force                              work with --install\n\
       ";

    printf( "%s\n", usage );
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

int main( int argc, char **argv )
{
    int             c, force, i, j, action, ret, err, flag, len;
    char            *tmp, *package_name, *ypk_path, *unzip_path, *file_name, *install_time, *version, *version2, *depend, *bdepend, *recommended, *conflict, *infile, *outfile, *file_type;
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
    infile = NULL;
    outfile = NULL;

    while( ( c = getopt_long( argc, argv, ":hCIclkxXbLsSmi:o:f", longopts, NULL ) ) != -1 )
    {
        switch( c )
        {
            case 'h': //show usage
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
                if( !action )
                    action = c;
                break;

            case 'i':
                infile = optarg;
                break;

            case 'o':
                outfile = optarg;
                break;

            case 'f':
                force = 1;
                break;

            case '?':
                break;
        }
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
                for( i = optind; i < argc; i++)
                {
                    packages_log( pm, argv[i], "remove" );
                    ret = packages_remove_package( pm, argv[i], ypkg_progress_callback, pm );
                    printf( "%s\n", ret < 0 ? "failed" : "successed" );
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
                for( i = optind; i < argc; i++)
                {
                    package_name = argv[i];
                    packages_log( pm, package_name, "install" );
                    printf( "Installing " COLOR_WHILE "%s" COLOR_RESET " ...\n", package_name );
                    ret = packages_install_local_package( pm, package_name, "/", force, ypkg_progress_callback, pm );

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
                            break;
                    }
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
                for( i = optind; i < argc; i++)
                {
                    package_name = argv[i];
                    printf( "Checking for " COLOR_WHILE "%s" COLOR_RESET " ...\n",  package_name );
                    ret = packages_check_package( pm, package_name, NULL, 0 );
                    switch( ret )
                    {
                        case 2:
                            printf( COLOR_GREEN "Can be upgraded.\n" COLOR_RESET );
                            break;
                        case 1:
                            printf( COLOR_WHILE "The latest version has installed.\n" COLOR_RESET );
                            break;
                        case 0:
                            printf( COLOR_GREEN "Ready.\n" COLOR_RESET );
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
                        default:
                            printf( COLOR_RED "Error: unknown error.\n" COLOR_RESET );
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
                    package_name = argv[i];
                    len = strlen( package_name );
                    if( strncmp( package_name+len-4, ".ypk", 4 ) || access( package_name, R_OK ) )
                        pkg_file = packages_get_package_file( pm, package_name );
                    else
                        pkg_file =  packages_get_package_file_from_ypk( package_name );

                    if( pkg_file )
                    {
                        printf( COLOR_YELLO "* Contents of %s:\n" COLOR_RESET, package_name );
                        for( j = 0; j < pkg_file->cnt; j++ )
                        {
                            printf( "%s|%10s| %s\n",  packages_get_package_file_attr( pkg_file, j, "type"), packages_get_package_file_attr( pkg_file, j, "size"), packages_get_package_file_attr( pkg_file, j, "file") );
                        }
                        packages_free_package_file( pkg_file );

                        printf( "\nFile: %d, Dir: %d, Link: %d, Size: %dK\n", pkg_file->cnt_file,  pkg_file->cnt_dir, pkg_file->cnt_link, pkg_file->size );
                        printf( COLOR_YELLO "--- Contents of %s ---\n" COLOR_RESET, package_name );
                    }
                    else
                    {
                        printf( COLOR_RED "* %s not found\n" COLOR_RESET,  package_name );
                    }
                }
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
                if( pkg = packages_get_package( pm, package_name, 1 ) )
                {
                    version = packages_get_package_attr( pkg, "version");
                }
                else
                {
                    printf( COLOR_RED "* %s not found\n" COLOR_RESET,  package_name );
                    break;
                }

                if( pkg_data = packages_get_package_data( pm, package_name, 1 ) )
                {

                    for( i = 0; i < pkg_data->cnt; i++ )
                    {
                        printf( ">> Dependencies of " COLOR_WHILE "%s_%s" COLOR_RESET " data %d:\n",  package_name, version, i );
                        bdepend = packages_get_package_data_attr( pkg_data, i, "data_bdepend");
                        if( bdepend )
                        {
                            printf( COLOR_GREEN "* Build_time"  COLOR_RESET  "\n%s\n",  bdepend );
                        }

                        depend = packages_get_package_data_attr( pkg_data, i, "data_depend");
                        if( depend )
                        {
                            printf( COLOR_GREEN "* Run_time"  COLOR_RESET  "\n%s\n",  depend );
                        }

                        recommended = packages_get_package_data_attr( pkg_data, i, "data_recommended");
                        if( recommended )
                        {
                            printf( COLOR_GREEN "* Recommend"  COLOR_RESET  "\n%s\n",  recommended );
                        }

                        conflict = packages_get_package_data_attr( pkg_data, i, "data_conflict");
                        if( conflict )
                        {
                            printf( COLOR_GREEN "* Conflict"  COLOR_RESET  "\n%s\n",  conflict );
                        }
                    }
                    packages_free_package_data( pkg_data );
                }
                packages_free_package( pkg );
            }

            break;

        /*
         * list all installed packages  
         */
        case 'L':
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

                    printf( COLOR_GREEN "[I] "  COLOR_RESET  "%s_%s\t%s\t%s\nDescription: %s\n", packages_get_list_attr( pkg_list, i, "name"), packages_get_list_attr( pkg_list, i, "version"), install_time ? install_time : "0", packages_get_list_attr( pkg_list, i, "size"), packages_get_list_attr( pkg_list, i, "description") );

                    if( install_time )
                        free( install_time );
                }
                packages_free_list( pkg_list );
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
                package_name = argv[optind];
                //depend
                printf( "* [R] stand for runtime depend, [B] for build, [A] for recommoneded, [C] for conflict.\n" );
                pkg_list = packages_get_list_by_depend( pm, 2000, 0, package_name, 1 );
                if( pkg_list )
                {
                    if( !flag )
                    {
                        flag = 1;
                        printf( COLOR_YELLO "* %s is related with:\n" COLOR_RESET,  package_name );
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
                        printf( COLOR_YELLO "* %s is related with:\n" COLOR_RESET,  package_name );
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
                        printf( COLOR_YELLO "* %s is related with:\n" COLOR_RESET,  package_name );
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
                        printf( COLOR_YELLO "* %s is related with:\n" COLOR_RESET,  package_name );
                    }
                    for( i = 0; i < pkg_list->cnt; i++ )
                    {
                        printf( COLOR_BLUE "[C]" COLOR_RESET " %s\n",  packages_get_list_attr( pkg_list, i, "name") );
                    }
                    packages_free_list( pkg_list );
                }

                if( !flag )
                {
                    printf( COLOR_RED "* %s not found\n" COLOR_RESET,  package_name );
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

                    printf( "* Searching for " COLOR_WHILE "%s" COLOR_RESET " ...\n",  file_name );
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
                        printf( COLOR_RED "* %s not owned by any packages.\n" COLOR_RESET,  file_name );
                    }
                    printf( "\n" );
                }
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
                unzip_path = NULL;
                pkg = NULL;

                if( access( ypk_path, R_OK ) )
                {
                    printf( COLOR_RED "* %s not found\n" COLOR_RESET,  ypk_path );
                    break;
                }

                if( packages_check_package( pm, ypk_path, NULL, 0 ) == -1 )
                {
                    printf( COLOR_RED "Error: Invalid format[%s]\n" COLOR_RESET, ypk_path );
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
                        unzip_path = util_strcat( package_name, "_", version, NULL );
                    }
                }

                packages_unpack_package( pm, ypk_path, tmp ? tmp : unzip_path , 1 );

                if( pkg )
                    packages_free_package( pkg );

                if( unzip_path )
                    free( unzip_path );
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
                unzip_path = NULL;
                pkg = NULL;

                if( access( ypk_path, R_OK ) )
                {
                    printf( COLOR_RED "* %s not found\n" COLOR_RESET,  ypk_path );
                    break;
                }

                if( packages_check_package( pm, ypk_path, NULL, 0 ) == -1 )
                {
                    printf( COLOR_RED "Error: Invalid format[%s]\n" COLOR_RESET, ypk_path );
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
                        unzip_path = util_strcat( package_name, "_", version, NULL );
                    }
                }

                packages_unpack_package( pm, ypk_path, tmp ? tmp : unzip_path , 2 );

                if( pkg )
                    packages_free_package( pkg );

                if( unzip_path )
                    free( unzip_path );
            }
            break;

        /*
         * pack directory to package
         */
        case 'b':
            if( !outfile )
                outfile = "./";
            printf("pack to %s\n", outfile);
            break;

        /*
         * comprare two version strings
         */
        case 'm':
            if( argc != 4 )
            {
                err = 1;
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
                            printf( "%s[version:%s] is newer than %s[version:%s]\n", argv[optind], version, argv[optind+1], version2 );
                        else if( ret < 0 )
                            printf( "%s[version:%s] is older than %s[version:%s]\n", argv[optind], version, argv[optind+1], version2  );
                        else
                            printf( "%s[version:%s] and %s[version:%s] has the same version\n", argv[optind], version, argv[optind+1], version2  );

                    }
                    else
                    {
                        printf( COLOR_RED "Error: Invalid format or File not found[%s]\n" COLOR_RESET, argv[optind + 1] );
                    }
                }
                else
                {
                    printf( COLOR_RED "Error: Invalid format or File not found[%s]\n" COLOR_RESET, argv[optind] );
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
    packages_manager_cleanup( pm );

    if( err == 1 )
        usage();
    else if( err == 2 )
        fprintf( stderr, "Permission Denied!\n" );
    return err;
}
