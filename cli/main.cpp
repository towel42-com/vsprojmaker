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

#include "MainLib/Settings.h"
#include "MainLib/VSProjectMaker.h"

#include <QApplication>
#include <QLabel>
#include <QVariant>
#include <QCommandLineParser>
#include <QProcess>
#include <iostream>

int main( int argc, char ** argv )
{
    QApplication appl( argc, argv );
    appl.setApplicationName( "VSProjectMaker" );
    appl.setApplicationVersion( "0.9" );
    appl.setOrganizationName( "Scott Aron Bloom" );
    appl.setOrganizationDomain( "www.towel42.com" );

    Q_INIT_RESOURCE( application );

    NVSProjectMaker::registerTypes();

    QCommandLineParser parser;
    parser.setApplicationDescription( "VS Project Maker CLI" );
    auto helpOption = parser.addHelpOption();

    QCommandLineOption optionsFileOption( QStringList() << "options" << "o", "The options INI file (required)", "Options file" );
    parser.addOption( optionsFileOption );
    parser.setSingleDashWordOptionMode( QCommandLineParser::ParseAsLongOptions );

    if ( !parser.parse( appl.arguments() ) )
    {
        std::cerr << parser.errorText().toStdString() << "\n";
        return -1;
    }
    if ( parser.isSet( helpOption ) )
    {
        parser.showHelp();
        return 0;
    }

    if ( !parser.isSet( optionsFileOption ) )
    {
        std::cerr << "-options must be set\n";
        auto txt = parser.helpText();
        std::cerr << txt.toStdString() << "\n";
        return -1;
    }

    auto settingsFile = parser.value( optionsFileOption );
    NVSProjectMaker::CSettings settings;
    if ( !settings.loadSettings( settingsFile ) )
    {
        std::cerr << "Options file not found\n";
        return -1;
    }

    auto clientDir = QDir( settings.getClientDir() );
    if ( !clientDir.exists() )
    {
        std::cerr << "Client directory '" << clientDir.absolutePath().toStdString() << "' does not exist.\n";
        return -1;
    }
    std::cout << "Finding directories\n";
    if ( settings.loadSourceFiles( clientDir.absoluteFilePath( settings.getSourceRelDir() ), clientDir.absoluteFilePath( settings.getSourceRelDir() ), nullptr,
         []( const QString & msg ) { std::cout << msg.toStdString() << "\n"; } ) )
    {
        std::cerr << "Could not load directories\n";
        return -1;
    }
    settings.addInclDirs( settings.getResults()->fInclDirs );

    std::cout
        << "============================================" << "\n"
        << settings.getResults()->getText( true ).toStdString() << "\n";

    settings.generate( nullptr, nullptr,
                       []( const QString & msg ) { std::cout << msg.toStdString() << "\n"; }
    );

    std::cout << "============================================" << "\n"
        << "Running CMake" << "\n";

    auto buildDir = settings.getBuildDir().value();
    auto cmakeExec = settings.getCMakePath();
    auto args = settings.getCmakeArgs();
    std::cout
        << QString( "Build Dir: %1" ).arg( buildDir ).toStdString() << "\n"
        << QString( "CMake Path: %1" ).arg( cmakeExec ).toStdString() << "\n"
        << QString( "Args: %1" ).arg( args.join( " " ) ).toStdString() << "\n"
        ;

    QProcess process;
    QObject::connect( &process, &QProcess::readyReadStandardError, 
             [&process]()
    {
        std::cerr << process.readAllStandardError().toStdString();
    } );
    QObject::connect( &process, &QProcess::readyReadStandardOutput, 
                      [&process]()
    {
        std::cerr << process.readAllStandardOutput().toStdString();
    } );

    QObject::connect( &process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
             [&process]( int exitCode, QProcess::ExitStatus status )
    {
        std::cout
            << "============================================" << "\n"
            << QString( "Finished: Exit Code: %1 Status: %2 " ).arg( exitCode ).arg( status == QProcess::NormalExit ? "Normal" : "Crashed" ).toStdString() << "\n";
        QCoreApplication::quit();
    } );
    process.setWorkingDirectory( buildDir );
    process.start( cmakeExec, args );

    return appl.exec();
}