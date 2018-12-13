#include "http.hpp"
#include <QDateTime>
#include <QtDebug>

using namespace HTTP;

QString ReplyManager::formatJsonResponse( QString response )
{
    auto ba         = response.toUtf8();
    QString resp    ( "HTTP/1.1 200 OK\r\n" );
    resp.append     ( "Date: " );
    resp.append     ( QDateTime::currentDateTime().toString("ddd, dd MMMM yyyy hh:mm:ss t" ));
    resp.append     ( "\r\n" );
    resp.append     ( "Server: Qt/5.11.1\r\n" );
    resp.append     ( "Content-Type: application/json; charset=utf-8\r\n" );
    resp.append     ( "Content-Length: " );
    resp.append     ( QString::number(ba.size()).append("\r\n"));
    resp.append     ( "\r\n" );   
    resp.append     ( response );  

    return resp;
}

QString ReplyManager::formatJsonResponse(QJsonObject obj)
{
    return ReplyManager::formatJsonResponse(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QByteArray ReplyManager::formatFileResponse(QByteArray file, QString MIME)
{
    QByteArray resp ( "HTTP/1.1 200 OK\r\n" );
    resp.append     ( "Date: " );
    resp.append     ( QDateTime::currentDateTime().toString("ddd, dd MMMM yyyy hh:mm:ss t" ));
    resp.append     ( "\r\n" );
    resp.append     ( "Server: Qt/5.11.1\r\n" );
    resp.append     ( "Content-Type: " );
    resp.append     ( MIME.append("\r\n"));
    resp.append     ( "Content-Length: " );
    resp.append     ( QString::number(file.size()) );
    resp.append     ( "\r\n" );
    resp.append     ( "\r\n" );
    resp.append     ( file );

    return resp;
}

ReplyManager::ReplyManager() : m_free(true)
{

}

ReplyManager::~ReplyManager() {}

void ReplyManager::enqueue(Reply rep)
{
    m_queue.enqueue(rep);
    if ( m_free ) next();
}

void ReplyManager::onBytesWritten(qint64 bytes)
{
    auto& rep = m_queue.head();

    if ( bytes < rep.reply.size() )
    {
        rep.reply.remove(0, bytes);
        rep.target->write(rep.reply);
        return;
    }

    rep.target->deleteLater();

    QObject::disconnect( rep.target, SIGNAL(bytesWritten(qint64)),
                         this, SLOT(onBytesWritten(qint64)));
    m_queue.dequeue();
    m_free = true;

    if ( !m_queue.isEmpty()) next();
}

void ReplyManager::onSocketError(QAbstractSocket::SocketError e)
{
    qDebug() << e;
}

void ReplyManager::next()
{
    auto rep = m_queue.head();

    QObject::connect( rep.target, SIGNAL(bytesWritten(qint64)),
                      this, SLOT(onBytesWritten(qint64)) );

    QObject::connect( rep.target, SIGNAL(error(QAbstractSocket::SocketError)),
                      this, SLOT(onSocketError(QAbstractSocket::SocketError)));

    rep.target->write(rep.reply);
    m_free = false;
}
