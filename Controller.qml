import QtQuick 2.1

Rectangle {
    id : root

    signal showDevices()
    readonly property double joystickDim : Math.min(width/2.5, height - showTrims.implicitHeight*2)
    ClickableLabel {
        id: showTrims
        text : joystickLeft.showTrims ? qsTr("Hide Trims") : qsTr("Show Trims")
        anchors.top : parent.top
        anchors.topMargin : implicitHeight/2
        anchors.horizontalCenter: parent.horizontalCenter

        onClicked : {
            joystickLeft.showTrims = !joystickLeft.showTrims
            joystickRight.showTrims = !joystickRight.showTrims
        }
    }

    ClickableLabel {
        id: selectDevice
        text :  qsTr("Select Device")
        anchors.top : parent.top
        anchors.topMargin : implicitHeight/2
        anchors.left : parent.left
        anchors.leftMargin : parent.width * 0.02
        visible : !Device.rxTxConnected
        onClicked : {
            root.showDevices()
        }
    }

    ClickableLabel {
        id: disconnecDevice
        text :  (Device.rxTxConnected ? qsTr("Disconnect ") : qsTr("Reconnect ")) + Device.connectedDeviceName
        anchors.top : parent.top
        anchors.topMargin : implicitHeight/2
        anchors.right : parent.right
        anchors.rightMargin : parent.width * 0.02
        visible : Device.connectedDeviceName.length != 0
        textColor : Device.rxTxConnected ? "green" : "black"
        onClicked : {
            if (Device.rxTxConnected) {
                Device.disconnectFromDevice()
            } else {
                Device.scanServices(Device.connectDeviceId)
            }
        }
    }

    Item{
        enabled : Device.rxTxConnected
        anchors.fill: parent
        JoyStick {
            id: joystickLeft
            width : root.joystickDim
            height : root.joystickDim
            anchors.left : parent.left
            anchors.bottom: parent.bottom
            horizontalOnly: true
        }

        Connections {
            target: joystickLeft
            function onJoystickMoved(x, y) {
                Device.controller.leftStickMoved(x,y)
            }
        }

        JoyStick {
            id: joystickRight
            showVerticalTrimOnLeft: true

            width : root.joystickDim
            height : root.joystickDim
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            verticalOnly: true
        }

        Connections {
            target: joystickRight
            function onJoystickMoved(x, y) {
                Device.controller.rightStickMoved(x,y)
            }
        }
    }
}
