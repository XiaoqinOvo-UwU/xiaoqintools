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
    readonly property color card:        "#1E1E1E"
    readonly property color inputBg:     "#262626"

    // ---- accents ----
    readonly property color accent:      "#3A3F4A"
    readonly property color accentHover: "#4A5160"
    readonly property color selected:    "#33383F"

    // hover highlight — neutral grey tint (no blue)
    readonly property color hoverBg:     "#252525"
    readonly property color hoverBgStrong: "#2E2E2E"

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
    // neutral grey — never blue
    readonly property color focusRing:   Qt.rgba(1,1,1,0.45)

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

    // typography hierarchy (semantic usage)
    readonly property int typeH1: fsHero      // page hero / big numbers
    readonly property int typeH2: fsPage      // page title
    readonly property int typeH3: fsTitle     // card title
    readonly property int typeBody: fsDefault // body / buttons
    readonly property int typeMeta: fsSmall   // secondary info
    readonly property int typeCaption: fsCaption // captions / badges

    // ================= RADIUS =================
    readonly property int rSmall: 6
    readonly property int rMd:    8
    readonly property int rLg:    10
    readonly property int rXl:    14
    readonly property int rFull:  999

    // ================= MOTION (ease-out) =================
    readonly property int durFast:   120
    readonly property int durMid:    200
    readonly property int durSlow:   320

    // ================= WALLPAPER STATE (set by Main.qml) =================
    // The wallpaper only affects the background LAYER. UI stays near-opaque
    // dark — NO glassmorphism / acrylic / white glass. This mimics "dark
    // desktop + wallpaper", not a transparent webpage.
    property bool wallpaperActive: false

    // ---- cards: original dark colour, only slight translucency (0.85) so the
    // (already 20% faded) wallpaper tints behind without harming readability.
    // NOTE: Qt.rgba() does NOT scale 0-255 — use fractions (/255) or it clamps
    // to 1.0 and renders WHITE.
    readonly property color cardFill: wallpaperActive ? Qt.rgba(24/255, 24/255, 24/255, 0.85) : Theme.surface
    readonly property color cardFillHover: wallpaperActive ? Qt.rgba(31/255, 31/255, 31/255, 0.85) : Theme.hoverBgStrong
    readonly property color cardFillPress: wallpaperActive ? Qt.rgba(38/255, 38/255, 38/255, 0.85) : Theme.hoverBgStrong

    // ---- buttons: #242424 default, hover slightly brighter, NO white fill ----
    readonly property color btnFill: wallpaperActive ? Qt.rgba(36/255, 36/255, 36/255, 0.95) : Qt.rgba(1,1,1,0.08)
    readonly property color btnFillHover: wallpaperActive ? Qt.rgba(52/255, 52/255, 52/255, 0.95) : Qt.rgba(1,1,1,0.14)
    readonly property color btnFillPress: wallpaperActive ? Qt.rgba(60/255, 60/255, 60/255, 0.95) : Qt.rgba(1,1,1,0.20)
    readonly property color btnBorder: wallpaperActive ? Qt.rgba(255,255,255,0.10) : Qt.rgba(1,1,1,0.12)

    // ---- chat overlay: independent layer; bg lets the wallpaper show through
    // (0.60), while bubbles/input stay near-opaque for readability ----
    readonly property color chatBg: wallpaperActive ? Qt.rgba(15/255, 15/255, 15/255, 0.60) : Theme.bg      // chat panel backdrop
    readonly property color chatPanelBg: wallpaperActive ? Qt.rgba(18/255, 18/255, 18/255, 0.90) : Theme.surface // chat header/input bar
    readonly property color aiBubbleFill: wallpaperActive ? Qt.rgba(37/255, 37/255, 37/255, 0.95) : Theme.surface // AI bubble (user stays opaque accent)
    readonly property color inputFill: wallpaperActive ? Qt.rgba(28/255, 28/255, 28/255, 0.95) : Theme.inputBg
}
