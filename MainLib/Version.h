#ifndef __VERSION_H
#define __VERSION_H

#include <QString>

namespace NVSProjectMaker
{
    inline const int MAJOR_VERSION = 1;
    inline const int MINOR_VERSION = 30;
    inline const int PATCH_VERSION = 0;

    inline QString getVersionString() { return QString( "%1.%2" ).arg( MAJOR_VERSION ).arg( MINOR_VERSION ) ; }
    inline QString getAppName() { return "VSProjectMaker"; }
}

#endif

