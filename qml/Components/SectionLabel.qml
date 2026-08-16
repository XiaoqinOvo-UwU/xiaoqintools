import QtQuick
import QtQuick.Layouts

// Section label: small bold caption with a left accent bar.
// Used to group cards into logical sections on a page.
Row {
    id: root

    property string text: ""
    property string hint: ""   // optional right-aligned meta text

    spacing: Theme.sp2
    height: 22

    Rectangle {
        width: 3
        height: 14
        radius: 1.5
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.sectionBar
    }
    Text {
        text: root.text
        color: Theme.text
        font.pixelSize: Theme.fsSmall
        font.bold: true
        anchors.verticalCenter: parent.verticalCenter
    }
    Text {
        text: root.hint
        color: Theme.textDim
        font.pixelSize: Theme.fsCaption
        anchors.verticalCenter: parent.verticalCenter
        visible: root.hint.length > 0
    }
}
