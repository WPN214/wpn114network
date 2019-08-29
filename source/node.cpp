#include "node.hpp"
#include "tree.hpp"

using namespace WPN114::Network;

WPN114::Network::Node::Node(Node& parent, QString name, Type::Values type) :
    m_type          (type),
    m_name          (name),
    m_parent_node   (&parent),
    m_tree          (parent.tree())
{
    if (parent.path() == "/")
         m_path = parent.path().append(name);
    else m_path = parent.path().append('/').append(name);

    // prevents qml from destroying nodes when referenced in javascript functions
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

WPN114::Network::Node::
Node(QJsonObject const& object)
{
    update(object);
}

WPN114::Network::Node::
~Node()
{
    if (!m_zombie)
        for (auto& subnode : m_subnodes)
             delete subnode;
}

void
WPN114::Network::Node::
componentComplete()
{
    if (m_name.isNull())
        m_name = m_path.split('/').last();

    if (m_tree == nullptr)
        m_tree = WPN114::Network::Tree::instance();

    assert(m_tree);
    m_tree->link(this);
}

void
WPN114::Network::Node::
setTarget(QQmlProperty const& target)
// bind node to qml property, taking its type and value
{
    m_target    = target;
    m_value     = target.read();

    set_type(static_cast<QMetaType::Type>(target.propertyType()));
    m_target.connectNotifySignal(this, SLOT(on_property_changed()));
}

void
WPN114::Network::Node::
on_property_changed()
{
    auto value = m_target.read();
    valueReceived(value);

    if (value != m_value) {
        valueChanged(value);
        m_value = value;
    }
}

QString
WPN114::Network::Node::
typetag() const
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

QJsonValue
WPN114::Network::Node::
value_json() const
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

void
WPN114::Network::Node::
set_type(QString const type)
// sets node value type (OSCtypetag variant)
{
    if (type == "f")            m_type = Type::Float;
    else if (type == "T" ||
             type == "F")       m_type = Type::Bool;
    else if (type == "i")       m_type = Type::Int;
    else if (type == "I")       m_type = Type::Impulse;
    else if (type == "s")       m_type = Type::String;
    else if (type == "ff")      m_type = Type::Vec2f;
    else if (type == "fff")     m_type = Type::Vec3f;
    else if (type == "ffff")    m_type = Type::Vec4f;
    else if (type == "N")       m_type = Type::Impulse;
    else                        m_type = Type::None;
}

void
WPN114::Network::Node::
set_type(QMetaType::Type type)
// sets node value type (QMetaType variant)
{
    m_type = static_cast<Type::Values>(type);
}

void
WPN114::Network::Node::
set_value(QVariant value)
{
    emit valueReceived(value);

    if (value != m_value) {
        emit valueChanged(value);
        m_value = value;
    }
}

//-------------------------------------------------------------------------------------------------
// TREE-STRUCTURE
//-------------------------------------------------------------------------------------------------

int
WPN114::Network::Node::
index() const
{
    for (int n = 0; n < m_parent_node->nsubnodes(); ++n)
        if (m_parent_node->subnode(n) == this)
            return n;
    return -1;
}

Node*
WPN114::Network::Node::
create_subnode(QString name)
// instantiates child node with name 'name' and type 'none'
{
    m_subnodes.push_back(new Node(*this, name, Type::None));
    return m_subnodes.back();
}

void
WPN114::Network::Node::
add_subnode(Node* subnode)
// add subnode as child
{
    subnode->set_parent_node(this);
    m_subnodes.push_back(subnode);
}

void
WPN114::Network::Node::
remove_subnode(Node* subnode)
// remove subnode from children
// marking it 'zombie', meaning it shouldn't delete its own children when destroyed

{
    m_subnodes.removeOne(subnode);
    subnode->set_zombie(true);
}

Node*
WPN114::Network::Node::
subnode(QString name)
// retrieve subnode from name, returning nullptr if not found
{
    for (const auto& subnode : m_subnodes)
         if (subnode->name() == name)
             return subnode;
    return nullptr;
}

Node*
WPN114::Network::Node::
subnode(size_t index)
// retrieve subnode from index, returning nullptr if out of bounds
{
    if  (index < m_subnodes.count())
         return m_subnodes[index];
    else return nullptr;
}

//-------------------------------------------------------------------------------------------------
// JSON
//-------------------------------------------------------------------------------------------------

static const char*
wpn_json_contents   = "CONTENTS";

static const char*
wpn_json_type       = "TYPE";

static const char*
wpn_json_critical   = "CRITICAL";

static const char*
wpn_json_value      = "VALUE";

static const char*
wpn_json_fullpath   = "FULL_PATH";

static const char*
wpn_json_exttype    = "EXTENDED_TYPE";


QJsonObject
WPN114::Network::Node::
attributes() const
// get Node's current attribute values, JSON-formatted
{
    QJsonObject attributes;
    attributes[wpn_json_fullpath] = m_path;

    if (m_type == Type::None)
        return attributes;

    attributes[wpn_json_type]      = typetag();
    attributes[wpn_json_critical]  = m_critical;
    attributes[wpn_json_value]     = value_json();

    if (!m_extended_type.isNull())
        attributes[wpn_json_exttype] = m_extended_type;

    return attributes;
}

WPN114::Network::Node::
operator QJsonObject() const
// get Node's current attribute values and contents recursively, JSON-formatted
{
    QJsonObject contents;
    for (auto& subnode : m_subnodes)
        contents.insert(subnode->name(), static_cast<QJsonObject>(*subnode));

    auto attr = attributes();
    attr[wpn_json_contents] = contents;
    return attr;
}

void
WPN114::Network::Node::
update(QJsonObject object)
// update Node's attribute values and contents recursively from JSON object
{
    if (object.contains(wpn_json_fullpath))
        m_path = object[wpn_json_fullpath].toString();

    if (object.contains(wpn_json_type))
        set_type(object[wpn_json_type].toString());

    if (object.contains(wpn_json_contents))
    {
        // recursively parse and build children nodes
        auto contents = object[wpn_json_contents].toObject();
        for (const auto& key : contents.keys()) {
            auto node = new Node(contents[key].toObject());
            node->set_name(key);
            add_subnode(node);
        }
    }

    if (object.contains(wpn_json_value))
        set_value(object[wpn_json_value].toVariant());
}

void
WPN114::Network::Node::
append_subnode(Node* node) { m_tree->link(node); }
