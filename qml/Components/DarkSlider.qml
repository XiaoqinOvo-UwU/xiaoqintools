import QtQuick
import QtQuick.Controls

// Dark-theme slider matching the app's black/grey palette.
Slider {
    id: root

    background: Rectangle {
        x: 0
        y: (parent.height - 4) / 2
        width: parent.width
        height: 4
        radius: 2
        color: Theme.sliderTrack
        Rectangle {
            width: parent.width * (root.value - root.from) / (root.to - root.from)
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
        x: parent.width * (root.value - root.from) / (root.to - root.from) - 8
        y: (parent.height - 16) / 2
    }
}
