#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include "package.h"

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
    { "install", no_argument, NULL, 'I' },
    { "install-dev", no_argument, NULL, 'i' },
    { "remove", no_argument, NULL, 'R' },
    { "autoremove", no_argument, NULL, 'A' },
    { "search", no_argument, NULL, 'S' },
    { "status", no_argument, NULL, 's' },
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
            install                 install packages and dependencies (pkg is leafpad not leafpad_0.8.17.ypk)\n\
            install-dev             install build-dependencies for packages (pkg is leafpad not leafpad_0.8.17.ypk)\n\
            remove                  remove package and orphaned dependencies\n\
            search                  search packages\n\
            clean                   remove all downloaded packages\n\
            upgrade                 upgrade system\n\
            update                  retrieve new lists of packages\n\
\n\
        Options:\n\
            -p                      instead of actually install, simply display what to do\n\
            -y                      assume Yes to all queries\n\
            -d		                download only - do NOT install\n\
            -f                      force install \n\
       ";

    printf( "%s\n", usage );
}

int main( int argc, char **argv )
{
    int             c, force, verbose, i, j, action, ret, err, flag, len;
    char            confirm, *tmp, *package_name, *file_name, *install_time, *build_date, *version, *depend, *bdepend, *recommended, *conflict, *infile, *outfile, *file_type, *installed, *can_update, *repo, *homepage;
    YPackageManager *pm;
    YPackage         *pkg;
    YPackageData    *pkg_data;
    YPackageFile    *pkg_file;
    YPackageList    *pkg_list;
    YPackageChangeList     *install_list, *remove_list, *upgrade_list, *cur_package;       

    if( argc == 1 )
    {
        usage();
        return 1;
    }

    action = 0;
    err = 0;
    flag = 0;
    force = 0;
    verbose = 0;


    while( ( c = getopt_long( argc, argv, ":hIiRASsCuUpydfv", longopts, NULL ) ) != -1 )
    {
        switch( c )
        {
            case 'h': //show usage
            case 'I': //install
            case 'i': //install-dev
            case 'R': //remove
            case 'A': //autoremove
            case 'S': //search
            case 's': //search
            case 'C': //clean
            case 'u': //update
            case 'U': //upgrade
                if( !action )
                    action = c;
                break;

            case 'p': //installation simulation
            case 'y': //non-interactive
            case 'd': //only download

            case 'f': //force
                force = 1;
                break;

            case 'v': //verbose
                verbose = 1;
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
                for( i = optind; i < argc; i++)
                {
                    package_name = argv[i];
                    remove_list = packages_get_remove_list( pm, package_name );
                    confirm = 'N';

                    if( remove_list )
                    {
                        printf( "Remove: %s", remove_list->name );
                        cur_package = remove_list->prev;
                        while( cur_package )
                        {
                            printf(",%s ", cur_package->name );
                            cur_package = cur_package->prev;
                        }
                        printf( "\nDo you want to continue [Y/N]?" );
                        confirm = getchar();
                        if( confirm == 'Y' || confirm == 'y' )
                        {
                            packages_remove_list( pm, remove_list );
                        }
                        packages_free_remove_list( remove_list );
                    }
                    else
                    {
                        printf( "%s has no installed.\n", package_name );
                    }
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
                    install_list = packages_get_install_list( pm, package_name );
                    confirm = 'N';

                    if( install_list )
                    {
                        printf( "Install: %s", install_list->name );
                        cur_package = install_list->prev;
                        while( cur_package )
                        {
                            printf(",%s ", cur_package->name );
                            cur_package = cur_package->prev;
                        }
                        printf( "\nDo you want to continue [Y/N]?" );
                        confirm = getchar();
                        if( confirm == 'Y' || confirm == 'y' )
                        {
                            packages_install_list( pm, install_list );
                        }
                        packages_free_install_list( install_list );
                    }
                    else
                    {
                        printf( "%s has installed.\n", package_name );
                    }
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
                    install_list = packages_get_install_list( pm, package_name );

                    if( install_list )
                    {
                        printf( "install list: %s", install_list->name );
                        cur_package = install_list->prev;
                        while( cur_package )
                        {
                            printf(",%s-dev ", cur_package->name );
                            cur_package = cur_package->prev;
                        }
                        putchar( '\n' );

                        packages_install_dev_list( pm, install_list );
                        packages_free_install_list( install_list );
                    }
                    else
                    {
                        printf( "%s-dev has installed.\n", package_name );
                    }
                }
            }
            break;

        /*
         * Clean
         */
        case 'C':
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
                for( i = optind; i < argc; i++)
                {
                    package_name = argv[i];

                    pkg_list = packages_get_list( pm, 10, 0, "name", package_name, 0, 0 );
                    if( pkg_list )
                    {
                        for( j = 0; j < pkg_list->cnt; j++ )
                        {
                            installed = packages_get_list_attr( pkg_list, j, "installed" );
                            can_update = packages_get_list_attr( pkg_list, j, "can_update" );
                            installed = installed[0] == '0' ? "[*]" : ( can_update[0] == '0' ? "[I]" : "[U]" );
                            repo = packages_get_list_attr( pkg_list, j, "repo" );

                            if( !verbose )
                            {
                                printf( 
                                        COLOR_GREEN "%s "  COLOR_RESET  "%s\t%s%c\t%s\n",
                                        installed, package_name, 
                                        packages_get_list_attr( pkg_list, j, "version"), 
                                        repo[0], 
                                        packages_get_list_attr( pkg_list, j, "description") 
                                        );
                            }
                            else
                            {
                                printf( COLOR_GREEN "%s "  COLOR_RESET  "%s\n", installed, package_name );
                                        
                                if( installed[1] != '*' && (pkg = packages_get_package( pm, package_name, 1 ) ) )
                                {
                                    tmp = packages_get_package_attr( pkg, "build_date" );
                                    if( tmp )
                                        build_date = util_time_to_str( atoi( tmp ) );
                                    else
                                        build_date = NULL;

                                    printf( 
                                            "\tInstalled: %s[%s]\t%s\n", 
                                            packages_get_package_attr( pkg, "version"),
                                            packages_get_package_attr( pkg, "repo"),
                                            build_date
                                            );

                                    if( build_date )
                                        free( build_date );

                                }

                                homepage = packages_get_list_attr( pkg_list, j, "homepage" );
                                tmp = packages_get_list_attr( pkg_list, j, "build_date" );
                                if( tmp )
                                    build_date = util_time_to_str( atoi( tmp ) );
                                else
                                    build_date = NULL;

                                printf( 
                                        "\tAvailable: %s[%s]\t%s\n\tDescription: %s\n", 
                                        packages_get_list_attr( pkg_list, j, "version"), 
                                        repo, 
                                        build_date,
                                        packages_get_list_attr( pkg_list, j, "description") 
                                        );

                                if( build_date )
                                    free( build_date );
                            }

                        }
                        packages_free_list( pkg_list );
                    }
                    else
                    {
                        printf( COLOR_RED "* %s not found\n" COLOR_RESET,  package_name );
                    }
                }
            }

            break;

        /*
         * Status
         */
        case 's':
            if( argc != 3 )
            {
                err = 1;
            }
            else
            {
                package_name = argv[optind];
                if( pkg = packages_get_package( pm, package_name, 0 ) )
                {
                    installed = packages_get_package_attr( pkg, "installed" );
                    can_update = packages_get_package_attr( pkg, "can_update" );
                    installed = installed[0] == '0' ? "N" : ( can_update[0] == '0' ? "I" : "U" );
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
                    upgrade_list = packages_get_upgrade_list( pm );
                    confirm = 'N';

                    if( upgrade_list )
                    {
                        printf( "Upgrade: %s", upgrade_list->name );

                        cur_package = upgrade_list->prev;
                        while( cur_package )
                        {
                            printf(",%s", cur_package->name );
                            cur_package = cur_package->prev;
                        }

                        printf( "\nDo you want to continue [Y/N]?" );
                        confirm = getchar();
                        if( confirm == 'Y' || confirm == 'y' )
                        {
                            packages_upgrade_list( pm, upgrade_list );
                        }

                        packages_free_upgrade_list( upgrade_list );
                    }
                    else
                    {
                        //printf( "has no installed.\n" );
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
                    printf("Updating ...");
                    packages_update( pm );
                    printf("Done!");
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
    return err;
}
