import QtQuick
import QtQuick.Controls

// "Dynamic Island" style toast with symmetric Q弹 enter/exit (收尾呼应).
Rectangle {
    id: island
    visible: false
    width: Math.min(420, text.implicitWidth + 56)
    height: 46
    radius: height / 2
    color: "#1A1A1E"
    border.color: "#2E2E38"
    border.width: 1

    property bool active: false
    property string message: ""

    function show(msg, duration) {
        message = msg
        visible = true
        opacity = 0
        scale = 0.7
        anchors.topMargin = 16
        hideTimer.interval = duration || 2400
        bounce.restart()
        hideTimer.restart()
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

    Text {
        id: text
        anchors.centerIn: parent
        color: "#FFFFFF"
        font.pixelSize: 13
        text: island.message
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
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
