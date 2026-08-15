import QtQuick
import QtQuick.Layouts

// Reusable surface card: raised surface layer + hairline border.
// Variants: static (display) or clickable (hover/press feedback).
Rectangle {
    id: root

    property string title: ""       // optional card title
    property string subtitle: ""    // optional support text
    property bool clickable: false
    property bool hovered: false
    property bool pressed: false
    signal clicked()

    // material: one raised layer + hairline (M3/HIG tone-layering, no shadows).
    // Hover/press modifies the card's OWN fill (single rounded surface — never
    // a separate overlay that could render as a boxy highlight).
    // unified card: surface fill + hairline border on ALL states (consistent
    // look across pages). Hover only brightens the fill + lifts slightly.
    color: root.pressed ? Qt.lighter(Theme.surface, 1.12)
         : root.hovered ? Qt.lighter(Theme.surface, 1.06)
         : Theme.surface
    radius: Theme.rXl
    border.color: Theme.glassBorder
    border.width: 1
    clip: true
    Behavior on color { ColorAnimation { duration: Theme.durFast } }

    // hover lift (kept): grids leave spacing so the enlarged card never clips
    scale: root.pressed ? 0.99 : root.hovered ? 1.008 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.durMid; easing.type: Easing.OutCubic } }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp4
        spacing: Theme.sp1
        visible: root.title.length > 0 || root.subtitle.length > 0
        Text {
            Layout.fillWidth: true
            text: root.title
            color: Theme.text
            font.pixelSize: Theme.fsDefault
            font.weight: Font.DemiBold
            elide: Text.ElideRight
            visible: root.title.length > 0
        }
        Text {
            Layout.fillWidth: true
            text: root.subtitle
            color: Theme.text
            opacity: 0.55
            font.pixelSize: Theme.fsSmall
            wrapMode: Text.Wrap
            visible: root.subtitle.length > 0
        }
    }

    MouseArea {
        anchors.fill: parent
        visible: root.clickable
        hoverEnabled: true
        onEntered: root.hovered = true
        onExited: { root.hovered = false; root.pressed = false }
        onPressed: root.pressed = true
        onReleased: { root.pressed = false; root.hovered = containsMouse }
        onClicked: root.clicked()
    }
}
