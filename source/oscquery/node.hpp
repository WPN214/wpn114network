#pragma once

#include <QQmlParserStatus>
#include <QQmlProperty>
#include <QQmlPropertyValueSource>
#include <QJsonObject>
#include <QVariant>
#include <QVector>
#include "device.hpp"

class WPNDevice;

class Type : public QObject
{
    Q_OBJECT
    public:
    enum Values
    {
        None        = 0,
        Bool        = 1,
        Int         = 2,
        Float       = 3,
        String      = 4,
        List        = 5,
        Vec2f       = 6,
        Vec3f       = 7,
        Vec4f       = 8,
        Char        = 9,
        Impulse     = 10,
        File        = 11
    };

    Q_ENUM ( Values )
};

class Access : public QObject
{
    Q_OBJECT
    public:
    enum Values
    {
        NONE    = 0,
        READ    = 1,
        WRITE   = 2,
        RW      = 3
    };

    Q_ENUM ( Values )
};

class Clipmode : public QObject
{
    Q_OBJECT
    public:
    enum Values
    {
        NONE    = 0,
        LOW     = 1,
        HIGH    = 2,
        BOTH    = 3,
    };

    Q_ENUM ( Values )
};


struct Range
{
    public:
    QVariant min;
    QVariant max;
    QVariantList vals;
};

struct Attributes
{
    QString             path;
    Type::Values        type;
    Access::Values      access;
    QVariant            value;
    QVariant            default_value;
    Range               range;
    QString             description;
    QStringList         tags;
    bool                critical;
    Clipmode::Values    clipmode;
    QString             extended_type;
};

class WPNNode;

struct WPNNodePointer
{
    QString     path;
    WPNNode*    ptr;
};

class WPNNode : public QObject, public QQmlParserStatus, public QQmlPropertyValueSource
{
    Q_OBJECT

    Q_INTERFACES    ( QQmlPropertyValueSource )

    Q_PROPERTY      ( QString name READ name WRITE setName NOTIFY nameChanged )
    Q_PROPERTY      ( WPNDevice* device READ device WRITE setDevice )
    Q_PROPERTY      ( WPNNode* parent READ parent WRITE setParent )

    Q_PROPERTY      ( QString path READ path WRITE setPath )
    Q_PROPERTY      ( Type::Values type READ type WRITE setType )
    Q_PROPERTY      ( Access::Values access READ access WRITE setAccess )
    Q_PROPERTY      ( QVariant value READ value WRITE setValue NOTIFY valueChanged )
    Q_PROPERTY      ( QVariant defaultValue READ defaultValue WRITE setDefaultValue )
    Q_PROPERTY      ( Range range READ range WRITE setRange )
    Q_PROPERTY      ( QString description READ description WRITE setDescription )
    Q_PROPERTY      ( QStringList tags READ tags WRITE setTags )
    Q_PROPERTY      ( bool critical READ critical WRITE setCritical )
    Q_PROPERTY      ( Clipmode::Values clipmode READ clipmode WRITE setClipmode )

    public:
    WPNNode();
    WPNNode(const WPNNode&) {}

    virtual ~WPNNode();

    static WPNNode* fromJson(QJsonObject, WPNDevice* dev);
    static QString parentPath(QString path);

    virtual void componentComplete  ( );
    virtual void classBegin         ( );

    virtual void setTarget(const QQmlProperty& property);    
    void setTarget(QObject* target, const QMetaProperty& property);

    qint16 index( ) const;

    QString name            ( ) const { return m_name; }
    WPNDevice* device       ( ) const { return m_device; }
    WPNNode* parent         ( ) { return m_parent; }
    void post               ( ) const;
    bool qml                ( ) const { return m_qml; }

    QString parentPath ( ) const;
    void setName    ( QString name );
    void setDevice  ( WPNDevice* device );
    void setParent  ( WPNNode* parent );

    // attributes & json -------------------------------------------------

    QJsonValue attributeValue     ( QString attr ) const;
    QJsonObject attributeJson     ( QString attr ) const;
    QJsonObject attributesJson    ( ) const;
    const Attributes& attributes  ( ) const;
    QJsonValue jsonValue          ( ) const;
    QString typeTag               ( ) const;
    QJsonObject toJson            ( ) const;

    // attributes --------------------------------------------------------

    QString path                ( ) const { return m_attributes.path; }
    Type::Values type           ( ) const { return m_attributes.type; }
    Access::Values access       ( ) const { return m_attributes.access; }
    QVariant value              ( ) const { return m_attributes.value; }
    Range range                 ( ) const { return m_attributes.range; }
    QString description         ( ) const { return m_attributes.description; }
    QStringList tags            ( ) const { return m_attributes.tags; }
    bool critical               ( ) const { return m_attributes.critical; }
    Clipmode::Values clipmode   ( ) const { return m_attributes.clipmode; }
    QString extended_type       ( ) const { return m_attributes.extended_type; }
    QVariant defaultValue       ( ) const { return m_attributes.default_value; }

    Q_INVOKABLE void resetValue  ( bool recursive = false );
    Q_INVOKABLE void setValue    ( QVariant value );   

    void setPath            ( QString path );
    void setType            ( Type::Values type );
    void setAccess          ( Access::Values access );    
    void setValueQuiet      ( QVariant value );
    void setRange           ( Range range );
    void setDescription     ( QString description );
    void setTags            ( QStringList tags );
    void setCritical        ( bool critical );    
    void setClipmode        ( Clipmode::Values clipmode );
    void setExtendedType    ( QString extended_type );
    void setDefaultValue    ( QVariant value );

    void setTypeFromTag         ( QString tag );
    void setTypeFromMetaType    ( int type );

    // tree/hierarchy ----------------------------------------------------

    void collect( QString pattern, QVector<WPNNode*>& rec );

    Q_INVOKABLE QVariantList collect   ( QString name );
    Q_INVOKABLE WPNNode* subnode       ( QString path );
    Q_INVOKABLE WPNNode* subnodeAt     ( int index );

    Q_INVOKABLE quint16 nsubnodes    ( ) const { return m_children.size(); }
    QVector<WPNNodePointer> subnodes ( ) const { return m_children; }

    WPNNode* createSubnode  ( QString name );
    bool hasSubnode         ( WPNNode* node );
    void addSubnode         ( WPNNode* node );
    void removeSubnode      ( QString path );
    void update             ( QJsonObject obj );

    bool listening          ( ) const { return m_listening; }
    void setListening       ( bool listen, WPNDevice* target, bool recursive = false );

    signals:
    void nodeRemoved        ( QString path );
    void nameChanged        ( );
    void valueChanged       ( QVariant newValue );
    void valueReceived      ( QVariant newValue );

    public slots:
    void propertyChanged      ( );
    void metaPropertyChanged  ( );

    protected:            
    Attributes      m_attributes;
    QString         m_name;
    WPNDevice*      m_device;
    WPNNode*        m_parent;

    QQmlProperty    m_target_property;
    QMetaProperty   m_meta_property;
    QObject*        m_target;

    bool  m_qml = false;
    bool  m_listening;

    QVector<WPNNodePointer>  m_children;
    QVector<WPNDevice*>      m_listeners;
};

Q_DECLARE_METATYPE( WPNNode )
