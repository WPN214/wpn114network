#include "qml_plugin.hpp"
#include <qqml.h>

#include <source/server.hpp>
#include <source/client.hpp>
#include <source/file.hpp>
#include <source/directory.hpp>

//-------------------------------------------------------------------------------------------------
void
qml_plugin::registerTypes(const char *uri)
//-------------------------------------------------------------------------------------------------
{
    Q_UNUSED(uri)

    qmlRegisterType<WPN114::Network::Connection, 1>
    ("WPN114.Network", 1, 1, "Connection");

    qmlRegisterType<WPN114::Network::Server, 1>
    ("WPN114.Network", 1, 1, "Server");

    qmlRegisterType<WPN114::Network::Client, 1>
    ("WPN114.Network", 1, 1, "Client");

    qmlRegisterType<WPN114::Network::Node, 1>
    ("WPN114.Network", 1, 1, "Node");

    qmlRegisterType<WPN114::Network::File, 1>
    ("WPN114.Network", 1, 1, "File");

    qmlRegisterType<WPN114::Network::Directory, 1>
    ("WPN114.Network", 1, 1, "Directory");

    qmlRegisterType<WPN114::Network::TreeModel, 1>
    ("WPN114.Network", 1, 1, "TreeModel");

    qmlRegisterUncreatableType<WPN114::Network::Type, 1>
    ("WPN114.Network", 1, 1, "Type", "Uncreatable");
}
