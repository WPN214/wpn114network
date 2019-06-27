#include "qml_plugin.hpp"
#include <source/network.hpp>

#include <QQmlEngine>
#include <qqml.h>

void
qml_plugin::registerTypes(const char *uri)
{
    Q_UNUSED(uri)

    qmlRegisterType<Server, 1>
    ("WPN114.Network", 1, 1, "Server");
}
