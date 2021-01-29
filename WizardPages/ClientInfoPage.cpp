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

#include "ClientInfoPage.h"
#include "ui_ClientInfoPage.h"

#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

CClientInfoPage::CClientInfoPage( QWidget * parent )
    : QWizardPage( parent ),
    fImpl( new Ui::CClientInfoPage )
{
    fImpl->setupUi( this );

    connect( fImpl->clientDir, &QLineEdit::textChanged, this, &CClientInfoPage::slotChanged );
    connect( fImpl->clientDirBtn, &QToolButton::clicked, this, &CClientInfoPage::slotSelectClientDir );
    connect( fImpl->sourceDirBtn, &QToolButton::clicked, this, &CClientInfoPage::slotSelectSourceDir );
    connect( fImpl->buildDirBtn, &QToolButton::clicked, this, &CClientInfoPage::slotSelectBuildDir );

    registerField( "clientDir*", fImpl->clientDir );
    registerField( "clientName", fImpl->clientName );
    registerField( "sourceRelDir*", fImpl->sourceRelDir );
    registerField( "buildRelDir*", fImpl->buildRelDir );
}

void CClientInfoPage::setDefaults()
{
}

CClientInfoPage::~CClientInfoPage()
{
}


bool CClientInfoPage::isComplete() const
{
    auto retVal = QWizardPage::isComplete();
    if ( !retVal )
        return false;
    QFileInfo fi( fImpl->clientDir->text() );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
        return false;

    QDir clientDir( fImpl->clientDir->text() );
    fi = QFileInfo( clientDir.absoluteFilePath( fImpl->sourceRelDir->text() ) );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
        return false;

    fi = QFileInfo( clientDir.absoluteFilePath( fImpl->buildRelDir->text() ) );
    if ( !fi.exists() || !fi.isDir() || !fi.isWritable() )
        return false;

    return true;
}

void CClientInfoPage::slotChanged()
{
    QString clientName;
    if ( !fImpl->clientDir->text().isEmpty() )
    {
        QDir dir( fImpl->clientDir->text() );
        clientName = dir.dirName();
    }
    fImpl->clientName->setText( clientName );
}

void CClientInfoPage::slotSelectClientDir()
{
    auto currPath = fImpl->clientDir->text();
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Client Directory" ), currPath );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    fImpl->clientDir->setText( dir );

    if ( QFileInfo( QDir( dir ).absoluteFilePath( "src" ) ).exists() )
        fImpl->sourceRelDir->setText( "src" );

    if ( QFileInfo( QDir( dir ).absoluteFilePath( "build/win64" ) ).exists() )
        fImpl->buildRelDir->setText( "build/win64" );
}

void CClientInfoPage::slotSelectSourceDir()
{
    auto clientDir = fImpl->clientDir->text();
    if ( clientDir.isEmpty() )
        return;
    auto currSrcRelDir = fImpl->sourceRelDir->text();
    if ( !currSrcRelDir.isEmpty() )
        currSrcRelDir = clientDir + "/" + currSrcRelDir;
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Source Directory" ), currSrcRelDir );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    fImpl->sourceRelDir->setText( QDir( clientDir ).relativeFilePath( dir ) );
}

void CClientInfoPage::slotSelectBuildDir()
{
    auto clientDir = fImpl->clientDir->text();
    if ( clientDir.isEmpty() )
        return;
    auto currBldRelDir = fImpl->buildRelDir->text();
    if ( !currBldRelDir.isEmpty() )
        currBldRelDir = clientDir + "/" + currBldRelDir;

    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Build Directory" ), currBldRelDir );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isWritable() )
    {
        QMessageBox::critical( this, tr( "Error Writable Directory not Selected" ), QString( "Error: '%1' is not an writable directory" ).arg( dir ) );
        return;
    }

    fImpl->buildRelDir->setText( QDir( clientDir ).relativeFilePath( dir ) );
}