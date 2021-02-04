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

#include "PreProcDefinesPage.h"
#include "ui_PreProcDefinesPage.h"
#include "SABUtils/UtilityModels.h"

#include <QInputDialog>

CPreProcDefinesPage::CPreProcDefinesPage( QWidget * parent )
    : QWizardPage( parent ),
    fImpl( new Ui::CPreProcDefinesPage )
{
    fImpl->setupUi( this );
    fPreProcDefines = new CCheckableStringListModel( this );
    auto PreProcDefines = QStringList() << "WIN32" << "Q_OS_WIN";
    fPreProcDefines->setStringList( PreProcDefines );
    fPreProcDefines->setChecked( PreProcDefines, true, true );
    fImpl->preProcDefines->setModel( fPreProcDefines );

    connect( fImpl->addPreProDefineBtn, &QToolButton::clicked, this, &CPreProcDefinesPage::slotAddPreProcDefine );
}

void CPreProcDefinesPage::setDefaults()
{
}

CPreProcDefinesPage::~CPreProcDefinesPage()
{
}

QStringList CPreProcDefinesPage::enabledPreProcDefines() const
{
    return fPreProcDefines->getCheckedStrings();
}

void CPreProcDefinesPage::slotAddPreProcDefine()
{
    auto define = QInputDialog::getText( this, tr( "PreProcessor Define:" ), tr( "Define:" ) );
    if ( define.isEmpty() )
        return;

    fPreProcDefines->append( define, true );
}


