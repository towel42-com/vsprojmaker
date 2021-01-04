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
#include <tuple>
#include <QProcess>
#include <optional>

class CMainWindow;
class QTextStream;
class QStandardItemModel;
class QStandardItem;
class QStringListModel;
class QProgressDialog;
class QProcess;
class CCheckableStringListModel;
namespace Ui {class CMainWindow;};

struct SDebugCmd
{
    QString fSourceDir;
    QString fName;
    QString fCmd;
    QString fArgs;
    QString fWorkDir;
    QString fEnvVars;

    QString getEnvVars() const;
    QString getProjectName() const;
    void cleanUp( const CMainWindow * mainWindow );
};

struct SDirInfo
{
    SDirInfo() {}
    SDirInfo( QStandardItem * item );
    bool isValid() const;
    void writeCMakeFile( const QString & bldDir ) const;
    void writePropSheet( const QString & srcDir, const QString & bldDir, const QString & includeDirs ) const;
    void createDebugProjects( const QString & bldDir ) const;

    QStringList getSubDirs() const;
    void replaceFiles( QString & text, const QString & variable, const QStringList & files ) const;
    void addDependencies( QTextStream & qts ) const;
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

    QStringList fExtraTargets;
    QList<SDebugCmd> fDebugCommands;

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

    std::optional< QDir > getClientDir() const;
    std::optional< QString > getSourceDir( bool relPath = false ) const;
    std::optional< QString > getBuildDir( bool relPath = false ) const;

public Q_SLOTS:
    void slotChanged();
    void slotQtChanged();
    void slotCMakeChanged();

    void slotLoadSource();

    bool expandDirectories( QStandardItem * rootNode );

    void slotSelectCMake();
    void slotSelectSourceDir();
    void slotSelectQtDir();
    void slotSelectBuildDir();
    void slotSelectClientDir();
    void slotAddIncDir();
    void slotAddCustomBuild();
    void slotAddDebugCommand();

    void addDebugCommand( const QString & sourceDir, const QString & name, const QString & cmd, const QString & args, const QString & workDir, const QString & envVars );
    void addDebugCommand( const SDebugCmd & dbgCmd );

    void slotGenerate();

    void slotReadStdErr();
    void slotReadStdOut();
    void slotFinished( int exitCode, QProcess::ExitStatus status );
private:
    void addInclDirs( const QStringList & inclDirs );
    std::list< SDirInfo > generateTopLevelFiles( QProgressDialog * progress );
    void loadSettings();
    void loadQtSettings();
    void saveSettings();
    void appendToLog( const QString & txt );
    struct SSourceFileInfo
    {
        int fDirs{ 0 };
        int fFiles{ 0 };
        QStringList fBuildDirs;
        QStringList fInclDirs;
        QList< QPair< QString, bool > >fExecutables;
        QStringList fQtLibs;
        QString getText( bool forText ) const;
    };
    std::list< SDirInfo > getDirInfo( QStandardItem * parent, QProgressDialog * progress ) const;


    QList < SDebugCmd > getDebugCommandsForSourceDir( const QString & sourceDir ) const;

    QList< SDebugCmd > getDebugCommands( bool abs ) const;

    QStringList getCustomBuildsForSourceDir( const QString & sourceDir ) const;
    QList< QPair< QString, QString > > getCustomBuilds( bool abs ) const;
    void addCustomBuild( const QPair< QString, QString > & buildInfo );
    bool loadSourceFiles( const QString & dir, QStandardItem * parent, QProgressDialog * progress, SSourceFileInfo & results );
    void incProgress( QProgressDialog * progress ) const;
    bool isBuildDir( const QDir & relToDir, const QDir & dir ) const;
    bool isInclDir( const QDir & relToDir, const QDir & dir ) const;
    QList< QPair< QString, bool > > getExecutables( const QDir & dir ) const;
    std::tuple< QSet< QString >, QHash< QString, QList< QPair< QString, bool > > > > findDirAttributes( QStandardItem * parent ) const;
    QStringList getCmakeArgs() const;
    QString getIncludeDirs() const;

    QSet< QString > fBuildDirs;
    QStringList fInclDirs;
    QHash< QString, QList< QPair< QString, bool > > > fExecutables;
    QStringList fQtLibs;

    std::optional< QDir > fSourceDir;
    QStandardItemModel * fSourceModel{ nullptr };
    QStringListModel * fIncDirModel{ nullptr };
    QStandardItemModel * fCustomBuildModel{ nullptr };
    QStandardItemModel * fDebugCommandsModel{ nullptr };
    CCheckableStringListModel * fQtLibsModel{ nullptr };
    std::unique_ptr< Ui::CMainWindow > fImpl;
    QProcess * fProcess{ nullptr };
};

#endif // _ALCULATOR_H
