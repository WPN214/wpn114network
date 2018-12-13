#include "folder.hpp"
#include "file.hpp"
#include <QtDebug>
#include <QStandardPaths>
#include <source/oscquery/client.hpp>
#include <QJsonArray>

WPNFolderNode::WPNFolderNode() : m_recursive(true), WPNNode()
{        
    m_attributes.type           = Type::List;
    m_attributes.access         = Access::READ;
    m_attributes.extended_type  = "folder";
}

void WPNFolderNode::parseDirectory(QDir dir)
{
    setValue(dir.entryList());

    for ( const auto& file : dir.entryList() )
    {
        WPNFileNode* node = new WPNFileNode;

        node->setName           ( file );
        node->setFilePath       ( dir.path()+"/"+file );
        node->setExtendedType   ( "file");
        addSubnode              ( node );
    }

    if ( m_recursive )
    {
        dir.setNameFilters  ( QStringList{} );
        dir.setFilter       ( QDir::Dirs | QDir::NoDotAndDotDot );

        for ( const auto& d : dir.entryList() )
        {
            WPNFolderNode* folder = new WPNFolderNode;

            folder->setName         ( d );
            folder->setRecursive    ( true );
            folder->setExtendedType ( "folder" );
            folder->setFilters      ( m_filters );
            folder->setFolderPath   ( dir.path()+"/"+d );
            addSubnode              ( folder );

            folder->componentComplete ( );
        }
    }
}

void WPNFolderNode::setFolderPath(QString path)
{
    m_folder_path = path;
}

void WPNFolderNode::componentComplete()
{
    WPNNode::componentComplete();

    QDir dir            ( m_folder_path );
    dir.setNameFilters  ( m_filters );
    parseDirectory      ( dir );
}

void WPNFolderNode::setExtensions(QString ext)
{
    m_extensions = ext;
}

void WPNFolderNode::setRecursive(bool rec)
{
    m_recursive = rec;
}

void WPNFolderNode::setFilters(QStringList filters)
{
    m_filters = filters;
}

//---------------------------------------------------------------------------------------------------------

WPNFolderMirror::WPNFolderMirror() : WPNNode(), m_recursive(true), m_current_download(nullptr)
{
    // whenever value is received, update downloads
    QObject::connect(this, SIGNAL(valueReceived(QVariant)), this, SLOT(onFileListChanged(QVariant)));
}

WPNFolderMirror::~WPNFolderMirror()
{
    for ( const auto& child : m_children_folders )
        delete child;
}

void WPNFolderMirror::setDestination(QString destination)
{
    m_destination = destination;
    m_abs_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).append(destination);
    if ( !QDir(m_abs_path).exists()) QDir().mkpath(m_abs_path);
}

QUrl WPNFolderMirror::toUrl(QString file)
{
    WPNQueryClient* device = qobject_cast<WPNQueryClient*>(m_device);
    QString root = device->hostUrl();
    root.append(file.prepend(m_destination+"/"));

    return QUrl(root);
}

void WPNFolderMirror::onFileListChanged(QVariant list)
{
    if ( m_destination.isEmpty() ) setDestination(m_attributes.path);

    for ( const auto& file : list.toList())
        m_downloads << file.toString();

    qDebug() << "[FOLDER-MIRROR] Setting up downloads: " << m_downloads;

    for ( const auto& file : m_downloads )
        m_queue.enqueue(toUrl(file));

    next();
}

void WPNFolderMirror::next()
{
    if ( m_queue.isEmpty() )
    {
        emit mirrorComplete();
        return;
    }

    QString path = m_downloads.first().prepend("/").prepend(m_abs_path);
    m_output.setFileName(path);

    if ( !m_output.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text ))
    {
        qDebug() << "error opening file for writing";
        qDebug() << path;
        qDebug() << m_output.errorString();
        return;
    }

    qDebug() << "[FOLDER-MIRROR] Donwloading file:" << m_downloads.first();

    QUrl url = m_queue.dequeue();
    QNetworkRequest req(url);

    m_current_download = m_netaccess.get(req);

    QObject::connect(m_current_download, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    QObject::connect(m_current_download, SIGNAL(finished()), this, SLOT(onComplete()));
}

void WPNFolderMirror::onReadyRead()
{
    auto read = m_current_download->readAll();
    m_output.write(read);
}

void WPNFolderMirror::onComplete()
{
    m_output.close();
    QObject::disconnect(m_current_download, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    QObject::disconnect(m_current_download, SIGNAL(finished()), this, SLOT(onComplete()));

    if  ( m_current_download->error() )
         qDebug() << "[FOLDER-MIRROR] Error: " << m_current_download->errorString();
    else qDebug() << "[FOLDER-MIRROR] File download complete:" << m_downloads.first();

    m_current_download->deleteLater();
    m_downloads.removeFirst();

    next();
}
