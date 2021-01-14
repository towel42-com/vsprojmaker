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

#include <QString>
#include <QPair>
#include <QList>
#include <QStringList>
#include <QMap>
class QWidget;
class QTextStream;

namespace NVSProjectMaker
{
    struct SDebugTarget;
    struct SSourceFileInfo;
    struct SDirInfo
    {
        SDirInfo() {}
        SDirInfo( const std::shared_ptr< SSourceFileInfo > & fileInfo );
        bool isValid() const;
        void writeCMakeFile( QWidget * parent, const QString & bldDir ) const;
        void writePropSheet( QWidget * parent, const QString & srcDir, const QString & bldDir, const QString & includeDirs ) const;
        void createDebugProjects( QWidget * parent, const QString & bldDir ) const;

        QStringList getSubDirs() const;
        void replaceFiles( QString & text, const QString & variable, const QStringList & files ) const;
        void addDependencies( QTextStream & qts ) const;
        void computeRelToDir( const std::shared_ptr< SSourceFileInfo > & fileInfo );

        QString fRelToDir;
        QString fProjectName;
        bool fIsInclDir{ false };
        bool fIsBuildDir{ false };
        QList< QPair< QString, bool > > fExecutables;

        QStringList fSourceFiles;
        QStringList fHeaderFiles;
        QStringList fUIFiles;
        QStringList fQRCFiles;
        QStringList fOtherFiles;

        QStringList fExtraTargets;
        QList<SDebugTarget> fDebugCommands;

        void getFiles( const std::shared_ptr< SSourceFileInfo > & fileInfo );
        void addFile( const QString & path );
    };

}
#endif 

