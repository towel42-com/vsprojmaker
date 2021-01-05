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

#include "DebugCmd.h"
#include "Settings.h"
#include "VSProjectMaker.h"

#include <QString>
#include <QStringList>
#include <QList>
#include <QSet>
#include <QSettings>
#include <QFileInfo>

namespace NVSProjectMaker
{
    CSettings::CSettings()
    {
        NVSProjectMaker::registerTypes();

        registerSettings();
        loadSettings();
    }

    CSettings::CSettings( const QString & fileName )
    {
        NVSProjectMaker::registerTypes();

        registerSettings();
        loadSettings( fileName );
    }

    CSettings::~CSettings()
    {
        saveSettings();
    }

    QString CSettings::fileName() const
    {
        if ( !fSettingsFile )
            return QString();
        auto fname = fSettingsFile->fileName();
        if ( fname.startsWith( "\\HKEY" ) )
            return QString();
        return fname;
    }

    bool CSettings::saveSettings()
    {
        if ( !fSettingsFile )
            return false;

        for ( auto && ii : fSettings )
            fSettingsFile->setValue( ii.first, *(ii.second) );

        return true;
    }

    void CSettings::loadSettings()
    {
        loadSettings( QString() );
    }

    bool CSettings::loadSettings( const QString & fileName )
    {
        saveSettings();
        if ( fileName.isEmpty() )
            fSettingsFile = std::make_unique< QSettings >();
        else
            fSettingsFile = std::make_unique< QSettings >( fileName, QSettings::IniFormat );
        return loadData();
    }

    void CSettings::registerSettings()
    {
        ADD_SETTING_VALUE( QString, CMakePath );
        ADD_SETTING_VALUE( QString, Generator );
        ADD_SETTING_VALUE( QString, ClientDir );
        ADD_SETTING_VALUE( QString, SourceRelDir );
        ADD_SETTING_VALUE( QString, BuildRelDir );
        ADD_SETTING_VALUE( QString, QtDir );
        ADD_SETTING_VALUE( QStringList, QtLibs );
        ADD_SETTING_VALUE( QSet< QString >, BuildDirs );
        ADD_SETTING_VALUE( QStringList, InclDirs );
        ADD_SETTING_VALUE( QStringList, SelectedInclDirs );
        ADD_SETTING_VALUE( TExecNameType, ExecNames );
        ADD_SETTING_VALUE( TListOfStringPair, CustomBuilds );
        ADD_SETTING_VALUE( TListOfDebugCmd, DebugCommands );
    }

    bool CSettings::loadData()
    {
        auto keys = fSettingsFile->childKeys();
        for ( auto && ii : keys )
        {
            auto pos = fSettings.find( ii );
            if ( pos == fSettings.end() )
                continue;

            auto value = fSettingsFile->value( ii );
            *( *pos ).second = value;
        }
        return true;
    }
}