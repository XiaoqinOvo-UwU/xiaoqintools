import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

Page {
    background: Rectangle { color: Theme.bg }

    Connections {
        target: sysService
        function onCleanupDone(message) {
            appCore.showToast(message)
            // record freed MB into stats (parse "释放 N MB")
            var m = message.match(/释放\s*(\d+)\s*MB/)
            if (m) statsService.record("clean", m[1])
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 20

        Text {
            text: "系统工具"
            color: "white"
            font.pixelSize: 20
            font.bold: true
        }

        GridLayout {
            columns: 3
            columnSpacing: 14
            rowSpacing: 14
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillHeight: false

            ActionCard {
                title: "清理垃圾"
                desc: "临时文件 / 缓存 / 崩溃转储"
                Layout.fillWidth: true
                onClicked: {
                    appCore.showToast("泉此方开始扫垃圾了...")
                    sysService.cleanJunkAsync()
                }
            }            ActionCard {
                title: "清理内存"
                desc: "释放进程内存"
                Layout.fillWidth: true
                onClicked: {
                    appCore.showToast(sysService.cleanMemory())
                    statsService.record("mem", "1")
                }
            }
            ActionCard {
                title: "开机自启管理"
                desc: "查看 / 禁用启动项"
                Layout.fillWidth: true
                onClicked: {
                    startupModel.clear()
                    var items = sysService.startupItemList()
                    for (var i = 0; i < items.length; i++) {
                        var parts = items[i].split("|")
                        if (parts.length >= 2) {
                            startupModel.append({ "name": parts[0], "cmd": parts[1], "src": parts.length > 2 ? parts[2] : "用户" })
                        }
                    }
                    startupDialog.open()
                }
            }
            ActionCard {
                title: "使用报告"
                desc: "抽卡 / 心情 / 清理统计"
                Layout.fillWidth: true
                onClicked: {
                    reportDialog.tab = 0
                    reportDialog.loadData()
                    reportDialog.open()
                }
            }
            ActionCard {
                title: "大文件扫描"
                desc: "找出 C 盘的大文件"
                Layout.fillWidth: true
                onClicked: {
                    toolResult.text = sysService.scanLargeFiles()
                    toolDialog.title = "大文件扫描"
                    toolDialog.open()
                }
            }
            ActionCard {
                title: "剪贴板历史"
                desc: "查看当前剪贴板内容"
                Layout.fillWidth: true
                onClicked: {
                    toolResult.text = sysService.clipboardText()
                    toolDialog.title = "剪贴板"
                    toolDialog.open()
                }
            }
        }
    }

    // startup items dialog (manageable: view + disable/enable)
    Dialog {
        id: startupDialog
        width: 560
        height: 460
        modal: true
        anchors.centerIn: Overlay.overlay
        title: "开机自启项"
        background: Rectangle {
            color: Theme.surface
            radius: 12
            border.color: Theme.glassBorder
            border.width: 1
        }
        contentItem: Column {
            spacing: 8

            ListModel { id: startupModel }

            ListView {
                id: startupView
                width: parent.width
                height: 330
                clip: true
                spacing: 4
                model: startupModel
                delegate: Rectangle {
                    width: startupView.width
                    height: 44
                    radius: 8
                    color: startupView.currentIndex === index ? Theme.selected : Theme.glass
                    border.color: Theme.glassBorder
                    border.width: 1
                    MouseArea {
                        anchors.fill: parent
                        onClicked: startupView.currentIndex = index
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8
                        Column {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: model.name
                                color: Theme.text
                                font.pixelSize: 13
                                font.bold: true
                                elide: Text.ElideRight
                            }
                            Text {
                                text: model.src + " · " + model.cmd
                                color: Theme.textDim
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }

            RowLayout {
                width: parent.width
                spacing: 8
                AppButton {
                    text: "禁用所选"
                    Layout.fillWidth: true
                    enabled: startupView.currentIndex >= 0
                    onClicked: {
                        var it = startupModel.get(startupView.currentIndex)
                        sysService.disableStartupItem(it.name)
                        startupModel.remove(startupView.currentIndex)
                    }
                }
                AppButton {
                    text: "关闭"
                    Layout.fillWidth: true
                    onClicked: startupDialog.close()
                }
            }
        }
    }

    // generic tool result dialog (usage report / file scan / clipboard)
    Dialog {
        id: toolDialog
        width: 520
        height: 380
        modal: true
        anchors.centerIn: Overlay.overlay
        background: Rectangle {
            color: Theme.surface
            radius: 12
            border.color: Theme.glassBorder
            border.width: 1
        }
        contentItem: Column {
            spacing: 8
            ScrollView {
                width: parent.width
                height: 300
                clip: true
                TextArea {
                    id: toolResult
                    width: parent.width
                    color: Theme.text
                    font.pixelSize: 12
                    font.family: "Consolas"
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    background: null
                }
            }
            AppButton {
                text: "关闭"
                anchors.horizontalCenter: parent.horizontalCenter
                implicitWidth: 100
                onClicked: toolDialog.close()
            }
        }
    }

    // usage report dialog with bar charts
    ReportDialog {
        id: reportDialog
    }
}
