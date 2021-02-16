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

#ifndef __BUILDINFODATA_H
#define __BUILDINFODATA_H
#include <QString>
#include <QStringList>
#include <map>
namespace NVSProjectMaker
{
    struct SCompileItem
    {
        SCompileItem( const QString & line, int pos );

        QString outDir() const;
        QString fSourceFile;
        QString fOutputFile;
        QStringList fDefines;
        QStringList fIncludeDirs;
        QStringList fOtherOptions;
    };

    struct SLibraryItem
    {
        SLibraryItem( const QString & line, int pos );
        QString outDir() const;

        QStringList fDefFiles;
        QStringList fErrorReports;
        QStringList fExports;
        QStringList fExtracts;
        QStringList fIncludes;
        QStringList fLibPaths;
        QStringList fLists;
        bool fLTCG{ false };
        QString fMachine;
        QStringList fNameFiles;
        QStringList fNoDefaultLibs;
        bool fNoLogo{ false };
        QString fOutputFile;
        QStringList fRemoveMembers;
        QStringList fSubSystems;
        bool fVerbose{ false };
        bool fWX{ false };

        QStringList fInputs;
    };

    struct SExecItem
    {
        SExecItem( const QString & line );
    };


    struct SDirItem
    {
        SDirItem( const QString & dirName );
        QString fDir;
        std::list< std::shared_ptr< SCompileItem > > fCompiledFiles;
        std::list< std::shared_ptr< SLibraryItem > > fLibraryItems;
        std::list< std::shared_ptr< SExecItem > > fExecutables;  // includes MT data as well
    };

    class CBuildInfoData
    {
    public:
        CBuildInfoData( const QString & fileName );
        bool status() const { return fStatus.first; }
        QString errorString() const { return fStatus.second; }
    private:
        bool loadCompile( const QString & line );
        bool loadLibrary( const QString & line );
        bool loadLink( const QString & line );
        bool loadMT( const QString & line );

        std::shared_ptr< SDirItem > addOutDir( const QString & dir );

        std::map< QString, std::shared_ptr< SDirItem > > fDirectories;
        std::pair< bool, QString > fStatus = std::make_pair( false, QString() );;
    };
}

#endif 
