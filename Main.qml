import QtQuick
import QtQuick.Layouts


Window {
    id: main
    width: 1920/3
    height: 1080/3
    visible: true
    title: qsTr("REMOTE_CONTROL")

    property double contentRotation : width > height ? 0 : 90
    property double contentWidth : width > height ? width : height
    property double contentHeight : width > height ? height : width

    Controller {
        id: joystickPage
        anchors.centerIn: parent
        rotation : main.contentRotation
        width : main.contentWidth
        height : main.contentHeight


        onShowDevices : {
            pagesLayout.visible = true
        }

        StackLayout {
            id: pagesLayout
            anchors.fill: parent
            visible : false

            currentIndex: 0

            Devices {
                onShowServices: pagesLayout.currentIndex = 1
            }
            // Services {
            //     onShowDevices: pagesLayout.currentIndex = 0
            //     onShowCharacteristics: pagesLayout.currentIndex = 2
            // }
            // Characteristics {
            //     onShowDevices: pagesLayout.currentIndex = 0
            //     onShowServices: pagesLayout.currentIndex = 1
            // }
        }

        Connections {
            target: Device
            function onRxTxConnectedChanged() {
                if (Device.rxTxConnected) {
                    pagesLayout.visible = false
                }
            }
        }
    }
}
