#pragma once

#include <mongoose.h>
#include <vector>
#include <pthread.h>
#include <QObject>
#include <QQmlParserStatus>
#include <QQmlPropertyValueSource>
#include <QVariant>
#include <QVector>
#include <QDataStream>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include <QtDebug>

//---------------------------------------------------------------------------------------------
template<typename _Valuetype> void
from_stream(QVariantList& arglst, QDataStream& stream);

//---------------------------------------------------------------------------------------------
template<> void
from_stream<QString>(QVariantList& arglist, QDataStream& stream);

//-------------------------------------------------------------------------------------------------
struct OSCMessage
//-------------------------------------------------------------------------------------------------
{
    QString method;
    QVariant arguments;

    OSCMessage() {}

    OSCMessage(QString mthd, QVariant args) :
        method(mthd), arguments(args) {}

    //---------------------------------------------------------------------------------------------
    static QByteArray
    encode(OSCMessage const& message)
    //---------------------------------------------------------------------------------------------
    {
        QByteArray data;
        QString typetag = OSCMessage::typetag(message.arguments).prepend(',');
        data.append(message.method);

        auto pads = 4-(message.method.count()%4);
        while (pads--) data.append((char)0);

        data.append(typetag);
        pads = 4-(typetag.count()%4);

        while (pads--) data.append((char)0);
        append(data, message.arguments);

        return data;
    }

    //---------------------------------------------------------------------------------------------
    static OSCMessage
    decode(QByteArray const& data)
    //---------------------------------------------------------------------------------------------
    {
        OSCMessage message;
        QDataStream stream;
        QString address, typetag;
        QVariantList arguments;

        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

        auto split = data.split(',');
        message.method = split[0];
        typetag = split[1].split('\0')[0];

        uint8_t adpads = 4-(message.method.count()%4);
        uint8_t ttpads = 4-(typetag.count()%4);
        uint32_t total = message.method.count()+adpads+typetag.count()+ttpads+1;

        stream.skipRawData(total);

        for (const auto& c : typetag) {
            switch(c.toLatin1()) {
                case 'i': from_stream<int>(arguments, stream); break;
                case 'f': from_stream<float>(arguments, stream); break;
                case 's': from_stream<QString>(arguments, stream); break;
                case 'T': arguments << true; break;
                case 'F': arguments << false; break;
            }}

        message.method = address;

        switch(arguments.count()) {
            case 0: return message;
            case 1: message.arguments = arguments[0]; break;
            default: message.arguments = arguments;
        }

        return message;
    }

    //---------------------------------------------------------------------------------------------
    static QString
    typetag(QVariant const& argument)
    //---------------------------------------------------------------------------------------------
    {
        switch (argument.type()) {
            case QVariant::Bool: return argument.value<bool>() ? "T" : "F";
            case QVariant::Int: return "i";
            case QVariant::Double: return "f";
            case QVariant::String: return "s";
            case QVariant::Vector2D: return "ff";
            case QVariant::Vector3D: return "fff";
            case QVariant::Vector4D: return "ffff";
        }

        if (argument.type() == QVariant::List ||
            strcmp(argument.typeName(), "QJSValue") == 0)
        {
            // if argument is QVariantList or QJSValue
            // recursively parse arguments
            QString tag;
            for (const auto& sub: argument.toList())
                 tag.append(OSCMessage::typetag(sub));
            return tag;
        }

        return "N";
    }

    //---------------------------------------------------------------------------------------------
    static void
    append(QByteArray& data, QVariant const& argument)
    // parse an OSC argument, integrate it with a byte array
    //---------------------------------------------------------------------------------------------
    {
        QDataStream stream(&data, QIODevice::ReadWrite);
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream.skipRawData(data.size());

        switch(argument.type())
        {
        case QVariant::Int: stream << argument.value<int>(); break;
        case QVariant::Double: stream << argument.value<float>(); break;
        case QVariant::Vector2D: stream << argument.value<QVector2D>(); break;
        case QVariant::Vector3D: stream << argument.value<QVector3D>(); break;
        case QVariant::Vector4D: stream << argument.value<QVector4D>(); break;

        case QVariant::String: {
            QByteArray str = argument.toString().toUtf8();
            auto pads = 4-(str.count()%4);
            while (pads--) str.append('\0');
            data.append(str);
            return;
        }
        case QVariant::List: {
            for (const auto& sub : argument.value<QVariantList>())
                 OSCMessage::append(data, sub);
        }
        }
    }
};

//-------------------------------------------------------------------------------------------------
class Connection : public QObject
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    mg_connection* m_udp_connection;
    mg_connection* m_ws_connection;
    uint16_t m_udp_port = 0;

public:
    Connection(mg_connection* ws_connection) :
        m_ws_connection(ws_connection) {}

    ~Connection() {}

    void
    set_udp(uint16_t udp)
    {
        m_udp_port = udp;

        // TODO: fetch host from ws connection
        m_udp_connection = mg_connect(nullptr, nullptr, nullptr);
    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    writeOSC(QString method, QVariantList arguments, bool critical = false)
    //-------------------------------------------------------------------------------------------------
    {
        OSCMessage msg(method, arguments);
        auto b_arr = OSCMessage::encode(msg);

        if (critical)
             mg_send_websocket_frame(m_ws_connection, WEBSOCKET_OP_BINARY,
             b_arr.data(), b_arr.count());
        else mg_send(m_udp_connection, b_arr.data(), b_arr.count());
    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    writeText(QString text)
    //-------------------------------------------------------------------------------------------------
    {
        auto utf8 = text.toUtf8();
        mg_send_websocket_frame(m_ws_connection, WEBSOCKET_OP_TEXT, utf8.data(), utf8.count());
    }
};

//-------------------------------------------------------------------------------------------------
class Server : public QObject, public QQmlParserStatus
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    Q_PROPERTY (int tcp READ tcp WRITE set_tcp)
    Q_PROPERTY (int udp READ udp WRITE set_udp)
    Q_PROPERTY (QString name READ name WRITE set_name) // zeroconf

    std::vector<Connection>
    m_connections;

    mg_mgr
    m_tcp,
    m_udp;

    uint16_t
    m_tcp_port = 5678,
    m_udp_port = 1234;

    pthread_t
    m_thread;

    bool
    m_running = false;

    QString
    m_name;

public:

    Q_SIGNAL void
    connection();

    Q_SIGNAL void
    disconnection();

    //-------------------------------------------------------------------------------------------------
    Server()
    //-------------------------------------------------------------------------------------------------
    {
        mg_mgr_init(&m_tcp, this);
        mg_mgr_init(&m_udp, this);
    }

    //-------------------------------------------------------------------------------------------------
    void
    classBegin() override {}

    //-------------------------------------------------------------------------------------------------
    void
    componentComplete() override
    //-------------------------------------------------------------------------------------------------
    {
        char s_tcp[5], s_udp[5], udp_hdr[32] = "udp://";

        sprintf(s_tcp, "%d", m_tcp_port);
        sprintf(s_udp, "%d", m_udp_port);
        strcat(udp_hdr, s_udp);

        mg_connection* tcp_connection, *udp_connection;
        tcp_connection = mg_bind(&m_tcp, s_tcp, ws_event_handler);
        udp_connection = mg_bind(&m_udp, udp_hdr, udp_event_handler);
        mg_set_protocol_http_websocket(tcp_connection);

        m_running = true;
        poll(this);
    }

    //-------------------------------------------------------------------------------------------------
    virtual
    ~Server() override
    //-------------------------------------------------------------------------------------------------
    {
        m_running = false;
        pthread_join(m_thread, nullptr);
        mg_mgr_free(&m_tcp);
        mg_mgr_free(&m_udp);
    }

    //-------------------------------------------------------------------------------------------------
    static void
    poll(Server* s)
    //-------------------------------------------------------------------------------------------------
    {
        pthread_create(&s->m_thread, nullptr, pthread_server_poll, s);
    }

    //-------------------------------------------------------------------------------------------------
    static void*
    pthread_server_poll(void* udata)
    //-------------------------------------------------------------------------------------------------
    {
        auto server = static_cast<Server*>(udata);

        while (server->m_running) {
            mg_mgr_poll(&server->m_tcp, 200);
            mg_mgr_poll(&server->m_udp, 200);
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------------------------------
    static void
    ws_event_handler(mg_connection* mgc, int event, void* data)
    //-------------------------------------------------------------------------------------------------
    {
        auto server = static_cast<Server*>(mgc->mgr_data);

        switch(event)
        {
        case MG_EV_RECV:
        {
            break;
        }
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
        {
            server->on_connection(mgc);
            break;
        }
        case MG_EV_WEBSOCKET_FRAME:
        {
            websocket_message* msg = static_cast<websocket_message*>(data);
            server->on_websocket_frame(mgc, msg);
            // parse message...
            break;
        }
        case MG_EV_HTTP_REQUEST:
        {
            http_message* msg = static_cast<http_message*>(data);
            server->on_http_request(mgc, msg->query_string);
            break;
        }
        case MG_EV_CLOSE:
        {
            server->on_disconnection(mgc);
            break;
        }
        }
    }

    //-------------------------------------------------------------------------------------------------
    static void
    udp_event_handler(mg_connection* mgc, int event, void* data)
    //-------------------------------------------------------------------------------------------------
    {
        auto server = static_cast<Server*>(mgc->mgr_data);

        switch(event) {
        case MG_EV_RECV:
            server->on_udp_datagram(mgc, &mgc->recv_mbuf);
            break;
        }
    }

    //-------------------------------------------------------------------------------------------------
    void
    on_connection(mg_connection* connection)
    //-------------------------------------------------------------------------------------------------
    {
        qDebug() << "new websocket connection";
        m_connections.emplace_back(connection);
    }

    //-------------------------------------------------------------------------------------------------
    void
    on_disconnection(mg_connection* connection)
    //-------------------------------------------------------------------------------------------------
    {
        qDebug() << "websocket disconnection";
    }

    //-------------------------------------------------------------------------------------------------
    void
    on_websocket_frame(mg_connection* connection, websocket_message* message)
    //-------------------------------------------------------------------------------------------------
    {
        QByteArray frame((const char*)message->data, message->size);
    }

    //-------------------------------------------------------------------------------------------------
    void
    on_udp_datagram(mg_connection* connection, mbuf* datagram)
    //-------------------------------------------------------------------------------------------------
    {
        QByteArray cdg(datagram->buf, datagram->len);
    }

    //-------------------------------------------------------------------------------------------------
    void
    on_http_request(mg_connection* connection, mg_str method)
    //-------------------------------------------------------------------------------------------------
    {
        char cmethod[256];
        memcpy(cmethod, method.p, method.len);
    }

    //-------------------------------------------------------------------------------------------------
    bool
    running() const { return m_running; }

    //-------------------------------------------------------------------------------------------------
    uint16_t
    tcp() const { return m_tcp_port; }

    uint16_t
    udp() const { return m_udp_port; }

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
    QString
    name() const { return m_name; }

    //-------------------------------------------------------------------------------------------------
    void
    set_name(QString name)
    {
        m_name = name;
    }
};

//-------------------------------------------------------------------------------------------------
class Client : public QObject
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

//    Q_PROPERTY (QString host READ host WRITE set_host)

    mg_connection*
    m_connection;

    pthread_t
    m_thread;

    mg_mgr
    m_mgr;

public:

    Q_SIGNAL void
    connected();

    Q_SIGNAL void
    disconnected();

    //-------------------------------------------------------------------------------------------------
    Client()
    //-------------------------------------------------------------------------------------------------
    {

    }

    //-------------------------------------------------------------------------------------------------
    virtual
    ~Client() override
    //-------------------------------------------------------------------------------------------------
    {

    }

};

class Tree;

//---------------------------------------------------------------------------------------------
class Node : public QObject, public QQmlParserStatus, public QQmlPropertyValueSource
//---------------------------------------------------------------------------------------------
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    Q_INTERFACES (QQmlParserStatus QQmlPropertyValueSource)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QString name READ name WRITE set_name)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QString path READ path WRITE set_path NOTIFY pathChanged)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QVariant value READ value WRITE set_value NOTIFY valueChanged)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (bool critical READ critical WRITE set_critical NOTIFY criticalChanged)

public:

    //---------------------------------------------------------------------------------------------
    Node()
    //---------------------------------------------------------------------------------------------
    {

    }

    //---------------------------------------------------------------------------------------------
    virtual
    ~Node() override
    //---------------------------------------------------------------------------------------------
    {

    }

    //---------------------------------------------------------------------------------------------
    virtual void
    classBegin() override {}

    //---------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    //---------------------------------------------------------------------------------------------
    {

    }

    //---------------------------------------------------------------------------------------------
    virtual void
    setTarget(const QQmlProperty& target) override
    //---------------------------------------------------------------------------------------------
    {

    }

    //---------------------------------------------------------------------------------------------
    QVariant
    value() const { return m_value; }

    Q_SIGNAL void
    valueChanged(QVariant new_value);

    Q_SIGNAL void
    valueReceived(QVariant value);

    //---------------------------------------------------------------------------------------------
    void
    set_value(QVariant value)
    //---------------------------------------------------------------------------------------------
    {
        emit valueReceived(value);

        if (value != m_value) {
            emit valueChanged(value);
            m_value = value;
        }
    }

    //---------------------------------------------------------------------------------------------
    QString
    name() const { return m_name; }

    void
    set_name(QString name)
    {
        m_name = name;
    }

    //---------------------------------------------------------------------------------------------
    QString
    path() const { return m_path; }

    Q_SIGNAL void
    pathChanged();

    void
    set_path(QString path)
    {
        m_path = path;
    }

    //---------------------------------------------------------------------------------------------
    Node*
    parent_node() { return m_parent_node; }

    void
    set_parent_node(Node* node)
    {
        m_parent_node = node;
    }

    //---------------------------------------------------------------------------------------------
    bool
    critical() const { return m_critical; }

    Q_SIGNAL void
    criticalChanged();

    void
    set_critical(bool critical)
    {
        m_critical = critical;
    }

    //---------------------------------------------------------------------------------------------
    QVector<Node*>&
    subnodes() { return m_subnodes; }

    //---------------------------------------------------------------------------------------------
    Node*
    create_subnode(QString name)
    //---------------------------------------------------------------------------------------------
    {
        Node* n = new Node;
        n->set_name(name);
        n->set_parent_node(this);
        m_subnodes.push_back(n);
        return n;
    }

    //---------------------------------------------------------------------------------------------
    void
    add_subnode(Node* subnode)
    //---------------------------------------------------------------------------------------------
    {
        m_subnodes.push_back(subnode);
    }

    //---------------------------------------------------------------------------------------------
    void
    remove_subnode(Node* subnode)
    //---------------------------------------------------------------------------------------------
    {
        m_subnodes.removeOne(subnode);
    }

    //---------------------------------------------------------------------------------------------
    Node*
    subnode(QString name, bool recursive = true)
    //---------------------------------------------------------------------------------------------
    {

    }

    //---------------------------------------------------------------------------------------------
    Node*
    subnode(size_t index)
    //---------------------------------------------------------------------------------------------
    {
        return m_subnodes[index];
    }

    //---------------------------------------------------------------------------------------------
    bool
    has_subnode(Node* subnode) { return m_subnodes.contains(subnode); }

private:

    QString
    m_name,
    m_path;

    QVariant
    m_value;

    Node*
    m_parent_node = nullptr;

    Tree*
    m_tree = nullptr;

    QVector<Node*>
    m_subnodes;

    bool
    m_critical = false;
};

//---------------------------------------------------------------------------------------------
class Tree : public QObject
//---------------------------------------------------------------------------------------------
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (bool singleton READ singleton WRITE set_singleton)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (bool exposed READ exposed WRITE set_exposed)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (int tcp READ tcp WRITE set_tcp)

    Q_PROPERTY (int udp READ udp WRITE set_udp)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (Node* root READ root)

    //---------------------------------------------------------------------------------------------
    QString
    m_name = "wpn114tree";

    Node
    m_root;

    bool
    m_exposed = false;

    static Tree*
    s_singleton;

public:

    //---------------------------------------------------------------------------------------------
    Tree()
    {
        m_root.set_path("/");
    }

    //---------------------------------------------------------------------------------------------
    static Tree&
    instance() { return *s_singleton; }

    //---------------------------------------------------------------------------------------------
    Node*
    root() { return &m_root; }

    //---------------------------------------------------------------------------------------------
    bool
    singleton() const  { return s_singleton == this; }

    //---------------------------------------------------------------------------------------------
    void
    set_singleton(bool singleton)
    //---------------------------------------------------------------------------------------------
    {
        if (singleton)
            s_singleton = this;
    }

    //---------------------------------------------------------------------------------------------
    bool
    exposed() const { return m_exposed; }

    void
    set_exposed(bool exposed)
    {
        m_exposed = exposed;
    }

    //---------------------------------------------------------------------------------------------
    uint16_t
    tcp() const
    {
        return 0;
    }

    void
    set_tcp(uint16_t)
    {

    }

    //---------------------------------------------------------------------------------------------
    uint16_t
    udp() const
    {
        return 0;
    }

    void
    set_udp(uint16_t)
    {

    }

    //---------------------------------------------------------------------------------------------
    Node*
    find_node(QString path)
    //---------------------------------------------------------------------------------------------
    {
        return m_root.subnode(path, true);
    }

    //---------------------------------------------------------------------------------------------
    Node*
    find_or_create_node(QString path)
    //---------------------------------------------------------------------------------------------
    {
        if (auto node = m_root.subnode(path, true))
             return node;
        else return m_root.create_subnode(path);
    }
};
