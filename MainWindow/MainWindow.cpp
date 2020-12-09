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
    fImpl( new Ui::CMainWindow )
{
    qRegisterMetaTypeStreamOperators< QSet<QString> >( "QSet<QString>" );
    qRegisterMetaTypeStreamOperators< QHash<QString, QList< QPair< QString, bool > >> >( "QHash<QString,QList<QPair<QString,bool>>>" );
    qRegisterMetaTypeStreamOperators< QList< QPair< QString, bool > > >( "QList<QPair<QString,bool>>" );
    qRegisterMetaTypeStreamOperators< QPair< QString, bool > >( "QPair<QString,bool>" );

    fImpl->setupUi( this );
    connect( fImpl->cmakePath, &QLineEdit::textChanged, this, &CMainWindow::slotCMakeChanged );
    connect( fImpl->generator, &QComboBox::currentTextChanged, this, &CMainWindow::slotChanged );
    connect( fImpl->sourceDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
    connect( fImpl->destDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );

    connect( fImpl->cmakePathBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectCMake );
    connect( fImpl->sourceDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectSourceDir );
    connect( fImpl->destDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectDestDir );
    connect( fImpl->generateBtn, &QToolButton::clicked, this, &CMainWindow::slotGenerate );


    fSourceModel = new QStandardItemModel( this );
    fImpl->sourceTree->setModel( fSourceModel );

    QTimer::singleShot( 0, this, &CMainWindow::loadSettings );
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

void CMainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue( "CMakePath", fImpl->cmakePath->text() );
    settings.setValue( "Generator", fImpl->generator->currentText() );
    settings.setValue( "SourceDir", fImpl->sourceDir->text() );
    settings.setValue( "DestDir", fImpl->destDir->text() );

    auto attribs = findDirAttributes( nullptr );
    settings.setValue( "BuildDirs", QVariant::fromValue( std::get< 0 >( attribs ) ) );
    settings.setValue( "InclDirs", QVariant::fromValue( std::get< 1 >( attribs ) ) );
    settings.setValue( "ExecNames", QVariant::fromValue( std::get< 2 >( attribs ) ) );
}

std::tuple< QSet< QString >, QSet< QString >, QHash< QString, QList< QPair< QString, bool > > > > CMainWindow::findDirAttributes( QStandardItem * parent ) const
{
    QSet< QString > buildDirs;
    QSet< QString > inclDirs;
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

            if ( curr->data( ERoles::eIsIncludeDir ).toBool() )
                inclDirs.insert( curr->data( ERoles::eRelPath ).toString() );

            auto execNames = curr->data( ERoles::eExecutables ).value < QList< QPair< QString, bool > > >();
            if ( !execNames.isEmpty() )
                execNamesMap.insert( curr->data( ERoles::eRelPath ).toString(), execNames );

            auto childValues = findDirAttributes( curr );
            buildDirs.unite( std::get< 0 >( childValues ) );
            inclDirs.unite( std::get< 1 >( childValues ) );
            for ( auto ii = std::get< 2 >( childValues ).cbegin(); ii != std::get< 2 >( childValues ).cend(); ++ii )
            {
                execNamesMap[ ii.key() ] << ii.value();
            }
        }
    }
    return std::make_tuple( buildDirs, inclDirs, execNamesMap );
}

void CMainWindow::loadSettings()
{
    QSettings settings;
    fImpl->cmakePath->setText( settings.value( "CMakePath" ).toString() );
    fImpl->generator->setCurrentText( settings.value( "Generator" ).toString() );
    fImpl->destDir->setText( settings.value( "DestDir" ).toString() );

    fBuildDirs = settings.value( "BuildDirs" ).value< QSet< QString > >();
    fInclDirs = settings.value( "InclDirs" ).value< QSet< QString > >();
    fExecutables = settings.value( "Executables" ).value< QHash< QString, QList< QPair< QString, bool > > > >();

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

    QProcess process;
    //process.setProcessChannelMode(QProcess::MergedChannels);
    process.start( fImpl->cmakePath->text(), QStringList() << "-help" );
    if ( !process.waitForFinished( -1 ) || ( process.exitStatus() != QProcess::NormalExit ) || ( process.exitCode() != 0 ) )
    {
        QMessageBox::critical( this, tr( "Error Running CMake" ), QString( "Error: '%1' Could not run cmake and determine Generators" ).arg( QString( process.readAllStandardError() ) ) );
        return;
    }
    auto data = QString( process.readAll() );
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

void CMainWindow::incProgress( QProgressDialog * progress ) const
{
    progress->setValue( progress->value() + 1 );
    //if ( progress->value() >= 100 )
    //    progress->setValue( 1 );
    qApp->processEvents();
}

bool CMainWindow::isBuildDir( const QDir & dir ) const
{
    bool retVal = QFileInfo( dir.absoluteFilePath( "subdir.mk" ) ).exists();
    retVal = retVal || QFileInfo( dir.absoluteFilePath( "Makefile" ) ).exists();
    retVal = retVal || fBuildDirs.contains( dir.path() );
    return retVal;
}

bool CMainWindow::isInclDir( const QDir & dir ) const
{
    bool retVal = QFileInfo( dir.absolutePath() ).fileName() == "incl";
    retVal = retVal || fInclDirs.contains( dir.path() );
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
            bool isBuildDir = CMainWindow::isBuildDir( relDirPath );
            bool isInclDir = CMainWindow::isInclDir( relDirPath );
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
                results.fBuildDirs++;
            if ( isInclDir )
                results.fInclDirs++;
            results.fExecutables += execList.count();

            auto buildNode = new QStandardItem( isBuildDir ? "Yes" : "" );
            auto execNode = new QStandardItem( execNames.join( " " ) );
            //auto guiExec = new QStandardItem( guiExecs.join( " " ) );
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

void CMainWindow::slotLoadSource()
{
    fSourceModel->clear();
    fSourceModel->setHorizontalHeaderLabels( QStringList() << "Name" << "Is Build Dir?" << "Exec Name" << "Is Include Path?" );

    auto text = fSourceDir.value().dirName();
    auto rootNode = new QStandardItem( text );
    rootNode->setData( true, ERoles::eIsDirRole );
    fSourceModel->appendRow( rootNode );

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

    if ( wasCancelled )
    {
        fSourceModel->clear();
        QMessageBox::warning( this, tr( "Cancelled" ), tr( "Finding Source files Cancelled" ) );
    }
    else
    {
        delete progress;
        qApp->processEvents();
        fImpl->sourceTree->setExpanded( rootNode->index(), true );
        QMessageBox::information( this, tr( "Results" ), results.getText() );
    }
}

std::list< SDirInfo > CMainWindow::getDirInfo( QStandardItem * parent, QProgressDialog * progress ) const
{
    std::list< SDirInfo > retVal;

    int numRows = parent ? parent->rowCount() : fSourceModel->rowCount();
    if ( parent && !parent->data( ERoles::eIsDirRole ).toBool() )
        return {};

    progress->setRange( 0, numRows );
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
        if ( currInfo.isValid() )
            retVal.push_back( currInfo );
        auto children = getDirInfo( curr, progress );
        retVal.insert( retVal.end(), children.begin(), children.end() );
    }
    if ( progress->wasCanceled() )
        return {};

    return retVal;
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
        return;
    }

    QTextStream qts( &fo );
    qts << topDirText;

    qApp->processEvents();
    progress.setLabelText( tr( "Finding Directories" ) );
    progress.adjustSize();
    auto dirs = getDirInfo( nullptr, &progress );
    if ( progress.wasCanceled() )
    {
        QApplication::restoreOverrideCursor();
        return;
    }
    QStringList subDirs;
    for ( auto && ii : dirs )
    {
        qts << QString( "add_subdirectory( %1 )\n" ).arg( ii.fRelToDir );
    }

    progress.setRange( 0, static_cast<int>( dirs.size() ) );
    progress.setLabelText( tr( "Generating CMake Files" ) );
    progress.adjustSize();
    qApp->processEvents();
    int currDir = 0;
    for ( auto && ii : dirs )
    {
        progress.setValue( currDir++ );
        if ( progress.wasCanceled() )
            break;
        ii.writeCMakeFile( fImpl->destDir->text() );
    }
    QApplication::restoreOverrideCursor();
}


QString CMainWindow::SSourceFileInfo::getText() const
{
    return QObject::tr(
        "<ul>"
            "<li>Files: %1</li>"
            "<li>Directories: %2</li>"
            "<li>Executables: %3</li>"
            "<li>Include Directories: %4</li>"
            "<li>Build Directories: %5</li>"
        "</ul>"
    )
        .arg( fFiles )
        .arg( fDirs )
        .arg( fExecutables )
        .arg( fInclDirs )
        .arg( fBuildDirs )
        ;
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
}

bool SDirInfo::isValid() const
{
    if ( fRelToDir.isEmpty() || ( fRelToDir == "." ) )
        return false;

    return ( fIsInclDir || fIsBuildDir || !fExecutables.isEmpty() );
}

void SDirInfo::replaceFiles( QString & text, const QString & variable, const QStringList & files ) const
{
    QStringList values;
    for ( auto && ii : files )
    {
        values << QString( "   \"${ROOT_SOURCE_DIR}/%1\"" ).arg( ii );
    }
    auto value = values.join( "\n" );
    text.replace( variable, value );
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
