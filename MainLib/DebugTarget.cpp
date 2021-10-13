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

#include "DebugTarget.h"
#include "Settings.h"

#include <QApplication>
#include <QMessageBox>
#include <QFile>
#include <QDebug>

QDebug & operator<<( QDebug & stream, const NVSProjectMaker::SDebugTarget & value )
{
    stream.noquote().nospace() << "("
        << value.fSourceDir << ", "
        << value.fName << ", "
        << value.fCmd << ", "
        << value.fArgs << ", "
        << value.fWorkDir << ", "
        << value.fEnvVars 
        << ")";
        ;
    return stream;
}

QDataStream & operator<<( QDataStream & stream, const NVSProjectMaker::SDebugTarget & value )
{
    stream << value.fSourceDir
        << value.fName
        << value.fCmd
        << value.fArgs
        << value.fWorkDir
        << value.fEnvVars
        ;
    return stream;
}

QDataStream & operator>>( QDataStream & stream, NVSProjectMaker::SDebugTarget & value )
{
    stream >> value.fSourceDir;
    stream >> value.fName;
    stream >> value.fCmd;
    stream >> value.fArgs;
    stream >> value.fWorkDir;
    stream >> value.fEnvVars;

    return stream;
}

QDataStream & operator<<( QDataStream & stream, const std::list< NVSProjectMaker::SDebugTarget > & value )
{
    stream << value.size();
    for ( auto ii = value.begin(); ii != value.end(); ++ii )
    {
        stream << *ii;
    }
    return stream;
}

QDataStream & operator>>( QDataStream & stream, std::list< NVSProjectMaker::SDebugTarget > & value )
{
    int count;
    stream >> count;

    for ( int ii = 0; ii < count; ++ii )
    {
        NVSProjectMaker::SDebugTarget curr;
        stream >> curr;
        value.push_back( curr );
    }
    return stream;
}

namespace NVSProjectMaker
{
    void ToJson( const SDebugTarget & value, QJsonValue& val )
    {
        value.toJson( val );
    }

    void FromJson( SDebugTarget & value, const QJsonValue & val )
    {
        value.fromJson( val );
    }

    void SDebugTarget::toJson( QJsonValue& value ) const
    {
        QJsonObject obj;
        obj["sourcedir"] = fSourceDir;
        obj["name"] = fName;
        obj["cmd"] = fCmd;
        QJsonValue args;
        ToJson( fArgs, args );
        obj["args"] = args;
        obj["workdir"] = fWorkDir;
        QJsonValue envVar;
        ToJson( fEnvVars, envVar );
        obj["envvars"] = envVar;
        value = obj;
    }

    void SDebugTarget::fromJson( const QJsonValue& value )
    {
        if ( !value.isObject() )
            return;
        auto obj = value.toObject();
        fSourceDir = obj["sourcedir"].toString();
        fName = obj["name"].toString();
        fCmd = obj["cmd"].toString();
        FromJson( fArgs, obj["args"] );
        fWorkDir = obj["workdir"].toString();
        fSourceDir = obj["sourcedir"].toString();
        FromJson( fEnvVars, obj["envvars"] );
    }

    QString SDebugTarget::getEnvVars() const
    {
        QStringList tmp;
        for (auto&& ii : fEnvVars)
        {
            auto curr = ii.trimmed();
            if (!curr.startsWith("{"))
                curr = "{" + curr;
            if (!curr.endsWith("}"))
                curr += "}";
            tmp << curr;
        }
        auto retVal = tmp.join( '\n' );
        return retVal;
    }

    QString SDebugTarget::getProjectName() const
    {
        QString retVal = fName;
        retVal.replace( "/", "_" );
        retVal = retVal.replace( "\\", "_" );
        retVal = retVal.replace( " ", "_" );
        return retVal;
    }

    QString SDebugTarget::getCmd() const
    {
        QString retVal = fCmd;
        retVal.replace( "\\", "/" );
        return retVal;
    }

    void SDebugTarget::cleanUp( const CSettings * settings )
    {
        fCmd = settings->cleanUp( fCmd );
        fArgs = settings->cleanUp( fArgs );
        fWorkDir = settings->cleanUp( fWorkDir );
        fEnvVars = settings->cleanUp( fEnvVars );
    }
}