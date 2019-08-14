import QtQuick 2.12
import QtQuick.Window 2.12
import WPN114.Network 1.1 as WPN114

Window
{
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")

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
        path: "/foo"
        onValueReceived: console.log("/foo", value);
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
        onValueReceived:
        {
            console.log("/pulse");
        }
    }
}
