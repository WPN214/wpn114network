#include "network.hpp"
#include "osc.hpp"

#include <QJsonDocument>

using namespace WPN114::Network;

WPN114::Network::Connection::
Connection(mg_connection* ws_connection) :
    m_ws_connection(ws_connection)
{
    char addr[32], port[5];
    mg_sock_addr_to_str(&ws_connection->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP);
    mg_sock_addr_to_str(&ws_connection->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_PORT);

    m_host_ip = addr;
    m_host_ip.append(":");
    m_host_ip.append(port);
}

void
WPN114::Network::Connection::
set_udp(uint16_t udp)
{
    m_udp_port = udp;
    m_host_udp = m_host_ip;
    m_host_udp.prepend("udp://");
    m_host_udp.append(":");
    m_host_udp.append(QString::number(m_udp_port));

    mg_mgr_init(&m_mgr, this);
}

void WPN114::Network::Connection::
on_value_changed(QVariant value)
{
    auto node = qobject_cast<Node*>(QObject::sender());
    writeOSC(node->path(), value.toList(), node->critical());
}

void
WPN114::Network::Connection::
writeOSC(QString method, QVariantList arguments, bool critical)
{
    OSCMessage msg(method, arguments);
    auto b_arr = msg.encode();

    if (critical) {
         mg_send_websocket_frame(m_ws_connection, WEBSOCKET_OP_BINARY,
                                 b_arr.data(), b_arr.count());
    } else {
        m_udp_connection = mg_connect(&m_mgr, CSTR(m_host_udp), nullptr);
        mg_send(m_udp_connection, b_arr.data(), b_arr.count());
    }
}

void
WPN114::Network::Connection::
writeText(QString text)
{
    auto utf8 = text.toUtf8();
    mg_send_websocket_frame(m_ws_connection, WEBSOCKET_OP_TEXT,
                            utf8.data(), utf8.count());

}

void
WPN114::Network::Connection::
writeJson(QJsonObject object)
{
    auto doc = QJsonDocument(object).toJson(QJsonDocument::Compact);
    mg_send_websocket_frame(m_ws_connection, WEBSOCKET_OP_TEXT,
                            doc.data(), doc.count());
}
