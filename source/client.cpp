#include "client.hpp"
#include "osc.hpp"
#include <QJsonDocument>

using namespace WPN114::Network;

WPN114::Network::Client::
Client()
{
    mg_mgr_init(&m_mgr, this);
    mg_bind(&m_mgr, "udp://1234", event_handler);
}

WPN114::Network::
Client::~Client()
{
    stop();
    mg_mgr_free(&m_mgr);
}

void
WPN114::Network::Client::
stop()
{
    m_running = false;
    m_thread.join();
}

void
WPN114::Network::Client::
componentComplete()
{
    if (m_host.startsWith("zc://")) {
        QObject::connect(
            &m_zeroconf, &QZeroConf::serviceAdded,
            this, &Client::on_zeroconf_service_added);
        m_zeroconf.startBrowser("_oscjson._tcp");
    } else {
        connect();
    }
}

void
WPN114::Network::Client::
on_zeroconf_service_added(QZeroConfService service)
{
    auto host = m_host;
    host.remove("zc://");

    if (service->name() == host) {
        m_host = service->ip().toString();
        m_port = service->port();
        connect();
        m_zeroconf.stopBrowser();
    }
}

void
WPN114::Network::Client::
connect()
{
    if (m_host.startsWith("ws://")) {
        if (m_host.contains(":")) {
            m_port = std::stoi(m_host.split("/").last().toStdString());
            m_connection = mg_connect_ws(&m_mgr, event_handler, CSTR(m_host), nullptr, nullptr);
        } else {
            QString addr(m_host);
            addr.append(":");
            addr.append(QString::number(m_port));
            m_connection = mg_connect_ws(&m_mgr, event_handler, CSTR(addr), nullptr, nullptr);
        }
    } else {
        QString addr("ws://");
        addr.append(m_host);

        if (!addr.contains(":")) {
            addr.append(":");
            addr.append(QString::number(m_port));
        }

        m_connection = mg_connect_ws(&m_mgr, event_handler, CSTR(addr), nullptr, nullptr);
    }

    m_running = true;
    m_thread = std::thread(&Client::poll, this);
}

void
WPN114::Network::Client::
request(QString req)
{
    QString addr(m_host);
    addr.append(":");
    addr.append(QString::number(m_port));
    addr.append(req);

    auto mgc = mg_connect_http(&m_mgr, event_handler, addr.toUtf8().data(), nullptr, nullptr);
}

void
WPN114::Network::Client::
listen(QString uri)
{
    QJsonObject command;
    command["COMMAND"] = "LISTEN";
    command["DATA"] = uri;
    m_connection.writeJson(command);
}

void
WPN114::Network::Client::
ignore(QString uri)
{
    QJsonObject command;
    command["COMMAND"] = "IGNORE";
    command["DATA"] = uri;
    m_connection.writeJson(command);
}

void
WPN114::Network::Client::
send(QString uri, QVariant arguments, bool critical)
{
    OSCMessage message(uri, arguments);
    m_connection.writeOSC(uri, arguments.toList(), critical);
}

void
WPN114::Network::Client::
poll()
{
    while (m_running)
           mg_mgr_poll(&m_mgr, 200);
}

void
WPN114::Network::Client::
on_connected()
{
    request("/?HOST_INFO");
    request("/");
}

void
WPN114::Network::Client::
parse_json(const QByteArray &frame)
{
    auto object = QJsonDocument::fromJson(frame).object();

    if (object.contains("COMMAND"))
    {
        auto type = object["COMMAND"].toString();
        auto data = object["DATA"].toObject();

        if (type == "PATH_ADDED") {
            for (auto& key : data.keys()) {
                auto objn = data[key].toObject();
                auto node = m_tree.find_or_create(objn["FULL_PATH"].toString());
                node->update(objn);
            }
        }

        else if (type == "PATH_REMOVED") {

        }
    }

    else if (object.contains("FULL_PATH"))
    {

    }

    else if (object.contains("OSC_PORT"))
    {
        m_connection.set_udp(object["OSC_PORT"].toInt());

        QJsonObject command, data;

        command.insert  ("COMMAND", "START_OSC_STREAMING");
        data.insert     ("LOCAL_SERVER_PORT", 1234);
        data.insert     ("LOCAL_SENDER_PORT", 0);
        command.insert  ("DATA", data);

        m_connection.writeJson(command);
        emit connected();
    }
}

void
WPN114::Network::Client::
parse_osc(const QByteArray &data)
{
    OSCMessage msg(data);
    if (auto node = m_tree.find(msg.m_method))
        node->set_value(msg.m_arguments);
}

void
WPN114::Network::Client::
on_http_reply(http_message *reply)
{
    QByteArray body(reply->body.p, reply->body.len);
    parse_json(body);
}

void
WPN114::Network::Client::
on_websocket_frame(websocket_message *message)
{
    QByteArray frame(reinterpret_cast<const char*>(message->data), message->size);

    if (message->flags & WEBSOCKET_OP_TEXT)
        parse_json(frame);

    else if (message->flags & WEBSOCKET_OP_BINARY)
        parse_osc(frame);
}

void
WPN114::Network::Client::
on_udp_datagram(mg_connection *connection)
{
    QByteArray cdg(connection->recv_mbuf.buf,
                   connection->recv_mbuf.len);
    parse_osc(cdg);
}

void
WPN114::Network::Client::
event_handler(mg_connection *mgc, int event, void *data)
{
    auto client = static_cast<Client*>(mgc->mgr->user_data);

    switch(event)
    {
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
        QMetaObject::invokeMethod(client, "on_connected", Qt::QueuedConnection);
        break;
    case MG_EV_WEBSOCKET_FRAME:
    {
        auto wm = static_cast<websocket_message*>(data);
        QMetaObject::invokeMethod(client, "on_websocket_frame",
            Qt::QueuedConnection,
            Q_ARG(websocket_message*, wm));
        break;
    }
    case MG_EV_HTTP_REPLY:
    {
        http_message* reply = static_cast<http_message*>(data);
        mgc->flags != MG_F_CLOSE_IMMEDIATELY;

        QMetaObject::invokeMethod(client, "on_http_reply",
            Qt::QueuedConnection,
            Q_ARG(http_message*, reply));
        break;
    }
    }
}
