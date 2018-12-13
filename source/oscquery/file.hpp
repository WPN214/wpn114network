#pragma once

#include "node.hpp"
#include <QFile>
#include <QImage>

class WPNFileNode : public WPNNode
{
    Q_OBJECT
    Q_PROPERTY ( QString filePath READ filePath WRITE setFilePath )

    public:
    WPNFileNode();
    ~WPNFileNode();

    QString filePath() const { return m_filepath; }
    void setFilePath(QString path);

    QByteArray data() const;

    private:
    QByteArray m_data;
    QFile* m_file;
    QString m_filepath;

};
