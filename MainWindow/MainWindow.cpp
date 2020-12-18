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

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "SetupDebug.h"

#include "SABUtils/UtilityModels.h"

#include <QSettings>
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

Q_DECLARE_METATYPE( SDebugCmd );

QDataStream & operator<<( QDataStream & stream, const QSet<QString> & value )
{
    stream << value.count();
    for ( auto ii = value.constBegin(); ii != value.constEnd(); ++ii )
    {
        stream << *ii;
    }
    return stream;
}

QDataStream & operator>>( QDataStream & stream, QSet<QString> & value )
{
    int count;
    stream >> count;

    for ( int ii = 0; ii < count; ++ii )
    {
        QString key;
        stream >> key;
        value.insert( key );
    }
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const QPair< QString, bool > & value )
{
    stream << value.first << value.second;
    return stream;
}

QDataStream & operator>>( QDataStream & stream, QPair< QString, bool > & value )
{
    stream >> value.first;
    stream >> value.second;
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const QPair< QString, QString > & value )
{
    stream << value.first << value.second;
    return stream;
}

QDataStream & operator>>( QDataStream & stream, QPair< QString, QString > & value )
{
    stream >> value.first;
    stream >> value.second;
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const SDebugCmd & value )
{
    stream << value.fSourceDir
        << value.fName
        << value.fCmd
        << value.fArgs
        << value.fWorkDir
        << value.fEnvVars
        ;
    return stream;
}

QDataStream & operator>>( QDataStream & stream, SDebugCmd & value )
{
    stream >> value.fSourceDir;
    stream >> value.fName;
    stream >> value.fCmd;
    stream >> value.fArgs;
    stream >> value.fWorkDir;
    stream >> value.fEnvVars;

    return stream;
}

QDataStream & operator<<( QDataStream & stream, const QList< QPair< QString, bool > > & value )
{
    stream << value.count();
    for ( auto ii = value.constBegin(); ii != value.constEnd(); ++ii )
    {
        stream << *ii;
    }
    return stream;
}

QDataStream & operator>>( QDataStream & stream, QList< QPair< QString, bool > > & value )
{
    int count;
    stream >> count;

    for ( int ii = 0; ii < count; ++ii )
    {
        QPair< QString, bool > curr;
        stream >> curr;
        value.push_back( curr );
    }
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const QList< QPair< QString, QString > > & value )
{
    stream << value.count();
    for ( auto ii = value.constBegin(); ii != value.constEnd(); ++ii )
    {
        stream << *ii;
    }
    return stream;
}

QDataStream & operator>>( QDataStream & stream, QList< QPair< QString, QString > > & value )
{
    int count;
    stream >> count;

    for ( int ii = 0; ii < count; ++ii )
    {
        QPair< QString, QString > curr;
        stream >> curr;
        value.push_back( curr );
    }
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const QList< SDebugCmd > & value )
{
    stream << value.count();
    for ( auto ii = value.constBegin(); ii != value.constEnd(); ++ii )
    {
        stream << *ii;
    }
    return stream;
}

QDataStream & operator>>( QDataStream & stream, QList< SDebugCmd > & value )
{
    int count;
    stream >> count;

    for ( int ii = 0; ii < count; ++ii )
    {
        SDebugCmd curr;
        stream >> curr;
        value.push_back( curr );
    }
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const QHash< QString, QList< QPair< QString, bool > > > & value )
{
    stream << value.count();
    for ( auto ii = value.constBegin(); ii != value.constEnd(); ++ii )
    {
        stream << ii.key();
        stream << ii.value();
    }
    return stream;
}

QDataStream & operator>>( QDataStream & stream, QHash< QString, QList< QPair< QString, bool > > > & value )
{
    int count;
    stream >> count;

    for ( int ii = 0; ii < count; ++ii )
    {
        QString key;
        stream >> key;
        QList< QPair< QString, bool > > currValue;
        stream >> currValue;
        value.insert( key, currValue );
    }
    return stream;
}


CMainWindow::CMainWindow( QWidget * parent )
    : QDialog( parent ),
    fImpl( new Ui::CMainWindow ),
    fProcess( new QProcess( this ) )
{
    qRegisterMetaTypeStreamOperators< QSet<QString> >( "QSet<QString>" );
    qRegisterMetaTypeStreamOperators< QHash<QString, QList< QPair< QString, bool > >> >( "QHash<QString,QList<QPair<QString,bool>>>" );
    qRegisterMetaTypeStreamOperators< QList< QPair< QString, bool > > >( "QList<QPair<QString,bool>>" );
    qRegisterMetaTypeStreamOperators< QPair< QString, bool > >( "QPair<QString,bool>" );
    qRegisterMetaTypeStreamOperators< QList< QPair< QString, QString > > >( "QList<QPair<QString,QString>>" );
    qRegisterMetaTypeStreamOperators< QPair< QString, QString > >( "QPair<QString,QString>" );
    qRegisterMetaTypeStreamOperators< SDebugCmd > ( "SDebugCmd" );
    qRegisterMetaTypeStreamOperators< QList< SDebugCmd > >( "QList< SDebugCmd >" );

    fImpl->setupUi( this );
    connect( fImpl->cmakePath, &QLineEdit::textChanged, this, &CMainWindow::slotCMakeChanged );
    connect( fImpl->generator, &QComboBox::currentTextChanged, this, &CMainWindow::slotChanged );
    connect( fImpl->sourceDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
    connect( fImpl->destDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
    connect( fImpl->qtDir, &QLineEdit::textChanged, this, &CMainWindow::slotQtChanged );

    connect( fImpl->cmakePathBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectCMake );
    connect( fImpl->sourceDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectSourceDir );
    connect( fImpl->destDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectDestDir );
    connect( fImpl->qtDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectQtDir );
    connect( fImpl->addIncDirBtn, &QToolButton::clicked, this, &CMainWindow::slotAddIncDir );
    connect( fImpl->addCustomBuildBtn, &QToolButton::clicked, this, &CMainWindow::slotAddCustomBuild );
    connect( fImpl->addDebugCommandBtn, &QToolButton::clicked, this, &CMainWindow::slotAddDebugCommand );
    
    
    connect( fImpl->generateBtn, &QToolButton::clicked, this, &CMainWindow::slotGenerate );
    


    fSourceModel = new QStandardItemModel( this );
    fImpl->sourceTree->setModel( fSourceModel );

    fIncDirModel = new QStringListModel( this );
    fImpl->incPaths->setModel( fIncDirModel );

    fQtLibsModel = new CCheckableStringListModel( this );
    fImpl->qtLibs->setModel( fQtLibsModel );

    fCustomBuildModel = new QStandardItemModel( this );
    fCustomBuildModel->setHorizontalHeaderLabels( QStringList() << "Directory" << "Target Name" );
    fImpl->customBuilds->setModel( fCustomBuildModel );

    fDebugCommandsModel = new QStandardItemModel( this );
    fDebugCommandsModel->setHorizontalHeaderLabels( QStringList() << "Source Dir" << "Name" << "Command" << "Args" << "Work Dir" << "EnvVars" );
    fImpl->debugCmds->setModel( fDebugCommandsModel );

    QTimer::singleShot( 0, this, &CMainWindow::loadSettings );
    QTimer::singleShot( 0, this, &CMainWindow::loadQtSettings );
}

CMainWindow::~CMainWindow()
{
    saveSettings();
}

QString CMainWindow::readResourceFile( QWidget * parent, const QString & resourceFile, const std::function< void( QString & data ) > & function )
{
    QFile fi( resourceFile );
    if ( !fi.open( QIODevice::OpenModeFlag::ReadOnly | QIODevice::OpenModeFlag::Text ) )
    {
        QApplication::restoreOverrideCursor();
        QMessageBox::critical( parent, tr( "Error:" ), tr( "Error opening resource file '%1'\n%2" ).arg( resourceFile ).arg( fi.errorString() ) );
        return QString();
    }
    auto resourceText = QString( fi.readAll() );
    function( resourceText );
    return resourceText;
}

std::tuple< QSet< QString >, QHash< QString, QList< QPair< QString, bool > > > > CMainWindow::findDirAttributes( QStandardItem * parent ) const
{
    QSet< QString > buildDirs;
    QHash< QString, QList< QPair< QString, bool > > > execNamesMap;

    int numRows = parent ? parent->rowCount() : fSourceModel->rowCount();
    if ( parent && !parent->data( ERoles::eIsDirRole ).toBool() )
        return {};

    for ( int ii = 0; ii < numRows; ++ii )
    {
        auto curr = parent ? parent->child( ii ) : fSourceModel->item( ii );
        if ( curr->data( ERoles::eIsDirRole ).toBool() )
        {
            if ( curr->data( ERoles::eIsBuildDir ).toBool() )
                buildDirs.insert( curr->data( ERoles::eRelPath ).toString() );

            auto execNames = curr->data( ERoles::eExecutables ).value < QList< QPair< QString, bool > > >();
            if ( !execNames.isEmpty() )
                execNamesMap.insert( curr->data( ERoles::eRelPath ).toString(), execNames );

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

QList < SDebugCmd > CMainWindow::getDebugCommandsForSourceDir( const QString & inSourcePath ) const
{
    QList < SDebugCmd > retVal;

    QDir sourceDir( fImpl->sourceDir->text() );
    sourceDir.cdUp();
    for ( int ii = 0; ii < fDebugCommandsModel->rowCount(); ++ii )
    {
        //( QStringList() << "Source Dir" << "Name" << "Command" << "Args" << "Work Dir" << "EnvVars" )
        SDebugCmd curr;
        auto sourcePath = fDebugCommandsModel->item( ii, 0 )->text();
        curr.fSourceDir  = QFileInfo( sourceDir.absoluteFilePath( sourcePath ) ).canonicalFilePath();
        curr.fName = fDebugCommandsModel->item( ii, 1 )->text();
        curr.fCmd = fDebugCommandsModel->item( ii, 2 )->text();
        curr.fArgs = fDebugCommandsModel->item( ii, 3 )->text();
        curr.fWorkDir = fDebugCommandsModel->item( ii, 4 )->text();
        curr.fEnvVars = fDebugCommandsModel->item( ii, 5 )->text();

        if ( curr.fSourceDir == inSourcePath )
        {
            retVal << curr;
        }
    }
    return retVal;
}

QList < SDebugCmd > CMainWindow::getDebugCommands( bool absDir ) const
{
    QList < SDebugCmd > retVal;

    QDir sourceDir( fImpl->sourceDir->text() );
    sourceDir.cdUp();
    for ( int ii = 0; ii < fDebugCommandsModel->rowCount(); ++ii )
    {
        //( QStringList() << "Source Dir" << "Name" << "Command" << "Args" << "Work Dir" << "EnvVars" )
        SDebugCmd curr;
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
    QDir sourceDir( fImpl->sourceDir->text() );
    sourceDir.cdUp();
    QStringList retVal;
    for ( int ii = 0; ii < fCustomBuildModel->rowCount(); ++ii )
    {
        auto currSourceDir = sourceDir.relativeFilePath( fCustomBuildModel->item( ii, 0 )->text() );
        if ( currSourceDir == inSourcePath )
            retVal << fCustomBuildModel->item( ii, 1 )->text();
    }
    return retVal;
}

QList< QPair< QString, QString > > CMainWindow::getCustomBuilds( bool absDir ) const
{
    QList< QPair< QString, QString > > retVal;

    QDir srcDir( fImpl->sourceDir->text() );
    srcDir.cdUp();

    for ( int ii = 0; ii < fCustomBuildModel->rowCount(); ++ii )
    {
        auto dir = fCustomBuildModel->item( ii, 0 )->text();
        if ( absDir )
            dir = srcDir.absoluteFilePath( dir );
        auto target = fCustomBuildModel->item( ii, 1 )->text();
        retVal << qMakePair( dir, target );
    }
    return retVal;
}

void CMainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue( "CMakePath", fImpl->cmakePath->text() );
    settings.setValue( "Generator", fImpl->generator->currentText() );
    settings.setValue( "SourceDir", fImpl->sourceDir->text() );
    settings.setValue( "DestDir", fImpl->destDir->text() );
    settings.setValue( "QtDir", fImpl->qtDir->text() );
    settings.setValue( "QtLibs", fQtLibsModel->getCheckedStrings() );

    auto attribs = findDirAttributes( nullptr );
    auto inclDirs = fIncDirModel->stringList();
    auto customBuilds = getCustomBuilds( false );
    auto dbgCommands = getDebugCommands( false );
    settings.setValue( "BuildDirs", QVariant::fromValue( std::get< 0 >( attribs ) ) );
    settings.setValue( "InclDirs", QVariant::fromValue( inclDirs ) );
    settings.setValue( "ExecNames", QVariant::fromValue( std::get< 1 >( attribs ) ) );
    settings.setValue( "CustomBuilds", QVariant::fromValue( customBuilds ) );
    settings.setValue( "DebugCommands", QVariant::fromValue( dbgCommands ) );
}


void CMainWindow::loadSettings()
{
    QSettings settings;
    disconnect( fImpl->qtDir, &QLineEdit::textChanged, this, &CMainWindow::slotQtChanged );

    fImpl->cmakePath->setText( settings.value( "CMakePath" ).toString() );
    fImpl->generator->setCurrentText( settings.value( "Generator" ).toString() );
    fImpl->destDir->setText( settings.value( "DestDir" ).toString() );
    fImpl->qtDir->setText( settings.value( "QtDir" ).toString() );
   
    fQtLibs = settings.value( "QtLibs" ).toStringList();
    connect( fImpl->qtDir, &QLineEdit::textChanged, this, &CMainWindow::slotQtChanged );
    loadQtSettings();

    fBuildDirs = settings.value( "BuildDirs" ).value< QSet< QString > >();
    fInclDirs = settings.value( "InclDirs" ).toStringList();
    fIncDirModel->setStringList( fInclDirs );
    fExecutables = settings.value( "Executables" ).value< QHash< QString, QList< QPair< QString, bool > > > >();
    auto customBuilds = settings.value( "CustomBuilds" ).value< QList< QPair< QString, QString > > >();
    for ( auto && ii : customBuilds )
        addCustomBuild( ii );

    auto dbgCmds = settings.value( "DebugCommands" ).value< QList< SDebugCmd > >();
    for ( auto && ii : dbgCmds )
        addDebugCommand( ii );

    fImpl->sourceDir->setText( settings.value( "SourceDir" ).toString() );
    slotChanged();
}

void CMainWindow::slotChanged()
{
    QFileInfo fi( fImpl->cmakePath->text() );
    bool cmakePathOK = !fImpl->cmakePath->text().isEmpty() && fi.exists() && fi.isExecutable();

    bool generatorOK = !fImpl->generator->currentText().isEmpty();

    fi = QFileInfo( fImpl->sourceDir->text() );
    bool sourceDirOK = !fImpl->sourceDir->text().isEmpty() && fi.exists() && fi.isDir() && fi.isReadable();

    auto sourceDir = QDir( fImpl->sourceDir->text() );
    if ( fSourceDir.has_value() && ( !sourceDirOK || ( fSourceDir.value() != sourceDir ) ) )
    {
        fSourceModel->clear();
    }
    if ( sourceDirOK && ( fSourceDir.has_value() && ( fSourceDir != sourceDir ) ) )
    {
        fSourceDir = sourceDir;
        sourceDir.cdUp();
        fImpl->clientName->setText( sourceDir.dirName() );

        QTimer::singleShot( 0, this, &CMainWindow::slotLoadSource );
    }

    if ( !sourceDirOK )
        fSourceDir = QDir();

    fi = QFileInfo( fImpl->destDir->text() );
    bool destDirOK = !fImpl->destDir->text().isEmpty() && fi.exists() && fi.isDir() && fi.isWritable();

    fImpl->generateBtn->setEnabled( cmakePathOK && generatorOK && sourceDirOK && destDirOK );
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
    auto currPath = fImpl->sourceDir->text();
    if ( currPath.isEmpty() )
        currPath = QString();
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Source Directory" ), currPath );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    fImpl->sourceDir->setText( dir );
}

void CMainWindow::slotSelectDestDir()
{
    auto currPath = fImpl->destDir->text();
    if ( currPath.isEmpty() )
        currPath = QString();
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Destination Directory" ), currPath );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isWritable() )
    {
        QMessageBox::critical( this, tr( "Error Writable Directory not Selected" ), QString( "Error: '%1' is not an writeable directory" ).arg( dir ) );
        return;
    }

    fImpl->destDir->setText( dir );
}

void CMainWindow::slotAddDebugCommand()
{
    CSetupDebug dlg( fImpl->sourceDir->text(), fImpl->destDir->text(), this );
    if ( dlg.exec() == QDialog::Accepted && !dlg.command().isEmpty() && !dlg.sourceDir().isEmpty() )
    {
        addDebugCommand( dlg.sourceDir(), dlg.name(), dlg.command(), dlg.args(), dlg.workDir(), dlg.envVars() );
    }
}

void CMainWindow::addDebugCommand( const SDebugCmd & cmd )
{
    addDebugCommand( cmd.fSourceDir, cmd.fName, cmd.fCmd, cmd.fArgs, cmd.fWorkDir, cmd.fEnvVars );
}

void CMainWindow::addDebugCommand( const QString & sourcePath, const QString & name, const QString & cmd, const QString & args, const QString & workDir, const QString & envVars )
{
    auto sourceDir = QDir( fImpl->sourceDir->text() );
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
    auto currPath = fImpl->sourceDir->text();
    if ( currPath.isEmpty() )
        currPath = QString();
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Directory to Build" ), currPath );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    auto target = QInputDialog::getText( this, tr( "Target Name" ), tr( "Target Name:" ) );
    if ( target.isEmpty() )
        return;

    auto relDir = QDir( fImpl->sourceDir->text() );
    relDir.cdUp();
    auto relDirPath = relDir.relativeFilePath( dir );
    addCustomBuild( qMakePair( relDirPath, target ) );
}

void CMainWindow::slotAddIncDir()
{
    auto currPath = fImpl->sourceDir->text();
    if ( currPath.isEmpty() )
        currPath = QString();
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Directory to Include" ), currPath );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    auto curr = fIncDirModel->stringList();
    dir = QDir( fImpl->sourceDir->text() ).relativeFilePath( dir );
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
        node->setData( curr.isDir(), ERoles::eIsDirRole );
        node->setData( relDirPath, ERoles::eRelPath );
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


            node->setData( isBuildDir, ERoles::eIsBuildDir );
            node->setData( isInclDir, ERoles::eIsIncludeDir );
            node->setData( QVariant::fromValue( execList ), ERoles::eExecutables );

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

    QSettings settings;
    fImpl->qtDir->setText( settings.value( "QtDir" ).toString() );
    fQtLibs = settings.value( "QtLibs" ).toStringList();

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
    fQtLibsModel->setChecked( fQtLibs, true, true );
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
    rootNode->setData( true, ERoles::eIsDirRole );
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
    bool wasCancelled = loadSourceFiles( fImpl->sourceDir->text(), rootNode, progress, results );
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
}

bool CMainWindow::expandDirectories( QStandardItem * node )
{
    auto num = node->rowCount();
    bool hasDir = false;
    for ( int ii = 0; ii < num; ++ii )
    {
        auto child = node->child( ii );
        bool isDir = child->data( ERoles::eIsDirRole ).toBool();
        hasDir = hasDir || isDir;
        if ( isDir )
            expandDirectories( child );
    }
    if ( hasDir )
        fImpl->sourceTree->setExpanded( node->index(), true );
    return hasDir;
}

std::list< SDirInfo > CMainWindow::getDirInfo( QStandardItem * parent, QProgressDialog * progress ) const
{
    std::list< SDirInfo > retVal;

    int numRows = parent ? parent->rowCount() : fSourceModel->rowCount();
    if ( parent && !parent->data( ERoles::eIsDirRole ).toBool() )
        return {};

    progress->setRange( 0, numRows );
    QDir sourceDir( fImpl->sourceDir->text() );

    for ( int ii = 0; ii < numRows; ++ii )
    {
        progress->setValue( ii );
        qApp->processEvents();
        if ( progress->wasCanceled() )
            return {};

        auto curr = parent ? parent->child( ii ) : fSourceModel->item( ii );
        if ( !curr->data( ERoles::eIsDirRole ).toBool() )
        {
            continue;
        }

        SDirInfo currInfo( curr );
        currInfo.fExtraTargets  = getCustomBuildsForSourceDir( QFileInfo( sourceDir.absoluteFilePath( currInfo.fRelToDir ) ).canonicalFilePath() );
        currInfo.fDebugCommands = getDebugCommandsForSourceDir( QFileInfo( sourceDir.absoluteFilePath( currInfo.fRelToDir ) ).canonicalFilePath() );
        if ( currInfo.isValid() )
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

    QDir sourceDir( fImpl->sourceDir->text() );
    auto inclDirs = fIncDirModel->stringList();
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
        ii.writeCMakeFile( fImpl->destDir->text() );
        ii.writePropSheet( fImpl->sourceDir->text(), fImpl->destDir->text(), inclDirs );
        ii.createDebugProjects( fImpl->destDir->text() );
    }
    progress.setRange( 0, static_cast<int>( dirs.size() ) );
    appendToLog( tr( "============================================" ) );
    appendToLog( tr( "Running CMake" ) );

    progress.close();

    connect( fProcess, &QProcess::readyReadStandardError, this, &CMainWindow::slotReadStdErr );
    connect( fProcess, &QProcess::readyReadStandardOutput, this, &CMainWindow::slotReadStdOut );
    connect( fProcess, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ), this, &CMainWindow::slotFinished );

    auto buildDir = fImpl->destDir->text();
    auto cmakeExec = fImpl->cmakePath->text();
    auto args = getCmakeArgs();
    appendToLog( tr( "Build Dir: %1" ).arg( buildDir ) );
    appendToLog( tr( "CMake Path: %1" ).arg( cmakeExec ) );
    appendToLog( tr( "Args: %1" ).arg( args.join( " " ) ) );

    fProcess->setWorkingDirectory( buildDir );
    fProcess->start( cmakeExec, args );

    QApplication::restoreOverrideCursor();
}

std::list< SDirInfo > CMainWindow::generateTopLevelFiles( QProgressDialog * progress )
{
    readResourceFile( this, QString( ":/resources/Project.cmake" ),
                      [this]( QString & text )
    {
        QString outFile = QDir( fImpl->destDir->text() ).absoluteFilePath( "Project.cmake" );
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

    auto topDirText = readResourceFile( this, QString( ":/resources/%1.txt" ).arg( "topdir" ),
                                        [this]( QString & text )
    {
        text.replace( "%CLIENT_NAME%", fImpl->clientName->text() );
        text.replace( "%ROOT_SOURCE_DIR%", fImpl->sourceDir->text() );
    }
    );

    QString outFile = QDir( fImpl->destDir->text() ).absoluteFilePath( "CMakeLists.txt" );
    QFile fo( outFile );
    if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
    {
        QApplication::restoreOverrideCursor();
        QMessageBox::critical( this, tr( "Error:" ), tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
        return {};
    }

    QTextStream qts( &fo );
    qts << topDirText;

    auto customTopBuilds = getCustomBuildsForSourceDir( QFileInfo( fImpl->sourceDir->text() ).fileName() );
    if ( !customTopBuilds.isEmpty() )
    {
        for ( auto && ii : customTopBuilds )
        {
            qts << QString( "add_subdirectory( CustomBuild/%1 )\n" ).arg( ii );

            if ( !QDir( fImpl->destDir->text() ).mkpath( QString( "CustomBuild/%1" ).arg( ii ) ) )
            {
                QApplication::restoreOverrideCursor();
                QMessageBox::critical( this, tr( "Error:" ), tr( "Error creating directory '%1" ).arg( QString( "CustomBuild/%1" ).arg( ii ) ) );
                return {};
            }
            QString fileName = QString( "CustomBuild/%1/CMakeLists.txt" ).arg( ii );
            auto topDirCustomBuildTxt = readResourceFile( this, QString( ":/resources/custombuilddir.txt" ),
                                                [ii, fileName, this]( QString & text )
            {
                text.replace( "%PROJECT_NAME%", ii );
                text.replace( "%BUILD_DIR%", fImpl->destDir->text() );
                text.replace( "%SOURCE_FILES%", QString() );
                text.replace( "%HEADER_FILES%", QString() );
                text.replace( "%UI_FILES%", QString() );
                text.replace( "%QRC_FILES%", QString() );
                text.replace( "%OTHER_FILES%", QString() );

                QString outFile = QDir( fImpl->destDir->text() ).absoluteFilePath( fileName );
                QFile fo( outFile );
                if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
                {
                    QApplication::restoreOverrideCursor();
                    QMessageBox::critical( this, tr( "Error:" ), tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
                    return;
                }
                QTextStream qts( &fo );
                qts << text;
                qts << "\n\nset_target_properties( " << ii << " PROPERTIES FOLDER " << "\"Top Level Targets\"" << " )\n";
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
        auto subDirs = ii.getSubDirs();
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

SDirInfo::SDirInfo( QStandardItem * item ) :
    fIsInclDir( item->data( CMainWindow::ERoles::eIsIncludeDir ).toBool() ),
    fIsBuildDir( item->data( CMainWindow::ERoles::eIsBuildDir ).toBool() ),
    fExecutables( item->data( CMainWindow::ERoles::eExecutables ).value< QList< QPair< QString, bool > > >() )
{
    computeRelToDir( item );
    getFiles( item );
}

void SDirInfo::computeRelToDir( QStandardItem * item )
{
    fRelToDir = item->data( CMainWindow::ERoles::eRelPath ).toString();
    fProjectName = fRelToDir;
    fProjectName.replace( "/", "_" );
    fProjectName = fProjectName.replace( "\\", "_" );
    fProjectName = fProjectName.replace( " ", "_" );
}

bool SDirInfo::isValid() const
{
    if ( fRelToDir.isEmpty() )
        return false;

    return ( fIsInclDir || fIsBuildDir || !fExecutables.isEmpty() || !fExtraTargets.isEmpty() || !fDebugCommands.isEmpty() );
}

void SDirInfo::replaceFiles( QString & text, const QString & variable, const QStringList & files ) const
{
    QStringList values;
    for ( auto && ii : files )
    {
        values << QString( "    \"${ROOT_SOURCE_DIR}/%1\"" ).arg( ii );
    }
    auto value = values.join( "\n" );
    text.replace( variable, value );
}

QString SDebugCmd::getEnvVars() const
{
    QStringList tmp;
    bool inParen = false;
    QString curr;
    for ( int ii = 0; ii < fEnvVars.length(); ++ii )
    {
        auto currChar = fEnvVars[ ii ];
        if ( inParen )
        {
            if ( currChar == '}' )
            {
                inParen = false;
                curr = curr.trimmed();
                tmp << curr;
                curr.clear();
            }
            else
                curr += currChar;
        }
        else
            inParen = currChar == '{';
    }
    if ( !curr.isEmpty() )
        tmp << curr;
    auto retVal = tmp.join( '\n' );
    return retVal;
}

QString SDebugCmd::getProjectName() const
{
    QString retVal = fName;
    retVal.replace( "/", "_" );
    retVal = retVal.replace( "\\", "_" );
    retVal = retVal.replace( " ", "_" );
    return retVal;
}

QStringList SDirInfo::getSubDirs() const
{
    QStringList retVal;
    retVal << fRelToDir;
    for ( auto && ii : fDebugCommands )
    {
        retVal << QString( "%1/DebugDir/%2" ).arg( fRelToDir ).arg( ii.getProjectName() );
    }
    return retVal;
}
void SDirInfo::createDebugProjects( const QString & bldDir ) const
{
    for ( auto && ii : fDebugCommands )
    {
        if ( !QDir( bldDir ).mkpath( QString( "%1/DebugDir/%2" ).arg( fRelToDir ).arg( ii.getProjectName() ) ) )
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical( nullptr, QObject::tr( "Error:" ), QObject::tr( "Error creating directory '%1" ).arg( QString( "CustomBuild/%1" ).arg( ii.fCmd ) ) );
            return;
        }

        CMainWindow::readResourceFile( nullptr, QString( ":/resources/PropertySheetWithDebug.props" ),
                                       [this, bldDir, ii]( QString & text )
        {
            QDir bldQDir( bldDir );
            bldQDir.cd( fRelToDir );
            bldQDir.cd( "DebugDir" );
            bldQDir.cd( ii.getProjectName() );

            QString outFile = bldQDir.absoluteFilePath( "PropertySheetWithDebug.props" );
            QFile fo( outFile );
            if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
            {
                QApplication::restoreOverrideCursor();
                QMessageBox::critical( nullptr, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
                return;
            }

            text.replace( "%PROJECT_NAME%", ii.getProjectName() );
            text.replace( "%DEBUG_COMMAND%", ii.fCmd );
            text.replace( "%DEBUG_ARGS%", ii.fArgs );
            text.replace( "%DEBUG_WORKDIR%", ii.fWorkDir );
            text.replace( "%DEBUG_ENVVARS%", ii.getEnvVars() );
            QTextStream qts( &fo );
            qts << text;
            fo.close();
        } 
        );
        CMainWindow::readResourceFile( nullptr, QString( ":/resources/subdebugdir.txt" ),
                                        [this, bldDir, ii]( QString & text )
        {
            QDir bldQDir( bldDir );
            bldQDir.cd( fRelToDir );
            bldQDir.cd( "DebugDir" );
            bldQDir.cd( ii.getProjectName() );

            QString outFile = bldQDir.absoluteFilePath( "CMakeLists.txt" );
            QFile fo( outFile );
            if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
            {
                QApplication::restoreOverrideCursor();
                QMessageBox::critical( nullptr, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
                return;
            }

            text.replace( "%PROJECT_NAME%", ii.getProjectName() );
            auto outDir = QDir( bldDir );
            auto outPath = outDir.absoluteFilePath( fRelToDir );
            text.replace( "%BUILD_DIR%", outPath );
            text.replace( "%DEBUG_COMMAND%", ii.fCmd );
            text.replace( "%PROPSFILENAME%", "PropertySheetWithDebug.props" );
            
            QTextStream qts( &fo );
            qts << text;
            qts << "\n\nset_target_properties( " << ii.getProjectName() << " PROPERTIES FOLDER " << "\"Debug Targets\"" << " )\n";
            fo.close();
        }
        );

    }
}

void SDirInfo::writePropSheet( const QString & srcDir, const QString & bldDir, const QString & includeDirs ) const
{
    QString fileName = "PropertySheetIncludes.props";
    CMainWindow::readResourceFile( nullptr, QString( ":/resources/%1" ).arg( fileName ),
                      [this, srcDir, bldDir, includeDirs, fileName ]( QString & text )
    {
        QDir bldQDir( bldDir );
        bldQDir.cd( fRelToDir );
        QString outFile = bldQDir.absoluteFilePath( fileName );
        QFile fo( outFile );
        if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical( nullptr, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
            return;
        }
        QString lclIncDirs = QDir( srcDir ).absoluteFilePath( fRelToDir ) + ";" + includeDirs;
        text.replace( "%INCLUDE_DIRS%", lclIncDirs );
        QTextStream qts( &fo );
        qts << text;
        fo.close();
    }
    );
}

void SDirInfo::writeCMakeFile( const QString & bldDir ) const
{
    auto outDir = QDir( bldDir );
    auto outPath = outDir.absoluteFilePath( fRelToDir );

    QString resourceFile = ( !fIsBuildDir && fExecutables.isEmpty() && fSourceFiles.isEmpty() ) ? "subheaderdir.txt" : "subbuilddir.txt";
    auto resourceText = CMainWindow::readResourceFile( nullptr, QString( ":/resources/%1" ).arg( resourceFile ),
                                        [this, outPath]( QString & resourceText )
    {
        resourceText.replace( "%PROJECT_NAME%", fProjectName );
        resourceText.replace( "%BUILD_DIR%", outPath );
        replaceFiles( resourceText, "%SOURCE_FILES%", fSourceFiles );
        replaceFiles( resourceText, "%HEADER_FILES%", fHeaderFiles );
        replaceFiles( resourceText, "%UI_FILES%", fUIFiles );
        replaceFiles( resourceText, "%QRC_FILES%", fQRCFiles );
        replaceFiles( resourceText, "%OTHER_FILES%", fOtherFiles );
        resourceText.replace( "%PROPSFILENAME%", "PropertySheetIncludes.props" );
    }
    );

    if ( !outDir.cd( fRelToDir ) )
    {
        if ( !outDir.mkpath( fRelToDir ) || !outDir.cd( fRelToDir ) )
        {
            QMessageBox::critical( nullptr, QObject::tr( "Error:" ), QObject::tr( "Could not create missing output directory '%1'" ).arg( outPath ) );
            return;
        }
    }

    QString outFile = outDir.absoluteFilePath( QString( "CMakeLists.txt" ) );
    QFile fo( outFile );
    if ( !fo.open( QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::Text ) )
    {
        QMessageBox::critical( nullptr, QObject::tr( "Error:" ), QObject::tr( "Error opening output file '%1'\n%2" ).arg( outFile ).arg( fo.errorString() ) );
        return;
    }

    QTextStream qts( &fo );
    qts << resourceText;

    QString folder = fRelToDir;
    QFileInfo( fRelToDir ).path();
    if ( fIsBuildDir || fExecutables.isEmpty() )
    {
        qts << "set_target_properties( " << fProjectName << " PROPERTIES FOLDER " << folder << " )\n";
    }
    else
    {
        for ( auto && ii : fExecutables )
        {
            qts << "set_target_properties( " << fProjectName << " PROPERTIES FOLDER " << folder << " )\n";
        }
    }
}

void SDirInfo::addDependencies( QTextStream & qts ) const
{
    qts << "                 ${project_SRCS}\n"
        << "                 ${project_H}\n"
        << "                 ${qtproject_UIS}\n"
        << "                 ${qtproject_QRC}\n"
        << "                 ${otherFILES}\n"
        ;
}

void SDirInfo::getFiles( QStandardItem * parent )
{
    if ( !parent || !parent->data( CMainWindow::ERoles::eIsDirRole ).toBool() )
        return;

    int numRows = parent->rowCount();
    for ( int ii = 0; ii < numRows; ++ii )
    {
        auto curr = parent->child( ii );
        if ( curr->data( CMainWindow::ERoles::eIsDirRole ).toBool() )
            continue;

        addFile( curr->data( CMainWindow::ERoles::eRelPath ).toString() );
    }
}

void SDirInfo::addFile( const QString & path )
{
    static std::unordered_set< QString > headerSuffixes =
    {
        "h", "hh", "hpp", "hxx"
    };
    static std::unordered_set< QString > sourceSuffixes =
    {
        "c", "cxx", "cpp"
    };
    QFileInfo fi( path );
    auto suffix = fi.suffix().toLower();
    if ( suffix == "ui" )
        fUIFiles << path;
    else if ( suffix == "qrc" )
        fQRCFiles << path;
    else if ( headerSuffixes.find( suffix ) != headerSuffixes.end() )
        fHeaderFiles << path;
    else if ( sourceSuffixes.find( suffix ) != sourceSuffixes.end() )
        fSourceFiles << path;
    else
        fOtherFiles << path;
}
