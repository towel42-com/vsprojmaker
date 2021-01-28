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

#ifndef _VSPROJECTMAKER_H
#define _VSPROJECTMAKER_H

#include <QString>
#include <functional>
#include <QPair>
#include <QHash>
#include <QList>
#include <QSet>

class QWidget;
class QStandardItem;
class QTextStream;

namespace NVSProjectMaker
{
    enum ERoles
    {
        eIsDirRole = Qt::UserRole + 1,
        eIsBuildDirRole,
        eIsIncludeDirRole,
        eExecutablesRole,
        eRelPathRole
    };

    QString readResourceFile( QWidget * parent, const QString & resourceFile, const std::function< void( QString & resourceText ) > & function = {} );
    void registerTypes();
}

QDataStream & operator<<( QDataStream & stream, const QSet<QString> & value );
QDataStream & operator>>( QDataStream & stream, QSet<QString> & value );
QDataStream & operator<<( QDataStream & stream, const QPair< QString, bool > & value );
QDataStream & operator>>( QDataStream & stream, QPair< QString, bool > & value );
QDataStream & operator<<( QDataStream & stream, const QPair< QString, QString > & value );
QDataStream & operator>>( QDataStream & stream, QPair< QString, QString > & value );
QDataStream & operator<<( QDataStream & stream, const QList< QPair< QString, bool > > & value );
QDataStream & operator>>( QDataStream & stream, QList< QPair< QString, bool > > & value );
QDataStream & operator<<( QDataStream & stream, const QList< QPair< QString, QString > > & value );
QDataStream & operator>>( QDataStream & stream, QList< QPair< QString, QString > > & value );
QDataStream & operator<<( QDataStream & stream, const QHash< QString, QList< QPair< QString, bool > > > & value );
QDataStream & operator>>( QDataStream & stream, QHash< QString, QList< QPair< QString, bool > > > & value );

#endif 
