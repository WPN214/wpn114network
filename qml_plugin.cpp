#include "qml_plugin.hpp"
#include <QQmlEngine>
#include <qqml.h>

#include <source/network.hpp>

//-------------------------------------------------------------------------------------------------
void
qml_plugin::registerTypes(const char *uri)
//-------------------------------------------------------------------------------------------------
{
    Q_UNUSED(uri)

    qmlRegisterType<Connection, 1>
    ("WPN114.Network", 1, 1, "Connection");

    qmlRegisterType<Server, 1>
    ("WPN114.Network", 1, 1, "Server");

    qmlRegisterType<Node, 1>
    ("WPN114.Network", 1, 1, "Node");

    qmlRegisterType<Client, 1>
    ("WPN114.Network", 1, 1, "Client");

    qmlRegisterUncreatableType<Type, 1>
    ("WPN114.Network", 1, 1, "Type", "Uncreatable");
}
