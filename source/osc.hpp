#pragma once

#include <QObject>
#include <QVariant>
#include <QDataStream>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

namespace WPN114  {
namespace Network {

//=================================================================================================
struct OSCMessage
//=================================================================================================
{
    QString
    m_method;

    QVariant
    m_arguments;

    //---------------------------------------------------------------------------------------------
    OSCMessage() {}

    OSCMessage(QString method, QVariant arguments) :
        m_method(method), m_arguments(arguments) {}

    OSCMessage(QByteArray const& data);

    //---------------------------------------------------------------------------------------------
    QByteArray
    encode() const;

    QString
    typetag(QVariant const& argument) const;

    void
    append(QByteArray& data, QVariant const& argument) const;

};
}
}
