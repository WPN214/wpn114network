#include "websocket.hpp"
#include <QDataStream>
#include <QRandomGenerator>
#include <QCryptographicHash>

// SERVER ----------------------------------------------------------------------------------------------

WPNWebSocketServer::WPNWebSocketServer(quint16 port) : m_port(port) { }

WPNWebSocketServer::~WPNWebSocketServer()
{
    delete m_tcp_server;
}

void WPNWebSocketServer::listen()
{
    m_tcp_server = new QTcpServer;
    QObject::connect( m_tcp_server, &QTcpServer::newConnection,
                      this, &WPNWebSocketServer::onNewConnection );

    m_tcp_server->listen( QHostAddress::Any, m_port );
}

void WPNWebSocketServer::stop()
{
    m_tcp_server->close();
}

void WPNWebSocketServer::onNewConnection()
{
    // catching new tcp connection
    while ( m_tcp_server->hasPendingConnections() )
    {
        QTcpSocket* con = m_tcp_server->nextPendingConnection();
        QObject::connect(con, SIGNAL(readyRead()), this, SLOT(onTcpReadyRead()));
    }
}

void WPNWebSocketServer::onTcpReadyRead()
{
     QTcpSocket* sender = qobject_cast<QTcpSocket*>(QObject::sender());

     while ( sender->bytesAvailable())
     {
         QByteArray data = sender->readAll ( );

         if ( data.contains("Sec-WebSocket-Key"))
         {
             onHandshakeRequest(sender, data);
             // once handshake has been done, no need to keep it
             QObject::disconnect(sender, SIGNAL(readyRead()), this, SLOT(onTcpReadyRead()));
         }

         else if ( data.contains("HTTP") )
             emit httpRequestReceived(sender, data);
     }
}

void WPNWebSocketServer::onHandshakeRequest(QTcpSocket *sender, QByteArray data)
{
    // parse the key
    auto contents = data.split('\n');

    for ( const auto& line : contents )
    {
        if ( line.startsWith ( "Sec-WebSocket-Key" ))
        {
            auto key_contents = line.split(' ');
            QString key = key_contents[ 1 ];
            key.remove( "\r" );

            sendHandshakeResponse(sender, key);
        }
    }
}

void WPNWebSocketServer::sendHandshakeResponse ( QTcpSocket* target, QString key )
{
    // concat key with 258EAFA5-E914-47DA-95CA-C5AB0DC85B11
    // take the SHA-1 of the result
    // return the base64 encoding of the hash

    key.append      ("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    auto hash       = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1);
    auto b64        = hash.toBase64();

    QString resp    ( "HTTP/1.1 101 Switching Protocols\r\n" );
    resp.append     ( "Upgrade: websocket\r\n" );
    resp.append     ( "Connection: Upgrade\r\n" );
    resp.append     ( "Sec-WebSocket-Accept: " );
    resp.append     ( b64 );
    resp.append     ( "\r\n");
    resp.append     ( "\r\n");

    // write accept response
    target->write ( resp.toUtf8() );

    // upgrade connection to websocket
    WPNWebSocket* client = new WPNWebSocket(target);
    emit newConnection(client);
}

// CLIENT ----------------------------------------------------------------------------------------------

WPNWebSocket::WPNWebSocket() : m_seed(0), m_connected(false), m_host_port(0), m_mask(true),
    m_tcp_con( new QTcpSocket )
{
    // direct client case

    QObject::connect( m_tcp_con, &QTcpSocket::connected, this, &WPNWebSocket::onConnected );
    QObject::connect( m_tcp_con, &QTcpSocket::disconnected, this, &WPNWebSocket::onDisconnected );
    QObject::connect( m_tcp_con, SIGNAL(error(QAbstractSocket::SocketError)),
                      this, SLOT(onError(QAbstractSocket::SocketError )));

    QObject::connect( m_tcp_con, &QTcpSocket::readyRead, this, &WPNWebSocket::onRawMessageReceived );
    QObject::connect( m_tcp_con, &QTcpSocket::bytesWritten, this, &WPNWebSocket::onBytesWritten );
}

WPNWebSocket::WPNWebSocket(QTcpSocket* con) : m_tcp_con(con), m_mask(false), m_seed(0), m_connected(true)
{
    // server catching a client
    // the proxy is already connected, so nothing to do here, except chain signals
    m_host_addr = m_tcp_con->peerAddress().toString();
    m_host_addr.remove("::ffff:");

    QObject::connect( m_tcp_con, &QTcpSocket::disconnected, this, &WPNWebSocket::disconnected );
    QObject::connect( m_tcp_con, &QTcpSocket::readyRead, this, &WPNWebSocket::onRawMessageReceived );
}

WPNWebSocket::~WPNWebSocket()
{
    delete m_tcp_con;
}

void WPNWebSocket::setHostAddr(QString addr)
{
    m_host_addr = addr;
}

void WPNWebSocket::setHostPort(quint16 port)
{
    m_host_port = port;
}

void WPNWebSocket::connect()
{
    m_tcp_con->connectToHost( m_host_addr, m_host_port );
}

void WPNWebSocket::disconnect()
{
    m_tcp_con->close();
}

void WPNWebSocket::onConnected()
{
    // tcp-connection established,
    // send http 'handshake' upgrade request to server
    generateEncryptedSecKey();
    requestHandshake();
}

void WPNWebSocket::onDisconnected()
{
    m_tcp_con->disconnectFromHost();
    emit disconnected();
}

void WPNWebSocket::onError(QAbstractSocket::SocketError err)
{
    qDebug() << m_tcp_con->errorString();
    m_tcp_con->disconnectFromHost();
}

void WPNWebSocket::onRawMessageReceived()
{
    QTcpSocket* sender = qobject_cast<QTcpSocket*>(QObject::sender());

    while ( sender->bytesAvailable())
    {
        QByteArray data = sender->readAll();

        if ( data.contains("HTTP") )
        {
             if ( data.contains("Sec-WebSocket-Accept"))
                 onHandshakeResponseReceived(data);
             else emit httpMessageReceived(data);
        }
        else decode(data);
    }
}

void WPNWebSocket::onHandshakeResponseReceived(QString response)
{
    // parse the key
    QString key;

    auto contents = response.split( '\n' );
    for ( const auto& line : contents )
    {
        if ( line.startsWith("Sec-WebSocket-Accept"))
        {
            auto accept_contents = line.split(' ');
            key = accept_contents [ 1 ];
            key.remove( '\r' );
        }
    }

    if ( key != m_accept_key )
    {
        qDebug() << "[WEBSOCKET] Error: Sec-WebSocket-Accept key is incorrect.";
        return;
    }

    m_connected = true;
    connected( );
}

void WPNWebSocket::generateEncryptedSecKey()
{
    QRandomGenerator kgen;
    QByteArray res;

    for ( quint8 i = 0; i < 25; ++i )
    {
        quint8 rdm = kgen.generate64();
        res.append(rdm);
    }

    m_sec_key         = res.toBase64();
    QByteArray key    = m_sec_key;
    key.append        ( "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" );
    auto hash         = QCryptographicHash::hash(key, QCryptographicHash::Sha1);
    m_accept_key      = hash.toBase64();
}

void WPNWebSocket::requestHandshake()
{
    QByteArray request;

    QString host      = m_host_addr;
    QString sec_key   = m_sec_key;

    request.append  ( "GET / HTTP/1.1\r\n" );
    request.append  ( "Connection: Upgrade\r\n" );
    request.append  ( host.remove( "ws://" ).append("\r\n").prepend("Host: "));
    request.append  ( "Sec-WebSocket-Key: " );
    request.append  ( sec_key.append("\r\n") );
    request.append  ( "Sec-WebSocket-Version: 13\r\n" );
    request.append  ( "Upgrade: websocket\r\n" );
    request.append  ( "User-Agent: Qt/5.1.1\r\n\r\n" );

    m_tcp_con->write( request );
}

void WPNWebSocket::request(QString request)
{
    m_tcp_con->write( request.toUtf8() );
}

void WPNWebSocket::onBytesWritten(qint64 nbytes)
{
//    qDebug() << nbytes << "bytes written";
}

void WPNWebSocket::write(QByteArray message, Opcodes op)
{
    QByteArray data;
    quint8 mask[4], size_mask = m_mask*0x80;
    quint64 sz = message.size();

    QDataStream stream ( &data, QIODevice::WriteOnly );
    stream << (quint8) ( 0x80 + static_cast<quint8>(op)) ;

    // we have to set the mask bit here
    if ( sz > 65535 )
    {
        stream << (quint8) ( 0x7F + size_mask );
        stream << sz;
    }
    else if ( sz > 0x7D )
    {
        stream << (quint8) ( 0x7E + size_mask );
        stream << (quint16) sz;
    }
    else stream << (quint8) (sz + size_mask);

    if ( m_mask )
    {
        for ( quint8 i = 0; i < 4; ++i )
        {
            QRandomGenerator rdm( m_seed );
            mask[i] = rdm.bounded( 256 );
            stream << mask[i];
            ++m_seed;
        }

        // ...and encode the message with it
        for ( quint8 i = 0; i < sz; ++i )
              stream << (quint8) ( message[ i ] ^ mask[ i%4 ]);
    }

    else data.append(message);

    m_tcp_con->flush();
    m_tcp_con->write(data);
}

void WPNWebSocket::writeBinary(QByteArray binary)
{
    write( binary, Opcodes::BINARY_FRAME );
}

void WPNWebSocket::writeText(QString message)
{
    write( message.toUtf8(), Opcodes::TEXT_FRAME );
}

void WPNWebSocket::decode(QByteArray data)
{
    QDataStream stream(data);
    QByteArray decoded;

    quint8 fbyte, fin, opcode, sbyte, maskbit, mask[ 4 ];
    quint64 payle;

    stream >> fbyte >> sbyte;

    if      ( fbyte-0x80 > 0 ) { fin = true; opcode = fbyte-0x80; }
    else    { fin = false; opcode = fbyte; }

    if ( sbyte-0x80 <= 0 )
    {
        maskbit = 0;

        if      ( sbyte <= 0x7D ) payle = sbyte;
        else if ( sbyte == 0x7F ) stream >> payle;
        else if ( sbyte == 0x7E )
        {
            quint16 payle16;
            stream >> payle16;
            payle = payle16;
        }

        for ( quint64 i = 0; i < payle; ++i )
        {
            quint8 byte;
            stream >> byte;
            decoded.append( byte );
        }
    }
    else
    {
        maskbit  = 1;
        sbyte   -= 0x80;

        if      ( sbyte <= 0x7D ) payle = sbyte;
        else if ( sbyte == 0x7F ) stream >> payle;
        else if ( sbyte == 0x7E )
        {
            quint16 payle16;
            stream >> payle16;
            payle = payle16;
        }

        for ( quint8  i = 0; i < 4; ++i ) stream >> mask[i];
        for ( quint64 i = 0; i < payle; ++i )
        {
            quint8 byte;
            stream >> byte;
            decoded.append ( byte^mask[ i%4 ] );
        }
    }    

    switch(static_cast<Opcodes>(opcode))
    {
    case Opcodes::BINARY_FRAME: emit binaryFrameReceived(decoded); break;
    case Opcodes::TEXT_FRAME:   emit textMessageReceived(decoded); break;
    }

    // check if another message is not hidden behind this one
    if ( !stream.atEnd() )
    {
        QByteArray next = data.remove(0, stream.device()->pos());
        decode( next );
    }
}

