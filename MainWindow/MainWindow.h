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

#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include <QDialog>
#include <QPointer>
#include <QDir>
#include <QProcess>
#include <QStandardItemModel>
#include <memory>
#include <optional>
#include <unordered_map>
#include <tuple>
#include <set>
#include <functional>
#include <tuple>
#include <list>
using TStringSet = std::set< QString >;

struct SDebugTargetInfo;
class QLineEdit;
namespace NVSProjectMaker
{
    struct SDirInfo;
    struct SDebugTarget;
    class CSettings;
    class CBuildInfoData;
    struct SSourceFileResults;
    struct SSourceFileInfo;
}

class CMainWindow;
class QTextStream;
class QStandardItemModel;
class QStandardItem;
class QStringListModel;
class QProgressDialog;
class QProcess;
namespace NSABUtils
{
    class CCheckableStringListModel;
}

namespace Ui { class CMainWindow; };

class CMainWindow : public QDialog
{
    Q_OBJECT
public:
    CMainWindow(QWidget *parent = 0);

    ~CMainWindow();

    std::optional< QDir > getClientDir() const;
    std::optional< QString > getSourceDir( bool relPath = false ) const;
    std::optional< QString > getBuildDir( bool relPath = false ) const;

Q_SIGNALS:
    void sigCMakeFinished();
public Q_SLOTS:
    void slotChanged();
    void slotQtChanged();
    void slotVSChanged();

    void slotLoadSource();
    void slotLoadOutputData();
    void slotLoadSourceAndOutputData();

    bool expandDirectories( QStandardItem * rootNode );

    void slotOpenProjectFile();
    void slotSaveProjectFile();

    void saveProjectFile( const QString & currPath );

    void slotSelectCMake();
    void slotRunWizard();
    //void slotSelectVS();
    void slotSelectSourceDir();
    void slotSelectQtDir();
    void slotSelectProdDir();
    void slotSelectMSys64Dir();
    void slotSelectBuildDir();
    void slotSelectClientDir();
    void slotAddPreProcDefine();
    void slotAddIncDir();
    void slotAddCustomBuild();
    void slotAddDebugTarget();
    void slotCurrentProjectChanged( const QString & projFile );
    void slotSetBuildOutputFile();
    void slotCMakeFinished();

    void addDebugTarget( const QString & sourceDir, const QString & name, const QString & cmd, const QStringList & args, const QString & workDir, const QStringList & envVars );
    void addDebugTarget( const NVSProjectMaker::SDebugTarget & dbgCmd );

    void slotGenerate();
    void slotBuildsChanged();
    void slotLoadInstalledVS();

private:
    QString getCMakeExec() const;
    QString getSelectedVSPath() const;

    QProcess * getProcess();
    void clearProcess();
    void loadBuildTargetsFromWizard(const QStringList & target, const QString & primaryTarget);
    void loadDebugTargetsFromWizard( const std::list< std::shared_ptr< SDebugTargetInfo > > & targets );
    void loadIncludeDirsFromWizard(const QStringList & includeDirs);
    void loadPreProcessorDefinesFromWizard( const QStringList & preProcDefines );

    void reset();
    QStandardItem * loadSourceFileModel();
    void pushDisconnected();
    void popDisconnected( bool force=false );

    void saveRecentProjects();
    QStringList getProjects() const;
    void setProjects( QStringList projects );

    void setProjectFile( const QString & projFile, bool forLoad );

    void setCurrentProject( const QString & projFile );

    std::optional< QString > getDir( const QLineEdit * lineEdit, bool relPath ) const;
    void addPreProccesorDefines( const QStringList & preProcDefines );
    void addInclDirs( const QStringList & inclDirs );
    std::list< std::shared_ptr< NVSProjectMaker::SDirInfo > > generateTopLevelFiles( QProgressDialog * progress );
    void loadSettings();
    void saveSettings();
    void appendToLog( const QString & txt );
    std::list < NVSProjectMaker::SDebugTarget > getDebugCommandsForSourceDir( const QString & sourceDir ) const;
    std::list< NVSProjectMaker::SDebugTarget > getDebugCommands( bool abs ) const;

    QStringList getCustomBuildsForSourceDir( const QString & sourceDir ) const;
    std::list< std::pair< QString, QString > > getCustomBuilds( bool abs ) const;
    void addCustomBuild( const std::pair< QString, QString > & buildInfo );
    std::list< std::pair< QString, bool > > getExecutables( const QDir & dir ) const;
    std::tuple< TStringSet, std::unordered_map< QString, std::list< std::pair< QString, bool > > > > findDirAttributes( QStandardItem * parent ) const;
    QString getIncludeDirs() const;

    void doChanged( bool loadSource );

    std::unordered_map< QString, std::list< std::pair< QString, bool > > > fExecutables;

    std::vector< QMetaObject::Connection > fConnectionList;
    std::optional< QDir > fSourceDir;
    std::optional< QString > fBuildTextFile;
    QStandardItemModel * fSourceModel{ nullptr };
    QStandardItemModel * fBuildInfoDataModel{ nullptr };
    NSABUtils::CCheckableStringListModel * fIncDirModel{ nullptr };
    NSABUtils::CCheckableStringListModel * fPreProcDefineModel{ nullptr };
    QStandardItemModel * fCustomBuildModel{ nullptr };
    QStandardItemModel * fDebugTargetsModel{ nullptr };
    NSABUtils::CCheckableStringListModel * fQtLibsModel{ nullptr };
    std::unique_ptr< Ui::CMainWindow > fImpl;
    QProcess * fProcess{ nullptr };

    int fDisconnected{ 0 };
    bool fLoadSourceAfterLoadData{ false };

    std::unique_ptr< NVSProjectMaker::CSettings > fSettings;
    std::shared_ptr< NVSProjectMaker::CBuildInfoData > fBuildInfoData;
    QStringList fProdDirUsages;
    QPointer< QProgressDialog > fProgress;
};

#endif // _ALCULATOR_H
