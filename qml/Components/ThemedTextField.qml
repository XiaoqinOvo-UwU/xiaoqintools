import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Themed single-line input: theme tokens for fill/text/placeholder and a
// uniform hairline background. Layout sizing lives here so call sites stay flat.
TextField {
    id: field

    Layout.fillWidth: true
    Layout.preferredHeight: 36
    color: Theme.text
    placeholderTextColor: Theme.textDim
    selectByMouse: true
    background: Rectangle { color: Theme.inputBg; radius: 8 }
}
