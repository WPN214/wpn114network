#include "tree.hpp"

using namespace WPN114::Network;

int WPN114::Network::TreeModel::
rowCount(const QModelIndex &parent) const
{
    Node* parent_node;

    if (!parent.isValid())
         parent_node = m_root_node;
    else parent_node = static_cast<Node*>(parent.internalPointer());

    return parent_node->nsubnodes();
}

QVariant
WPN114::Network::TreeModel::
data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto node = static_cast<Node*>(index.internalPointer());

    if (role == NodeName)
        return node->name();
    else if (role == NodeValue)
        return node->value();

    return QVariant();
}

bool
WPN114::Network::TreeModel::
setData(const QModelIndex &index, const QVariant &value, int role)
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

QModelIndex
WPN114::Network::TreeModel::
index(int row, int column, const QModelIndex &parent) const
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

QModelIndex
WPN114::Network::TreeModel::
parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    auto child_node = static_cast<Node*>(child.internalPointer());
    auto parent_node = child_node->parent_node();

    if (parent_node == m_root_node)
        return QModelIndex();

    return createIndex(parent_node->index(), 0, parent_node);
}

Qt::ItemFlags
WPN114::Network::TreeModel::
flags(const QModelIndex &index) const
{
    if (!index.isValid())
         return 0;
    else return QAbstractItemModel::flags(index);
}

QVariant
WPN114::Network::TreeModel::
headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
//            return m_root_node->description();
        return QVariant();

    return QVariant();
}

//-------------------------------------------------------------------------------------------------

void
WPN114::Network::Tree::
link(Node* node)
{
    // if node already exists, replace it
    if (auto dup = find(node->path()))
    {
        auto parent = dup->parent_node();

        for (auto subnode : dup->subnodes())
             node->add_subnode(subnode);

        parent->remove_subnode(dup);
        parent->add_subnode(node);
        delete dup;
        return;
    }

    auto parent = find_or_create(parent_path(node->path()));
    parent->add_subnode(node);
}


Node*
WPN114::Network::Tree::
find(QString path)
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

Node*
WPN114::Network::Tree::
find_or_create(QString path)
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

QString
WPN114::Network::Tree::
parent_path(QString path)
{
    auto split = path.split('/');
    split.removeLast();
    return split.join('/');
}

QJsonObject const
WPN114::Network::Tree::
query(QString const& uri)
{
    if (uri == "/")
        return static_cast<QJsonObject>(m_root);

    auto node = find(uri);
    if (!node)
         return QJsonObject();
    else return static_cast<QJsonObject>(*node);
}
