import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../Components"

// Settings page: scrollable, glass cards. AI section includes base config,
// persona editing and memory viewer (AIRI-inspired).
Page {
    id: root
    background: Rectangle { color: Theme.bg }

    property string note: ""

    // scrollable content
    ScrollView {
        id: scroll
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: scroll.availableWidth
            spacing: 14

            Text {
                text: "设置"
                color: "white"
                font.pixelSize: 20
                font.bold: true
            }

            // ---- glass section card: 梯子路径 ----
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: col1.implicitHeight + 36
                color: Qt.rgba(255,255,255,0.05)
                radius: 12
                border.color: Qt.rgba(255,255,255,0.10)
                border.width: 1

                ColumnLayout {
                    id: col1
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 18
                    anchors.rightMargin: 18
                    anchors.topMargin: 18
                    spacing: 10

                    Text { text: "梯子路径"; color: "white"; font.pixelSize: 15; font.bold: true }

                    Text { text: "Clash Verge"; color: Theme.textDim; font.pixelSize: 12 }
                    TextField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        color: "white"
                        padding: 8
                        placeholderText: "自动检测"
                        placeholderTextColor: Theme.textDim
                        text: proxyService.clashExe()
                        background: Rectangle { color: Theme.inputBg; radius: 8 }
                    }
                    Text { text: "v2rayN"; color: Theme.textDim; font.pixelSize: 12 }
                    TextField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        color: "white"
                        placeholderText: "自动检测"
                        placeholderTextColor: Theme.textDim
                        text: proxyService.v2rayExe()
                        background: Rectangle { color: Theme.inputBg; radius: 8 }
                    }
                }
            }

            // ---- glass section card: AI 配置（基础配置 + 人设 + 记忆） ----
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: col2.implicitHeight + 36
                color: Qt.rgba(255,255,255,0.05)
                radius: 12
                border.color: Qt.rgba(255,255,255,0.10)
                border.width: 1

                ColumnLayout {
                    id: col2
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 18
                    anchors.rightMargin: 18
                    anchors.topMargin: 18
                    spacing: 10

                    Text { text: "AI 配置"; color: "white"; font.pixelSize: 15; font.bold: true }
                    Text { text: "Base URL"; color: Theme.textDim; font.pixelSize: 12 }
                    TextField {
                        id: editBaseUrl
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        color: "white"
                        placeholderText: "https://api.deepseek.com/v1"
                        placeholderTextColor: Theme.textDim
                        text: aiService.apiBaseUrl()
                        background: Rectangle { color: Theme.inputBg; radius: 8 }
                    }
                    Text { text: "Model"; color: Theme.textDim; font.pixelSize: 12 }
                    TextField {
                        id: editModel
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        color: "white"
                        placeholderText: "deepseek-chat"
                        placeholderTextColor: Theme.textDim
                        text: aiService.apiModel()
                        background: Rectangle { color: Theme.inputBg; radius: 8 }
                    }
                    Text { text: "API Key"; color: Theme.textDim; font.pixelSize: 12 }
                    TextField {
                        id: editApiKey
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        color: "white"
                        placeholderText: "sk-..."
                        placeholderTextColor: Theme.textDim
                        text: aiService.apiKey()
                        echoMode: TextInput.Password
                        background: Rectangle { color: Theme.inputBg; radius: 8 }
                    }

                    // ---- 人设更改 ----
                    Text { text: "AI 名字"; color: Theme.textDim; font.pixelSize: 12 }
                    TextField {
                        id: editAiName
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        color: "white"
                        placeholderText: "白栀"
                        placeholderTextColor: Theme.textDim
                        text: aiService.aiName()
                        background: Rectangle { color: Theme.inputBg; radius: 8 }
                    }
                    Text { text: "AI 人设（性格、说话风格）"; color: Theme.textDim; font.pixelSize: 12 }
                    TextArea {
                        id: editAiPersonality
                        Layout.fillWidth: true
                        Layout.preferredHeight: 72
                        color: "white"
                        placeholderText: "温柔、成熟、略带忧郁"
                        placeholderTextColor: Theme.textDim
                        text: aiService.aiPersonality()
                        wrapMode: TextEdit.Wrap
                        background: Rectangle { color: Theme.inputBg; radius: 8 }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        AppButton {
                            text: "🖼 上传 AI 头像"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            onClicked: aiAvatarFileDialog.open()
                        }
                        AppButton {
                            text: "保存 AI 配置"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            onClicked: {
                                aiService.setApiBaseUrl(editBaseUrl.text.trim())
                                aiService.setApiModel(editModel.text.trim())
                                aiService.setApiKey(editApiKey.text.trim())
                                aiService.setAiName(editAiName.text.trim())
                                aiService.setAiPersonality(editAiPersonality.text.trim())
                                root.note = "AI 配置已保存~"
                                appCore.showToast("AI 配置已保存~")
                            }
                        }
                    }

                    // ---- 记忆查看 ----
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Qt.rgba(255,255,255,0.08)
                    }
                    Text { text: "AI 记忆"; color: "white"; font.pixelSize: 14; font.bold: true }
                    Text {
                        Layout.fillWidth: true
                        text: aiService.memoryDetail().length > 20 ? "已有记忆数据，点「查看记忆」浏览；聊天时 AI 也会参考这些记忆" : "暂无记忆，多用一用 AI 会慢慢记住你"
                        color: Theme.textDim
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        AppButton {
                            text: "📖 查看记忆"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            onClicked: memoryDialog.open()
                        }
                        AppButton {
                            text: "➕ 添加记忆"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            onClicked: {
                                memoryInput.text = ""
                                memoryInputDialog.open()
                            }
                        }
                        AppButton {
                            text: "🗑 清空记忆"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            glassColor: Qt.rgba(0.77,0.35,0.35,0.35)
                            onClicked: {
                                aiService.clearMemory()
                                root.note = "记忆已清空~"
                                appCore.showToast("记忆已清空~")
                            }
                        }
                    }
                }
            }

            // ---- maintenance ----
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: col3.implicitHeight + 36
                color: Qt.rgba(255,255,255,0.05)
                radius: 12
                border.color: Qt.rgba(255,255,255,0.10)
                border.width: 1

                ColumnLayout {
                    id: col3
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 18
                    anchors.rightMargin: 18
                    anchors.topMargin: 18
                    spacing: 10

                    Text { text: "维护"; color: "white"; font.pixelSize: 15; font.bold: true }

                    Text { text: "Gitee 仓库（owner/repo）"; color: Theme.textDim; font.pixelSize: 12 }
                    TextField {
                        id: editGiteeRepo
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        color: "white"
                        placeholderText: "xiao-qin-uwu/xiaoqintools"
                        placeholderTextColor: Theme.textDim
                        text: aiService.giteeRepo()
                        background: Rectangle { color: Theme.inputBg; radius: 8 }
                    }
                    Text { text: "Gitee Token（私有仓库需要）"; color: Theme.textDim; font.pixelSize: 12 }
                    TextField {
                        id: editGiteeToken
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        color: "white"
                        placeholderText: "xxx"
                        placeholderTextColor: Theme.textDim
                        text: aiService.giteeToken()
                        echoMode: TextInput.Password
                        background: Rectangle { color: Theme.inputBg; radius: 8 }
                    }
                    RowLayout {
                        spacing: 10
                        AppButton {
                            text: "检查更新"
                            Layout.fillWidth: true
                            onClicked: {
                                aiService.setGiteeRepo(editGiteeRepo.text.trim())
                                aiService.setGiteeToken(editGiteeToken.text.trim())
                                root.note = "正在检查更新..."
                                updateService.checkForUpdates()
                                checkTimer.start()
                            }
                        }
                        AppButton {
                            text: "导出配置"
                            Layout.fillWidth: true
                            onClicked: {
                                var ok = syncService.exportConfig(syncService.defaultExportPath())
                                root.note = ok ? "配置已导出到杂货铺" : "导出失败"
                            }
                        }
                        AppButton {
                            text: "导入配置"
                            Layout.fillWidth: true
                            onClicked: root.note = "导入配置（文件选择框预留）"
                        }
                    }

                    // update download section (visible when an update is available)
                    Rectangle {
                        visible: updateService.updateAvailable
                        Layout.fillWidth: true
                        Layout.preferredHeight: updateCol.implicitHeight + 16
                        color: Qt.rgba(0.36,0.6,0.5,0.22)
                        radius: 10
                        border.color: Qt.rgba(0.4,0.8,0.6,0.35)
                        border.width: 1
                        ColumnLayout {
                            id: updateCol
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6
                            Text {
                                Layout.fillWidth: true
                                text: "发现新版本 " + updateService.latestVersion + "（当前 " + updateService.currentVersion() + "）"
                                color: "#8FE0C0"
                                font.pixelSize: 13
                                font.bold: true
                                wrapMode: Text.Wrap
                            }
                            ProgressBar {
                                Layout.fillWidth: true
                                visible: updateService.downloading
                                from: 0; to: 100
                                value: updateService.downloadProgress
                                height: 6
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                AppButton {
                                    text: "下载并安装"
                                    Layout.fillWidth: true
                                    enabled: !updateService.downloading
                                    onClicked: updateService.downloadAndInstall()
                                }
                                AppButton {
                                    text: "忽略"
                                    Layout.fillWidth: true
                                    onClicked: root.note = "本次忽略更新"
                                }
                            }
                        }
                    }
                    Text {
                        text: "已发现插件：" + pluginManager.listPlugins().join("、")
                        color: Theme.textDim
                        font.pixelSize: 12
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                color: "#4EA86B"
                font.pixelSize: 13
                text: root.note
                wrapMode: Text.Wrap
                visible: root.note.length > 0
            }

            Timer {
                id: checkTimer
                interval: 3000
                repeat: false
                onTriggered: {
                    if (updateService.updateAvailable) {
                        root.note = "发现新版本 " + updateService.latestVersion + "，可点「下载并安装」"
                    } else {
                        root.note = "当前已是最新版本"
                    }
                }
            }

            Connections {
                target: updateService
                function onDownloadFinished(ok, message) {
                    root.note = message
                    appCore.showToast(message)
                }
            }

            Item { Layout.preferredHeight: 16 }

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: "小钦的工具 v3.0.1 · 泉此方天下第一"
                color: Theme.textDim
                font.pixelSize: 12
            }
        }
    }

    // ---- memory viewer dialog ----
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

    // ---- add memory note dialog ----
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
                        root.note = "记住了~"
                        appCore.showToast("记住了~")
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

    // ---- AI avatar upload ----
    FileDialog {
        id: aiAvatarFileDialog
        title: "选择 AI 头像"
        nameFilters: ["图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)", "所有文件 (*.*)"]
        onAccepted: {
            var url = aiAvatarFileDialog.selectedFile.toString()
            var p = url.replace("file:///", "").replace("file://", "")
            if (p.length > 0) {
                aiService.setAiAvatar(p)
                root.note = "AI 头像已更新~"
                appCore.showToast("AI 头像已更新~")
            }
        }
    }
}
