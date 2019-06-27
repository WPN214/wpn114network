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
