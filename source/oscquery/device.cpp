#include "device.hpp"
#include <QtDebug>
#include <QStandardPaths>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>

QJsonObject HostExtensions::toJson() const
{
    QJsonObject ext
    {
        { "OSC_STREAMING", osc_streaming },
        { "ACCESS", access },
        { "VALUE", value },
        { "RANGE", range },
        { "DESCRIPTION", description },
        { "TAGS", tags },
        { "EXTENDED_TYPE", extended_type },
        { "UNIT", unit },
        { "CRITICAL", critical },
        { "CLIPMODE", clipmode },
        { "LISTEN", listen },
        { "PATH_CHANGED", path_changed },
        { "PATH_REMOVED", path_removed },
        { "PATH_ADDED", path_added },
        { "PATH_RENAMED", path_renamed },
        { "HTML", html },
        { "ECHO", echo }
    };

    return ext;
}

QJsonObject HostSettings::toJson() const
{
    QJsonObject obj
    {
        { "NAME", name },
        { "OSC_PORT", osc_port },
        { "OSC_TRANSPORT", osc_transport },
        { "EXTENSIONS", extensions.toJson() }
    };

    return obj;
}

WPNDevice* WPNDevice::m_singleton;
QVector<WPNNode*> WPNDevice::s_registered_nodes;

WPNDevice* WPNDevice::instance()
{
    return m_singleton;
}

void WPNDevice::registerNode(WPNNode* node)
{
    s_registered_nodes.push_back(node);
}

WPNDevice::WPNDevice() : m_singleDevice(false), m_node_tree(nullptr)
{
    m_root_node = new WPNNode;
    m_root_node ->setPath("/");

    m_node_tree = new WPNNodeTree( m_root_node );
}

void WPNDevice::componentComplete()
{
    if ( m_singleDevice )
        for ( const auto& node : s_registered_nodes )
              link( node );
}

WPNDevice::~WPNDevice()
{
    delete m_root_node;
    delete m_node_tree;
}

void WPNDevice::setSingleDevice(bool single)
{
    m_singleDevice = single;
    if ( single ) m_singleton = this;
}

WPNNode* WPNDevice::findOrCreateNode(QString path)
{
    QStringList split = path.split('/', QString::SkipEmptyParts);
    WPNNode* target = m_root_node;

    for ( const auto& node : split )
    {
        for ( const auto& sub : target->subnodes() )
            if ( sub.ptr->name() == node )
            {
                target = sub.ptr;
                goto next;
            }

        target = target->createSubnode(node);
        next: ;
    }

    return target;
}

void WPNDevice::link(WPNNode* node)
{
    auto parent = findOrCreateNode(node->parentPath());
    if ( parent ) parent->addSubnode(node);
    else
    {
        qDebug() << "[DEVICE] Couldn't link node" << node->path();
        return;
    }

    QObject::connect( node, &WPNNode::nodeRemoved, this, &WPNDevice::onNodeRemoved,
                      Qt::QueuedConnection );

    emit nodeAdded(node);
}

WPNNode* WPNDevice::get(QString path)
{
    auto node = m_root_node->subnode(path);
    if ( !node ) qDebug() << path << "no node at that address";

    return node;
}

void WPNDevice::onValueUpdate(QString method, QVariant arguments)
{
    auto node = m_root_node->subnode(method);
    if ( node )
    {
        node->setValueQuiet( arguments );

        for( const auto& map : m_maps )
        {
            if ( map.listen_all && method.startsWith(map.source) )
                 map.target->onValueUpdate(method, arguments );

            else if ( map.source == method && map.target )
                      map.target->onValueUpdate(method, arguments);
        }
    }

    else qDebug() << "[WPNDEVICE] Node" << method << "not found";
}

void WPNDevice::setDeviceName(QString name)
{
    m_name = name;
}

void WPNDevice::explore() const
{
    m_root_node->post();
}

QVariant WPNDevice::value(QString method) const
{
    auto node = m_root_node->subnode(method);
    if ( node ) return node->value();
    else return QVariant();
}

void WPNDevice::onNodeRemoved(QString path)
{
    auto parent = get( WPNNode::parentPath(path) );
    if ( parent ) parent->removeSubnode(path);

    emit nodeRemoved(path);
}

QVariantList WPNDevice::collectNodes(QString pattern)
{
    QVector<WPNNode*> vec;
    QVariantList result;

    m_root_node->collect(pattern, vec);

    for ( const auto& node : vec )
          result << QVariant::fromValue<WPNNode*>(node);

    return result;
}

void WPNDevice::map(WPNDevice* device, QString source, QString destination)
{
    m_maps.push_back({ device, source, destination, false });
}

void WPNDevice::mapAll(WPNDevice* device, QString source)
{
    m_maps.push_back({ device, source, source, true });
}

void WPNDevice::unmap(WPNDevice* device, QString source, QString destination)
{
    qint32 idx = -1;

    for ( qint32 i = 0; i < m_maps.size(); ++i )
    {
        auto map = m_maps[ i ];

        if ( map.listen_all && map.source == source )
        {
             idx = i;
             break;
        }

        else if ( map.source == source &&
             map.destination == destination &&
             map.target == device )
        {
            idx = i;
            break;
        }
    }

    if ( idx >= 0 ) m_maps.removeAt(idx);
}

void WPNDevice::savePreset(QString name, QStringList filters, QStringList attributes)
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            .append("/presets/");

     if ( !QDir(path).exists()) QDir().mkpath(path);
     QFile file( path+name );

     if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text ))
     {
         qDebug() << "ERROR, COULD NOT SAVE PRESET at location"
                  << path+name;
         return;
     }

     QVector<WPNNode*> nodes;
     for ( const auto& filter : filters )
           m_root_node->collect(filter, nodes);

     QJsonArray arr;
     for ( const auto& node : nodes )
     {
         QJsonObject obj;
         obj.insert( "FULL_PATH", node->path()  );

         for ( const auto& attribute : attributes )
             obj.insert( attribute, node->attributeValue( attribute ) );

         arr.append( obj );
     }

     file.write( QJsonDocument(arr).toJson(QJsonDocument::Compact) );
}

void WPNDevice::loadPreset(QString name)
{
    QString path = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation )
            .append("/presets/");

    QFile file( path+name );

    if ( !file.open(QIODevice::ReadOnly | QIODevice::Text ))
    {
        qDebug() << "ERROR, COULD NOT LOAD PRESET at location"
                 << path+name;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());

    for ( const auto& object : doc.array())
    {
        auto obj  = object.toObject();
        auto node = m_root_node->subnode(obj["FULL_PATH"].toString());
        if ( !node ) continue;

        node->update( obj );
    }

    emit presetLoaded();
}

