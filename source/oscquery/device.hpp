#pragma once

#include "node.hpp"
#include "tree.hpp"
#include <QObject>

class WPNNode;
class WPNDevice;

struct HostExtensions
{
    bool access;
    bool value;
    bool range;
    bool description;
    bool tags;
    bool extended_type;
    bool unit;
    bool critical;
    bool clipmode;
    bool listen;
    bool path_changed;
    bool path_removed;
    bool path_added;
    bool path_renamed;
    bool osc_streaming;
    bool html;
    bool echo;

    QJsonObject toJson() const;
};

struct HostSettings
{
    QString name;
    QString osc_transport;
    quint16 osc_port;
    quint16 tcp_port;
    HostExtensions extensions;

    QJsonObject toJson() const;
};

struct WPNValueMap
{
    WPNDevice* target;
    QString source;
    QString destination;
    bool listen_all;
};

class WPNDevice : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES    ( QQmlParserStatus )

    Q_PROPERTY      ( QString deviceName READ deviceName WRITE setDeviceName )
    Q_PROPERTY      ( bool singleDevice READ singleDevice WRITE setSingleDevice )

    public:
    WPNDevice  ( );
    ~WPNDevice ( );

    static WPNDevice* instance();
    static void registerNode(WPNNode* node);
    bool singleDevice() { return m_singleDevice; }
    void setSingleDevice(bool single);

    virtual void componentComplete() override;
    virtual void classBegin() override {}

    virtual void pushNodeValue  ( WPNNode* node ) = 0;

    WPNNode* findOrCreateNode   ( QString path );
    QString deviceName          ( ) const { return m_name; }    
    void setDeviceName          ( QString name );

    Q_INVOKABLE WPNNode* rootNode   ( ) { return m_root_node; }
    Q_INVOKABLE void explore        ( ) const;
    Q_INVOKABLE QVariant value      ( QString method ) const;
    Q_INVOKABLE WPNNode* get        ( QString path );

    Q_INVOKABLE QVariantList collectNodes ( QString pattern );

    Q_INVOKABLE void map       ( WPNDevice*, QString source, QString destination );
    Q_INVOKABLE void mapAll    ( WPNDevice*, QString source );
    Q_INVOKABLE void unmap     ( WPNDevice*, QString source, QString destination );

    Q_INVOKABLE void savePreset ( QString name, QStringList filters, QStringList attributes );
    Q_INVOKABLE void loadPreset ( QString name );

    Q_INVOKABLE WPNNodeTree* nodeTree ( ) const { return m_node_tree; }

    void link(WPNNode* node);

    signals:
    void presetLoaded   ( );
    void nodeAdded      ( WPNNode* );
    void nodeRemoved    ( QString );

    public slots:
    void onNodeRemoved  ( QString );
    void onValueUpdate  ( QString method, QVariant arguments );

    protected:
    bool m_singleDevice;
    static WPNDevice* m_singleton;
    static QVector<WPNNode*> s_registered_nodes;

    QString       m_name;
    WPNNode*      m_root_node;
    WPNNodeTree*  m_node_tree;
    QVector<WPNValueMap> m_maps;
};
