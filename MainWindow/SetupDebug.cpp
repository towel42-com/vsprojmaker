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

#include "SetupDebug.h"
#include "ui_SetupDebug.h"
#include "SABUtils/StringUtils.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>

CSetupDebug::CSetupDebug( const QString & sourceDir, const QString & buildDir, QWidget * parent )
    : QDialog( parent ),
    fImpl( new Ui::CSetupDebug ),
    fSourceDir( sourceDir ),
    fBuildDir( buildDir )
{
    fImpl->setupUi( this );
    connect( fImpl->selectCmdBtn, &QToolButton::clicked, this, &CSetupDebug::slotSelectCommand );
    connect( fImpl->selectWorkDirBtn, &QToolButton::clicked, this, &CSetupDebug::slotSelectWorkingDir );
    connect( fImpl->sourceDirBtn, &QToolButton::clicked, this, &CSetupDebug::slotSelectSourceDir );
}

CSetupDebug::~CSetupDebug()
{
}

QString CSetupDebug::name() const
{
    return fImpl->name->text();
}

QString CSetupDebug::command() const
{
    return fImpl->cmd->text();
}

QStringList CSetupDebug::args() const
{
    return fImpl->cmdArgs->text().split( " ", NSABUtils::NStringUtils::TSkipEmptyParts);
}

QString CSetupDebug::workDir() const
{
    return fImpl->workDir->text();
}

QString CSetupDebug::sourceDir() const
{
    return fImpl->sourceDir->text();
}

QStringList CSetupDebug::envVars() const
{
    return fImpl->envVars->text().split(" ", NSABUtils::NStringUtils::TSkipEmptyParts);
}

void CSetupDebug::slotSelectCommand()
{
    auto currPath = fImpl->cmd->text();
    if ( currPath.isEmpty() )
        currPath = fBuildDir;
    auto exe = QFileDialog::getOpenFileName( this, tr( "Select Executable" ), currPath, "All Executables *.exe;;All Files *.*" );
    if ( exe.isEmpty() )
        return;

    QFileInfo fi( exe );
    if ( !fi.exists() || !fi.isExecutable() )
    {
        QMessageBox::critical( this, tr( "Error Executable not Selected" ), QString( "Error: '%1' is not an executable" ).arg( exe ) );
        return;
    }

    fImpl->cmd->setText( exe );
    fImpl->workDir->setText( fi.absolutePath() );
}

void CSetupDebug::slotSelectWorkingDir()
{
    auto currPath = fImpl->workDir->text();
    if ( currPath.isEmpty() )
        currPath = fBuildDir;
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Working Directory" ), currPath );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    fImpl->workDir->setText( dir );
}

void CSetupDebug::slotSelectSourceDir()
{
    auto currPath = fImpl->sourceDir->text();
    auto srcDir = QDir( fSourceDir );
    srcDir.cdUp();
    if ( currPath.isEmpty() )
    {
        currPath = srcDir.absolutePath();
    }
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Source Directory" ), currPath );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    fImpl->sourceDir->setText( srcDir.relativeFilePath( dir ) );
}

