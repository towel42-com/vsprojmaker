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

#ifndef __CLIENTINFOPAGE_H
#define __CLIENTINFOPAGE_H

#include <QWizardPage>
#include <memory>
namespace Ui {class CClientInfoPage;};

class CClientInfoPage : public QWizardPage
{
    Q_OBJECT
public:
    CClientInfoPage( QWidget * parent=nullptr );
    ~CClientInfoPage();

    void setDefaults();
    virtual bool isComplete() const override;
public Q_SLOTS:
    void slotChanged();
    void slotSelectClientDir();
    void slotSelectSourceDir();
    void slotSelectBuildDir();
    void slotSelectModelTechDir();
private:
    std::unique_ptr< Ui::CClientInfoPage > fImpl;
    bool fSourceDirManuallySet{ false };
    bool fBuildDirManuallySet{ false };
    bool fModelTechDirManuallySet{ false };
};

#endif // _ALCULATOR_H
