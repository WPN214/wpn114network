#include "osc.hpp"
#include <QDataStream>
#include <QNetworkDatagram>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QVariant>

OSCHandler::OSCHandler() :
    m_local_port(0), m_remote_port(0), m_remote_address("127.0.0.1"), m_udpsocket(new QUdpSocket)
{

}

OSCHandler::~OSCHandler()
{
    delete m_udpsocket;
}

void OSCHandler::componentComplete() {}

void OSCHandler::listen()
{
    QObject::connect ( m_udpsocket, &QUdpSocket::readyRead, this, &OSCHandler::readPendingDatagrams);
    m_udpsocket->bind( QHostAddress::Any, m_local_port );

    m_local_port = m_udpsocket->localPort();
}

void OSCHandler::setLocalPort(uint16_t port)
{
    m_local_port = port;
}

void OSCHandler::setRemotePort(uint16_t port)
{
    m_remote_port = port;
}

void OSCHandler::setRemoteAddress(QString address)
{
    m_remote_address = address;
}

void OSCHandler::readOSCBundle(QByteArray bundle)
{
    QDataStream stream(bundle);

    // skipping 16 first bytes, including time-tag, useless for now
    stream.skipRawData(16);

    while ( !stream.atEnd() )
    {
        quint32     element_sz;
        stream >>   element_sz;
        QByteArray  message(element_sz, '\0');

        stream.readRawData  ( message.data(), element_sz );
        readOSCMessage      ( message );
    }
}

template<typename T>
inline void parseArgumentsFromStream(QVariantList& dest, QDataStream& stream)
{
    T value; stream >> value; dest << value;
}

OSCMessage OSCHandler::decode(const QByteArray& data)
{
    QString         address, typetag;
    QVariantList    arguments;
    QDataStream     stream(data);
    OSCMessage      message;

    stream.setFloatingPointPrecision ( QDataStream::SinglePrecision );

    auto split  = data.split(',');
    address     = split[0];
    typetag     = split[1].split('\0')[0];

    uint8_t adpads = 4-(address.size()%4);
    uint8_t ttpads = 4-((typetag.size()+1)%4);
    uint32_t total = address.size()+adpads+typetag.size()+ttpads+1;

    stream.skipRawData(total);

    for ( const auto& c : typetag )
    {
        if      ( c == 'i' ) parseArgumentsFromStream<int>(arguments, stream);
        else if ( c == 'f' ) parseArgumentsFromStream<float>(arguments, stream);
        else if ( c == 'T' ) arguments << true;
        else if ( c == 'F' ) arguments << false;
        else if ( c == 's' )
        {
            quint8  byte, padding;
            QByteArray res;

            stream >> byte;

            while ( byte ) {
                res.append(byte);
                stream >> byte;
            }

            // we add padding after string's end
            padding = 4-(res.size()%4);
            stream.skipRawData(padding-1);

            arguments << QString::fromUtf8(res);
        }
    }

    message.address = address;

    if ( arguments.size() == 0 ) return message;
    else if ( arguments.size() == 1 )
         message.arguments = arguments[0];
    else message.arguments = arguments;

    return message;
}

void OSCHandler::readOSCMessage(QByteArray data)
{
    OSCMessage message = decode(data);
    emit messageReceived(message.address, message.arguments);
    qDebug() << "OSCMessage received:" << message.address << message.arguments;
}

void OSCHandler::readPendingDatagrams()
{
    while ( m_udpsocket->hasPendingDatagrams() )
    {
        QNetworkDatagram datagram = m_udpsocket->receiveDatagram();
        QByteArray data = datagram.data();

        if       ( data.startsWith('#') ) readOSCBundle(data);
        else if  ( data.startsWith('/') ) readOSCMessage(data);
    }
}

QString OSCHandler::typeTag(const QVariant& argument)
{
    if ( QString(argument.typeName()) == "QJSValue" )
    {
        QString tt;
        for ( const auto& sub : argument.toList() )
            tt.append(typeTag(sub));

        return tt;
    }

    switch(argument.type())
    {
    case QMetaType::Void:         return "N";
    case QMetaType::Int:          return "i";
    case QMetaType::Float:        return "f";
    case QMetaType::Double:       return "f";
    case QMetaType::QString:      return "s";
    case QMetaType::QVector2D:    return "ff";
    case QMetaType::QVector3D:    return "fff";
    case QMetaType::QVector4D:    return "ffff";
    case QMetaType::Bool:
    {
        if ( argument.toBool() )  return "T";
        else return "F";
    }
    case QMetaType::QVariantList:
    {
        QString tt;
        for ( const auto& sub : argument.toList() )
            tt.append(typeTag(sub));

        return tt;
    }

    default: return "N";
    }
}

void OSCHandler::append(QByteArray& data, const QVariant& argument)
{    
    if ( QString(argument.typeName()) == "QJSValue" )
    {
        for ( const auto& v : argument.toList())
            append(data, v);
        return;
    }

    switch(argument.type())
    {
    case QMetaType::Void: return;
    case QMetaType::Bool: return;
    case QMetaType::QString:
    {
        QByteArray str  = argument.toString().toUtf8();
        auto pads       = 4-(str.size() % 4);
        while ( pads--) str.append('\0');
        data.append     ( str );
        return;
    }
    }

    QDataStream stream(&data, QIODevice::ReadWrite);
    stream.skipRawData(data.size());

    if ( argument.type() == QMetaType::Int )
    {
        stream << (int) argument.toInt(); return;
    }

    stream.setFloatingPointPrecision ( QDataStream::SinglePrecision );   

    switch(argument.type())
    {
    case QMetaType::Float:
    case QMetaType::Double: stream << (float) argument.toFloat(); break;
    case QMetaType::QVector2D:
    {
        auto vec = qvariant_cast<QVector2D>(argument);
        stream << vec; break;
    }
    case QMetaType::QVector3D:
    {
        auto vec = qvariant_cast<QVector3D>(argument);
        stream << vec; break;
    }
    case QMetaType::QVector4D:
    {
        auto vec = qvariant_cast<QVector4D>(argument);
        stream << vec; break;
    }
    case QMetaType::QVariantList:
    {
        for ( const auto& sub : argument.toList() )
            append(data, sub);
        break;
    }
    }
}

QByteArray OSCHandler::encode(const OSCMessage& message)
{
    QByteArray  data;
    QString     typetag = typeTag(message.arguments).prepend(',');

    data.append(message.address);

    auto    pads = 4-(message.address.size() % 4);
    while   ( pads-- ) data.append((char)0);
    data.append(typetag);

    pads    = 4-(typetag.size() % 4);
    while   ( pads-- ) data.append((char)0);

    append(data, message.arguments);

    return data;
}

void OSCHandler::sendMessage(const OSCMessage& message)
{
    QNetworkDatagram datagram;
    QByteArray data = encode(message);

    datagram.setData ( data );
    datagram.setDestination ( QHostAddress(m_remote_address), m_remote_port );
    m_udpsocket->writeDatagram ( datagram );
}

void OSCHandler::sendMessage(QString address, QVariant arguments)
{
    OSCMessage msg { address, arguments };
    sendMessage(msg);
}


