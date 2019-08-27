import QtQuick 2.12
import QtQuick.Window 2.12
import WPN114.Network 1.1 as WPN114

Window
{
    visible: true
    width: 640
    height: 480
    title: qsTr("basic-client")

    WPN114.Server
    {
        id: server
        udp: 1234
        tcp: 5678
    }

    WPN114.Node
    {
        tree: server.tree()
        path: "/foo"
        type: WPN114.Type.Int
        value: 214
    }

    WPN114.Client
    {
        id: client
        host: "ws://localhost:5678"

        onConnected: {
            client.send("/foo", 314);
            client.listen("/foo");
        }
    }

    WPN114.Node
    {
        tree: client.tree()
        path: "/foo"

        onValueChanged:
            console.log(value);
    }
}
