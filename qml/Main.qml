import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.LocalStorage
import "Pages"
import "Components"

ApplicationWindow {
    id: root
    width: 1100
    height: 720
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    title: "小钦的工具"
    color: Theme.bg   // opaque — DWM rounds the frameless window corners natively

    // frameless: the native title bar is replaced by the custom title strip
    // below so the window chrome blends into the app (Discord/Telegram style).
    flags: Qt.Window | Qt.FramelessWindowHint

    // ---- frameless helpers ----
    function isMaximized() { return root.visibility === Window.Maximized }
    function toggleMaximize() {
        if (root.isMaximized()) root.showNormal()
        else root.showMaximized()
    }

    // Global dark palette so Quick Controls render dark.
    palette {
        window: "#141414"
        windowText: "#F0F0F0"
        base: "#262626"
        alternateBase: "#1E1E1E"
        text: "#F0F0F0"
        button: "#3A3F4A"
        buttonText: "#FFFFFF"
        highlight: "#3A3F4A"
        highlightedText: "#FFFFFF"
        toolTipBase: "#1E1E1E"
        toolTipText: "#F0F0F0"
        placeholderText: "#9A9A9A"
    }

    property int currentPage: 0
    property string aiGreeting: "你好。"
    property bool userBtnHover: false   // sidebar user-avatar ⌄ hover flag

    // custom wallpaper (blurred copy from AiService) shown behind the right pane.
    // Layer 1 only — the dark overlay + near-opaque UI sit ABOVE it.
    property string wallpaperUrl: ""
    property real wallpaperBrightness: 0.5    // 0..1 avg luminance, drives the dark overlay
    function refreshWallpaper() {
        var oldUrl = root.wallpaperUrl
        root.wallpaperUrl = aiService.wallpaperPath()
        root.wallpaperBrightness = aiService.wallpaperBrightness()
        Theme.wallpaperActive = root.wallpaperUrl.length > 0
        // crossfade only when switching between two real wallpapers (300ms)
        if (oldUrl.length > 0 && root.wallpaperUrl.length > 0 && oldUrl !== root.wallpaperUrl) {
            wpFront.source = root.wallpaperUrl
            wpFront.opacity = 0
            wpCrossfade.start()
        } else if (oldUrl.length === 0 && root.wallpaperUrl.length > 0) {
            wpBack.source = root.wallpaperUrl   // first set: just show
        }
    }
    Connections {
        target: aiService
        function onWallpaperChanged() { root.refreshWallpaper() }
    }

    // contact list model (id|name|hasAvatar)
    ListModel { id: contactModel }

    // read the last message of a contact from the chat DB (preview)
    function lastMsgFor(contactId) {
        try {
            var db = LocalStorage.openDatabaseSync("XiaoQinChat", "1.0", "chat history", 8*1024*1024)
            var preview = ""
            db.transaction(function(tx) {
                tx.executeSql("CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY AUTOINCREMENT, contact TEXT, isAi INTEGER, msg TEXT)")
                var rs = tx.executeSql("SELECT isAi, msg FROM messages WHERE contact=? ORDER BY id DESC LIMIT 1", [contactId])
                if (rs.rows.length > 0) {
                    var m = rs.rows.item(0)
                    preview = (m.isAi === 1 ? "" : "我: ") + m.msg
                }
            })
            return preview
        } catch (e) { return "" }
    }

    function refreshContacts() {
        contactModel.clear()
        var list = contactService.contactList()
        for (var i = 0; i < list.length; i++) {
            var parts = list[i].split("|")
            if (parts.length >= 3) {
                var avatarUrl = ""
                if (parts[2] === "1")
                    avatarUrl = contactService.contactAvatarUrl(parts[0])
                var preview = lastMsgFor(parts[0])
                contactModel.append({ "cid": parts[0], "cname": parts[1], "hasAvatar": parts[2], "avatarUrl": avatarUrl, "lastMsg": preview })
            }
        }
    }

    // refresh profile labels after edit
    function refreshProfile() {
        userAvatarText.text = aiService.avatarChar()
        userNameText.text = aiService.userName()
        root.aiAvatarPath = toFileUrl(aiService.aiAvatarPath())
        root.userAvatarPath = toFileUrl(aiService.userAvatarPath())
        refreshContacts()
    }

    // convert a local path to a file:// URL usable by Image.source
    function toFileUrl(p) {
        if (p.length === 0) return ""
        if (p.indexOf("file://") === 0) return p
        return "file:///" + p.replace(/\\/g, "/")
    }

    // ================= ROUNDED WINDOW FRAME =================
    // frameless window; Windows 11 DWM rounds the corners natively (opaque
    // window, so the corner clip is handled by the OS).
    Rectangle {
        id: rootPanel
        anchors.fill: parent
        radius: Theme.rXl
        color: Theme.bg
        border.color: Qt.rgba(255,255,255,0.07)
        border.width: 1
        clip: true

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ================= CUSTOM TITLE STRIP (frameless) =================
        // blends into the app: dark, matches the sidebar tone, window controls
        // on the right (Discord/Telegram/Linear style).
        Rectangle {
            id: titleBar
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.sidebar

            // drag region (whole strip minus the control buttons)
            MouseArea {
                id: titleDrag
                anchors.fill: parent
                anchors.right: winControls.left
                cursorShape: Qt.ArrowCursor
                onPressed: function(mouse) { if (mouse.button === Qt.LeftButton) root.startSystemMove() }
                onDoubleClicked: root.toggleMaximize()
            }

            // subtle hairline under the strip
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.glassBorder
            }

            // app icon + title — blends over the sidebar zone (restores the
            // icon that the native title bar used to show top-left)
            Image {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                width: 16; height: 16
                source: "qrc:/icons/app.ico"
                fillMode: Image.PreserveAspectFit
            }
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 38
                anchors.verticalCenter: parent.verticalCenter
                text: "小钦的工具"
                color: Theme.textDim
                font.pixelSize: Theme.fsCaption
                font.bold: true
            }

            // window controls: minimize / maximize / close
            Row {
                id: winControls
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                spacing: 0
                WindowButton { variant: "min"; tip: "最小化"; onClicked: root.showMinimized() }
                WindowButton {
                    variant: root.isMaximized() ? "restore" : "max"
                    tip: root.isMaximized() ? "还原" : "最大化"
                    onClicked: root.toggleMaximize()
                }
                WindowButton { variant: "close"; tip: "关闭"; onClicked: Qt.quit() }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ================= LEFT NAVIGATION =================
            Rectangle {
                Layout.preferredWidth: 248
                Layout.fillHeight: true
                color: Theme.sidebar

                ColumnLayout {
                    anchors.fill: parent
                spacing: 0

                // ---- top: user avatar + name ----
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 66
                    color: "transparent"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 18
                        anchors.rightMargin: 14
                        spacing: 12

                        Rectangle {
                            id: userAvatarBox
                            width: 42; height: 42
                            radius: 21
                            color: Theme.accent
                            clip: true
                            scale: userBtn.pressed ? 0.92 : 1.0
                            Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                            Image {
                                anchors.fill: parent
                                visible: root.userAvatarPath.length > 0
                                source: root.userAvatarPath
                                fillMode: Image.PreserveAspectCrop
                                smooth: true
                                mipmap: true
                            }
                            Text {
                                id: userAvatarText
                                anchors.centerIn: parent
                                text: root.userAvatarPath.length > 0 ? "" : aiService.avatarChar()
                                color: "white"
                                font.pixelSize: 17
                                font.bold: true
                            }
                        }

                        Column {
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 2
                            Text {
                                id: userNameText
                                text: aiService.userName()
                                color: Theme.text
                                font.pixelSize: 15
                                font.bold: true
                            }
                            Row {
                                spacing: 6
                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: Theme.ok
                                }
                                Text { text: appCore.statusText; color: Theme.textDim; font.pixelSize: 11 }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // shows on hover — uses an explicit flag (onEntered/onExited)
                        // because MouseArea.hovered is unavailable in this Qt build
                        Text {
                            text: "⌄"
                            color: Theme.textDim
                            font.pixelSize: 16
                            visible: root.userBtnHover
                            opacity: root.userBtnHover ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: 150 } }
                        }
                    }
                    MouseArea {
                        id: userBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: root.userBtnHover = true
                        onExited: root.userBtnHover = false
                        onClicked: userMenu.popup(userBtn, 0, userBtn.height + 4)
                    }
                }

                // user menu
                Menu {
                    id: userMenu
                    width: 200
                    background: Rectangle {
                        color: Theme.surface
                        radius: 10
                        border.color: Theme.glassBorder
                        border.width: 1
                    }
                    MenuItem {
                        text: "编 辑资料"
                        onClicked: profileDialog.open()
                    }
                    MenuItem {
                        text: "设 置"
                        onClicked: pageStack.switchPage(3)
                    }
                    MenuItem {
                        text: "导 出配置"
                        onClicked: {
                            var ok = syncService.exportConfig(syncService.defaultExportPath())
                            islandToast.show(ok ? "配置已导出到杂货铺" : "导出失败")
                        }
                    }
                    MenuItem {
                        text: "退 出"
                        onClicked: Qt.quit()
                    }
                }

                Rectangle { // separator
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.glassBorder
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                }

                // ================= AI CONTACTS =================
                // contact list
                Repeater {
                    model: contactModel
                    delegate: Rectangle {
                        id: contactItem
                        Layout.fillWidth: true
                        Layout.preferredHeight: 58
                        Layout.leftMargin: Theme.sp2
                        Layout.rightMargin: Theme.sp2
                        radius: Theme.rMd
                        // selected contact: only a thin left indicator bar (no full highlight)
                        color: cHover ? Theme.hoverBg : "transparent"
                        property bool cHover: false
                        Behavior on color { ColorAnimation { duration: 140 } }

                        // selected indicator (small vertical bar, like nav tabs)
                        Rectangle {
                            width: 3; height: cid === contactService.currentId() ? 24 : 0
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            color: Theme.textDim
                            radius: 1.5
                            opacity: cid === contactService.currentId() ? 1 : 0
                            Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                            Behavior on opacity { NumberAnimation { duration: 180 } }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 12
                            spacing: 10
                            Rectangle {
                                width: 36; height: 36
                                radius: 18
                                color: Theme.accent
                                clip: true
                                antialiasing: true
                                smooth: true
                                Image {
                                    anchors.fill: parent
                                    visible: avatarUrl.length > 0
                                    source: avatarUrl
                                    fillMode: Image.PreserveAspectCrop
                                    smooth: true
                                    mipmap: true
                                }
                                Text {
                                    anchors.centerIn: parent
                                    visible: avatarUrl.length === 0
                                    text: cname.length > 0 ? cname.charAt(0) : "A"
                                    color: "white"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                // unread red dot (proactive AI messages while user is away)
                                Rectangle {
                                    width: 10; height: 10
                                    radius: 5
                                    color: "#E5534B"
                                    border.color: Theme.sidebar
                                    border.width: 1
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.rightMargin: -1
                                    anchors.topMargin: -1
                                    visible: cid === contactService.currentId() && root.unreadCount > 0 && (!root.chatOpen || !root.active)
                                }
                            }
                            Column {
                                Layout.fillWidth: true
                                spacing: 1
                                Text {
                                    text: cname
                                    color: Theme.text
                                    font.pixelSize: 13
                                    font.bold: cid === contactService.currentId()
                                }
                                // live conversation preview (last message), falls back to a hint
                                Text {
                                    text: lastMsg.length > 0 ? lastMsg : "点击开始聊天"
                                    color: Theme.textDim
                                    font.pixelSize: 10
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                    width: parent.width
                                }
                            }
                            Text {
                                text: "›"
                                color: Theme.textDim
                                font.pixelSize: 15
                            }
                        }
                        // MouseArea last = on top, so clicks always reach it
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: contactItem.cHover = true
                            onExited: contactItem.cHover = false
                            onClicked: {
                                // open the chat first, then refresh profile:
                                // setCurrent fires contactsChanged -> refreshProfile (reentrant),
                                // so chatOpen must come before it or the signal chain breaks.
                                contactService.setCurrent(cid)
                                root.chatOpen = true
                                chatPage.openContact(cid)
                                root.clearUnread()
                                root.refreshProfile()
                            }
                        }
                    }
                }

                // empty state: no AI contacts yet — explain + CTA
                Column {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.sp6
                    visible: contactModel.count === 0
                    spacing: Theme.sp3
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "还没有 AI 联系人"
                        color: Theme.textDim
                        font.pixelSize: Theme.fsSmall
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "去「设置 → AI 配置」添加第一个 AI 吧"
                        color: Theme.textMuted
                        font.pixelSize: Theme.fsCaption
                    }
                }

                Rectangle { // separator
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.glassBorder
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                }

                Item { Layout.fillHeight: true }

                // ---- nav items (lower-middle) — continuous sidebar list ----
                Repeater {
                    model: ListModel {
                        ListElement { label: "网络" }
                        ListElement { label: "系统" }
                        ListElement { label: "娱乐" }
                        ListElement { label: "设置" }
                    }
                    Rectangle {
                        id: navItem
                        property bool hovered: false
                        property int myIndex: index
                        // when the chat page is open, no nav tab stays highlighted
                        readonly property bool active: !root.chatOpen && root.currentPage === myIndex
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        // continuous list: transparent default, faint hover tint,
                        // single flat active block (no border / no outline / no card)
                        color: active ? Qt.rgba(1,1,1,0.06)
                             : hovered ? Qt.rgba(1,1,1,0.035)
                             : "transparent"
                        Behavior on color { ColorAnimation { duration: 140 } }
                        focus: true

                        // selected: left thin line (Linear/macOS settings style)
                        Rectangle {
                            width: 2; height: active ? 20 : 0
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            color: Theme.text
                            radius: 1
                            opacity: active ? 1 : 0
                            Behavior on height { NumberAnimation { duration: Theme.durMid; easing.type: Easing.OutCubic } }
                            Behavior on opacity { NumberAnimation { duration: Theme.durMid } }
                        }

                        Keys.onReturnPressed: activate()
                        Keys.onEnterPressed: activate()
                        Keys.onSpacePressed: activate()
                         function activate() {
                            pageStack.switchPage(myIndex)
                            root.chatOpen = false
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.sp4
                            anchors.rightMargin: Theme.sp3
                            spacing: Theme.sp3
                            Text {
                                text: model.label
                                color: active ? Theme.text : Theme.textDim
                                font.pixelSize: Theme.fsDefault
                                font.weight: active ? Font.DemiBold : Font.Normal
                            }
                            Item { Layout.fillWidth: true }
                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: navItem.hovered = true
                            onExited: navItem.hovered = false
                            onClicked: navItem.activate()
                        }
                    }
                }

                // ---- bottom: version line ----
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    color: "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "小钦的工具 v" + updateService.currentVersion()
                        color: Theme.textDim
                        font.pixelSize: Theme.fsCaption
                    }
                }
            }
        }

        // ================= MAIN CONTENT =================
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"

            // ================= LAYER 1: WALLPAPER BACKGROUND =================
            // Only this layer shows the wallpaper — at 20% opacity so it can
            // NEVER wash out the UI. 300ms crossfade on change.
            Item {
                anchors.fill: parent
                visible: root.wallpaperUrl.length > 0
                Image {
                    id: wpBack
                    anchors.fill: parent
                    source: root.wallpaperUrl
                    fillMode: Image.PreserveAspectCrop
                    smooth: true
                    mipmap: true
                    opacity: 0.20
                }
                Image {
                    id: wpFront
                    anchors.fill: parent
                    source: ""
                    fillMode: Image.PreserveAspectCrop
                    smooth: true
                    mipmap: true
                    opacity: 0
                }
                SequentialAnimation {
                    id: wpCrossfade
                    NumberAnimation { target: wpFront; property: "opacity"; from: 0; to: 0.20; duration: 300; easing.type: Easing.OutCubic }
                    ScriptAction { script: { wpBack.source = wpFront.source; wpFront.source = ""; wpFront.opacity = 0 } }
                }
            }

            // ================= LAYER 2: DARK OVERLAY =================
            // Adapts to the wallpaper brightness (0.45 dark .. 0.65 bright).
            Rectangle {
                anchors.fill: parent
                color: root.wallpaperUrl.length > 0
                     ? Qt.rgba(0, 0, 0, 0.45 + 0.20 * root.wallpaperBrightness)
                     : "transparent"
            }

            // subtle vignette: darken the extreme top/bottom edges so cards and
            // the header pop (depth, macOS dark-desktop feel). No white/glass.
            Rectangle {
                anchors.fill: parent
                visible: root.wallpaperUrl.length > 0
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.30) }
                    GradientStop { position: 0.20; color: Qt.rgba(0, 0, 0, 0.0) }
                    GradientStop { position: 0.80; color: Qt.rgba(0, 0, 0, 0.0) }
                    GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.30) }
                }
            }

            StackLayout {
                id: pageStack
                anchors.fill: parent
                currentIndex: root.currentPage
                // LAYER 3 is hidden while the chat overlay (LAYER 4) is open, so
                // business pages never show through the chat.
                visible: !root.chatOpen

                onCurrentIndexChanged: {
                    // NOTE: pageStack.currentItem is UNDEFINED in this handler;
                    // use itemAt(currentIndex) — that's why page switches had no
                    // animation before.
                    var item = pageStack.itemAt(pageStack.currentIndex)
                    if (item) {
                        item.opacity = 0
                        item.scale = 0.96
                        itemBehavior.target = item
                        itemBehavior.restart()
                    }
                }

                SequentialAnimation {
                    id: itemBehavior
                    property Item target
                    ParallelAnimation {
                        NumberAnimation { target: itemBehavior.target; property: "opacity"; to: 1; duration: 170; easing.type: Easing.OutCubic }
                        NumberAnimation { target: itemBehavior.target; property: "scale"; to: 1.0; duration: 170; easing.type: Easing.OutCubic }
                    }
                }

                // ---- page exit: fade out the current page, then switch ----
                // The outgoing page must animate BEFORE currentIndex changes
                // (StackLayout hides it instantly on switch), so switching goes
                // through switchPage() which runs this first.
                property int pendingPage: -1
                function switchPage(idx) {
                    if (idx === root.currentPage) return
                    var out = pageStack.itemAt(pageStack.currentIndex)
                    if (out) {
                        // NOTE: unqualified `pendingPage` is NOT visible inside
                        // nested signal handlers (ReferenceError -> frozen UI).
                        // Qualify explicitly.
                        pageStack.pendingPage = idx
                        pageOutAnim.target = out
                        pageOutAnim.restart()
                    } else {
                        root.currentPage = idx
                    }
                }
                ParallelAnimation {
                    id: pageOutAnim
                    property Item target
                    // fast exit (quicker than enter, quick start) so switching
                    // never feels sticky: 110ms OutCubic reads as a brisk
                    // fade-out instead of a slow drag.
                    NumberAnimation { target: pageOutAnim.target; property: "opacity"; to: 0; duration: 110; easing.type: Easing.OutCubic }
                    NumberAnimation { target: pageOutAnim.target; property: "scale"; to: 0.98; duration: 110; easing.type: Easing.OutCubic }
                    onFinished: {
                        root.currentPage = pageStack.pendingPage
                        pageStack.pendingPage = -1
                    }
                }

                NetworkPage {}
                SystemPage {}
                EntertainmentPage {}
                SettingsPage {}
            }

            // Chat page overlays the right content area (sidebar stays visible)
            ChatPage {
                id: chatPage
                anchors.fill: parent
                visible: root.chatOpen
                aiAvatarSource: root.aiAvatarPath
                userAvatarSource: root.userAvatarPath
                onBackRequested: chatPage.closeWithAnim()
                onCloseFinished: root.chatOpen = false
                onAiProfileRequested: aiProfileDialog.open()
            }
        }
    }
    } // end ColumnLayout (title strip + content)
    } // end rootPanel (rounded window frame)

    // ===== frameless resize handles (native startSystemResize) =====
    // invisible strips on the window edges so a frameless window can be resized
    Rectangle {
        width: parent.width; height: 6
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        color: "transparent"; z: 20
        MouseArea {
            anchors.fill: parent; cursorShape: Qt.SizeVerCursor
            onPressed: root.startSystemResize(Qt.BottomEdge)
        }
    }
    Rectangle {
        width: 6; height: parent.height
        anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
        color: "transparent"; z: 20
        MouseArea {
            anchors.fill: parent; cursorShape: Qt.SizeHorCursor
            onPressed: root.startSystemResize(Qt.RightEdge)
        }
    }
    Rectangle {
        width: 14; height: 14
        anchors.right: parent.right; anchors.bottom: parent.bottom
        color: "transparent"; z: 20
        MouseArea {
            anchors.fill: parent; cursorShape: Qt.SizeFDiagCursor
            onPressed: root.startSystemResize(Qt.BottomEdge | Qt.RightEdge)
        }
    }
    Rectangle {
        width: 14; height: 14
        anchors.left: parent.left; anchors.bottom: parent.bottom
        color: "transparent"; z: 20
        MouseArea {
            anchors.fill: parent; cursorShape: Qt.SizeBDiagCursor
            onPressed: root.startSystemResize(Qt.BottomEdge | Qt.LeftEdge)
        }
    }

    // ================= PROFILE EDIT DIALOG =================
    Dialog {
        id: profileDialog
        width: 340
        height: 480
        modal: true
        padding: 18
        background: Rectangle {
            color: Theme.surface
            radius: 14
            border.color: Theme.glassBorder
            border.width: 1
        }
        // entrance / exit: fade + scale (not a jarring pop)
        enter: Transition {
            NumberAnimation { property: "scale"; from: 0.94; to: 1.0; duration: 200; easing.type: Easing.OutCubic }
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.OutCubic }
        }
        exit: Transition {
            NumberAnimation { property: "scale"; to: 0.96; duration: 160; easing.type: Easing.InCubic }
            NumberAnimation { property: "opacity"; to: 0; duration: 160; easing.type: Easing.InCubic }
        }

        property bool closeHover: false

        function saveProfile() {
            aiService.setUserName(editName.text.trim())
            aiService.setAvatarChar(editAvatar.text.trim())
            root.refreshProfile()
            profileDialog.close()
            islandToast.show("资料已更新~")
        }

        header: Item {
            height: 36
            Text {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: "编辑资料"
                color: Theme.text
                font.pixelSize: 16
                font.bold: true
            }
            // ghost close button (no emoji)
            Text {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: "✕"
                color: profileDialog.closeHover ? Theme.text : Theme.textDim
                font.pixelSize: 15
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: profileDialog.closeHover = true
                    onExited: profileDialog.closeHover = false
                    onClicked: profileDialog.close()
                }
            }
        }

        ColumnLayout {
            width: parent.width
            height: parent.height
            spacing: 12

            // avatar preview — click to change (shows current avatar or char)
            Item {
                Layout.alignment: Qt.AlignHCenter
                width: 96; height: 96
                Rectangle {
                    id: userAvatarPreview
                    width: 72; height: 72; radius: 36
                    anchors.centerIn: parent
                    color: Theme.accent
                    clip: true
                    antialiasing: true
                    border.color: Theme.glassBorder
                    border.width: 1
                    Image {
                        anchors.fill: parent
                        visible: root.userAvatarPath.length > 0
                        source: root.userAvatarPath
                        fillMode: Image.PreserveAspectCrop
                        smooth: true; mipmap: true
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: root.userAvatarPath.length === 0
                        text: editAvatar.text.length ? editAvatar.text : aiService.avatarChar()
                        color: "white"
                        font.pixelSize: 28
                        font.bold: true
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: { fileDialog.avatarTarget = "user"; fileDialog.open() }
                }
                Text {
                    anchors.top: parent.bottom
                    anchors.topMargin: 4
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "点击更换头像"
                    color: Theme.textDim
                    font.pixelSize: 11
                }
            }

            Text { text: "名字"; color: Theme.textDim; font.pixelSize: 12 }
            TextField {
                id: editName
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                color: Theme.text
                text: aiService.userName()
                background: Rectangle { color: Theme.inputBg; radius: 8 }
                Keys.onReturnPressed: profileDialog.saveProfile()
            }

            Text { text: "头像文字（单字）"; color: Theme.textDim; font.pixelSize: 12 }
            TextField {
                id: editAvatar
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                color: Theme.text
                text: aiService.avatarChar()
                maximumLength: 1
                background: Rectangle { color: Theme.inputBg; radius: 8 }
                Keys.onReturnPressed: profileDialog.saveProfile()
            }

            AppButton {
                text: "选择头像图片"
                Layout.fillWidth: true
                implicitHeight: 36
                onClicked: { fileDialog.avatarTarget = "user"; fileDialog.open() }
            }

            Text {
                Layout.fillWidth: true
                text: "AI 的名字、人设和头像：打开聊天后点击左上角 AI 头像修改"
                color: Theme.textDim
                font.pixelSize: 11
                wrapMode: Text.Wrap
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                AppButton {
                    text: "保存"
                    Layout.fillWidth: true
                    onClicked: profileDialog.saveProfile()
                }
                AppButton {
                    text: "取消"
                    variant: "ghost"
                    Layout.fillWidth: true
                    onClicked: profileDialog.close()
                }
            }
        }
    }

    // =====================================================================
    // AI PROFILE — single unified container (no nested dialogs)
    // page 0 = overview (header + category cards)
    // pages 1..6 = detail panels for each category (back button returns)
    // =====================================================================
    DialogContainer {
        id: aiProfileDialog
        dialogTitle: "AI 资料"
        dialogSubtitle: "角色、记忆与关系管理"
        dialogWidth: 600
        dialogHeight: 640

        // detail navigation state
        property int detailPage: 0   // 0=overview, 1..6=detail

        function showDetail(p) {
            aiProfileDialog.detailPage = p
            profileStack.currentIndex = p
            aiProfileDialog.dialogTitle = aiProfileDialog.detailTitle(p)
            aiProfileDialog.dialogSubtitle = aiProfileDialog.detailSubtitle(p)
        }
        function showOverview() {
            aiProfileDialog.detailPage = 0
            profileStack.currentIndex = 0
            aiProfileDialog.dialogTitle = "AI 资料"
            aiProfileDialog.dialogSubtitle = "角色、记忆与关系管理"
        }
        function detailTitle(p) {
            return ["AI 资料", "AI 人设", "共同经历", "与我的关系", "我的兴趣", "未完成话题", "使用统计"][p]
        }
        function detailSubtitle(p) {
            return ["角色、记忆与关系管理",
                    "角色设定和性格",
                    "重要事件记忆",
                    "亲密度和关系设置",
                    "兴趣偏好",
                    "聊天中断的话题",
                    "陪伴时间"][p]
        }

        StackLayout {
            id: profileStack
            anchors.fill: parent
            currentIndex: aiProfileDialog.detailPage

            // ---------------- page 0: overview ----------------
            Item {
                ScrollView {
                    id: profileScroll
                    anchors.fill: parent
                    clip: true
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ColumnLayout {
                        width: profileScroll.availableWidth
                        anchors.margins: Theme.sp5
                        spacing: Theme.sp3

                    ProfileHeader {
                        Layout.fillWidth: true
                        avatarSource: root.aiAvatarPath
                        name: aiService.aiName()
                        status: "在线"
                        statusOnline: true
                        onAvatarClicked: {
                            fileDialog.avatarTarget = "ai"
                            fileDialog.open()
                        }
                    }

                    // category cards
                    SettingCard {
                        Layout.fillWidth: true
                        iconText: "A"
                        title: "AI 人设"
                        description: "角色设定和性格"
                        onClicked: aiProfileDialog.showDetail(1)
                    }
                    SettingCard {
                        Layout.fillWidth: true
                        iconText: "E"
                        title: "共同经历"
                        description: "重要事件记忆"
                        onClicked: aiProfileDialog.showDetail(2)
                    }
                    SettingCard {
                        Layout.fillWidth: true
                        iconText: "R"
                        title: "与我的关系"
                        description: "亲密度和关系设置"
                        onClicked: aiProfileDialog.showDetail(3)
                    }
                    SettingCard {
                        Layout.fillWidth: true
                        iconText: "I"
                        title: "我的兴趣"
                        description: "兴趣偏好"
                        onClicked: aiProfileDialog.showDetail(4)
                    }
                    SettingCard {
                        Layout.fillWidth: true
                        iconText: "T"
                        title: "未完成话题"
                        description: "聊天中断的话题"
                        onClicked: aiProfileDialog.showDetail(5)
                    }
                    SettingCard {
                        Layout.fillWidth: true
                        iconText: "S"
                        title: "使用统计"
                        description: "陪伴时间"
                        onClicked: aiProfileDialog.showDetail(6)
                    }

                    Item { Layout.fillHeight: true }

                    // memory quick actions
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.sp3
                        AppButton {
                            text: "查看记忆"
                            variant: "secondary"
                            Layout.fillWidth: true
                            onClicked: memoryViewDialog.open()
                        }
                        AppButton {
                            text: "添加记忆"
                            variant: "secondary"
                            Layout.fillWidth: true
                            onClicked: {
                                memoryInput.text = ""
                                memoryInputDialog.open()
                            }
                        }
                        AppButton {
                            text: "清空记忆"
                            variant: "secondary"
                            Layout.fillWidth: true
                            borderColor: Qt.rgba(0.77,0.35,0.35,0.55)
                            onClicked: {
                                aiService.clearMemory()
                                islandToast.show("记忆已清空~")
                            }
                        }
                    }
                    }
                }
            }

            // ---------------- page 1: AI 人设 ----------------
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.sp5
                    spacing: Theme.sp3

                    AppButton {
                        text: "‹ 返回"
                        variant: "ghost"
                        implicitWidth: 80
                        onClicked: aiProfileDialog.showOverview()
                    }
                    Text { text: "AI 名字"; color: Theme.textDim; font.pixelSize: Theme.fsSmall }
                    TextField {
                        id: profileAiName
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        color: Theme.text
                        text: aiService.aiName()
                        background: Rectangle { color: Theme.inputBg; radius: Theme.rMd }
                        onAccepted: saveProfileBtn.clicked()
                    }
                    Text { text: "AI 人设"; color: Theme.textDim; font.pixelSize: Theme.fsSmall }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Theme.inputBg
                        radius: Theme.rMd
                        clip: true
                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: Theme.sp2
                            clip: true
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded
                            TextArea {
                                id: profileAiPersonality
                                width: parent.width - Theme.sp2
                                height: Math.max(parent.height, implicitHeight)
                                color: Theme.text
                                text: aiService.aiPersonality()
                                placeholderText: "描述 AI 的性格、语气与陪伴风格…"
                                placeholderTextColor: Theme.textDim
                                wrapMode: TextEdit.Wrap
                                background: null
                            }
                        }
                    }
                    AppButton {
                        id: saveProfileBtn
                        text: "保存 AI 人设"
                        variant: "primary"
                        Layout.fillWidth: true
                        onClicked: {
                            aiService.setAiName(profileAiName.text)
                            aiService.setAiPersonality(profileAiPersonality.text)
                            root.refreshProfile()
                            islandToast.show("AI 资料已保存~")
                        }
                    }
                }
            }

            // ---------------- page 2: 共同经历 (read-only) ----------------
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.sp5
                    spacing: Theme.sp3

                    AppButton {
                        text: "‹ 返回"
                        variant: "ghost"
                        implicitWidth: 80
                        onClicked: aiProfileDialog.showOverview()
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded
                        TextArea {
                            id: eventsText
                            width: parent.width
                            color: Theme.text
                            font.pixelSize: Theme.fsDefault
                            readOnly: true
                            wrapMode: TextEdit.Wrap
                            background: null
                            text: aiService.eventMemoryText(50)
                        }
                    }
                }
            }

            // ---------------- page 3: 与我的关系 ----------------
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.sp5
                    spacing: Theme.sp4

                    AppButton {
                        text: "‹ 返回"
                        variant: "ghost"
                        implicitWidth: 80
                        onClicked: aiProfileDialog.showOverview()
                    }
                    Card {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.sp4
                            spacing: Theme.sp3

                            Text { text: "亲密度"; color: Theme.textDim; font.pixelSize: Theme.fsSmall }
                            RowLayout {
                                Layout.fillWidth: true
                                DarkSlider {
                                    id: relIntimacy
                                    Layout.fillWidth: true
                                    from: 0; to: 100; stepSize: 5
                                    Component.onCompleted: value = aiService.relationshipIntimacy()
                                    onValueChanged: aiService.setRelationship(value, aiService.relationshipTrust())
                                }
                                Text { text: relIntimacy.value; color: Theme.textDim; font.pixelSize: Theme.fsCaption }
                            }
                            Text { text: "信任度"; color: Theme.textDim; font.pixelSize: Theme.fsSmall }
                            RowLayout {
                                Layout.fillWidth: true
                                DarkSlider {
                                    id: relTrust
                                    Layout.fillWidth: true
                                    from: 0; to: 100; stepSize: 5
                                    Component.onCompleted: value = aiService.relationshipTrust()
                                    onValueChanged: aiService.setRelationship(aiService.relationshipIntimacy(), value)
                                }
                                Text { text: relTrust.value; color: Theme.textDim; font.pixelSize: Theme.fsCaption }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: {
                                    var itm = relIntimacy.value
                                    var tr = relTrust.value
                                    var level = "刚认识"
                                    if (itm >= 80) level = "形影不离"
                                    else if (itm >= 60) level = "很亲近"
                                    else if (itm >= 40) level = "好朋友"
                                    else if (itm >= 20) level = "逐渐熟悉"
                                    return "亲密度 " + itm + "，信任度 " + tr + "：" + level + " 的关系。"
                                }
                                color: Theme.textDim
                                font.pixelSize: Theme.fsSmall
                                wrapMode: Text.Wrap
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }
                }
            }

            // ---------------- page 4: 我的兴趣 ----------------
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.sp5
                    spacing: Theme.sp3

                    AppButton {
                        text: "‹ 返回"
                        variant: "ghost"
                        implicitWidth: 80
                        onClicked: aiProfileDialog.showOverview()
                    }
                    ListView {
                        id: interestsListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: Theme.sp2
                        model: aiService.interestList()
                        delegate: RowLayout {
                            width: interestsListView.width
                            spacing: Theme.sp3
                            Text {
                                Layout.fillWidth: true
                                text: modelData
                                color: Theme.text
                                font.pixelSize: Theme.fsDefault
                                elide: Text.ElideRight
                            }
                            AppButton {
                                text: "删除"
                                variant: "ghost"
                                implicitHeight: 30
                                onClicked: {
                                    aiService.removeInterest(modelData)
                                    interestsListView.model = aiService.interestList()
                                }
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.sp3
                        TextField {
                            id: interestInput
                            Layout.fillWidth: true
                            Layout.preferredHeight: 38
                            color: Theme.text
                            placeholderText: "添加一个兴趣（如 Minecraft）"
                            placeholderTextColor: Theme.textDim
                            background: Rectangle { color: Theme.inputBg; radius: Theme.rMd }
                            onAccepted: addInterestBtn.clicked()
                        }
                        AppButton {
                            id: addInterestBtn
                            text: "添加"
                            variant: "secondary"
                            implicitHeight: 38
                            onClicked: {
                                aiService.addInterest(interestInput.text)
                                interestInput.text = ""
                                interestsListView.model = aiService.interestList()
                            }
                        }
                    }
                }
            }

            // ---------------- page 5: 未完成话题 ----------------
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.sp5
                    spacing: Theme.sp3

                    AppButton {
                        text: "‹ 返回"
                        variant: "ghost"
                        implicitWidth: 80
                        onClicked: aiProfileDialog.showOverview()
                    }
                    ListView {
                        id: topicsListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: Theme.sp2
                        model: aiService.topicList()
                        delegate: RowLayout {
                            width: topicsListView.width
                            spacing: Theme.sp3
                            Text {
                                Layout.fillWidth: true
                                text: modelData
                                color: Theme.text
                                font.pixelSize: Theme.fsDefault
                                wrapMode: Text.Wrap
                            }
                            AppButton {
                                text: "删除"
                                variant: "ghost"
                                implicitHeight: 30
                                onClicked: {
                                    aiService.removeTopic(modelData)
                                    topicsListView.model = aiService.topicList()
                                }
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.sp3
                        TextField {
                            id: topicInput
                            Layout.fillWidth: true
                            Layout.preferredHeight: 38
                            color: Theme.text
                            placeholderText: "添加一个聊到一半的话题"
                            placeholderTextColor: Theme.textDim
                            background: Rectangle { color: Theme.inputBg; radius: Theme.rMd }
                            onAccepted: addTopicBtn.clicked()
                        }
                        AppButton {
                            id: addTopicBtn
                            text: "添加"
                            variant: "secondary"
                            implicitHeight: 38
                            onClicked: {
                                aiService.addTopic(topicInput.text)
                                topicInput.text = ""
                                topicsListView.model = aiService.topicList()
                            }
                        }
                    }
                }
            }

            // ---------------- page 6: 使用统计 ----------------
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.sp5
                    spacing: Theme.sp3

                    AppButton {
                        text: "‹ 返回"
                        variant: "ghost"
                        implicitWidth: 80
                        onClicked: aiProfileDialog.showOverview()
                    }
                    Card {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: Theme.sp3
                            clip: true
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded
                            TextArea {
                                id: usageTextArea
                                width: parent.width
                                color: Theme.text
                                font.pixelSize: Theme.fsDefault
                                readOnly: true
                                wrapMode: TextEdit.Wrap
                                background: null
                                text: aiService.usageText() + "\n\n" + aiService.uptimeText()
                            }
                        }
                    }
                }
            }
        }
    }

    // memory viewer dialog: shows the AI's full memory report
    DialogContainer {
        id: memoryViewDialog
        dialogTitle: "查看记忆"
        dialogSubtitle: "AI 记住的关于你的一切"
        dialogWidth: 560
        dialogHeight: 520
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.sp4
            spacing: Theme.sp3

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.inputBg
                radius: Theme.rMd
                ScrollView {
                    anchors.fill: parent
                    anchors.margins: Theme.sp2
                    clip: true
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    TextArea {
                        id: memoryReportText
                        width: parent.width
                        color: Theme.text
                        font.pixelSize: Theme.fsSmall
                        readOnly: true
                        wrapMode: TextEdit.Wrap
                        background: null
                        text: aiService.memoryDetail()
                        // hint when entering edit mode
                        onReadOnlyChanged: { if (!readOnly) text = aiService.memoryRaw() }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.sp3
                AppButton {
                    text: memoryReportText.readOnly ? "编辑原始记忆" : "取消编辑"
                    variant: "secondary"
                    Layout.fillWidth: true
                    onClicked: {
                        if (memoryReportText.readOnly) {
                            // switch to raw JSON for editing
                            memoryReportText.readOnly = false
                        } else {
                            memoryReportText.readOnly = true
                            memoryReportText.text = aiService.memoryDetail()
                        }
                    }
                }
                AppButton {
                    text: "保存修改"
                    variant: "primary"
                    Layout.fillWidth: true
                    enabled: !memoryReportText.readOnly
                    onClicked: {
                        aiService.setMemoryRaw(memoryReportText.text)
                        memoryReportText.readOnly = true
                        memoryReportText.text = aiService.memoryDetail()
                        islandToast.show("记忆已保存~")
                    }
                }
            }
        }
    }

    // small add-memory dialog (kept minimal on purpose)
    DialogContainer {
        id: memoryInputDialog
        dialogTitle: "添加记忆"
        dialogSubtitle: "告诉 AI 一件值得记住的事"
        dialogWidth: 460
        dialogHeight: 300
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.sp5
            spacing: Theme.sp3

            TextArea {
                id: memoryInput
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.text
                placeholderText: "比如：我喜欢喝奶茶，讨厌下雨天…"
                placeholderTextColor: Theme.textDim
                wrapMode: TextEdit.Wrap
                background: Rectangle { color: Theme.inputBg; radius: Theme.rMd }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.sp3
                AppButton {
                    text: "记住"
                    variant: "primary"
                    Layout.fillWidth: true
                    onClicked: {
                        aiService.addMemoryNote(memoryInput.text)
                        islandToast.show("记住了~")
                        memoryInputDialog.close()
                    }
                }
                AppButton {
                    text: "取消"
                    variant: "secondary"
                    Layout.fillWidth: true
                    onClicked: memoryInputDialog.close()
                }
            }
        }
    }

    // ================= ISLAND TOAST (single instance) =================
    IslandToast {
        id: islandToast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 16
        z: 999
        onActionChosen: function(index) { appCore.selectIslandAction(index) }
    }

    Connections {
        target: appCore
        function onToastRequested() {
            islandToast.show(appCore.toastMessage)
        }
        function onIslandRequested(message, options, actionId) {
            if (options.length > 0)
                islandToast.showChoice(message, options)
            else
                islandToast.show(message)
        }
        function onIslandActionChosen(actionId, index) {
            // single decision point for island actions
            if (actionId === "open_proxy") {
                var which = index === 0 ? "Clash Verge" : "v2rayN"
                appCore.setStatus("打开梯子中...")
                var ok = index === 0 ? proxyService.launchClash() : proxyService.launchV2ray()
                islandToast.show(ok ? "已打开 " + which : "打开失败")
                if (ok) statsService.record("proxy", which)
            }
        }
    }

    // chat page open state
    property bool chatOpen: false
    property bool appReady: false

    // unread badge for proactive AI messages
    property int unreadCount: 0
    function bumpUnread() {
        root.unreadCount++
    }
    function clearUnread() {
        root.unreadCount = 0
    }

    // avatar image paths (empty = char avatar), stored as file:// URLs
    property string aiAvatarPath: ""
    property string userAvatarPath: ""

    // file picker for avatar upload
    FileDialog {
        id: fileDialog
        property string avatarTarget: ""
        nameFilters: ["图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)", "所有文件 (*.*)"]
        onAccepted: {
            var url = fileDialog.selectedFile.toString()
            var p = url.replace("file:///", "").replace("file://", "")
            if (p.length > 0) {
                if (fileDialog.avatarTarget === "user") {
                    aiService.setUserAvatar(p)
                    root.userAvatarPath = toFileUrl(aiService.userAvatarPath())
                } else {
                    aiService.setAiAvatar(p)
                    root.aiAvatarPath = toFileUrl(aiService.aiAvatarPath())
                }
                islandToast.show("头像已更新~")
            }
        }
    }

    // startup: greeting shown once via island toast for 20s; AI knows what was said
    Component.onCompleted: {
        root.appReady = true
        // load avatars (files may appear after first upload)
        root.aiAvatarPath = toFileUrl(aiService.aiAvatarPath())
        root.userAvatarPath = toFileUrl(aiService.userAvatarPath())
        root.refreshWallpaper()
        root.refreshContacts()
        root.aiGreeting = aiService.greeting()
        if (aiService.shouldGreetToday()) {
            // show once on the island AND persist into chat history so the
            // greeting reads like a real first message from the AI
            islandToast.show(root.aiGreeting, 20000)
            chatPage.insertAiMessage(root.aiGreeting)
            aiService.generateGreeting()
            aiService.markGreeted()
        }
        // warn when running from a stray copy (not the installed location)
        if (!appCore.isProperLocation())
            locationWarnTimer.start()
        idleTimer.start()
        restTimer.start()
    }

    Timer {
        id: locationWarnTimer
        interval: 2500
        repeat: false
        onTriggered: {
            islandToast.show("当前是旧副本，请用安装器重新安装到 Program Files", 6000)
        }
    }

    Connections {
        target: aiService
        function onChatReply(text) {
            chatPage.appendAi(text)
        }
        function onIdleReply(text) {
            // proactive message from the AI:
            // - if the window is active AND chat page is open for the current AI -> no sound / no badge
            // - otherwise (backgrounded, minimized, or chat closed) play sound and show the unread dot
            if (root.active && root.chatOpen) {
                root.clearUnread()
                return
            }
            appCore.playNotify()
            root.bumpUnread()
        }
        function onGreetingReady(text) {
            // async regeneration: keep the new text for future sessions,
            // but do NOT re-show / re-insert (already done once at startup)
            root.aiGreeting = text
        }
        function onProfileChanged() {
            root.refreshProfile()
        }
    }

    Connections {
        target: chatPage
        function onMessageSaved(contactId, isAi, msg) {
            // update the preview for this contact immediately (latest wins),
            // regardless of DB state — refresh afterwards for consistency
            for (var i = 0; i < contactModel.count; i++) {
                var it = contactModel.get(i)
                if (it.cid === contactId) {
                    var preview = (isAi ? "" : "我: ") + msg
                    contactModel.set(i, { "cid": it.cid, "cname": it.cname, "hasAvatar": it.hasAvatar, "avatarUrl": it.avatarUrl, "lastMsg": preview })
                    break
                }
            }
            // also refresh from DB so restart-time state stays consistent
            root.refreshContacts()
        }
    }

    Connections {
        target: contactService
        function onContactsChanged() {
            if (root.appReady) {
                root.refreshContacts()
                root.refreshProfile()
            }
        }
    }

    // ================= PROACTIVE CHAT (every ~30 min, randomized) =================
    // AI proactively sends a message on a 25~35 min random schedule (~30 min
    // base with jitter, so it never feels like clockwork).
    // Coding/busy foreground never interrupted; the next attempt is scheduled
    // from the moment a message was actually sent (no spam on game sessions).
    // NOTE: must be `var` (not `int`) — Date.now() is a 13-digit ms timestamp
    // that overflows QML's 32-bit int and becomes negative, which would make
    // `Date.now() >= nextProactiveAt` always true and spam proactive chats.
    property var nextProactiveAt: 0

    function scheduleNextProactive() {
        var mins = 25 + Math.floor(Math.random() * 11) // 25..35 min
        root.nextProactiveAt = Date.now() + mins * 60 * 1000
    }

    // reset the proactive schedule when the window regains focus / user interacts
    onActiveChanged: {
        if (root.active) root.scheduleNextProactive()
    }
    function onUserActivity() {
        root.scheduleNextProactive()
    }

    Timer {
        id: idleTimer
        interval: 30000
        repeat: true
        onTriggered: {
            if (!root.nextProactiveAt) root.scheduleNextProactive()
            // coding / busy foreground -> never interrupt (schedule holds)
            if (aiService.userActivityState() === "coding") return
            if (Date.now() >= root.nextProactiveAt) {
                root.scheduleNextProactive()
                // feed the AI the last 3 chat messages so it can start a
                // topic based on what was actually being discussed
                var recent = chatPage.recentMessages(3)
                if (recent.length > 0)
                    aiService.setChatHistory(recent.join("\n"))
                aiService.idleChat()
                islandToast.show(aiService.aiName() + "想找你聊聊~", 6000)
            }
        }
    }

    // ================= long-session reminder: nudge to rest after long continuous use =================
    property var appStartTime: Date.now()
    property bool restReminded: false
    Timer {
        id: restTimer
        interval: 30 * 60 * 1000   // check every 30 min
        repeat: true
        onTriggered: {
            if (root.restReminded) return
            var elapsedH = (Date.now() - root.appStartTime) / 3600000
            if (elapsedH >= 3) {
                root.restReminded = true
                islandToast.show("已经连续使用 3 小时了，起来活动一下吧~", 8000)
            }
        }
    }
}
