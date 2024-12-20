import QtQuick
import QtQuick.Controls

Window {
    id: main

    width: 640
    height: 480
    visible: true
    title: qsTr("REMOTE_CONTROL")

    Button {
        id: showTrims
        text : qsTr("Show Trims")
        anchors.top : parent.top
        anchors.horizontalCenter: parent.horizontalCenter

        onClicked : {
            joystickLeft.showTrims = !joystickLeft.showTrims
            joystickRight.showTrims = !joystickRight.showTrims
        }
    }

    JoyStick {
        id: joystickLeft
        width : main.width / 2.5
        height : width
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

        width : main.width / 2.5
        height : width
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
