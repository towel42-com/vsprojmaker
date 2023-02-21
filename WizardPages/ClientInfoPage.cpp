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
#include <QCompleter>
#include <QFileSystemModel>

CClientInfoPage::CClientInfoPage( QWidget * parent )
    : QWizardPage( parent ),
    fImpl( new Ui::CClientInfoPage )
{
    fImpl->setupUi( this );

    connect( fImpl->clientDir, &QLineEdit::textChanged, this, &CClientInfoPage::slotChanged );
    connect( fImpl->sourceRelativeDir, &QLineEdit::textChanged, this, &CClientInfoPage::slotChanged );
    connect( fImpl->buildRelativeDir, &QLineEdit::textChanged, this, &CClientInfoPage::slotChanged );
    connect( fImpl->modelTechRelativeDir, &QLineEdit::textChanged, this, &CClientInfoPage::slotChanged );
    connect( fImpl->clientDirBtn, &QToolButton::clicked, this, &CClientInfoPage::slotSelectClientDir );
    connect( fImpl->sourceRelativeDirBtn, &QToolButton::clicked, this, &CClientInfoPage::slotSelectSourceDir );
    connect( fImpl->buildRelativeDirBtn, &QToolButton::clicked, this, &CClientInfoPage::slotSelectBuildDir );
    connect( fImpl->modelTechRelativeDirBtn, &QToolButton::clicked, this, &CClientInfoPage::slotSelectModelTechDir );

    registerField( "clientDir*", fImpl->clientDir );
    registerField( "clientName", fImpl->clientName );
    registerField( "sourceRelativeDir*", fImpl->sourceRelativeDir );
    registerField( "buildRelativeDir*", fImpl->buildRelativeDir );

    auto completer = new QCompleter( this );
    completer->setCompletionMode( QCompleter::InlineCompletion );
    completer->setModel( new QFileSystemModel( completer ) );
    fImpl->clientDir->setCompleter( completer );

    registerField( "modelTechRelativeDir*", fImpl->modelTechRelativeDir );

    slotChanged();
}

void CClientInfoPage::setDefaults()
{
}

CClientInfoPage::~CClientInfoPage()
{
}


bool CClientInfoPage::isComplete() const
{
    QFileInfo fi( fImpl->clientDir->text() );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
        return false;

    QDir clientDir( fImpl->clientDir->text() );
    fi = QFileInfo( clientDir.absoluteFilePath( fImpl->sourceRelativeDir->text() ) );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
        return false;

    fi = QFileInfo( clientDir.absoluteFilePath( fImpl->buildRelativeDir->text() ) );
    if ( !fi.exists() || !fi.isDir() || !fi.isWritable() )
        return false;

    fi = QFileInfo( clientDir.absoluteFilePath( fImpl->modelTechRelativeDir->text() ) );
    if ( !fi.exists() || !fi.isDir() || !fi.isWritable() )
        return false;

    return true;
}

void CClientInfoPage::slotChanged()
{
    disconnect( fImpl->clientName, &QLineEdit::textChanged, this, &CClientInfoPage::slotChanged );
    disconnect( fImpl->sourceRelativeDir, &QLineEdit::textChanged, this, &CClientInfoPage::slotChanged );
    disconnect( fImpl->buildRelativeDir, &QLineEdit::textChanged, this, &CClientInfoPage::slotChanged );
    disconnect( fImpl->modelTechRelativeDir, &QLineEdit::textChanged, this, &CClientInfoPage::slotChanged );

    QString clientName;
    QDir dir;
    if ( !fImpl->clientDir->text().isEmpty() )
    {
        dir = QDir( fImpl->clientDir->text() );
        clientName = dir.dirName();
    }
    fImpl->clientName->setText( clientName );

    fImpl->sourceRelativeDirBtn->setEnabled( !clientName.isEmpty() );
    fImpl->buildRelativeDirBtn->setEnabled( !clientName.isEmpty() );
    fImpl->modelTechRelativeDirBtn->setEnabled( !clientName.isEmpty() );

    fImpl->sourceRelativeDir->setEnabled( fSourceDirManuallySet );
    fImpl->buildRelativeDir->setEnabled( fBuildDirManuallySet );
    fImpl->modelTechRelativeDir->setEnabled( fModelTechDirManuallySet );

    if ( fSourceDirManuallySet && fImpl->sourceRelativeDir->text().isEmpty() )
        fSourceDirManuallySet = false;

    if ( fBuildDirManuallySet && fImpl->buildRelativeDir->text().isEmpty() )
        fBuildDirManuallySet = false;

    if ( fModelTechDirManuallySet && fImpl->modelTechRelativeDir->text().isEmpty() )
        fModelTechDirManuallySet = false;

    if ( !fSourceDirManuallySet )
    {
        if ( QFileInfo( dir.absoluteFilePath( "src" ) ).exists() )
            fImpl->sourceRelativeDir->setText( "src" );
    }

    if ( !fBuildDirManuallySet )
    {
        if ( QFileInfo( dir.absoluteFilePath( "build/win64" ) ).exists() )
            fImpl->buildRelativeDir->setText( "build/win64" );
    }

    if ( !fModelTechDirManuallySet )
    {
        if ( QFileInfo( dir.absoluteFilePath( "modeltech/win64" ) ).exists() )
            fImpl->modelTechRelativeDir->setText( "modeltech/win64" );
    }
    connect( fImpl->clientName, &QLineEdit::textChanged, this, &CClientInfoPage::slotChanged );
    connect( fImpl->sourceRelativeDir, &QLineEdit::textChanged, this, &CClientInfoPage::slotChanged );
    connect( fImpl->buildRelativeDir, &QLineEdit::textChanged, this, &CClientInfoPage::slotChanged );
    connect( fImpl->modelTechRelativeDir, &QLineEdit::textChanged, this, &CClientInfoPage::slotChanged );
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
}

void CClientInfoPage::slotSelectSourceDir()
{
    auto clientDir = fImpl->clientDir->text();
    if ( clientDir.isEmpty() )
        return;
    auto currSrcRelativeDir = fImpl->sourceRelativeDir->text();
    if ( !currSrcRelativeDir.isEmpty() )
        currSrcRelativeDir = clientDir + "/" + currSrcRelativeDir;
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Source Directory" ), currSrcRelativeDir );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not an readable directory" ).arg( dir ) );
        return;
    }

    fImpl->sourceRelativeDir->setText( QDir( clientDir ).relativeFilePath( dir ) );
    fSourceDirManuallySet = true;
    slotChanged();
}

void CClientInfoPage::slotSelectBuildDir()
{
    auto clientDir = fImpl->clientDir->text();
    if ( clientDir.isEmpty() )
        return;
    auto currBldRelativeDir = fImpl->buildRelativeDir->text();
    if ( !currBldRelativeDir.isEmpty() )
        currBldRelativeDir = clientDir + "/" + currBldRelativeDir;

    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Build Directory" ), currBldRelativeDir );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isWritable() )
    {
        QMessageBox::critical( this, tr( "Error Writable Directory not Selected" ), QString( "Error: '%1' is not an writable directory" ).arg( dir ) );
        return;
    }

    fImpl->buildRelativeDir->setText( QDir( clientDir ).relativeFilePath( dir ) );
    fBuildDirManuallySet = true;
    slotChanged();
}

void CClientInfoPage::slotSelectModelTechDir()
{
    auto clientDir = fImpl->clientDir->text();
    if ( clientDir.isEmpty() )
        return;
    auto currModelTechRelativeDir = fImpl->modelTechRelativeDir->text();
    if ( !currModelTechRelativeDir.isEmpty() )
        currModelTechRelativeDir = clientDir + "/" + currModelTechRelativeDir;

    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select ModelTech Directory" ), currModelTechRelativeDir );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isWritable() )
    {
        QMessageBox::critical( this, tr( "Error Writable Directory not Selected" ), QString( "Error: '%1' is not an writable directory" ).arg( dir ) );
        return;
    }

    fImpl->modelTechRelativeDir->setText( QDir( clientDir ).relativeFilePath( dir ) );
    fModelTechDirManuallySet = true;
    slotChanged();
}
