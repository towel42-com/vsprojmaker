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
#include <QString>
#include <QStringList>
#include <map>
#include <optional>
#include "SABUtils/StringComparisonClasses.h"
class QProgressDialog;
class QStandardItemModel;
class QStandardItem;

namespace NVSProjectMaker
{
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
        SItem( Qt::CaseSensitivity cs );

        virtual void initOptions() = 0;
        virtual bool loadData( const QString & line, int pos ) = 0;
        bool loadLine( const QString & line, int pos, std::function< void( const QString & nonOptLine ) > noOptFunc );

        virtual QString outFileOption() const = 0;
        virtual QString outFile() const;
        virtual QString outDir() const final;
        virtual QString firstSrcFile() const=0;
        virtual QString srcDir() const final;
        virtual QString dirForItem() const final;

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

        TOptionTypeMap fOptions;
        QStringList fOtherOptions;
        QString fPrevOption;

        std::pair< bool, QString > fStatus = std::make_pair( false, QString() );
    };

    struct SCompileItem : public SItem
    {
        SCompileItem();
        virtual bool loadData( const QString & line, int pos ) override;

        virtual void initOptions();
        virtual QString outFileOption() const override { return "Fo"; };
        virtual QString firstSrcFile() const override;
        virtual Qt::CaseSensitivity caseInsensitiveOptions() const override { return Qt::CaseSensitive; }
        virtual QString getItemTypeName() const { return "Compile"; }

        QStringList fSourceFiles;
    };

    struct SLibraryItem : public SItem
    {
        SLibraryItem();
        virtual bool loadData( const QString & line, int pos ) override;

        void initOptions();
        virtual QString outFileOption() const override { return "OUT"; };
        virtual QString firstSrcFile() const override;
        virtual Qt::CaseSensitivity caseInsensitiveOptions() const override { return Qt::CaseInsensitive; }
        virtual QString getItemTypeName() const { return "Library"; }

        QStringList fInputs;
    };

    struct SExecItem : public SItem
    {
        SExecItem();
        virtual bool loadData( const QString & line, int pos ) override;

        void initOptions();
        virtual QString outFileOption() const override { return "OUT"; };
        virtual QString firstSrcFile() const override;
        virtual Qt::CaseSensitivity caseInsensitiveOptions() const override { return Qt::CaseInsensitive; }
        virtual QString getItemTypeName() const { return "App/DLL"; }

        QStringList fFiles;
        QStringList fCommandFiles;

        std::list< std::shared_ptr< SExecItem > > fDependencies;
    };

    struct SManifestItem : public SItem
    {
        SManifestItem();
        virtual bool loadData( const QString & line, int pos ) override;

        void initOptions();
        virtual QString outFileOption() const override { return "OUT"; };
        virtual QString outFile() const override;

        virtual QString firstSrcFile() const override;
        virtual Qt::CaseSensitivity caseInsensitiveOptions() const override { return Qt::CaseInsensitive; }
        virtual QString getItemTypeName() const { return "Manifest"; }
    };

    struct SDirItem
    {
        SDirItem( const QString & dirName );

        QList< QStandardItem * > loadIntoTree();
        QString fDir;
        std::list< std::shared_ptr< SCompileItem > > fCompiledFiles;
        std::list< std::shared_ptr< SLibraryItem > > fLibraryItems;
        std::list< std::shared_ptr< SExecItem > > fExecutables;
        std::list< std::shared_ptr< SManifestItem > > fManifests;
    };

    class CBuildInfoData
    {
    public:
        CBuildInfoData( const QString & fileName, std::function< void( const QString & msg ) > reportFunc, QProgressDialog * progress );
        bool status() const { return fStatus.first; }
        QString errorString() const { return fStatus.second; }

        void loadIntoTree( QStandardItemModel * model );
    private:
        std::shared_ptr< SItem > loadLine( QRegularExpression & regExp, const QString & line, int lineNum, std::shared_ptr< SItem > item );
        
        bool loadCompile( const QString & line, int lineNum );
        bool loadLibrary( const QString & line, int lineNum );
        bool loadLink( const QString & line, int lineNum );
        bool loadManifest( const QString & line, int lineNum );

        std::shared_ptr< SDirItem > addDir( const QString & dir );
        std::function< void( const QString & msg ) > fReportFunc;
        std::map< QString, std::shared_ptr< SDirItem > > fDirectories;
        std::pair< bool, QString > fStatus = std::make_pair( false, QString() );;
    };
}

#endif 
