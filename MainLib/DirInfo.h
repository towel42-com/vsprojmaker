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

#ifndef _DIRINFO_H
#define _DIRINFO_H

#include <unordered_map>
#include <QString>
#include <QStringList>
#include <memory>

class QWidget;
class QTextStream;

namespace NVSProjectMaker
{
    class CSettings;
    struct SDebugTarget;
    struct SSourceFileInfo;
    struct SDirInfo
    {
        SDirInfo() {}
        SDirInfo( const std::shared_ptr< SSourceFileInfo > & fileInfo );
        bool isValid() const;
        void writeCMakeFile( QWidget * parent, const CSettings * settings ) const;
        void writePropSheet( QWidget * parent, const CSettings * settings ) const;

        QString getPreprocessorDefines( const CSettings * settings ) const;
        QString getInclDirs( const CSettings * settings ) const;

        void createDebugProjects( QWidget * parent, const CSettings * settings ) const;

        std::list< std::shared_ptr< NVSProjectMaker::SDirInfo > > findPairDirs( const std::unordered_map < QString, std::shared_ptr< NVSProjectMaker::SDirInfo > > & map ) const;

        QStringList getSubDirs() const;
        void replaceFiles( QString & text, const QString & variable, const QStringList & files ) const;
        void addDependencies( QTextStream & qts ) const;
        void computeRelToDir( const std::shared_ptr< SSourceFileInfo > & fileInfo );
        bool isBuildFile( const QString & path ) const;

        static QString getBuildItShellCmd( const QString & buildItFile );

        QString getSrcRelPath() const;
        QString getInclRelPath() const;

        QString fRelToDir;
        bool fIsPairedDir{ false };
        QString fProjectName;
        bool fIsInclDir{ false };
        bool fIsBuildDir{ false };
        std::list< std::pair< QString, bool > > fExecutables;

        QStringList fSourceFiles;
        QStringList fHeaderFiles;
        QStringList fYAMLFiles;
        QStringList fUIFiles;
        QStringList fQRCFiles;
        QStringList fBuildFiles;
        QStringList fOtherFiles;

        QStringList fExtraTargets;
        std::list<SDebugTarget> fDebugCommands;

        void getFiles( const std::shared_ptr< SSourceFileInfo > & fileInfo );
        void addFile( const QString & path );
    };

}
#endif 

