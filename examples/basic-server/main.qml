import QtQuick 2.12
import QtQuick.Window 2.12
import WPN114.Network 1.1 as WPN114

Window
{
    visible: true
    width: 640
    height: 480
    title: qsTr("basic-server")

    property real foo: 3.1415926

    WPN114.Server //-------------------------------------------------------------------------------
    {
        id: server
        tcp: 5678
        name: "wpn114server"
        singleton: true
    }

    WPN114.Node on foo //--------------------------------------------------------------------------
    {
        // property binding case:
        // type will be deduced automatically, as well as the initial value of the node
        path: "/foo"
        onValueReceived: console.log("/foo", value);

        // server is registered as a singleton device, therefore, specifying the device here
        // is not necessary
        device: server
    }

    WPN114.Node //---------------------------------------------------------------------------------
    {
        path: "/foo/bar"
        type: WPN114.Type.Int
        value: 214

        onValueReceived: console.log("/foo/bar", value);
    }

    WPN114.Node //---------------------------------------------------------------------------------
    {
        path: "/pulse"
        type: WPN114.Type.Impulse
        critical: true
        // critical means it will communication will be done using websocket, not udp
        // this is useful for 'trigger' or 'on/off' Node types

        onValueReceived: console.log("/pulse");
    }
}
