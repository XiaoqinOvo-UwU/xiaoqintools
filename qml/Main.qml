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
                if (parts[2] === "1") {
                    var raw = contactService.contactAvatarPath(parts[0])
                    avatarUrl = raw.length > 0 ? "file:///" + raw.replace(/\\/g, "/") : ""
                }
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
                        text: "小钦的工具 v3.5.20"
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
        height: 600
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
        contentItem: ColumnLayout {
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
                // scrollable editor: ScrollView provides the scrollbar & wheel
                // scrolling (same pattern as the note list), so long personas
                // never stretch the dialog
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 160
                    color: Theme.inputBg
                    radius: 8
                    clip: true
                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 8
                        clip: true
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded
                        TextArea {
                            id: profileAiPersonality
                            width: parent.width - 8
                            height: Math.max(parent.height, implicitHeight)
                            color: Theme.text
                            text: aiService.aiPersonality()
                            wrapMode: TextEdit.Wrap
                            background: null
                        }
                    }
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

    // ================= MEMORY DIALOGS =================
    // main memory dialog: category list, each opens its own viewer/editor
    Dialog {
        id: memoryDialog
        width: 500
        height: 480
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
            ColumnLayout {
                width: parent.width
                spacing: 8

                // 用户笔记
                AppButton {
                    Layout.fillWidth: true
                    implicitHeight: 40
                    text: "📝 用户笔记（你告诉过我的事）"
                    onClicked: { notesDialog.open() }
                }
                // 共同经历
                AppButton {
                    Layout.fillWidth: true
                    implicitHeight: 40
                    text: "📌 共同经历（事件记忆）"
                    onClicked: { memoryCat.openCat("共同经历", aiService.eventMemoryText(50), false) }
                }
                // 关系
                AppButton {
                    Layout.fillWidth: true
                    implicitHeight: 40
                    text: "💞 与我的关系（亲密度/信任度，可调整）"
                    onClicked: { relationshipDialog.open() }
                }
                // 兴趣
                AppButton {
                    Layout.fillWidth: true
                    implicitHeight: 40
                    text: "⭐ 我的兴趣（可编辑）"
                    onClicked: { interestsDialog.open() }
                }
                // 未完成话题
                AppButton {
                    Layout.fillWidth: true
                    implicitHeight: 40
                    text: "💬 未完成话题（可编辑）"
                    onClicked: { topicsDialog.open() }
                }
                // 使用时长
                AppButton {
                    Layout.fillWidth: true
                    implicitHeight: 40
                    text: "⏱ 使用时长"
                    onClicked: { memoryCat.openCat("时长", aiService.usageText(), false) }
                }
            }
        }
    }

    // generic category dialog: view-only or editable (JSON-ish text)
    Dialog {
        id: memoryCat
        property string catTitle: ""
        property bool editable: false
        property bool isRelationship: false
        width: 500
        height: 460
        modal: true
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        background: Rectangle {
            color: Theme.surface
            radius: 14
            border.color: Theme.glassBorder
            border.width: 1
        }
        function openCat(title, content, edit, rel) {
            memoryCat.catTitle = title
            memoryCat.editable = edit
            memoryCat.isRelationship = rel || false
            catViewText.text = content
            catEditText.text = content
            catEditText.visible = edit
            catViewText.visible = !edit
            saveBtn.visible = edit
            memoryCat.open()
        }
        header: Item {
            height: 40
            Text {
                anchors.centerIn: parent
                text: memoryCat.catTitle
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
                onClicked: memoryCat.close()
            }
        }
        contentItem: ColumnLayout {
            spacing: 8
            TextArea {
                id: catViewText
                Layout.fillWidth: true
                Layout.fillHeight: true
                readOnly: true
                color: Theme.text
                font.pixelSize: 13
                wrapMode: TextEdit.Wrap
                background: null
            }
            TextArea {
                id: catEditText
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.text
                font.pixelSize: 12
                wrapMode: TextEdit.Wrap
                placeholderText: "可编辑的配置内容（JSON 格式）"
            }
            AppButton {
                id: saveBtn
                Layout.fillWidth: true
                implicitHeight: 36
                text: "💾 保存"
                visible: false
                onClicked: {
                    var t = catEditText.text.trim()
                    if (memoryCat.catTitle === "兴趣") aiService.setInterestsRaw(t)
                    else if (memoryCat.catTitle === "未完成话题") aiService.setUnfinishedRaw(t)
                    else if (memoryCat.catTitle === "关系") {
                        try {
                            var robj = JSON.parse(t)
                            aiService.setRelationship(parseInt(robj.intimacy) || 0, parseInt(robj.trust) || 0)
                            islandToast.show("关系已更新~")
                            memoryCat.close()
                            return
                        } catch (e) { islandToast.show("格式错误：需要 {\"intimacy\":70,\"trust\":80}") }
                    }
                    islandToast.show(memoryCat.catTitle + " 已保存~")
                    memoryCat.close()
                }
            }
        }
    }

    // ================= FRIENDLY CATEGORY DIALOGS =================
    // 用户笔记：列表 + 增删
    Dialog {
        id: notesDialog
        width: 460
        height: 420
        modal: true
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        background: Rectangle { color: Theme.surface; radius: 14; border.color: Theme.glassBorder; border.width: 1 }
        header: Item {
            height: 40
            Text { anchors.centerIn: parent; text: "用户笔记"; color: Theme.text; font.pixelSize: 16; font.bold: true }
            AppButton {
                anchors.right: parent.right; anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                text: "✕"; implicitWidth: 30; implicitHeight: 30; btnRadius: 15
                onClicked: notesDialog.close()
            }
        }
        contentItem: ColumnLayout {
            spacing: 8
            ListView {
                id: notesListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 4
                model: aiService.noteList()
                delegate: RowLayout {
                    width: notesListView.width
                    spacing: 8
                    Text {
                        Layout.fillWidth: true
                        text: modelData
                        color: Theme.text
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                    }
                    AppButton {
                        text: "删"
                        implicitWidth: 34; implicitHeight: 30
                        onClicked: {
                            aiService.removeNote(index)
                            notesListView.model = aiService.noteList()
                        }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                TextField {
                    id: noteInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    color: Theme.text
                    placeholderText: "告诉 AI 一件值得记住的事..."
                    background: Rectangle { color: Theme.inputBg; radius: 8 }
                }
                AppButton {
                    text: "添加"
                    implicitHeight: 36
                    onClicked: {
                        aiService.addMemoryNote(noteInput.text)
                        noteInput.text = ""
                        notesListView.model = aiService.noteList()
                    }
                }
            }
        }
    }

    Dialog {
        id: relationshipDialog
        width: 440
        height: 320
        modal: true
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        background: Rectangle { color: Theme.surface; radius: 14; border.color: Theme.glassBorder; border.width: 1 }
        header: Item {
            height: 40
            Text { anchors.centerIn: parent; text: "与我的关系"; color: Theme.text; font.pixelSize: 16; font.bold: true }
            AppButton {
                anchors.right: parent.right; anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                text: "✕"; implicitWidth: 30; implicitHeight: 30; btnRadius: 15
                onClicked: relationshipDialog.close()
            }
        }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: "亲密度"; color: Theme.textDim; font.pixelSize: 12 }
            RowLayout {
                Layout.fillWidth: true
                DarkSlider {
                    id: relIntimacy
                    Layout.fillWidth: true
                    from: 0; to: 100; stepSize: 5
                    Component.onCompleted: value = aiService.relationshipIntimacy()
                    onValueChanged: aiService.setRelationship(value, aiService.relationshipTrust())
                }
                Text { text: relIntimacy.value; color: Theme.textDim; font.pixelSize: 11 }
            }
            Text { text: "信任度"; color: Theme.textDim; font.pixelSize: 12 }
            RowLayout {
                Layout.fillWidth: true
                DarkSlider {
                    id: relTrust
                    Layout.fillWidth: true
                    from: 0; to: 100; stepSize: 5
                    Component.onCompleted: value = aiService.relationshipTrust()
                    onValueChanged: aiService.setRelationship(aiService.relationshipIntimacy(), value)
                }
                Text { text: relTrust.value; color: Theme.textDim; font.pixelSize: 11 }
            }
            Text {
                Layout.fillWidth: true
                // live text: recomputes whenever the sliders move
                text: {
                    var itm = relIntimacy.value
                    var tr = relTrust.value
                    var level = "刚认识"
                    if (itm >= 80) level = "形影不离"
                    else if (itm >= 60) level = "很亲近"
                    else if (itm >= 40) level = "好朋友"
                    else if (itm >= 20) level = "逐渐熟悉"
                    return "亲密度 " + itm + "，信任度 " + tr + "：" + level + " 的关系。AI 会记住我们认识的深度，保持关系连续性。"
                }
                color: Theme.textDim
                font.pixelSize: 11
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Dialog {
        id: interestsDialog
        width: 460
        height: 420
        modal: true
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        background: Rectangle { color: Theme.surface; radius: 14; border.color: Theme.glassBorder; border.width: 1 }
        header: Item {
            height: 40
            Text { anchors.centerIn: parent; text: "我的兴趣"; color: Theme.text; font.pixelSize: 16; font.bold: true }
            AppButton {
                anchors.right: parent.right; anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                text: "✕"; implicitWidth: 30; implicitHeight: 30; btnRadius: 15
                onClicked: interestsDialog.close()
            }
        }
        contentItem: ColumnLayout {
            spacing: 8
            ListView {
                id: interestsListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 4
                model: aiService.interestList()
                delegate: RowLayout {
                    width: interestsListView.width
                    spacing: 8
                    Text {
                        Layout.fillWidth: true
                        text: modelData
                        color: Theme.text
                        font.pixelSize: 12
                    }
                    AppButton {
                        text: "删"
                        implicitWidth: 34; implicitHeight: 30
                        onClicked: {
                            aiService.removeInterest(modelData)
                            interestsListView.model = aiService.interestList()
                        }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                TextField {
                    id: interestInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    color: Theme.text
                    placeholderText: "添加一个兴趣（如 Minecraft）..."
                    background: Rectangle { color: Theme.inputBg; radius: 8 }
                }
                AppButton {
                    text: "添加"
                    implicitHeight: 36
                    onClicked: {
                        aiService.addInterest(interestInput.text)
                        interestInput.text = ""
                        interestsListView.model = aiService.interestList()
                    }
                }
            }
        }
    }

    // 未完成话题：列表 + 增删
    Dialog {
        id: topicsDialog
        width: 460
        height: 420
        modal: true
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        background: Rectangle { color: Theme.surface; radius: 14; border.color: Theme.glassBorder; border.width: 1 }
        header: Item {
            height: 40
            Text { anchors.centerIn: parent; text: "未完成话题"; color: Theme.text; font.pixelSize: 16; font.bold: true }
            AppButton {
                anchors.right: parent.right; anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                text: "✕"; implicitWidth: 30; implicitHeight: 30; btnRadius: 15
                onClicked: topicsDialog.close()
            }
        }
        contentItem: ColumnLayout {
            spacing: 8
            ListView {
                id: topicsListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 4
                model: aiService.topicList()
                delegate: RowLayout {
                    width: topicsListView.width
                    spacing: 8
                    Text {
                        Layout.fillWidth: true
                        text: modelData
                        color: Theme.text
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                    }
                    AppButton {
                        text: "删"
                        implicitWidth: 34; implicitHeight: 30
                        onClicked: {
                            aiService.removeTopic(modelData)
                            topicsListView.model = aiService.topicList()
                        }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                TextField {
                    id: topicInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    color: Theme.text
                    placeholderText: "添加一个聊到一半的话题..."
                    background: Rectangle { color: Theme.inputBg; radius: 8 }
                }
                AppButton {
                    text: "添加"
                    implicitHeight: 36
                    onClicked: {
                        aiService.addTopic(topicInput.text)
                        topicInput.text = ""
                        topicsListView.model = aiService.topicList()
                    }
                }
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
            root.aiGreeting = text
            islandToast.show(text, 20000)
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
