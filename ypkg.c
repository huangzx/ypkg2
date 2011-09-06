#include <getopt.h>
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
    { "remove", no_argument, NULL, 'C' },
    { "install", no_argument, NULL, 'I' },
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
        -I|--install  [*.ypk]                   install package (pkg is leafpad_0.8.17.ypk not leafpad)\n\
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
\n\
Options:\n\
        -i infile\n\
        -o outfile\n\
       ";

    printf( "%s\n", usage );
}

int main( int argc, char **argv )
{
    int             c, action, ret, err, flag, i, j;
    char            *package_name, *depend,  *infile, *outfile, *file_type;
    PACKAGE_MANAGER *pm;
    PACKAGE_DATA    *pkg_data;
    PACKAGE_FILE    *pkg_file;
    PACKAGE_LIST    *pkg_list;
        
    if( argc == 1 )
    {
        usage();
        return 1;
    }

    action = 0;
    err = 0;
    flag = 0;
    infile = NULL;
    outfile = NULL;
    if( !pm )
    {
        fprintf( stderr, "Error: Can not open database.\n" );
        return 1;
    }

    while( ( c = getopt_long( argc, argv, ":hCIclkxbLsSmi:o:", longopts, NULL ) ) != -1 )
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

            case '?':
                break;
        }
    }

    pm = packages_manager_init();
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
                    printf("removing %s ... ", argv[i]);
                    ret = packages_remove_package( pm, argv[i] );
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
                    printf("installing %s ... ", argv[i]);
                    packages_install_local_package( pm, argv[i], "/" );
                    printf( "%s\n", ret < 0 ? "failed" : "successed" );
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
                    pkg_file = packages_get_package_file( pm, argv[i] );
                    if( pkg_file )
                    {
                        printf("\n%s 's file list:\n\n", argv[i]);
                        for( j = 0; j < pkg_file->cnt; j++ )
                        {
                            printf( "%s\n",  packages_get_package_attr( pkg_file->file[j], "file") );
                        }
                        packages_free_package_file( pkg_file );
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
                printf( "* [R] stand for runtime depend, [A] for recommoneded, [C] for conflict.\n" );
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
                        printf( COLOR_BLUE "[R]" COLOR_RESET " %s\n",  packages_get_package_attr( pkg_list->list[i], "name") );
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
                        printf( COLOR_BLUE "[A]" COLOR_RESET " %s\n",  packages_get_package_attr( pkg_list->list[i], "name") );
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
                        printf( COLOR_BLUE "[C]" COLOR_RESET " %s\n",  packages_get_package_attr( pkg_list->list[i], "name") );
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
            if( argc != 3 )
            {
                err = 1;
            }
            else
            {
                package_name = argv[optind];
                printf( "* Searching for " COLOR_WHILE "%s" COLOR_RESET " ...\n",  package_name );
                pkg_list = packages_get_list_by_file( pm, 2000, 0, package_name );
                if( pkg_list )
                {
                    for( i = 0; i < pkg_list->cnt; i++ )
                    {
                        file_type = packages_get_package_attr( pkg_list->list[i], "type");
                        printf( "%s_%s: %s, " COLOR_WHILE "%s" COLOR_RESET,  packages_get_package_attr( pkg_list->list[i], "name"), packages_get_package_attr( pkg_list->list[i], "version"), file_type, packages_get_package_attr( pkg_list->list[i], "file") );
                        if( file_type[0] == 'S' )
                            printf( " -> %s", packages_get_package_attr( pkg_list->list[i], "extra") );
                        printf( " ...\n" );
                    }
                    packages_free_list( pkg_list );
                }
                else
                {
                    printf( COLOR_RED "* /home/eric/workspace/c/sm/ypkg not owned by any packages.\n" COLOR_RESET,  package_name );
                }
            }
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
            printf("^_^\n");
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
