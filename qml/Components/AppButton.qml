import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Unified frosted-glass button: translucent dark fill + soft border + hover/press tint.
Button {
    id: root

    property color glassColor: Qt.rgba(255,255,255,0.08)
    property color glassHover: Qt.rgba(255,255,255,0.16)
    property color glassPress: Qt.rgba(255,255,255,0.22)
    property color borderColor: Qt.rgba(255,255,255,0.12)
    property int btnRadius: 8
    property int btnHeight: 40

    implicitHeight: btnHeight
    font.pixelSize: 14
    font.family: "Microsoft YaHei UI"

    background: Rectangle {
        radius: btnRadius
        color: root.down ? glassPress : root.hovered ? glassHover : glassColor
        border.color: root.hovered ? Qt.lighter(borderColor, 1.4) : borderColor
        border.width: 1
        Behavior on color { ColorAnimation { duration: 140 } }
        Behavior on border.color { ColorAnimation { duration: 140 } }
    }

    contentItem: Text {
        text: root.text
        font: root.font
        color: "#FFFFFF"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
