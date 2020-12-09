// The MIT License( MIT )
//
// Copyright( c ) 2020 Scott Aron Bloom
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

#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include <QDialog>
#include <QDir>
#include <memory>
#include <optional>
#include <QHash>
#include <tuple>
#include <QSet>
#include <functional>

class QTextStream;
class QStandardItemModel;
class QStandardItem;
class QStringListModel;
class QProgressDialog;

namespace Ui {class CMainWindow;};

struct SDirInfo
{
    SDirInfo() {}
    SDirInfo( QStandardItem * item );
    bool isValid() const;
    void writeCMakeFile( const QString & bldDir ) const;

    void addDependencies( QTextStream & qts ) const;
    void replaceFiles( QString & text, const QString & variable, const QStringList & files ) const;
    void computeRelToDir( QStandardItem * item );

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

    void getFiles( QStandardItem * parent );
    void addFile( const QString & path );
};

class CMainWindow : public QDialog
{
    Q_OBJECT
public:
    enum ERoles
    {
        eIsDirRole = Qt::UserRole + 1,
        eIsBuildDir,
        eExecutables,
        eIsIncludeDir,
        eRelPath
    };

    CMainWindow(QWidget *parent = 0);
    ~CMainWindow();

    static QString readResourceFile( QWidget * parent, const QString & resourceFile, const std::function< void( QString & data ) > & function = {} );

public Q_SLOTS:
    void slotChanged();
    void slotCMakeChanged();

    void slotLoadSource();
    void slotSelectCMake();
    void slotSelectSourceDir();
    void slotSelectDestDir();

    void slotGenerate();
private:
    void loadSettings();
    void saveSettings();
    struct SSourceFileInfo
    {
        int fDirs{ 0 };
        int fFiles{ 0 };
        int fBuildDirs{ 0 };
        int fInclDirs{ 0 };
        int fExecutables{ 0 };
        QString getText() const;
    };
    std::list< SDirInfo > getDirInfo( QStandardItem * parent, QProgressDialog * progress ) const;

    bool loadSourceFiles( const QString & dir, QStandardItem * parent, QProgressDialog * progress, SSourceFileInfo & results );
    void incProgress( QProgressDialog * progress ) const;
    bool isBuildDir( const QDir & dir ) const;
    bool isInclDir( const QDir & dir ) const;
    QList< QPair< QString, bool > > getExecutables( const QDir & dir ) const;
    std::tuple< QSet< QString >, QSet< QString >, QHash< QString, QList< QPair< QString, bool > > > > findDirAttributes( QStandardItem * parent ) const;

    QSet< QString > fBuildDirs;
    QSet< QString > fInclDirs;
    QHash< QString, QList< QPair< QString, bool > > > fExecutables;

    std::optional< QDir > fSourceDir;
    QStandardItemModel * fSourceModel{ nullptr };
    std::unique_ptr< Ui::CMainWindow > fImpl;
};

#endif // _ALCULATOR_H
