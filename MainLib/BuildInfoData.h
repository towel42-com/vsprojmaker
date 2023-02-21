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
// THE SOFTWARE IS PROVIDED "AS IS", WITHOut WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NoT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NoNINFRINGEMENT.IN No EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// Out OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __BUILDINFODATA_H
#define __BUILDINFODATA_H

#include "SABUtils/StringComparisonClasses.h"

#include <QString>
#include <QStringList>
#include <map>
#include <optional>
#include <set>
#include <functional>
#include <memory>

using TStringSet = std::set< QString >;

class QProgressDialog;
class QStandardItemModel;
class QStandardItem;

namespace NVSProjectMaker
{
    class CSettings;
    enum class EOptionType
    {
        eBool,
        eString,
        eStringList
    };

    // the value is Type, has Colon in Option, value
    using TOptionValue = std::optional< std::tuple< bool, QString, QStringList > >;
    class QStringCmp
    {
    public:
        explicit QStringCmp( Qt::CaseSensitivity cs ) :
            fCaseSensitivity( cs )
        {
        }
        bool operator() ( const QString & s1, const QString & s2 ) const
        {
            return s1.compare( s2, fCaseSensitivity ) < 0;
        }
    private:
        Qt::CaseSensitivity fCaseSensitivity;
    };

    using TOptionType = std::tuple< EOptionType, bool, TOptionValue >;
    using TOptionTypeMap = std::map< QString, TOptionType, QStringCmp >;
    struct SItem
    {
        SItem( int lineNum, Qt::CaseSensitivity cs );

        virtual void initOptions() = 0;
        virtual bool loadData( const QString & line, int pos ) = 0;
        bool loadLine( const QString & line, int pos, std::function< void( const QString & nonOptLine ) > noOptFunc );

        virtual QString targetFileOption() const = 0;
        virtual QString targetFile() const;
        virtual QString targetDir() const final;
        virtual QString firstSrcFile() const final;
        virtual QStringList allSources() const=0;
        virtual QString srcDir() const final;
        virtual QString dirForItem() const final;
        virtual bool srcPriorityForDir() const { return true; }

        TOptionValue getOptionValue( const QString & optName ) const
        {
            auto pos = fOptions.find( optName );
            if ( pos == fOptions.end() )
                return TOptionValue();

            auto foundOptInfo = ( *pos ).second;

            auto optValue = std::get< 2 >( foundOptInfo );
            return optValue;
        }

        template< typename T >
        bool getOptionValue( T & retVal, EOptionType optType, const QString & optName ) const
        {
            return false;
        }

        bool getOptionValue( bool & retVal, EOptionType optType, const QString & optName ) const
        {
            if ( optType != EOptionType::eBool )
                return false;

            auto optValue = getOptionValue( optName );
            if ( !optValue.has_value() )
                return false;

            retVal = std::get< 0 >( optValue.value() );
            return true;
        }

        bool getOptionValue( QString & retVal, EOptionType optType, const QString & optName ) const
        {
            if ( optType != EOptionType::eString )
                return false;

            auto optValue = getOptionValue( optName );
            if ( !optValue.has_value() )
                return false;

            retVal = std::get< 1 >( optValue.value() );
            return true;
        }

        bool getOptionValue( QStringList & retVal, EOptionType optType, const QString & optName ) const
        {
            if ( optType != EOptionType::eStringList )
                return false;

            auto optValue = getOptionValue( optName );
            if ( !optValue.has_value() )
                return false;

            retVal = std::get< 2 >( optValue.value() );
            return true;
        }

        virtual Qt::CaseSensitivity caseInsensitiveOptions() const = 0;

        virtual QString getItemTypeName() const = 0;

        virtual void loadIntoTree( QStandardItem * parent );

        bool status() const { return fStatus.first; }
        QString errorString() const { return fStatus.second; }

        static bool isTrue( const QString & value );

        virtual QStringList postLoadData( int lineNum, const QString & origProdDir, std::function< void( const QString & msg ) > reportFunc );
        QStringList transformProdDir( QString & curr, const QString & origProdDir ) const;
        QStringList transformProdDir( QStringList & currValues, const QString & origProdDir ) const;
        QStringList transformProdDir( TOptionTypeMap & currValues, const QString & origProdDir ) const;
        virtual QStringList xformProdDirInSourceAndTarget( const QString & origProdDir )=0;

        QString dump() const;
        QStringList fOtherOptions;
        QString fPrevOption;

        int fLineNumber{ -1 };
        TOptionTypeMap fOptions;
        std::pair< bool, QString > fStatus = std::make_pair( false, QString() );
        std::list< std::shared_ptr< SItem > > fDependencyItems;
        std::shared_ptr< SItem > fTargetItem;
    };

    struct SCompileItem : public SItem
    {
        SCompileItem( int lineNum ) :
            SItem( lineNum, Qt::CaseSensitivity::CaseSensitive )
        {
        }

        virtual QString getItemTypeName() const { return "Compile"; }
        virtual Qt::CaseSensitivity caseInsensitiveOptions() const override { return Qt::CaseSensitive; }
        virtual QStringList xformProdDirInSourceAndTarget( const QString & origProdDir );

        virtual QStringList allSources() const override;

        QStringList fSourceFiles;
    };

    struct SVSCLCompileItem : public SCompileItem
    {
        SVSCLCompileItem( int lineNum );
        virtual bool loadData( const QString & line, int pos ) override;

        virtual void initOptions();
        virtual QString targetFileOption() const override { return "Fo"; };

    };

    struct SGccCompileItem : public SCompileItem
    {
        SGccCompileItem( int lineNum );
        virtual bool loadData( const QString & line, int pos ) override;

        virtual void initOptions();
        virtual QString targetFileOption() const override { return "o"; };
    };

    struct SLibraryItem : public SItem
    {
        SLibraryItem( int lineNum );
        virtual bool loadData( const QString & line, int pos ) override;

        void initOptions();
        virtual QString targetFileOption() const override { return "OUT"; };
        virtual QStringList allSources() const override;
        virtual Qt::CaseSensitivity caseInsensitiveOptions() const override { return Qt::CaseInsensitive; }
        virtual QStringList xformProdDirInSourceAndTarget( const QString & origProdDir );
        virtual QString getItemTypeName() const { return "Library"; }
        virtual bool srcPriorityForDir() const { return false; }

        QStringList fInputs;
    };

    struct SExecItem : public SItem
    {
        SExecItem( int lineNum );
        virtual bool loadData( const QString & line, int pos ) override;

        void initOptions();
        virtual QString targetFileOption() const override { return "OUT"; };
        virtual QStringList allSources() const override;
        virtual Qt::CaseSensitivity caseInsensitiveOptions() const override { return Qt::CaseInsensitive; }
        virtual QStringList xformProdDirInSourceAndTarget( const QString & origProdDir );
        virtual QString getItemTypeName() const { return "App/DLL"; }
        virtual bool srcPriorityForDir() const { return false; }

        QStringList fFiles;
        QStringList fCommandFiles;
    };

    struct SManifestItem : public SItem
    {
        SManifestItem( int lineNum );
        virtual bool loadData( const QString & line, int pos ) override;

        void initOptions();
        virtual QString targetFileOption() const override { return "OUT"; };
        virtual QString targetFile() const override;

        virtual QStringList allSources() const override;
        virtual Qt::CaseSensitivity caseInsensitiveOptions() const override { return Qt::CaseInsensitive; }
        virtual QStringList xformProdDirInSourceAndTarget( const QString & origProdDir );
        virtual QString getItemTypeName() const { return "Manifest"; }
    };

    struct SObfuscatedItem : public SItem
    {
        SObfuscatedItem( int lineNum );
        virtual bool loadData( const QString & line, int pos ) override;

        void initOptions();
        virtual QString targetFileOption() const override { return "o"; };

        virtual QStringList allSources() const override;
        virtual Qt::CaseSensitivity caseInsensitiveOptions() const override { return Qt::CaseInsensitive; }
        virtual QStringList xformProdDirInSourceAndTarget( const QString & origProdDir );
        virtual QString getItemTypeName() const { return "Obfuscated"; }

        QString fInputFile;
    };

    struct SDirItem
    {
        SDirItem( const QString & dirName );

        QList< QStandardItem * > loadIntoTree( std::function< void( const QString & msg ) > reportFunc );
        QString fDir;
        std::list< std::shared_ptr< SVSCLCompileItem > > fVSCLCompiledFiles;
        std::list< std::shared_ptr< SGccCompileItem > > fGccCompiledFiles;
        std::list< std::shared_ptr< SLibraryItem > > fLibraryItems;
        std::list< std::shared_ptr< SExecItem > > fExecutables;
        std::list< std::shared_ptr< SManifestItem > > fManifests;
        std::list< std::shared_ptr< SObfuscatedItem > > fObfuscatedItems;
    };

    class CBuildInfoData
    {
    public:
        CBuildInfoData( const QString & fileName, std::function< void( const QString & msg ) > reportFunc, CSettings * settings, QProgressDialog * progress );
        bool status() const { return fStatus.first; }
        QString errorString() const { return fStatus.second; }

        void loadIntoTree( QStandardItemModel * model );
    private:
        bool isSourceFile( const QString & fileName ) const;
        void determineDependencies();
        void cleanupProdDirUsages( QStringList & currData );
        std::shared_ptr< SItem > loadLine( QRegularExpression & regExp, const QString & line, int lineNum, std::shared_ptr< SItem > item );

        struct SStatusInfo
        {
            int fLineNum;
            int fNumCL{ 0 };
            int fNumGcc{ 0 };
            int fNumCygwinCC{ 0 };
            int fNumLib{ 0 };
            int fNumLink{ 0 };
            int fNumManifest{ 0 };
            int fNumObfuscate{ 0 };
            int fNumMoc{ 0 };
            int fNumUIC{ 0 };
            int fNumRcc{ 0 };
            int fNumUnloaded{ 0 };

            QString getStatusString( size_t numDirectories, bool forGUI ) const;
        };

        bool loadVSCl( const QString & line, int lineNum );
        bool loadGcc( const QString & line, int lineNum );
        bool loadLibrary( const QString & line, int lineNum );
        bool loadLink( const QString & line, int lineNum );
        bool loadManifest( const QString & line, int lineNum );
        bool loadCygwinCC( const QString & line, int lineNum );
        bool loadObfuscate( const QString & line, int lineNum );
        bool loadMoc( const QString & line, int lineNum );
        bool loadUic( const QString & line, int lineNum );
        bool loadRcc(const QString & line, int lineNum);

        void addItem( std::shared_ptr< SItem > item );
        std::shared_ptr< SDirItem > addDir( const QString & dir );
        CSettings * fSettings{ nullptr }; // not owned`
        std::function< void( const QString & msg ) > fReportFunc;
        std::map< QString, std::shared_ptr< SDirItem > > fDirectories;

        std::multimap< QString, std::shared_ptr< SItem > > fTargets;
        std::multimap< QString, std::shared_ptr< SItem > > fSources;

        std::pair< bool, QString > fStatus = std::make_pair( false, QString() );
        TStringSet fProdDirUsages;
    };
}

#endif 
