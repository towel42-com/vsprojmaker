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

#ifndef __SETTINGS_H
#define __SETTINGS_H

#include <utility>
#include <QVariant>
#include <QSettings>
#include <QSet>
#include <QDir>
#include <functional>
#include "DebugTarget.h"
#include <functional>
#include <optional>
class QStandardItem;
class QProgressDialog;

#define ADD_SETTING( Type, Name ) \
public: \
inline void set ## Name( const Type & value ){ f ## Name = QVariant::fromValue( value ); } \
inline Type get ## Name() const { return f ## Name.value< Type >(); } \
private: \
QVariant f ## Name

using TFuncType = std::function< void() >;

#define ADD_SETTING_SETFUNC( Type, Name, func ) \
public: \
inline void set ## Name( const Type & value ){ f ## Name = QVariant::fromValue( value ); func(); } \
inline Type get ## Name() const { return f ## Name.value< Type >(); } \
private: \
QVariant f ## Name

#define ADD_SETTING_VALUE( Type, Name ) \
registerSetting< Type >( #Name, const_cast<QVariant *>( &f ## Name ) );

namespace NVSProjectMaker
{
    struct SDebugTarget;
    struct SDirInfo;
}

using TExecNameType = QHash< QString, QList< QPair< QString, bool > > >;
using TListOfStringPair = QList< QPair< QString, QString > >;
using TListOfDebugTargets = QList< NVSProjectMaker::SDebugTarget >;
Q_DECLARE_METATYPE( TListOfDebugTargets );

namespace NVSProjectMaker
{
    struct SSourceFileInfo
    {
        QString fName;
        QString fRelPath;

        bool fIsBuildDir{ false };
        bool fIsDir{ false };
        bool fIsIncludeDir{ false };
        QList< QPair< QString, bool > > fExecutables;

        std::list< std::shared_ptr< SSourceFileInfo > > fChildren;

        void createItem( QStandardItem * parent ) const;
    };

    struct SSourceFileResults
    {
        SSourceFileResults() :
            fRootDir( std::make_shared< SSourceFileInfo >() )
        {
        }

        int fDirs{ 0 };
        int fFiles{ 0 };
        QStringList fBuildDirs;
        QStringList fInclDirs;
        QList< QPair< QString, bool > > fExecutables;
        QStringList fQtLibs;
        QString getText( bool forText ) const;

        void clear()
        {
            fDirs = 0;
            fFiles = 0;
            fBuildDirs.clear();
            fInclDirs.clear();
            fExecutables.clear();
            fQtLibs.clear();
            fRootDir = std::make_shared< SSourceFileInfo >();
        }
        std::shared_ptr< SSourceFileInfo > fRootDir;
    };

    class CSettings
    {
    public:
        CSettings();
        CSettings( const QString & fileName );

        ~CSettings();

        QString fileName() const;
        bool loadSourceFiles( const QDir & sourceDir, const QString & dir, QProgressDialog * progress, const std::function< void( const QString & msg ) > & logit );
        std::optional< QString > getBuildDir( bool relPath = false ) const;
        std::optional< QString > getSourceDir( bool relPath = false ) const;
        std::optional< QString > getDir( const QString & relDir, bool relPath ) const;
        QStringList getCmakeArgs() const;

        bool saveSettings(); // only valid if filename has been set;
        void loadSettings(); // loads from the registry
        bool loadSettings( const QString & fileName ); // sets the filename and loads from it
        void setFileName( const QString & fileName, bool andSave=true );

        bool isBuildDir( const QDir & relToDir, const QDir & dir ) const;
        bool isInclDir( const QDir & relToDir, const QDir & dir ) const;
        QList< QPair< QString, bool > > getExecutables( const QDir & dir ) const;

        QStringList addInclDirs( const QStringList & inclDirs );
        void generate( QProgressDialog * progress, QWidget * parent, const std::function< void( const QString & msg ) > & logit ) const;
        std::list< std::shared_ptr< NVSProjectMaker::SDirInfo > > generateTopLevelFiles( QProgressDialog * progress, const std::function< void( const QString & msg ) > & logit, QWidget * parent ) const;
        std::list< std::shared_ptr< NVSProjectMaker::SDirInfo > > getDirInfo( std::shared_ptr< SSourceFileInfo > parent, QProgressDialog * progress ) const;

        std::shared_ptr< NVSProjectMaker::SSourceFileResults > getResults() const { return fResults; }
        QString getClientName() const;
        ADD_SETTING( QString, CMakePath );
        ADD_SETTING( QString, Generator );
        ADD_SETTING( QString, ClientDir );
        ADD_SETTING( QString, SourceRelDir );
        ADD_SETTING( QString, BuildRelDir );
        ADD_SETTING_SETFUNC( QString, QtDir, [ this ]() { loadQtSettings(); } );
        ADD_SETTING( QStringList, QtDirs );
        ADD_SETTING( QStringList, SelectedQtDirs );
        ADD_SETTING( QSet< QString >, BuildDirs );
        ADD_SETTING( QStringList, InclDirs );
        ADD_SETTING( QStringList, SelectedInclDirs );
        ADD_SETTING( TExecNameType, ExecNames );
        ADD_SETTING( TListOfStringPair, CustomBuilds );
        ADD_SETTING( TListOfDebugTargets, DebugCommands );

    private:
        template< typename Type >
        void registerSetting( const QString & attribName, QVariant * value ) const
        {
            auto pos = fSettings.find( attribName );
            if ( pos != fSettings.end() )
                return;

            Q_ASSERT( value );
            if ( !value )
                return;
            if ( !value->isValid() )
                *value = QVariant::fromValue( Type() );
            fSettings[ attribName ] = value;
        }
        QStringList getCustomBuildsForSourceDir( const QString & inSourcePath ) const;
        QList < NVSProjectMaker::SDebugTarget > getDebugCommandsForSourceDir( const QString & inSourcePath ) const;
        bool loadSourceFiles( const QDir & sourceDir, const QString & dir, QProgressDialog * progress, std::shared_ptr< SSourceFileInfo  > parent, const std::function< void( const QString & msg ) > & logit );
        bool loadData();
        void incProgress( QProgressDialog * progress ) const;
        void registerSettings();
        void loadQtSettings();
        QString getIncludeDirs() const;

        std::unique_ptr< QSettings > fSettingsFile;
        std::shared_ptr< NVSProjectMaker::SSourceFileResults > fResults;


        mutable std::unordered_map< QString, QVariant * > fSettings;
    };
}

#endif 