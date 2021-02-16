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
        fRelToDir = fileInfo->fRelToDir;
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

    void SDirInfo::createDebugProjects( QWidget * parent, const CSettings * settings ) const
    {
        for ( auto && ii : fDebugCommands )
        {
            if ( !QDir( settings->getBuildDir().value() ).mkpath( QString( "%1/DebugDir/%2" ).arg( fRelToDir ).arg( ii.getProjectName() ) ) )
            {
                QApplication::restoreOverrideCursor();
                QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error creating directory '%1" ).arg( QString( "CustomBuild/%1" ).arg( ii.fCmd ) ) );
                return;
            }

            NVSProjectMaker::readResourceFile( parent, QString( ":/resources/PropertySheetWithDebug.props" ),
                                [this, settings, ii, parent]( QString & resourceText )
            {
                QDir bldQDir( settings->getBuildDir().value() );
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

                resourceText.replace( "<PROJECT_NAME>", ii.getProjectName() );
                resourceText.replace( "<DEBUG_COMMAND>", ii.getCmd() );
                resourceText.replace( "<DEBUG_ARGS>", ii.fArgs );
                resourceText.replace( "<DEBUG_WORKDIR>", ii.fWorkDir );
                resourceText.replace( "<DEBUG_ENVVARS>", ii.getEnvVars() );

                resourceText.replace( "<INCLUDE_DIRS>", getInclDirs( settings ) );
                resourceText.replace( "<PREPROP_DEFS>", getPreprocessorDefines( settings ) );
                QTextStream qts( &fo );
                qts << resourceText;
                fo.close();
            }
            );
            NVSProjectMaker::readResourceFile( parent, QString( ":/resources/subdebugdir.cmake" ),
                              [this, settings, ii, parent]( QString & resourceText )
            {
                QDir bldQDir( settings->getBuildDir().value() );
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

                resourceText.replace( "<PROJECT_NAME>", ii.getProjectName() );
                auto outDir = QDir( settings->getBuildDir().value() );
                auto outPath = outDir.absoluteFilePath( fRelToDir );
                resourceText.replace( "<BUILD_DIR>", outPath );
                resourceText.replace( "<VSPROJDIR>", outPath );
                resourceText.replace( "<DEBUG_COMMAND>", ii.getCmd() );
                resourceText.replace( "<PROPSFILENAME>", "PropertySheetWithDebug.props" );

                QTextStream qts( &fo );
                qts << resourceText;
                qts << "\n\nset_target_properties( " << ii.getProjectName() << " PROPERTIES FOLDER " << "\"Debug Targets\"" << " )\n";
                fo.close();
            }
            );

        }
    }

    std::list< std::shared_ptr< NVSProjectMaker::SDirInfo > > SDirInfo::findPairDirs( const std::unordered_map < QString, std::shared_ptr< NVSProjectMaker::SDirInfo > > & map ) const
    {
        if ( !fRelToDir.endsWith( "src" ) )
            return {};

        auto relDir = fRelToDir;
        relDir = relDir.replace( '\\', '/' );

        auto pos = relDir.lastIndexOf( '/' );
        if ( pos != -1 )
            relDir = relDir.left( pos );

        relDir += "/incl";
        auto pos2 = map.find( relDir );
        if ( pos2 == map.end() )
            return {};
        return { (*pos2).second };
    }

    void SDirInfo::writePropSheet( QWidget * parent, const CSettings * settings ) const
    {
        QString fileName = "PropertySheetIncludes.props";
        NVSProjectMaker::readResourceFile( parent, QString( ":/resources/%1" ).arg( fileName ),
                                           [this, settings, fileName, parent]( QString & resourceText )
        {
            QDir bldQDir( settings->getBuildDir().value() );
            bldQDir.cd( fRelToDir );
            QString outFile = bldQDir.absoluteFilePath( fileName );
            QFile fo( outFile );
            if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
            {
                QApplication::restoreOverrideCursor();
                QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
                return;
            }
            resourceText.replace( "<PREPROP_DEFS>", getPreprocessorDefines( settings ) );
            resourceText.replace( "<INCLUDE_DIRS>", getInclDirs( settings ) );
            QTextStream qts( &fo );
            qts << resourceText;
            fo.close();
        }
        );
    }

    QString SDirInfo::getPreprocessorDefines( const CSettings * settings ) const
    {
        auto preProcDefs = settings->getSelectedPreProcDefines();

        return preProcDefs.join( ";" );
    }

    QString SDirInfo::getInclDirs( const CSettings * settings ) const
    {
        auto inclDirs = QStringList()
            << QDir( settings->getSourceDir().value() ).absoluteFilePath( getInclRelPath() )
            ;
    
        if ( getInclRelPath() != getSrcRelPath() )
        {
            inclDirs
                << QDir( settings->getSourceDir().value() ).absoluteFilePath( getSrcRelPath() )
                ;
        }

        inclDirs
            << QDir( settings->getBuildDir().value() ).absoluteFilePath( getSrcRelPath() )
            << settings->getIncludeDirs();

        return inclDirs.join( ";" );
    }

    QString SDirInfo::getSrcRelPath() const
    {
        QString retVal = fRelToDir;
        if ( fIsPairedDir )
            retVal = retVal + "/src";
        return retVal;
    }

    QString SDirInfo::getInclRelPath() const
    {
        QString retVal = fRelToDir;
        if ( fIsPairedDir )
            retVal = retVal + "/incl";
        return retVal;
    }

    void SDirInfo::writeCMakeFile( QWidget * parent, const CSettings * settings ) const
    {
        auto vsProjDir = QDir( settings->getBuildDir().value() );
        auto outPath = vsProjDir.absoluteFilePath( getSrcRelPath() );

        if ( !vsProjDir.cd( fRelToDir ) )
        {
            if ( !vsProjDir.mkpath( fRelToDir ) || !vsProjDir.cd( fRelToDir ) )
            {
                QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Could not create missing output directory '%1'" ).arg( outPath ) );
                return;
            }
        }

        bool isSubHeader = ( !fIsBuildDir && fExecutables.isEmpty() && fSourceFiles.isEmpty() );
        auto buildItFileName = vsProjDir.absoluteFilePath( QString( "buildit.sh" ) );
        if ( !isSubHeader )
        {
            QFile fo( buildItFileName );
            if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
            {
                QApplication::restoreOverrideCursor();
                QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( buildItFileName ).arg( fo.errorString() ) );
                return;
            }

            QTextStream qts( &fo );
            QString cmd = QString( "%1/usr/bin/make -w -j24" ).arg( settings->getMSys64Dir( true ) );
            qts << "echo cd " << outPath << "\n"
                << "cd \"" << outPath << "\"\n"
                << "echo " << cmd << "\n"
                << settings->getEnvVarsForShell()
                << cmd << "\n";
            fo.close();
        }
        QString resourceFile = isSubHeader ? "subheaderdir.cmake" : "subbuilddir.cmake";
        auto resourceText = NVSProjectMaker::readResourceFile( parent, QString( ":/resources/%1" ).arg( resourceFile ),
                                              [this, settings, buildItFileName, outPath, vsProjDir, parent ]( QString & resourceText )
        {
            resourceText.replace( "<PROJECT_NAME>", fProjectName );
            resourceText.replace( "<BUILD_DIR>", outPath );
            resourceText.replace( "<VSPROJDIR>", vsProjDir.absolutePath() );
            resourceText.replace( "<MSYS64DIR_MSYS>", settings->getMSys64Dir( true ) );
            resourceText.replace( "<MSYS64DIR_WIN>", settings->getMSys64Dir( false ) );
            resourceText.replace( "<BUILDITSHELL>", buildItFileName );
            replaceFiles( resourceText, "<SOURCE_FILES>", QStringList() << fSourceFiles );
            replaceFiles( resourceText, "<HEADER_FILES>", QStringList() << fHeaderFiles );
            replaceFiles( resourceText, "<UI_FILES>", QStringList() << fUIFiles );
            replaceFiles( resourceText, "<QRC_FILES>", QStringList() << fQRCFiles );
            replaceFiles( resourceText, "<OTHER_FILES>", QStringList() << fOtherFiles );
            resourceText.replace( "<PROPSFILENAME>", "PropertySheetIncludes.props" );
        }
        );

        auto outFile = vsProjDir.absoluteFilePath( QString( "CMakeLists.txt" ) );
        QFile fo( outFile );
        if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
        {
            QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
            return;
        }

        QTextStream qts( &fo );
        qts << resourceText;

        QString folder = "src/" + fRelToDir;
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

        for ( auto && curr : fileInfo->fChildren )
        {
            if ( curr->fIsDir )
                continue;

            addFile( curr->fRelToDir );
        }

        bool srcFound = false;
        for ( auto && ii : fileInfo->fPairedChildDirectores )
        {
            if ( ii->fRelToDir.endsWith( "src" ) )
                srcFound = true;
            getFiles( ii );
        }
        if ( srcFound )
            fIsPairedDir = true;
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