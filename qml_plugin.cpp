#include "qml_plugin.hpp"
#include <qqml.h>

#include <source/network.hpp>
#include <source/tree.hpp>

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

    qmlRegisterType<File, 1>
    ("WPN114.Network", 1, 1, "File");

    qmlRegisterType<Directory, 1>
    ("WPN114.Network", 1, 1, "Directory");

    qmlRegisterType<TreeModel, 1>
    ("WPN114.Network", 1, 1, "TreeModel");

    qmlRegisterType<Client, 1>
    ("WPN114.Network", 1, 1, "Client");

    qmlRegisterUncreatableType<Type, 1>
    ("WPN114.Network", 1, 1, "Type", "Uncreatable");
}
