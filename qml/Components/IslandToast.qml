import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// "Dynamic Island" — a single floating capsule at the top of the window.
// ONE instance lives in Main.qml; every request is routed through AppCore so
// the app can never show two islands at once.
//
// Modes:
//   show(msg, duration)          plain notification
//   showChoice(msg, options)     choice toast with buttons -> actionChosen(index)
//
// Motion (per UI skill group): fade + scale spring, exit FASTER than enter,
// using transform/opacity only. Focus rings on choice buttons (a11y).
Rectangle {
    id: island
    visible: false
    // dynamic width: the island SHRINKS/GROWS with the text (and the choice
    // buttons when present), capped at 440px. Width changes animate smoothly.
    //
    // IMPORTANT: measure the text with a hidden unconstrained Text. Using the
    // visible text's implicitWidth here was UNSTABLE: the visible Text is
    // stretched by Layout.fillWidth, and with wrapMode set its implicitWidth
    // depends on the island width -> circular binding, the island ended up
    // wrong sizes (too narrow -> text wraps; or stale -> oversized).
    width: Math.min(440, Math.max(measureText.implicitWidth + 64,
                                  island.options.length > 0 ? choiceRow.implicitWidth + 64 : 64))
    // text-only: height follows content with symmetric padding (min 48) and the
    // text fills+vertically centers, so the text sits EXACTLY in the pill middle.
    // choice: fixed 88, label above buttons.
    height: choiceRow.visible ? 88 : Math.max(48, text.implicitHeight + 32)
    radius: height / 2
    color: "#1C1C20"
    border.color: Qt.rgba(255,255,255,0.10)
    border.width: 1
    Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

    // hidden measure text: natural (unwrapped, unconstrained) width of the
    // message, drives the capsule width deterministically
    Text {
        id: measureText
        visible: false
        text: island.message
        font.pixelSize: Theme.fsBody
    }

    property bool active: false
    property string message: ""
    property var options: []
    signal actionChosen(int index)

    // ---- plain notification ----
    function show(msg, duration) {
        message = msg
        options = []
        present(duration || 2400)
    }

    // ---- choice toast ----
    function showChoice(msg, opts) {
        message = msg
        options = opts
        hideTimer.stop()
        present(0)
    }

    function dismiss() {
        hideTimer.stop()
        animOut.start()
    }

    function present(durationMs) {
        visible = true
        opacity = 0
        scale = 0.82
        anchors.topMargin = 14
        bounce.restart()
        if (durationMs > 0) {
            hideTimer.interval = durationMs
            hideTimer.restart()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 22
        anchors.rightMargin: 22
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        spacing: 0

        Text {
            id: text
            Layout.fillWidth: true
            Layout.fillHeight: island.options.length === 0
            Layout.alignment: Qt.AlignHCenter
            color: "#F0F0F0"
            font.pixelSize: Theme.fsBody
            text: island.message
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            lineHeight: 1.25
        }

        RowLayout {
            id: choiceRow
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 8
            visible: island.options.length > 0
            spacing: 8

            Repeater {
                model: island.options
                delegate: Button {
                    id: choiceBtn
                    text: modelData
                    implicitHeight: 34
                    implicitWidth: Math.max(96, contentW + 28)
                    font.pixelSize: Theme.fsSmall
                    focusPolicy: Qt.StrongFocus

                    readonly property int contentW: Math.ceil(choiceBtn.text.length * choiceBtn.font.pixelSize)

                    contentItem: Text {
                        text: choiceBtn.text
                        color: "#FFFFFF"
                        font: choiceBtn.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: height / 2
                        color: choiceBtn.down ? "#3A4656"
                             : choiceBtn.hovered ? "#2C3644"
                             : "#242E3C"
                        border.color: Qt.rgba(255,255,255,0.12)
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: Theme.durFast; easing.type: Easing.OutCubic } }

                        // focus ring (a11y)
                        Rectangle {
                            anchors.fill: parent
                            radius: height / 2
                            color: "transparent"
                            border.color: Theme.focusRing
                            border.width: 1
                            visible: choiceBtn.activeFocus
                            opacity: 0.9
                        }
                    }
                    onClicked: {
                        island.actionChosen(index)
                        island.dismiss()
                    }
                }
            }
        }
    }

    // enter: SAME style as exit, just reversed — fade + shrink-to-place +
    // slide down into position (no springy overshoot bounce).
    ParallelAnimation {
        id: bounce
        NumberAnimation { target: island; property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.OutQuad }
        NumberAnimation {
            target: island; property: "scale"
            from: 0.84; to: 1.0
            duration: 260
            easing.type: Easing.OutCubic
        }
        NumberAnimation { target: island; property: "anchors.topMargin"; from: -10; to: 14; duration: 200; easing.type: Easing.OutQuad }
    }

    Timer {
        id: hideTimer
        interval: 2400
        onTriggered: animOut.start()
    }

    // exit: mirror of enter — fade out, shrink back, slide up (收尾呼应)
    ParallelAnimation {
        id: animOut
        NumberAnimation { target: island; property: "opacity"; to: 0; duration: 200; easing.type: Easing.InQuad }
        NumberAnimation {
            target: island; property: "scale"
            to: 0.84
            duration: 260
            easing.type: Easing.InCubic
        }
        NumberAnimation { target: island; property: "anchors.topMargin"; to: -10; duration: 200; easing.type: Easing.InQuad }
        onFinished: island.visible = false
    }
}
