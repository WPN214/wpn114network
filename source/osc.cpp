#include "osc.hpp"


using namespace WPN114::Network;

//---------------------------------------------------------------------------------------------
template<typename _Valuetype> void
deserialize(QVariantList& arglst, QDataStream& stream)
//---------------------------------------------------------------------------------------------
{
    _Valuetype target_value;
    stream >> target_value;
    arglst << target_value;
}

//---------------------------------------------------------------------------------------------
template<> void
deserialize<QString>(QVariantList& arglist, QDataStream& stream)
//---------------------------------------------------------------------------------------------
{
    uint8_t byte;
    QByteArray data;

    stream >> byte;
    while (byte) { data.append(byte); stream >> byte; }

    uint8_t padding = 4-(data.count()%4);
    stream.skipRawData(padding-1);
    arglist << QString::fromUtf8(data);
}

//---------------------------------------------------------------------------------------------
WPN114::Network::OSCMessage::
OSCMessage(QByteArray const& data)
//---------------------------------------------------------------------------------------------
{
    QDataStream stream(data);
    QVariantList arguments;
    QString typetag;

    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::BigEndian);

    auto comsep = data.split(',');
    m_method = comsep[0];
    typetag  = comsep[1].split('\0')[0];

    uint8_t adpads = 4-(m_method.count()%4);
    uint8_t ttpads = 4-((typetag.count()+1)%4);
    uint32_t total = m_method.count()+adpads+typetag.count()+1+ttpads;

    stream.skipRawData(total);

    for (const auto& c : typetag)
    {
        switch (c.toLatin1())
        {
        case 'i':
             deserialize<int>(arguments, stream);
             break;

        case 'f':
             deserialize<float>(arguments, stream);
             break;

        case 's':
             deserialize<QString>(arguments, stream);
             break;

        case 'T':
             arguments << true;
             break;

        case 'F':
             arguments << false;
            break;
        }
    }

    switch (arguments.count())
    {
        case 0:
             break;
        case 1:
             m_arguments = arguments[0];
             break;
        default:
             m_arguments = arguments;
    }
}

QByteArray
WPN114::Network::OSCMessage::
encode() const
{
    QByteArray data;
    QString tt = typetag(m_arguments).prepend(',');
    data.append(m_method);

    auto pads = 4-(m_method.count()%4);
    while (pads--) data.append((char)0);

    data.append(tt);
    pads = 4-(tt.count()%4);

    while (pads--) data.append((char)0);
    append(data, m_arguments);

    return data;
}

QString
WPN114::Network::OSCMessage::
typetag(QVariant const& argument) const
{
    switch (argument.type())
    {
    case QVariant::Bool:
        return argument.value<bool>() ? "T" : "F";

    case QVariant::Int:         return "i";
    case QVariant::Double:      return "f";
    case QVariant::String:      return "s";
    case QVariant::Vector2D:    return "ff";
    case QVariant::Vector3D:    return "fff";
    case QVariant::Vector4D:    return "ffff";
    }

    if (argument.type() == QVariant::List ||
        strcmp(argument.typeName(), "QJSValue") == 0)
    {
        // if argument is QVariantList or QJSValue
        // recursively parse arguments
        QString tag;
        for (const auto& sub: argument.toList())
            tag.append(OSCMessage::typetag(sub));
        return tag;
    }

    return "N";
}

void
WPN114::Network::OSCMessage::
append(QByteArray& data, QVariant const& argument) const
{
    QDataStream stream(&data, QIODevice::ReadWrite);

    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.skipRawData(data.size());

    switch(argument.type())
    {
    case QVariant::Bool:
         stream << argument.value<bool>();
         break;
    case QVariant::Int:
          stream << argument.value<int>();
          break;
    case QVariant::Double:
          stream << argument.value<float>();
          break;
    case QVariant::Vector2D:
         stream << argument.value<QVector2D>();
         break;
    case QVariant::Vector3D:
         stream << argument.value<QVector3D>();
         break;
    case QVariant::Vector4D:
         stream << argument.value<QVector4D>();
         break;

    case QVariant::String:
    {
        QByteArray str = argument.toString().toUtf8();
        auto pads = 4-(str.count()%4);
        while (pads--) str.append('\0');
        data.append(str);
        break;
    }
    case QVariant::List:
    {
        for (const auto& sub : argument.value<QVariantList>())
             OSCMessage::append(data, sub);
    }
    }
}

