import QtQuick
import QtQuick.Layouts

// Chat message bubble: AI left / user right, rounded-corner chat style
// (Telegram-like asymmetric radius), optional time label, hover timestamp.
Rectangle {
    id: root

    property bool isAi: true
    property string text: ""
    property string time: ""

    // Telegram-style asymmetric corners
    radius: isAi ? Theme.rXl : Theme.rXl
    color: isAi ? Qt.rgba(1,1,1,0.06) : Theme.accent
    border.color: isAi ? Theme.glassBorder : "transparent"
    border.width: isAi ? 1 : 0

    implicitWidth: Math.min(root.parent ? root.parent.width * 0.68 : 320, content.width + 28)
    implicitHeight: Math.max(36, content.height + 20)

    ColumnLayout {
        id: content
        width: root.width - 28
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 2
        Text {
            Layout.fillWidth: true
            text: root.text
            color: Theme.text
            font.pixelSize: Theme.fsDefault
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
        }
        Text {
            text: root.time
            color: Theme.textDim
            font.pixelSize: 10
            horizontalAlignment: isAi ? Text.AlignLeft : Text.AlignRight
            Layout.alignment: isAi ? Qt.AlignLeft : Qt.AlignRight
            visible: root.time.length > 0
        }
    }
}
