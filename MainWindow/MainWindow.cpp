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

CMainWindow::CMainWindow( QWidget* parent )
    : QDialog( parent ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );
    connect( fImpl->cmakePath, &QLineEdit::textChanged, this, &CMainWindow::slotCMakeChanged );
    connect(fImpl->generator, &QComboBox::currentTextChanged, this, &CMainWindow::slotChanged);
    connect(fImpl->sourceDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged);
    connect(fImpl->destDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged);

    connect( fImpl->cmakePathBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectCMake );
    connect( fImpl->sourceDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectSourceDir );
    connect( fImpl->destDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectDestDir );
    connect( fImpl->generateBtn, &QToolButton::clicked, this, &CMainWindow::slotGenerate );

    QTimer::singleShot(0, this, &CMainWindow::loadSettings);
}

CMainWindow::~CMainWindow()
{
    saveSettings();
}

void CMainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue( "CMakePath", fImpl->cmakePath->text() );
    settings.setValue( "Generator", fImpl->generator->currentText() );
    settings.setValue( "SourceDir", fImpl->sourceDir->text() );
    settings.setValue( "DestDir", fImpl->destDir->text());
}

void CMainWindow::loadSettings()
{
    QSettings settings;
    fImpl->cmakePath->setText( settings.value("CMakePath").toString() );
    fImpl->generator->setCurrentText( settings.value("Generator").toString() );
    fImpl->sourceDir->setText( settings.value("SourceDir").toString());
    fImpl->destDir->setText( settings.value("DestDir").toString());

    slotChanged();
}

void CMainWindow::slotChanged()
{
    QFileInfo fi(fImpl->cmakePath->text());
    bool aOK = !fImpl->cmakePath->text().isEmpty() && fi.exists() && fi.isExecutable();
    aOK = aOK && !fImpl->generator->currentText().isEmpty();
    fi = QFileInfo(fImpl->sourceDir->text());
    aOK = aOK && !fImpl->sourceDir->text().isEmpty() && fi.exists() && fi.isDir() && fi.isReadable();

    fi = QFileInfo(fImpl->destDir->text());
    aOK = aOK && !fImpl->destDir->text().isEmpty() && fi.exists() && fi.isDir() && fi.isWritable();

    fImpl->generateBtn->setEnabled(aOK);
}

void CMainWindow::slotSelectCMake()
{
    auto currPath = fImpl->cmakePath->text();
    if ( currPath.isEmpty() )
        currPath = QString();
    auto exe = QFileDialog::getOpenFileName(this, tr("Select CMake Executable"), currPath, "All Executables *.exe;;All Files *.*");
    if (exe.isEmpty())
        return;

    QFileInfo fi(exe);
    if ( !fi.exists() || !fi.isExecutable() )
    {
        QMessageBox::critical(this, tr("Error Executable not Selected"), QString("Error: '%1' is not an executable").arg(exe));
        return;
    }

    fImpl->cmakePath->setText(exe);
}

void CMainWindow::slotCMakeChanged()
{
    QFileInfo fi(fImpl->cmakePath->text());
    if (!fi.exists() || !fi.isExecutable())
        return;

    disconnect(fImpl->generator, &QComboBox::currentTextChanged, this, &CMainWindow::slotChanged);
    auto currText = fImpl->generator->currentText();
    fImpl->generator->clear();

    QApplication::setOverrideCursor( Qt::WaitCursor );

    QProcess process;
    //process.setProcessChannelMode(QProcess::MergedChannels);
    process.start( fImpl->cmakePath->text(), QStringList() << "-help" );
    if ( !process.waitForFinished( -1 ) || ( process.exitStatus() != QProcess::NormalExit ) || ( process.exitCode() != 0 ) )
    {
        QMessageBox::critical(this, tr("Error Running CMake"), QString("Error: '%1' Could not run cmake and determine Generators").arg( QString( process.readAllStandardError() ) ) );
        return;
    }
    auto data = QString( process.readAll() );
    auto lines = data.split( '\n' );
    QStringList generators;
    bool inGenerators = false;
    for ( auto && curr : lines )
    {
        if (!inGenerators)
        {
            inGenerators = curr == "Generators";
            continue;
        }
        if ( curr.startsWith("The following") )
            continue;

        curr = curr.mid( 2 );

        if ( curr.isEmpty() )
            continue;

        auto lineCont = (curr[0] == ' ');
        if ( lineCont && generators.isEmpty() )
            continue;

        curr = curr.trimmed();

        if ( lineCont )
            generators.back() += " " + curr;
        else
        {
            curr.replace( QRegularExpression( "\\s+=\\s+" ), " = " );
            generators.push_back(curr);
        }
    }

    fImpl->generator->addItems( generators );

    QApplication::restoreOverrideCursor();
    connect(fImpl->generator, &QComboBox::currentTextChanged, this, &CMainWindow::slotChanged);

    auto idx = fImpl->generator->findText(currText);
    if (idx != -1)
        fImpl->generator->setCurrentIndex(idx);
}

void CMainWindow::slotSelectSourceDir()
{
    auto currPath = fImpl->sourceDir->text();
    if (currPath.isEmpty())
        currPath = QString();
    auto dir = QFileDialog::getExistingDirectory( this, tr("Select Source Directory"), currPath );
    if (dir.isEmpty())
        return;

    QFileInfo fi(dir);
    if (!fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical(this, tr("Error Readable Directory not Selected"), QString("Error: '%1' is not an readable directory").arg(dir));
        return;
    }

    fImpl->sourceDir->setText(dir);
}

void CMainWindow::slotSelectDestDir()
{
    auto currPath = fImpl->destDir->text();
    if (currPath.isEmpty())
        currPath = QString();
    auto dir = QFileDialog::getExistingDirectory(this, tr("Select Destination Directory"), currPath);
    if (dir.isEmpty())
        return;

    QFileInfo fi(dir);
    if (!fi.exists() || !fi.isDir() || !fi.isWritable())
    {
        QMessageBox::critical(this, tr("Error Writable Directory not Selected"), QString("Error: '%1' is not an writeable directory").arg(dir));
        return;
    }

    fImpl->destDir->setText(dir);
}

void CMainWindow::slotGenerate()
{

}


