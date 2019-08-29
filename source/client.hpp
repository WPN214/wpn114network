#pragma once

#include "network.hpp"
#include <thread>

namespace WPN114   {
namespace Network  {

//=================================================================================================
class Client : public NetworkDevice
//=================================================================================================
{
    Q_OBJECT

    Q_PROPERTY   (QString host READ host WRITE set_host)
    Q_PROPERTY   (int port READ port WRITE set_port)
    Q_INTERFACES (QQmlParserStatus)

public:

    Q_SIGNAL void connected();
    Q_SIGNAL void disconnected();

    //-------------------------------------------------------------------------------------------------
    Client();

    //-------------------------------------------------------------------------------------------------
    virtual
    ~Client() override;

    //-------------------------------------------------------------------------------------------------
    void
    stop();

    //-------------------------------------------------------------------------------------------------
    QString
    host() const { return m_host; }

    uint16_t
    port() const { return m_port; }

    //-------------------------------------------------------------------------------------------------
    void
    set_host(QString host)
    //-------------------------------------------------------------------------------------------------
    {
        m_host = host;
    }

    //-------------------------------------------------------------------------------------------------
    void
    set_port(uint16_t port)
    //-------------------------------------------------------------------------------------------------
    {
        m_port = port;
    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override;

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    connect();

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    request(QString req);

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    listen(QString uri);

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    ignore(QString uri);

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    send(QString uri, QVariant arguments, bool critical = true);

private:

    void
    on_zeroconf_service_added(QZeroConfService service);

    //-------------------------------------------------------------------------------------------------
    void
    poll();

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    on_connected();
    // send host info request as well as namespace query

    //-------------------------------------------------------------------------------------------------
    void
    parse_json(QByteArray const& frame);

    void
    parse_osc(QByteArray const& data);

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    on_http_reply(http_message* reply);

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    on_websocket_frame(websocket_message* message);
    // for commands/osc messages

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    on_udp_datagram(mg_connection* connection);

    //-------------------------------------------------------------------------------------------------
    static void
    event_handler(mg_connection* mgc, int event, void* data);

    //-------------------------------------------------------------------------------------------------
    Connection
    m_connection;

    std::thread
    m_thread;

    mg_mgr
    m_mgr;

    QString
    m_host;

    uint16_t
    m_port = 0;

    bool
    m_running = false;
};

}
}
