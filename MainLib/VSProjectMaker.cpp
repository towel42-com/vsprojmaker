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

#include "DebugCmd.h"
#include <QSet>
#include <QDataStream>
#include <QFile>
#include <QApplication>
#include <QMessageBox>

QDataStream & operator<<( QDataStream & stream, const QSet<QString> & value )
{
    stream << value.count();
    for ( auto ii = value.constBegin(); ii != value.constEnd(); ++ii )
    {
        stream << *ii;
    }
    return stream;
}

QDataStream & operator>>( QDataStream & stream, QSet<QString> & value )
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

QDataStream & operator<<( QDataStream & stream, const QPair< QString, bool > & value )
{
    stream << value.first << value.second;
    return stream;
}

QDataStream & operator>>( QDataStream & stream, QPair< QString, bool > & value )
{
    stream >> value.first;
    stream >> value.second;
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const QPair< QString, QString > & value )
{
    stream << value.first << value.second;
    return stream;
}

QDataStream & operator>>( QDataStream & stream, QPair< QString, QString > & value )
{
    stream >> value.first;
    stream >> value.second;
    return stream;
}


QDataStream & operator<<( QDataStream & stream, const QList< QPair< QString, bool > > & value )
{
    stream << value.count();
    for ( auto ii = value.constBegin(); ii != value.constEnd(); ++ii )
    {
        stream << *ii;
    }
    return stream;
}

QDataStream & operator>>( QDataStream & stream, QList< QPair< QString, bool > > & value )
{
    int count;
    stream >> count;

    for ( int ii = 0; ii < count; ++ii )
    {
        QPair< QString, bool > curr;
        stream >> curr;
        value.push_back( curr );
    }
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const TListOfStringPair & value )
{
    stream << value.count();
    for ( auto ii = value.constBegin(); ii != value.constEnd(); ++ii )
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
        QPair< QString, QString > curr;
        stream >> curr;
        value.push_back( curr );
    }
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const QHash< QString, QList< QPair< QString, bool > > > & value )
{
    stream << value.count();
    for ( auto ii = value.constBegin(); ii != value.constEnd(); ++ii )
    {
        stream << ii.key();
        stream << ii.value();
    }
    return stream;
}

QDataStream & operator>>( QDataStream & stream, QHash< QString, QList< QPair< QString, bool > > > & value )
{
    int count;
    stream >> count;

    for ( int ii = 0; ii < count; ++ii )
    {
        QString key;
        stream >> key;
        QList< QPair< QString, bool > > currValue;
        stream >> currValue;
        value.insert( key, currValue );
    }
    return stream;
}

namespace NVSProjectMaker
{
    QString readResourceFile( QWidget * parent, const QString & resourceFile, const std::function< void( QString & data ) > & function )
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
        qRegisterMetaTypeStreamOperators< QSet<QString> >( "QSet<QString>" );
        qRegisterMetaTypeStreamOperators< QHash<QString, QList< QPair< QString, bool > >> >( "QHash<QString,QList<QPair<QString,bool>>>" );
        qRegisterMetaTypeStreamOperators< QList< QPair< QString, bool > > >( "QList<QPair<QString,bool>>" );
        qRegisterMetaTypeStreamOperators< QPair< QString, bool > >( "QPair<QString,bool>" );
        qRegisterMetaTypeStreamOperators< TListOfStringPair >( "TListOfStringPair" );
        qRegisterMetaTypeStreamOperators< QPair< QString, QString > >( "QPair<QString,QString>" );
        qRegisterMetaTypeStreamOperators< SDebugCmd >( "SDebugCmd" );
        qRegisterMetaTypeStreamOperators< QList< SDebugCmd > >( "QList< SDebugCmd >" );
    }
}