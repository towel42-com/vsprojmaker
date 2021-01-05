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

#ifndef __SETTINGS_H
#define __SETTINGS_H

#include <utility>
#include <QVariant>
#include <QSettings>
#include <QSet>

#define ADD_SETTING( Type, Name ) \
public: \
inline void set ## Name( const Type & value ){ f ## Name = QVariant::fromValue( value ); } \
inline Type get ## Name() const { return f ## Name.value< Type >(); } \
private: \
QVariant f ## Name

#define ADD_SETTING_VALUE( Type, Name ) \
registerSetting< Type >( #Name, const_cast<QVariant *>( &f ## Name ) );

namespace NVSProjectMaker
{
    struct SDebugCmd;
}

using TExecNameType = QHash< QString, QList< QPair< QString, bool > > >;
using TListOfStringPair = QList< QPair< QString, QString > >;
using TListOfDebugCmd = QList< NVSProjectMaker::SDebugCmd >;

namespace NVSProjectMaker
{
    class CSettings
    {
    public:
        CSettings();
        CSettings( const QString & fileName );

        ~CSettings();

        QString fileName() const;

        bool saveSettings(); // only valid if filename has been set;
        void loadSettings(); // loads from the registry
        bool loadSettings( const QString & fileName ); // sets the filename and loads from it
        void setFileName( const QString & fileName, bool andSave=true );

        ADD_SETTING( QString, CMakePath );
        ADD_SETTING( QString, Generator );
        ADD_SETTING( QString, ClientDir );
        ADD_SETTING( QString, SourceRelDir );
        ADD_SETTING( QString, BuildRelDir );
        ADD_SETTING( QString, QtDir );
        ADD_SETTING( QStringList, QtLibs );
        ADD_SETTING( QSet< QString >, BuildDirs );
        ADD_SETTING( QStringList, InclDirs );
        ADD_SETTING( QStringList, SelectedInclDirs );
        ADD_SETTING( TExecNameType, ExecNames );
        ADD_SETTING( TListOfStringPair, CustomBuilds );
        ADD_SETTING( TListOfDebugCmd, DebugCommands );
    private:
        template< typename Type >
        void registerSetting( const QString & attribName, QVariant * value ) const
        {
            auto pos = fSettings.find( attribName );
            if ( pos != fSettings.end() )
                return;

            Q_ASSERT( value );
            if ( !value )
                return;
            if ( !value->isValid() )
                *value = QVariant::fromValue( Type() );
            fSettings[ attribName ] = value;
        }
        bool loadData();

        void registerSettings();
        std::unique_ptr< QSettings > fSettingsFile;

        mutable std::unordered_map< QString, QVariant * > fSettings;
    };
}

#endif 
