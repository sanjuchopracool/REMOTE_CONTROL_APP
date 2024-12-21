import QtQuick 2.1
import QtQuick.Controls

Item {
    id: root

    width : 200
    height: 200

    property bool verticalOnly : false
    property bool horizontalOnly : false

    property bool showTrims : false
    property double horizontalTrim : 0.0
    property double verticalTrim : 0.0
    property bool showVerticalTrimOnLeft : false


    signal joystickMoved(double x, double y);
    readonly property double minDimension : height < width ? height : width
    property double trimStep : 0.01

    onHorizontalTrimChanged : {
        joystick.updateMove()
    }

    onVerticalTrimChanged : {
        joystick.updateMove()
    }

    Rectangle {
        id: joystick

        width : root.showTrims ? root.minDimension* 0.8 : root.minDimension
        height : width
        radius : height / 2
        color: "gray"
        property real angle : 0
        property real distance : 0

        anchors.centerIn: parent

        ParallelAnimation {
            id: returnAnimation
            NumberAnimation { target: thumb.anchors; property: "horizontalCenterOffset";
                to: 0; duration: 200; easing.type: Easing.OutSine }
            NumberAnimation { target: thumb.anchors; property: "verticalCenterOffset";
                to: 0; duration: 200; easing.type: Easing.OutSine }

        }

        function updateMove() {
            mouse.posChanged()
        }

        MultiPointTouchArea  {
            id: mouse
            touchPoints: [
                TouchPoint { id: point1 }
            ]
            property real mouseX2 : root.verticalOnly ? width * 0.5 : point1.x
            property real mouseY2 : root.horizontalOnly ? height * 0.5 : point1.y

            property real fingerAngle : Math.atan2(mouseX2, mouseY2)
            property int mcx : mouseX2 - width * 0.5
            property int mcy : mouseY2 - height * 0.5
            property bool fingerInBounds : fingerDistance2 < distanceBound2
            property real fingerDistance2 : mcx * mcx + mcy * mcy
            property real distanceBound : width * 0.5 - thumb.width * 0.5
            property real distanceBound2 : distanceBound * distanceBound

            property double signal_x : (mouseX2 - joystick.width/2) / distanceBound
            property double signal_y : -(mouseY2 - joystick.height/2) / distanceBound

            property double xMovement : 0
            property double yMovement : 0

            anchors.fill: parent

            onPressed: {
                returnAnimation.stop();
            }

            onReleased: {
                returnAnimation.restart()
                // console.log("RESTART")
            }

            onUpdated: {
                if (fingerInBounds) {
                    thumb.anchors.horizontalCenterOffset = mcx
                    thumb.anchors.verticalCenterOffset = mcy
                } else {
                    var angle = Math.atan2(mcy, mcx)
                    thumb.anchors.horizontalCenterOffset = Math.cos(angle) * distanceBound
                    thumb.anchors.verticalCenterOffset = Math.sin(angle) * distanceBound
                }

                angle = Math.atan2(signal_y, signal_x)

                if(fingerInBounds) {
                    xMovement = root.verticalOnly ? 0 : Math.cos(angle) * Math.sqrt(fingerDistance2) / distanceBound
                    yMovement = root.horizontalOnly ? 0 : Math.sin(angle) * Math.sqrt(fingerDistance2) / distanceBound
                } else {
                    xMovement = root.verticalOnly ? 0 : Math.cos(angle) * 1
                    yMovement = root.horizontalOnly ? 0 : Math.sin(angle) * 1
                }

                posChanged()
            }

            function onUpdateThumb() {
                let xThumbPos = thumb.anchors.horizontalCenterOffset
                let yThumbPos = -thumb.anchors.verticalCenterOffset
                let thumbDistance = xThumbPos * xThumbPos + yThumbPos * yThumbPos

                let xThumbPosFactor = thumb.anchors.horizontalCenterOffset / distanceBound
                let yThumbPosFactor = -thumb.anchors.verticalCenterOffset / distanceBound

                let  angle = Math.atan2(yThumbPosFactor, xThumbPosFactor)
                // console.log("Angle = ", angle)

                xMovement = root.verticalOnly ? 0 : Math.cos(angle) * Math.sqrt(thumbDistance) / distanceBound
                yMovement = root.horizontalOnly ? 0 : Math.sin(angle) * Math.sqrt(thumbDistance) / distanceBound
                posChanged()
            }

            function clamp(val) {
                return Math.min(Math.max(val, -1.0), 1.0);
            }

            function posChanged() {
                let xVal = xMovement + root.horizontalTrim;
                let yVal = yMovement + root.verticalTrim;
                root.joystickMoved(clamp(xVal), clamp(yVal))
            }
        }

        Rectangle {
            id: thumb

            width : parent.height * 0.3
            height : width
            radius : height / 2

            color : "black"
            anchors.centerIn: parent

            onXChanged: {
                updateMovement()
            }

            onYChanged: {
                updateMovement()
            }

            function updateMovement() {
                if (returnAnimation.running) {
                    mouse.onUpdateThumb()
                }
            }
        }
    }

    Item {
        id: trimControl
        anchors.fill: parent
        visible: root.showTrims

        function roundUptoTwo(val) {
            return Math.round(val * 100) / 100
        }

        Button {
            id: xMinuxTrim
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.leftMargin: parent.width*0.2
            text: "-"

            onClicked : {
                Math.round(1.9)
                root.horizontalTrim = trimControl.roundUptoTwo(root.horizontalTrim - root.trimStep)
            }
        }

        Button {
            id: xPlusTrim
            text: "+"

            anchors.top: parent.top
            anchors.right: parent.right
            anchors.rightMargin: parent.width*0.2

            onClicked : {
                root.horizontalTrim = trimControl.roundUptoTwo(root.horizontalTrim + root.trimStep)
            }
        }

        Text {
            id: xTrimValue
            text: root.horizontalTrim

            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Button {
            id: yMinuxTrim
            text: "-"
            anchors.right: root.showVerticalTrimOnLeft ? undefined : parent.right
            anchors.left: root.showVerticalTrimOnLeft ? parent.left : undefined
            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height*0.2

            onClicked : {
                root.verticalTrim = trimControl.roundUptoTwo(root.verticalTrim - root.trimStep)
            }
        }

        Button {
            id: yPlusTrim
            text: "+"
            anchors.right: root.showVerticalTrimOnLeft ? undefined : parent.right
            anchors.left: root.showVerticalTrimOnLeft ? parent.left : undefined
            anchors.top: parent.top
            anchors.topMargin: parent.height*0.2

            onClicked : {
                root.verticalTrim = trimControl.roundUptoTwo(root.verticalTrim + root.trimStep)
            }
        }

        Text {
            id: yTrimValue
            text: root.verticalTrim
            anchors.right: root.showVerticalTrimOnLeft ? undefined : parent.right
            anchors.left: root.showVerticalTrimOnLeft ? parent.left : undefined
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
