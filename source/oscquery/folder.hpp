#pragma once

#include <QObject>
#include "node.hpp"
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QQmlParserStatus>

class WPNFolderNode : public WPNNode
{
    Q_OBJECT
    Q_PROPERTY  ( QString folderPath READ folderPath WRITE setFolderPath )
    Q_PROPERTY  ( QString extensions READ extensions WRITE setExtensions )
    Q_PROPERTY  ( QStringList filters READ filters WRITE setFilters )
    Q_PROPERTY  ( bool recursive READ recursive WRITE setRecursive )

    public:
    WPNFolderNode();

    virtual void componentComplete() override;
    QString folderPath() const { return m_folder_path; }
    QString extensions() const { return m_extensions; }
    bool recursive() const { return m_recursive; }
    QStringList filters() const { return m_filters; }

    void setFolderPath(QString path);
    void setExtensions(QString ext);
    void setRecursive(bool rec);
    void setFilters(QStringList filters);

    private:    
    void parseDirectory(QDir dir);
    QStringList m_filters;
    bool m_recursive;
    QString m_folder_path;
    QString m_extensions;

};

class WPNFolderMirror : public WPNNode
{
    Q_OBJECT
    Q_PROPERTY  ( bool recursive READ recursive WRITE setRecursive )
    Q_PROPERTY  ( QString destination READ destination WRITE setDestination )

    public:
    WPNFolderMirror();
    ~WPNFolderMirror();

    bool recursive() const { return m_recursive; }
    QString destination() const { return m_destination; }

    void setRecursive(bool recursive) { m_recursive = recursive; }
    void setDestination(QString destination);

    protected slots:
    QUrl toUrl(QString file);
    void onFileListChanged(QVariant list);
    void next();
    void onReadyRead();
    void onComplete();

    signals:
    void mirrorComplete();

    private:
    bool m_recursive;
    QString m_abs_path;
    QString m_destination;
    QStringList m_downloads;
    QVector<WPNFolderMirror*> m_children_folders;

    QNetworkAccessManager m_netaccess;
    QQueue<QUrl> m_queue;
    QFile m_output;
    QNetworkReply* m_current_download;
};
