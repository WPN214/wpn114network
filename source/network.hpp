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
    QString
    m_method;

    QVariant
    m_arguments;

    //---------------------------------------------------------------------------------------------
    OSCMessage() {}

    //---------------------------------------------------------------------------------------------
    OSCMessage(QString method, QVariant arguments) :
        m_method(method), m_arguments(arguments) {}

    //---------------------------------------------------------------------------------------------
    QByteArray
    encode() const
    //---------------------------------------------------------------------------------------------
    {
        QByteArray data;
        QString tt = typetag(m_arguments).prepend(',');
        data.append(m_method);

        auto pads = 4-(m_method.count()%4);
        while (pads--) data.append((char)0);

        data.append(tt);
        pads = 4-(tt.count()%4);

        while (pads--) data.append((char)0);
        append(data, m_arguments);

        return data;
    }

    //---------------------------------------------------------------------------------------------
    OSCMessage(QByteArray const& data)
    //---------------------------------------------------------------------------------------------
    {
        QDataStream stream;
        QString address, typetag;
        QVariantList arguments;

        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

        auto split = data.split(',');
        m_method = split[0];
        typetag = split[1].split('\0')[0];

        uint8_t adpads = 4-(m_method.count()%4);
        uint8_t ttpads = 4-(typetag.count()%4);
        uint32_t total = m_method.count()+adpads+typetag.count()+ttpads+1;

        stream.skipRawData(total);

        for (const auto& c : typetag) {
            switch(c.toLatin1()) {
                case 'i': from_stream<int>(arguments, stream); break;
                case 'f': from_stream<float>(arguments, stream); break;
                case 's': from_stream<QString>(arguments, stream); break;
                case 'T': arguments << true; break;
                case 'F': arguments << false; break;
            }}

        m_method = address;

        switch(arguments.count()) {
            case 0: break;
            case 1: m_arguments = arguments[0]; break;
            default: m_arguments = arguments;
        }
    }

    //---------------------------------------------------------------------------------------------
    QString
    typetag(QVariant const& argument) const
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
    void
    append(QByteArray& data, QVariant const& argument) const
    // parse an OSC argument, integrate it with a byte array
    //---------------------------------------------------------------------------------------------
    {
        QDataStream stream(&data, QIODevice::ReadWrite);
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream.skipRawData(data.size());

        switch(argument.type())
        {
        case QVariant::Bool: stream << argument.value<bool>(); break;
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

    mg_connection*
    m_udp_connection = nullptr;

    mg_connection*
    m_ws_connection = nullptr;

    uint16_t
    m_udp_port = 0;

    QString
    m_host_ip;

public:

    //---------------------------------------------------------------------------------------------
    Connection() {}

    //---------------------------------------------------------------------------------------------
    Connection(mg_connection* ws_connection) :
        m_ws_connection(ws_connection)
    {
        char addr[32];
        mg_sock_addr_to_str(&ws_connection->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP);
        m_host_ip = addr;

        qDebug() << "new connection" << m_host_ip;
    }

    //---------------------------------------------------------------------------------------------
    Connection(Connection const& cp) :
        m_ws_connection(cp.m_ws_connection),
        m_udp_connection(cp.m_udp_connection),
        m_udp_port(cp.m_udp_port),
        m_host_ip(cp.m_host_ip) {}

    //---------------------------------------------------------------------------------------------
    virtual
    ~Connection() override{}

    //---------------------------------------------------------------------------------------------
    Connection&
    operator=(Connection const& cp)
    //---------------------------------------------------------------------------------------------
    {
        m_ws_connection = cp.m_ws_connection;
        m_udp_connection = cp.m_udp_connection;
        m_udp_port = cp.m_udp_port;
        m_host_ip = cp.m_host_ip;

        return *this;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_udp(uint16_t udp)
    //---------------------------------------------------------------------------------------------
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
        auto b_arr = msg.encode();

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

Q_DECLARE_METATYPE(Connection)

#include <avahi-client/client.h>
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>

using avahi_client = AvahiClient;
using avahi_simple_poll = AvahiSimplePoll;
using avahi_entry_group = AvahiEntryGroup;

//-------------------------------------------------------------------------------------------------
class Server : public QObject, public QQmlParserStatus
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    Q_PROPERTY (int tcp READ tcp WRITE set_tcp)
    Q_PROPERTY (int udp READ udp WRITE set_udp)
    Q_PROPERTY (QString name READ name WRITE set_name) // zeroconf

    Q_INTERFACES (QQmlParserStatus)

    avahi_simple_poll*
    m_avpoll = nullptr;

    avahi_entry_group*
    m_avgroup = nullptr;

    avahi_client*
    m_avclient;

    std::vector<Connection>
    m_connections;

    mg_mgr
    m_tcp,
    m_udp;

    uint16_t
    m_tcp_port = 5678,
    m_udp_port = 1234;

    pthread_t
    m_mgthread,
    m_avthread;

    bool
    m_running = false;

    QString
    m_name = "wpn114";

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

        mg_connection* tcp_connection = mg_bind(&m_tcp, s_tcp, ws_event_handler);
        mg_connection* udp_connection = mg_bind(&m_udp, udp_hdr, udp_event_handler);

        mg_set_protocol_http_websocket(tcp_connection);

        m_avpoll    = avahi_simple_poll_new();
        m_avclient  = avahi_client_new(avahi_simple_poll_get(m_avpoll),
                      static_cast<AvahiClientFlags>(0), avahi_client_callback, this, nullptr);

        m_running = true;
        poll();
    }

    //-------------------------------------------------------------------------------------------------
    virtual
    ~Server() override
    //-------------------------------------------------------------------------------------------------
    {
        m_running = false;
        pthread_join(m_mgthread, nullptr);
        pthread_join(m_avthread, nullptr);

        avahi_client_free(m_avclient);
        avahi_simple_poll_free(m_avpoll);

        mg_mgr_free(&m_tcp);
        mg_mgr_free(&m_udp);
    }

    //-------------------------------------------------------------------------------------------------
    void
    poll()
    //-------------------------------------------------------------------------------------------------
    {
        pthread_create(&m_mgthread, nullptr, pthread_server_poll, this);
        pthread_create(&m_avthread, nullptr, pthread_avahi_poll, this);
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
    static void*
    pthread_avahi_poll(void* udata)
    //-------------------------------------------------------------------------------------------------
    {
        auto server = static_cast<Server*>(udata);
        avahi_simple_poll_loop(server->m_avpoll);
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------------
    static void
    avahi_group_callback(avahi_entry_group* group, AvahiEntryGroupState state, void* udata)
    //-------------------------------------------------------------------------------------------------
    {
        switch(state)
        {
        case AVAHI_ENTRY_GROUP_REGISTERING:
        {
            fprintf(stderr, "[avahi] entry group registering\n");
            break;
        }
        case AVAHI_ENTRY_GROUP_ESTABLISHED:
        {
            fprintf(stderr, "[avahi] entry group established\n");
            break;
        }
        case AVAHI_ENTRY_GROUP_COLLISION:
        {
            fprintf(stderr, "[avahi] entry group collision\n");
            break;
        }
        case AVAHI_ENTRY_GROUP_FAILURE:
        {
            fprintf(stderr, "[avahi] entry group failure\n");
            break;
        }
        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        {
            fprintf(stderr, "[avahi] entry group uncommited\n");
            break;
        }
        }
    }

    //-------------------------------------------------------------------------------------------------
    static void
    avahi_client_callback(avahi_client* client, AvahiClientState state, void* udata)
    //-------------------------------------------------------------------------------------------------
    {
        auto server = static_cast<Server*>(udata);

        switch(state)
        {
        case AVAHI_CLIENT_CONNECTING:
        {
            fprintf(stderr, "[avahi] client connecting\n");
            break;
        }
        case AVAHI_CLIENT_S_REGISTERING:
        {
            fprintf(stderr, "[avahi] client registering\n");
            break;
        }
        case AVAHI_CLIENT_S_RUNNING:
        {
            fprintf(stderr, "[avahi] client running\n");

            auto group = server->m_avgroup;
            if(!group)
            {
                fprintf(stderr, "[avahi] creating entry group\n");
                group  = avahi_entry_group_new(client, avahi_group_callback, server);
                server->m_avgroup = group;
            }

            if (avahi_entry_group_is_empty(group))
            {
                fprintf(stderr, "[avahi] adding service\n");

                int err = avahi_entry_group_add_service(group,
                    AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, static_cast<AvahiPublishFlags>(0),
                    server->m_name.toStdString().c_str(), "_oscjson._tcp", nullptr, nullptr, server->m_tcp_port, nullptr);

                if (err) {
                     fprintf(stderr, "Failed to add service: %s\n", avahi_strerror(err));
                     return;
                }

                fprintf(stderr, "[avahi] commiting service\n");
                err = avahi_entry_group_commit(group);

                if (err) {
                    fprintf(stderr, "Failed to commit group: %s\n", avahi_strerror(err));
                    return;
                }
            }
            break;
        }
        case AVAHI_CLIENT_FAILURE:
        {
            fprintf(stderr, "[avahi] client failure");
            break;
        }
        case AVAHI_CLIENT_S_COLLISION:
        {
            fprintf(stderr, "[avahi] client collision");
            break;
        }
        }
    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    on_connection(Connection const& con)
    // this method dwells in the qt thread, it has to be invoked with a queued connection
    // from the mgr callback
    //-------------------------------------------------------------------------------------------------
    {
        m_connections.push_back(con);
    }

    //-------------------------------------------------------------------------------------------------
    static void
    ws_event_handler(mg_connection* mgc, int event, void* data)
    //-------------------------------------------------------------------------------------------------
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
            Connection con(mgc);
            QMetaObject::invokeMethod(server, "on_connection",
                Qt::QueuedConnection, Q_ARG(Connection, con));
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
    Q_SLOT void
    on_disconnection(mg_connection* connection)
    //-------------------------------------------------------------------------------------------------
    {
        qDebug() << "websocket disconnection";
    }

    //-------------------------------------------------------------------------------------------------
    Q_SLOT void
    on_websocket_frame(mg_connection* connection, websocket_message* message)
    //-------------------------------------------------------------------------------------------------
    {
        QByteArray frame((const char*)message->data, message->size);
    }

    //-------------------------------------------------------------------------------------------------
    Q_SLOT void
    on_udp_datagram(mg_connection* connection, mbuf* datagram)
    //-------------------------------------------------------------------------------------------------
    {
        QByteArray cdg(datagram->buf, datagram->len);
    }

    //-------------------------------------------------------------------------------------------------
    Q_SLOT void
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
