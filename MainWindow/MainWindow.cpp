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
#include "SABUtils/StringUtils.h"

#include <set>
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
    fProcess( nullptr ),
    fSettings( new NVSProjectMaker::CSettings )
{
    NVSProjectMaker::registerTypes();

    fImpl->setupUi( this );
    popDisconnected( true );

    connect( fImpl->openProjectFileBtn, &QToolButton::clicked, this, &CMainWindow::slotOpenProjectFile );
    connect( fImpl->saveProjectFileBtn, &QToolButton::clicked, this, &CMainWindow::slotSaveProjectFile );
    connect( fImpl->runWizardBtn, &QToolButton::clicked, this, &CMainWindow::slotRunWizard );
    connect( fImpl->vsPathBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectVS );
    connect(fImpl->cmakeExecBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectCMake);
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

    connect( fCustomBuildModel, &CCheckableStringListModel::dataChanged, this, &CMainWindow::slotBuildsChanged );

    fDebugTargetsModel = new QStandardItemModel( this );
    fDebugTargetsModel->setHorizontalHeaderLabels( QStringList() << "Source Dir" << "Name" << "Command" << "Args" << "Work Dir" << "EnvVars" );
    fImpl->debugTargets->setModel( fDebugTargetsModel );

    fBuildInfoDataModel = new QStandardItemModel( this );
    fBuildInfoDataModel->setHorizontalHeaderLabels( QStringList() << "Source Directory" << "Target Directory" << "Type" << "Primary Input File" << "Output File" );
    fImpl->bldData->setModel( fBuildInfoDataModel );
    QSettings settings;
    setProjects( settings.value( "RecentProjects" ).toStringList() );

    fImpl->projectFile->setFocus();
    fImpl->tabWidget->setCurrentIndex( 0 );
    fImpl->verbose->setChecked( true );
}

void CMainWindow::popDisconnected( bool force )
{
    if ( !force && fDisconnected )
        fDisconnected--;
    if ( force || fDisconnected == 0 )
    {
        connect( fImpl->projectFile, &QComboBox::currentTextChanged, this, &CMainWindow::slotCurrentProjectChanged );
        connect( fImpl->vsPath, &QLineEdit::textChanged, this, &CMainWindow::slotVSChanged );
        connect(fImpl->cmakeExec, &QLineEdit::textChanged, this, &CMainWindow::slotChanged);
        connect(fImpl->useCustomCMake, &QGroupBox::clicked, this, &CMainWindow::slotChanged);
        connect( fImpl->generator, &QComboBox::currentTextChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->clientDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->sourceRelDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->buildRelDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->qtDir, &QLineEdit::textChanged, this, &CMainWindow::slotQtChanged );
        connect( fImpl->bldOutputFile, &QLineEdit::textChanged, this, &CMainWindow::slotLoadOutputData );
        connect( fCustomBuildModel, &CCheckableStringListModel::dataChanged, this, &CMainWindow::slotBuildsChanged );
    }
}

void CMainWindow::pushDisconnected()
{
    if ( fDisconnected == 0 )
    {
        disconnect( fImpl->projectFile, &QComboBox::currentTextChanged, this, &CMainWindow::slotCurrentProjectChanged );
        disconnect( fImpl->vsPath, &QLineEdit::textChanged, this, &CMainWindow::slotVSChanged );
        disconnect(fImpl->cmakeExec, &QLineEdit::textChanged, this, &CMainWindow::slotChanged);
        disconnect(fImpl->useCustomCMake, &QGroupBox::clicked, this, &CMainWindow::slotChanged);
        disconnect( fImpl->generator, &QComboBox::currentTextChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->clientDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->sourceRelDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->buildRelDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->qtDir, &QLineEdit::textChanged, this, &CMainWindow::slotQtChanged );
        disconnect( fImpl->bldOutputFile, &QLineEdit::textChanged, this, &CMainWindow::slotLoadOutputData );
        disconnect( fCustomBuildModel, &CCheckableStringListModel::dataChanged, this, &CMainWindow::slotBuildsChanged );
    }
    fDisconnected++;
}

CMainWindow::~CMainWindow()
{
    saveSettings();
    
    saveRecentProjects();
}

void CMainWindow::saveRecentProjects()
{
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
    saveRecentProjects();
}

std::tuple< TStringSet, std::unordered_map< QString, std::list< std::pair< QString, bool > > > > CMainWindow::findDirAttributes( QStandardItem * parent ) const
{
    TStringSet buildDirs;
    std::unordered_map< QString, std::list< std::pair< QString, bool > > > execNamesMap;

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

            auto execNames = curr->data( NVSProjectMaker::ERoles::eExecutablesRole ).value < std::list< std::pair< QString, bool > > >();
            if ( !execNames.empty() )
                execNamesMap[curr->data( NVSProjectMaker::ERoles::eRelPathRole ).toString() ] = execNames;

            auto childValues = findDirAttributes( curr );
            for ( auto && ii : std::get< 0 >( childValues ) )
            {
                if ( buildDirs.find( ii ) == buildDirs.end() )
                    buildDirs.insert( ii );
            }
            for ( auto && ii : std::get< 1 >( childValues ) )
            {
                auto && curr = execNamesMap[ii.first];  // creates the list if necessary
                curr.insert( curr.end(), ii.second.begin(), ii.second.end() );
            }
        }
    }
    return std::make_tuple( buildDirs, execNamesMap );
}

void CMainWindow::addCustomBuild( const std::pair< QString, QString > & customBuild )
{
    auto dirItem = new QStandardItem( customBuild.first );
    auto targetItem = new QStandardItem( customBuild.second );
    fCustomBuildModel->appendRow( QList< QStandardItem * >( { dirItem, targetItem } ) );
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

QStringList argsFromString(const QString& argList)
{
    return argList.split(" ", TSkipEmptyParts);
}

QStringList envVarsFromString(const QString& envVarList)
{
    return envVarList.split(" ", TSkipEmptyParts);
}

QString stringFromEnvVars(const QStringList& envVars)
{
    QString retVal;
    for (auto&& ii : envVars)
    {
        auto curr = "{" + ii + "}";
        if (!retVal.isEmpty())
            retVal += " ";
        retVal += curr;
    }
    return retVal;
}

QString stringFromArgs(const QStringList& envVars)
{
    return envVars.join(" ");
}

std::list < NVSProjectMaker::SDebugTarget > CMainWindow::getDebugCommands( bool absDir ) const
{
    std::list < NVSProjectMaker::SDebugTarget > retVal;

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
        curr.fArgs = argsFromString( fDebugTargetsModel->item( ii, 3 )->text() );
        curr.fWorkDir = fDebugTargetsModel->item( ii, 4 )->text();
        curr.fEnvVars = envVarsFromString(fDebugTargetsModel->item(ii, 5)->text());
        retVal.push_back( curr );
    }
    return retVal;
}

std::list< std::pair< QString, QString > > CMainWindow::getCustomBuilds( bool absDir ) const
{
    std::list< std::pair< QString, QString > > retVal;

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
        retVal.push_back( std::pair( dir, target ) );
    }
    return retVal;
}

void CMainWindow::saveSettings()
{
    fSettings->setVSPath(fImpl->vsPath->text());
    fSettings->setUseCustomCMake(fImpl->useCustomCMake->isChecked());
    fSettings->setCustomCMakeExec(fImpl->cmakeExec->text());
    fSettings->setGenerator(fImpl->generator->currentText());
    fSettings->setClientDir(fImpl->clientDir->text());
    auto srcDir = getSourceDir(true);
    fSettings->setSourceRelDir(srcDir.has_value() ? srcDir.value() : QString());
    auto bldDir = getBuildDir(true);
    fSettings->setBuildRelDir(bldDir.has_value() ? bldDir.value() : QString());
    fSettings->setQtDir(fImpl->qtDir->text());
    fSettings->setProdDir(fImpl->prodDir->text());
    fSettings->setMSys64Dir(fImpl->msys64Dir->text());
    fSettings->setSelectedQtDirs(fQtLibsModel->getCheckedStrings());
    fSettings->setBuildOutputDataFile(fImpl->bldOutputFile->text());
    fSettings->setBldTxtProdDir(fImpl->origBldTxtProdDir->text());
    fSettings->setVerbose(fImpl->verbose->isChecked());

    auto attribs = findDirAttributes(nullptr);
    auto customBuilds = getCustomBuilds(false);
    auto dbgCommands = getDebugCommands(false);
    fSettings->setBuildDirs(std::get< 0 >(attribs));
    fSettings->setInclDirs(fIncDirModel->stringList());
    fSettings->setSelectedInclDirs(fIncDirModel->getCheckedStrings());
    fSettings->setPreProcDefines(fPreProcDefineModel->stringList());
    fSettings->setSelectedPreProcDefines(fPreProcDefineModel->getCheckedStrings());
    fSettings->setExecNames(std::get< 1 >(attribs));
    fSettings->setCustomBuilds(customBuilds);
    fSettings->setPrimaryTarget(fImpl->primaryBuildTarget->currentText());
    fSettings->setDebugCommands(dbgCommands);
    fSettings->saveSettings();
}


void CMainWindow::loadSettings()
{
    pushDisconnected();

    fImpl->vsPath->setText(fSettings->getVSPath());
    fImpl->useCustomCMake->setChecked(fSettings->getUseCustomCMake());
    fImpl->cmakeExec->setText(fSettings->getCustomCMakeExec());
    fImpl->generator->setCurrentText(fSettings->getGenerator());
    fImpl->clientDir->setText(fSettings->getClientDir());
    fImpl->buildRelDir->setText(fSettings->getBuildRelDir());
    fImpl->qtDir->setText(fSettings->getQtDir());
    fImpl->prodDir->setText(fSettings->getProdDir());
    fImpl->msys64Dir->setText(fSettings->getMSys64Dir());
    fImpl->origBldTxtProdDir->setText(fSettings->getBldTxtProdDir());
    fImpl->bldOutputFile->setText(fSettings->getBuildOutputDataFile());
    fImpl->verbose->setChecked(fSettings->getVerbose());

    fQtLibsModel->setStringList(fSettings->getQtDirs());
    fQtLibsModel->setChecked(fSettings->getSelectedQtDirs(), true, true);

    fIncDirModel->setStringList(fSettings->getInclDirs());
    auto selectedIncDirs = fSettings->getSelectedInclDirs();
    if (selectedIncDirs.isEmpty())
        fIncDirModel->checkAll(true);
    else
        fIncDirModel->setChecked(selectedIncDirs, true, true);

    fPreProcDefineModel->setStringList(fSettings->getPreProcDefines());
    auto selectedPreProcDefines = fSettings->getSelectedPreProcDefines();
    if (selectedPreProcDefines.isEmpty())
        fPreProcDefineModel->checkAll(true);
    else
        fPreProcDefineModel->setChecked(selectedPreProcDefines, true, true);

    fCustomBuildModel->clear();
    fCustomBuildModel->setHorizontalHeaderLabels(QStringList() << "Directory" << "Target Name");

    fDebugTargetsModel->clear();
    fDebugTargetsModel->setHorizontalHeaderLabels(QStringList() << "Source Dir" << "Name" << "Command" << "Args" << "Work Dir" << "EnvVars");

    fExecutables = fSettings->getExecNames();
    auto customBuilds = fSettings->getCustomBuilds();
    for (auto && ii : customBuilds)
        addCustomBuild(ii);

    slotBuildsChanged();
    fImpl->primaryBuildTarget->setCurrentText(fSettings->getPrimaryTarget());

    fImpl->sourceRelDir->setText(fSettings->getSourceRelDir());

    auto dbgCmds = fSettings->getDebugCommands();
    for (auto && ii : dbgCmds)
        addDebugTarget(ii);

    popDisconnected();
    slotChanged();
}

void CMainWindow::slotChanged()
{
    doChanged(true);
}

QString CMainWindow::getCMakeExec() const
{
    if (fImpl->useCustomCMake->isChecked())
        return fImpl->cmakeExec->text();
    else
        return fSettings->getCMakeExecViaVSPath(fImpl->vsPath->text());
}

void CMainWindow::doChanged( bool andLoad )
{
    auto clientDir = getClientDir();
    auto clientDirOK = clientDir.has_value();
    if ( clientDirOK )
    {
        fImpl->clientName->setText( clientDir.value().dirName() );
    }

    auto vsDir = QDir( fImpl->vsPath->text() );
    auto cmakeExecPath = QFileInfo(getCMakeExec());
    bool cmakePathOK = vsDir.exists() && cmakeExecPath.exists() && cmakeExecPath.isExecutable();
    if ( cmakePathOK )
    {
        slotVSChanged();
    }

    slotBuildsChanged();

    auto sourceDirPath = getSourceDir();
    auto fi = clientDirOK && sourceDirPath.has_value() ? QFileInfo( sourceDirPath.value() ) : QFileInfo();
    bool sourceDirOK = clientDirOK && getSourceDir().has_value() && fi.exists() && fi.isDir() && fi.isReadable();

    auto sourceDir = sourceDirPath.has_value() ? QDir( sourceDirPath.value() ) : QDir();
    if ( fSourceDir.has_value() && ( !sourceDirOK || ( fSourceDir.value() != sourceDir ) ) )
    {
        fSourceModel->clear();
    }

    bool loadSource = false;
    bool loadOutputData = false;
    if ( sourceDirOK && ( !fSourceDir.has_value() || ( fSourceDir.has_value() && ( fSourceDir != sourceDir ) ) ) )
    {
        fSourceDir = sourceDir;
        loadSource = true;
    }

    if ( !sourceDirOK )
        fSourceDir = QDir();

    auto buildTxtFile = fSettings->getBuildOutputDataFile();
    if ( !buildTxtFile.isEmpty() && QFileInfo( buildTxtFile ).exists() && QFileInfo( buildTxtFile ).isReadable() )
    {
        if ( !fBuildTextFile.has_value() || ( fBuildTextFile.has_value() && ( fBuildTextFile.value() != buildTxtFile ) ) )
        {
            fBuildTextFile = buildTxtFile;
            loadOutputData = true;
        }
    }

    if ( andLoad )
    {
        if ( loadSource && loadOutputData )
            QTimer::singleShot( 0, this, &CMainWindow::slotLoadSourceAndOutputData );
        else if ( loadSource )
            QTimer::singleShot( 0, this, &CMainWindow::slotLoadSource );
        else if ( loadOutputData )
            QTimer::singleShot( 0, this, &CMainWindow::slotLoadOutputData );
    }

    auto bldDir = getBuildDir();
    fi = clientDirOK && bldDir.has_value() ? QFileInfo( bldDir.value() ) : QFileInfo();
    bool bldDirOK = clientDirOK && bldDir.has_value() && fi.exists() && fi.isDir() && fi.isWritable();

    bool generatorOK = !fImpl->generator->currentText().isEmpty();

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

    auto clientInfoPage = new CClientInfoPage;
    wizard.addPage( clientInfoPage );
    clientInfoPage->setDefaults();

    auto sysInfoPage = new CSystemInfoPage;
    wizard.addPage( sysInfoPage );
    sysInfoPage->setDefaults();
    
    auto qtPage = new CQtPage;
    wizard.addPage( qtPage );
    qtPage->setDefaults();

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

        loadBuildTargetsFromWizard( buildTargetsPage->enabledBuildTargets(), buildTargetsPage->primaryTarget() );
        loadDebugTargetsFromWizard( debugTargetsPage->enabledDebugTargets( fSettings.get() ) );

        popDisconnected();

        doChanged( false );
        loadIncludeDirsFromWizard( includesPage->enabledIncludeDirs() );
        loadPreProcessorDefinesFromWizard( preProcDefinesPage->enabledPreProcDefines() );

        QString projFile = QDir( wizard.field( "clientDir" ).toString() ).absoluteFilePath( wizard.field( "clientName" ).toString() ) + ".vsprjmkr.json";
        saveProjectFile( projFile );

        slotLoadSource();
        doChanged( false );
    }
}

void CMainWindow::loadBuildTargetsFromWizard( const QStringList & targets, const QString & primaryTarget )
{
    for ( auto && ii : targets )
    {
        addCustomBuild( std::make_pair( fImpl->buildRelDir->text(), ii ) );
    }
    fImpl->primaryBuildTarget->clear();
    fImpl->primaryBuildTarget->addItems( targets );
    fImpl->primaryBuildTarget->setCurrentText( primaryTarget );
}

void CMainWindow::loadDebugTargetsFromWizard( const std::list< std::shared_ptr< SDebugTargetInfo > > & targets )
{
    for (auto&& ii : targets)
    {
        addDebugTarget( ii->fSourceDirPath, ii->fTargetName, ii->fExecutable, ii->fArgs, ii->fWorkingDir, ii->fEnvVars );
    }
}

void CMainWindow::loadIncludeDirsFromWizard( const QStringList & includeDirs )
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

void CMainWindow::loadPreProcessorDefinesFromWizard( const QStringList & preProcDefines )
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
    auto projFile = QFileDialog::getOpenFileName( this, tr( "Select Project File to open" ), currPath, "Project Files *.json;;All Files *.*" );
    if ( projFile.isEmpty() )
        return;
    setProjectFile( projFile, true );
}

void CMainWindow::slotSaveProjectFile()
{
    auto currPath = fImpl->projectFile->currentText();
    if ( currPath.isEmpty() )
        currPath = fSettings->fileName();
    if ( currPath.isEmpty() )
        currPath = QString();

    saveProjectFile( currPath );
}

void CMainWindow::saveProjectFile( const QString & currPath )
{
    auto projFile = QFileDialog::getSaveFileName( this, tr( "Select Project File to save" ), currPath, "Project Files *.json;;All Files *.*" );
    if ( !projFile.isEmpty() )
        setProjectFile( projFile, false );
}

void CMainWindow::slotCurrentProjectChanged( const QString & projFile )
{
    setProjectFile( projFile, true );
}

void CMainWindow::setProjectFile( const QString & projFile, bool forLoad )
{
    if ( projFile.isEmpty() )
        return;
    if ( !forLoad && !QFileInfo::exists( projFile ) )
    {
        QFile fi( projFile );
        fi.open( QFile::WriteOnly ); // force it to exist 
    }
    setCurrentProject( projFile );

    if ( forLoad )
    {
        if ( !fSettings->loadSettings( projFile ) )
        {
            QMessageBox::critical( this, tr( "Project File not Opened" ), QString( "Error: '%1' is not a valid project file" ).arg( projFile ) );
            return;
        }
    }
    else
        fSettings->setFileName( projFile, true );
    setWindowTitle( tr( "Visual Studio Project Generator - %1" ).arg( projFile ) );
    if ( forLoad )
        loadSettings();
    else
        saveSettings();
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

    fi = QFileInfo( NVSProjectMaker::CSettings::getCMakeExecViaVSPath( dir ) );
    if ( !fi.exists() || !fi.isExecutable() )
    {
        QMessageBox::critical( this, tr( "Error Valid Directory not Selected" ), QString( "Error: '%1' is not an executable" ).arg( fi.absoluteFilePath() ) );
        return;
    }
    fImpl->vsPath->setText( dir );
}

QProcess * CMainWindow::getProcess()
{
    if (!fProcess)
        fProcess = new QProcess(this);
    return fProcess;
}

void CMainWindow::clearProcess()
{
    delete fProcess;
    fProcess = nullptr;
}

void CMainWindow::slotVSChanged()
{
    if (getProcess() && (getProcess()->state() != QProcess::ProcessState::NotRunning))
    {
        QTimer::singleShot(500, this, &CMainWindow::slotVSChanged);
        return;
    }
    auto currPath = fImpl->vsPath->text();
    if ( currPath.isEmpty() )
        return;
    QFileInfo fi( currPath );
    if ( !fi.exists() || !fi.isDir() )
        return;

    auto cmakePath = getCMakeExec();
    if (cmakePath.isEmpty() )
        return;

    pushDisconnected();
    auto currText = fImpl->generator->currentText();
    if (currText.isEmpty())
        currText = fSettings->getGenerator();
    fImpl->generator->clear();

    QApplication::setOverrideCursor( Qt::WaitCursor );

    //process.setProcessChannelMode(QProcess::MergedChannels);
    getProcess()->start( cmakePath, QStringList() << "-help" );
    if ( !getProcess()->waitForFinished( -1 ) || ( getProcess()->exitStatus() != QProcess::NormalExit ) || ( getProcess()->exitCode() != 0 ) )
    {
        QMessageBox::critical( this, tr( "Error Running CMake" ), QString( "Error: '%1' Could not run cmake and determine Generators" ).arg( QString( getProcess()->readAllStandardError() ) ) );
        return;
    }
    auto data = QString( getProcess()->readAll() );
    clearProcess();
    auto lines = data.split( '\n', TSkipEmptyParts);
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

    if (currText.isEmpty())
    {
        auto vsPath = fSettings->getVSPath();
        auto whichVS = QDir(vsPath).dirName();
        for (auto && ii : generators)
        {
            if (ii.contains(whichVS))
            {
                currText = ii;
                break;
            }
        }
    }

    auto idx = fImpl->generator->findText( currText );
    if ( idx != -1 )
        fImpl->generator->setCurrentIndex( idx );
    popDisconnected();
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
        addDebugTarget(dlg.sourceDir(), dlg.name(), dlg.command(), dlg.args(), dlg.workDir(), dlg.envVars() );
    }
}

void CMainWindow::addDebugTarget( const NVSProjectMaker::SDebugTarget & cmd )
{
    addDebugTarget(cmd.fSourceDir, cmd.fName, cmd.fCmd, cmd.fArgs, cmd.fWorkDir, cmd.fEnvVars );
}

void CMainWindow::addDebugTarget( const QString & sourcePath, const QString & name, const QString & cmd, const QStringList & args, const QString & workDir, const QStringList & envVars )
{
    if ( !getSourceDir().has_value() )
        return;

    auto sourceDir = QDir( getSourceDir().value() );
    sourceDir.cdUp();
    auto sourceDirItem = new QStandardItem( sourceDir.relativeFilePath( sourcePath ) );
    auto nameItem = new QStandardItem( name );
    auto cmdItem = new QStandardItem( cmd );
    auto argsItem = new QStandardItem( stringFromArgs( args ) );
    auto workDirItem = new QStandardItem( workDir );
    auto envVarsItem = new QStandardItem(stringFromEnvVars( envVars ) );
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
        addCustomBuild( std::make_pair( dlg.buildDir(), dlg.targetName() ) );
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

void CMainWindow::slotSelectCMake()
{
    auto currPath = fImpl->cmakeExec->text();
    if (currPath.isEmpty())
        currPath = fSettings->getCMakeExec();

    auto newPath = QFileDialog::getOpenFileName(this, tr("Select CMake Executable"), currPath, tr("Executables *.exe"));
    if (newPath.isEmpty())
        return;

    QFileInfo fi(newPath);
    if ( !fi.exists() || !fi.isFile() || !fi.isExecutable() )
    {
        QMessageBox::critical(this, tr("Error Executable File not Selected"), QString("Error: '%1' is not an executable file").arg(newPath));
        return;
    }
    fImpl->cmakeExec->setText(newPath);
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

    fProgress = new QProgressDialog( tr( "Generating CMake Files..." ), tr( "Cancel" ), 0, 0, this );
    QPushButton * pb = new QPushButton( tr( "Cancel" ) );
    fProgress->setCancelButton( pb );
    fProgress->setWindowFlags( fProgress->windowFlags() & ~Qt::WindowCloseButtonHint );
    fProgress->setAutoReset( false );
    fProgress->setAutoClose( false );
    fProgress->setWindowModality( Qt::WindowModal );
    fProgress->setMinimumDuration( 1 );
    fProgress->setRange( 0, 100 );
    fProgress->setValue( 0 );
    qApp->processEvents();

    saveSettings();
    if ( !fSettings->generate( fProgress, this, [this]( const QString & msg ) { appendToLog( msg ); } ) )
        return;

    fProgress->setLabelText( tr( "Running CMake" ) );
    pb->setEnabled( false );
    fProgress->setRange( 0, 0 );

    fSettings->runCMake(
        [this]( const QString & outMsg )
    {
        appendToLog( outMsg );
    },
        [this]( const QString & errMsg )
    {
        appendToLog( "ERROR: " + errMsg );
    },
        getProcess()
        , { false, [this]() { QApplication::restoreOverrideCursor(); fProgress->deleteLater();  } });
}

void CMainWindow::slotLoadSourceAndOutputData()
{
    fLoadSourceAfterLoadData = true;
    QTimer::singleShot( 0, this, &CMainWindow::slotLoadOutputData );
}

void CMainWindow::slotLoadSource()
{
    fImpl->tabWidget->setCurrentIndex( 0 );
    fImpl->log->clear();

    auto text = fSourceDir.value().dirName();

    fSettings->getResults()->clear();
    fSettings->getResults()->fRootDir->fName = text;
    fSettings->getResults()->fRootDir->fIsDir = true;

    appendToLog( tr( "============================================" ) );
    appendToLog( tr( "Finding Source Files..." ) );
    auto progress = std::make_unique< QProgressDialog >( tr( "Finding Source Files..." ), tr( "Cancel" ), 0, 0, this );
    auto label = new QLabel;
    label->setAlignment( Qt::AlignLeft );
    progress->setLabel( label );
    progress->setAutoReset( false );
    progress->setAutoClose( false );
    progress->setWindowModality( Qt::WindowModal );
    progress->setMinimumDuration( 1 );
    progress->setRange( 0, 100 );
    progress->setValue( 0 );
    bool wasCanceled = fSettings->loadSourceFiles( fSourceDir.value(), getSourceDir().value(), progress.get(), [this]( const QString & msg ) { appendToLog( msg ); } );
    auto rootNode = loadSourceFileModel();

    addInclDirs( fSettings->getResults()->fInclDirs );
    appendToLog( tr( "============================================" ) );

    if ( wasCanceled )
    {
        fSourceModel->clear();
        appendToLog( tr( "Finding Source Files Canceled" ) );
        QMessageBox::warning( this, tr( "canceled" ), tr( "Finding Source files Canceled" ) );
    }
    else
    {
        qApp->processEvents();
        expandDirectories( rootNode );

        appendToLog( tr( "Finished Finding Source Files" ) );
        appendToLog( tr( "Results:" ) );
        appendToLog( fSettings->getResults()->getText( true ) );
    }
    fImpl->tabWidget->setCurrentIndex( 0 );
}

void CMainWindow::slotLoadOutputData()
{
    fImpl->tabWidget->setCurrentIndex( 1 );

    fBuildInfoDataModel->clear();
    fImpl->log->clear();

    auto progress = std::make_unique< QProgressDialog >( tr( "Reading Build Output..." ), tr( "Cancel" ), 0, 0, this );
    progress->setLabelText( "Reading Build Output" );
    progress->setAutoReset( false );
    progress->setAutoClose( false );
    progress->setWindowModality( Qt::WindowModal );
    progress->setMinimumDuration( 1 );
    progress->setRange( 0, 100 );
    progress->setValue( 0 );

    fBuildInfoData = std::make_shared< NVSProjectMaker::CBuildInfoData >( fImpl->bldOutputFile->text(),
                                                                          [this]( const QString & msg )
    {
        appendToLog( msg );
        qApp->processEvents();
    }, fSettings.get(), progress.get() );
    if ( !fBuildInfoData->status() )
    {
        progress->close();
        QMessageBox::critical( this, tr( "Could not read Output Data File" ), fBuildInfoData->errorString() );
        fBuildInfoData.reset();
        return;
    }
    fBuildInfoData->loadIntoTree( fBuildInfoDataModel );
    if ( !progress->wasCanceled() && fLoadSourceAfterLoadData )
    {
        QTimer::singleShot( 0, this, &CMainWindow::slotLoadSource );
        fLoadSourceAfterLoadData = false;
    }
}

void CMainWindow::slotBuildsChanged()
{
    auto builds = getCustomBuilds( false );
    auto list = QStringList() << QString();
    for ( auto && ii : builds )
    {
        list << ii.second;
    }
    auto curr = fImpl->primaryBuildTarget->currentText();
    fImpl->primaryBuildTarget->clear();
    fImpl->primaryBuildTarget->addItems( list );
    fImpl->primaryBuildTarget->setCurrentText( curr );
}



