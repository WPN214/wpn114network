#include "directory.hpp"

void
WPN114::Network::Directory::
componentComplete()
{
    Node::componentComplete();

    QDir directory(m_directory_path);
    directory.setNameFilters(m_filters);
    parse_target(directory);
}

void
WPN114::Network::Directory::
parse_target(QDir directory)
{
    set_value(directory.entryList());

    for (const auto& file : directory.entryList()) {
        qDebug() << file;
        auto node = new File(*this, file);
        node->set_file(directory.path().append("/").append(file));
        add_subnode(node);
        node->componentComplete();
    }

    if (m_recursive)
    {
        directory.setNameFilters(QStringList{});
        directory.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

        for (const auto& subdirectory : directory.entryList()) {
            qDebug() << subdirectory;
            auto node = new Directory(*this, subdirectory);
            node->set_recursive(true);
            node->set_filters(m_filters);
            node->set_target(directory.path().append("/").append(subdirectory));
            add_subnode(node);
            node->componentComplete();
        }
    }
}

void
WPN114::Network::MirrorDirectory::
set_destination(QString destination)
{
    m_destination = destination;
    m_absolute_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                     .append(destination);

    if (!QDir(m_absolute_path).exists())
        QDir().mkpath(m_absolute_path);

}

QUrl
WPN114::Network::MirrorDirectory::
to_url(QString file)
{
    file.prepend("/").prepend(m_destination).prepend(m_host);
    return QUrl(file);

}

void
WPN114::Network::MirrorDirectory::
on_file_list_changed(QVariant vlist)
{
    if (m_destination.isNull())
        set_destination(m_path);

    for (const auto& file : vlist.toList())
         m_downloads.append(file.toString());

    for (const auto& file : m_downloads)
         m_queue.enqueue(to_url(file));

    next();
}

void
WPN114::Network::MirrorDirectory::next()
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

void
WPN114::Network::MirrorDirectory::on_ready_read()
{
    auto read = m_current_download->readAll();
    m_output.write(read);
}

void
WPN114::Network::MirrorDirectory::on_complete()
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
