#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QQmlPropertyValueSource>
#include <QQmlParserStatus>
#include <QQmlProperty>
#include <QVariant>
#include <QJsonObject>
#include <QJsonArray>
#include <QtDebug>

class Tree;

//=================================================================================================
class Type : public QObject
//=================================================================================================
{
    Q_OBJECT

public:
    enum Values
    {
        None        = 43,
        Bool        = 1,
        Int         = 2,
        Float       = 6,
        String      = 10,
        List        = 9,
        Vec2f       = 82,
        Vec3f       = 83,
        Vec4f       = 84,
        Char        = 34,
        Impulse     = 0,
        File        = 11
    };

    Q_ENUM (Values)
};

//=================================================================================================
class Node : public QObject, public QQmlParserStatus, public QQmlPropertyValueSource
//=================================================================================================
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    Q_INTERFACES (QQmlParserStatus QQmlPropertyValueSource)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QString name READ name WRITE set_name)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QString path READ path WRITE set_path NOTIFY pathChanged)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY(Type::Values type READ type WRITE set_type NOTIFY typeChanged)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QVariant value READ value WRITE set_value NOTIFY valueChanged)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (bool critical READ critical WRITE set_critical NOTIFY criticalChanged)

    //-------------------------------------------------------------------------------------------------
    Q_PROPERTY (Tree* tree READ tree WRITE set_tree)

protected:

    QString
    m_name,
    m_path,
    m_extended_type;

    Type::Values
    m_type = Type::None;

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

    QQmlProperty
    m_target;

public:

    //---------------------------------------------------------------------------------------------
    Node() {}

    //---------------------------------------------------------------------------------------------
    Node(Node& parent, QString name, Type::Values type) :
      m_type            (type),
      m_name            (name),
      m_parent_node     (&parent),
      m_tree            (parent.tree())
    //---------------------------------------------------------------------------------------------
    {
        if (parent.path() == "/")
             m_path = parent.path().append(name);
        else m_path = parent.path().append('/').append(name);

        // prevents qml from destroying nodes when referenced in javascript functions
        QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    }

    //---------------------------------------------------------------------------------------------
    static Node*
    from_json(QJsonObject const& object)
    //---------------------------------------------------------------------------------------------
    {
        auto node = new Node;
        node->update(object);
        return node;
    }

    //---------------------------------------------------------------------------------------------
    virtual
    ~Node() override
    //---------------------------------------------------------------------------------------------
    {
        for (auto& subnode : m_subnodes)
             delete subnode;
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    classBegin() override {}

    //---------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override;

    //---------------------------------------------------------------------------------------------
    virtual void
    setTarget(const QQmlProperty& target) override
    //---------------------------------------------------------------------------------------------
    {
        m_target = target;
        set_type(static_cast<QMetaType::Type>(target.propertyType()));
        m_value = target.read();
        m_target.connectNotifySignal(this, SLOT(on_property_changed()));
    }

    //---------------------------------------------------------------------------------------------
    Q_SLOT void
    on_property_changed()
    //---------------------------------------------------------------------------------------------
    {
        auto value = m_target.read();
        valueReceived(value);

        if (value != m_value) {
            valueChanged(value);
            m_value = value;
        }
    }

    //---------------------------------------------------------------------------------------------
    QVariant
    value() const { return m_value; }

    bool
    critical() const { return m_critical; }

    QString
    name() const { return m_name; }

    QString
    path() const { return m_path; }

    Type::Values
    type() const { return m_type; }

    Node*
    parent_node() { return m_parent_node; }

    Tree*
    tree() { return m_tree; }

    int
    nsubnodes() const { return m_subnodes.count(); }

    //---------------------------------------------------------------------------------------------
    int
    index() const
    //---------------------------------------------------------------------------------------------
    {
        for (int n = 0; n < m_parent_node->nsubnodes(); ++n)
            if (m_parent_node->subnode(n) == this)
                return n;
        return -1;
    }

    //---------------------------------------------------------------------------------------------
    QString
    typetag() const
    //---------------------------------------------------------------------------------------------
    {
        switch (m_type)
        {
        case Type::Bool:        return "T";
        case Type::Char:        return "c";
        case Type::Float:       return "f";
        case Type::Int:         return "i";
        case Type::List:        return "l";
        case Type::None:        return "N";
        case Type::Impulse:     return "I";
        case Type::String:      return "s";
        case Type::Vec2f:       return "ff";
        case Type::Vec3f:       return "fff";
        case Type::Vec4f:       return "ffff";
        default:                return "";
        }
    }

    //---------------------------------------------------------------------------------------------
    QJsonValue
    json_value() const
    //---------------------------------------------------------------------------------------------
    {
        switch (m_type)
        {
        case Type::Bool:        return m_value.toBool();
        case Type::Char:        return m_value.toString();
        case Type::Float:       return m_value.toFloat();
        case Type::Int:         return m_value.toInt();
        case Type::String:      return m_value.toString();
        case Type::Vec2f:
        case Type::Vec3f:
        case Type::Vec4f:
        case Type::List:        return QJsonArray::fromVariantList(m_value.toList());
        case Type::None:
        case Type::Impulse:
        default:                return QJsonValue();
        }
    }

    //---------------------------------------------------------------------------------------------
    Q_SIGNAL void
    valueChanged(QVariant value);

    Q_SIGNAL void
    valueReceived(QVariant value);

    Q_SIGNAL void
    pathChanged();

    Q_SIGNAL void
    typeChanged();

    Q_SIGNAL void
    criticalChanged();

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
    void
    set_name(QString name)
    //---------------------------------------------------------------------------------------------
    {
        m_name = name;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_path(QString path)
    //---------------------------------------------------------------------------------------------
    {
        m_path = path;
    }

    //-------------------------------------------------------------------------------------------------
    void
    set_tree(Tree* tree)
    {
        m_tree = tree;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_type(Type::Values type)
    //---------------------------------------------------------------------------------------------
    {
        m_type = type;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_type(QString type)
    //---------------------------------------------------------------------------------------------
    {
        if (type == "f")
            m_type = Type::Float;
        else if (type == "T" || type == "F")
            m_type = Type::Bool;
        else if (type == "i")
            m_type = Type::Int;
        else if (type == "I")
            m_type = Type::Impulse;
        else if (type == "s")
            m_type = Type::String;
        else if (type == "ff")
            m_type = Type::Vec2f;
        else if (type == "fff")
            m_type = Type::Vec3f;
        else if (type == "ffff")
            m_type = Type::Vec4f;
        else if (type == "N")
            m_type = Type::Impulse;
        else m_type = Type::None;
    }

    //---------------------------------------------------------------------------------------------v
    void
    set_type(QMetaType::Type type)
    //---------------------------------------------------------------------------------------------
    {
        m_type = static_cast<Type::Values>(type);
    }

    //---------------------------------------------------------------------------------------------
    void
    set_parent_node(Node* node)
    //---------------------------------------------------------------------------------------------
    {
        m_parent_node = node;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_critical(bool critical)
    //---------------------------------------------------------------------------------------------
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
        m_subnodes.push_back(new Node(*this, name, Type::None));
        return m_subnodes.back();
    }

    //---------------------------------------------------------------------------------------------
    void
    add_subnode(Node* subnode)
    //---------------------------------------------------------------------------------------------
    {
        subnode->set_parent_node(this);
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
    subnode(QString name)
    //---------------------------------------------------------------------------------------------
    {
        for (const auto& subnode : m_subnodes)
             if (subnode->name() == name)
                 return subnode;
        return nullptr;
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

    //---------------------------------------------------------------------------------------------
    QJsonObject
    attributes_json() const
    //---------------------------------------------------------------------------------------------
    {
        QJsonObject attributes;
        attributes["FULL_PATH"] = m_path;

        if (m_type == Type::None)
            return attributes;

        attributes["TYPE"]      = typetag();
        attributes["CRITICAL"]  = m_critical;
        attributes["VALUE"]     = json_value();

        return attributes;
    }

    //---------------------------------------------------------------------------------------------
    QJsonObject const
    to_json() const
    //---------------------------------------------------------------------------------------------
    {
        QJsonObject contents;
        for (auto& subnode : m_subnodes)
            contents.insert(subnode->name(), subnode->to_json());

        auto attributes = attributes_json();
        attributes["CONTENTS"] = contents;
        return attributes;
    }

    //---------------------------------------------------------------------------------------------
    void
    update(QJsonObject object)
    //---------------------------------------------------------------------------------------------
    {
        if (object.contains("FULL_PATH"))
            m_path = object["FULL_PATH"].toString();

        if (object.contains("TYPE"))
            set_type(object["TYPE"].toString());

        if (object.contains("CONTENTS"))
        {
            // recursively parse and build children nodes
            auto contents = object["CONTENTS"].toObject();
            for (const auto& key : contents.keys()) {
                auto node = Node::from_json(contents[key].toObject());
                node->set_name(key);
                add_subnode(node);
            }
        }

        if (object.contains("VALUE"))
            set_value(object["VALUE"].toVariant());
    }
};

#include <QFile>

//=================================================================================================
class File : public Node
//=================================================================================================
{
    Q_OBJECT

    Q_PROPERTY (QString file READ file WRITE set_file)

    QFile*
    m_file = nullptr;

    QByteArray
    m_data;

    QString
    m_file_path;

public:

    //---------------------------------------------------------------------------------------------
    File()
    //---------------------------------------------------------------------------------------------
    {
        m_type = Type::File;
    }

    //---------------------------------------------------------------------------------------------
    QString
    file() const { return m_file_path; }

    QByteArray
    data() const { return m_data; }

    //---------------------------------------------------------------------------------------------
    void
    set_file(QString path)
    //---------------------------------------------------------------------------------------------
    {
        m_file_path = path;
        m_file = new QFile(path, this);

        if (path.endsWith(".qml")) {
            m_file->open(QIODevice::ReadOnly | QIODevice::Text);
            m_data = m_file->readAll();
        }
    }
};

#include <QDir>

//=================================================================================================
class Directory : public Node
//=================================================================================================
{
    Q_OBJECT

    Q_PROPERTY (QString target READ target WRITE set_target)
    Q_PROPERTY (QString extensions READ extensions WRITE set_extensions)
    Q_PROPERTY (QStringList filters READ filters WRITE set_filters)
    Q_PROPERTY (bool recursive READ recursive WRITE set_recursive)

    QStringList
    m_filters;

    bool
    m_recursive;

    QString
    m_directory_path,
    m_extensions;

public:

    //---------------------------------------------------------------------------------------------
    Directory()
    //---------------------------------------------------------------------------------------------
    {
        m_type = Type::List;
        m_extended_type = "directory";
    }

    //---------------------------------------------------------------------------------------------
    QString
    target() const { return m_directory_path; }

    QString
    extensions() const { return m_extensions; }

    QStringList
    filters() const { return m_filters; }

    bool
    recursive() const { return m_recursive; }

    //---------------------------------------------------------------------------------------------
    void
    set_target(QString path)
    //---------------------------------------------------------------------------------------------
    {
        m_directory_path = path;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_extensions(QString extensions)
    //---------------------------------------------------------------------------------------------
    {
        m_extensions = extensions;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_filters(QStringList filters)
    //---------------------------------------------------------------------------------------------
    {
        m_filters = filters;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_recursive(bool recursive)
    //---------------------------------------------------------------------------------------------
    {
        m_recursive = recursive;
    }

    //---------------------------------------------------------------------------------------------
    void
    parse_directory(QDir directory)
    //---------------------------------------------------------------------------------------------
    {
        set_value(directory.entryList());

        for (const auto& file : directory.entryList()) {
            auto node = new File;
            node->set_name(file);
            node->set_file(directory.path().append("/").append(file));
            add_subnode(node);
        }

        if (m_recursive)
        {
            directory.setNameFilters(QStringList{});
            directory.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

            for (const auto& subdirectory : directory.entryList()) {
                auto node = new Directory;
                node->set_name(subdirectory);
                node->set_recursive(true);
                node->set_filters(m_filters);
                node->set_target(directory.path().append("/").append(subdirectory));
                add_subnode(node);

                node->componentComplete();
            }
        }
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    //---------------------------------------------------------------------------------------------
    {
        Node::componentComplete();

        QDir directory(m_directory_path);
        directory.setNameFilters(m_filters);
        parse_directory(directory);
    }
};

#include <QQueue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>

//=================================================================================================
class MirrorDirectory : public Node
//=================================================================================================
{
    Q_OBJECT

    Q_PROPERTY (bool recursive READ recursive WRITE set_recursive)
    Q_PROPERTY (QString destination READ destination WRITE set_destination)
    Q_PROPERTY (QString host READ host WRITE set_host)

    bool
    m_recursive = true;

    QString
    m_destination,
    m_absolute_path,
    m_host;

    QStringList
    m_downloads;

    QVector<MirrorDirectory*>
    m_children_directories;

    QNetworkAccessManager
    m_netaccess;

    QQueue<QUrl>
    m_queue;

    QFile
    m_output;

    QNetworkReply*
    m_current_download = nullptr;

public:

    //---------------------------------------------------------------------------------------------
    MirrorDirectory()
    //---------------------------------------------------------------------------------------------
    {
        // update downloads whenever node value is changed
        QObject::connect(this, &Node::valueChanged, this, &MirrorDirectory::on_file_list_changed);
    }

    //---------------------------------------------------------------------------------------------
    bool
    recursive() const { return m_recursive; }

    QString
    destination() const { return m_destination; }

    QString
    host() const { return m_host; }

    //---------------------------------------------------------------------------------------------
    void
    set_recursive(bool recursive)
    //---------------------------------------------------------------------------------------------
    {
        m_recursive = recursive;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_destination(QString destination)
    //---------------------------------------------------------------------------------------------
    {
        m_destination = destination;
        m_absolute_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         .append(destination);

        if (!QDir(m_absolute_path).exists())
            QDir().mkpath(m_absolute_path);
    }

    //---------------------------------------------------------------------------------------------
    void
    set_host(QString host)
    //---------------------------------------------------------------------------------------------
    {
        m_host = host;
    }

    //---------------------------------------------------------------------------------------------
    Q_SIGNAL void
    complete();


protected slots:

    //---------------------------------------------------------------------------------------------
    QUrl
    to_url(QString file)
    //---------------------------------------------------------------------------------------------
    {
        file.prepend("/").prepend(m_destination).prepend(m_host);
        return QUrl(file);
    }

    //---------------------------------------------------------------------------------------------
    void
    on_file_list_changed(QVariant vlist)
    //---------------------------------------------------------------------------------------------
    {
        if (m_destination.isNull())
            set_destination(m_path);

        for (const auto& file : vlist.toList())
             m_downloads.append(file.toString());

        for (const auto& file : m_downloads)
             m_queue.enqueue(to_url(file));

        next();
    }

    //---------------------------------------------------------------------------------------------
    void
    next()
    //---------------------------------------------------------------------------------------------
    {
        if (m_queue.isEmpty()) {
            emit complete();
            return;
        }

        auto path = m_downloads.first().prepend("/").prepend(m_absolute_path);
        m_output.setFileName(path);

        if (!m_output.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            qDebug() << "error opening file for writing";
            qDebug() << m_path;
            qDebug() << m_output.errorString();
        }

        qDebug() << "[Directory] Downloading file:"
                 << m_downloads.first();

        auto url = m_queue.dequeue();
        QNetworkRequest request(url);
        m_current_download = m_netaccess.get(request);

        QObject::connect(m_current_download, &QNetworkReply::readyRead, this, &MirrorDirectory::on_ready_read);
        QObject::connect(m_current_download, &QNetworkReply::finished, this, &MirrorDirectory::on_complete);
    }

    //---------------------------------------------------------------------------------------------
    void
    on_ready_read()
    //---------------------------------------------------------------------------------------------
    {
        auto read = m_current_download->readAll();
        m_output.write(read);
    }

    //---------------------------------------------------------------------------------------------
    void
    on_complete()
    //---------------------------------------------------------------------------------------------
    {
        m_output.close();
        QObject::disconnect(m_current_download, &QNetworkReply::readyRead, this, &MirrorDirectory::on_ready_read);
        QObject::disconnect(m_current_download, &QNetworkReply::finished, this, &MirrorDirectory::on_complete);

        if  (m_current_download->error())
             qDebug() << "[Directory] Error:" << m_current_download->errorString();
        else qDebug() << "[Directory] File download complete:" << m_downloads.first();

        m_current_download->deleteLater();
        m_downloads.removeFirst();
        next();
    }
};

//=================================================================================================
class Tree : public QObject
//=================================================================================================
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (bool singleton READ singleton WRITE set_singleton)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (Node* root READ root)

    //---------------------------------------------------------------------------------------------
    QString
    m_name = "wpn114tree";

    Node
    m_root;

    static Tree*
    s_singleton;

public:

    //---------------------------------------------------------------------------------------------
    Tree()
    //---------------------------------------------------------------------------------------------
    {
        m_root.set_path("/");
        m_root.set_tree(this);
    }

    //---------------------------------------------------------------------------------------------
    static Tree*
    instance() { return s_singleton; }

    //---------------------------------------------------------------------------------------------
    Q_SIGNAL void
    nodeAdded(Node* node);

    //---------------------------------------------------------------------------------------------
    Q_SIGNAL void
    nodeRemoved(Node* node);

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
    void
    link(Node* node)
    //---------------------------------------------------------------------------------------------
    {
        // if node already exists, replace it
        if (auto dup = find(node->path()))
        {
            auto parent = dup->parent_node();

            for (auto subnode : dup->subnodes())
                 node->add_subnode(subnode);

            parent->remove_subnode(dup);
            parent->add_subnode(node);
            return;
        }

        auto parent = find_or_create(parent_path(node->path()));
        parent->add_subnode(node);
    }

    //---------------------------------------------------------------------------------------------
    Node*
    find(QString path)
    //---------------------------------------------------------------------------------------------
    {
        if (path == "/" || path.isEmpty())
            return &m_root;

        auto split = path.split('/');
        split.removeFirst();

        Node* target = &m_root;

        for (auto& name : split) {
            auto subnode = target->subnode(name);
            if (subnode == nullptr)
                 return nullptr;
            else target = subnode;
        }

        return target;
    }

    //---------------------------------------------------------------------------------------------
    Node*
    find_or_create(QString path)
    //---------------------------------------------------------------------------------------------
    {
        if (path == "/" || path.isEmpty())
            return &m_root;

        auto split = path.split('/');
        split.removeFirst();

        Node* target = &m_root;

        for (auto& name : split) {
            auto subnode = target->subnode(name);
            if (subnode == nullptr)
                subnode = target->create_subnode(name);
            target = subnode;
        }

        return target;
    }

    //---------------------------------------------------------------------------------------------
    QString
    parent_path(QString path)
    //---------------------------------------------------------------------------------------------
    {
        auto split = path.split('/');
        split.removeLast();
        return split.join('/');
    }

    //---------------------------------------------------------------------------------------------
    QJsonObject const
    query(QString uri)
    //---------------------------------------------------------------------------------------------
    {
        if (uri == "/")
            return m_root.to_json();

        auto node = find(uri);
        if (!node)
             return QJsonObject();
        else return node->to_json();
    }
};

#include <QAbstractItemModel>
#include <QHash>

//=================================================================================================
class TreeModel : public QAbstractItemModel
//=================================================================================================
{
    Q_OBJECT

    Node*
    m_root_node = nullptr;

public:

    //---------------------------------------------------------------------------------------------
    TreeModel() : QAbstractItemModel() {}

    //---------------------------------------------------------------------------------------------
    enum Roles
    //---------------------------------------------------------------------------------------------
    {
        NodeName    = Qt::UserRole+10,
        NodeType    = Qt::UserRole+11,
        NodeValue   = Qt::UserRole+12
    };

    //---------------------------------------------------------------------------------------------
    void
    set_root(Node* node)
    //---------------------------------------------------------------------------------------------
    {
        m_root_node = node;
    }

    //---------------------------------------------------------------------------------------------
    Q_INVOKABLE Node*
    get(QModelIndex const& index)
    //---------------------------------------------------------------------------------------------
    {
        return static_cast<Node*>(index.internalPointer());
    }

    //---------------------------------------------------------------------------------------------
    virtual QHash<int, QByteArray>
    roleNames() const override
    //---------------------------------------------------------------------------------------------
    {
        QHash<int, QByteArray> roles;

        roles[NodeName]  = "NodeName";
        roles[NodeValue] = "NodeValue";
        return roles;
    }

    //---------------------------------------------------------------------------------------------
    virtual int
    columnCount(const QModelIndex &parent = QModelIndex()) const override { return 2; }

    //---------------------------------------------------------------------------------------------
    virtual int
    rowCount(const QModelIndex &parent = QModelIndex()) const override
    //---------------------------------------------------------------------------------------------
    {
        Node* parent_node;

        if (!parent.isValid())
             parent_node = m_root_node;
        else parent_node = static_cast<Node*>(parent.internalPointer());

        return parent_node->nsubnodes();
    }

    //---------------------------------------------------------------------------------------------
    virtual QVariant
    data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    //---------------------------------------------------------------------------------------------
    {
        if (!index.isValid())
            return QVariant();

        auto node = static_cast<Node*>(index.internalPointer());

        if (role == NodeName)
            return node->name();
        else if (role == NodeValue)
            return node->value();
    }

    //---------------------------------------------------------------------------------------------
    virtual bool
    setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    //---------------------------------------------------------------------------------------------
    {
        if (!index.isValid())
            return false;

        auto node = static_cast<Node*>(index.internalPointer());

        if (role == NodeName)
            return false;
        else if (role == NodeValue)
            node->set_value(value);

        return true;
    }

    //---------------------------------------------------------------------------------------------
    virtual QModelIndex
    index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    //---------------------------------------------------------------------------------------------
    {
        Node* parent_node;

        if (!parent.isValid())
             parent_node = m_root_node;
        else parent_node = static_cast<Node*>(parent.internalPointer());

        auto child_node = parent_node->subnode(row);

        if (child_node)
             return createIndex(row, column, child_node);
        else return QModelIndex();
    }

    //---------------------------------------------------------------------------------------------
    virtual QModelIndex
    parent(const QModelIndex &child) const override
    //---------------------------------------------------------------------------------------------
    {
        if (!child.isValid())
            return QModelIndex();

        auto child_node = static_cast<Node*>(child.internalPointer());
        auto parent_node = child_node->parent_node();

        if (parent_node == m_root_node)
            return QModelIndex();

        return createIndex(parent_node->index(), 0, parent_node);
    }

    //---------------------------------------------------------------------------------------------
    Qt::ItemFlags
    flags(QModelIndex const& index) const override
    //---------------------------------------------------------------------------------------------
    {
        if (!index.isValid())
             return 0;
        else return QAbstractItemModel::flags(index);
    }

    //---------------------------------------------------------------------------------------------
    QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    //---------------------------------------------------------------------------------------------
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
//            return m_root_node->description();
            return QVariant();

        return QVariant();
    }
};
