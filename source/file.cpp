#include "file.hpp"

void WPN114::Network::File::
set_file(QString path)
{
    m_file_path = path;
    m_file = new QFile(path, this);

    if (path.endsWith(".qml")) {
        m_file->open(QIODevice::ReadOnly | QIODevice::Text);
        m_data = m_file->readAll();
    }
}
