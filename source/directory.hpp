#pragma once

#include "file.hpp"

namespace WPN114  {
namespace Network {

//=================================================================================================
class Directory : public Node
//=================================================================================================
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QString target READ target WRITE set_target)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QString extensions READ extensions WRITE set_extensions)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QStringList filters READ filters WRITE set_filters)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (bool recursive READ recursive WRITE set_recursive)

public:

    Directory() {
        m_type = Type::List;
        m_extended_type = "directory";
    }

    Directory(Node& parent, QString name) :
        Node(parent, name, Type::List) { m_extended_type = "directory"; }

    //---------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override;

    //---------------------------------------------------------------------------------------------
    QString
    target()        const { return m_directory_path; }

    QString
    extensions()    const { return m_extensions; }

    QStringList
    filters()       const { return m_filters; }

    bool
    recursive()     const { return m_recursive; }

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

private:

    //---------------------------------------------------------------------------------------------
    void
    parse_target(QDir directory);

    //---------------------------------------------------------------------------------------------

    QStringList
    m_filters;

    bool
    m_recursive;

    QString
    m_directory_path,
    m_extensions;
};

//=================================================================================================
class MirrorDirectory : public Node
//=================================================================================================
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (bool recursive READ recursive WRITE set_recursive)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QString destination READ destination WRITE set_destination)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QString host READ host WRITE set_host)

public:

    //---------------------------------------------------------------------------------------------
    MirrorDirectory()
    //---------------------------------------------------------------------------------------------
    {
        // update downloads whenever node value is changed
        QObject::connect(this, &Node::valueChanged,
                         this, &MirrorDirectory::on_file_list_changed);
    }

    Q_SIGNAL void
    complete();

    //---------------------------------------------------------------------------------------------
    bool
    recursive()     const { return m_recursive; }

    QString
    destination()   const { return m_destination; }

    QString
    host()          const { return m_host; }

    //---------------------------------------------------------------------------------------------
    void
    set_recursive(bool recursive)
    //---------------------------------------------------------------------------------------------
    {
        m_recursive = recursive;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_destination(QString destination);

    //---------------------------------------------------------------------------------------------
    void
    set_host(QString host)
    //---------------------------------------------------------------------------------------------
    {
        m_host = host;
    }


protected slots:

    //---------------------------------------------------------------------------------------------
    QUrl
    to_url(QString file);

    //---------------------------------------------------------------------------------------------
    void
    on_file_list_changed(QVariant vlist);

    //---------------------------------------------------------------------------------------------
    void
    on_ready_read();

    //---------------------------------------------------------------------------------------------
    void
    on_complete();

    //---------------------------------------------------------------------------------------------
    void
    next();

private:

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
};
}
}
