#include <stdio.h>
#include "package.h"

int main()
{
    YPackageManager *pm;

    if( geteuid() )
    {
        fprintf( stderr, "Permission Denied!\n" );
        return 1;
    }

    pm = packages_manager_init();
    if( !pm )
    {
        fprintf( stderr, "Error: Can not open database.\n" );
        return 1;
    }

    packages_import_local_data( pm );
    packages_manager_cleanup( pm );
    return 0;
}
