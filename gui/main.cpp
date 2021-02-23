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
#include <iostream>
#include <QApplication>
#include <QLabel>
#include <QVariant>

static uint typeOfVariant( const QVariant & value )
{
    //return 0 for integer, 1 for floating point and 2 for other
    switch ( value.userType() )
    {
        case QVariant::Bool:
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong:
        case QVariant::Char:
        case QMetaType::Short:
        case QMetaType::UShort:
        case QMetaType::UChar:
        case QMetaType::ULong:
        case QMetaType::Long:
        return 0;
        case QVariant::Double:
        case QMetaType::Float:
        return 1;
        default:
        return 2;
    }
}

bool variantLessThan( const QVariant & v1, const QVariant & v2 )
{
    switch ( qMax( typeOfVariant( v1 ), typeOfVariant( v2 ) ) )
    {
        case 0: //integer type
        return v1.toLongLong() < v2.toLongLong();
        case 1: //floating point
        return v1.toReal() < v2.toReal();
        default:
        return v1.toString().localeAwareCompare( v2.toString() ) < 0;
    }
}

int main( int argc, char ** argv )
{
    QApplication appl( argc, argv );
    appl.setApplicationName( "VSProjectMaker" );
    appl.setApplicationVersion( "0.9" );
    appl.setOrganizationName( "Scott Aron Bloom" );
    appl.setOrganizationDomain( "www.towel42.com" );

    Q_INIT_RESOURCE( MainWindow );
    Q_INIT_RESOURCE( MainLib );
    CMainWindow mainWindow;
    return mainWindow.exec();
}
