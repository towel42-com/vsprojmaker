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

#include "BuildTargetsPage.h"
#include "ui_BuildTargetsPage.h"
#include "SABUtils/UtilityModels.h"

CBuildTargetsPage::CBuildTargetsPage( QWidget * parent )
    : QWizardPage( parent ),
    fImpl( new Ui::CBuildTargetsPage )
{
    fImpl->setupUi( this );
    fBuildTargets = new CCheckableStringListModel( this );
    auto buildTargets = QStringList() << "qworld" << "qoffice" << "world";
    fBuildTargets->setStringList( buildTargets );
    fBuildTargets->setChecked( buildTargets, true, true );
    fImpl->buildTargets->setModel( fBuildTargets );

    connect( fBuildTargets, &CCheckableStringListModel::dataChanged, this, &CBuildTargetsPage::slotBuildsChanged );

    fImpl->primaryTarget->addItems( buildTargets );
    fImpl->primaryTarget->setCurrentText( "qworld" );
    registerField( "primaryTarget", fImpl->primaryTarget );
    slotBuildsChanged();
}

void CBuildTargetsPage::setDefaults()
{
}

CBuildTargetsPage::~CBuildTargetsPage()
{
}

QStringList CBuildTargetsPage::enabledBuildTargets() const
{
    return fBuildTargets->getCheckedStrings();
}

QString CBuildTargetsPage::primaryTarget() const
{
    return fImpl->primaryTarget->currentText();
}

void CBuildTargetsPage::slotBuildsChanged()
{
    auto list = QStringList() << QString() << fBuildTargets->getCheckedStrings();
    auto curr = fImpl->primaryTarget->currentText();
    fImpl->primaryTarget->clear();
    fImpl->primaryTarget->addItems( list );
    fImpl->primaryTarget->setCurrentText( curr );
}




