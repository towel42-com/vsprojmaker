// The MIT License( MIT )
//
// Copyright( c ) 2020-2021 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Softwa , and to permit persons to whom the Software is
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

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "SetupDebug.h"
#include "AddCustomBuild.h"
#include "WizardPages/ClientInfoPage.h"
#include "WizardPages/QtPage.h"
#include "WizardPages/SystemInfoPage.h"
#include "WizardPages/BuildTargetsPage.h"
#include "WizardPages/DebugTargetsPage.h"
#include "WizardPages/IncludesPage.h"
#include "WizardPages/PreProcDefinesPage.h"
#include "MainLib/VSProjectMaker.h"
#include "MainLib/DebugTarget.h"
#include "MainLib/DirInfo.h"
#include "MainLib/Settings.h"
#include "MainLib/BuildInfoData.h"

#include "SABUtils/UtilityModels.h"

#include <QSet>
#include <QFileInfo>
#include <QProcess>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QTimer>
#include <QRegularExpression>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QDirIterator>
#include <QProgressDialog>
#include <QTextStream>
#include <unordered_set>
#include <QLabel>
#include <QDirIterator>
#include <QDebug>
#include <QInputDialog>
#include <QWizard>

CMainWindow::CMainWindow( QWidget * parent )
    : QDialog( parent ),
    fImpl( new Ui::CMainWindow ),
    fProcess( new QProcess( this ) ),
    fSettings( new NVSProjectMaker::CSettings )
{
    NVSProjectMaker::registerTypes();

    fImpl->setupUi( this );
    popDisconnected( true );

    connect( fImpl->openProjectFileBtn, &QToolButton::clicked, this, &CMainWindow::slotOpenProjectFile );
    connect( fImpl->saveProjectFileBtn, &QToolButton::clicked, this, &CMainWindow::slotSaveProjectFile );
    connect( fImpl->runWizardBtn, &QToolButton::clicked, this, &CMainWindow::slotRunWizard );
    connect( fImpl->vsPathBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectVS );
    connect( fImpl->clientDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectClientDir );
    connect( fImpl->sourceDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectSourceDir );
    connect( fImpl->buildDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectBuildDir );
    connect( fImpl->qtDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectQtDir );
    connect( fImpl->prodDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectProdDir );
    connect( fImpl->msys64DirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectMSys64Dir );
    connect( fImpl->addIncDirBtn, &QToolButton::clicked, this, &CMainWindow::slotAddIncDir );
    connect( fImpl->addPreProcDefine, &QToolButton::clicked, this, &CMainWindow::slotAddPreProcDefine );
    connect( fImpl->addCustomBuildBtn, &QToolButton::clicked, this, &CMainWindow::slotAddCustomBuild );
    connect( fImpl->addDebugTargetBtn, &QToolButton::clicked, this, &CMainWindow::slotAddDebugTarget );
    connect( fImpl->bldOutputFileBtn, &QToolButton::clicked, this, &CMainWindow::slotSetBuildOutputFile );
    connect( fImpl->bldOutputFile, &QLineEdit::textChanged, this, &CMainWindow::slotLoadOutputData );
    connect( fImpl->runBuildAnalysisBtn, &QToolButton::clicked, this, &CMainWindow::slotLoadOutputData );
    
    connect( fImpl->generateBtn, &QToolButton::clicked, this, &CMainWindow::slotGenerate );

    fSourceModel = new QStandardItemModel( this );
    fImpl->sourceTree->setModel( fSourceModel );

    fPreProcDefineModel = new CCheckableStringListModel( this );
    fImpl->preProcDefines->setModel( fPreProcDefineModel );

    fIncDirModel = new CCheckableStringListModel( this );
    fImpl->incPaths->setModel( fIncDirModel );

    fQtLibsModel = new CCheckableStringListModel( this );
    fImpl->qtLibs->setModel( fQtLibsModel );

    fCustomBuildModel = new QStandardItemModel( this );
    fCustomBuildModel->setHorizontalHeaderLabels( QStringList() << "Directory" << "Target Name" );
    fImpl->customBuilds->setModel( fCustomBuildModel );

    fDebugTargetsModel = new QStandardItemModel( this );
    fDebugTargetsModel->setHorizontalHeaderLabels( QStringList() << "Source Dir" << "Name" << "Command" << "Args" << "Work Dir" << "EnvVars" );
    fImpl->debugTargets->setModel( fDebugTargetsModel );

    fBuildInfoDataModel = new QStandardItemModel( this );
    fBuildInfoDataModel->setHorizontalHeaderLabels( QStringList() << "Source Directory" << "Target Directory" << "Type" << "Primary Input File" << "Output File" );
    fImpl->bldData->setModel( fBuildInfoDataModel );
    QSettings settings;
    setProjects( settings.value( "RecentProjects" ).toStringList() );

    fImpl->projectFile->setFocus();
}

void CMainWindow::popDisconnected( bool force )
{
    if ( !force && fDisconnected )
        fDisconnected--;
    if ( force || fDisconnected == 0 )
    {
        connect( fImpl->projectFile, &QComboBox::currentTextChanged, this, &CMainWindow::slotCurrentProjectChanged );
        connect( fImpl->vsPath, &QLineEdit::textChanged, this, &CMainWindow::slotVSChanged );
        connect( fImpl->generator, &QComboBox::currentTextChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->clientDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->sourceRelDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->buildRelDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->qtDir, &QLineEdit::textChanged, this, &CMainWindow::slotQtChanged );
    }
}

void CMainWindow::pushDisconnected()
{
    if ( fDisconnected == 0 )
    {
        disconnect( fImpl->projectFile, &QComboBox::currentTextChanged, this, &CMainWindow::slotCurrentProjectChanged );
        disconnect( fImpl->vsPath, &QLineEdit::textChanged, this, &CMainWindow::slotVSChanged );
        disconnect( fImpl->generator, &QComboBox::currentTextChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->clientDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->sourceRelDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->buildRelDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->qtDir, &QLineEdit::textChanged, this, &CMainWindow::slotQtChanged );
    }
    fDisconnected++;
}

CMainWindow::~CMainWindow()
{
    saveSettings();
    
    QSettings settings;
    settings.setValue( "RecentProjects", getProjects() );
}

void CMainWindow::setCurrentProject( const QString & projFile )
{
    pushDisconnected();
    auto projects = getProjects();
    for ( int ii = 0; ii < projects.count(); ++ii )
    {
        if ( projects[ ii ] == projFile )
        {
            projects.removeAt( ii );
            ii--;
        }
    }
    projects.insert( 0, projFile );
    setProjects( projects );
    fImpl->projectFile->setCurrentText( projFile );
    popDisconnected();
}


QStringList CMainWindow::getProjects() const
{
    QStringList projects;
    for ( int ii = 1; ii < fImpl->projectFile->count(); ++ii )
        projects << fImpl->projectFile->itemText( ii );
    return projects;
}

void CMainWindow::reset()
{
    fSettings->clear();
    fImpl->cmakeExec->clear();
    pushDisconnected();
    fImpl->generator->clear();
    popDisconnected();
    fImpl->projectFile->setCurrentIndex( 0 );
    fImpl->log->clear();
    loadSettings();
}

void CMainWindow::setProjects( QStringList projects )
{
    for ( int ii = 0; ii < projects.count(); ++ii )
    {
        if ( projects[ ii ].isEmpty() || !QFileInfo( projects[ ii ] ).exists() )
        {
            projects.removeAt( ii );
            ii--;
        }
    }

    pushDisconnected();
    fImpl->projectFile->clear();
    fImpl->projectFile->addItems( QStringList() << QString() << projects );
    popDisconnected();
}

std::tuple< QSet< QString >, QHash< QString, QList< QPair< QString, bool > > > > CMainWindow::findDirAttributes( QStandardItem * parent ) const
{
    QSet< QString > buildDirs;
    QHash< QString, QList< QPair< QString, bool > > > execNamesMap;

    int numRows = parent ? parent->rowCount() : fSourceModel->rowCount();
    if ( parent && !parent->data( NVSProjectMaker::ERoles::eIsDirRole ).toBool() )
        return {};

    for ( int ii = 0; ii < numRows; ++ii )
    {
        auto curr = parent ? parent->child( ii ) : fSourceModel->item( ii );
        if ( curr->data( NVSProjectMaker::ERoles::eIsDirRole ).toBool() )
        {
            if ( curr->data( NVSProjectMaker::ERoles::eIsBuildDirRole ).toBool() )
                buildDirs.insert( curr->data( NVSProjectMaker::ERoles::eRelPathRole ).toString() );

            auto execNames = curr->data( NVSProjectMaker::ERoles::eExecutablesRole ).value < QList< QPair< QString, bool > > >();
            if ( !execNames.isEmpty() )
                execNamesMap.insert( curr->data( NVSProjectMaker::ERoles::eRelPathRole ).toString(), execNames );

            auto childValues = findDirAttributes( curr );
            buildDirs.unite( std::get< 0 >( childValues ) );
            for ( auto ii = std::get< 1 >( childValues ).cbegin(); ii != std::get< 1 >( childValues ).cend(); ++ii )
            {
                execNamesMap[ ii.key() ] << ii.value();
            }
        }
    }
    return std::make_tuple( buildDirs, execNamesMap );
}

void CMainWindow::addCustomBuild( const QPair< QString, QString > & customBuild )
{
    auto dirItem = new QStandardItem( customBuild.first );
    auto targetItem = new QStandardItem( customBuild.second );
    fCustomBuildModel->appendRow( QList< QStandardItem * >() << dirItem << targetItem );
}

std::optional< QDir > CMainWindow::getClientDir() const
{
    if ( fImpl->clientDir->text().isEmpty() )
        return {};
    QDir retVal( fImpl->clientDir->text() );
    if ( !retVal.exists() )
        return {};
    return retVal;
}


void CMainWindow::addPreProccesorDefines( const QStringList & preProcDefines )
{
    auto newPreProcDefines = fSettings->addPreProcessorDefines( preProcDefines );
    fPreProcDefineModel->setStringList( newPreProcDefines );
}

void CMainWindow::addInclDirs( const QStringList & inclDirs )
{
    auto incDirs = fSettings->addInclDirs( inclDirs );
    fIncDirModel->setStringList( incDirs );
}

std::optional< QString > CMainWindow::getDir( const QLineEdit * lineEdit, bool relPath ) const
{
    fSettings->setClientDir( fImpl->clientDir->text() );
    return fSettings->getDir( lineEdit->text(), relPath );
}

std::optional< QString > CMainWindow::getSourceDir( bool relPath ) const
{
    fSettings->setClientDir( fImpl->clientDir->text() );
    fSettings->setSourceRelDir( fImpl->sourceRelDir->text() );
    return fSettings->getSourceDir( relPath );
}

std::optional< QString > CMainWindow::getBuildDir( bool relPath ) const
{
    fSettings->setClientDir( fImpl->clientDir->text() );
    fSettings->setBuildRelDir( fImpl->buildRelDir->text() );
    return fSettings->getBuildDir( relPath );
}

QList < NVSProjectMaker::SDebugTarget > CMainWindow::getDebugCommands( bool absDir ) const
{
    QList < NVSProjectMaker::SDebugTarget > retVal;

    auto sourceDirPath = getSourceDir();
    if ( !sourceDirPath.has_value() )
        return {};
    QDir sourceDir( sourceDirPath.value() );
    sourceDir.cdUp();
    for ( int ii = 0; ii < fDebugTargetsModel->rowCount(); ++ii )
    {
        //( QStringList() << "Source Dir" << "Name" << "Command" << "Args" << "Work Dir" << "EnvVars" )
        NVSProjectMaker::SDebugTarget curr;
        curr.fSourceDir = fDebugTargetsModel->item( ii, 0 )->text();
        if ( absDir )
            curr.fSourceDir = sourceDir.absoluteFilePath( curr.fSourceDir );
        curr.fName = fDebugTargetsModel->item( ii, 1 )->text();
        curr.fCmd = fDebugTargetsModel->item( ii, 2 )->text();
        curr.fArgs = fDebugTargetsModel->item( ii, 3 )->text();
        curr.fWorkDir = fDebugTargetsModel->item( ii, 4 )->text();
        curr.fEnvVars = fDebugTargetsModel->item( ii, 5 )->text();
        retVal << curr;
    }
    return retVal;
}

QList< QPair< QString, QString > > CMainWindow::getCustomBuilds( bool absDir ) const
{
    QList< QPair< QString, QString > > retVal;

    auto bldDirPath = getBuildDir();
    if ( !bldDirPath.has_value() )
        return {};
    QDir bldDir( bldDirPath.value() );
    bldDir.cdUp();
    bldDir.cdUp();

    for ( int ii = 0; ii < fCustomBuildModel->rowCount(); ++ii )
    {
        auto dir = fCustomBuildModel->item( ii, 0 )->text();
        if ( absDir )
            dir = bldDir.absoluteFilePath( dir );
        auto target = fCustomBuildModel->item( ii, 1 )->text();
        retVal << qMakePair( dir, target );
    }
    return retVal;
}

void CMainWindow::saveSettings()
{
    fSettings->setVSPath( fImpl->vsPath->text() );
    fSettings->setGenerator( fImpl->generator->currentText() );
    fSettings->setClientDir( fImpl->clientDir->text() );
    auto srcDir = getSourceDir( true );
    fSettings->setSourceRelDir( srcDir.has_value() ? srcDir.value() : QString() );
    auto bldDir = getBuildDir( true );
    fSettings->setBuildRelDir( bldDir.has_value() ? bldDir.value() : QString() );
    fSettings->setQtDir( fImpl->qtDir->text() );
    fSettings->setProdDir( fImpl->prodDir->text() );
    fSettings->setMSys64Dir( fImpl->msys64Dir->text() );
    fSettings->setSelectedQtDirs( fQtLibsModel->getCheckedStrings() );

    auto attribs = findDirAttributes( nullptr );
    auto customBuilds = getCustomBuilds( false );
    auto dbgCommands = getDebugCommands( false );
    fSettings->setBuildDirs( std::get< 0 >( attribs ) );
    fSettings->setInclDirs( fIncDirModel->stringList() );
    fSettings->setSelectedInclDirs( fIncDirModel->getCheckedStrings() );
    fSettings->setPreProcDefines( fPreProcDefineModel->stringList() );
    fSettings->setSelectedPreProcDefines( fPreProcDefineModel->getCheckedStrings() );
    fSettings->setExecNames( std::get< 1 >( attribs ) );
    fSettings->setCustomBuilds( customBuilds );
    fSettings->setDebugCommands( dbgCommands );
}


void CMainWindow::loadSettings()
{
    pushDisconnected();

    fImpl->vsPath->setText( fSettings->getVSPath() );
    fImpl->generator->setCurrentText( fSettings->getGenerator() );
    fImpl->clientDir->setText( fSettings->getClientDir() );
    fImpl->buildRelDir->setText( fSettings->getBuildRelDir() );
    fImpl->qtDir->setText( fSettings->getQtDir() );
    fImpl->prodDir->setText( fSettings->getProdDir() );
    fImpl->msys64Dir->setText( fSettings->getMSys64Dir() );

    fQtLibsModel->setStringList( fSettings->getQtDirs() );
    fQtLibsModel->setChecked( fSettings->getSelectedQtDirs(), true, true );

    popDisconnected();

    fIncDirModel->setStringList( fSettings->getInclDirs() );
    auto selectedIncDirs = fSettings->getSelectedInclDirs();
    if ( selectedIncDirs.isEmpty() )
        fIncDirModel->checkAll( true );
    else
        fIncDirModel->setChecked( selectedIncDirs, true, true );

    fPreProcDefineModel->setStringList( fSettings->getPreProcDefines() );
    auto selectedPreProcDefines = fSettings->getSelectedPreProcDefines();
    if ( selectedPreProcDefines.isEmpty() )
        fPreProcDefineModel->checkAll( true );
    else
        fPreProcDefineModel->setChecked( selectedPreProcDefines, true, true );

    fCustomBuildModel->clear();
    fCustomBuildModel->setHorizontalHeaderLabels( QStringList() << "Directory" << "Target Name" );

    fDebugTargetsModel->clear();
    fDebugTargetsModel->setHorizontalHeaderLabels( QStringList() << "Source Dir" << "Name" << "Command" << "Args" << "Work Dir" << "EnvVars" );

    fExecutables = fSettings->getExecNames();
    auto customBuilds = fSettings->getCustomBuilds();
    for ( auto && ii : customBuilds )
        addCustomBuild( ii );

    fImpl->sourceRelDir->setText( fSettings->getSourceRelDir() );

    auto dbgCmds = fSettings->getDebugCommands();
    for ( auto && ii : dbgCmds )
        addDebugTarget( ii );

    slotChanged();
}

void CMainWindow::slotChanged()
{
    doChanged( true );
}

void CMainWindow::doChanged( bool loadSource )
{
    bool generatorOK = !fImpl->generator->currentText().isEmpty();

    auto clientDir = getClientDir();
    auto clientDirOK = clientDir.has_value();
    if ( clientDirOK )
    {
        fImpl->clientName->setText( clientDir.value().dirName() );
    }

    auto vsDir = QDir( fImpl->vsPath->text() );
    auto vsPath = QFileInfo( NVSProjectMaker::CSettings::getCMakeExec( fImpl->vsPath->text() ) );
    bool cmakePathOK = false;
    if ( vsDir.exists() && vsPath.exists() && vsPath.isExecutable() )
    {
        fImpl->cmakeExec->setText( vsPath.absoluteFilePath() );
        QFileInfo fi( fImpl->cmakeExec->text() );
        cmakePathOK = !fImpl->cmakeExec->text().isEmpty() && fi.exists() && fi.isExecutable();
    }
    if ( cmakePathOK )
    {
        slotVSChanged();
    }

    auto sourceDirPath = getSourceDir();
    auto fi = clientDirOK && sourceDirPath.has_value() ? QFileInfo( sourceDirPath.value() ) : QFileInfo();
    bool sourceDirOK = clientDirOK && getSourceDir().has_value() && fi.exists() && fi.isDir() && fi.isReadable();

    auto sourceDir = sourceDirPath.has_value() ? QDir( sourceDirPath.value() ) : QDir();
    if ( fSourceDir.has_value() && ( !sourceDirOK || ( fSourceDir.value() != sourceDir ) ) )
    {
        fSourceModel->clear();
    }

    if ( sourceDirOK && ( !fSourceDir.has_value() || ( fSourceDir.has_value() && ( fSourceDir != sourceDir ) ) ) )
    {
        fSourceDir = sourceDir;
        if ( loadSource )
            QTimer::singleShot( 0, this, &CMainWindow::slotLoadSource );
    }

    if ( !sourceDirOK )
        fSourceDir = QDir();

    auto bldDir = getBuildDir();
    fi = clientDirOK && bldDir.has_value() ? QFileInfo( bldDir.value() ) : QFileInfo();
    bool bldDirOK = clientDirOK && bldDir.has_value() && fi.exists() && fi.isDir() && fi.isWritable();

    fImpl->sourceDirBtn->setEnabled( clientDirOK );
    fImpl->buildDirBtn->setEnabled( clientDirOK );
    fImpl->addDebugTargetBtn->setEnabled( sourceDirOK && bldDirOK );
    fImpl->addIncDirBtn->setEnabled( sourceDirOK );
    fImpl->generateBtn->setEnabled( cmakePathOK && clientDirOK && generatorOK && sourceDirOK && bldDirOK );
}

void CMainWindow::slotRunWizard()
{
    reset();
    QWizard wizard;
    CSystemInfoPage * sysInfoPage = new CSystemInfoPage;
    wizard.addPage( sysInfoPage );
    sysInfoPage->setDefaults();
    
    auto qtPage = new CQtPage;
    wizard.addPage( qtPage );
    qtPage->setDefaults();

    auto clientInfoPage = new CClientInfoPage;
    wizard.addPage( clientInfoPage );
    clientInfoPage->setDefaults();

    auto buildTargetsPage = new CBuildTargetsPage;
    wizard.addPage( buildTargetsPage );

    auto debugTargetsPage = new CDebugTargetsPage;
    wizard.addPage( debugTargetsPage );
    
    auto includesPage = new CIncludesPage;
    wizard.addPage( includesPage );

    auto preProcDefinesPage = new CPreProcDefinesPage;
    wizard.addPage( preProcDefinesPage );

    if ( wizard.exec() == QWizard::Accepted )
    {
        pushDisconnected();

        fSettings->setClientDir( wizard.field( "clientDir" ).toString() );

        fImpl->vsPath->setText( wizard.field( "vsPath" ).toString() );
        fImpl->prodDir->setText( wizard.field( "prodDir" ).toString() );
        fImpl->msys64Dir->setText( wizard.field( "msys64Dir" ).toString() );

        fImpl->clientDir->setText( wizard.field( "clientDir" ).toString() );
        fImpl->buildRelDir->setText( wizard.field( "buildRelDir" ).toString() );
        fImpl->sourceRelDir->setText( wizard.field( "sourceRelDir" ).toString() );

        fImpl->qtDir->setText( wizard.field( "qtDir" ).toString() );

        auto qtDirs = qtPage->qtDirs();
        fQtLibsModel->setStringList( qtDirs.first );
        fQtLibsModel->setChecked( qtDirs.second, true, true );

        loadBuildTargets( buildTargetsPage->enabledBuildTargets() );
        loadDebugTargets( debugTargetsPage->enabledDebugTargets() );

        popDisconnected();

        doChanged( false );
        slotLoadSource();
        loadIncludeDirs( includesPage->enabledIncludeDirs() );
        loadPreProcessorDefines( preProcDefinesPage->enabledPreProcDefines() );
        QString projFile = QDir( wizard.field( "clientDir" ).toString() ).absoluteFilePath( wizard.field( "clientName" ).toString() ) + ".ini";
        setProjectFile( projFile, false );
        doChanged( false );
    }
}

void CMainWindow::loadBuildTargets( const QStringList & targets )
{
    for ( auto && ii : targets )
    {
        addCustomBuild( qMakePair( fImpl->buildRelDir->text(), ii ) );
    }
}

void CMainWindow::loadDebugTargets( const QStringList & targets )
{
    for ( auto && ii : targets )
    {
        auto path = QString( "src/hdloffice/hdlstudio/cxx/main/src" );
        auto name = ii;
        QString args;
        if ( ii.startsWith( "HDL Client+" ) )
        {
            auto nm = ii.mid( 11 );
            args = QString( "+external_client+<CLIENTDIR>/devapps/visualizer/win64/%1.dll" ).arg( nm );
        }

        auto executable = QString( "<CLIENTDIR>/modeltech/win64/VisualizerRls/bin/hdlclient.exe" );
        if ( ii == "HDL Studio" )
        {
            executable = QString( "<CLIENTDIR>/modeltech/win64/VisualizerRls/bin/hdlstudio.exe" );
        }
        auto workDir = QString( "<CLIENTDIR>" );

        QString envVars;
        if ( ii == "HDL Client+vxcSampleWithProject" )
            envVars = QString( "{SKIP_LIMITED_FEATURES_KEY=1}{ENABLE_VFPRJ=1}" );
        else if ( ii == "HDL Client+vxcSampleWithFlowNav" )
            envVars = QString( "{SKIP_LIMITED_FEATURES_KEY=1}{ENABLE_FLOWNAV=1}" );

        addDebugTarget( path, name, executable, args, workDir, envVars );
    }
}

void CMainWindow::loadIncludeDirs( const QStringList & includeDirs )
{
    auto currDirs = QStringList() << includeDirs << fIncDirModel->stringList();

    std::list< std::pair< QString, bool > > newIncludes;
    for ( auto && ii : currDirs )
    {
        auto checked = !ii.startsWith( "solver/alanmi" );
        newIncludes.push_back( std::make_pair( ii, checked ) );
    }
    fIncDirModel->setStringList( newIncludes );
}

void CMainWindow::loadPreProcessorDefines( const QStringList & preProcDefines )
{
    auto curr = QStringList() << preProcDefines << fPreProcDefineModel->stringList();

    fPreProcDefineModel->setStringList( curr, true );
}

void CMainWindow::slotOpenProjectFile()
{
    auto currPath = fImpl->projectFile->currentText();
    if ( currPath.isEmpty() )
        currPath = fSettings->fileName();
    if ( currPath.isEmpty() )
        currPath = QString();
    auto projFile = QFileDialog::getOpenFileName( this, tr( "Select Project File to open" ), currPath, "Project Files *.ini;;All Files *.*" );
    setProjectFile( projFile, true );
}

void CMainWindow::slotSaveProjectFile()
{
    auto currPath = fImpl->projectFile->currentText();
    if ( currPath.isEmpty() )
        currPath = fSettings->fileName();
    if ( currPath.isEmpty() )
        currPath = QString();
    auto projFile = QFileDialog::getSaveFileName( this, tr( "Select Project File to save" ), currPath, "Project Files *.ini;;All Files *.*" );
    setProjectFile( projFile, false );
}

void CMainWindow::slotCurrentProjectChanged( const QString & projFile )
{
    setProjectFile( projFile, true );
}

void CMainWindow::setProjectFile( const QString & projFile, bool load )
{
    if ( projFile.isEmpty() )
        return;
    setCurrentProject( projFile );

    if ( load )
    {
        if ( !fSettings->loadSettings( projFile ) )
        {
            QMessageBox::critical( this, tr( "Project File not Opened" ), QString( "Error: '%1' is not a valid project file" ).arg( projFile ) );
            return;
        }
    }
    else
        fSettings->setFileName( projFile );
    setWindowTitle( tr( "Visual Studio Project Generator - %1" ).arg( projFile ) );
    if ( load )
        loadSettings();
    else
    {
        saveSettings();
        fSettings->saveSettings();
    }
}

void CMainWindow::slotSelectVS()
{
    auto currPath = fImpl->vsPath->text();
    if ( currPath.isEmpty() )
        currPath = QString( "C:/Program Files (x86)/Microsoft Visual Studio/2017" );
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Visual Studio Directory" ), currPath );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() )
    {
        QMessageBox::critical( this, tr( "Error Valid Directory not Selected" ), QString( "Error: '%1' is not a directory" ).arg( dir ) );
        return;
    }

    fi = QFileInfo( NVSProjectMaker::CSettings::getCMakeExec( dir ) );
    if ( !fi.exists() || !fi.isExecutable() )
    {
        QMessageBox::critical( this, tr( "Error Valid Directory not Selected" ), QString( "Error: '%1' is not an executable" ).arg( fi.absoluteFilePath() ) );
        return;
    }
    fImpl->vsPath->setText( dir );
}

void CMainWindow::slotVSChanged()
{
    auto currPath = fImpl->vsPath->text();
    if ( currPath.isEmpty() )
        return;
    QFileInfo fi( currPath );
    if ( !fi.exists() || !fi.isDir() )
        return;

    auto cmakePath = NVSProjectMaker::CSettings::getCMakeExec( currPath );
    if ( currPath.isEmpty() )
        return;

    pushDisconnected();
    auto currText = fImpl->generator->currentText();
    fImpl->generator->clear();

    QApplication::setOverrideCursor( Qt::WaitCursor );

    //process.setProcessChannelMode(QProcess::MergedChannels);
    fProcess->start( cmakePath, QStringList() << "-help" );
    if ( !fProcess->waitForFinished( -1 ) || ( fProcess->exitStatus() != QProcess::NormalExit ) || ( fProcess->exitCode() != 0 ) )
    {
        QMessageBox::critical( this, tr( "Error Running CMake" ), QString( "Error: '%1' Could not run cmake and determine Generators" ).arg( QString( fProcess->readAllStandardError() ) ) );
        return;
    }
    auto data = QString( fProcess->readAll() );
    auto lines = data.split( '\n' );
    QStringList generators;
    bool inGenerators = false;
    bool usePrev = false;
    for ( auto && curr : lines )
    {
        if ( !inGenerators )
        {
            inGenerators = curr == "Generators";
            continue;
        }
        if ( curr.startsWith( "The following" ) )
            continue;

        curr = curr.mid( 2 );

        if ( curr.isEmpty() )
            continue;

        auto lineCont = ( curr[ 0 ] == ' ' );
        if ( lineCont && generators.isEmpty() )
            continue;

        curr = curr.trimmed();

        if ( lineCont )
        {
            if ( usePrev )
                generators.back() += " " + curr;
        }
        else
        {
            curr.replace( QRegularExpression( "\\s+=\\s+" ), " = " );
            bool usePrev = false;
            if ( !curr.contains( "Visual Studio" ) &&
                 !curr.startsWith( "NMake" ) )
                continue;
            usePrev = true;
            generators.push_back( curr );
        }
    }

    fImpl->generator->addItems( generators );

    QApplication::restoreOverrideCursor();
    popDisconnected();

    auto idx = fImpl->generator->findText( currText );
    if ( idx != -1 )
        fImpl->generator->setCurrentIndex( idx );
}

void CMainWindow::slotSelectSourceDir()
{
    if ( !getClientDir().has_value() )
        return;

    auto currPath = getSourceDir( false );
    if ( !currPath.has_value() )
        currPath = fImpl->clientDir->text();
    if ( !currPath.has_value() || currPath.value().isEmpty() )
        currPath = QString();
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Source Directory" ), currPath.value() );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    fImpl->sourceRelDir->setText( QDir( getClientDir().value() ).relativeFilePath( dir ) );
}

void CMainWindow::slotSelectClientDir()
{
    auto currPath = getClientDir();
    if ( !currPath.has_value() )
        currPath = QDir();
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Client Directory" ), currPath.value().absolutePath() );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Writable Directory not Selected" ), QString( "Error: '%1' is not an writable directory" ).arg( dir ) );
        return;
    }

    fImpl->clientDir->setText( dir );
}

void CMainWindow::slotSelectBuildDir()
{
    auto currPath = getBuildDir();
    if ( !currPath.has_value() )
        currPath = fImpl->clientDir->text();
    if ( !currPath.has_value() || currPath.value().isEmpty() )
        currPath = QString();
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Build Directory" ), currPath.value() );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isWritable() )
    {
        QMessageBox::critical( this, tr( "Error Writable Directory not Selected" ), QString( "Error: '%1' is not an writable directory" ).arg( dir ) );
        return;
    }

    fImpl->buildRelDir->setText( QDir( getClientDir().value() ).relativeFilePath( dir ) );
}

void CMainWindow::slotAddDebugTarget()
{
    CSetupDebug dlg( getSourceDir().value(), getBuildDir().value(), this );
    if ( dlg.exec() == QDialog::Accepted && !dlg.command().isEmpty() && !dlg.sourceDir().isEmpty() )
    {
        addDebugTarget( dlg.sourceDir(), dlg.name(), dlg.command(), dlg.args(), dlg.workDir(), dlg.envVars() );
    }
}

void CMainWindow::addDebugTarget( const NVSProjectMaker::SDebugTarget & cmd )
{
    addDebugTarget( cmd.fSourceDir, cmd.fName, cmd.fCmd, cmd.fArgs, cmd.fWorkDir, cmd.fEnvVars );
}

void CMainWindow::addDebugTarget( const QString & sourcePath, const QString & name, const QString & cmd, const QString & args, const QString & workDir, const QString & envVars )
{
    if ( !getSourceDir().has_value() )
        return;

    auto sourceDir = QDir( getSourceDir().value() );
    sourceDir.cdUp();
    auto sourceDirItem = new QStandardItem( sourceDir.relativeFilePath( sourcePath ) );
    auto nameItem = new QStandardItem( name );
    auto cmdItem = new QStandardItem( cmd );
    auto argsItem = new QStandardItem( args );
    auto workDirItem = new QStandardItem( workDir );
    auto envVarsItem = new QStandardItem( envVars );
    fDebugTargetsModel->appendRow( QList< QStandardItem * >() << sourceDirItem << nameItem << cmdItem << argsItem << workDirItem << envVarsItem );
}

void CMainWindow::slotAddCustomBuild()
{
    auto currPath = getBuildDir();
    if ( !currPath.has_value() )
        return;

    CAddCustomBuild dlg( getBuildDir().value(), this );
    if ( dlg.exec() == QDialog::Accepted && !dlg.buildDir().isEmpty() && !dlg.targetName().isEmpty() )
    {
        addCustomBuild( qMakePair( dlg.buildDir(), dlg.targetName() ) );
    }
}

void CMainWindow::slotAddPreProcDefine()
{
    auto define = QInputDialog::getText( this, tr( "PreProcessor Define:" ), tr( "Define:" ) );
    if ( define.isEmpty() )
        return;

    fPreProcDefineModel->insertFront( define, true );
}

void CMainWindow::slotAddIncDir()
{
    auto sourceDir = getSourceDir();
    if ( !sourceDir.has_value() )
        sourceDir = QString();
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Directory to Include" ), sourceDir.value() );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    if ( fi.absolutePath().startsWith( fSettings->getProdDir() ) )
        dir = QDir( fSettings->getProdDir() ).relativeFilePath( dir );
    else
        dir = QDir( sourceDir.value() ).relativeFilePath( dir );
    fIncDirModel->insertFront( dir, true );
}

void CMainWindow::slotSelectQtDir()
{
    auto currPath = fImpl->qtDir->text();
    if ( currPath.isEmpty() )
        currPath = QString();
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Qt Directory" ), currPath );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    fImpl->qtDir->setText( dir );
}

void CMainWindow::slotSelectProdDir()
{
    auto currPath = fImpl->prodDir->text();
    if ( currPath.isEmpty() )
        currPath = "C:/localprod";
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Product Directory" ), currPath );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    fImpl->prodDir->setText( dir );
}

void CMainWindow::slotSelectMSys64Dir()
{
    auto currPath = fImpl->msys64Dir->text();
    if ( currPath.isEmpty() )
        currPath = "C:/msys64";
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select MSys64 Directory" ), currPath );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    fImpl->msys64Dir->setText( dir );
}

void CMainWindow::slotSetBuildOutputFile()
{
    auto currPath = fImpl->bldOutputFile->text();
    if ( currPath.isEmpty() && fSettings->getBuildDir().has_value() )
        currPath = fSettings->getBuildDir().value();

    auto newPath = QFileDialog::getOpenFileName( this, tr( "Select Output Data File from Build" ), currPath, tr( "Output Data Files *.txt;;All Files *.*" ) );
    if ( newPath.isEmpty() )
        return;

    QFileInfo fi( newPath );
    if ( !fi.exists() || !fi.isFile() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable File not Selected" ), QString( "Error: '%1' is not an readable file" ).arg( newPath ) );
        return;
    }

    fImpl->bldOutputFile->setText( newPath );
}

void CMainWindow::slotLoadOutputData()
{
    fBuildInfoDataModel->clear();
    fImpl->log->clear();

    QProgressDialog * progress = new QProgressDialog( tr( "Reading Build Output..." ), tr( "Cancel" ), 0, 0, this );
    progress->setLabelText( "Reading Build Output" );
    progress->setAutoReset( false );
    progress->setAutoClose( false );
    progress->setWindowModality( Qt::WindowModal );
    progress->setMinimumDuration( 1 );
    progress->setRange( 0, 100 );
    progress->setValue( 0 );

    fBuildInfoData = std::make_shared< NVSProjectMaker::CBuildInfoData >( fImpl->bldOutputFile->text(),
                                                                     [ this ]( const QString & msg )
                                                                     {    
                                                                          appendToLog( msg );
                                                                          qApp->processEvents();
                                                                     }, progress );
    if ( !fBuildInfoData->status() )
    {
        progress->close();
        QMessageBox::critical( this, tr( "Could not read Output Data File" ), fBuildInfoData->errorString() );
        fBuildInfoData.reset();
        return;
    }
    fBuildInfoData->loadIntoTree( fBuildInfoDataModel );
    progress->deleteLater();
}


void CMainWindow::appendToLog( const QString & txt )
{
    fImpl->log->appendPlainText( txt.trimmed() );
}

void CMainWindow::slotQtChanged()
{
    pushDisconnected();

    saveSettings();
    fQtLibsModel->setStringList( QStringList() );

    fImpl->qtDir->setText( fSettings->getQtDir() );
    fSettings->setQtDir( fImpl->qtDir->text() );
    fQtLibsModel->setStringList( fSettings->getQtDirs() );
    fQtLibsModel->setChecked( fSettings->getSelectedQtDirs(), true, true );

    popDisconnected();
}

QStandardItem * CMainWindow::loadSourceFileModel()
{
    fSourceModel->clear();
    fSourceModel->setHorizontalHeaderLabels( QStringList() << "Name" << "Is Build Dir?" << "Exec Name" << "Is Include Path?" );

    auto rootNode = new QStandardItem( fSettings->getResults()->fRootDir->fName );
    rootNode->setData( true, NVSProjectMaker::ERoles::eIsDirRole );
    fSourceModel->appendRow( rootNode );
    for ( auto && ii : fSettings->getResults()->fRootDir->fChildren )
    {
        ii->createItem( rootNode );
    }
    return rootNode;
}

void CMainWindow::slotLoadSource()
{
    fImpl->log->clear();

    auto text = fSourceDir.value().dirName();

    fSettings->getResults()->clear();
    fSettings->getResults()->fRootDir->fName = text;
    fSettings->getResults()->fRootDir->fIsDir = true;

    appendToLog( tr( "============================================" ) );
    appendToLog( tr( "Finding Source Files..." ) );
    QProgressDialog * progress = new QProgressDialog( tr( "Finding Source Files..." ), tr( "Cancel" ), 0, 0, this );
    auto label = new QLabel;
    label->setAlignment( Qt::AlignLeft );
    progress->setLabel( label );
    progress->setAutoReset( false );
    progress->setAutoClose( false );
    progress->setWindowModality( Qt::WindowModal );
    progress->setMinimumDuration( 1 );
    progress->setRange( 0, 100 );
    progress->setValue( 0 );
    bool wasCancelled = fSettings->loadSourceFiles( fSourceDir.value(), getSourceDir().value(), progress, [this]( const QString & msg ) { appendToLog( msg ); } );
    auto rootNode = loadSourceFileModel();

    addInclDirs( fSettings->getResults()->fInclDirs );
    appendToLog( tr( "============================================" ) );

    if ( wasCancelled )
    {
        fSourceModel->clear();
        appendToLog( tr( "Finding Source Files Canceled" ) );
        QMessageBox::warning( this, tr( "canceled" ), tr( "Finding Source files Canceled" ) );
    }
    else
    {
        delete progress;
        qApp->processEvents();
        expandDirectories( rootNode );

        appendToLog( tr( "Finished Finding Source Files" ) );
        appendToLog( tr( "Results:" ) );
        appendToLog( fSettings->getResults()->getText( true ) );
    }
    fImpl->tabWidget->setCurrentIndex( 0 );
}

bool CMainWindow::expandDirectories( QStandardItem * node )
{
    auto num = node->rowCount();
    bool hasDir = false;
    for ( int ii = 0; ii < num; ++ii )
    {
        auto child = node->child( ii );
        bool isDir = child->data( NVSProjectMaker::ERoles::eIsDirRole ).toBool();
        hasDir = hasDir || isDir;
        if ( isDir )
            expandDirectories( child );
    }
    if ( hasDir )
        fImpl->sourceTree->setExpanded( node->index(), true );
    return hasDir;
}

QString CMainWindow::getIncludeDirs() const
{
    QStringList retVal;

    QDir sourceDir( getSourceDir().value() );
    auto inclDirs = fIncDirModel->getCheckedStrings();
    for ( auto && ii : inclDirs )
    {
        retVal << sourceDir.absoluteFilePath( ii );
    }

    auto qtDirs = fQtLibsModel->getCheckedStrings();
    auto qtInclDir = QDir( fImpl->qtDir->text() );
    qtInclDir.cd( "include" );
    retVal << qtInclDir.absolutePath();
    for ( auto && ii : qtDirs )
    {
        retVal << qtInclDir.absoluteFilePath( ii );
    }

    return retVal.join( ";" );
}

void CMainWindow::slotGenerate()
{
    QApplication::setOverrideCursor( Qt::WaitCursor );

    auto progress = new QProgressDialog( tr( "Generating CMake Files..." ), tr( "Cancel" ), 0, 0, this );
    QPushButton * pb = new QPushButton( tr( "Cancel" ) );
    progress->setCancelButton( pb );
    progress->setWindowFlags( progress->windowFlags() & ~Qt::WindowCloseButtonHint );
    progress->setAutoReset( false );
    progress->setAutoClose( false );
    progress->setWindowModality( Qt::WindowModal );
    progress->setMinimumDuration( 1 );
    progress->setRange( 0, 100 );
    progress->setValue( 0 );
    qApp->processEvents();

    saveSettings();
    if ( !fSettings->generate( progress, this, [this]( const QString & msg ) { appendToLog( msg ); } ) )
        return;

    progress->setLabelText( tr( "Running CMake" ) );
    pb->setEnabled( false );
    progress->setRange( 0, 0 );

    fSettings->runCMake(
        [this]( const QString & outMsg )
    {
        appendToLog( outMsg );
    },
        [this]( const QString & errMsg )
    {
        appendToLog( "ERROR: " + errMsg );
    },
        fProcess
        , { false, [this, progress]() { QApplication::restoreOverrideCursor(); progress->deleteLater(); } } );
}
