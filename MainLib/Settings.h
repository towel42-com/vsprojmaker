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

#include "DebugTarget.h"
#include "SABUtils/JsonUtils.h"
#include "SABUtils/VSInstallUtils.h"

#include <utility>
#include <QVariant>
#include <QDebug>
#include <set>
#include <QDir>
#include <functional>
#include <optional>
#include <tuple>
#include <QMetaObject>

class QStandardItem;
class QProgressDialog;
class QProcess;
class QJsonObject;
class QJsonValue;

class CValueBase
{
public:
    virtual QVariant get( const QVariant& defValue = QVariant() ) const = 0;
    virtual void set( const QVariant& value ) = 0;
    virtual bool isSet() const { return false; }
    virtual void reset() = 0;
    virtual void save( QJsonValue& value ) const = 0;
    virtual void load( const QJsonValue& value ) = 0;
};

template< typename T >
class CValue : public CValueBase
{
public:
    CValue() = default;
    CValue( const CValue& rhs ) = default;
    CValue& operator=( const CValue& rhs ) = default;

    virtual void set( const QVariant& value ) override
    {
        setValue( value.value< T >() );
    }
    void setValue( const T& value ) { fValue = value; }

    [[nodiscard]] virtual QVariant get( const QVariant& defValueVariant = QVariant() ) const override
    {
        T defValue = defValueVariant.value< T >();
        return QVariant::fromValue( getValue( defValue ) );
    }

    [[nodiscard]] T getValue( const T& defValue = T() ) const
    {
        if ( !isSet() )
            return defValue;
        return fValue.value();
    }

    virtual void save( QJsonValue & value ) const
    {
        if ( !fValue.has_value() )
            return;
        return ToJson( fValue.value(), value );
    }

    virtual void load( const QJsonValue& value )
    {
        T tmpValue;
        FromJson( tmpValue, value );
        fValue = tmpValue;
    }

    virtual void reset() override
    {
        fValue.reset();
    }

    virtual bool isSet() const override
    {
        return fValue.has_value();
    }
private:
    std::optional< T > fValue;
};

#define ADD_SETTING_BASE( Type, Name ) \
inline void set ## Name( const Type & value ){ /* qDebug() << #Name; */ f ## Name .setValue( value ); } \
inline [[nodiscard]] Type get ## Name( const Type & defValue = Type() ) const \
{ return f ## Name.getValue( defValue ); } \
inline [[nodiscard]] bool is ## Name ## Set() const { return f ## Name.isSet() ; } \
private: \
    CValue< Type > f ## Name

#define ADD_SETTING( Type, Name ) \
public: \
ADD_SETTING_BASE( Type, Name )

#define ADD_SETTING_SETFUNC( Type, Name, func ) \
public: \
inline void set ## Name( const Type & value ){ f ## Name .setValue( value ); func(); } \
inline [[nodiscard]] Type get ## Name( const Type & defValue = Type() ) const { return f ## Name.getValue( defValue ); } \
private: \
    CValue< Type > f ## Name

#define ADD_SETTING_DEPRICATED( Type, Name ) \
private: \
ADD_SETTING_BASE( Type, Name )

#define ADD_SETTING_VALUE( Name ) \
registerSetting( #Name, &f ## Name );

namespace NVSProjectMaker
{
    struct SDebugTarget;
    struct SDirInfo;
}

using TExecNameType = std::unordered_map< QString, std::list< std::pair< QString, bool > > >;
using TListOfStringPair = std::list< std::pair< QString, QString > >;
using TListOfDebugTargets = std::list< NVSProjectMaker::SDebugTarget >;
using TStringSet = std::set< QString >;
Q_DECLARE_METATYPE( TListOfDebugTargets );
Q_DECLARE_METATYPE( TListOfStringPair );
Q_DECLARE_METATYPE( TExecNameType );
Q_DECLARE_METATYPE( TStringSet );

QDebug& operator<<( QDebug& stream, const TExecNameType& value );

namespace NVSProjectMaker
{
    struct SSourceFileInfo
    {
        QString fName;
        QString fRelToDir;

        bool fIsBuildDir{ false };
        bool fIsDir{ false };
        bool fIsIncludeDir{ false };
        std::list< std::pair< QString, bool > > fExecutables;

        std::list< std::shared_ptr< SSourceFileInfo > > fChildren;

        std::list< std::shared_ptr< SSourceFileInfo > > fPairedChildDirectores;

        void createItem( QStandardItem * parent ) const;

        bool isPairedInclSrcDir( const QString & srcDir ) const; // return true when name is incl or src, and the peer directory exists
        bool isParentToPairedDirs( const QString & srcDir ) const; // returns true when the dir has both an incl and src child dir
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
        std::list< std::pair< QString, bool > > fExecutables;
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
        int computeTotal( std::shared_ptr< SSourceFileInfo > parent );

        int computeTotal( QProgressDialog * progress );
        std::shared_ptr< SSourceFileInfo > fRootDir;
    };

    class CSettings
    {
    public:
        CSettings();

        ~CSettings();

        void clear();
        void reset();

        QString fileName() const;
        bool loadSourceFiles( const QDir & sourceDir, const QString & dir, QProgressDialog * progress, const std::function< void( const QString & msg ) > & logit );
        std::optional< QString > getBuildDir( bool relPath = false ) const;
        std::optional< QString > getSourceDir( bool relPath = false ) const;
        std::optional< QString > getDir( const QString & relDir, bool relPath ) const;
        QStringList getCmakeArgs() const;

        bool saveSettings(); // only valid if filename has been set;

        void loadSettings(); // loads from the registry
        bool loadSettings( const QString & fileName ); // sets the filename and loads from it
        bool setFileName( const QString & fileName, bool andSave );

        bool isBuildDir( const QDir & relToDir, const QDir & dir ) const;
        bool isInclDir( const QDir & relToDir, const QDir & dir ) const;
        std::list< std::pair< QString, bool > > getExecutables( const QDir & dir ) const;

        QStringList addInclDirs( const QStringList & inclDirs );
        QStringList addPreProcessorDefines( const QStringList & preProcDefines );
        bool generate( QProgressDialog * progress, QWidget * parent, const std::function< void( const QString & msg ) > & logit ) const;
        std::list< std::shared_ptr< NVSProjectMaker::SDirInfo > > generateTopLevelFiles( QProgressDialog * progress, const std::function< void( const QString & msg ) > & logit, QWidget * parent ) const;
        std::list< std::shared_ptr< NVSProjectMaker::SDirInfo > > getDirInfo( std::shared_ptr< SSourceFileInfo > parent, QProgressDialog * progress ) const;
        bool getParentOfPairDirectoriesMap( std::shared_ptr< SSourceFileInfo > parent, QProgressDialog * progress ) const;

        std::pair< QString, bool > findSampleOutputPath(const QString & baseName ) const;
        
        QString getVersion() const;

        QString getPrimaryTargetSetting( const QString & projectName ) const;
        [[nodiscard]] QString getEnvVarsForShell() const;

        [[nodiscard]] std::shared_ptr< NVSProjectMaker::SSourceFileResults > getResults() const { return fResults; }
        [[nodiscard]] QString getClientName() const;

        [[nodiscard]] static QString getCMakeExecViaVSPath( const QString & dir );
        [[nodiscard]] QString getCMakeExecViaVSPath() const;

        [[nodiscard]] QString getCMakeExec() const;

        [[nodiscard]] QString getMSys64Dir( bool msys ) const;
        [[nodiscard]] QStringList getIncludeDirs() const;

        [[nodiscard]] QString cleanUp( const QDir & relToDir, const QString & str ) const;
        [[nodiscard]] QString cleanUp( const QString & str ) const;
        [[nodiscard]] QStringList cleanUp(const QStringList& stringlist) const;

        [[nodiscard]] QString getBuildItScript( const QString & buildDir, const QString & cmd, const QString & descriptiveName ) const;

        static [[nodiscard]] QStringList getQtIncludeDirs( const QString & qtDir );
        std::pair< int, std::vector< QMetaObject::Connection > > runCMake( const std::function< void( const QString & ) > & outFunc, const std::function< void( const QString & ) > & errFunc, QProcess * process, const std::pair< bool, std::function< void() > > & finishedInfo ) const;

        ADD_SETTING( QString, VSVersion );
        ADD_SETTING( bool, UseCustomCMake);
        ADD_SETTING( QString, CustomCMakeExec );
        ADD_SETTING( QString, Generator );
        ADD_SETTING( QString, ClientDir );
        ADD_SETTING( QString, SourceRelDir );
        ADD_SETTING( QString, BuildRelDir );
        ADD_SETTING_SETFUNC( QString, QtDir, [ this ]() { loadQtSettings(); } );
        ADD_SETTING( QStringList, QtDirs );
        ADD_SETTING( QString, ProdDir );
        ADD_SETTING( QString, MSys64Dir );
        ADD_SETTING( QStringList, SelectedQtDirs );
        ADD_SETTING( TStringSet, BuildDirs );
        ADD_SETTING( QStringList, InclDirs );
        ADD_SETTING( QStringList, SelectedInclDirs );
        ADD_SETTING( QStringList, PreProcDefines );
        ADD_SETTING( QStringList, SelectedPreProcDefines );
        ADD_SETTING( TExecNameType, ExecNames );
        ADD_SETTING( TListOfStringPair, CustomBuilds );
        ADD_SETTING( QString, PrimaryTarget );
        ADD_SETTING( TListOfDebugTargets, DebugCommands );
        ADD_SETTING( QString, BuildOutputDataFile );
        ADD_SETTING( QString, BldTxtProdDir );

        ADD_SETTING( bool, Verbose );

    public:
        QString getVSPathForVersion( const QString & version ) const;
        std::tuple< bool, QString, NVSInstallUtils::TInstalledVisualStudios > setupInstalledVSes( QProcess * process, bool * retry );
        static std::tuple< bool, QString, NVSInstallUtils::TInstalledVisualStudios > setupInstalledVSes( NVSInstallUtils::TInstalledVisualStudios & installedVSes, QProcess * process, bool * retry );
    private:
        void updateProcessEnvironment( QProcess * process ) const;
        QMap< QString, QString > getVarMap() const;

        void registerSetting( const QString & attribName, CValueBase * value ) const;
        QStringList getCustomBuildsForSourceDir( const QString & inSourcePath ) const;
        std::list < NVSProjectMaker::SDebugTarget > getDebugCommandsForSourceDir( const QString & inSourcePath ) const;
        bool loadSourceFiles( const QDir & sourceDir, const QString & dir, QProgressDialog * progress, std::shared_ptr< SSourceFileInfo  > parent, const std::function< void( const QString & msg ) > & logit );
        bool loadData();
        void incProgress( QProgressDialog * progress ) const;
        void registerSettings();
        void loadQtSettings();

        void save( QJsonObject& json ) const;
        void load( const QJsonObject& json );

        struct SCustomBuildDirInfo
        {
            SCustomBuildDirInfo( const QString & dirName );

            bool generateCustomTopBuild(const CSettings * settings, QWidget * parent) const;
            QString getDirName() const;
        private:
            QString getExtraArgs() const;

            QString fMakeProjectName;  // name to use for mtimake at the top level
            QString fCMakeProjectName; // used inside cmake
            QString fDirName;          // the subdir
            QStringList fExtraOptions; // the options
        };

        QString fSettingsFileName;
        void dump() const;
        std::shared_ptr< NVSProjectMaker::SSourceFileResults > fResults;
        mutable std::map< QString, std::pair< QString, bool > > fSamplesMap;
        mutable std::unordered_map< QString, CValueBase* > fSettings;
        mutable NVSInstallUtils::TInstalledVisualStudios fInstalledVSes;
    };
}

#endif 
