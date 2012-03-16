#include <stdio.h>
#include <dlfcn.h>
#include "ypackage.h"

int main( int argc, char **argv )
{
    int                 ret;
    char                *exec_path, *package_path, *dl_error;
    void                *dl_handle;

    YPackageManager     *pm;
    YPackageManager     *(*packages_manager_init)();
    void                (*packages_manager_cleanup)( YPackageManager *pm );
    int                 (*packages_install_local_package)( YPackageManager *pm, char *ypk_path, char *dest_dir, int force, ypk_progress_callback cb, void *cb_arg );

    if( argc != 3 )
        return 1;

    exec_path = argv[1];
    package_path = argv[2];

    if( access( exec_path, F_OK ) )
        return 2;

    if( access( package_path, F_OK ) )
        return 2;

    if( chdir( exec_path ) == -1 )
        return 2;

    if( access( "ypkg2", F_OK ) || access( "yget2", F_OK ) || access( "libypk.so", F_OK ) || access( "ypkg2-upgrade", F_OK ) )
        return 2;

    dl_handle = dlopen( "./libypk.so", RTLD_LAZY | RTLD_GLOBAL );
    if( !dl_handle )
    {
        fprintf( stderr, "Error: %s\n", dlerror() );
        return 3;
    }

    dlerror();

    packages_manager_init = dlsym( dl_handle, "packages_manager_init" );
    if( (dl_error = dlerror()) != NULL )
    {
        fprintf( stderr, "Error: %s\n", dl_error );
        return 3;
    }

    packages_manager_cleanup = dlsym( dl_handle, "packages_manager_cleanup" );
    if( (dl_error = dlerror()) != NULL )
    {
        fprintf( stderr, "Error: %s\n", dl_error );
        return 3;
    }

    packages_install_local_package = dlsym( dl_handle, "packages_install_local_package" );
    if( (dl_error = dlerror()) != NULL )
    {
        fprintf( stderr, "Error: %s\n", dl_error );
        return 3;
    }

    pm = packages_manager_init();
    if( !pm )
    {
        fprintf( stderr, "Error: Can not open database.\n" );
        return 4;
    }

    sleep( 1 );

    ret = packages_install_local_package( pm, package_path, "/", 1, NULL, NULL );
    if( ret != 0 )
    {
        fprintf( stderr, "Error: Upgrade failed.\n" );
        system( "cp libypk.so /usr/lib/libypk.so" );
        system( "cp ypkg2 /usr/bin/ypkg2" );
        system( "cp yget2 /usr/bin/yget2" );
        system( "cp ypkg2-upgrade /usr/bin/ypkg2-upgrade" );
        return 4;
    }
    else
    {
        printf( "Installation successful.\n" );
    }


    packages_manager_cleanup( pm );

    dlclose( dl_handle );

    system( "yget2 --upgrade -y" );

    return 0;
}
