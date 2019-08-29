#pragma once

#include "node.hpp"

#include <QFile>
#include <QQueue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>

namespace WPN114  {
namespace Network {

//=================================================================================================
class File : public Node
//=================================================================================================
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QString file READ file WRITE set_file)

public:

    File() { m_type = Type::File; }

    File(Node& parent, QString name) :
         Node(parent, name, Type::File) {}

    //---------------------------------------------------------------------------------------------

    QString
    file() const { return m_file_path; }

    QByteArray
    data() const { return m_data; }

    //---------------------------------------------------------------------------------------------
    void
    set_file(QString path);

private:

    QFile*
    m_file = nullptr;

    QByteArray
    m_data;

    QString
    m_file_path;
};
}
}
