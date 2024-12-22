import QtQuick 2.1

Rectangle {
    id: root

    border.width: 1
    anchors.margins: height* 0.02
    radius: height * 0.05

    implicitWidth: labelText.implicitWidth*1.1
    implicitHeight: labelText.implicitHeight*1.1

    property alias text : labelText.text
    property alias textColor : labelText.color
    signal clicked()

    Text {
        id : labelText
        anchors.fill : parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        anchors.fill : parent
        onClicked: root.clicked()
    }
}
