import QtQuick
import QtQuick.Controls

Rectangle {
    id : root

    signal showDevices()
    readonly property double joystickDim : Math.min(width/2.5, height - showTrims.implicitHeight)
    Button {
        id: showTrims
        text : joystickLeft.showTrims ? qsTr("Hide Trims") : qsTr("Show Trims")
        anchors.top : parent.top
        anchors.horizontalCenter: parent.horizontalCenter

        onClicked : {
            joystickLeft.showTrims = !joystickLeft.showTrims
            joystickRight.showTrims = !joystickRight.showTrims
        }
    }

    Button {
        id: selectDevice
        text :  qsTr("Select Device")
        anchors.top : parent.top
        anchors.left : parent.left
        anchors.leftMargin : parent.width * 0.05


        onClicked : {
            root.showDevices()
        }
    }

    JoyStick {
        id: joystickLeft
        width : root.joystickDim
        height : root.joystickDim
        anchors.left : parent.left
        anchors.bottom: parent.bottom
    }

    Connections {
        target: joystickLeft
        function onJoystickMoved(x, y) {
            console.log("Left Moved!", x, y)
        }
    }

    JoyStick {
        id: joystickRight
        showVerticalTrimOnLeft: true

        width : root.joystickDim
        height : root.joystickDim
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

    Connections {
        target: joystickRight
        function onJoystickMoved(x, y) {
            console.log("Right Moved!", x, y)
        }
    }
}
