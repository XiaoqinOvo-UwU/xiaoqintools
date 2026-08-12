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

    // refresh profile labels after edit
    function refreshProfile() {
        userAvatarText.text = aiService.avatarChar()
        userNameText.text = aiService.userName()
        aiCardName.text = aiService.aiName()
        root.aiAvatarPath = toFileUrl(aiService.aiAvatarPath())
        root.userAvatarPath = toFileUrl(aiService.userAvatarPath())
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

                // ================= AI chat card (click -> full-screen ChatPage) =================
                Rectangle {
                    id: aiChatCard
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    color: "transparent"
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: aiChatCard.color = Theme.hoverBg
                        onExited: aiChatCard.color = "transparent"
                        onClicked: root.chatOpen = true
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10
                        Rectangle {
                            width: 40; height: 40
                            radius: 20
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
                                text: root.aiAvatarPath.length > 0 ? "" : (aiService.aiName().length > 0 ? aiService.aiName().charAt(0) : "A")
                                color: "white"
                                font.pixelSize: 16
                                font.bold: true
                            }
                        }
                        Column {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                id: aiCardName
                                text: aiService.aiName()
                                color: Theme.text
                                font.pixelSize: 14
                                font.bold: true
                            }
                            Text {
                                text: "点击进入聊天"
                                color: Theme.textDim
                                font.pixelSize: 11
                            }
                        }
                        Text {
                            text: "›"
                            color: Theme.textDim
                            font.pixelSize: 18
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
                        readonly property bool active: root.currentPage === myIndex
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
                        text: "小钦的工具 v3.0.2"
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
        // load avatars (files may appear after first upload)
        root.aiAvatarPath = toFileUrl(aiService.aiAvatarPath())
        root.userAvatarPath = toFileUrl(aiService.userAvatarPath())
        root.aiGreeting = aiService.greeting()
        if (aiService.shouldGreetToday()) {
            islandToast.show(root.aiGreeting, 20000)
            aiService.generateGreeting()
            aiService.markGreeted()
        }
        idleTimer.start()
    }

    Connections {
        target: aiService
        function onChatReply(text) {
            chatPage.appendAi(text)
        }
        function onGreetingReady(text) {
            root.aiGreeting = text
            islandToast.show(text, 20000)
        }
        function onProfileChanged() {
            root.refreshProfile()
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
