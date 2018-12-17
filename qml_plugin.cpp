#include "qml_plugin.hpp"

#include <source/oscquery/client.hpp>
#include <source/oscquery/qserver.hpp>
#include <source/oscquery/node.hpp>
#include <source/oscquery/file.hpp>
#include <source/oscquery/folder.hpp>
#include <source/osc/osc.hpp>

#include <QQmlEngine>
#include <qqml.h>

void qml_plugin::registerTypes(const char *uri)
{
    Q_UNUSED    ( uri );

    qmlRegisterUncreatableType<Type, 1>         ( "WPN114.Network", 1, 0, "Type", "Uncreatable" );
    qmlRegisterUncreatableType<Access, 1>       ( "WPN114.Network", 1, 0, "Access", "Uncreatable" );
    qmlRegisterUncreatableType<Clipmode, 1>     ( "WPN114.Network", 1, 0, "Clipmode", "Uncreatable" );
    qmlRegisterUncreatableType<WPNNodeTree, 1>  ( "WPN114.Network", 1, 0, "NodeTree", "Uncreatable" );
    qmlRegisterType<OSCHandler, 1>              ( "WPN114.Network", 1, 0, "OSCDevice" );
    qmlRegisterType<WPNNode, 1>                 ( "WPN114.Network", 1, 0, "Node" );
    qmlRegisterType<WPNFileNode, 1>             ( "WPN114.Network", 1, 0, "FileNode" );
    qmlRegisterType<WPNFolderNode, 1>           ( "WPN114.Network", 1, 0, "FolderNode" );
    qmlRegisterType<WPNFolderMirror, 1>         ( "WPN114.Network", 1, 0, "FolderMirror" );
    qmlRegisterType<WPNQueryServer, 1>          ( "WPN114.Network", 1, 0, "OSCQueryServer" );
    qmlRegisterType<WPNQueryClient, 1>          ( "WPN114.Network", 1, 0, "OSCQueryClient" );
}
