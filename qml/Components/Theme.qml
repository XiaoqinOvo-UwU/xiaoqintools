pragma Singleton
import QtQuick

// Central color/theme definitions — black/grey professional palette.
QtObject {
    // ---- backgrounds (black-grey) ----
    readonly property color bg:          "#141414"
    readonly property color sidebar:     "#0D0D0D"
    readonly property color surface:     "#1E1E1E"
    readonly property color inputBg:     "#262626"
    readonly property color hoverBg:     "#2E2E2E"

    // ---- accents ----
    readonly property color accent:      "#3A3F4A"
    readonly property color accentHover: "#4A5160"
    readonly property color selected:    "#33383F"

    // ---- text ----
    readonly property color text:        "#F0F0F0"
    readonly property color textDim:     "#9A9A9A"
    readonly property color textMuted:   "#B0B0B0"

    // ---- status ----
    readonly property color ok:          "#5FA87A"
    readonly property color warn:        "#C9A15A"
    readonly property color danger:      "#C55A5A"

    // ---- slider ----
    readonly property color sliderTrack: "#33383F"
    readonly property color sliderFill:  "#5FA87A"
    readonly property color sliderHandle:"#E8E8E8"

    // ---- glass (frosted, subtle grey) ----
    readonly property color glass:       Qt.rgba(255,255,255,0.04)
    readonly property color glassHover:  Qt.rgba(255,255,255,0.09)
    readonly property color glassPress:  Qt.rgba(255,255,255,0.13)
    readonly property color glassBorder: Qt.rgba(255,255,255,0.08)
}
