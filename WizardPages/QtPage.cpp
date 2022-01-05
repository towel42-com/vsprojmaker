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

#include "QtPage.h"
#include "ui_QtPage.h"
#include "MainLib/Settings.h"
#include "SABUtils/UtilityModels.h"

#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

CQtPage::CQtPage( QWidget * parent )
    : QWizardPage( parent ),
    fImpl( new Ui::CQtPage )
{
    fImpl->setupUi( this );
    fQtLibsModel = new NSABUtils::CCheckableStringListModel( this );
    fImpl->qtLibs->setModel( fQtLibsModel );

    connect( fImpl->qtDirBtn, &QToolButton::clicked, this, &CQtPage::slotSelectQtDir );
    connect( fImpl->qtDir, &QLineEdit::textChanged, this, &CQtPage::slotChanged );

    registerField( "qtDir*", fImpl->qtDir );
}

static QString sDefaultQtVersion = "/qt/qt5129_20210303/win64VC15";
void CQtPage::setDefaults()
{
    auto currPath = field( "prodDir" ).toString() + sDefaultQtVersion;
    fImpl->qtDir->setText( currPath );
}

CQtPage::~CQtPage()
{
}


bool CQtPage::isComplete() const
{
    auto retVal = QWizardPage::isComplete();
    if ( !retVal )
        return false;
    QFileInfo fi( fImpl->qtDir->text() );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
        return false;

    return true;
}

std::pair< QStringList, QStringList > CQtPage::qtDirs() const
{
    return std::make_pair( fQtLibsModel->stringList(), fQtLibsModel->getCheckedStrings() );
}

void CQtPage::slotChanged()
{
    fQtLibsModel->setStringList( NVSProjectMaker::CSettings::getQtIncludeDirs( fImpl->qtDir->text() ) );
    auto defaultChecked = QStringList()
        << "QtCharts"
        << "QtConcurrent"
        << "QtCore"
        << "QtGui"
        << "QtNetwork"
        << "QtPrintSupport"
        << "QtSql"
        << "QtSvg"
        << "QtWidgets"
        << "QtWinExtras"
        << "QtXml"
        ;
    fQtLibsModel->setChecked( defaultChecked, true, true );
}

void CQtPage::slotSelectQtDir()
{
    auto currPath = fImpl->qtDir->text();
    if ( currPath.isEmpty() )
        currPath = field( "prodDir" ).toString() + sDefaultQtVersion;
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

