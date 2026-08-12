import QtQuick
import QtQuick.Controls

// Generic frosted-glass action card used across pages.
Rectangle {
    id: card
    property string title: ""
    property string desc: ""
    property color textColor: "#FFFFFF"
    property bool hovered: false
    property bool pressed: false
    signal clicked()

    // frosted glass look
    implicitWidth: 180
    implicitHeight: 92
    radius: 12

    // translucent frosted look (flat, low-saturation, no gradients)
    color: pressed ? Qt.rgba(255,255,255,0.18)
         : hovered ? Qt.rgba(255,255,255,0.12)
         : Qt.rgba(255,255,255,0.06)
    border.color: Qt.rgba(255,255,255,0.10)
    border.width: 1

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onEntered: card.hovered = true
        onExited: {
            card.hovered = false
            card.pressed = false
        }
        onPressed: card.pressed = true
        onReleased: {
            card.pressed = false
            card.hovered = mouse.containsMouse
        }
        onClicked: card.clicked()
    }

    Column {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 6
        Text {
            text: card.title
            color: card.textColor
            font.pixelSize: 15
            font.bold: true
        }
        Text {
            text: card.desc
            color: "#A8B5C4"
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
    }
}
