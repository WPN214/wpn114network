#ifndef OSCHANDLER_H
#define OSCHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QQmlParserStatus>

struct OSCMessage
{
    QString     address;
    QVariant    arguments;
};

class OSCHandler : public QObject, public QQmlParserStatus
{
    Q_OBJECT

    Q_PROPERTY ( int localPort READ localPort WRITE setLocalPort NOTIFY localPortChanged )
    Q_PROPERTY ( int remotePort READ remotePort WRITE setRemotePort NOTIFY remotePortChanged )
    Q_PROPERTY ( QString remoteAddress READ remoteAddress WRITE setRemoteAddress NOTIFY remoteAddressChanged )

    public:
    OSCHandler();
    ~OSCHandler();

    virtual void componentComplete();
    virtual void classBegin() {}

    static QByteArray encode(const OSCMessage& message);
    static OSCMessage decode(const QByteArray& data);

    uint16_t localPort() const      { return m_local_port; }
    uint16_t remotePort() const     { return m_remote_port; }
    QString remoteAddress() const   { return m_remote_address; }

    void setLocalPort               ( uint16_t port );
    void setRemotePort              ( uint16_t port );
    void setRemoteAddress           ( QString address );

    public slots:
    void listen         ( );

    void sendMessage    ( QString address, QVariant arguments );
    void sendMessage    ( const OSCMessage& message );

    protected slots:
    static QString typeTag      ( const QVariant& argument );
    static void append          ( QByteArray& data, const QVariant& arguments );

    void readPendingDatagrams   ( );
    void readOSCMessage         ( QByteArray message );
    void readOSCBundle          ( QByteArray bundle );

    signals:
    void localPortChanged       ( );
    void remotePortChanged      ( );
    void remoteAddressChanged   ( );
    void messageReceived        ( QString address, QVariant arguments );

    private:
    uint16_t m_local_port;
    uint16_t m_remote_port;
    QString m_remote_address;
    QUdpSocket* m_udpsocket;

};

#endif // OSCHANDLER_H
