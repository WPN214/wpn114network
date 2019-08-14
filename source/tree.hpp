#pragma once

#include <QObject>
#include <QQmlPropertyValueSource>
#include <QQmlParserStatus>
#include <QVariant>
#include <QQmlEngine>
#include <QJsonObject>

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
        Float       = 38,
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

    QString
    m_name,
    m_path;

    Type::Values
    m_type;

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

public:

    //---------------------------------------------------------------------------------------------
    Node()
    //---------------------------------------------------------------------------------------------
    {

    }

    //---------------------------------------------------------------------------------------------
    Node(Node& parent, QString name, Type::Values type) :
      m_type            (type),
      m_name            (name),
      m_parent_node     (&parent),
      m_tree            (parent.tree()),
      m_path            (parent.path().append('/').append(name))
    //---------------------------------------------------------------------------------------------
    {
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
    QJsonObject const
    to_json(bool recursive = true)
    //---------------------------------------------------------------------------------------------
    {
        return QJsonObject();
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
    {
        m_root.set_path("/");
        m_root.set_tree(this);
    }

    //---------------------------------------------------------------------------------------------
    static Tree*
    instance() { return s_singleton; }

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
        auto parent = find_or_create(parent_path(node->path()));
        parent->add_subnode(node);
    }

    //---------------------------------------------------------------------------------------------
    Node*
    find(QString path)
    //---------------------------------------------------------------------------------------------
    {
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
        auto node = find(uri);
        if (!node)
            return QJsonObject();
        return node->to_json(true);
    }
};
