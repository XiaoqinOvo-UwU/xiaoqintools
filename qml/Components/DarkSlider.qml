import QtQuick
import QtQuick.Controls

// Dark-theme slider matching the app's black/grey palette.
// Uses the official handle positioning (visualPosition + availableWidth) so
// dragging/clicking keeps working like the default Slider.
Slider {
    id: root

    implicitHeight: 24

    background: Rectangle {
        x: 0
        y: (parent.height - 4) / 2
        width: parent.width
        height: 4
        radius: 2
        color: Theme.sliderTrack
        Rectangle {
            width: parent.width * (root.visualPosition)
            height: 4
            radius: 2
            color: Theme.sliderFill
        }
    }
    handle: Rectangle {
        width: 16; height: 16
        radius: 8
        color: Theme.sliderHandle
        border.color: "#555555"
        border.width: 1
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y: root.topPadding + (root.availableHeight - height) / 2
    }
}
