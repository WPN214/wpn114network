#include "client.hpp"
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QDataStream>
#include <source/http/http.hpp>
#include <QNetworkReply>
#include <QTimer>

WPNQueryClient::WPNQueryClient() : WPNDevice(), m_direct(true),
    m_ws_con(new WPNWebSocket), m_osc_hdl(new OSCHandler), m_http_manager(nullptr)
{
    // direct client
    QObject::connect( this, &WPNQueryClient::command, this, &WPNQueryClient::onCommand );
    QObject::connect( m_ws_con, &WPNWebSocket::connected, this, &WPNQueryClient::onConnected );
    QObject::connect( m_ws_con, &WPNWebSocket::disconnected, this, &WPNQueryClient::onDisconnected );
    QObject::connect( m_ws_con, &WPNWebSocket::textMessageReceived, this, &WPNQueryClient::onTextMessageReceived );
    QObject::connect( m_ws_con, &WPNWebSocket::binaryFrameReceived, this, &WPNQueryClient::onBinaryMessageReceived );

    QObject::connect( this, SIGNAL(OSCOut(const OSCMessage&)), m_osc_hdl, SLOT(sendMessage(const OSCMessage&)));
    QObject::connect( m_osc_hdl, SIGNAL(messageReceived(QString,QVariant)), this, SLOT(onValueUpdate(QString,QVariant)));

    QObject::connect( this, &WPNQueryClient::start, m_ws_con, &WPNWebSocket::connect );
    QObject::connect( this, &WPNQueryClient::start, m_osc_hdl, &OSCHandler::listen );

    m_http_manager = new QNetworkAccessManager;

    QObject::connect( m_http_manager, SIGNAL(finished(QNetworkReply*)),
                      this, SLOT(onHttpReplyReceived(QNetworkReply*)));

}

WPNQueryClient::WPNQueryClient(WPNWebSocket* con) : m_direct(false), m_osc_hdl(new OSCHandler), m_http_manager(nullptr)
{
    // indirect client (server image)
    // no need for a local udp port
    m_ws_con = con;
    setHostAddr(m_ws_con->hostAddr());

    QObject::connect( m_ws_con, &WPNWebSocket::textMessageReceived, this, &WPNQueryClient::onTextMessageReceived );
    QObject::connect( m_ws_con, &WPNWebSocket::binaryFrameReceived, this, &WPNQueryClient::onBinaryMessageReceived );
    QObject::connect( m_ws_con, &WPNWebSocket::httpMessageReceived, this, &WPNQueryClient::httpMessageReceived );
    QObject::connect( m_ws_con, &WPNWebSocket::disconnected, this, &WPNQueryClient::disconnected );
}

WPNQueryClient::~WPNQueryClient()
{
    delete m_ws_con;
    delete m_osc_hdl;
    delete m_http_manager;
}

void WPNQueryClient::setHostAddr(QString addr)
{
    m_host_addr = addr;
    m_osc_hdl->setRemoteAddress( addr );
}

void WPNQueryClient::setPort(quint16 port)
{
    m_host_port = port;
}

void WPNQueryClient::setRemoteUdpPort(quint16 port)
{
    m_osc_hdl->setRemotePort(port);
}

void WPNQueryClient::componentComplete()
{
    WPNDevice::componentComplete();

    if      ( !m_host_addr.isEmpty()) start();
    else if ( !m_zconf_host.isEmpty() )
    {
        QObject::connect( &m_zconf, &QZeroConf::serviceAdded,
                          this, &WPNQueryClient::onZeroConfServiceAdded );

        startDiscovery();
    }
}

void WPNQueryClient::startDiscovery()
{
    m_zconf.startBrowser( "_oscjson._tcp" );
}

void WPNQueryClient::onZeroConfServiceAdded(QZeroConfService service)
{
    if ( service.name() == m_zconf_host )
    {
        setHostAddr ( service.ip().toString() );
        m_host_port = service.port();

        m_zconf.stopBrowser ( );
        connect();

        qDebug() << "[OSCQUERY-CLIENT] Target acquired: "
                 << m_host_addr << m_host_port;
    }
}

void WPNQueryClient::connect()
{
    m_ws_con->setHostAddr( m_host_addr );
    m_ws_con->setHostPort( m_host_port );

    start();
}

void WPNQueryClient::connect(QString host)
{
    auto host_address  = host.split(':');

    m_host_addr   = host_address[ 0 ];
    m_host_port   = host_address[ 1 ].toInt();

    connect();
}

void WPNQueryClient::onConnected()
{    
    m_host_url = m_host_addr;
    m_host_url.prepend("http://").append(":")
              .append(QString::number(m_host_port));

    requestHttp( "/?HOST_INFO" );
    requestHttp( "/" );

    connected( );
}

void WPNQueryClient::requestHttp(QString address)
{
    QNetworkRequest request;

    auto url = m_host_url+address;
    request.setUrl( QUrl(url) );
    m_http_manager->get(request);
}

void WPNQueryClient::onDisconnected()
{
    m_ws_con->tcpConnection()->disconnectFromHost();
    disconnected( );   

    if ( !m_zconf_host.isEmpty() )
         QTimer::singleShot( 5000, this, SLOT(startDiscovery()) );
}

void WPNQueryClient::onBinaryMessageReceived(QByteArray data)
{
    OSCMessage message = OSCHandler::decode(data);

    if ( m_direct ) onValueUpdate ( message.address, message.arguments );
    else emit valueUpdate ( message.address, message.arguments );

    qDebug() << "WebSocket-OSC In:" << message.address << message.arguments;
}

void WPNQueryClient::onTextMessageReceived(QString message)
{
    auto obj = QJsonDocument::fromJson(message.toUtf8()).object();        

    if       ( obj.contains("COMMAND")  )  emit command(obj);
    else if  ( obj.contains("FULL_PATH"))  onNamespaceReceived(obj);
    else if  ( obj.contains("OSC_PORT") )  onHostInfoReceived(obj);

    qDebug() << "Message In:" << message;
}

void WPNQueryClient::onCommand(QJsonObject command)
{
    auto type = command["COMMAND"].toString();
    auto data = command["DATA"].toObject();

    if ( type == "PATH_ADDED" )
    {
        for ( const auto& key : data.keys() )
        {
            auto obj  = data[key].toObject();
            auto node = findOrCreateNode(obj["FULL_PATH"].toString());

            node->setDevice ( this );
            node->update ( obj );
        }
    }

    else if ( type == "PATH_REMOVED" )
              onNodeRemoved(command["DATA"].toString());
}

void WPNQueryClient::onHttpReplyReceived(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    onTextMessageReceived(data);

    reply->deleteLater();
}

void WPNQueryClient::onHostInfoReceived(QJsonObject info)
{
    if ( info.contains("NAME" ) )  m_settings.name = info["NAME"].toString();
    if ( info.contains("OSC_PORT") ) m_settings.osc_port = info["OSC_PORT"].toInt();
    if ( info.contains("OSC_TRANSPORT")) m_settings.osc_transport = info["OSC_TRANSPORT"].toString();

    QJsonObject ext;
    if ( info.contains("EXTENSIONS") ) ext = info["EXTENSIONS"].toObject();
    else return;

    if ( ext.contains("ACCESS")) m_settings.extensions.access = ext["ACCESS"].toBool();
    if ( ext.contains("VALUE")) m_settings.extensions.value = ext["VALUE"].toBool();
    if ( ext.contains("RANGE")) m_settings.extensions.range = ext["RANGE"].toBool();
    if ( ext.contains("TAGS")) m_settings.extensions.tags = ext["TAGS"].toBool();
    if ( ext.contains("CLIPMODE")) m_settings.extensions.clipmode = ext["CLIPMODE"].toBool();
    if ( ext.contains("UNIT")) m_settings.extensions.unit = ext["UNIT"].toBool();
    if ( ext.contains("CRITICAL")) m_settings.extensions.critical = ext["CRITICAL"].toBool();
    if ( ext.contains("DESCRIPTION")) m_settings.extensions.description = ext["DESCRIPTION"].toBool();
    if ( ext.contains("OSC_STREAMING")) m_settings.extensions.osc_streaming = ext["OSC_STREAMING"].toBool();
    if ( ext.contains("LISTEN")) m_settings.extensions.listen = ext["LISTEN"].toBool();
    if ( ext.contains("PATH_CHANGED")) m_settings.extensions.path_changed = ext["PATH_CHANGED"].toBool();
    if ( ext.contains("PATH_RENAMED")) m_settings.extensions.path_renamed = ext["PATH_RENAMED"].toBool();
    if ( ext.contains("PATH_ADDED")) m_settings.extensions.path_added = ext["PATH_ADDED"].toBool();
    if ( ext.contains("PATH_REMOVED")) m_settings.extensions.path_removed = ext["PATH_REMOVED"].toBool();

    if ( m_settings.extensions.osc_streaming ) requestStreamStart();
}

void WPNQueryClient::requestStreamStart()
{
    QJsonObject command, data;

    command.insert      ( "COMMAND", "START_OSC_STREAMING" );
    data.insert         ( "LOCAL_SERVER_PORT", m_osc_hdl->localPort() );
    data.insert         ( "LOCAL_SENDER_PORT", m_osc_hdl->remotePort() );
    command.insert      ( "DATA", data );   

    m_ws_con->writeText  ( QJsonDocument(command).toJson(QJsonDocument::Compact) );
}

void WPNQueryClient::onNamespaceReceived(QJsonObject nspace)
{
    QJsonObject contents = nspace["CONTENTS"].toObject();

    for ( const auto& key : contents.keys() )
    {
        QJsonObject jsnode = contents[key].toObject();

        auto node = findOrCreateNode(jsnode["FULL_PATH"].toString());
        node->setDevice ( this );
        node->update ( jsnode );
    }

    emit treeComplete();
}

void WPNQueryClient::sendMessage(QString address, QVariant arguments, bool critical)
{
    OSCMessage message { address, arguments };

    if ( critical )
    {
        auto encoded = OSCHandler::encode(message);
        m_ws_con->writeBinary(encoded);
    }

    else emit OSCOut( message );
}

void WPNQueryClient::writeOsc(QString method, QVariant arguments) { }
void WPNQueryClient::writeWebSocket(QString method, QVariant arguments) { }
void WPNQueryClient::writeWebSocket(QString message)
{
    m_ws_con->writeText(message);
}

void WPNQueryClient::writeWebSocket(QJsonObject json)
{
    auto doc = QJsonDocument(json).toJson(QJsonDocument::Compact);
    m_ws_con->writeText(doc);
}

void WPNQueryClient::pushNodeValue(WPNNode* node)
{    
    OSCMessage message { node->path(), node->value() };

    if ( node->critical() ) m_ws_con->writeBinary(OSCHandler::encode(message));
    else m_osc_hdl->sendMessage(message);
}

QTcpSocket* WPNQueryClient::tcpConnection()
{
    return m_ws_con->tcpConnection();
}

void WPNQueryClient::listen(QString method)
{
    QJsonObject command;
    command.insert("COMMAND", "LISTEN");
    command.insert("DATA", method );

    writeWebSocket( command );
}

void WPNQueryClient::listenAll(QString method)
{
    QJsonObject command;
    command.insert("COMMAND", "LISTEN_ALL");
    command.insert("DATA", method );

    writeWebSocket( command );
}

void WPNQueryClient::ignore(QString method)
{
    QJsonObject command;
    command.insert("COMMAND", "IGNORE");
    command.insert("DATA", method);

    writeWebSocket(command);
}

void WPNQueryClient::ignoreAll(QString method)
{
    QJsonObject command;
    command.insert("COMMAND", "IGNORE_ALL");
    command.insert("DATA", method );

    writeWebSocket( command );
}

