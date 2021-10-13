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

#ifndef __DEBUGCMD_H
#define __DEBUGCMD_H

#include <QString>
#include <QMap>
#include <QDataStream>

class QJsonObject;

namespace NVSProjectMaker
{
    class CSettings;
    struct SDebugTarget
    {
        QString fSourceDir;
        QString fName;
        QString fCmd;
        QStringList fArgs;
        QString fWorkDir;
        QStringList fEnvVars;

        void toJson( QJsonValue& obj ) const;
        void fromJson( const QJsonValue& obj );
        QString getEnvVars() const;
        QString getProjectName() const;
        QString getCmd() const;
        void cleanUp( const CSettings * settings );
    };
    void ToJson( const NVSProjectMaker::SDebugTarget & value, QJsonValue& obj );
    void FromJson( NVSProjectMaker::SDebugTarget& value, const QJsonValue& obj );
}

QDataStream & operator<<( QDataStream & stream, const NVSProjectMaker::SDebugTarget & value );
QDataStream & operator>>( QDataStream & stream, NVSProjectMaker::SDebugTarget & value );
QDataStream & operator<<( QDataStream & stream, const std::list< NVSProjectMaker::SDebugTarget > & value );
QDataStream & operator>>( QDataStream & stream, std::list< NVSProjectMaker::SDebugTarget > & value );

QDebug & operator<<( QDebug & stream, const NVSProjectMaker::SDebugTarget & value );

Q_DECLARE_METATYPE( NVSProjectMaker::SDebugTarget );

#endif 

