#ifndef NODETREE_HPP
#define NODETREE_HPP

#include <QAbstractItemModel>

class WPNNode;

class WPNNodeTree : public QAbstractItemModel
{
    Q_OBJECT

    public:
    WPNNodeTree();
    WPNNodeTree(WPNNode* node);

    enum Roles
    {
        NodeName    = Qt::UserRole+10,
        NodeType    = Qt::UserRole+11,
        NodeValue   = Qt::UserRole+12
    };

    void setRoot(WPNNode* node);

    Q_INVOKABLE WPNNode* get(const QModelIndex& index);

    virtual QHash<int, QByteArray> roleNames() const override;

    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex& index) const;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    private:
    WPNNode* m_root_node;

};

#endif // NODETREE_HPP
