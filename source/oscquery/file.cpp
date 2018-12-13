#include "file.hpp"
#include <QtDebug>
#include <QBuffer>

WPNFileNode::WPNFileNode() : WPNNode(), m_file(nullptr)
{
    m_attributes.type       = Type::File;
    m_attributes.access     = Access::READ;
}

WPNFileNode::~WPNFileNode()
{
    m_file->close();
    delete m_file;
}

void WPNFileNode::setFilePath(QString path)
{
    m_filepath = path;
    m_file = new QFile(path, this);

    if ( path.endsWith(".qml"))
    {
        m_file->open(QIODevice::ReadOnly | QIODevice::Text );
        m_data = m_file->readAll();
    }
    else if ( path.endsWith(".png") )
    {
        qDebug() << "Loading image: " << path;
        QImage img;

        img.load        ( path, "PNG" );
        QBuffer buffer  ( &m_data );
        buffer.open     ( QIODevice::WriteOnly );
        img.save        ( &buffer, "PNG");

        return;
    }
}

QByteArray WPNFileNode::data() const
{
    return m_data;
}
