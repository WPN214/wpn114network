#include "network.hpp"

Tree* Tree::s_singleton;

//---------------------------------------------------------------------------------------------
template<typename _Valuetype> void
from_stream(QVariantList& arglst, QDataStream& stream)
//---------------------------------------------------------------------------------------------
{
    _Valuetype target_value;
    stream >> target_value;
    arglst << target_value;
}

//---------------------------------------------------------------------------------------------
template<> void
from_stream<QString>(QVariantList& arglist, QDataStream& stream)
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
OSCMessage::OSCMessage(QByteArray const& data)
//---------------------------------------------------------------------------------------------
{
    QDataStream stream(data);
    QString typetag;
    QVariantList arguments;

    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::BigEndian);

    auto comsep = data.split(',');
    m_method = comsep[0];
    typetag = comsep[1].split('\0')[0];

    uint8_t adpads = 4-(m_method.count()%4);
    uint8_t ttpads = 4-((typetag.count()+1)%4);
    uint32_t total = m_method.count()+adpads+typetag.count()+1+ttpads;

    stream.skipRawData(total);

    for (const auto& c : typetag) {
        switch(c.toLatin1()) {
            case 'i': from_stream<int>(arguments, stream); break;
            case 'f': from_stream<float>(arguments, stream); break;
            case 's': from_stream<QString>(arguments, stream); break;
            case 'T': arguments << true; break;
            case 'F': arguments << false; break;
        }}

    switch(arguments.count()) {
        case 0: break;
        case 1: m_arguments = arguments[0]; break;
        default: m_arguments = arguments;
    }
}
