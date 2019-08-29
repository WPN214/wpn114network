#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QQmlPropertyValueSource>
#include <QQmlParserStatus>
#include <QQmlProperty>
#include <QQmlListProperty>
#include <QVariant>
#include <QJsonObject>
#include <QJsonArray>
#include <QtDebug>
#include <QFile>
#include <QAbstractItemModel>
#include <QHash>

#include "node.hpp"

namespace WPN114  {
namespace Network {

class Tree;

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
    TreeModel(Node& root, QObject* parent = nullptr) :
        m_root_node(&root), QAbstractItemModel()
    {
        setParent(parent);
    }

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
    rowCount(const QModelIndex &parent = QModelIndex()) const override;

    //---------------------------------------------------------------------------------------------
    virtual QVariant
    data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    //---------------------------------------------------------------------------------------------
    virtual bool
    setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    //---------------------------------------------------------------------------------------------
    virtual QModelIndex
    index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    //---------------------------------------------------------------------------------------------
    virtual QModelIndex
    parent(const QModelIndex &child) const override;

    //---------------------------------------------------------------------------------------------
    Qt::ItemFlags
    flags(QModelIndex const& index) const override;

    //---------------------------------------------------------------------------------------------
    QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

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
    link(Node* node);


    //---------------------------------------------------------------------------------------------
    Node*
    find(QString path);


    //---------------------------------------------------------------------------------------------
    Node*
    find_or_create(QString path);

    //---------------------------------------------------------------------------------------------
    QString
    parent_path(QString path);

    //---------------------------------------------------------------------------------------------
    QJsonObject const
    query(QString const& uri);

    //---------------------------------------------------------------------------------------------
    Q_INVOKABLE TreeModel*
    model()
    //---------------------------------------------------------------------------------------------
    {
        return new TreeModel(m_root, this);
    }

    //---------------------------------------------------------------------------------------------
    QVariant
    value(QString const& uri)
    //---------------------------------------------------------------------------------------------
    {
        if  (auto node = find(uri))
             return node->value();
        else return QVariant();
    }
};

}
}
