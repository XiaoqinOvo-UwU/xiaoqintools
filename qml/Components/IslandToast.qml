import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// "Dynamic Island" style toast with symmetric Q弹 enter/exit (收尾呼应).
// Supports two modes:
//   show(msg, duration)            - plain notification
//   showChoice(msg, options)       - choice toast with buttons, emits actionChosen(index)
Rectangle {
    id: island
    visible: false
    width: Math.min(440, Math.max(text.implicitWidth + 56, choiceRow.implicitWidth + 56))
    height: choiceRow.visible ? 84 : 46
    radius: height / 2
    color: "#1A1A1E"
    border.color: "#2E2E38"
    border.width: 1

    property bool active: false
    property string message: ""
    property var options: []
    signal actionChosen(int index)

    function show(msg, duration) {
        message = msg
        options = []
        visible = true
        opacity = 0
        scale = 0.7
        anchors.topMargin = 16
        hideTimer.interval = duration || 2400
        bounce.restart()
        hideTimer.restart()
    }

    function showChoice(msg, opts) {
        message = msg
        options = opts
        visible = true
        opacity = 0
        scale = 0.7
        anchors.topMargin = 16
        hideTimer.stop()
        bounce.restart()
    }

    function dismiss() {
        hideTimer.stop()
        animOut.start()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        spacing: 0

        Text {
            id: text
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            color: "#FFFFFF"
            font.pixelSize: 13
            text: island.message
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            maximumLineCount: 1
        }

        RowLayout {
            id: choiceRow
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 8
            visible: island.options.length > 0
            spacing: 10

            Repeater {
                model: island.options
                delegate: Button {
                    id: choiceBtn
                    text: modelData
                    implicitWidth: 120
                    implicitHeight: 30
                    font.pixelSize: 12
                    contentItem: Text {
                        text: choiceBtn.text
                        color: "#FFFFFF"
                        font: choiceBtn.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 15
                        color: choiceBtn.down ? "#3A4656" : choiceBtn.hovered ? "#2C3644" : "#242E3C"
                        border.color: "#3A4656"
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 140 } }
                    }
                    onClicked: {
                        island.actionChosen(index)
                        island.dismiss()
                    }
                }
            }
        }
    }

    // enter: Q弹 spring-in
    ParallelAnimation {
        id: bounce
        NumberAnimation { target: island; property: "opacity"; from: 0; to: 1; duration: 240; easing.type: Easing.OutQuad }
        NumberAnimation {
            target: island; property: "scale"
            from: 0.7; to: 1.0
            duration: 480
            easing.type: Easing.OutBack
            easing.overshoot: 1.8
        }
    }

    Timer {
        id: hideTimer
        interval: 2400
        onTriggered: animOut.start()
    }

    // exit: mirror of enter — shrink back with Q弹 overshoot + slide up (收尾呼应)
    ParallelAnimation {
        id: animOut
        NumberAnimation { target: island; property: "opacity"; to: 0; duration: 260; easing.type: Easing.InQuad }
        NumberAnimation {
            target: island; property: "scale"
            to: 0.7
            duration: 360
            easing.type: Easing.InBack
            easing.overshoot: 1.8
        }
        NumberAnimation { target: island; property: "anchors.topMargin"; to: -12; duration: 260; easing.type: Easing.InQuad }
        onFinished: island.visible = false
    }
}
