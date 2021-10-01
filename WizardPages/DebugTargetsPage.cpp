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

#include "DebugTargetsPage.h"
#include "ui_DebugTargetsPage.h"
#include "SABUtils/UtilityModels.h"
#include "MainLib/Settings.h"
#include "SABUtils/StringUtils.h"

#include <QDir>
#include <QDebug>

CDebugTargetsPage::CDebugTargetsPage( QWidget * parent )
    : QWizardPage( parent ),
    fImpl( new Ui::CDebugTargetsPage )
{
    fImpl->setupUi( this );
    fDebugTargets = new CCheckableStringListModel( this );
    fImpl->debugTargets->setModel( fDebugTargets );
}

void CDebugTargetsPage::setDefaults()
{
}

CDebugTargetsPage::~CDebugTargetsPage()
{
}

void CDebugTargetsPage::initializePage()
{
    fAllTargets = getDebugTargets();
    QStringList customTargets;
    for (auto&& ii : fAllTargets)
    {
        if (ii.empty())
            continue;
        QString mainApp;
        QStringList addOns;
        for (auto&& jj : ii)
        {
            QString currTarget = jj;
            QString remaining;
            auto pos = currTarget.indexOf("+");
            if (pos != -1)
            {
                remaining = currTarget.mid(pos + 1).trimmed();
                currTarget = currTarget.left(pos).trimmed();
                if (mainApp.isEmpty())
                    mainApp = currTarget;
                addOns << remaining;
            }
            else
            {
                if (mainApp.isEmpty())
                    mainApp = currTarget;
            }
        }
        auto target = mainApp;
        if ( !addOns.empty() )
            target += "+" + addOns.join("+");
        customTargets << target;
    }
    fDebugTargets->setStringList(customTargets);
    fDebugTargets->setChecked(customTargets, true, true);
}

std::list< std::list< QString > > CDebugTargetsPage::getDebugTargets() const
{
    auto clientDir = field("clientDir").toString();
    auto dir = QDir(QDir(clientDir).absoluteFilePath("src/hdloffice/hdlstudio/cxx/vxcSamples"));

    auto retVal = std::list< std::list< QString > >();
    retVal.push_back(std::list< QString >({ "HDL Client" }));
    retVal.push_back(std::list< QString >({ "HDL Studio" }));

    auto sampleDirs = getSampleDirs(dir);
    retVal.insert(retVal.end(), sampleDirs.begin(), sampleDirs.end());
    
    return retVal;
}

std::list< std::list< QString > > CDebugTargetsPage::getSampleDirs(const QDir & currDir ) const
{
    qDebug() << currDir.absolutePath();
    std::list< std::list< QString > > retVal;

    auto files = currDir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    bool isBuildDir = false;
    for (auto&& file : files)
    {
        if (file == "subdir.mk")
        {
            isBuildDir = true;
            break;
        }
    }
    if (isBuildDir)
        retVal.push_back(std::list< QString >({ QString("HDL Client + %1").arg(currDir.dirName()) }));

    auto subDirs = currDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    for (auto&& ii : subDirs)
    {
        auto subDir = currDir.absoluteFilePath(ii);
        auto subSampleDirs= getSampleDirs(subDir);
        // flatten all sub
        std::list< QString > flattened;
        for (auto&& jj : subSampleDirs)
        {
            flattened.insert(flattened.end(), jj.begin(), jj.end());
        }
        retVal.push_back(flattened);
    }

    return retVal;
}

std::list< std::shared_ptr< SDebugTargetInfo > > CDebugTargetsPage::enabledDebugTargets(NVSProjectMaker::CSettings * settings ) const
{
    std::list< std::shared_ptr< SDebugTargetInfo > > retVal;
    auto enabledTargets = fDebugTargets->getCheckedStrings();
    for (auto && ii : enabledTargets)
    {
        auto curr = std::make_shared< SDebugTargetInfo >(ii, settings);
        if (!curr->isAOK())
            continue;
        retVal.push_back( curr );
    }
    return retVal;
}

SDebugTargetInfo::SDebugTargetInfo(const QString & initTargetName, NVSProjectMaker::CSettings * settings)
{
    fTargetName = initTargetName;
    fExecutable = QString("<CLIENTDIR>/modeltech/win64/VisualizerRls/bin/hdlclient.exe");

    auto pos = fTargetName.indexOf("+");
    if (pos != -1)
    {
        auto remaining = fTargetName.mid(pos + 1).trimmed();
        auto addOns = remaining.split("+", TSkipEmptyParts );
        for (auto && ii : addOns)
        {
            QString name;
            bool isExe{ false };
            std::tie(name, isExe) = settings->findSampleOutputPath(ii.trimmed());
            if (isExe)
            {
                fTargetName = remaining.trimmed();
                fExecutable = QString("<CLIENTDIR>/devapps/win64/%1.exe").arg(name);
            }
            else
                fArgs << QString("+external_client+<CLIENTDIR>/devapps/visualizer/win64/%1.dll").arg(name);
        }
    }

    if (fTargetName == "HDL Studio")
    {
        fExecutable = QString("<CLIENTDIR>/modeltech/win64/VisualizerRls/bin/hdlstudio.exe");
    }

    if (fTargetName.contains("ProjectSample"))
    {
        fEnvVars << "SKIP_LIMITED_FEATURES_KEY=1" << "ENABLE_VFPRJ=1";
        fArgs << "+vfprj_mode";
    }
    else if (fTargetName.contains("ProjectCLISample"))
    {
        fEnvVars << "SKIP_LIMITED_FEATURES_KEY=1" << "ENABLE_VFPRJ=1";
        fArgs << "+vfprj_mode";
    }
    else if (fTargetName.contains("FlowNavSample"))
    {
        fEnvVars << "SKIP_LIMITED_FEATURES_KEY=1" << "ENABLE_VFPRJ=1";
        fArgs << "+vfprj_mode";
    }
    if (fExecutable.isEmpty())
        return;
    fWorkingDir = QString("<CLIENTDIR>");
    fSourceDirPath = QString("src/hdloffice/hdlstudio/cxx/main/src");
}
