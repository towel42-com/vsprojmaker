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

#include "IncludesPage.h"
#include "ui_IncludesPage.h"
#include "SABUtils/UtilityModels.h"

#include <QFileDialog>
#include <QMessageBox>

CIncludesPage::CIncludesPage( QWidget * parent )
    : QWizardPage( parent ),
    fImpl( new Ui::CIncludesPage )
{
    fImpl->setupUi( this );
    fIncludes = new CCheckableStringListModel( this );
    auto includes = QStringList() << "<PRODDIR>/tcltk/tcl8605-20201227/debug/win64/usr/include";
    fIncludes->setStringList( includes );
    fIncludes->setChecked( includes, true, true );
    fImpl->includes->setModel( fIncludes );

    connect( fImpl->addIncludeDirBtn, &QToolButton::clicked, this, &CIncludesPage::slotAddIncludeDir );
}

void CIncludesPage::setDefaults()
{
}

CIncludesPage::~CIncludesPage()
{
}

QStringList CIncludesPage::enabledIncludeDirs() const
{
    return fIncludes->getCheckedStrings();
}

void CIncludesPage::slotAddIncludeDir()
{
    auto defaultPath = field( "prodDir" ).toString();
    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Additional Include Directory" ), defaultPath );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::critical( this, tr( "Error Readable Directory not Selected" ), QString( "Error: '%1' is not a readable directory" ).arg( dir ) );
        return;
    }

    auto prodDir = QDir( defaultPath );
    auto realDir = prodDir.relativeFilePath( dir );
    if ( realDir.isEmpty() )
        realDir = "<PRODDIR>";
    else if ( realDir.startsWith( "." ) )
        realDir = dir;
    else
        realDir = "<PRODDIR>/" + realDir;
    fIncludes->append( realDir, true );
}


