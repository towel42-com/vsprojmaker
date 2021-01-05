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

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "SetupDebug.h"
#include <QSet>
#include "AddCustomBuild.h"

#include "MainLib/VSProjectMaker.h"
#include "MainLib/DebugCmd.h"
#include "MainLib/DirInfo.h"
#include "MainLib/Settings.h"

#include "SABUtils/UtilityModels.h"

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

CMainWindow::CMainWindow( QWidget * parent )
    : QDialog( parent ),
    fImpl( new Ui::CMainWindow ),
    fProcess( new QProcess( this ) ),
    fSettings( new NVSProjectMaker::CSettings )
{
    NVSProjectMaker::registerTypes();

    fImpl->setupUi( this );
    connect( fImpl->projectFile, &QLineEdit::textChanged, this, &CMainWindow::slotProjectFileChanged );
    connect( fImpl->cmakePath, &QLineEdit::textChanged, this, &CMainWindow::slotCMakeChanged );
    connect( fImpl->generator, &QComboBox::currentTextChanged, this, &CMainWindow::slotChanged );
    connect( fImpl->clientDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
    connect( fImpl->sourceRelDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
    connect( fImpl->buildRelDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
    connect( fImpl->qtDir, &QLineEdit::textChanged, this, &CMainWindow::slotQtChanged );

    connect( fImpl->projectFileBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectProjectFile );
    connect( fImpl->cmakePathBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectCMake );
    connect( fImpl->clientDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectClientDir );
    connect( fImpl->sourceDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectSourceDir );
    connect( fImpl->buildDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectBuildDir );
    connect( fImpl->qtDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectQtDir );
    connect( fImpl->addIncDirBtn, &QToolButton::clicked, this, &CMainWindow::slotAddIncDir );
    connect( fImpl->addCustomBuildBtn, &QToolButton::clicked, this, &CMainWindow::slotAddCustomBuild );
    connect( fImpl->addDebugCommandBtn, &QToolButton::clicked, this, &CMainWindow::slotAddDebugCommand );
    
    
    connect( fImpl->generateBtn, &QToolButton::clicked, this, &CMainWindow::slotGenerate );
    


    fSourceModel = new QStandardItemModel( this );
    fImpl->sourceTree->setModel( fSourceModel );

    fIncDirModel = new CCheckableStringListModel( this );
    fImpl->incPaths->setModel( fIncDirModel );

    fQtLibsModel = new CCheckableStringListModel( this );
    fImpl->qtLibs->setModel( fQtLibsModel );

    fCustomBuildModel = new QStandardItemModel( this );
    fCustomBuildModel->setHorizontalHeaderLabels( QStringList() << "Directory" << "Target Name" );
    fImpl->customBuilds->setModel( fCustomBuildModel );

    fDebugCommandsModel = new QStandardItemModel( this );
    fDebugCommandsModel->setHorizontalHeaderLabels( QStringList() << "Source Dir" << "Name" << "Command" << "Args" << "Work Dir" << "EnvVars" );
    fImpl->debugCmds->setModel( fDebugCommandsModel );

    fImpl->projectFile->setFocus();
    QTimer::singleShot( 0, this, &CMainWindow::loadSettings );
    QTimer::singleShot( 0, this, &CMainWindow::loadQtSettings );
}

CMainWindow::~CMainWindow()
{
    saveSettings();
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
            if ( curr->data( NVSProjectMaker::ERoles::eIsBuildDir ).toBool() )
                buildDirs.insert( curr->data( NVSProjectMaker::ERoles::eRelPath ).toString() );

            auto execNames = curr->data( NVSProjectMaker::ERoles::eExecutables ).value < QList< QPair< QString, bool > > >();
            if ( !execNames.isEmpty() )
                execNamesMap.insert( curr->data( NVSProjectMaker::ERoles::eRelPath ).toString(), execNames );

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

QList < NVSProjectMaker::SDebugCmd > CMainWindow::getDebugCommandsForSourceDir( const QString & inSourcePath ) const
{
    QList < NVSProjectMaker::SDebugCmd > retVal;

    auto sourceDirPath = getSourceDir();
    if ( !sourceDirPath.has_value() )
        return {};
    QDir sourceDir( sourceDirPath.value() );
    sourceDir.cdUp();
    for ( int ii = 0; ii < fDebugCommandsModel->rowCount(); ++ii )
    {
        //( QStringList() << "Source Dir" << "Name" << "Command" << "Args" << "Work Dir" << "EnvVars" )
        NVSProjectMaker::SDebugCmd curr;
        auto sourcePath = fDebugCommandsModel->item( ii, 0 )->text();
        curr.fSourceDir  = QFileInfo( sourceDir.absoluteFilePath( sourcePath ) ).canonicalFilePath();
        curr.fName = fDebugCommandsModel->item( ii, 1 )->text();
        curr.fCmd = fDebugCommandsModel->item( ii, 2 )->text();
        curr.fArgs = fDebugCommandsModel->item( ii, 3 )->text();
        curr.fWorkDir = fDebugCommandsModel->item( ii, 4 )->text();
        curr.fEnvVars = fDebugCommandsModel->item( ii, 5 )->text();

        QMap< QString, QString > map =
        {
              { "CLIENTDIR", getClientDir().value().absolutePath() }
            , { "SOURCEDIR", getSourceDir().value() }
            , { "BUILDDIR", getBuildDir().value() }
        };
        curr.cleanUp( map );

        if ( curr.fSourceDir == inSourcePath )
        {
            retVal << curr;
        }
    }
    return retVal;
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

std::optional< QString > CMainWindow::getDir( const QLineEdit * lineEdit, bool relPath ) const
{
    if ( relPath )
        return lineEdit->text();

    auto clientDir = getClientDir();
    if ( !clientDir.has_value() )
        return {};

    if ( lineEdit->text().isEmpty() )
        return {};

    if ( !QFileInfo( clientDir.value().absoluteFilePath( lineEdit->text() ) ).exists() )
        return {};

    return clientDir.value().absoluteFilePath( lineEdit->text() );
}

std::optional< QString > CMainWindow::getSourceDir( bool relPath ) const
{
    return getDir( fImpl->sourceRelDir, relPath );
}

std::optional< QString > CMainWindow::getBuildDir( bool relPath ) const
{
    return getDir( fImpl->buildRelDir, relPath );
}

QList < NVSProjectMaker::SDebugCmd > CMainWindow::getDebugCommands( bool absDir ) const
{
    QList < NVSProjectMaker::SDebugCmd > retVal;

    auto sourceDirPath = getSourceDir();
    if ( !sourceDirPath.has_value() )
        return {};
    QDir sourceDir( sourceDirPath.value() );
    sourceDir.cdUp();
    for ( int ii = 0; ii < fDebugCommandsModel->rowCount(); ++ii )
    {
        //( QStringList() << "Source Dir" << "Name" << "Command" << "Args" << "Work Dir" << "EnvVars" )
        NVSProjectMaker::SDebugCmd curr;
        curr.fSourceDir = fDebugCommandsModel->item( ii, 0 )->text();
        if ( absDir )
            curr.fSourceDir = sourceDir.absoluteFilePath( curr.fSourceDir );
        curr.fName = fDebugCommandsModel->item( ii, 1 )->text();
        curr.fCmd = fDebugCommandsModel->item( ii, 2 )->text();
        curr.fArgs = fDebugCommandsModel->item( ii, 3 )->text();
        curr.fWorkDir = fDebugCommandsModel->item( ii, 4 )->text();
        curr.fEnvVars = fDebugCommandsModel->item( ii, 5 )->text();
        retVal << curr;
    }
    return retVal;
}

QStringList CMainWindow::getCustomBuildsForSourceDir( const QString & inSourcePath ) const
{
    auto sourceDirPath = getSourceDir();
    if ( !sourceDirPath.has_value() )
        return {};

    QDir sourceDir( sourceDirPath.value() );
    auto srcPath = sourceDir.relativeFilePath( inSourcePath );

    auto buildDirPath = getBuildDir();
    if ( !buildDirPath.has_value() )
        return {};
    QDir buildDir( buildDirPath.value() );
    buildDir.cdUp();
    buildDir.cdUp();

    QStringList retVal;
    for ( int ii = 0; ii < fCustomBuildModel->rowCount(); ++ii )
    {
        auto currBuildDirPath = buildDir.absoluteFilePath( fCustomBuildModel->item( ii, 0 )->text() );
        auto currRelBuildDir = QDir( buildDirPath.value() ).relativeFilePath( currBuildDirPath );
        if ( currRelBuildDir == srcPath )
            retVal << fCustomBuildModel->item( ii, 1 )->text();
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
    fSettings->setCMakePath( fImpl->cmakePath->text() );
    fSettings->setGenerator( fImpl->generator->currentText() );
    fSettings->setClientDir( fImpl->clientDir->text() );
    auto srcDir = getSourceDir( true );
    fSettings->setSourceRelDir( srcDir.has_value() ? srcDir.value() : QString() );
    auto bldDir = getBuildDir( true );
    fSettings->setBuildRelDir( bldDir.has_value() ? bldDir.value() : QString() );
    fSettings->setQtDir( fImpl->qtDir->text() );
    fSettings->setQtLibs( fQtLibsModel->getCheckedStrings() );

    auto attribs = findDirAttributes( nullptr );
    auto customBuilds = getCustomBuilds( false );
    auto dbgCommands = getDebugCommands( false );
    fSettings->setBuildDirs( std::get< 0 >( attribs ) );
    fSettings->setInclDirs( fIncDirModel->stringList() );
    fSettings->setSelectedInclDirs( fIncDirModel->getCheckedStrings() );
    fSettings->setExecNames( std::get< 1 >( attribs ) );
    fSettings->setCustomBuilds( customBuilds );
    fSettings->setDebugCommands( dbgCommands );
}


void CMainWindow::loadSettings()
{
    disconnect( fImpl->qtDir, &QLineEdit::textChanged, this, &CMainWindow::slotQtChanged );

    fImpl->cmakePath->setText( fSettings->getCMakePath() );
    fImpl->generator->setCurrentText( fSettings->getGenerator() );
    fImpl->clientDir->setText( fSettings->getClientDir() );
    fImpl->buildRelDir->setText( fSettings->getBuildRelDir() );
    fImpl->qtDir->setText( fSettings->getQtDir() );
   
    connect( fImpl->qtDir, &QLineEdit::textChanged, this, &CMainWindow::slotQtChanged );
    loadQtSettings();

    fBuildDirs = fSettings->getBuildDirs();
    fInclDirs = fSettings->getInclDirs();
    fIncDirModel->setStringList( fInclDirs );
    auto selectedIncDirs = fSettings->getSelectedInclDirs();
    if ( selectedIncDirs.isEmpty() )
        fIncDirModel->checkAll( true );
    else
        fIncDirModel->setChecked( selectedIncDirs, true, true );

    fExecutables = fSettings->getExecNames();
    auto customBuilds = fSettings->getCustomBuilds();
    for ( auto && ii : customBuilds )
        addCustomBuild( ii );

    fImpl->sourceRelDir->setText( fSettings->getSourceRelDir() );

    auto dbgCmds = fSettings->getDebugCommands();
    for ( auto && ii : dbgCmds )
        addDebugCommand( ii );

    slotChanged();
}

void CMainWindow::slotChanged()
{
    QFileInfo fi( fImpl->cmakePath->text() );
    bool cmakePathOK = !fImpl->cmakePath->text().isEmpty() && fi.exists() && fi.isExecutable();

    bool generatorOK = !fImpl->generator->currentText().isEmpty();

    auto clientDir = getClientDir();
    auto clientDirOK = clientDir.has_value();
    if ( clientDirOK )
    {
        fImpl->clientName->setText( clientDir.value().dirName() );
    }
    
    auto sourceDirPath = getSourceDir();
    fi = clientDirOK && sourceDirPath.has_value() ? QFileInfo( sourceDirPath.value() ) : QFileInfo();
    bool sourceDirOK = clientDirOK && getSourceDir().has_value() && fi.exists() && fi.isDir() && fi.isReadable();

    auto sourceDir = sourceDirPath.has_value() ? QDir( sourceDirPath.value() ) : QDir();
    if ( fSourceDir.has_value() && ( !sourceDirOK || ( fSourceDir.value() != sourceDir ) ) )
    {
        fSourceModel->clear();
    }
    if ( sourceDirOK && ( fSourceDir.has_value() && ( fSourceDir != sourceDir ) ) )
    {
        fSourceDir = sourceDir;
        QTimer::singleShot( 0, this, &CMainWindow::slotLoadSource );
    }

    if ( !sourceDirOK )
        fSourceDir = QDir();

    auto bldDir = getBuildDir();
    fi = clientDirOK && bldDir.has_value() ? QFileInfo( bldDir.value() ) : QFileInfo();
    bool bldDirOK = clientDirOK && bldDir.has_value() && fi.exists() && fi.isDir() && fi.isWritable();

    fImpl->sourceDirBtn->setEnabled( clientDirOK );
    fImpl->buildDirBtn->setEnabled( clientDirOK );
    fImpl->addDebugCommandBtn->setEnabled( sourceDirOK && bldDirOK );
    fImpl->addIncDirBtn->setEnabled( sourceDirOK );
    fImpl->generateBtn->setEnabled( cmakePathOK && clientDirOK && generatorOK && sourceDirOK && bldDirOK );
}

void CMainWindow::slotSelectProjectFile()
{
    auto currPath = fImpl->projectFile->text();
    if ( currPath.isEmpty() )
        currPath = fSettings->fileName();
    if ( currPath.isEmpty() )
        currPath = QString();
    auto projFile = QFileDialog::getOpenFileName( this, tr( "Select Project File" ), currPath, "Project Files *.ini;;All Files *.*" );
    if ( projFile.isEmpty() )
        return;

    fImpl->projectFile->setText( projFile );
}

void CMainWindow::slotProjectFileChanged()
{
    auto projFile = fImpl->projectFile->text();
    if ( !fSettings->loadSettings( projFile ) )
    {
        QMessageBox::critical( this, tr( "Project File not Opened" ), QString( "Error: '%1' is not a valid project file" ).arg( projFile ) );
        return;
    }

    setWindowTitle( tr( "Visual Studio Project Generator - %1" ).arg( projFile ) );
    loadSettings();
}

void CMainWindow::slotSelectCMake()
{
    auto currPath = fImpl->cmakePath->text();
    if ( currPath.isEmpty() )
        currPath = QString();
    auto exe = QFileDialog::getOpenFileName( this, tr( "Select CMake Executable" ), currPath, "All Executables *.exe;;All Files *.*" );
    if ( exe.isEmpty() )
        return;

    QFileInfo fi( exe );
    if ( !fi.exists() || !fi.isExecutable() )
    {
        QMessageBox::critical( this, tr( "Error Executable not Selected" ), QString( "Error: '%1' is not an executable" ).arg( exe ) );
        return;
    }

    fImpl->cmakePath->setText( exe );
}

void CMainWindow::slotCMakeChanged()
{
    QFileInfo fi( fImpl->cmakePath->text() );
    if ( !fi.exists() || !fi.isExecutable() )
        return;

    disconnect( fImpl->generator, &QComboBox::currentTextChanged, this, &CMainWindow::slotChanged );
    auto currText = fImpl->generator->currentText();
    fImpl->generator->clear();

    QApplication::setOverrideCursor( Qt::WaitCursor );

    //process.setProcessChannelMode(QProcess::MergedChannels);
    fProcess->start( fImpl->cmakePath->text(), QStringList() << "-help" );
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
    connect( fImpl->generator, &QComboBox::currentTextChanged, this, &CMainWindow::slotChanged );

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

void CMainWindow::slotAddDebugCommand()
{
    CSetupDebug dlg( getSourceDir().value(), getBuildDir().value(), this );
    if ( dlg.exec() == QDialog::Accepted && !dlg.command().isEmpty() && !dlg.sourceDir().isEmpty() )
    {
        addDebugCommand( dlg.sourceDir(), dlg.name(), dlg.command(), dlg.args(), dlg.workDir(), dlg.envVars() );
    }
}

void CMainWindow::addDebugCommand( const NVSProjectMaker::SDebugCmd & cmd )
{
    addDebugCommand( cmd.fSourceDir, cmd.fName, cmd.fCmd, cmd.fArgs, cmd.fWorkDir, cmd.fEnvVars );
}

void CMainWindow::addDebugCommand( const QString & sourcePath, const QString & name, const QString & cmd, const QString & args, const QString & workDir, const QString & envVars )
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
    fDebugCommandsModel->appendRow( QList< QStandardItem * >() << sourceDirItem << nameItem << cmdItem << argsItem << workDirItem << envVarsItem );
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

void CMainWindow::slotAddIncDir()
{
    auto currPath = getSourceDir();
    if ( !currPath.has_value() )
        currPath = QString();
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Directory to Include" ), currPath.value() );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    auto curr = fIncDirModel->stringList();
    dir = QDir( currPath.value() ).relativeFilePath( dir );
    curr.push_front( dir );
    fIncDirModel->setStringList( curr );
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
void CMainWindow::incProgress( QProgressDialog * progress ) const
{
    progress->setValue( progress->value() + 1 );
    //if ( progress->value() >= 100 )
    //    progress->setValue( 1 );
    qApp->processEvents();
}

bool CMainWindow::isBuildDir( const QDir & relToDir, const QDir & dir ) const
{
    bool retVal = QFileInfo( relToDir.absoluteFilePath( dir.filePath( "subdir.mk" ) ) ).exists();
    retVal = retVal || QFileInfo( relToDir.absoluteFilePath( dir.filePath( "Makefile" ) ) ).exists();
    retVal = retVal || fBuildDirs.contains( dir.path() );
    return retVal;
}

bool CMainWindow::isInclDir( const QDir & relToDir, const QDir & dir ) const
{
    bool retVal = dir.dirName() == "incl";
    retVal = retVal || fInclDirs.contains( dir.path() );
    if ( !retVal )
    {
        QDirIterator di( relToDir.absoluteFilePath( dir.path() ), { "*.h", "*.hh", "*.hpp" }, QDir::Files );
        retVal = di.hasNext();
    }
    return retVal;
}

QList< QPair< QString, bool > > CMainWindow::getExecutables( const QDir & dir ) const
{
    auto pos = fExecutables.find( dir.path() );
    if ( pos != fExecutables.end() )
        return pos.value();
    return {};
}

bool CMainWindow::loadSourceFiles( const QString & dir, QStandardItem * parent, QProgressDialog * progress, SSourceFileInfo & results )
{
    QDir baseDir( dir );
    if ( !baseDir.exists() )
        return false;

    auto relDirPath = fSourceDir.value().relativeFilePath( dir );
    appendToLog( relDirPath );
    progress->setLabelText( tr( "Finding Source Files...\n%1" ).arg( relDirPath ) );
    progress->adjustSize();
    progress->setRange( 0, baseDir.count() );
    incProgress( progress );

    QDirIterator iter( dir, QStringList(), QDir::Filter::AllDirs | QDir::Filter::Files | QDir::Filter::NoDotAndDotDot | QDir::Filter::Readable, QDirIterator::IteratorFlag::NoIteratorFlags );
    while ( iter.hasNext() )
    {
        incProgress( progress );
        if ( progress->wasCanceled() )
            return true;

        auto curr = QFileInfo( iter.next() );
        relDirPath = fSourceDir.value().relativeFilePath( curr.absoluteFilePath() );
        auto node = new QStandardItem( relDirPath );
        node->setData( curr.isDir(), NVSProjectMaker::ERoles::eIsDirRole );
        node->setData( relDirPath, NVSProjectMaker::ERoles::eRelPath );
        QList<QStandardItem *> row;
        row << node;
        if ( !curr.isDir() )
        {
            parent->appendRow( node );
            results.fFiles++;
        }
        else
        {
            results.fDirs++;
            // determine if its a build dir, has an exec name (or names), or include path
            bool isBuildDir = CMainWindow::isBuildDir( fSourceDir.value(), relDirPath );
            bool isInclDir = CMainWindow::isInclDir( fSourceDir.value(), relDirPath );
            auto execList = CMainWindow::getExecutables( relDirPath );


            node->setData( isBuildDir, NVSProjectMaker::ERoles::eIsBuildDir );
            node->setData( isInclDir, NVSProjectMaker::ERoles::eIsIncludeDir );
            node->setData( QVariant::fromValue( execList ), NVSProjectMaker::ERoles::eExecutables );

            QStringList execNames;
            for ( auto && ii : execList )
                execNames << ii.first;
            QStringList guiExecs;
            for ( auto && ii : execList )
                guiExecs << ( ii.second ? "Yes" : "No" );

            if ( isBuildDir )
                results.fBuildDirs.push_back( relDirPath );
            if ( isInclDir )
                results.fInclDirs.push_back( relDirPath );
            results.fExecutables << execList;

            auto buildNode = new QStandardItem( isBuildDir ? "Yes" : "" );
            auto execNode = new QStandardItem( execNames.join( " " ) );
            auto inclDir = new QStandardItem( isInclDir ? "Yes" : "" );
            row << buildNode << execNode << inclDir;

            if ( parent )
                parent->appendRow( row );
            else
                fSourceModel->appendRow( row );

            if ( loadSourceFiles( curr.absoluteFilePath(), node, progress, results ) )
                return true;
        }

    }
    return progress->wasCanceled();
}

void CMainWindow::appendToLog( const QString & txt )
{
    fImpl->log->appendPlainText( txt.trimmed() );
}

void CMainWindow::slotQtChanged()
{
    disconnect( fImpl->qtDir, &QLineEdit::textChanged, this, &CMainWindow::slotQtChanged );

    saveSettings();
    fQtLibsModel->setStringList( QStringList() );

    fImpl->qtDir->setText( fSettings->getQtDir() );

    loadQtSettings();
    connect( fImpl->qtDir, &QLineEdit::textChanged, this, &CMainWindow::slotQtChanged );
}

void CMainWindow::loadQtSettings()
{
    if ( fImpl->qtDir->text().isEmpty() )
        return;

    auto qtDir = QDir( fImpl->qtDir->text() );
    if ( !qtDir.exists() )
        return;

    if ( !qtDir.cd( "include" ) )
        return;

    QDirIterator iter( qtDir.absolutePath(), QStringList(), QDir::Filter::AllDirs | QDir::Filter::NoDotAndDotDot | QDir::Filter::Readable, QDirIterator::IteratorFlag::NoIteratorFlags );
    QStringList qtDirs;
    while ( iter.hasNext() )
    {
        qtDirs << QFileInfo( iter.next() ).fileName();
    }
    fQtLibsModel->setStringList( qtDirs );
    fQtLibsModel->setChecked( fSettings->getQtLibs(), true, true );
}

void CMainWindow::addInclDirs( const QStringList & inclDirs )
{
    auto current = fIncDirModel->stringList();
    for ( auto && ii : inclDirs )
    {
        if ( !current.contains( ii ) )
            current << ii;
    }

    fIncDirModel->setStringList( current );
}
void CMainWindow::slotLoadSource()
{
    fImpl->log->clear();
    fSourceModel->clear();
    fSourceModel->setHorizontalHeaderLabels( QStringList() << "Name" << "Is Build Dir?" << "Exec Name" << "Is Include Path?" );

    auto text = fSourceDir.value().dirName();
    auto rootNode = new QStandardItem( text );
    rootNode->setData( true, NVSProjectMaker::ERoles::eIsDirRole );
    fSourceModel->appendRow( rootNode );

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
    SSourceFileInfo results;
    bool wasCancelled = loadSourceFiles( getSourceDir().value(), rootNode, progress, results );
    addInclDirs( results.fInclDirs );
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
        appendToLog( results.getText( true ) );
        //QMessageBox::information( this, tr( "Results" ), results.getText( false ) );
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

std::list< std::shared_ptr< NVSProjectMaker::SDirInfo > > CMainWindow::getDirInfo( QStandardItem * parent, QProgressDialog * progress ) const
{
    std::list< std::shared_ptr< NVSProjectMaker::SDirInfo > > retVal;

    int numRows = parent ? parent->rowCount() : fSourceModel->rowCount();
    if ( parent && !parent->data( NVSProjectMaker::ERoles::eIsDirRole ).toBool() )
        return {};

    progress->setRange( 0, numRows );
    QDir sourceDir( getSourceDir().value() );

    for ( int ii = 0; ii < numRows; ++ii )
    {
        progress->setValue( ii );
        qApp->processEvents();
        if ( progress->wasCanceled() )
            return {};

        auto curr = parent ? parent->child( ii ) : fSourceModel->item( ii );
        if ( !curr->data( NVSProjectMaker::ERoles::eIsDirRole ).toBool() )
        {
            continue;
        }

        auto currInfo = std::make_shared< NVSProjectMaker::SDirInfo >( curr );
        currInfo->fExtraTargets  = getCustomBuildsForSourceDir( QFileInfo( sourceDir.absoluteFilePath( currInfo->fRelToDir ) ).canonicalFilePath() );
        currInfo->fDebugCommands = getDebugCommandsForSourceDir( QFileInfo( sourceDir.absoluteFilePath( currInfo->fRelToDir ) ).canonicalFilePath() );
        if ( currInfo->isValid() )
        {
            retVal.push_back( currInfo );
        }
        auto children = getDirInfo( curr, progress );
        retVal.insert( retVal.end(), children.begin(), children.end() );
    }
    if ( progress->wasCanceled() )
        return {};

    return retVal;
}

QStringList CMainWindow::getCmakeArgs() const
{
    QStringList retVal;

    auto whichGenerator = fImpl->generator->currentText();

    auto pos = whichGenerator.indexOf( "=" );
    if ( pos != -1 )
        whichGenerator = whichGenerator.left( pos );

    whichGenerator.replace( "[arch]", "Win64" );
    whichGenerator = whichGenerator.trimmed();

    retVal << "-G" << whichGenerator << ".";
    return retVal;
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

    QProgressDialog progress( tr( "Generating CMake Files..." ), tr( "Cancel" ), 0, 0, this );
    progress.setAutoReset( false );
    progress.setAutoClose( false );
    progress.setWindowModality( Qt::WindowModal );
    progress.setMinimumDuration( 1 );
    progress.setRange( 0, 100 );
    progress.setValue( 0 );
    qApp->processEvents();

    auto dirs = generateTopLevelFiles( &progress );

    progress.setRange( 0, static_cast<int>( dirs.size() ) );
    appendToLog( tr( "============================================" ) );
    appendToLog( tr( "Generating CMake Files" ) );
    progress.setLabelText( tr( "Generating CMake Files" ) );
    progress.adjustSize();
    qApp->processEvents();

    auto inclDirs = getIncludeDirs();
    int currDir = 0;
    for ( auto && ii : dirs )
    {
        progress.setValue( currDir++ );
        if ( progress.wasCanceled() )
            break;
        ii->writeCMakeFile( this, getBuildDir().value() );
        ii->writePropSheet( this, getSourceDir().value(), getBuildDir().value(), inclDirs );
        ii->createDebugProjects( this, getBuildDir().value() );
    }
    progress.setRange( 0, static_cast<int>( dirs.size() ) );
    appendToLog( tr( "============================================" ) );
    appendToLog( tr( "Running CMake" ) );

    progress.close();

    connect( fProcess, &QProcess::readyReadStandardError, this, &CMainWindow::slotReadStdErr );
    connect( fProcess, &QProcess::readyReadStandardOutput, this, &CMainWindow::slotReadStdOut );
    connect( fProcess, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ), this, &CMainWindow::slotFinished );

    auto buildDir = getBuildDir().value();
    auto cmakeExec = fImpl->cmakePath->text();
    auto args = getCmakeArgs();
    appendToLog( tr( "Build Dir: %1" ).arg( buildDir ) );
    appendToLog( tr( "CMake Path: %1" ).arg( cmakeExec ) );
    appendToLog( tr( "Args: %1" ).arg( args.join( " " ) ) );

    fProcess->setWorkingDirectory( buildDir );
    fProcess->start( cmakeExec, args );

    QApplication::restoreOverrideCursor();
}

std::list< std::shared_ptr< NVSProjectMaker::SDirInfo > > CMainWindow::generateTopLevelFiles( QProgressDialog * progress )
{
    NVSProjectMaker::readResourceFile( this, QString( ":/resources/Project.cmake" ),
                      [this]( QString & text )
    {
        QString outFile = QDir( getBuildDir().value() ).absoluteFilePath( "Project.cmake" );
        QFile fo( outFile );
        if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical( this, tr( "Error:" ), tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
            return;
        }
        QTextStream qts( &fo );
        qts << text;
        fo.close();
    }
    );

    auto topDirText = NVSProjectMaker::readResourceFile( this, QString( ":/resources/%1.txt" ).arg( "topdir" ),
                                        [this]( QString & text )
    {
        text.replace( "%CLIENT_NAME%", fImpl->clientName->text() );
        text.replace( "%ROOT_SOURCE_DIR%", getSourceDir().value() );
    }
    );

    QString outFile = QDir( getBuildDir().value() ).absoluteFilePath( "CMakeLists.txt" );
    QFile fo( outFile );
    if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
    {
        QApplication::restoreOverrideCursor();
        QMessageBox::critical( this, tr( "Error:" ), tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
        return {};
    }

    QTextStream qts( &fo );
    qts << topDirText;

    auto customTopBuilds = getCustomBuildsForSourceDir( QFileInfo( getSourceDir().value() ).absoluteFilePath() );
    if ( !customTopBuilds.isEmpty() )
    {
        for ( auto && ii : customTopBuilds )
        {
            qts << QString( "add_subdirectory( CustomBuild/%1 )\n" ).arg( ii );

            if ( !QDir( getBuildDir().value() ).mkpath( QString( "CustomBuild/%1" ).arg( ii ) ) )
            {
                QApplication::restoreOverrideCursor();
                QMessageBox::critical( this, tr( "Error:" ), tr( "Error creating directory '%1" ).arg( QString( "CustomBuild/%1" ).arg( ii ) ) );
                return {};
            }
            QString fileName = QString( "CustomBuild/%1/CMakeLists.txt" ).arg( ii );
            auto topDirCustomBuildTxt = NVSProjectMaker::readResourceFile( this, QString( ":/resources/custombuilddir.txt" ),
                                                [ii, fileName, this]( QString & text )
            {
                text.replace( "%PROJECT_NAME%", ii );
                text.replace( "%BUILD_DIR%", getBuildDir().value() );
                text.replace( "%SOURCE_FILES%", QString() );
                text.replace( "%HEADER_FILES%", QString() );
                text.replace( "%UI_FILES%", QString() );
                text.replace( "%QRC_FILES%", QString() );
                text.replace( "%OTHER_FILES%", QString() );

                QString outFile = QDir( getBuildDir().value() ).absoluteFilePath( fileName );
                QFile fo( outFile );
                if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
                {
                    QApplication::restoreOverrideCursor();
                    QMessageBox::critical( this, tr( "Error:" ), tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
                    return;
                }
                QTextStream qts( &fo );
                qts << text;
                qts << "\n\nset_target_properties( " << ii << " PROPERTIES FOLDER " << "\"CMakePredefinedTargets/Top Level Targets\"" << " )\n";
                fo.close();
            }
            );
        }
        qts << "\n\n";
    }

    qApp->processEvents();
    appendToLog( tr( "============================================" ) );
    appendToLog( tr( "Finding Directories" ) );
    progress->setLabelText( tr( "Finding Directories" ) );
    progress->adjustSize();
    auto dirs = getDirInfo( nullptr, progress );
    if ( progress->wasCanceled() )
    {
        QApplication::restoreOverrideCursor();
        return {};
    }
    qts << "#######################################################\n"
        << "##### Sub Directories\n"
        ;

    for ( auto && ii : dirs )
    {
        auto subDirs = ii->getSubDirs();
        for( auto && jj : subDirs )
            qts << QString( "add_subdirectory( %1 )\n" ).arg( jj );
    }
    return dirs;
}

void CMainWindow::slotFinished( int exitCode, QProcess::ExitStatus status )
{
    disconnect( fProcess, &QProcess::readyReadStandardError, this, &CMainWindow::slotReadStdErr );
    disconnect( fProcess, &QProcess::readyReadStandardOutput, this, &CMainWindow::slotReadStdOut );
    disconnect( fProcess, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ), this, &CMainWindow::slotFinished );
    appendToLog( tr( "============================================" ) );
    appendToLog( QString( "Finished: Exit Code: %1 Status: %2 " ).arg( exitCode ).arg( status == QProcess::NormalExit ? tr( "Normal" ) : tr( "Crashed" ) ) );
}

void CMainWindow::slotReadStdErr()
{
    appendToLog( "ERROR: " + fProcess->readAllStandardError() );
}

void CMainWindow::slotReadStdOut()
{
    appendToLog( fProcess->readAllStandardOutput() );
}

QString CMainWindow::SSourceFileInfo::getText( bool forText ) const
{
    QString retVal = ( forText ?
        QObject::tr( "Files: %1 Directories: %2 Executables: %3 Include Directories: %4 Build Directories: %5" ) :
        QObject::tr(
            "<ul>"
                "<li>Files: %1</li>"
                "<li>Directories: %2</li>"
                "<li>Executables: %3</li>"
                "<li>Include Directories: %4</li>"
                "<li>Build Directories: %5</li>"
            "</ul>"
        ) 
    )   .arg( fFiles )
        .arg( fDirs )
        .arg( fExecutables.count() )
        .arg( fInclDirs.count() )
        .arg( fBuildDirs.count() )
        ;
    return retVal;
}



