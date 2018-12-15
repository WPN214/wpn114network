#include "qserver.hpp"
#include <QJsonObject>
#include <QJsonDocument>
#include <source/http/http.hpp>
#include "file.hpp"

WPNQueryServer::WPNQueryServer() : WPNDevice (),
    m_ws_server(new WPNWebSocketServer(5678)),
    m_osc_hdl(new OSCHandler)
{
    // default settings & extensions
    m_settings.name             = "WPN-SERVER";
    m_settings.tcp_port         = 5678;
    m_settings.osc_port         = 1234;
    m_settings.osc_transport    = "UDP";

    m_settings.extensions.osc_streaming     = true;
    m_settings.extensions.access            = true;
    m_settings.extensions.clipmode          = true;
    m_settings.extensions.critical          = true;
    m_settings.extensions.description       = true;
    m_settings.extensions.extended_type     = true;
    m_settings.extensions.listen            = true;
    m_settings.extensions.path_added        = true;
    m_settings.extensions.path_changed      = false;
    m_settings.extensions.path_removed      = true;
    m_settings.extensions.path_renamed      = false;
    m_settings.extensions.range             = false;
    m_settings.extensions.tags              = true;
    m_settings.extensions.unit              = false;
    m_settings.extensions.value             = true;
    m_settings.extensions.echo              = false;
    m_settings.extensions.html              = false;
}

WPNQueryServer::~WPNQueryServer()
{
    m_zeroconf.stopServicePublish();

    delete m_ws_server;
    delete m_osc_hdl;

    for ( const auto& client : m_clients )
          delete client;
}

void WPNQueryServer::componentComplete()
{    
    WPNDevice::componentComplete();

    QObject::connect( this, &WPNQueryServer::startNetwork, m_ws_server, &WPNWebSocketServer::listen );
    QObject::connect( this, &WPNQueryServer::startNetwork, m_osc_hdl, &OSCHandler::listen);
    QObject::connect( this, &WPNQueryServer::stopNetwork, m_ws_server, &WPNWebSocketServer::stop);
    QObject::connect( this, &WPNDevice::nodeAdded, this, &WPNQueryServer::onNodeAdded );
    QObject::connect( this, &WPNDevice::nodeRemoved, this, &WPNQueryServer::onQueryNodeRemoved );

    QObject::connect( m_ws_server, &WPNWebSocketServer::newConnection, this, &WPNQueryServer::onNewConnection );
    QObject::connect( m_ws_server, &WPNWebSocketServer::httpRequestReceived, this, &WPNQueryServer::onHttpRequestReceived );
    QObject::connect( &m_zeroconf, &QZeroConf::error, this, &WPNQueryServer::onZConfError);    
    QObject::connect( m_osc_hdl, &OSCHandler::messageReceived, this, &WPNDevice::onValueUpdate );

    startNetwork( );
    m_zeroconf.startServicePublish( m_settings.name.toStdString().c_str(), "_oscjson._tcp", "local", m_settings.tcp_port );
}

void WPNQueryServer::onZConfError(QZeroConf::error_t err)
{
    qDebug() << "[ZEROCONF-SERVER] Error" << err;
}

void WPNQueryServer::setTcpPort(quint16 port)
{
    m_settings.tcp_port   = port;
    m_ws_server->setPort  ( port );
}

void WPNQueryServer::setUdpPort(quint16 port)
{
    m_settings.osc_port     = port;
    m_osc_hdl->setLocalPort ( port );
}

void WPNQueryServer::onNewConnection(WPNWebSocket* con)
{
    auto client = new WPNQueryClient(con);
    m_clients.push_back(client);   

    QObject::connect( client, &WPNQueryClient::command, this, &WPNQueryServer::onCommand);
    QObject::connect( client, &WPNQueryClient::disconnected, this, &WPNQueryServer::onDisconnection);
    QObject::connect( client, &WPNQueryClient::httpMessageReceived, this, &WPNQueryServer::onClientHttpQuery);
    QObject::connect( client, SIGNAL(valueUpdate(QString, QVariant)), this, SLOT(onValueUpdate(QString, QVariant)));

    QString host = client->hostAddr().append(":")
            .append( QString::number(client->port()) );

   emit newConnection( host );
}

void WPNQueryServer::onDisconnection()
{
    auto sender = qobject_cast<WPNQueryClient*>(QObject::sender());    

    QString host = sender->hostAddr()
            .append( ":" )
            .append( QString::number(sender->port()) );

    m_clients.removeAll(sender);
    disconnection( host );
}

void WPNQueryServer::onClientHttpQuery(QString query)
{
    WPNQueryClient* sender = qobject_cast<WPNQueryClient*>(QObject::sender());
    onHttpRequestReceived(sender->tcpConnection(), query);
}

void WPNQueryServer::onHttpRequestReceived(QTcpSocket* sender, QString data)
{
    auto requests = data.split( "GET", QString::SkipEmptyParts );

    for ( const auto& request : requests )
    {
        if ( request.contains("HOST_INFO") )
        {
            HTTP::Reply response { sender, hostInfoJson().toUtf8() };
            m_reply_manager.enqueue(response);
        }
        else // request is namespace
        {
            HTTP::Reply response;
            response.target  = sender;

            auto namespace_path = request.split(' ', QString::SkipEmptyParts )[0];
            response.reply = namespaceJson(namespace_path).toUtf8();
            m_reply_manager.enqueue(response);
        }
    }
}

QString WPNQueryServer::hostInfoJson()
{
    return HTTP::ReplyManager::formatJsonResponse(m_settings.toJson());
}

QString WPNQueryServer::namespaceJson(QString method)
{
//    qDebug() << "[OSCQUERY-SERVER] NAMESPACE requested for method:" << method ;

    if ( method.contains("?") )
    {
        auto target_address    = method.split( '?' );
        auto target_node       = m_root_node->subnode(target_address[ 0 ]);
        auto target_attribute  = target_address[ 1 ];

        if ( !target_node )     return "";

        auto obj = target_node->attributeJson(target_attribute);

        // workaround for http value requests don't seem to work with score for some reason
        if ( target_attribute == "VALUE" ) pushNodeValue ( target_node );
        return HTTP::ReplyManager::formatJsonResponse(obj);
    }
    else
    {
        auto target_node  = m_root_node->subnode(method);

        if ( !target_node )
        {
            emit unknownMethodRequested( method );
            return ""; // TODO: 404 reply
        }

        if ( auto file = dynamic_cast<WPNFileNode*>(target_node) )
        {
            // if node is a file
            // reply with the contents of the file
            if  ( file->path().endsWith(".png") )
                  return HTTP::ReplyManager::formatFileResponse(file->data(), "image/png");

            else if ( file->path().endsWith(".qml"))
                return HTTP::ReplyManager::formatFileResponse(file->data(), "text/plain");
        }

        return HTTP::ReplyManager::formatJsonResponse(target_node->toJson());
    }
}

void WPNQueryServer::onCommand(QJsonObject command_obj)
{
    WPNQueryClient* listener = qobject_cast<WPNQueryClient*>(QObject::sender());
    QString command = command_obj["COMMAND"].toString();

    if ( command == "LISTEN" || command == "IGNORE" )
    {
        QString method = command_obj["DATA"].toString();

        if ( auto node = m_root_node->subnode(method) )
             node->setListening(command == "LISTEN", listener);

        else qDebug() << "[OSCQUERY-SERVER] LISTEN/IGNORE command ignored"
                      << "cannot find node:" << method;
    }

    else if ( command == "LISTEN_ALL" || command == "IGNORE_ALL" )
    {
        QString method = command_obj["DATA"].toString();

        if ( auto node = m_root_node->subnode(method) )
             node->setListening(command == "LISTEN_ALL", listener, true);

        else qDebug() << "[OSCQUERY-SERVER] LISTEN/IGNORE command ignored"
                      << "cannot find node:" << method;
    }

    else if ( command == "START_OSC_STREAMING" )
    {
        quint16 port = command_obj["DATA"].toObject()["LOCAL_SERVER_PORT"].toInt();
        listener->setRemoteUdpPort(port);
    }
}

void WPNQueryServer::onNodeAdded(WPNNode* node)
{
    if ( !m_clients.isEmpty() )
    {
        QJsonObject command, data;

        command.insert   ( "COMMAND", "PATH_ADDED" );
        data.insert      ( node->name(), node->toJson() );
        command.insert   ( "DATA", data );

        for ( const auto& client : m_clients )
              client->writeWebSocket( command );
    }
}

void WPNQueryServer::onQueryNodeRemoved(QString path)
{
    if ( !m_clients.isEmpty() )
    {
        QJsonObject command;

        command.insert  ( "COMMAND", "PATH_REMOVED" );
        command.insert  ( "DATA", path );

        for ( const auto& client : m_clients )
              client->writeWebSocket( command );
    }
}

void WPNQueryServer::pushNodeValue(WPNNode* node)
{
    for ( const auto& client : m_clients )
          client->pushNodeValue(node);
}
