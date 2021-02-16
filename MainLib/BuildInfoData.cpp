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

#include <QObject>
//#include <QApplication>
//#include <QStandardItem>
//#include <QString>
//#include <QStringList>
//#include <QList>
//#include <QSet>
//#include <QSettings>
#include <QFileInfo>
//#include <QDir>
//#include <QDirIterator>
//#include <QProgressDialog>
//#include <QMessageBox>
#include <QRegularExpression>
#include <QTextStream>
//#include <QProcess>
#include <QDebug>

namespace NVSProjectMaker
{
    CBuildInfoData::CBuildInfoData( const QString & fileName )
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

        QTextStream ts( &file );
        QString currLine;
        int numLibLines = 0;
        int numClLines = 0;
        int numLinkLines = 0;
        int numMTLines = 0;
        int lineNum = 0;
        while ( ts.readLineInto( &currLine ) )
        {
            lineNum++;
            currLine = currLine.simplified();
            if ( currLine.isEmpty() )
                continue;

            if ( loadCompile( currLine ) )
                numClLines++;
            else if ( loadLibrary( currLine ) )
                numLibLines++;
            else if ( loadLink( currLine ) )
                numLinkLines++;
            else if ( loadMT( currLine ) )
                numMTLines++;
            else
                int xyz = 0;
            qDebug() << "LineNum: " << lineNum << " Line: " << currLine;
        }

        qDebug() << "Num Directories: " << fDirectories.size()
            << " Num CL: " << numClLines
            << " Num Libs: " << numLibLines
            << " Num Link: " << numLinkLines
            << " Num MT: " << numMTLines
            ;
        fStatus = std::make_pair( true, QString() );
    }

    bool CBuildInfoData::loadCompile( const QString & line )
    {
        auto regExp = QRegularExpression( "C\\:\\/.*\\/cl\\s+" );

        QRegularExpressionMatch match;
        if ( line.indexOf( regExp, 0, &match ) != 0 )
            return false;

        auto item = std::make_shared< SCompileItem >( line, match.capturedLength() );
        auto outDir = item->outDir();

        auto outDirItem = addOutDir( outDir );
        outDirItem->fCompiledFiles.push_back( item );

        return true;
    }

    bool CBuildInfoData::loadLibrary( const QString & line )
    {
        auto regExp = QRegularExpression( "C\\:\\/.*\\/lib\\s+" );

        QRegularExpressionMatch match;
        if ( line.indexOf( regExp, 0, &match ) != 0 )
            return false;

        auto item = std::make_shared< SLibraryItem >( line, match.capturedLength() );
        auto outDir = item->outDir();

        auto outDirItem = addOutDir( outDir );
        outDirItem->fLibraryItems.push_back( item );

        return true;
    }

    bool CBuildInfoData::loadLink( const QString & line )
    {
        auto regExp = QRegularExpression( "C\\:\\/.*\\/link\\s+" );

        if ( line.indexOf( regExp ) != 0 )
            return false;

        return true;
    }

    bool CBuildInfoData::loadMT( const QString & line )
    {
        auto regExp = QRegularExpression( "C\\:\\/.*\\/mt\\s+" );

        if ( line.indexOf( regExp ) != 0 )
            return false;

        return true;
    }

    std::shared_ptr< NVSProjectMaker::SDirItem > CBuildInfoData::addOutDir( const QString & dir )
    {
        auto pos = fDirectories.find( dir );
        if ( pos != fDirectories.end() )
            return ( *pos ).second;
        auto retVal = std::make_shared< SDirItem >( dir );
        fDirectories[ dir ] = retVal;
        return retVal;
    }

    SCompileItem::SCompileItem( const QString & line, int pos )
    {
        auto prevPos = pos;
        pos = line.indexOf( " ", prevPos + 1 );
        while ( ( prevPos != -1 ) && ( prevPos < line.length() ) )
        {
            auto currOption = line.mid( prevPos, ( pos == -1 ) ? -1 : ( pos - prevPos ) );
            if ( currOption.startsWith( "-D", Qt::CaseInsensitive ) || currOption.startsWith( "/D", Qt::CaseInsensitive ) )
                fDefines << currOption.mid( 2 );
            else if ( currOption.startsWith( "/Fo", Qt::CaseInsensitive ) )
                fOutputFile = currOption.mid( 3 );
            else if ( currOption.startsWith( "/I", Qt::CaseInsensitive ) || currOption.startsWith( "-I", Qt::CaseInsensitive ) )
                fIncludeDirs << currOption.mid( 2 );
            else if ( currOption.startsWith( "/" ) || currOption.startsWith( "-" ) )
                fOtherOptions << currOption;
            else
                fSourceFile = currOption;

            if ( pos == -1 )
                break;

            prevPos = pos + 1;
            pos = line.indexOf( " ", prevPos );
        }
    }

    QString SCompileItem::outDir() const
    {
        auto fi = QFileInfo( fOutputFile );
        auto path = fi.path();
        return path;
    }

    SDirItem::SDirItem( const QString & dirName ) :
        fDir( dirName )
    {

    }

    bool isTrue( const QString & value )
    {
        if ( value.isEmpty() )
            return true;
        if ( value == "0" )
            return false;
        if ( value.compare( "no", Qt::CaseInsensitive ) == 0 )
            return false;
        if ( value.compare( "false", Qt::CaseInsensitive ) == 0 )
            return false;

        return true;
    }

    SLibraryItem::SLibraryItem( const QString & line, int pos )
    {
        auto prevPos = pos;
        pos = line.indexOf( " ", prevPos + 1 );
        while ( ( prevPos != -1 ) && ( prevPos < line.length() ) )
        {
            auto currOption = line.mid( prevPos, ( pos == -1 ) ? -1 : ( pos - prevPos ) );
            bool isOpt = currOption.startsWith( "-" ) || currOption.startsWith( "/" );
            if ( isOpt )
            {
                currOption = currOption.mid( 1 );
                auto colonPos = currOption.indexOf( ':' );
                auto remainder = ( colonPos != -1 ) ? currOption.mid( colonPos + 1 ) : QString();
                if ( currOption.startsWith( "def", Qt::CaseInsensitive ) )
                    fDefFiles << remainder;
                else if ( currOption.startsWith( "errorreport:", Qt::CaseInsensitive ) )
                    fErrorReports << remainder;
                else if ( currOption.startsWith( "export:", Qt::CaseInsensitive ) )
                    fExports << remainder;
                else if ( currOption.startsWith( "extract:", Qt::CaseInsensitive ) )
                    fExtracts << remainder;
                else if ( currOption.startsWith( "include:", Qt::CaseInsensitive ) )
                    fIncludes << remainder;
                else if ( currOption.startsWith( "libpath:", Qt::CaseInsensitive ) )
                    fLibPaths << remainder;
                else if ( currOption.startsWith( "list", Qt::CaseInsensitive ) )
                    fLists << remainder;
                else if ( currOption.startsWith( "ltcg", Qt::CaseInsensitive ) )
                    fLTCG = isTrue( remainder );
                else if ( currOption.startsWith( "machine:", Qt::CaseInsensitive ) )
                    fMachine = remainder;
                else if ( currOption.startsWith( "name:", Qt::CaseInsensitive ) )
                    fNameFiles << remainder;
                else if ( currOption.startsWith( "nodefaultlib:", Qt::CaseInsensitive ) )
                    fNoDefaultLibs << remainder;
                else if ( currOption.startsWith( "nologo", Qt::CaseInsensitive ) )
                    fNoLogo = isTrue( remainder );
                else if ( currOption.startsWith( "out:", Qt::CaseInsensitive ) )
                    fOutputFile = remainder;
                else if ( currOption.startsWith( "remove:", Qt::CaseInsensitive ) )
                    fRemoveMembers << remainder;
                else if ( currOption.startsWith( "subsystem:", Qt::CaseInsensitive ) )
                    fSubSystems << remainder;
                else if ( currOption.startsWith( "verbose", Qt::CaseInsensitive ) )
                    fVerbose = isTrue( remainder );
                else if ( currOption.startsWith( "wx", Qt::CaseInsensitive ) )
                    fWX = isTrue( remainder );
                else
                    int xyz = 0;
            }
            else
                fInputs << currOption;

            if ( pos == -1 )
                break;

            prevPos = pos + 1;
            pos = line.indexOf( " ", prevPos );
        }
    }

    QString SLibraryItem::outDir() const
    {
        auto fi = QFileInfo( fOutputFile );
        auto path = fi.path();
        return path;
    }

}