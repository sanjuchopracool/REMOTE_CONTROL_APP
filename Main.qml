import QtQuick
import QtQuick.Controls

Window {
    id: main
    width: 1920
    height: 1080
    visible: true
    title: qsTr("REMOTE_CONTROL")

    property double contentRotation : width > height ? 0 : -90
    property double contentWidth : width > height ? width : height
    property double contentHeight : width > height ? height : width

    Rectangle {
        anchors.centerIn: parent
        rotation : main.contentRotation
        width : main.contentWidth
        height : main.contentHeight

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

        JoyStick {
            id: joystickLeft
            width : parent.width / 2.5
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

            width : parent.width / 2.5
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
}
