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

#include "SystemInfoPage.h"
#include "ui_SystemInfoPage.h"

#include "MainLib/Settings.h"
#include "SABUtils/VSInstallUtils.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QProcess>

CSystemInfoPage::CSystemInfoPage( QWidget * parent )
    : QWizardPage( parent ),
    fImpl( new Ui::CSystemInfoPage )
{
    fImpl->setupUi( this );

    connect( fImpl->vsVersion, &QComboBox::currentTextChanged, this, &CSystemInfoPage::slotChanged );
    connect( fImpl->cmakeExec, &QLineEdit::textChanged, this, &CSystemInfoPage::slotChanged );
    connect( fImpl->useCustomCMake, &QGroupBox::clicked, this, &CSystemInfoPage::slotChanged );
    connect( fImpl->cmakeExecBtn, &QToolButton::clicked, this, &CSystemInfoPage::slotSelectCMake );
    connect( fImpl->prodDirBtn, &QToolButton::clicked, this, &CSystemInfoPage::slotSelectProdDir );
    connect( fImpl->msys64DirBtn, &QToolButton::clicked, this, &CSystemInfoPage::slotSelectMSys64Dir );

    registerField( "useCustomCMake", fImpl->useCustomCMake );
    registerField( "cmakeExec", fImpl->cmakeExec );
    registerField( "vsVersion*", fImpl->vsVersion, "currentText" );
    registerField( "prodDir*", fImpl->prodDir );
    registerField( "msys64Dir*", fImpl->msys64Dir );
}

void CSystemInfoPage::setDefaults()
{
    bool aOK = false;
    bool retry = false;
    QString errorMsg;
    std::tie( aOK, errorMsg, fInstalledVSes ) = NSABUtils::NVSInstallUtils::getInstalledVisualStudios( new QProcess( this ), &retry );

    fImpl->vsVersion->addItems( fInstalledVSes.second );
    // default is 2017
    QString defVS;
    for ( auto && ii : fInstalledVSes.second )
    {
        if ( ii.contains( "2017" ) )
        {
            defVS = ii;
            break;
        }
    }
    fImpl->useCustomCMake->setChecked( false );
    fImpl->vsVersion->setCurrentText( defVS );
    fImpl->prodDir->setText( "C:/localprod" );
    fImpl->msys64Dir->setText( "C:/msys64" );
}

CSystemInfoPage::~CSystemInfoPage()
{
}


bool CSystemInfoPage::isComplete() const
{
    auto retVal = QWizardPage::isComplete();
    if ( !retVal )
        return false;
    QFileInfo fi( fImpl->prodDir->text() );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
        return false;

    fi = QFileInfo( fImpl->msys64Dir->text() );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
        return false;

    if ( fImpl->useCustomCMake->isChecked() )
    {
        fi = QFileInfo( fImpl->cmakeExec->text() );
    }
    else
    {
        auto pos = fInstalledVSes.first.find( fImpl->vsVersion->currentText() );
        if ( pos == fInstalledVSes.first.end() )
            return false;

        auto cmakePath = NVSProjectMaker::CSettings::getCMakeExecViaVSPath( (*pos).second );
        fi = QFileInfo( cmakePath );
    }

    if ( !fi.exists() || !fi.isExecutable() || !fi.isReadable() )
        return false;

    return true;
}

void CSystemInfoPage::slotChanged()
{
    //QString cmakeExec = NVSProjectMaker::CSettings::getCMakeExecViaVSPath( fImpl->vsPath->currentText() );
    //fImpl->cmakeExec->setText( cmakeExec );
    emit completeChanged();
}

void CSystemInfoPage::slotSelectCMake()
{
    auto currPath = fImpl->cmakeExec->text();
    auto cmakePath = QFileDialog::getOpenFileName( this, tr( "Select Visual Studio Directory" ), currPath );
    if ( cmakePath.isEmpty() )
        return;

    auto fi = QFileInfo( cmakePath );

    if ( !fi.exists() || !fi.isExecutable() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Valid Executable not Selected" ), QString( "Error: '%1' is not a valid executable" ).arg( cmakePath ) );
        return;
    }

    fImpl->cmakeExec->setText( cmakePath );
}

void CSystemInfoPage::slotSelectProdDir()
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

void CSystemInfoPage::slotSelectMSys64Dir()
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

QString CSystemInfoPage::getVSPathForSelection( const QString & selected ) const
{
    auto pos = fInstalledVSes.first.find( selected );
    if ( pos == fInstalledVSes.first.end() )
        return QString();
    return ( *pos ).second;
}
