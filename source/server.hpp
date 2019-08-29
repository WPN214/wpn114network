#pragma once

#include "network.hpp"
#include "osc.hpp"
#include <thread>

namespace WPN114   {
namespace Network  {

//-------------------------------------------------------------------------------------------------
static QJsonObject
ServerExtensions =
//-------------------------------------------------------------------------------------------------
{
    { "ACCESS", false },
    { "VALUE", true },
    { "RANGE", false },
    { "DESCRIPTION", false },
    { "TAGS", false },
    { "EXTENDED_TYPE", true },
    { "UNIT", false },
    { "CRITICAL", true },
    { "CLIPMODE", false },
    { "LISTEN", true },
    { "PATH_CHANGED", false },
    { "PATH_REMOVED", true },
    { "PATH_ADDED", true },
    { "PATH_RENAMED", false },
    { "OSC_STREAMING", true },
    { "HTML", false },
    { "ECHO", false }
};

//=================================================================================================
class Server : public NetworkDevice
//=================================================================================================
{
    Q_OBJECT

    Q_PROPERTY (int tcp READ tcp WRITE set_tcp)
    Q_PROPERTY (int udp READ udp WRITE set_udp)
    Q_PROPERTY (QString name READ name WRITE set_name) // zeroconf
    Q_PROPERTY (bool singleton READ singleton WRITE set_singleton)

public:

    //-------------------------------------------------------------------------------------------------
    Q_SIGNAL void
    connection(Connection connection);

    Q_SIGNAL void
    disconnection(Connection connection);

    Q_SIGNAL void
    oscMessageReceived(OSCMessage message);

    Q_SIGNAL void
    httpRequestReceived(QString request);

    Q_SIGNAL void
    websocketMessageReceived(QString message);

    //-------------------------------------------------------------------------------------------------
    Server();

    //-------------------------------------------------------------------------------------------------
    void
    componentComplete() override;

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    stop();

    //-------------------------------------------------------------------------------------------------
    virtual
    ~Server() override;

    //-------------------------------------------------------------------------------------------------
    void
    poll();

    //-------------------------------------------------------------------------------------------------
    void
    server_poll();

    //-------------------------------------------------------------------------------------------------
    static void
    ws_event_handler(mg_connection* mgc, int event, void* data);

    static void
    udp_event_handler(mg_connection* mgc, int event, void* data);

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    on_connection(mg_connection* con);
    // this method dwells in the qt thread, it has to be invoked with a queued connection
    // from the mgr callback

    Q_INVOKABLE void
    on_disconnection(mg_connection* connection);

    //-------------------------------------------------------------------------------------------------

    Q_INVOKABLE void
    on_http_request(mg_connection* connection, QString uri, QString query);

    Q_INVOKABLE void
    on_websocket_frame(mg_connection* mgc, websocket_message* message);

    Q_INVOKABLE void
    on_udp_datagram(mg_connection* connection);

    //-------------------------------------------------------------------------------------------------
    QJsonObject const
    info() const;

    //-------------------------------------------------------------------------------------------------
    Q_SLOT void
    on_node_added(Node* node);

    Q_SLOT void
    on_node_removed(Node* node);

    //-------------------------------------------------------------------------------------------------
    bool
    running() const { return m_running; }

    //-------------------------------------------------------------------------------------------------
    bool
    singleton() const { return m_tree.singleton(); }

    //-------------------------------------------------------------------------------------------------
    QString
    name() const { return m_name; }

    //-------------------------------------------------------------------------------------------------
    uint16_t
    tcp() const { return m_tcp_port; }

    uint16_t
    udp() const { return m_udp_port; }

    //-------------------------------------------------------------------------------------------------
    void
    set_singleton(bool singleton) { m_tree.set_singleton(singleton); }

    //-------------------------------------------------------------------------------------------------
    void
    set_tcp(uint16_t port)
    {
        m_tcp_port = port;
    }

    //-------------------------------------------------------------------------------------------------
    void
    set_udp(uint16_t port)
    {
        m_udp_port = port;
    }

    //-------------------------------------------------------------------------------------------------
    void
    set_name(QString name)
    {
        m_name = name;
    }

private:

    std::vector<Connection>
    m_connections;

    mg_connection
    *m_tcp_connection = nullptr,
    *m_udp_connection = nullptr;

    mg_mgr
    m_tcp,
    m_udp;

    uint16_t
    m_tcp_port = 5678,
    m_udp_port = 1234;

    std::thread
    m_mgthread;

    bool
    m_running = false;

    QString
    m_name = "wpn114";
};

}
}
