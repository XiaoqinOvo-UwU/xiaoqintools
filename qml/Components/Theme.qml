pragma Singleton
import QtQuick

// Central design tokens — black/grey professional palette.
// Organized as: color, spacing (8px grid), type scale, radius, elevation, motion.
// Keep component files token-driven: no random hex or 13px in pages.
QtObject {
    // ================= COLOR =================
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

    // ---- interaction / focus (a11y: visible focus ring on keyboard nav) ----
    readonly property color focusRing:   "#8FA4C8"

    // ================= SPACING (8px grid) =================
    readonly property int sp1: 4
    readonly property int sp2: 8
    readonly property int sp3: 12
    readonly property int sp4: 16
    readonly property int sp5: 24
    readonly property int sp6: 32
    readonly property int sp7: 48

    // ================= TYPE SCALE =================
    readonly property int fsCaption: 11
    readonly property int fsSmall:   12
    readonly property int fsBody:    13
    readonly property int fsDefault: 14
    readonly property int fsTitle:   16
    readonly property int fsPage:    20
    readonly property int fsHero:    28

    // ================= RADIUS =================
    readonly property int rSmall: 6
    readonly property int rMd:    8
    readonly property int rLg:    10
    readonly property int rXl:    12
    readonly property int rFull:  999

    // ================= MOTION (ease-out) =================
    readonly property int durFast:   120
    readonly property int durMid:    200
    readonly property int durSlow:   320
}
