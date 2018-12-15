#pragma once

#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include <QQmlParserStatus>
#include "client.hpp"
#include "../websocket/websocket.hpp"
#include <source/http/http.hpp>
#include <qzeroconf.h>

class WPNQueryServer : public WPNDevice
{
    Q_OBJECT

    Q_PROPERTY ( int tcpPort READ tcpPort WRITE setTcpPort )
    Q_PROPERTY ( int udpPort READ udpPort WRITE setUdpPort )
    Q_PROPERTY ( QString name READ name WRITE setName )

    public:
    WPNQueryServer();
    ~WPNQueryServer();

    virtual void componentComplete  ( );
    virtual void classBegin         ( ) {}
    virtual void pushNodeValue      ( WPNNode* node );

    quint16 tcpPort() const { return m_settings.tcp_port; }
    quint16 udpPort() const { return m_settings.osc_port; }
    QString name() const { return m_settings.name; }

    void setTcpPort ( quint16 port );
    void setUdpPort ( quint16 port );
    void setName    ( QString name ) { m_settings.name = name; }

    signals:
    void startNetwork();
    void stopNetwork();

    void newConnection(QString host);
    void disconnection(QString host);
    void unknownMethodRequested ( QString method );

    protected slots:
    void onClientHttpQuery      ( QString query );
    void onCommand              ( QJsonObject command_obj );
    void onNewConnection        ( WPNWebSocket* client );
    void onDisconnection        ( );
    void onHttpRequestReceived  ( QTcpSocket* sender, QString req );
    void onNodeAdded            ( WPNNode *node );
    void onQueryNodeRemoved     ( QString path );
    QString hostInfoJson        ( );
    QString namespaceJson       ( QString method );
    void onZConfError           ( QZeroConf::error_t err);

    private:        
    QThread m_osc_thread;
    QThread m_ws_thread;

    OSCHandler* m_osc_hdl;
    HostSettings m_settings;
    WPNWebSocketServer* m_ws_server;
    QVector<WPNQueryClient*> m_clients;
    QZeroConf m_zeroconf;
    HTTP::ReplyManager m_reply_manager;
};
