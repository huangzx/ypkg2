#include <getopt.h>
#include "package.h"

struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "remove", no_argument, NULL, 'C' },
    { "install", no_argument, NULL, 'i' },
    { "check", no_argument, NULL, 'c' },
    { "list-files", no_argument, NULL, 'l' },
    { "depend", no_argument, NULL, 'k' },
    { "unpack-binary", no_argument, NULL, 'x' },
    { "pack-directory", no_argument, NULL, 'b' },
    { "list-installed", no_argument, NULL, 'L' },
    { "whatrequires", no_argument, NULL, 's' },
    { "whatprovides", no_argument, NULL, 'S' },
    { "compare-version", no_argument, NULL, 'm' },
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
       -i|--install  [*.ypk]                   install package (pkg is leafpad_0.8.17.ypk not leafpad)\n\
       -c|--check    [*.ypk]                   check dependencies of package (pkg is leafpad_0.8.17.ypk not leafpad)\n\
       -l|--list-files                         list all files of installed package\n\
       -k|--depend                             show dependency of package\n\
       -x|--unpack-binary [*.ypk]              unpack ypk package \n\
       -b|--pack-directory                     pack directory to package\n\
       -L|--list-installed                     list all installed packages\n\
       -s|--whatrequires                       show which package needs package\n\
       -S|--whatprovides [file]                search which package provide this file\n\
       --compare-version old new               comprare two version strings \n\
                                               return 0 if same. \n\
                                               return 1 if old is grater than new \n\
                                               return 2 if old is lesser then new\n\
       ";

    printf( "%s\n", usage );
}

int main( int argc, char **argv )
{
    int             c, done, i, j, ret, err;
    char            *depend;
    PACKAGE_MANAGER *pm;
    PACKAGE_DATA    *pkg_data;
    PACKAGE_FILE    *pkg_file;
    PACKAGE_LIST    *pkg_list;
        
    if( argc == 1 )
    {
        usage();
        return 1;
    }

    err = 0;
    done = 0;
    pm = packages_manager_init();
    if( !pm )
    {
        fprintf( stderr, "Error: Can not open database.\n" );
        return 1;
    }

    while( !done && ( c = getopt_long( argc, argv, ":hCiclkxbLsSm", longopts, NULL ) ) != -1 )
    {
        switch( c )
        {
            /*
             * show usage
             */
            case 'h':
                usage();
                done = 1;
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
                        printf("removing %s ... ", argv[i]);
                        ret = packages_remove_package( pm, argv[i] );
                        printf( "%s\n", ret < 0 ? "failed" : "successed" );
                    }
                }
                done = 1;
                break;

            /*
             * install package
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
                        printf("installing %s ... ", argv[i]);
                        packages_install_local_package( pm, argv[i], "/" );
                        printf( "%s\n", ret < 0 ? "failed" : "successed" );
                    }
                }
                done = 1;
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
                        printf("checking %s ... ", argv[i]);
                        ret = packages_check_package( pm, argv[i] );
                        switch( ret )
                        {
                            case 0:
                                printf( "ok.\n" );
                                break;
                            case -1:
                                printf( "invalid format or file not found.\n" );
                                break;
                            case -2:
                                printf( "has been installed.\n" );
                                break;
                            case -3:
                                printf( "missing runtime deps.\n" );
                                break;
                            case -4:
                                printf( "conflicting deps.\n" );
                                break;
                            default:
                                printf( "unknown error.\n" );
                        }
                    }
                }

                done = 1;
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
                        printf("\n%s 's file list:\n\n", argv[i]);
                        pkg_file = packages_get_package_file( pm, argv[i] );

                        for( j = 0; j < pkg_file->cnt; j++ )
                        {
                            printf( "%s\n",  packages_get_package_attr( pkg_file->file[j], "file") );
                        }
                        packages_free_package_file( pkg_file );
                    }
                }

                done = 1;
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
                    if( pkg_data = packages_get_package_data( pm, argv[2], 1 ) )
                    {

                        for( i = 0; i < pkg_data->cnt; i++ )
                        {
                            depend = packages_get_package_attr( pkg_data->data[i], "data_depend");
                            if( depend )
                            {
                                printf( "%s\n",  depend );
                            }
                        }
                        packages_free_package_data( pkg_data );
                    }
                }

                done = 1;
                break;

            /*
             * list all installed packages  
             */
            case 'L':
                pkg_list = packages_get_list( pm, 2000, 0, NULL, NULL, 0, 1 );

                if( pkg_list )
                {
                    printf( "Package Version Description\n" );
                    for( i = 0; i < pkg_list->cnt; i++ )
                    {
                        printf( "%s %s %s\n",  packages_get_package_attr( pkg_list->list[i], "name"), packages_get_package_attr( pkg_list->list[i], "version"), packages_get_package_attr( pkg_list->list[i], "description") );
                    }
                    packages_free_list( pkg_list );
                }
                done = 1;
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
                    pkg_list = packages_get_list_by_depend( pm, 2000, 0, argv[2], 1 );
                    if( pkg_list )
                    {
                        printf( "Package Version Description\n" );
                        for( i = 0; i < pkg_list->cnt; i++ )
                        {
                            printf( "%s %s %s\n",  packages_get_package_attr( pkg_list->list[i], "name"), packages_get_package_attr( pkg_list->list[i], "version"), packages_get_package_attr( pkg_list->list[i], "description") );
                        }
                        packages_free_list( pkg_list );
                    }
                }

                done = 1;
                break;

            /*
             * search which package provide this file
             */
            case 'S':
                if( argc != 3 )
                {
                    err = 1;
                }
                else
                {
                    pkg_list = packages_get_list_by_file( pm, 2000, 0, argv[2] );
                    if( pkg_list )
                    {
                        printf( "Package Version Description\n" );
                        for( i = 0; i < pkg_list->cnt; i++ )
                        {
                            printf( "%s %s %s\n",  packages_get_package_attr( pkg_list->list[i], "name"), packages_get_package_attr( pkg_list->list[i], "version"), packages_get_package_attr( pkg_list->list[i], "description") );
                        }
                        packages_free_list( pkg_list );
                    }
                }
                done = 1;
                break;

            /*
             * unpack ypk package
             */
            case 'x':
                if( argc != 4 )
                {
                    err = 1;
                }
                else
                {
                    packages_unpack_package( pm, argv[2], argv[3] );
                }
                done = 1;
                break;

            /*
             * pack directory to package
             */
            case 'b':
                printf("^_^\n");
                done = 1;
                break;

            /*
             * comprare two version strings
             */
            case 'm':
                printf("^_^\n");
                done = 1;
                break;
            default:
                usage();
                done = 1;
        }
    }
    packages_manager_cleanup( pm );

    if( err == 1 )
        usage();
    else if( err == 2 )
        fprintf( stderr, "Permission Denied!\n" );
    return err;
}
