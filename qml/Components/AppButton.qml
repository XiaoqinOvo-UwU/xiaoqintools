import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Unified frosted-glass button: translucent dark fill + soft border + hover/press tint.
// a11y: keyboard focus shows a visible focus ring; disabled state dims and blocks.
Button {
    id: root

    property color glassColor: Qt.rgba(255,255,255,0.08)
    property color glassHover: Qt.rgba(255,255,255,0.16)
    property color glassPress: Qt.rgba(255,255,255,0.22)
    property color borderColor: Qt.rgba(255,255,255,0.12)
    property color disabledColor: Qt.rgba(255,255,255,0.04)
    property int btnRadius: Theme.rMd
    property int btnHeight: 40

    implicitHeight: btnHeight
    font.pixelSize: Theme.fsDefault
    font.family: "Microsoft YaHei UI"
    focusPolicy: Qt.StrongFocus

    background: Rectangle {
        radius: btnRadius
        color: !root.enabled ? disabledColor
             : root.down ? glassPress
             : root.hovered ? glassHover
             : glassColor
        border.color: root.hovered && root.enabled ? Qt.lighter(borderColor, 1.4) : borderColor
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }

        // visible focus ring for keyboard navigation (a11y)
        Rectangle {
            anchors.fill: parent
            radius: btnRadius
            color: "transparent"
            border.color: Theme.focusRing
            border.width: 1
            visible: root.visible && root.activeFocus
            opacity: 0.9
        }
    }

    contentItem: Text {
        text: root.text
        font: root.font
        color: root.enabled ? "#FFFFFF" : "#777777"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
