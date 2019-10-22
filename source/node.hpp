#pragma once

#include <QObject>
#include <QQmlListProperty>
#include <QQmlPropertyValueSource>
#include <QQmlParserStatus>
#include <QQmlProperty>
#include <QQmlEngine>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>

namespace WPN114   {
namespace Network  {

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

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (Tree* tree READ tree WRITE set_tree)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QQmlListProperty<Node> subnodes READ subnodes_list)

    //---------------------------------------------------------------------------------------------
    Q_CLASSINFO ("DefaultProperty", "subnodes")

public:

    Node() {}

    Node(Node& parent, QString name, Type::Values type);

    Node(QJsonObject const& object);

    //---------------------------------------------------------------------------------------------
    virtual
    ~Node() override;

    //---------------------------------------------------------------------------------------------
    virtual void
    classBegin() override {}

    virtual void
    componentComplete() override;

    //---------------------------------------------------------------------------------------------
    virtual void
    setTarget(const QQmlProperty& target) override;

    //---------------------------------------------------------------------------------------------
    Q_SLOT void
    on_property_changed();

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

    bool
    zombie() const { return m_zombie; }

    //---------------------------------------------------------------------------------------------
    void
    set_zombie(bool zombie)
    //---------------------------------------------------------------------------------------------
    {
        m_zombie = zombie;
    }

    //---------------------------------------------------------------------------------------------
    int
    index() const;

    //---------------------------------------------------------------------------------------------
    QString
    typetag() const;

    //---------------------------------------------------------------------------------------------
    QJsonValue
    value_json() const;

    //---------------------------------------------------------------------------------------------
    void
    set_value(QVariant value);

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
    void
    set_type(Type::Values type) { m_type = type; }

    void
    set_type(QString const type);

    void
    set_type(QMetaType::Type type);

    //---------------------------------------------------------------------------------------------
    QVector<Node*>&
    subnodes() { return m_subnodes; }

    Node*
    create_subnode(QString name);

    void
    add_subnode(Node* subnode);

    void
    remove_subnode(Node* subnode);

    Node*
    subnode(QString name);

    Node*
    subnode(size_t index);

    //---------------------------------------------------------------------------------------------
    QJsonObject
    attributes() const;

    operator
    QJsonObject() const;

    void
    update(QJsonObject object);

    // --------------------------------------------------------------------------------------------

    QQmlListProperty<Node>
    subnodes_list()
    // returns subnodes (QML format)
    {
        return QQmlListProperty<Node>(
               this, this,
               &Node::append_subnode,
               &Node::nsubnodes,
               &Node::subnode,
               &Node::clear_subnodes);
    }

    Q_INVOKABLE void
    append_subnode(Node* node);
    // appends a subnode to this Node children

    Q_INVOKABLE int
    nsubnodes() { return m_subnodes.count(); }
    // returns this Node' subnodes count

    Q_INVOKABLE Node*
    subnode(int index) { return m_subnodes.at(index); }
    // returns this Node' subnode at index

    Q_INVOKABLE void
    clear_subnodes() {}

    // --------------------------------------------------------------------------------------------
    static void
    append_subnode(QQmlListProperty<Node>* list, Node* node)
    // static Qml version, see above
    {
        reinterpret_cast<Node*>(list->data)->append_subnode(node);
    }

    // --------------------------------------------------------------------------------------------
    static int
    nsubnodes(QQmlListProperty<Node>* list)
    // static Qml version, see above
    {
        return reinterpret_cast<Node*>(list)->nsubnodes();
    }

    // --------------------------------------------------------------------------------------------
    static Node*
    subnode(QQmlListProperty<Node>* list, int index)
    // static Qml version, see above
    {
        return reinterpret_cast<Node*>(list)->subnode(index);
    }

    // --------------------------------------------------------------------------------------------
    static void
    clear_subnodes(QQmlListProperty<Node>* list)
    // static Qml version, see above
    {
        reinterpret_cast<Node*>(list)->clear_subnodes();
    }

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
    m_critical = false,
    m_zombie = false;

    QQmlProperty
    m_target;
};


} // namespace Network
} // namespace WPN114
