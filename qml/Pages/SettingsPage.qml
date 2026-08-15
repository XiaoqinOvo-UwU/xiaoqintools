import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../Components"

// Settings page: scrollable, glass cards. AI section includes base config,
// persona editing and memory viewer (AIRI-inspired).
Page {
    id: root
    padding: 0
    background: Rectangle { color: "transparent" }

    property string note: ""
    property string wpPreviewUrl: ""   // current wallpaper thumbnail (file:// url)
    property int wpPreset: -1          // which built-in preset is active (-1 = custom/none)

    Connections {
        target: aiService
        function onWallpaperChanged() { root.wpPreviewUrl = aiService.wallpaperPath() }
    }
    Component.onCompleted: root.wpPreviewUrl = aiService.wallpaperPath()

    // fixed header + scrollable body — same top slot as other pages
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        PageHeader {
            title: "设置"
                subtitle: "按你的习惯，把一切都调好"
        }

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: scroll.availableWidth
                spacing: 14

            // ---- glass section card: 梯子路径 ----
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: col1.implicitHeight + 36
                color: Theme.cardFill
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
                color: Theme.cardFill
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

            // ---- appearance / custom wallpaper ----
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: colWp.implicitHeight + 36
                color: Theme.cardFill
                radius: 12
                border.color: Qt.rgba(255,255,255,0.10)
                border.width: 1

                ColumnLayout {
                    id: colWp
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 18
                    anchors.rightMargin: 18
                    anchors.topMargin: 18
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Text {
                            text: "自定义壁纸"
                            color: "white"; font.pixelSize: 15; font.bold: true
                            Layout.fillWidth: true
                        }
                        // thumbnail preview of the current wallpaper
                        Rectangle {
                            width: 72; height: 44
                            radius: 8
                            color: Theme.inputBg
                            border.color: Qt.rgba(255,255,255,0.10)
                            border.width: 1
                            clip: true
                            Image {
                                anchors.fill: parent
                                visible: root.wpPreviewUrl.length > 0
                                source: root.wpPreviewUrl
                                fillMode: Image.PreserveAspectCrop
                                smooth: true
                                mipmap: true
                            }
                            Text {
                                anchors.centerIn: parent
                                visible: root.wpPreviewUrl.length === 0
                                text: "无"
                                color: Theme.textDim
                                font.pixelSize: 12
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "壁纸只作为右侧面板的背景层，UI 保持深色、文字始终清晰。"
                        color: Theme.textDim
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                    }

                    // ---- built-in preset wallpapers (dark gradients) ----
                    Text {
                        text: "内置壁纸"
                        color: Theme.text; font.pixelSize: 13; font.bold: true
                        Layout.topMargin: 2
                    }
                    Row {
                        Layout.fillWidth: true
                        spacing: 8
                        Repeater {
                            model: [
                                { n: "炭黑",  c0: "#26282C", c1: "#0E0F11" },
                                { n: "石墨灰", c0: "#2B2C30", c1: "#101113" },
                                { n: "墨绿",  c0: "#22302B", c1: "#0C100E" },
                                { n: "深青",  c0: "#1F2E33", c1: "#0B0F11" }
                            ]
                            Rectangle {
                                id: tile
                                readonly property int idx: index
                                property bool hov: false
                                width: 66; height: 44
                                radius: Theme.rMd
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: modelData.c0 }
                                    GradientStop { position: 1.0; color: modelData.c1 }
                                }
                                border.color: root.wpPreset === idx ? Theme.ok
                                            : tile.hov ? Qt.rgba(255,255,255,0.35)
                                            : Qt.rgba(255,255,255,0.10)
                                border.width: root.wpPreset === idx ? 2 : 1
                                scale: tile.hov ? 1.06 : 1.0
                                Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutCubic } }
                                Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
                                Text {
                                    anchors.bottom: parent.bottom
                                    anchors.bottomMargin: 3
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: modelData.n
                                    color: "white"
                                    font.pixelSize: 10
                                    opacity: 0.85
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onEntered: tile.hov = true
                                    onExited: tile.hov = false
                                    onClicked: {
                                        aiService.setWallpaperPreset(idx)
                                        root.wpPreset = idx
                                        root.wpPreviewUrl = aiService.wallpaperPath()
                                        appCore.showToast("壁纸已应用~")
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        text: "或上传自定义图片"
                        color: Theme.textDim
                        font.pixelSize: 12
                        Layout.topMargin: 4
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        AppButton {
                            text: "选择图片"
                            implicitHeight: 34
                            onClicked: wallpaperFileDialog.open()
                        }
                        AppButton {
                            text: "移除壁纸"
                            variant: "ghost"
                            implicitHeight: 34
                            visible: root.wpPreviewUrl.length > 0
                            onClicked: {
                                aiService.removeWallpaper()
                                root.wpPreset = -1
                                root.wpPreviewUrl = aiService.wallpaperPath()
                            }
                        }
                        Item { Layout.fillWidth: true }
                    }

                    // ---- blur control: on/off + strength ----
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Qt.rgba(255,255,255,0.08)
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Column {
                            Layout.fillWidth: true
                            spacing: 2
                            Text { text: "模糊背景"; color: Theme.text; font.pixelSize: 13; font.bold: true }
                            Text {
                                text: root.wpPreviewUrl.length > 0 ? "关闭后直接显示原图" : "先选择壁纸后生效"
                                color: Theme.textDim; font.pixelSize: 11
                            }
                        }
                        ThemeSwitch {
                            id: wpBlurSwitch
                            checked: aiService.wallpaperBlurEnabled()
                            onToggled: aiService.setWallpaperBlurEnabled(checked)
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: wpBlurSwitch.checked
                        spacing: 10
                        Text {
                            text: "模糊程度"
                            color: Theme.text
                            font.pixelSize: 13
                        }
                        DarkSlider {
                            id: wpBlurSlider
                            Layout.fillWidth: true
                            from: 0; to: 40; stepSize: 1
                            value: aiService.wallpaperBlurRadius()
                            onMoved: aiService.setWallpaperBlurRadius(value)
                        }
                        Text {
                            text: wpBlurSlider.value
                            color: Theme.textDim
                            font.pixelSize: 12
                            Layout.preferredWidth: 24
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                }
            }

            FileDialog {
                id: wallpaperFileDialog
                title: "选择壁纸"
                nameFilters: ["图片 (*.png *.jpg *.jpeg *.bmp *.webp)"]
                onAccepted: {
                    var path = wallpaperFileDialog.selectedFile.toString()
                    if (path.indexOf("file:///") === 0) path = path.substring(8)
                    else if (path.indexOf("file://") === 0) path = path.substring(7)
                    path = decodeURIComponent(path)
                    aiService.setWallpaper(path)
                    root.wpPreset = -1
                    root.wpPreviewUrl = aiService.wallpaperPath()
                    appCore.showToast("壁纸已应用~")
                }
            }

            // ---- maintenance ----
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: col3.implicitHeight + 36
                color: Theme.cardFill
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
                        ThemeSwitch {
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
                                appCore.showIsland("正在检查更新...", [], "check_update")
                                updateService.checkForUpdates()
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
                    // theme-consistent inset panel: same dark fill/hairline border
                    // as inputs, subtle left accent bar instead of a green wash.
                    Rectangle {
                        visible: updateService.updateAvailable
                        Layout.fillWidth: true
                        Layout.preferredHeight: updateCol.implicitHeight + 20
                        color: Theme.inputBg
                        radius: Theme.rMd
                        border.color: Qt.rgba(255,255,255,0.08)
                        border.width: 1

                        Rectangle {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.topMargin: 8
                            anchors.bottomMargin: 8
                            width: 3
                            radius: 1.5
                            color: Theme.accent
                        }

                        ColumnLayout {
                            id: updateCol
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 12
                            anchors.topMargin: 10
                            anchors.bottomMargin: 10
                            spacing: 8
                            Text {
                                Layout.fillWidth: true
                                text: "发现新版本 " + updateService.latestVersion + "（当前 " + updateService.currentVersion() + "）"
                                color: Theme.text
                                font.pixelSize: 13
                                font.bold: true
                                wrapMode: Text.Wrap
                            }
                            ProgressBar {
                                id: updateBar
                                Layout.fillWidth: true
                                visible: updateService.downloading
                                from: 0; to: 100
                                value: updateService.downloadProgress
                                height: 6
                                // NOTE: Qt6 does NOT auto-scale a custom
                                // contentItem width to visualPosition — it gets
                                // the FULL bar width (the bar was all-green).
                                // Render the fill ourselves against the track.
                                background: Rectangle { radius: 3; color: Theme.sliderTrack }
                                contentItem: Item {
                                    Rectangle {
                                        width: parent.width * updateBar.visualPosition
                                        height: parent.height
                                        radius: 3
                                        color: Theme.sliderFill
                                        Behavior on width { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
                                    }
                                }
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
                                    onClicked: appCore.showToast("已忽略本次更新")
                                }
                            }
                        }
                    }
                    Text {
                        text: "已发现插件：" + pluginManager.listPlugins().join("、")
                        color: Theme.textDim
                        font.pixelSize: 12
                    }

                    // ---- privacy toggles ----
                    Text { text: "隐私与陪伴"; color: "white"; font.pixelSize: 15; font.bold: true; Layout.topMargin: 8 }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Text {
                            Layout.fillWidth: true
                            text: "允许 AI 读取电脑状态（当前应用/空闲/电量）"
                            color: Theme.text
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                        }
                        ThemeSwitch {
                            id: stateSwitch
                            checked: aiService.allowStateRead()
                            onToggled: {
                                aiService.setAllowStateRead(checked)
                                root.note = checked ? "已开启状态感知" : "已关闭状态感知"
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Text {
                            Layout.fillWidth: true
                            text: "允许记录使用时长与习惯"
                            color: Theme.text
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                        }
                        ThemeSwitch {
                            id: timeSwitch
                            checked: aiService.allowTimeRecord()
                            onToggled: {
                                aiService.setAllowTimeRecord(checked)
                                root.note = checked ? "已开启时长记录" : "已关闭时长记录"
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Text {
                            Layout.fillWidth: true
                            text: "允许长期记忆（AI 记住共同经历）"
                            color: Theme.text
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                        }
                        ThemeSwitch {
                            id: memSwitch
                            checked: aiService.allowLongTermMemory()
                            onToggled: {
                                aiService.setAllowLongTermMemory(checked)
                                root.note = checked ? "已开启长期记忆" : "已关闭长期记忆"
                            }
                        }
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

            Connections {
                target: updateService
                // result of the version check -> shown on the dynamic island
                // (single decision point routed via AppCore) + inline note.
                function onCheckFinished(ok) {
                    if (updateService.updateAvailable) {
                        // download prompt lives in the inline panel below
                        // (visible: updateService.updateAvailable) — not the island.
                        return
                    } else if (updateService.lastError.length > 0) {
                        appCore.showIsland(updateService.lastError, [])
                    } else {
                        appCore.showIsland("当前已是最新版本 v" + updateService.currentVersion(), [])
                    }
                }
                function onDownloadFinished(ok, message) {
                    appCore.showToast(message)
                }
            }

            Item { Layout.preferredHeight: 16 }

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: "小钦的工具 v" + updateService.currentVersion() + " · 泉此方天下第一"
                color: Theme.textDim
                font.pixelSize: 12
            }
            }
        }
    }
}
