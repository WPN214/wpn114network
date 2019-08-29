#pragma once

#include <QObject>

#include <source/tree.hpp>
#include <dependencies/mongoose/mongoose.h>
#include <dependencies/qzeroconf/qzeroconf.h>

#define CSTR(_qstring) _qstring.toStdString().c_str()

Q_DECLARE_METATYPE(mg_connection*)
Q_DECLARE_METATYPE(http_message*)
Q_DECLARE_METATYPE(websocket_message*)

namespace WPN114   {
namespace Network  {

//=================================================================================================
class Connection : public QObject
//=================================================================================================
{
    Q_OBJECT

public:

    Connection() {}

    Connection(mg_connection* ws_connection);

    Connection(Connection const& cp) :
        m_ws_connection     (cp.m_ws_connection),
        m_udp_connection    (cp.m_udp_connection),
        m_udp_port          (cp.m_udp_port),
        m_host_ip           (cp.m_host_ip) {}

    Connection&
    operator=(Connection const& cp)
    {
        m_ws_connection     = cp.m_ws_connection;
        m_udp_connection    = cp.m_udp_connection;
        m_udp_port          = cp.m_udp_port;
        m_host_ip           = cp.m_host_ip;

        return *this;
    }

    virtual
    ~Connection() override {}

    //---------------------------------------------------------------------------------------------
    mg_connection*
    mgc() { return m_ws_connection; }

    //---------------------------------------------------------------------------------------------
    void
    set_udp(uint16_t udp);

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE
    QString address() const { return m_host_ip; }

    //-------------------------------------------------------------------------------------------------
    Q_SLOT void
    on_value_changed(QVariant value);

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    writeOSC(QString method, QVariantList arguments, bool critical = false);

    Q_INVOKABLE void
    writeText(QString text);

    Q_INVOKABLE void
    writeJson(QJsonObject object);

private:

    mg_connection*
    m_udp_connection = nullptr;

    mg_connection*
    m_ws_connection = nullptr;

    mg_mgr
    m_mgr;

    uint16_t
    m_udp_port = 0;

    QString
    m_host_ip,
    m_host_udp;
};

//=================================================================================================
class NetworkDevice : public QObject, public QQmlParserStatus
//=================================================================================================
{
    Q_OBJECT

    Q_PROPERTY      (QQmlListProperty<Node> subnodes READ subnodes)
    Q_INTERFACES    (QQmlParserStatus)
    Q_CLASSINFO     ("DefaultProperty", "subnodes")

public:

    //---------------------------------------------------------------------------------------------
    void
    classBegin() override {}

    //---------------------------------------------------------------------------------------------
    void
    componentComplete() override {}

    //---------------------------------------------------------------------------------------------
    Q_INVOKABLE Node*
    get(QString path) { return m_tree.find(path); }

    Q_INVOKABLE Tree*
    tree() { return &m_tree; }

    //---------------------------------------------------------------------------------------------
    Q_INVOKABLE QVariant
    value(QString path)
    //---------------------------------------------------------------------------------------------
    {
        if  (auto node = m_tree.find(path))
             return node->value();
        else return QVariant();
    }

    // --------------------------------------------------------------------------------------------
    QQmlListProperty<Node>
    subnodes()
    // returns subnodes (QML format)
    // --------------------------------------------------------------------------------------------
    {
        return QQmlListProperty<Node>(
               this, this,
               &NetworkDevice::append_subnode,
               &NetworkDevice::nsubnodes,
               &NetworkDevice::subnode,
               &NetworkDevice::clear_subnodes);
    }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    append_subnode(Node* node) { m_tree.link(node); }
    // appends a subnode to this Node children

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE int
    nsubnodes() { return m_tree.root()->nsubnodes(); }
    // returns this Node' subnodes count

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE Node*
    subnode(int index) { return m_tree.root()->subnode(index); }
    // returns this Node' subnode at index

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    clear_subnodes() {  }

    // --------------------------------------------------------------------------------------------
    static void
    append_subnode(QQmlListProperty<Node>* list, Node* node)
    // static Qml version, see above
    {
        reinterpret_cast<NetworkDevice*>(list->data)->append_subnode(node);
    }

    // --------------------------------------------------------------------------------------------
    static int
    nsubnodes(QQmlListProperty<Node>* list)
    // static Qml version, see above
    {
        return reinterpret_cast<NetworkDevice*>(list)->nsubnodes();
    }

    // --------------------------------------------------------------------------------------------
    static Node*
    subnode(QQmlListProperty<Node>* list, int index)
    // static Qml version, see above
    {
        return reinterpret_cast<NetworkDevice*>(list)->subnode(index);
    }

    // --------------------------------------------------------------------------------------------
    static void
    clear_subnodes(QQmlListProperty<Node>* list)
    // static Qml version, see above
    {
        reinterpret_cast<NetworkDevice*>(list)->clear_subnodes();
    }

protected:

    Tree
    m_tree;

    QZeroConf
    m_zeroconf;

};

}
}

Q_DECLARE_METATYPE(WPN114::Network::Connection)
