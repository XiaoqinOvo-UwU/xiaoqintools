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

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        AppButton {
                            text: "保存 AI 配置"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            onClicked: {
                                aiService.setApiBaseUrl(editBaseUrl.text.trim())
                                aiService.setApiModel(editModel.text.trim())
                                aiService.setApiKey(editApiKey.text.trim())
                                root.note = "AI 配置已保存~"
                                appCore.showToast("AI 配置已保存~")
                            }
                        }
                    }

                    // AI name / persona / avatar / memory moved to the chat page:
                    // click the AI avatar in the chat window to manage them.
                    Text {
                        Layout.fillWidth: true
                        text: "AI 的名字、人设、头像和记忆：打开聊天后点击左上角 AI 头像即可设置"
                        color: Theme.textDim
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
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

                    Text {
                        Layout.fillWidth: true
                        text: "更新源：GitHub 公开仓库（xiaoqinnb666/xiaoqintools），无需配置令牌"
                        color: Theme.textDim
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                    }

                    // auto-start (开机自启) toggle
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Text {
                            text: "开机自启"
                            color: Theme.text
                            font.pixelSize: 13
                            Layout.fillWidth: true
                        }
                        Text {
                            text: autostartSwitch.checked ? "已开启" : "已关闭"
                            color: autostartSwitch.checked ? Theme.ok : Theme.textDim
                            font.pixelSize: 11
                        }
                        Switch {
                            id: autostartSwitch
                            checked: sysService.isAutoStartEnabled()
                            onToggled: {
                                sysService.setAutoStart(checked)
                                root.note = checked ? "已开启开机自启" : "已关闭开机自启"
                            }
                        }
                    }

                    RowLayout {
                        spacing: 10
                        AppButton {
                            text: "检查更新"
                            Layout.fillWidth: true
                            onClicked: {
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
                text: "小钦的工具 v3.3.3 · 泉此方天下第一"
                color: Theme.textDim
                font.pixelSize: 12
            }
        }
    }
}
