// The MIT License( MIT )
//
// Copyright( c ) 2020-2021 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "DirInfo.h"
#include "DebugTarget.h"
#include "VSProjectMaker.h"
#include "Settings.h"

#include <QStandardItem>
#include <QDir>
#include <QApplication>
#include <QMessageBox>
#include <QTextStream>
#include <unordered_set>

namespace NVSProjectMaker
{
    SDirInfo::SDirInfo( const std::shared_ptr< SSourceFileInfo > & fileInfo ) :
        fIsInclDir( fileInfo->fIsIncludeDir ),
        fIsBuildDir( fileInfo->fIsBuildDir ),
        fExecutables( fileInfo->fExecutables )
    {
        computeRelToDir( fileInfo );
        getFiles( fileInfo );
    }

    void SDirInfo::computeRelToDir( const std::shared_ptr< SSourceFileInfo > & fileInfo )
    {
        fRelToDir = fileInfo->fRelPath;
        fProjectName = fRelToDir;
        fProjectName.replace( "/", "_" );
        fProjectName = fProjectName.replace( "\\", "_" );
        fProjectName = fProjectName.replace( " ", "_" );
    }

    bool SDirInfo::isValid() const
    {
        if ( fRelToDir.isEmpty() )
            return false;

        return ( fIsInclDir || fIsBuildDir || !fExecutables.isEmpty() || !fExtraTargets.isEmpty() || !fDebugCommands.isEmpty() );
    }

    void SDirInfo::replaceFiles( QString & text, const QString & variable, const QStringList & files ) const
    {
        QStringList values;
        for ( auto && ii : files )
        {
            values << QString( "    \"${ROOT_SOURCE_DIR}/%1\"" ).arg( ii );
        }
        auto value = values.join( "\n" );
        text.replace( variable, value );
    }

    QStringList SDirInfo::getSubDirs() const
    {
        QStringList retVal;
        retVal << fRelToDir;
        for ( auto && ii : fDebugCommands )
        {
            retVal << QString( "%1/DebugDir/%2" ).arg( fRelToDir ).arg( ii.getProjectName() );
        }
        return retVal;
    }

    void SDirInfo::createDebugProjects( QWidget * parent, const QString & bldDir ) const
    {
        for ( auto && ii : fDebugCommands )
        {
            if ( !QDir( bldDir ).mkpath( QString( "%1/DebugDir/%2" ).arg( fRelToDir ).arg( ii.getProjectName() ) ) )
            {
                QApplication::restoreOverrideCursor();
                QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error creating directory '%1" ).arg( QString( "CustomBuild/%1" ).arg( ii.fCmd ) ) );
                return;
            }

            NVSProjectMaker::readResourceFile( parent, QString( ":/resources/PropertySheetWithDebug.props" ),
                                [this, bldDir, ii, parent]( QString & text )
            {
                QDir bldQDir( bldDir );
                bldQDir.cd( fRelToDir );
                bldQDir.cd( "DebugDir" );
                bldQDir.cd( ii.getProjectName() );

                QString outFile = bldQDir.absoluteFilePath( "PropertySheetWithDebug.props" );
                QFile fo( outFile );
                if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
                {
                    QApplication::restoreOverrideCursor();
                    QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
                    return;
                }

                text.replace( "%PROJECT_NAME%", ii.getProjectName() );
                text.replace( "%DEBUG_COMMAND%", ii.getCmd() );
                text.replace( "%DEBUG_ARGS%", ii.fArgs );
                text.replace( "%DEBUG_WORKDIR%", ii.fWorkDir );
                text.replace( "%DEBUG_ENVVARS%", ii.getEnvVars() );
                QTextStream qts( &fo );
                qts << text;
                fo.close();
            }
            );
            NVSProjectMaker::readResourceFile( parent, QString( ":/resources/subdebugdir.txt" ),
                              [this, bldDir, ii, parent]( QString & text )
            {
                QDir bldQDir( bldDir );
                bldQDir.cd( fRelToDir );
                bldQDir.cd( "DebugDir" );
                bldQDir.cd( ii.getProjectName() );

                QString outFile = bldQDir.absoluteFilePath( "CMakeLists.txt" );
                QFile fo( outFile );
                if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
                {
                    QApplication::restoreOverrideCursor();
                    QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
                    return;
                }

                text.replace( "%PROJECT_NAME%", ii.getProjectName() );
                auto outDir = QDir( bldDir );
                auto outPath = outDir.absoluteFilePath( fRelToDir );
                text.replace( "%BUILD_DIR%", outPath );
                text.replace( "%DEBUG_COMMAND%", ii.getCmd() );
                text.replace( "%PROPSFILENAME%", "PropertySheetWithDebug.props" );

                QTextStream qts( &fo );
                qts << text;
                qts << "\n\nset_target_properties( " << ii.getProjectName() << " PROPERTIES FOLDER " << "\"Debug Targets\"" << " )\n";
                fo.close();
            }
            );

        }
    }

    void SDirInfo::writePropSheet( QWidget * parent, const QString & srcDir, const QString & bldDir, const QString & includeDirs ) const
    {
        QString fileName = "PropertySheetIncludes.props";
        NVSProjectMaker::readResourceFile( parent, QString( ":/resources/%1" ).arg( fileName ),
                          [this, srcDir, bldDir, includeDirs, fileName, parent]( QString & text )
        {
            QDir bldQDir( bldDir );
            bldQDir.cd( fRelToDir );
            QString outFile = bldQDir.absoluteFilePath( fileName );
            QFile fo( outFile );
            if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
            {
                QApplication::restoreOverrideCursor();
                QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
                return;
            }
            QString lclIncDirs = QDir( srcDir ).absoluteFilePath( fRelToDir ) + ";" + includeDirs;
            text.replace( "%INCLUDE_DIRS%", lclIncDirs );
            QTextStream qts( &fo );
            qts << text;
            fo.close();
        }
        );
    }

    void SDirInfo::writeCMakeFile( QWidget * parent, const QString & bldDir ) const
    {
        auto outDir = QDir( bldDir );
        auto outPath = outDir.absoluteFilePath( fRelToDir );

        QString resourceFile = ( !fIsBuildDir && fExecutables.isEmpty() && fSourceFiles.isEmpty() ) ? "subheaderdir.txt" : "subbuilddir.txt";
        auto resourceText = NVSProjectMaker::readResourceFile( parent, QString( ":/resources/%1" ).arg( resourceFile ),
                                              [this, outPath, parent]( QString & resourceText )
        {
            resourceText.replace( "%PROJECT_NAME%", fProjectName );
            resourceText.replace( "%BUILD_DIR%", outPath );
            replaceFiles( resourceText, "%SOURCE_FILES%", fSourceFiles );
            replaceFiles( resourceText, "%HEADER_FILES%", fHeaderFiles );
            replaceFiles( resourceText, "%UI_FILES%", fUIFiles );
            replaceFiles( resourceText, "%QRC_FILES%", fQRCFiles );
            replaceFiles( resourceText, "%OTHER_FILES%", fOtherFiles );
            resourceText.replace( "%PROPSFILENAME%", "PropertySheetIncludes.props" );
        }
        );

        if ( !outDir.cd( fRelToDir ) )
        {
            if ( !outDir.mkpath( fRelToDir ) || !outDir.cd( fRelToDir ) )
            {
                QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Could not create missing output directory '%1'" ).arg( outPath ) );
                return;
            }
        }

        QString outFile = outDir.absoluteFilePath( QString( "CMakeLists.txt" ) );
        QFile fo( outFile );
        if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
        {
            QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
            return;
        }

        QTextStream qts( &fo );
        qts << resourceText;

        QString folder = "Directories/" + fRelToDir;
        QFileInfo( fRelToDir ).path();
        if ( fIsBuildDir || fExecutables.isEmpty() )
        {
            qts << "set_target_properties( " << fProjectName << " PROPERTIES FOLDER " << folder << " )\n";
        }
        else
        {
            for ( auto && ii : fExecutables )
            {
                qts << "set_target_properties( " << fProjectName << " PROPERTIES FOLDER " << folder << " )\n";
            }
        }
    }

    void SDirInfo::addDependencies( QTextStream & qts ) const
    {
        qts << "                 ${project_SRCS}\n"
            << "                 ${project_H}\n"
            << "                 ${qtproject_UIS}\n"
            << "                 ${qtproject_QRC}\n"
            << "                 ${otherFILES}\n"
            ;
    }

    void SDirInfo::getFiles( const std::shared_ptr< SSourceFileInfo > & fileInfo )
    {
        if ( !fileInfo || !fileInfo->fIsDir )
            return;

        size_t numRows = fileInfo->fChildren.size();
        for ( auto && curr : fileInfo->fChildren )
        {
            if ( curr->fIsDir )
                continue;

            addFile( curr->fRelPath );
        }
    }

    void SDirInfo::addFile( const QString & path )
    {
        static std::unordered_set< QString > headerSuffixes =
        {
            "h", "hh", "hpp", "hxx"
        };
        static std::unordered_set< QString > sourceSuffixes =
        {
            "c", "cxx", "cpp"
        };
        QFileInfo fi( path );
        auto suffix = fi.suffix().toLower();
        if ( suffix == "ui" )
            fUIFiles << path;
        else if ( suffix == "qrc" )
            fQRCFiles << path;
        else if ( headerSuffixes.find( suffix ) != headerSuffixes.end() )
            fHeaderFiles << path;
        else if ( sourceSuffixes.find( suffix ) != sourceSuffixes.end() )
            fSourceFiles << path;
        else
            fOtherFiles << path;
    }
}