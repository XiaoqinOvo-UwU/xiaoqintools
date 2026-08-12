import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
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
    color: Theme.bg

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

    // contact list model (id|name|hasAvatar)
    ListModel { id: contactModel }

    function refreshContacts() {
        contactModel.clear()
        var list = contactService.contactList()
        for (var i = 0; i < list.length; i++) {
            var parts = list[i].split("|")
            if (parts.length >= 3) {
                var avatarUrl = ""
                if (parts[2] === "1") {
                    var raw = contactService.contactAvatarPath(parts[0])
                    avatarUrl = raw.length > 0 ? "file:///" + raw.replace(/\\/g, "/") : ""
                }
                contactModel.append({ "cid": parts[0], "cname": parts[1], "hasAvatar": parts[2], "avatarUrl": avatarUrl })
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

    RowLayout {
        anchors.fill: parent
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

                        Text {
                            text: "⌄"
                            color: Theme.textDim
                            font.pixelSize: 16
                            visible: userBtn.hovered
                            opacity: userBtn.hovered ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: 150 } }
                        }
                    }
                    MouseArea {
                        id: userBtn
                        anchors.fill: parent
                        hoverEnabled: true
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
                        text: "✏️ 编辑资料"
                        onClicked: profileDialog.open()
                    }
                    MenuItem {
                        text: "⚙ 设置"
                        onClicked: root.currentPage = 3
                    }
                    MenuItem {
                        text: "💾 导出配置"
                        onClicked: {
                            var ok = syncService.exportConfig(syncService.defaultExportPath())
                            islandToast.show(ok ? "配置已导出到杂货铺" : "导出失败")
                        }
                    }
                    MenuItem {
                        text: "❌ 退出"
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
                                Image {
                                    anchors.fill: parent
                                    visible: avatarUrl.length > 0
                                    source: avatarUrl
                                    fillMode: Image.PreserveAspectCrop
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
                                    visible: cid === contactService.currentId() && root.unreadCount > 0 && !root.chatOpen
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
                                Text {
                                    text: "点击开始聊天"
                                    color: Theme.textDim
                                    font.pixelSize: 10
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

                Rectangle { // separator
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.glassBorder
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                }

                Item { Layout.fillHeight: true }

                // ---- nav items (lower-middle) ----
                Repeater {
                    model: ListModel {
                        ListElement { icon: "🚀"; label: "网络" }
                        ListElement { icon: "🖥"; label: "系统" }
                        ListElement { icon: "🎵"; label: "娱乐" }
                        ListElement { icon: "⚙"; label: "设置" }
                    }
                    Rectangle {
                        id: navItem
                        property bool hovered: false
                        property int myIndex: index
                        // when the chat page is open, no nav tab stays highlighted
                        readonly property bool active: !root.chatOpen && root.currentPage === myIndex
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        color: active ? Theme.selected
                             : hovered ? Theme.hoverBg
                             : "transparent"
                        Behavior on color { ColorAnimation { duration: 150 } }

                        Rectangle {
                            width: 3; height: active ? 22 : 0
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            color: Theme.textDim
                            radius: 1.5
                            opacity: active ? 1 : 0
                            Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                            Behavior on opacity { NumberAnimation { duration: 180 } }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 18
                            anchors.rightMargin: 16
                            spacing: 12
                            Text {
                                text: model.icon
                                font.pixelSize: 17
                                Layout.preferredWidth: 22
                                opacity: active ? 1 : 0.7
                                Behavior on opacity { NumberAnimation { duration: 150 } }
                            }
                            Text {
                                text: model.label
                                color: active ? "white" : Theme.textDim
                                font.pixelSize: 14
                                font.bold: active
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: navItem.hovered = true
                            onExited: navItem.hovered = false
                            onClicked: {
                                root.currentPage = index
                                root.chatOpen = false
                            }                        }
                    }
                }

                // ---- bottom: about line ----
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    color: "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "小钦的工具 v3.3.1"
                        color: Theme.textDim
                        font.pixelSize: 11
                    }
                }
            }
        }

        // ================= MAIN CONTENT =================
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.bg

            StackLayout {
                id: pageStack
                anchors.fill: parent
                currentIndex: root.currentPage

                onCurrentIndexChanged: {
                    var item = pageStack.currentItem
                    if (item) {
                        item.opacity = 0
                        item.scale = 0.995
                        itemBehavior.target = item
                        itemBehavior.restart()
                    }
                }

                SequentialAnimation {
                    id: itemBehavior
                    property Item target
                    ParallelAnimation {
                        NumberAnimation { target: itemBehavior.target; property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.OutCubic }
                        NumberAnimation { target: itemBehavior.target; property: "scale"; from: 0.995; to: 1; duration: 200; easing.type: Easing.OutCubic }
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
                onBackRequested: chatPage.closeWithAnim()
                onCloseFinished: root.chatOpen = false
                onAiProfileRequested: aiProfileDialog.open()
            }
        }
    }

    // ================= PROFILE EDIT DIALOG =================
    Dialog {
        id: profileDialog
        width: 340
        height: 560
        modal: true
        padding: 18
        background: Rectangle {
            color: Theme.surface
            radius: 14
            border.color: Theme.glassBorder
            border.width: 1
        }
        header: Item {
            height: 36
            Text {
                anchors.centerIn: parent
                text: "编辑资料"
                color: Theme.text
                font.pixelSize: 16
                font.bold: true
            }
        }
        ColumnLayout {
            width: parent.width
            height: parent.height
            spacing: 10

            Text { text: "名字"; color: Theme.textDim; font.pixelSize: 12 }
            TextField {
                id: editName
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                color: Theme.text
                text: aiService.userName()
                background: Rectangle { color: Theme.inputBg; radius: 8 }
            }
            Text { text: "头像文字"; color: Theme.textDim; font.pixelSize: 12 }
            TextField {
                id: editAvatar
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                color: Theme.text
                text: aiService.avatarChar()
                maximumLength: 1
                background: Rectangle { color: Theme.inputBg; radius: 8 }
            }
            AppButton {
                text: "🖼 用户头像"
                Layout.fillWidth: true
                implicitHeight: 32
                onClicked: {
                    fileDialog.avatarTarget = "user"
                    fileDialog.open()
                }
            }
            Text { text: "AI 人设"; color: Theme.textDim; font.pixelSize: 12 }
            Text {
                Layout.fillWidth: true
                text: "AI 名字、人设和头像请到「设置 → AI 配置」里修改"
                color: Theme.textDim
                font.pixelSize: 11
                wrapMode: Text.Wrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                AppButton {
                    text: "保存"
                    Layout.fillWidth: true
                    onClicked: {
                        aiService.setUserName(editName.text.trim())
                        aiService.setAvatarChar(editAvatar.text.trim())
                        root.refreshProfile()
                        profileDialog.close()
                        islandToast.show("资料已更新~")
                    }
                }
                AppButton {
                    text: "取消"
                    Layout.fillWidth: true
                    onClicked: profileDialog.close()
                }
            }
        }
    }

    // ================= AI PROFILE DIALOG (click AI avatar in chat) =================
    Dialog {
        id: aiProfileDialog
        width: 440
        height: Math.min(560, profileCol.implicitHeight + 110)
        modal: true
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        background: Rectangle {
            color: Theme.surface
            radius: 14
            border.color: Theme.glassBorder
            border.width: 1
        }
        header: Item {
            height: 44
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 8
                spacing: 10
                Rectangle {
                    width: 34; height: 34
                    radius: 17
                    color: Theme.accent
                    clip: true
                    Image {
                        anchors.fill: parent
                        visible: root.aiAvatarPath.length > 0
                        source: root.aiAvatarPath
                        fillMode: Image.PreserveAspectCrop
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: root.aiAvatarPath.length === 0
                        text: aiService.aiName().length > 0 ? aiService.aiName().charAt(0) : "A"
                        color: "white"
                        font.pixelSize: 15
                        font.bold: true
                    }
                }
                Text {
                    text: "AI 资料"
                    color: Theme.text
                    font.pixelSize: 16
                    font.bold: true
                    Layout.fillWidth: true
                }
                AppButton {
                    text: "✕"
                    implicitWidth: 30
                    implicitHeight: 30
                    btnRadius: 15
                    onClicked: aiProfileDialog.close()
                }
            }
        }
        contentItem: ScrollView {
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ColumnLayout {
                id: profileCol
                width: aiProfileDialog.width - 36
                spacing: 10

                Text { text: "AI 名字"; color: Theme.textDim; font.pixelSize: 12 }
                TextField {
                    id: profileAiName
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    color: Theme.text
                    text: aiService.aiName()
                    background: Rectangle { color: Theme.inputBg; radius: 8 }
                }
                Text { text: "AI 人设"; color: Theme.textDim; font.pixelSize: 12 }
                TextArea {
                    id: profileAiPersonality
                    Layout.fillWidth: true
                    // grow with content so long personas are fully visible & editable
                    implicitHeight: Math.max(90, contentHeight + 20)
                    color: Theme.text
                    text: aiService.aiPersonality()
                    wrapMode: TextEdit.Wrap
                    background: Rectangle { color: Theme.inputBg; radius: 8 }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    AppButton {
                        text: "🖼 上传头像"
                        Layout.fillWidth: true
                        implicitHeight: 34
                        onClicked: {
                            fileDialog.avatarTarget = "ai"
                            fileDialog.open()
                        }
                    }
                    AppButton {
                        text: "保存资料"
                        Layout.fillWidth: true
                        implicitHeight: 34
                        onClicked: {
                            aiService.setAiName(profileAiName.text)
                            aiService.setAiPersonality(profileAiPersonality.text)
                            root.refreshProfile()
                            islandToast.show("AI 资料已保存~")
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Qt.rgba(255,255,255,0.08)
                }

                Text { text: "AI 记忆"; color: Theme.text; font.pixelSize: 14; font.bold: true }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    AppButton {
                        text: "📖 查看记忆"
                        Layout.fillWidth: true
                        implicitHeight: 34
                        onClicked: memoryDialog.open()
                    }
                    AppButton {
                        text: "➕ 添加记忆"
                        Layout.fillWidth: true
                        implicitHeight: 34
                        onClicked: {
                            memoryInput.text = ""
                            memoryInputDialog.open()
                        }
                    }
                    AppButton {
                        text: "🗑 清空"
                        Layout.fillWidth: true
                        implicitHeight: 34
                        glassColor: Qt.rgba(0.77,0.35,0.35,0.35)
                        onClicked: {
                            aiService.clearMemory()
                            islandToast.show("记忆已清空~")
                        }
                    }
                }

                Item { Layout.preferredHeight: 10 }
            }
        }
    }

    // ================= MEMORY DIALOGS =================
    Dialog {
        id: memoryDialog
        width: 460
        height: 420
        modal: true
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        background: Rectangle {
            color: Theme.surface
            radius: 14
            border.color: Theme.glassBorder
            border.width: 1
        }
        header: Item {
            height: 40
            Text {
                anchors.centerIn: parent
                text: "AI 的记忆"
                color: Theme.text
                font.pixelSize: 16
                font.bold: true
            }
            AppButton {
                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                text: "✕"
                implicitWidth: 30
                implicitHeight: 30
                btnRadius: 15
                onClicked: memoryDialog.close()
            }
        }
        contentItem: ScrollView {
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            TextArea {
                readOnly: true
                color: Theme.text
                font.pixelSize: 13
                wrapMode: TextEdit.Wrap
                text: aiService.memoryDetail()
                background: null
            }
        }
    }

    Dialog {
        id: memoryInputDialog
        width: 420
        height: 220
        modal: true
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        background: Rectangle {
            color: Theme.surface
            radius: 14
            border.color: Theme.glassBorder
            border.width: 1
        }
        header: Item {
            height: 40
            Text {
                anchors.centerIn: parent
                text: "添加记忆"
                color: Theme.text
                font.pixelSize: 16
                font.bold: true
            }
        }
        contentItem: ColumnLayout {
            spacing: 10
            Text {
                text: "告诉 AI 一件值得记住的事（比如你的喜好、习惯）"
                color: Theme.textDim
                font.pixelSize: 12
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            TextArea {
                id: memoryInput
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.text
                placeholderText: "比如：我喜欢喝奶茶，讨厌下雨天..."
                placeholderTextColor: Theme.textDim
                wrapMode: TextEdit.Wrap
                background: Rectangle { color: Theme.inputBg; radius: 8 }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                AppButton {
                    text: "记住"
                    Layout.fillWidth: true
                    onClicked: {
                        aiService.addMemoryNote(memoryInput.text)
                        islandToast.show("记住了~")
                        memoryInputDialog.close()
                    }
                }
                AppButton {
                    text: "取消"
                    Layout.fillWidth: true
                    onClicked: memoryInputDialog.close()
                }
            }
        }
    }

    // ================= ISLAND TOAST =================
    IslandToast {
        id: islandToast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 16
        z: 999
    }

    Connections {
        target: appCore
        function onToastRequested() {
            islandToast.show(appCore.toastMessage)
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
        root.refreshContacts()
        root.aiGreeting = aiService.greeting()
        if (aiService.shouldGreetToday()) {
            islandToast.show(root.aiGreeting, 20000)
            aiService.generateGreeting()
            aiService.markGreeted()
        }
        // warn when running from a stray copy (not the installed location)
        if (!appCore.isProperLocation())
            locationWarnTimer.start()
        idleTimer.start()
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
            // - if the chat page is open for the current AI, no sound / no badge
            // - otherwise play the notification sound and show the unread dot
            if (root.chatOpen) {
                root.clearUnread()
                return
            }
            appCore.playNotify()
            root.bumpUnread()
        }
        function onGreetingReady(text) {
            root.aiGreeting = text
            islandToast.show(text, 20000)
        }
        function onProfileChanged() {
            root.refreshProfile()
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

    // ================= IDLE DETECTION (10min no interaction + no game -> AI chats) =================
    property int lastInteraction: Date.now()
    property bool idleChatDone: false

    // reset idle timer when window regains focus / user interacts
    onActiveChanged: {
        if (root.active) {
            root.lastInteraction = Date.now()
            root.idleChatDone = false
        }
    }
    function onUserActivity() {
        root.lastInteraction = Date.now()
        root.idleChatDone = false
    }

    Timer {
        id: idleTimer
        interval: 30000
        repeat: true
        onTriggered: {
            var idleMs = Date.now() - root.lastInteraction
            if (idleMs >= 10 * 60 * 1000 && !root.idleChatDone) {
                if (!aiService.isGameRunning()) {
                    root.idleChatDone = true
                    aiService.idleChat()
                    islandToast.show(aiService.aiName() + "想找你聊聊~", 6000)
                }
            }
        }
    }
}
