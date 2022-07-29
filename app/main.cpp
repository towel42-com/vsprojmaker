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

#include "MainWindow/MainWindow.h"
#include "MainLib/VSProjectMaker.h"
#include "MainLib/Settings.h"
#include "SABUtils/ConsoleUtils.h"
#include "SABUtils/utils.h"

#include <QApplication>
#include <QLabel>
#include <QVariant>
#include <QCommandLineParser>
#include <QSharedPointer>
#include <iostream>
#include <string>
#include <qt_windows.h>

int waitForPrompt(bool consoleCreated, int value)
{
    return consoleCreated ? NSABUtils::waitForPrompt(value) : value;
}

int runCLI(QSharedPointer< QCoreApplication > & appl)
{
    bool consoleCreated = false;
    if (!NSABUtils::runningAsConsole())
    {
        QString msg;
        auto aOK = NSABUtils::attachConsole(&msg);
        if (!aOK)
        {
            msg = msg.trimmed();
            std::cerr << msg.toStdString() << std::endl;
            return NSABUtils::waitForPrompt( -1 );
        }
        consoleCreated = true;
    }

    Q_INIT_RESOURCE(MainLib);

    NVSProjectMaker::registerTypes();

    QCommandLineParser parser;
    parser.setApplicationDescription("VS Project Maker");
    auto helpOption = parser.addHelpOption();

    QCommandLineOption optionsFileOption(QStringList() << "options" << "o", "The options INI file (required)", "Options file");
    parser.addOption(optionsFileOption);
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    if (!parser.parse(appl->arguments()))
    {
        std::cerr << parser.errorText().toStdString() << "\n";
        return waitForPrompt( consoleCreated, -1 );
    }
    if (parser.isSet(helpOption))
    {
        parser.showHelp();
        return waitForPrompt( consoleCreated, 0);
    }

    if (!parser.isSet(optionsFileOption))
    {
        std::cerr << "-options must be set\n";
        auto txt = parser.helpText();
        std::cerr << txt.toStdString() << "\n";
        return waitForPrompt( consoleCreated, -1);
    }

    std::cout << "PWD: " << qPrintable(QDir::currentPath()) << "\n";
    std::cout << "CLI: ";
    auto args = appl->arguments();
    for ( int ii =0; ii < args.count(); ++ii )
    {
        if ( ii == 0 )
        {
            std::cout << QFileInfo(args[ ii ] ).fileName().toStdString();
        }
        else
        {
            std::cout << " ";
            std::cout << args[ ii ].toStdString();
        }
    }
    std::cout << "\n";


    auto settingsFile = parser.value(optionsFileOption);
    NVSProjectMaker::CSettings settings;
    if (!settings.loadSettings(settingsFile))
    {
        std::cerr << "Options file not found\n";
        return waitForPrompt( consoleCreated, -1);
    }

    auto clientDir = QDir(settings.getClientDir());
    if (!clientDir.exists())
    {
        std::cerr << "Client directory '" << clientDir.absolutePath().toStdString() << "' does not exist.\n";
        return waitForPrompt( consoleCreated, -1);
    }
    std::cout << "Finding directories\n";
    if (settings.loadSourceFiles(clientDir.absoluteFilePath(settings.getSourceRelativeDir()), clientDir.absoluteFilePath(settings.getSourceRelativeDir()), nullptr,
        [](const QString & msg) { std::cout << msg.toStdString() << "\n"; }))
    {
        std::cerr << "Could not load directories\n";
        return waitForPrompt( consoleCreated, -1);
    }
    settings.addInclDirs(settings.getResults()->fInclDirs);

    std::cout
        << "============================================" << "\n"
        << settings.getResults()->getText(true).toStdString() << "\n";

    settings.generate(nullptr, nullptr,
        [](const QString & msg) { std::cout << msg.toStdString() << "\n"; }
    );

    std::cout << "============================================" << "\n"
        << "Running CMake" << "\n";

    QProcess process;
    auto retVal = settings.runCMake([](const QString & outMsg) { std::cout << outMsg.toStdString(); }, [](const QString & errMsg) { std::cerr << errMsg.toStdString(); }, &process, { true, {} });
    for (auto && ii : retVal.second)
        QObject::disconnect(ii);
    return consoleCreated ? NSABUtils::waitForPrompt(retVal.first) : retVal.first;
}

bool hasCLIOptions(int argc, char ** /*argv*/)
{
    return argc != 1;
}

int runGUI( QSharedPointer< QCoreApplication > & /*appl*/)
{
    Q_INIT_RESOURCE(MainWindow);
    CMainWindow mainWindow;
    return mainWindow.exec();
}

int main( int argc, char ** argv )
{
    Q_INIT_RESOURCE(MainLib);

    auto isCLIMode = hasCLIOptions(argc, argv);

    QSharedPointer<QCoreApplication> appl = isCLIMode ? QSharedPointer< QCoreApplication >(new QCoreApplication(argc, argv)) : QSharedPointer< QApplication >(new QApplication(argc, argv));
    appl->setApplicationName("VSProjectMaker");
    appl->setApplicationVersion("0.9");
    appl->setOrganizationName("Scott Aron Bloom");
    appl->setOrganizationDomain("www.towel42.com");

    if (isCLIMode)
        return runCLI( appl );
    else
        return runGUI( appl );
}
