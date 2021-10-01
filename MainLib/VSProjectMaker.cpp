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

#include "VSProjectMaker.h"
#include "Settings.h"
#include "DebugTarget.h"

#include <set>
#include <QDataStream>
#include <QFile>
#include <QApplication>
#include <QMessageBox>

QDataStream & operator<<( QDataStream & stream, const TStringSet & value )
{
    stream << value.size();
    for ( auto ii = value.begin(); ii != value.end(); ++ii )
    {
        stream << *ii;
    }
    return stream;
}

QDebug& operator<<( QDebug& stream, const TStringSet& value )
{
    const bool oldSetting = stream.autoInsertSpaces();
    stream.nospace() << "std::set< QString >(";
    auto && ii = value.begin();
    if ( ii != value.end() )
        stream << *ii;
    while ( ii != value.end() )
    {
        stream << ", " << *ii;
        ++ii;
    }
    stream.nospace() << ')';
    stream.setAutoInsertSpaces( oldSetting );
    return stream.maybeSpace();
}

QDataStream & operator>>( QDataStream & stream, TStringSet & value )
{
    int count;
    stream >> count;

    for ( int ii = 0; ii < count; ++ii )
    {
        QString key;
        stream >> key;
        value.insert( key );
    }
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const std::pair< QString, bool > & value )
{
    stream << value.first << value.second;
    return stream;
}

QDataStream & operator>>( QDataStream & stream, std::pair< QString, bool > & value )
{
    stream >> value.first;
    stream >> value.second;
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const std::pair< QString, QString > & value )
{
    stream << value.first << value.second;
    return stream;
}

QDataStream & operator>>( QDataStream & stream, std::pair< QString, QString > & value )
{
    stream >> value.first;
    stream >> value.second;
    return stream;
}


QDataStream & operator<<( QDataStream & stream, const std::list< std::pair< QString, bool > > & value )
{
    stream << value.size();
    for ( auto ii = value.begin(); ii != value.end(); ++ii )
    {
        stream << *ii;
    }
    return stream;
}

QDataStream & operator>>( QDataStream & stream, std::list< std::pair< QString, bool > > & value )
{
    int count;
    stream >> count;

    for ( int ii = 0; ii < count; ++ii )
    {
        std::pair< QString, bool > curr;
        stream >> curr;
        value.push_back( curr );
    }
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const TListOfStringPair & value )
{
    stream << value.size();
    for ( auto ii = value.begin(); ii != value.end(); ++ii )
    {
        stream << *ii;
    }
    return stream;
}

QDataStream & operator>>( QDataStream & stream, TListOfStringPair & value )
{
    int count;
    stream >> count;

    for ( int ii = 0; ii < count; ++ii )
    {
        std::pair< QString, QString > curr;
        stream >> curr;
        value.push_back( curr );
    }
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const std::unordered_map< QString, std::list< std::pair< QString, bool > > > & value )
{
    stream << value.size();
    for ( auto ii = value.begin(); ii != value.end(); ++ii )
    {
        stream << (*ii).first;
        stream << (*ii).second;
    }
    return stream;
}

QDataStream & operator>>( QDataStream & stream, std::unordered_map< QString, std::list< std::pair< QString, bool > > > & value )
{
    int count;
    stream >> count;

    for ( int ii = 0; ii < count; ++ii )
    {
        QString key;
        stream >> key;
        std::list< std::pair< QString, bool > > currValue;
        stream >> currValue;
        value[key] = currValue;
    }
    return stream;
}

namespace NVSProjectMaker
{
    QString readResourceFile( QWidget * parent, const QString & resourceFile, const std::function< void( QString & resourceText ) > & function )
    {
        QFile fi( resourceFile );
        if ( !fi.open( QIODevice::OpenModeFlag::ReadOnly | QIODevice::OpenModeFlag::Text ) )
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical( parent, QObject::tr( "Error:" ), QObject::tr( "Error opening resource file '%1'\n%2" ).arg( resourceFile ).arg( fi.errorString() ) );
            return QString();
        }
        auto resourceText = QString( fi.readAll() );
        function( resourceText );
        return resourceText;
    }

    void registerTypes()
    {
        qRegisterMetaTypeStreamOperators< TStringSet >( "TStringSet" );
        qRegisterMetaTypeStreamOperators< std::unordered_map<QString, std::list< std::pair< QString, bool > >> >( "std::unordered_map<QString,std::list<std::pair<QString,bool>>>" );
        qRegisterMetaTypeStreamOperators< std::list< std::pair< QString, bool > > >( "std::list<std::pair<QString,bool>>" );
        qRegisterMetaTypeStreamOperators< std::pair< QString, bool > >( "std::pair<QString,bool>" );
        qRegisterMetaTypeStreamOperators< TListOfStringPair >( "TListOfStringPair" );
        qRegisterMetaTypeStreamOperators< std::pair< QString, QString > >( "std::pair<QString,QString>" );
        qRegisterMetaTypeStreamOperators< SDebugTarget >( "SDebugTarget" );
        qRegisterMetaTypeStreamOperators< std::list< SDebugTarget > >( "std::list< SDebugTarget >" );
    }
}