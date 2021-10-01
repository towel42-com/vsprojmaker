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

#include "BuildInfoData.h"
#include "Settings.h"

#include <QObject>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>
#include <QProgressDialog>
#include <QStandardItemModel>

namespace NVSProjectMaker
{
    CBuildInfoData::CBuildInfoData( const QString & fileName, std::function< void( const QString & msg ) > reportFunc, CSettings * settings, QProgressDialog * progress ) :
        fReportFunc( reportFunc ),
        fSettings( settings )
    {
        QFileInfo fi( fileName );
        if ( !fi.exists() || !fi.isFile() || !fi.isReadable() )
        {
            fStatus.second = QObject::tr( "'%1' is not readable or does not exist" ).arg( fileName );
            fStatus.first = false;
            return;
        }

        QFile file( fileName );
        if ( !file.open( QFile::ReadOnly | QFile::Text ) )
        {
            fStatus.second = QObject::tr( "'%1' could not be opened for reading" ).arg( fileName );
            fStatus.first = false;
            return;
        }
        if ( progress )
        {
            progress->setRange( 0, file.size() );
            progress->setValue( file.pos() );
        }

        QTextStream ts( &file );
        QString currLine;
        SStatusInfo statusInfo;
        while ( ts.readLineInto( &currLine ) )
        {
            if ( progress )
            {
                progress->setValue( file.pos() );
                if ( progress->wasCanceled() )
                    break;

                auto labelText = statusInfo.getStatusString( fDirectories.size(), true );
                progress->setLabelText( labelText );
            }

            statusInfo.fLineNum++;
            currLine = currLine.simplified();
            if ( currLine.isEmpty() )
                continue;

            if ( loadVSCl( currLine, statusInfo.fLineNum ) )
                statusInfo.fNumCL++;
            else if ( loadGcc( currLine, statusInfo.fLineNum ) )
                statusInfo.fNumGcc++;
            else if ( loadLibrary( currLine, statusInfo.fLineNum ) )
                statusInfo.fNumLib++;
            else if ( loadLink( currLine, statusInfo.fLineNum ) )
                statusInfo.fNumLink++;
            else if ( loadManifest( currLine, statusInfo.fLineNum ) )
                statusInfo.fNumManifest++;
            else if ( loadCygwinCC( currLine, statusInfo.fLineNum ) )
                statusInfo.fNumCygwinCC++;
            else if ( loadObfuscate( currLine, statusInfo.fLineNum ) )
                statusInfo.fNumObfuscate++;
            else if ( loadMoc( currLine, statusInfo.fLineNum ) )
                statusInfo.fNumMoc++;
            else if ( loadUic( currLine, statusInfo.fLineNum ) )
                statusInfo.fNumUIC++;
            else if ( loadRcc( currLine, statusInfo.fLineNum ) )
                statusInfo.fNumRcc++;
            else
            {
                statusInfo.fNumUnloaded++;
                reportFunc( QString( "ERROR: LineNum: %1 Could not load line: %2" ).arg( statusInfo.fLineNum ).arg( currLine ) );
            }
        }
        
        if ( progress && progress->wasCanceled() )
        {
            reportFunc( QString( "Process Canceled" ) );
            fStatus = std::make_pair( false, QString( "Process Canceled" ) );
            return;
        }

        determineDependencies();
        reportFunc( "Product Dir Usages:" );
        for ( auto && ii : fProdDirUsages )
        {
            reportFunc( ii );
        }
        reportFunc( "================" );
        reportFunc( statusInfo.getStatusString( fDirectories.size(), false ) );
        fStatus = std::make_pair( true, QString() );
    }

    bool CBuildInfoData::isSourceFile( const QString & fileName ) const
    {
        static std::map< QString, bool > suffixes;
        QFileInfo fi( fileName );
        auto suffix = fi.completeSuffix().toLower();
        auto pos = suffixes.find( suffix );
        if ( pos != suffixes.end() )
            return ( *pos ).second;

        bool retVal = false;
        if ( ( suffix == "c" ) || ( suffix == "obf.c" ) || ( suffix == "sdf.c" ) )
            retVal = true;
        else if ( suffix == "cpp" )
            retVal = true;
        else if ( suffix == "cxx" )
            retVal = true;
        else if ( suffix == "h" )
            retVal = true;

        if ( fi.fileName().startsWith( "moc_" ) && suffix == "pic.cxx" )
            return true;

        suffixes[ suffix ] = retVal;
        for ( auto && ii : suffixes )
            qDebug() << ii.first << " = " << ii.second;
        return retVal;
    }

    void CBuildInfoData::cleanupProdDirUsages( QStringList & data )
    {
        for ( auto && ii : data )
        {
            QFileInfo fi( ii );
            ii = fi.path();
        }
    }

    void CBuildInfoData::determineDependencies()
    {
        for ( auto && ii : fTargets )
        {
            qDebug() << ii.second->dump();
            for ( auto && jj : ii.second->allSources() )
            {
                qDebug() << "    " << jj;
                if ( isSourceFile( jj ) )
                    continue;
                auto pos = fSources.find( jj );
                std::shared_ptr< SItem > srcItem;
                if ( pos == fSources.end() )
                {
                    pos = fTargets.find( jj );
                    if ( pos == fTargets.end() )
                    {
                        fReportFunc( QString( "ERROR: LineNumber: %1 - Could not find source item: %2" ).arg( ii.second->fLineNumber ).arg( jj ) );
                    }
                    else
                        srcItem = ( *pos ).second;
                }
                else
                    srcItem = ( *pos ).second;
                if ( srcItem )
                    ii.second->fDependencyItems.push_back( srcItem );
            }
        }

        for ( auto && ii : fSources )
        {
            auto target = ii.second->targetFile();
            auto pos = fTargets.find( target );
            std::shared_ptr< SItem > tgtItem;
            if ( pos == fTargets.end() )
            {
                pos = fSources.find( target );
                if ( pos == fSources.end() )
                    fReportFunc( QString( "ERROR: LineNumber: %1 - Could not find target item: %2" ).arg( ii.second->fLineNumber ).arg( target ) );
                else
                    tgtItem = ( *pos ).second;
            }
            else
                tgtItem = ( *pos ).second;

            ii.second->fTargetItem = tgtItem;
        }
    }

    QString CBuildInfoData::SStatusInfo::getStatusString( size_t numDirectories, bool forGUI ) const
    {
        QStringList data = QStringList()
            << QString( "Directories: %1" ).arg( numDirectories )
            << QString( "CL: %1" ).arg( fNumCL )
            << QString( "GCC: %1" ).arg( fNumGcc )
            << QString( "Libs: %1" ).arg( fNumLib )
            << QString( "Link: %1" ).arg( fNumLink )
            << QString( "Manifests: %1" ).arg( fNumManifest )
            << QString( "CygwinCC.pl: %1" ).arg( fNumCygwinCC )
            << QString( "Obfuscated: %1" ).arg( fNumObfuscate )
            << QString( "Moc: %1" ).arg( fNumMoc )
            << QString( "Uic: %1" ).arg( fNumUIC )
            << QString( "Rcc: %1" ).arg( fNumRcc )
            << QString( "Unhandled: %1" ).arg( fNumUnloaded )
            ;
        QString prefix;
        QString suffix;
        if ( forGUI )
        {
            prefix = "<br>Reading Build Output...</br><ul align=\"center\">";
            for ( auto && ii : data )
                ii = "<li>" + ii + "</li>";
            suffix = "<ul>";
        }
        else
        {
            for ( auto && ii : data )
                ii = ii + "\n";
        }
        return prefix + data.join( " " ) + suffix;
    }

    std::shared_ptr< SItem > CBuildInfoData::loadLine( QRegularExpression & regExp, const QString & line, int lineNum, std::shared_ptr< SItem > item )
    {
        QRegularExpressionMatch match;
        if ( line.indexOf( regExp, 0, &match ) != 0 )
            return nullptr;

        if ( !item )
            return nullptr;

        item->loadData( line, match.capturedLength() );
        if ( !item->status() )
        {
            fStatus = item->fStatus;
            fReportFunc( QString( "Error LineNum: %1 - %2\n" ).arg( lineNum ).arg( item->errorString() ) );
            return nullptr;
        }

        auto tmp = item->postLoadData( lineNum, fSettings->getBldTxtProdDir(), fReportFunc );
        cleanupProdDirUsages( tmp );
        fProdDirUsages.insert( tmp.begin(), tmp.end() );
        return item;
    }

    bool CBuildInfoData::loadVSCl( const QString & line, int lineNum )
    {
        auto regExp = QRegularExpression( "^.*\\/cl(.exe)?\\s+" );
        if ( line.indexOf( regExp, 0 ) != 0 )
            return false;

        auto item = std::dynamic_pointer_cast< SVSCLCompileItem >( loadLine( regExp, line, lineNum, std::make_shared< SVSCLCompileItem >( lineNum ) ) );
        if ( !item )
            return false;

        addItem( item );
        return true;
    }

    bool CBuildInfoData::loadGcc( const QString & line, int lineNum )
    {
        auto regExp = QRegularExpression( "^.*\\/(gcc|g++)(.exe)?\\s+" );
        if ( line.indexOf( regExp, 0 ) != 0 )
            return false;

        auto item = std::dynamic_pointer_cast<SGccCompileItem>( loadLine( regExp, line, lineNum, std::make_shared< SGccCompileItem >( lineNum ) ) );
        if ( !item )
            return false;

        addItem( item );
        return true;
    }

    bool CBuildInfoData::loadLibrary( const QString & line, int lineNum )
    {
        auto regExp = QRegularExpression( "^.*\\/lib(.exe)?\\s+" );
        if ( line.indexOf( regExp, 0 ) != 0 )
            return false;

        auto item = std::dynamic_pointer_cast<SLibraryItem>( loadLine( regExp, line, lineNum, std::make_shared< SLibraryItem >( lineNum ) ) );
        if ( !item )
            return false;

        addItem( item );
        return true;
    }

    bool CBuildInfoData::loadLink( const QString & line, int lineNum )
    {
        auto regExp = QRegularExpression( "^.*\\/link(.exe)?\\s+" );
        if ( line.indexOf( regExp, 0 ) != 0 )
            return false;

        auto item = std::dynamic_pointer_cast<SExecItem>( loadLine( regExp, line, lineNum, std::make_shared< SExecItem >( lineNum ) ) );
        if ( !item )
            return false;

        qDebug() << item->targetFile();
        qDebug() << item->srcDir();
        qDebug() << item->targetDir();
        qDebug() << item->dirForItem();

        addItem( item );
        return true;
    }

    bool CBuildInfoData::loadManifest( const QString & line, int lineNum )
    {
        auto regExp = QRegularExpression( "^.*\\/mt(.exe)?\\s+" );
        if ( line.indexOf( regExp, 0 ) != 0 )
            return false;

        auto item = std::dynamic_pointer_cast<SManifestItem>( loadLine( regExp, line, lineNum, std::make_shared< SManifestItem >( lineNum ) ) );
        if ( !item )
            return false;

        addItem( item );
        return true;
    }

    bool CBuildInfoData::loadCygwinCC( const QString & line, int /*lineNum*/ )
    {
        auto regExp = QRegularExpression( "^.*\\/perl(.exe)?\\s+.*cygwin_cc.pl\\s+" );
        QRegularExpressionMatch match;
        return line.indexOf( regExp, 0, &match ) == 0;
    }

    bool CBuildInfoData::loadObfuscate( const QString & line, int lineNum )
    {
        auto regExp = QRegularExpression( "^.*\\/mtiObfuscate.pl\\s+" );
        auto item = std::dynamic_pointer_cast<SObfuscatedItem>( loadLine( regExp, line, lineNum, std::make_shared< SObfuscatedItem >( lineNum ) ) );
        if ( !item )
            return false;

        addItem( item );

        return true;
    }

    bool CBuildInfoData::loadMoc( const QString & line, int /*lineNum*/ )
    {
        auto regExp = QRegularExpression( "^.*\\/moc(.exe)?\\s+" );
        return line.indexOf(regExp, 0) == 0;
    }

    bool CBuildInfoData::loadUic(const QString & line, int /*lineNum*/)
    {
        auto regExp = QRegularExpression( "^.*\\/uic(.exe)?\\s+" );
        return line.indexOf(regExp, 0) == 0;
    }

    bool CBuildInfoData::loadRcc(const QString & line, int /*lineNum*/)
    {
        auto regExp = QRegularExpression("^.*\\/rcc(.exe)?\\s+");
        return line.indexOf(regExp, 0) == 0;
    }

    void CBuildInfoData::addItem( std::shared_ptr< SItem > item )
    {
        if ( !item )
            return;

        auto dir = item->dirForItem();
        auto outDirItem = addDir( dir );
        if ( std::dynamic_pointer_cast<SVSCLCompileItem>( item ) )
            outDirItem->fVSCLCompiledFiles.push_back( std::dynamic_pointer_cast<SVSCLCompileItem>( item ) );
        if ( std::dynamic_pointer_cast<SGccCompileItem>( item ) )
            outDirItem->fGccCompiledFiles.push_back( std::dynamic_pointer_cast<SGccCompileItem>( item ) );
        else if ( std::dynamic_pointer_cast<SLibraryItem>( item ) )
            outDirItem->fLibraryItems.push_back( std::dynamic_pointer_cast<SLibraryItem>( item ) );
        else if ( std::dynamic_pointer_cast<SExecItem>( item ) )
            outDirItem->fExecutables.push_back( std::dynamic_pointer_cast<SExecItem>( item ) );
        else if ( std::dynamic_pointer_cast<SManifestItem>( item ) )
            outDirItem->fManifests.push_back( std::dynamic_pointer_cast<SManifestItem>( item ) );
        else if ( std::dynamic_pointer_cast<SObfuscatedItem>( item ) )
            outDirItem->fObfuscatedItems.push_back( std::dynamic_pointer_cast<SObfuscatedItem>( item ) );

        if ( !std::dynamic_pointer_cast<SManifestItem>( item ) )
        {
            auto tgtFile = item->targetFile();
            if ( !tgtFile.isEmpty() )
                fTargets.insert( std::make_pair( tgtFile, item ) );
        }
        if ( !std::dynamic_pointer_cast<SCompileItem>( item ) )
        {
            auto srcs = item->allSources();
            for ( auto ii : srcs )
            {
                fSources.insert( std::make_pair( ii, item ) );
            }
        }
    }

    SManifestItem::SManifestItem( int lineNum ) :
        SItem( lineNum, Qt::CaseSensitivity::CaseInsensitive )
    {
        initOptions();
    }

    bool SManifestItem::loadData( const QString & line, int pos )
    {
        return loadLine( line, pos,
                         [this]( const QString & nonOptLine )
        {
            if ( fPrevOption.compare( "manifest", Qt::CaseInsensitive ) == 0 )
            {
                auto pos = fOptions.find( "manifest" );
                if ( pos != fOptions.end() )
                {
                    auto && currValue = std::get< 2 >( (*pos).second );
                    if ( currValue.has_value() )
                    {
                        if ( std::get< 2 >( currValue.value() ).length() == 1 && std::get< 2 >( currValue.value() ).front().isEmpty() )
                            std::get< 2 >( currValue.value() ).clear();

                        std::get< 2 >( currValue.value() ) << nonOptLine;
                    }
                    else
                        currValue = std::make_tuple( false, QString(), QStringList() << nonOptLine );
                }
                else
                    std::get< 2 >( fOptions[ "manifest" ] ) = std::make_tuple( false, QString(), QStringList() << nonOptLine );
            }
        }
        );
    }

    void SManifestItem::initOptions()
    {
        fOptions =
        {
             {"manifest", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) }
            ,{"identity", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }

            ,{"rgs", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
            ,{"tlb", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
            ,{"winmd", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
            ,{"dll", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
            ,{"replacements", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }

            ,{"managedassemblyname", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
            ,{"nodependency", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            ,{"out", std::make_tuple( EOptionType::eString, true, TOptionValue() ) }
            ,{"inputresource", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
            ,{"outputresource", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
            ,{"updateresource", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
            ,{"hashupdate", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
            ,{"makecdfs", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            ,{"validate_manifest", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            ,{"validate_file_hashes", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
            ,{"canonicalize", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            ,{"check_for_duplicates", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            ,{"nologo", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
        };
    }

    QString SManifestItem::targetFile() const
    {
        QStringList paths;
        if ( !getOptionValue( paths, EOptionType::eStringList, "outputresource" ) )
            return QString();

        if ( paths.isEmpty() )
            return QString();
        auto lRetVal = paths.front();
        auto pos = lRetVal.lastIndexOf( ";" );
        if ( pos != -1 )
            lRetVal = lRetVal.left( pos );

        return lRetVal;
    }

    QStringList SManifestItem::allSources() const
    {
        QStringList paths;
        getOptionValue( paths, EOptionType::eStringList, "manifest" );

        return paths;
    }

    QStringList SManifestItem::xformProdDirInSourceAndTarget( const QString & origProdDir )
    {
        QStringList retVal;
        QStringList paths;
        if ( getOptionValue( paths, EOptionType::eStringList, "outputresource" ) )
        {
            retVal << transformProdDir( paths, origProdDir );
        }
        if ( getOptionValue( paths, EOptionType::eStringList, "manifest" ) )
        {
            retVal << transformProdDir( paths, origProdDir );
        }
        return retVal;
    }

    SObfuscatedItem::SObfuscatedItem( int lineNum ) :
        SItem( lineNum, Qt::CaseSensitivity::CaseInsensitive )
    {
        initOptions();
    }

    bool SObfuscatedItem::loadData( const QString & line, int pos )
    {
        return loadLine( line, pos,
                         [this]( const QString & nonOptLine )
        {
            if ( fPrevOption.compare( "o", Qt::CaseInsensitive ) == 0 )
            {
                auto pos = fOptions.find( "o" );
                if ( pos != fOptions.end() )
                {
                    std::get< 2 >( ( *pos ).second ) = std::make_tuple( false, nonOptLine, QStringList() );
                }
                else
                    std::get< 2 >( fOptions[ "o" ] ) = std::make_tuple( false, nonOptLine, QStringList() );
                fPrevOption.clear();
            }
            else
                fInputFile = nonOptLine;
        }
        );
    }

    void SObfuscatedItem::initOptions()
    {
        fOptions =
        {
             {"o", std::make_tuple( EOptionType::eString, false, TOptionValue() ) }
        };
    }

    QStringList SObfuscatedItem::allSources() const
    {
        return QStringList() << fInputFile;
    }

    QStringList SObfuscatedItem::xformProdDirInSourceAndTarget( const QString & origProdDir )
    {
        auto retVal = QStringList() << transformProdDir( fInputFile, origProdDir );
        return retVal;
    }

    std::shared_ptr< NVSProjectMaker::SDirItem > CBuildInfoData::addDir( const QString & dir )
    {
        auto pos = fDirectories.find( dir );
        if ( pos != fDirectories.end() )
            return ( *pos ).second;
        auto retVal = std::make_shared< SDirItem >( dir );
        fDirectories[ dir ] = retVal;
        return retVal;
    }

    SVSCLCompileItem::SVSCLCompileItem( int lineNum ) :
        SCompileItem( lineNum )
    {
        initOptions();
    }

    bool SVSCLCompileItem::loadData( const QString & line, int pos )
    {
        return loadLine( line, pos,
                  [this]( const QString & nonOptLine )
        {
            if ( fPrevOption == ">" )
            {
                auto pos = fOptions.find( "Fo" );
                if ( pos != fOptions.end() )
                    std::get< 2 >( ( *pos ).second ) = std::make_tuple( false, nonOptLine, QStringList() );
                else
                    std::get< 2 >( fOptions[ "Fo" ] ) = std::make_tuple( false, nonOptLine, QStringList() );
                fPrevOption.clear();
            }
            else
                fSourceFiles << nonOptLine;
        } );
    }

    void SVSCLCompileItem::initOptions()
    {
        fOptions =
        {
             {"O1", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } // maximum optimizations (favor space)
            ,{"O2", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //  maximum optimizations (favor speed)
            ,{"Ob", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //inline expansion (default n=0)
            ,{"Od", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,  disable optimizations (default)
            ,{"Og", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,  enable global optimization
            ,{"Oi", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } // , [-] enable intrinsic functions
            ,{"Os", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,  favor code space
            ,{"Ot", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //  favor code speed
            ,{"Ox", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //  optimizations (favor speed)
            ,{"Oy", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //  optimizations (favor speed)
            ,{"favor", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //, :<blend|AMD64|INTEL64|ATOM> select processor to optimize for, one of:
                //blend - a combination of optimizations for several different x64 processors
                //AMD64 - 64-bit AMD processors                                 
                //INTEL64 - Intel(R)64 architecture processors                  
                //ATOM - Intel(R) Atom(TM) processors                           
            
            ,{"Gu", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, [-] ensure distinct functions have distinct addresses
            ,{"Gw", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,[-] separate global variables for linker
            ,{"GF", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,  enable read-only string pooling
            ,{"Gm", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,[-] enable minimal rebuild
            ,{"Gy", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,[-] separate functions for linker
            ,{"GS", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,[-] enable security checks
            ,{"GR", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,[-] enable C++ RTTI
            ,{"GX", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,[-] enable C++ EH (same as /EHsc)
            ,{"guard:cf", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, [-] enable CFG (control flow guard)
            ,{"guard:ehcont", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,[-] enable EH continuation metadata (CET)
            ,{"EHs", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, enable C++ EH (no SEH exceptions)
            ,{"EHa", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, enable C++ EH (w/ SEH exceptions)
            ,{"EHc", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, extern "C" defaults to nothrow
            ,{"EHr", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, always generate noexcept runtime termination checks
            ,{"fp", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //,  :<except[-]|fast|precise|strict> choose floating-point model:
                //except[-] - consider floating-point exceptions when generating code
                //fast - "fast" floating-point model; results are less predictable
                //precise - "precise" floating-point model; results are predictable
                //strict - "strict" floating-point model (implies /fp:except)
            ,{"Qfast_transcendentals", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, generate inline FP intrinsics even with /fp:except
            ,{"Qspectre", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,[-] enable mitigations for CVE 2017-5753
            ,{"Qpar", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, [-] enable parallel code generation
            ,{"Qpar-report:1", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //, true,:1 auto-parallelizer diagnostic; indicate parallelized loops
            ,{"Qpar-report:2", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //, true,:1 auto-parallelizer diagnostic; indicate parallelized loops
            ,{"Qvec-report:1", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,:1 auto-vectorizer diagnostic; indicate vectorized loops
            ,{"Qvec-report:2", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,:1 auto-vectorizer diagnostic; indicate vectorized loops
            ,{"GL", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,[-] enable link-time code generation
            ,{"volatile", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //,:<iso|ms> choose volatile model:
                //iso - Acquire/release semantics not guaranteed on volatile accesses
                //ms  - Acquire/release semantics guaranteed on volatile accesses
            ,{"GA", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, optimize for Windows Application
            ,{"Ge", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, force stack checking for all funcs
            ,{"Gs", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,[num] control stack checking calls
            ,{"Gh", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, enable _penter function call
            ,{"GH", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, enable _pexit function call
            ,{"GT", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, generate fiber-safe TLS accesses
            ,{"RTC1", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, Enable fast checks (/RTCsu)
            ,{"RTCc", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, Convert to smaller type checks
            ,{"RTCs", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, Stack Frame runtime checking
            ,{"RTCu", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, Uninitialized local usage checks
            ,{"clr", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //,[:option] compile for common language runtime, where option is:
                //pure - produce IL-only output file (no native executable code)
                //safe - produce IL-only verifiable output file
                //netcore - produce assemblies targeting .NET Core runtime
                //noAssembly - do not produce an assembly
                //nostdlib - ignore the system .NET framework directory when searching for assemblies
                //nostdimport - do not import any required assemblies implicitly
                //initialAppDomain - enable initial AppDomain behavior of Visual C++ 2002
            ,{"homeparams", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, Force parameters passed in registers to be written to the stack
            ,{"GZ", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, Enable stack checks (/RTCs)
            ,{"Gv", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, __vectorcall calling convention
            ,{"arch", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,:<AVX|AVX2|AVX512> minimum CPU architecture requirements, one of:
               //AVX - enable use of instructions available with AVX-enabled CPUs
               //AVX2 - enable use of instructions available with AVX2-enabled CPUs
               //AVX512 - enable use of instructions available with AVX-512-enabled CPUs
            ,{"QIntel-jcc-erratum", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, enable mitigations for Intel JCC erratum
            ,{"Qspectre-load Enable", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, spectre mitigations for all instructions which load memory
            ,{"Qspectre-load-cf Enable", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, spectre mitigations for all control-flow instructions which load memory
            
            ,{"Fa", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,[file] name assembly listing file
            ,{"FA", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,[scu] configure assembly listing
            ,{"Fd", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,[file] name .PDB file
            ,{"Fe", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,<file> name executable file
            ,{"Fm", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,[file] name map file
            ,{"Fo", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,<file> name object file
            ,{"Fp", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,<file> name precompiled header file
            ,{"Fr", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,[file] name source browser file
            ,{"FR", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,[file] name extended .SBR file
            ,{"Fi", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,[file] name preprocessed file
            ,{"Fd", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //,: <file> name .PDB file
            ,{"Ft", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //,<dir> location of the header files generated for #import
            ,{"doc", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //, [file] process XML documentation comments and optionally name the .xdc file
            
            ,{"AI", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } //,<dir> add to assembly search path
            ,{"FU", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } //,<file> forced using assembly/module
            ,{"C", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, don't strip comments                 
            ,{"D", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } //,<name>,{=|#}<text> define macro
            ,{"E", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, preprocess to stdout
            ,{"EP", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, preprocess to stdout, no #line
            ,{"P", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //, preprocess to file
            ,{"Fx", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, merge injected code to file
            ,{"FI", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } //,<file> name forced include file
            ,{"U", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } //,<name> remove predefined macro
            ,{"u", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, remove all predefined macros
            ,{"I", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } //,<dir> add to include search path
            ,{"X", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, ignore "standard places"
            ,{"PH", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, generate #pragma file_hash when preprocessing
            ,{"PD", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, print all macro definitions
            ,{"std", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //, ,:<c++14|c++17|c++latest> C++ standard version
                //c++14 - ISO/IEC 14882:2014 (default)
                //c++17 - ISO/IEC 14882:2017
                //c++latest - latest draft standard (feature set subject to change)
            ,{"permissive", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, ,[-] enable some nonconforming code to compile (feature set subject to change) (on by default)
            ,{"Ze", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, , enable extensions (default)
            ,{"Za", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, , disable extensions
            ,{"ZW", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, , enable WinRT language extensions
            ,{"Zs", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, , syntax check only
            ,{"Zc", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //, ,:arg1[,arg2] C++ language conformance, where arguments can be:
              //forScope[-]           enforce Standard C++ for scoping rules
              //wchar_t[-]            wchar_t is the native type, not a typedef
              //auto[-]               enforce the new Standard C++ meaning for auto
              //trigraphs[-]          enable trigraphs (off by default)
              //rvalueCast[-]         enforce Standard C++ explicit type conversion rules
              //strictStrings[-]      disable string-literal to [char|wchar_t]*
              //                      conversion (off by default)
              //implicitNoexcept[-]   enable implicit noexcept on required functions
              //threadSafeInit[-]     enable thread-safe local static initialization
              //inline[-]             remove unreferenced function or data if it is
              //                      COMDAT or has internal linkage only (off by default)
              //sizedDealloc[-]       enable C++14 global sized deallocation
              //                      functions (on by default)
              //throwingNew[-]        assume operator new throws on failure (off by default)
              //referenceBinding[-]   a temporary will not bind to an non-const
              //                      lvalue reference (off by default)
              //twoPhase-             disable two-phase name lookup
              //ternary[-]            enforce C++11 rules for conditional operator (off by default)
              //noexceptTypes[-]      enforce C++17 noexcept rules (on by default in C++17 or later)
              //alignedNew[-]         enable C++17 alignment of dynamically allocated objects (on by default)
              //hiddenFriend[-]       enforce Standard C++ hidden friend rules (implied by /permissive-)
              //externC[-]            enforce Standard C++ rules for 'extern "C"' functions (implied by /permissive-)
              //lambda[-]             better lambda support by using the newer lambda processor (off by default)
              //tlsGuards[-]          generate runtime checks for TLS variable initialization (on by default)
            ,{"await", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } //, , enable resumable functions extension
            ,{"constexpr:depth", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //, ,:depth<N>     recursion depth limit for constexpr evaluation (default: 512)
            ,{"constexpr:backtrace", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //, :backtrace<N> show N constexpr evaluations in diagnostics (default: 10)
            ,{"constexpr:steps", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //, :steps<N>     terminate constexpr evaluation after N steps (default: 100000)
            ,{"Zi", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, enable debugging information
            ,{"Z7", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, enable old-style debug info
            ,{"Zo", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,[-] generate richer debugging information for optimized code (on by default)
            ,{"ZH", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //, :[MD5|SHA1|SHA_256] hash algorithm for calculation of file checksum in debug info (default: MD5)
            ,{"Zp", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //, [n] pack structs on n-byte boundary
            ,{"Zl", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, omit default library name in .OBJ
            ,{"vd", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,,{0|1|2} disable/enable vtordisp
            ,{"vm", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,<x> type of pointers to members
            ,{"std", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //:<c11|c17> C standard version
                //c11 - ISO/IEC 9899:2011
                //c17 - ISO/IEC 9899:2018
            , { "ZI", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, enable Edit and Continue debug info
            , { "openmp", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //enable OpenMP 2.0 language extensions
            ,{"openmp:experimental", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,:experimental enable OpenMP 2.0 language extensions plus select OpenMP 3.0+ language extensions
            ,{"@", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } //,<file> options response file           
            ,{"bigobj", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, generate extended object format
            ,{"c", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, compile only, no link
            ,{"errorReport", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //,:option deprecated. Report internal compiler errors to Microsoft
                //none - do not send report                
                //prompt - prompt to immediately send report
                //queue - at next admin logon, prompt to send report (default)
                //send - send report automatically         
            ,{"FC", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, use full pathnames in diagnostics
            ,{"H", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //, false,<num> max external name length
            ,{"J", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, default char type is unsigned
            ,{"MP", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,[n] use up to 'n' processes for compilation
            ,{"nologo", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, suppress copyright message
            ,{"showIncludes", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, show include file names
            ,{"Tc", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } //,<source file> compile file as .c
            ,{"Tp", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } //,<source file> compile file as .cpp
            ,{"TC", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, compile all files as .c
            ,{"TP", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, compile all files as .cpp
            ,{"V", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,<string> set version string
            ,{"Yc", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //,[file] create .PCH file
            ,{"Yd", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, put debug info in every .OBJ
            ,{"Yl", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } //,[sym] inject .PCH ref for debug lib
            ,{"Yu", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } //,[file] use .PCH file
            ,{"Y", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, disable all PCH options
            ,{"Zm", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } //, <n> max memory alloc (% of default)  
            ,{"FS", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,  force to use MSPDBSRV.EXE
            ,{"source-charset", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //, :<iana-name>|.nnnn set source character set
            ,{"execution-charset", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //, :<iana-name>|.nnnn set execution character set
            ,{"utf-8", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,  set source and execution character set to UTF-8
            ,{"validate-charset", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, [-] validate UTF-8 files for only legal characters
            ,{"LD",std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } // Create .DLL                         
            ,{"LDd", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } // Create .DLL debug library
            ,{"LN",  std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } // Create a .netmodule
            ,{"F", std::make_tuple( EOptionType::eString, false, TOptionValue() ) } // <num> set stack size
            ,{"link",std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } // [linker options and libraries]
            ,{"MD", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //  link with MSVCRT.LIB
            ,{"MT", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //  link with LIBCMT.LIB
            ,{"MDd", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //  link with MSVCRTD.LIB debug lib
            ,{"MTd", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //  link with LIBCMTD.LIB debug lib

            ,{"fastfail", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,[-] enable fast-fail mode
            ,{"JMC", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,[-] enable native just my code
            ,{"presetPadding", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,[-] zero initialize padding for stack based class types

            ,{"diagnostics", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //, :<args,...> controls the format of diagnostic messages:
                         //classic   - retains prior format
                         //column[-] - prints column information
                         //caret[-]  - prints column and the indicated line of source
            ,{"Wall", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //, enable all warnings
            ,{"w", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) } //,    disable all warnings
            ,{"W", std::make_tuple( EOptionType::eStringList, false, TOptionValue() ) } //, <n> set warning level (default n=1)
            ,{"Wv", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //, :xx[.yy[.zzzzz]] disable warnings introduced after version xx.yy.zzzzz
            ,{"WX", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //,  treat warnings as errors
            ,{"WL", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //,  enable one line diagnostics
            ,{"wd", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //, <n> disable warning n
            ,{"we", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //, <n> treat warning n as an error
            ,{"wo", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //, <n> issue warning n once
            ,{"w", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //, <l><n> set warning level 1-4 for n
            ,{"external:I", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //, I <path>      - location of external headers
            ,{"external:env", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //,      - environment variable with locations of external headers
            ,{"external:anglebrackets", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //,  - treat all headers included via <> as external
            ,{"external:W", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //,<n>          - warning level for external headers
            ,{"external:templates", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //,[-]  - evaluate warning level across template instantiation chain
            ,{"sdl", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //, enable additional security features and warnings
        };
    }

    QStringList SCompileItem::allSources() const
    {
        return fSourceFiles;
    }

    QStringList SCompileItem::xformProdDirInSourceAndTarget( const QString & origProdDir )
    {
        return transformProdDir( fSourceFiles, origProdDir );
    }

    SGccCompileItem::SGccCompileItem( int lineNum ) :
        SCompileItem( lineNum )
    {
        initOptions();
    }

    bool SGccCompileItem::loadData( const QString & line, int pos )
    {
        return loadLine( line, pos,
                         [this]( const QString & nonOptLine )
        {
            if ( fPrevOption.compare( "o", Qt::CaseInsensitive ) == 0 )
            {
                auto pos = fOptions.find( "o" );
                if ( pos != fOptions.end() )
                    std::get< 2 >( ( *pos ).second ) = std::make_tuple( false, nonOptLine, QStringList() );
                else
                    std::get< 2 >( fOptions[ "o" ] ) = std::make_tuple( false, nonOptLine, QStringList() );
                fPrevOption.clear();
            }
            else
                fSourceFiles << nonOptLine;
        } );
    }

    void SGccCompileItem::initOptions()
    {
        fOptions =
        {
             {"c", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) }
            ,{"o", std::make_tuple( EOptionType::eString, false, TOptionValue() ) }
            ,{"g", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) }
            ,{"Wall", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) }
            ,{"f", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) }
            ,{"msse2", std::make_tuple( EOptionType::eBool, false, TOptionValue() ) }
        };
    }


    SLibraryItem::SLibraryItem( int lineNum ) :
        SItem( lineNum, Qt::CaseSensitivity::CaseInsensitive )
    {
        initOptions();
    }

    bool SLibraryItem::loadData( const QString & line, int pos )
    {
        return loadLine( line, pos,
                  [this]( const QString & nonOptLine )
        {
            fInputs << nonOptLine;
        } );
    }

    void SLibraryItem::initOptions()
    {
        fOptions =
        {
            { "DEF", std::make_tuple( EOptionType::eString, true, TOptionValue() ) }
           ,{ "ERRORREPORT", std::make_tuple( EOptionType::eString, true, TOptionValue() ) }
           ,{ "EXPORT", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
           ,{ "EXTRACT", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
           ,{ "INCLUDE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
           ,{ "LIBPATH", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
           ,{ "LIST", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
           ,{ "LTCG", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
           ,{ "MACHINE", std::make_tuple( EOptionType::eString, true, TOptionValue() ) }
           ,{ "NAME", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
           ,{ "NODEFAULTLIB", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
           ,{ "NOLOGO", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
           ,{ "OUT", std::make_tuple( EOptionType::eString, true, TOptionValue() ) }
           ,{ "REMOVE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
           ,{ "SUBSYSTEM", std::make_tuple( EOptionType::eString, true, TOptionValue() ) }
           ,{ "VERBOSE", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
           ,{ "WX", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
        };
    }

    QStringList SLibraryItem::allSources() const
    {
        auto retVal = fInputs;
        QString defFile;
        if ( !getOptionValue( defFile, EOptionType::eString, "DeF" ) )
            return retVal;

        retVal << defFile;
        return retVal;
    }

    QStringList SLibraryItem::xformProdDirInSourceAndTarget( const QString & origProdDir )
    {
        return transformProdDir( fInputs, origProdDir );
    }

    SExecItem::SExecItem( int lineNum ) :
        SItem( lineNum, Qt::CaseSensitivity::CaseInsensitive )
    {
        initOptions();
    }
    
    bool SExecItem::loadData( const QString & line, int pos )
    {
        return loadLine( line, pos,
        [this]( const QString & nonOptLine )
        {
            if ( nonOptLine.startsWith( "@" ) )
                fCommandFiles << nonOptLine;
            else
                fFiles << nonOptLine;
        }
        );
    }

    void SExecItem::initOptions()
    {
        fOptions =
        {
              { "ALIGN", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
            , { "ALLOWBIND", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "ALLOWISOLATION", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "APPCONTAINER", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "ASSEMBLYDEBUG", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "ASSEMBLYLINKRESOURCE", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "ASSEMBLYMODULE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
            , { "ASSEMBLYRESOURCE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }

            , { "BASE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) }
            , { "CLRIMAGETYPE", std::make_tuple( EOptionType::eString, true, TOptionValue() ) }
            , { "CLRLOADEROPTIMIZATION", std::make_tuple( EOptionType::eString, true, TOptionValue() ) }
            , { "CLRSUPPORTLASTERROR", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //[:, {NO|SYSTEMDLL}]
            , { "CLRTHREADATTRIBUTE", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //:, {MTA|NONE|STA}
            , { "CLRRUNMANAGEDCODECHECK", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //[:NO]
            , { "DEBUG", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //[:, {FASTLINK|FULL|NONE}]
            , { "DEF", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:FILENAME
            , { "DEFAULTLIB", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:LIBRARY
            , { "DELAY", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //:, {NOBIND|UNLOAD}
            , { "DELAYLOAD", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //:DLL
            , { "DELAYSIGN", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //[:NO]
            , { "DEPENDENTLOADFLAG", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:FLAG
            , { "DLL", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "DRIVER", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //[:, {UPONLY|WDM}]
            , { "DYNAMICBASE", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //[:NO]
            , { "ENTRY", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:SYMBOL
            , { "ERRORREPORT", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:, {NONE|PROMPT|QUEUE|SEND}
            , { "EXPORT", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:SYMBOL
            , { "EXPORTADMIN", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //[:SIZE]
            , { "FASTGENPROFILE", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //[:, {COUNTER32|COUNTER64|EXACT|MEMMAX=#|MEMMIN=#|NOEXACT|
            //    ////NOPATH|NOTRACKEH|PATH|PGD=FILENAME|TRACKEH}]
            , { "FILEALIGN", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //:#
            , { "FIXED", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //[:NO]
            , { "FORCE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //[:, {MULTIPLE|UNRESOLVED}]
            , { "FUNCTIONPADMIN", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //[:SIZE]
            , { "GUARD", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //:, {CF|NO}
            , { "GENPROFILE", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //[:, {COUNTER32|COUNTER64|EXACT|MEMMAX=#|MEMMIN=#|NOEXACT|
    //    ////NOPATH|NOTRACKEH|PATH|PGD=FILENAME|TRACKEH}]
            , { "HEAP", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //:RESERVE[,COMMIT]
            , { "HIGHENTROPYVA", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //[:NO]
            , { "IDLOUT", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:FILENAME
            , { "IGNORE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:#
            , { "IGNOREIDL", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "IMPLIB", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:FILENAME
            , { "INCLUDE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:SYMBOL
            , { "INCREMENTAL", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //[:NO]
            , { "INTEGRITYCHECK", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "KERNEL", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "KEYCONTAINER", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:NAME
            , { "KEYFILE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:FILENAME
            , { "LARGEADDRESSAWARE", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //[:NO]
            , { "LIBPATH", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:DIR
            , { "LTCG", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //[:, {INCREMENTAL|NOSTATUS|OFF|STATUS|}]
            , { "MACHINE", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //:, {ARM|ARM64|EBC|X64|X86}
            , { "MANIFEST", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //[:, {EMBED[,ID=#]|NO}]
            , { "MANIFESTDEPENDENCY", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:MANIFESTDEPENDENCY
            , { "MANIFESTFILE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:FILENAME
            , { "MANIFESTINPUT", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:FILENAME
            , { "MANIFESTUAC", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //[:, {NO|UACFRAGMENT}]
            , { "MAP", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //[:FILENAME]
            , { "MAPINFO", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:, {EXPORTS}
            , { "MERGE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:FROM=TO
            , { "MIDL", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:@COMMANDFILE
            , { "NATVIS", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:FILENAME
            , { "NOASSEMBLY", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "NODEFAULTLIB", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //[:LIBRARY]
            , { "NOENTRY", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "NOIMPLIB", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "NOLOGO", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "NXCOMPAT", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //[:NO]
            , { "OPT", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:, {ICF[=ITERATIONS]|LBR|NOICF|NOLBR|NOREF|REF}
            , { "ORDER", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:@FILENAME
            , { "OUT", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //:FILENAME
            , { "PDB", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //:FILENAME
            , { "PDBSTRIPPED", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //[:FILENAME]
            , { "PROFILE", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "RELEASE", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "SAFESEH", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //[:NO]
            , { "SECTION", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:NAME,[[!], {DEKPRSW}][,ALIGN=#]
            , { "SOURCELINK", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //[:FILENAME]
            , { "STACK", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //:RESERVE[,COMMIT]
            , { "STUB", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:FILENAME
            , { "SUBSYSTEM", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //:, {BOOT_APPLICATION|CONSOLE|EFI_APPLICATION|
            //    ////EFI_BOOT_SERVICE_DRIVER|EFI_ROM|EFI_RUNTIME_DRIVER|
            //    ////NATIVE|POSIX|WINDOWS|WINDOWSCE}[,#[.##]]
            , { "SWAPRUN", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:, {CD|NET}
            , { "TLBID", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:#
            , { "TLBOUT", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:FILENAME
            , { "TIME", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) }
            , { "TSAWARE", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //[:NO]
            , { "USEPROFILE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //[:, {AGGRESSIVE|PGD=FILENAME}]
            , { "VERBOSE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //[:, {CLR|ICF|INCR|LIB|REF|SAFESEH|UNUSEDDELAYLOAD|UNUSEDLIBS}]
            , { "VERSION", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:#[.#]
            , { "WINMD", std::make_tuple( EOptionType::eString, true, TOptionValue() ) } //[:, {NO|ONLY}]
            , { "WINMDDELAYSIGN", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //[:NO]
            , { "WINMDFILE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:FILENAME
            , { "WINMDKEYCONTAINER", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:NAME
            , { "WINMDKEYFILE", std::make_tuple( EOptionType::eStringList, true, TOptionValue() ) } //:FILENAME
            , { "WHOLEARCHIVE", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //[:LIBRARY]
            , { "WX", std::make_tuple( EOptionType::eBool, true, TOptionValue() ) } //[:NO]
        };
    }

    QStringList SExecItem::allSources() const
    {
        auto retVal = QStringList() << fFiles << fCommandFiles;
        return retVal;
    }

    QStringList SExecItem::xformProdDirInSourceAndTarget( const QString & origProdDir )
    {
        auto retVal = QStringList() << transformProdDir( fFiles, origProdDir ) << transformProdDir( fCommandFiles, origProdDir );
        return retVal;
    }

    bool SItem::isTrue( const QString & value )
    {
        if ( value.isEmpty() )
            return true;
        if ( value == "0" )
            return false;
        if ( value.compare( "no", Qt::CaseInsensitive ) == 0 )
            return false;
        if ( value.compare( "false", Qt::CaseInsensitive ) == 0 )
            return false;
        if ( value.compare( "-", Qt::CaseInsensitive ) == 0 )
            return false;
        return true;
    }

    QStringList SItem::transformProdDir( QString & curr, const QString & origProdDir ) const
    {
        auto tmp = origProdDir;
        tmp = tmp.replace( "\\", "\\\\" );
        tmp = tmp.replace( "/", "\\/" );
        tmp = tmp.replace( "~", "\\~" );
        tmp = tmp.replace( ":", "\\:" );
        tmp += "[\\/\\\\]?";
        QRegularExpression regEx( tmp, QRegularExpression::CaseInsensitiveOption );
        if ( curr.contains( origProdDir ) )
        {
            curr = curr.replace( regEx, "<PRODDIR>/" );
            return QStringList() << curr;;
        }
        return {};
    }

    QStringList SItem::transformProdDir( QStringList & curr, const QString & origProdDir ) const
    {
        QStringList retVal;

        for ( auto && ii : curr )
        {
            retVal << transformProdDir( ii, origProdDir );
        }
        return retVal;
    }

    QStringList SItem::transformProdDir( TOptionTypeMap & currValues, const QString & origProdDir ) const
    {
        QStringList retVal;
        for ( auto && ii : currValues )
        {
            auto && currType = ii.second;
            switch ( std::get< 0 >( currType ) )
            {
                case EOptionType::eBool:
                continue;
                break;
                case EOptionType::eString:
                if ( std::get< 2 >( currType ).has_value() )
                    retVal << transformProdDir( std::get< 1 >( std::get< 2 >( currType ).value() ), origProdDir );
                break;
                case EOptionType::eStringList:
                if ( std::get< 2 >( currType ).has_value() )
                    retVal << transformProdDir( std::get< 2 >( std::get< 2 >( currType ).value() ), origProdDir );
                break;
            }
        }
        return retVal;
    }

    QStringList SItem::postLoadData( int lineNum, const QString & origProdDir, std::function< void( const QString & msg ) > reportFunc )
    {
        QStringList retVal = 
            transformProdDir( fOtherOptions, origProdDir ) 
            << xformProdDirInSourceAndTarget( origProdDir )
            << transformProdDir( fOptions, origProdDir )
        ;

        for ( auto && ii : fOtherOptions )
        {
            reportFunc( QString( "Warning LineNum: %1 - Unknown Options: %2\n" ).arg( lineNum ).arg( fOtherOptions.join( " " ) ) );
        }

        if ( firstSrcFile().isEmpty() )
        {
            reportFunc( QString( "Warning LineNum: %1 - Could not determine Primary Source File\n" ).arg( lineNum ) );
        }

        if ( targetFile().isEmpty() )
        {
            reportFunc( QString( "Warning LineNum: %1 - Could not determine Output File\n" ).arg( lineNum ) );
        }
        return retVal;
    }

    QString SItem::dump() const
    {
        QString retVal = QString( "%1: LineNum:%2 - %3" ).arg( getItemTypeName() ).arg( fLineNumber ).arg( targetFile() );

        return retVal;
    }

    SItem::SItem( int lineNum, Qt::CaseSensitivity cs ) :
        fLineNumber( lineNum ),
        fOptions( ( QStringCmp( cs ) ) )
    {

    }

    bool SItem::loadLine( const QString & line, int pos, std::function< void( const QString & noOpt ) > noOptFunc )
    {
        fPrevOption.clear();

        auto prevPos = pos;
        pos = line.indexOf( " ", prevPos + 1 );
        while ( ( prevPos != -1 ) && ( prevPos < line.length() ) )
        {
            auto currOption = line.mid( prevPos, ( pos == -1 ) ? -1 : ( pos - prevPos ) );
            bool isOpt = currOption.startsWith( "-" ) || currOption.startsWith( "/" );
            if ( isOpt )
            {
                auto origOption = currOption;
                currOption = currOption.mid( 1 );
                fPrevOption = currOption;
                auto colonPos = currOption.indexOf( ':' );
                auto remainder = ( colonPos != -1 ) ? currOption.mid( colonPos + 1 ) : QString();
                currOption = ( colonPos != -1 ) ? currOption.left( colonPos ) : currOption;
                
                auto pos = fOptions.find( currOption );
                if ( pos == fOptions.end() )
                {
                    std::list< std::pair< QString, TOptionType > > possibleOptions;

                    for ( auto && ii = fOptions.begin(); ii != fOptions.end(); ++ii )
                    {
                        if ( std::get< 1 >( ( *ii ).second ) ) // colon required dont look here
                            continue;
                        if ( currOption.startsWith( ( *ii ).first, caseInsensitiveOptions() ) )
                        {
                            possibleOptions.push_back( *ii );
                        }
                    }

                    if ( possibleOptions.empty() )
                    {
                        fOtherOptions << origOption;
                        continue;
                    }

                    std::pair< QString, TOptionType > bestOption;
                    int max = 0;
                    for ( auto && ii : possibleOptions )
                    {
                        if ( ii.first.length() > max )
                        {
                            bestOption = ii;
                            max = ii.first.length();
                        }
                    }

                    pos = fOptions.find( bestOption.first );
                    remainder = currOption.mid( bestOption.first.length() );
                    currOption = bestOption.first;
                }

                if ( pos != fOptions.end() )
                {
                    switch ( std::get< 0 >( ( *pos ).second ) )
                    {
                        case EOptionType::eBool:
                        {
                            auto newValue = isTrue( remainder );
                            if ( std::get< 2 >( ( *pos ).second ).has_value() )
                            {
                                auto existingValue = std::get< 0 >( std::get< 2 >( ( *pos ).second ).value() );
                                if ( newValue != existingValue )
                                {
                                    fStatus = std::make_pair( false, QString( "Option: %1 already set to %2" ).arg( currOption ).arg( existingValue ? "True" : "False" ) );
                                    return false;
                                }
                            }
                            else
                                std::get< 2 >( ( *pos ).second ) = std::make_tuple( isTrue( remainder ), QString(), QStringList() );
                        }
                        break;
                        case EOptionType::eString:
                        {
                            auto newValue = remainder;
                            if ( std::get< 2 >( ( *pos ).second ).has_value() )
                            {
                                auto existingValue = std::get< 1 >( std::get< 2 >( ( *pos ).second ).value() );
                                if ( newValue != existingValue )
                                {
                                    fStatus = std::make_pair( false, QString( "Option: %1 already set to %2" ).arg( currOption ).arg( existingValue ) );
                                    return false;
                                }
                            }
                            else
                                std::get< 2 >( ( *pos ).second ) = std::make_tuple( false, remainder, QStringList() );
                        }
                        break;
                        case EOptionType::eStringList:
                        {
                            if ( std::get< 2 >( ( *pos ).second ).has_value() )
                            {
                                std::get< 2 >( std::get< 2 >( ( *pos ).second ).value() ) << remainder;
                            }
                            else
                            {
                                std::get< 2 >( ( *pos ).second ) = std::make_tuple( false, QString(), QStringList() << remainder );
                            }
                        }
                        break;

                    }
                }
                else
                    fOtherOptions << origOption;
            }
            else if ( currOption == ">" )
            {
                fPrevOption = ">";
            }
            else
                noOptFunc( currOption );

            if ( pos == -1 )
                break;

            prevPos = pos + 1;
            pos = line.indexOf( " ", prevPos );
        }

        fStatus = std::make_pair( true, QString() );
        return true;
    }

    QString SItem::targetFile() const
    {
        auto pos = fOptions.find( targetFileOption() );
        if ( pos == fOptions.end() )
            return QString();
        if ( !std::get< 2 >( ( *pos ).second ).has_value() )
            return QString();
        return std::get< 1 >( std::get< 2 >( ( *pos ).second ).value() );
    }

    QString SItem::targetDir() const
    {
        auto path = targetFile();
        if ( path.isEmpty() )
            return QString();

        auto fi = QFileInfo( path );
        auto dir = fi.path();
        return dir;
    }

    QString SItem::firstSrcFile() const
    {
        auto srcs = allSources();
        if ( !srcs.isEmpty() )
            return srcs.front();
        return QString();
    }

    QString SItem::srcDir() const
    {
        auto path = firstSrcFile();
        if ( path.isEmpty() )
            return QString();

        auto fi = QFileInfo( path );
        auto dir = fi.path();
        return dir;
    }

    QString SItem::dirForItem() const
    {
        QString srcDir = this->srcDir();
        QString targetDir = this->targetDir();

        if ( srcPriorityForDir() )
        {
            if ( !srcDir.isEmpty() )
                return srcDir;
            return targetDir;
        }
        else
        {
            if ( !targetDir.isEmpty() )
                return targetDir;
            return srcDir;
        }
    }

    void SItem::loadIntoTree( QStandardItem * parent  )
    {
        QList< QStandardItem * > currRow;
        currRow << new QStandardItem( QString() );
        currRow << new QStandardItem( targetDir() );
        currRow << new QStandardItem( getItemTypeName() );
        currRow << new QStandardItem( firstSrcFile() );
        currRow << new QStandardItem( targetFile() );
        parent->appendRow( currRow );
        if ( !targetFile().isEmpty() )
        {
            auto folder = new QStandardItem( "Target" );
            currRow.front()->appendRow( QList< QStandardItem * >() << folder );
            auto tgtItem = new QStandardItem( targetFile() );
            folder->appendRow( QList< QStandardItem * >() << tgtItem );
        }

        auto allSources = this->allSources();
        if ( !allSources.isEmpty() )
        {
            auto folder = new QStandardItem( "Dependencies" );
            currRow.front()->appendRow( QList< QStandardItem * >() << folder );
            for ( auto && ii : allSources )
            {
                auto item = new QStandardItem( ii );
                folder->appendRow( QList< QStandardItem * >() << item );
            }
        }
    }

    SDirItem::SDirItem( const QString & dirName ) :
        fDir( dirName )
    {

    }

    QList< QStandardItem * > SDirItem::loadIntoTree( std::function< void( const QString & msg ) > reportFunc )
    {
        QList< QStandardItem * > retVal;
        auto dir = new QStandardItem( fDir );
        retVal << dir;

        for ( auto && ii : fVSCLCompiledFiles )
        {
            ii->loadIntoTree( dir );
        }
        for ( auto && ii : fLibraryItems )
        {
            ii->loadIntoTree( dir );
        }
        for ( auto && ii : fExecutables )
        {
            ii->loadIntoTree( dir );
        }
        for ( auto && ii : fManifests )
        {
            ii->loadIntoTree( dir );
        }
        return retVal;
    }

    void CBuildInfoData::loadIntoTree( QStandardItemModel * model )
    {
        if ( !model )
            return;
        model->clear();
        model->setHorizontalHeaderLabels( QStringList() << "Source Directory" << "Target Directory" << "Type" << "Primary Input File" << "Output File" );
        if ( !fStatus.first )
            return;

        for ( auto && ii : fDirectories )
        {
            auto items = ii.second->loadIntoTree( fReportFunc );
            model->appendRow( items );
        }
    }

}