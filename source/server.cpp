#include "server.hpp"
#include <QJsonDocument>

using namespace WPN114::Network;

WPN114::Network::Server::
Server()
{
    mg_mgr_init(&m_tcp, this);
    mg_mgr_init(&m_udp, this);
}

void
WPN114::Network::Server::
componentComplete()
{
    char
    s_tcp   [5],
    s_udp   [5],
    udp_hdr [32] = "udp://";

    sprintf(s_tcp, "%d", m_tcp_port);
    sprintf(s_udp, "%d", m_udp_port);
    strcat(udp_hdr, s_udp);

    m_tcp_connection = mg_bind(&m_tcp, s_tcp, ws_event_handler);
    m_udp_connection = mg_bind(&m_udp, udp_hdr, udp_event_handler);
    mg_set_protocol_http_websocket(m_tcp_connection);

    m_zeroconf.startServicePublish(CSTR(m_name), "_oscjson._tcp", "local", m_tcp_port);
    m_running = true;

    poll();
}

void
WPN114::Network::Server::
stop()
{
    m_running = false;
    m_mgthread.join();
}

WPN114::Network::Server::
~Server()
{
    stop();
    mg_mgr_free(&m_tcp);
    mg_mgr_free(&m_udp);
}

void
WPN114::Network::Server::
poll()
{
    m_mgthread = std::thread(&Server::server_poll, this);
}

void
WPN114::Network::Server::
server_poll()
{
    while (m_running) {
        mg_mgr_poll(&m_tcp, 200);
        mg_mgr_poll(&m_udp, 200);
    }
}

void
WPN114::Network::Server::
ws_event_handler(mg_connection *mgc, int event, void *data)
{
    auto server = static_cast<Server*>(mgc->mgr->user_data);

    switch(event)
    {
    case MG_EV_RECV:
    {
        break;
    }
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
    {
        QMetaObject::invokeMethod(server, "on_connection",
            Qt::QueuedConnection, Q_ARG(mg_connection*, mgc));
        break;
    }
    case MG_EV_WEBSOCKET_FRAME:
    {
        QMetaObject::invokeMethod(server, "on_websocket_frame",
            Qt::QueuedConnection,
            Q_ARG(mg_connection*, mgc),
            Q_ARG(websocket_message*, static_cast<websocket_message*>(data)));
        break;
    }
    case MG_EV_HTTP_REQUEST:
    {
        http_message* hm = static_cast<http_message*>(data);
        auto uri = QString::fromUtf8(hm->uri.p, hm->uri.len);
        auto qst = QString::fromUtf8(hm->query_string.p, hm->query_string.len);

        QMetaObject::invokeMethod(server, "on_http_request",
            Qt::QueuedConnection,
            Q_ARG(mg_connection*, mgc),
            Q_ARG(QString, uri),
            Q_ARG(QString, qst));

        break;
    }
    case MG_EV_CLOSE:
    {
        QMetaObject::invokeMethod(server, "on_disconnection",
            Qt::QueuedConnection,
            Q_ARG(mg_connection*, mgc));
        break;
    }
    }
}

void
WPN114::Network::Server::
udp_event_handler(mg_connection *mgc, int event, void *data)
{
    auto server = static_cast<Server*>(mgc->mgr->user_data);

    switch(event) {
        case MG_EV_RECV:
        QMetaObject::invokeMethod(server, "on_udp_datagram",
            Qt::QueuedConnection,
            Q_ARG(mg_connection*, mgc));
            break;
    }
}

void
WPN114::Network::Server::
on_connection(mg_connection *con)
{
    m_connections.emplace_back(con);
}

void
WPN114::Network::Server::
on_disconnection(mg_connection *connection)
{

}

void
WPN114::Network::Server::
on_websocket_frame(mg_connection *mgc, websocket_message *message)
{
    QByteArray frame(
                reinterpret_cast<const char*>(message->data),
                message->size);

    if (message->flags & WEBSOCKET_OP_TEXT)
    {
        // it would have to be json
        auto doc = QJsonDocument::fromJson(frame);
        auto obj = doc.object();

        auto command = obj["COMMAND"].toString();

        Connection* sender = nullptr;
        for (auto& connection : m_connections)
            if (connection.mgc() == mgc)
                sender = &connection;

        assert(sender);

        if (command == "LISTEN" || command == "IGNORE")
        {
            auto target = obj["DATA"].toString();

            if (auto node = m_tree.find(target)) {
                if (command == "LISTEN")
                     QObject::connect(node, &Node::valueChanged, sender, &Connection::on_value_changed);
                else QObject::disconnect(node, &Node::valueChanged, sender, &Connection::on_value_changed);
            }
        }

        else if (command == "START_OSC_STREAMING") {
            uint16_t port = obj["DATA"].toObject()["LOCAL_SERVER_PORT"].toInt();
            sender->set_udp(port);

            // at this point it is safe to validate the oscquery connection
            // and send it back to qml
            emit connection(*sender);
        }

        emit websocketMessageReceived(frame);
    }

    else if (message->flags & WEBSOCKET_OP_BINARY) {
        // it would have to be OSC
        OSCMessage oscmg(frame);

        if (auto node = m_tree.find(oscmg.m_method))
            node->set_value(oscmg.m_arguments);

        emit oscMessageReceived(oscmg);
    }
}

void
WPN114::Network::Server::
on_http_request(mg_connection *connection, QString uri, QString query)
{
    if (query == "HOST_INFO") {
        QJsonDocument doc(info());
        auto ba = doc.toJson(QJsonDocument::Compact);
        mg_send_head(connection, 200, ba.count(), "Content-Type: application/json; charset=utf-8");
        mg_send(connection, ba.data(), ba.count());
    }
    else
    {
        // query root
        QJsonDocument doc(m_tree.query(uri));
        auto ba = doc.toJson(QJsonDocument::Compact);
        mg_send_head(connection, 200, ba.count(), "Content-Type: application/json; charset=utf-8");
        mg_send(connection, ba.data(), ba.count());
    }

    emit httpRequestReceived(uri+query);
}

void
WPN114::Network::Server::
on_udp_datagram(mg_connection *connection)
{
    QByteArray cdg(
               connection->recv_mbuf.buf,
               connection->recv_mbuf.len);

    OSCMessage msg(cdg);
    emit oscMessageReceived(msg);
}

QJsonObject const
WPN114::Network::Server::
info() const
{
    QJsonObject info  {
        { "NAME", m_name },
        { "OSC_PORT", m_udp_port },
        { "OSC_TRANSPORT", "UDP" },
        { "EXTENSIONS", ServerExtensions }
    };

    return info;
}

void
WPN114::Network::Server::
on_node_added(Node* node)
{
    if (m_connections.empty())
        return;

    QJsonObject command, data;
    command.insert("COMMAND", "PATH_ADDED");
    data.insert(node->name(), static_cast<QJsonObject>(*node));
    command.insert("DATA", data);

    for (auto& connection : m_connections)
        connection.writeJson(command);
}


void
WPN114::Network::Server::
on_node_removed(Node* node)
{
    if (m_connections.empty())
        return;

    QJsonObject command;
    command.insert("COMMAND", "PATH_REMOVED");
    command.insert("DATA", node->path());

    for (auto& connection : m_connections)
        connection.writeJson(command);
}
