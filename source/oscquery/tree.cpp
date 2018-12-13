#include "tree.hpp"
#include "node.hpp"
#include <QtDebug>

WPNNodeTree::WPNNodeTree() : m_root_node(nullptr),
    QAbstractItemModel()
{

}

WPNNodeTree::WPNNodeTree(WPNNode* root) : m_root_node(root)
{

}

void WPNNodeTree::setRoot(WPNNode *node)
{
    m_root_node = node;
}

QHash<int, QByteArray> WPNNodeTree::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[NodeName]   = "NodeName";
    roles[NodeValue]  = "NodeValue";

    return roles;

}

int WPNNodeTree::columnCount(const QModelIndex& parent) const
{
    return 2;
}

int WPNNodeTree::rowCount(const QModelIndex& parent) const
{
    WPNNode* parent_node;

    if ( !parent.isValid() )
         parent_node = m_root_node;
    else parent_node = static_cast<WPNNode*>(parent.internalPointer());

    return parent_node->nsubnodes();
}

QVariant WPNNodeTree::data(const QModelIndex& index, int role) const
{
    if ( !index.isValid() )
         return QVariant();

    auto node = static_cast<WPNNode*>(index.internalPointer());

    if ( role == NodeName )
         return node->name();

    else if ( role == NodeValue )
        return node->value();

}

bool WPNNodeTree::setData(const QModelIndex& index, const QVariant& value, int role )
{
    if ( !index.isValid() )
        return false;

    auto node = static_cast<WPNNode*>(index.internalPointer());
    if ( role == NodeName ) return false;

    else if ( role == NodeValue ) node->setValue(value);
    return true;
}

QModelIndex WPNNodeTree::index(int row, int column, const QModelIndex& parent) const
{
    WPNNode* parent_node;

    if ( !parent.isValid() )
         parent_node = m_root_node;
    else parent_node = static_cast<WPNNode*>(parent.internalPointer());

    auto child_node = parent_node->subnodeAt(row);

    if ( child_node ) return createIndex(row, column, child_node);
    else return QModelIndex();
}

QModelIndex WPNNodeTree::parent(const QModelIndex& index) const
{    
    if ( !index.isValid() )
         return QModelIndex();

    auto child_node = static_cast<WPNNode*>(index.internalPointer());
    auto parent_node = child_node->parent();

    if ( parent_node == m_root_node )
         return QModelIndex();

    return createIndex(parent_node->index(), 0, parent_node);

}

Qt::ItemFlags WPNNodeTree::flags(const QModelIndex& index) const
{
    if ( !index.isValid() ) return 0;
    return QAbstractItemModel::flags(index);

}

QVariant WPNNodeTree::headerData(int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole )
         return m_root_node->description();

    return QVariant();
}

WPNNode* WPNNodeTree::get(const QModelIndex& index)
{
    return static_cast<WPNNode*>(index.internalPointer());
}
