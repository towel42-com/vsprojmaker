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

#include "AddCustomBuild.h"
#include "ui_AddCustomBuild.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>

CAddCustomBuild::CAddCustomBuild( const QString & buildDir, QWidget * parent )
    : QDialog( parent ),
    fImpl( new Ui::CAddCustomBuild ),
    fBuildDir( buildDir )
{
    fImpl->setupUi( this );
    auto bldDir = QDir( buildDir );
    bldDir.cdUp();
    bldDir.cdUp();

    fImpl->buildDir->setText( bldDir.relativeFilePath( buildDir ) );
    connect( fImpl->buildDirBtn, &QToolButton::clicked, this, &CAddCustomBuild::slotSelectBuildDir );
}

CAddCustomBuild::~CAddCustomBuild()
{
}

QString CAddCustomBuild::buildDir() const
{
    return fImpl->buildDir->text();
}

QString CAddCustomBuild::targetName() const
{
    return fImpl->targetName->text();
}

void CAddCustomBuild::slotSelectBuildDir()
{
    auto currPath = fImpl->buildDir->text();
    auto bldDir = QDir( fBuildDir );
    if ( currPath.isEmpty() )
        currPath = bldDir.absolutePath();

    auto dir = QFileDialog::getExistingDirectory( this, tr( "Select Build Director" ), currPath );
    if ( dir.isEmpty() )
        return;

    QFileInfo fi( dir );
    if ( !fi.exists() || !fi.isDir() || !fi.isWritable() )
    {
        QMessageBox::critical( this, tr( "Error Writable Directory not Selected" ), QString( "Error: '%1' is not an writable directory" ).arg( dir ) );
        return;
    }

    bldDir.cdUp();
    bldDir.cdUp();
    fImpl->buildDir->setText( bldDir.relativeFilePath( dir ) );
}

