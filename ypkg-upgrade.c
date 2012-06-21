#include <stdio.h>
#include <dlfcn.h>
#include "ypackage.h"

int main( int argc, char **argv )
{
    int                 ret;
    char                *exec_path, *package_path, *dl_error;
    void                *dl_handle = NULL;

    YPackageManager     *pm;
    YPackageManager     *(*packages_manager_init)() = NULL;
    void                (*packages_manager_cleanup)( YPackageManager *pm ) = NULL;
    int                 (*packages_install_local_package)( YPackageManager *pm, char *ypk_path, char *dest_dir, int force, ypk_progress_callback cb, void *cb_arg ) = NULL;
    //ssize_t             (*packages_upgrade_db)( YPackageManager *pm ) = NULL;

    if( argc != 3 )
        return 1;

    exec_path = argv[1];
    package_path = argv[2];

    if( access( exec_path, F_OK ) )
    {
        ret = 2;
        goto exception_handler;
    }

    if( access( package_path, F_OK ) )
    {
        ret = 2;
        goto exception_handler;
    }

    if( chdir( exec_path ) == -1 )
    {
        ret = 2;
        goto exception_handler;
    }

    if( access( "ypkg2", F_OK ) || access( "yget2", F_OK ) || access( "libypk.so", F_OK ) || access( "ypkg2-upgrade", F_OK ) )
    {
        ret = 2;
        goto exception_handler;
    }

    dl_handle = dlopen( "./libypk.so", RTLD_LAZY | RTLD_GLOBAL );
    if( !dl_handle )
    {
        ret = 3;
        fprintf( stderr, "Error: %s\n", dlerror() );
        goto exception_handler;
    }

    dlerror();

    packages_manager_init = dlsym( dl_handle, "packages_manager_init" );
    if( (dl_error = dlerror()) != NULL )
    {
        ret = 3;
        fprintf( stderr, "Error: %s\n", dlerror() );
        goto exception_handler;
    }

    packages_manager_cleanup = dlsym( dl_handle, "packages_manager_cleanup" );
    if( (dl_error = dlerror()) != NULL )
    {
        ret = 3;
        fprintf( stderr, "Error: %s\n", dlerror() );
        goto exception_handler;
    }

    packages_install_local_package = dlsym( dl_handle, "packages_install_local_package" );
    if( (dl_error = dlerror()) != NULL )
    {
        ret = 3;
        fprintf( stderr, "Error: %s\n", dlerror() );
        goto exception_handler;
    }

    /*
    packages_upgrade_db = dlsym( dl_handle, "packages_upgrade_db" );
    if( (dl_error = dlerror()) != NULL )
    {
        ret = 3;
        fprintf( stderr, "Error: %s\n", dlerror() );
        goto exception_handler;
    }
    */

    pm = packages_manager_init();
    if( !pm )
    {
        ret = 4;
        fprintf( stderr, "Error: Can not open database.\n" );
        goto exception_handler;
    }


    sleep( 1 );

    if( packages_install_local_package( pm, package_path, "/", 1, NULL, NULL ) != 0 )
    {
        ret = 5;
        fprintf( stderr, "Error: Upgrade failed.\n" );
        goto exception_handler;
    }

    /*
    if( packages_upgrade_db( pm ) != 0 )
    {
        ret = 5;
        fprintf( stderr, "Error: Upgrade failed.\n" );
        goto exception_handler;
    }
    */

    printf( "Upgrade successful.\n" );


    packages_manager_cleanup( pm );
    dlclose( dl_handle );

    system( "yget2 --upgrade -y" );

    putchar( '\n' );
    return 0;

exception_handler:
    if( ret == 5 )
    {
        system( "cp libypk.so /usr/lib/libypk.so" );
        system( "cp ypkg2 /usr/bin/ypkg2" );
        system( "cp yget2 /usr/bin/yget2" );
        system( "cp ypkg2-upgrade /usr/bin/ypkg2-upgrade" );
    }

    if( dl_handle )
        dlclose( dl_handle );

    packages_manager_cleanup( pm );

    return ret;
}
