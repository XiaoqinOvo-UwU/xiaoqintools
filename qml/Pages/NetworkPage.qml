import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

Page {
    id: page
    background: Rectangle { color: Theme.bg }

    property string proxyStatus: "检测中..."

    function refreshStatus() {
        proxyService.statusAsync()
    }

    function notify(msg) {
        appCore.showToast(msg)
        appCore.setStatus(msg)
    }

    function launchProxy(which) {
        appCore.setStatus("打开梯子中...")
        var ok = which === "Clash" ? proxyService.launchClash() : proxyService.launchV2ray()
        notify(ok ? "已打开 " + which : "打开失败")
        if (ok) statsService.record("proxy", which)
        page.refreshStatus()
    }

    // async status response
    Connections {
        target: proxyService
        function onStatusReady(status) {
            page.proxyStatus = status
        }
    }

    Component.onCompleted: {
        refreshStatus()
        statusTimer.start()
    }

    Timer {
        id: statusTimer
        interval: 3000
        repeat: true
        onTriggered: page.refreshStatus()
    }

    // two fixed columns for perfect alignment
    RowLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 28

        // ---------- LEFT: 梯子管理 ----------
        ColumnLayout {
            Layout.preferredWidth: 420
            Layout.fillHeight: false
            Layout.alignment: Qt.AlignTop
            spacing: 14

            Text {
                text: "梯子管理"
                color: "white"
                font.pixelSize: Theme.fsPage
                font.bold: true
            }
            Text {
                text: "导入节点或订阅链接，一键打开梯子"
                color: Theme.textDim
                font.pixelSize: Theme.fsSmall
            }


            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 96
                color: Theme.surface
                radius: 10
                TextArea {
                    id: input
                    anchors.fill: parent
                    anchors.margins: 12
                    color: "white"
                    placeholderText: "粘贴 vmess/vless/trojan/ss 链接 或 订阅地址"
                    placeholderTextColor: Theme.textDim
                    background: null
                    font.pixelSize: 13
                    wrapMode: TextEdit.Wrap
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                AppButton {
                    text: "导入到梯子"
                    Layout.fillWidth: true
                    onClicked: {
                        if (input.text.trim().length === 0) {
                            notify("请先粘贴配置链接或订阅地址")
                            return
                        }
                        proxyService.copyToClipboard(input.text.trim())
                        notify("已复制到剪贴板，请在梯子中确认导入")
                    }
                }
                AppButton {
                    text: "导入订阅文件"
                    implicitWidth: 130
                    onClicked: notify("请选择订阅文件（文件选择框预留）")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                AppButton {
                    text: "打开梯子"
                    Layout.fillWidth: true
                    onClicked: {
                        var clash = proxyService.hasClash()
                        var v2ray = proxyService.hasV2ray()
                        if (clash && v2ray) {
                            // both installed -> ask via dynamic island
                            proxyChoice.showChoice("打开哪个梯子？", ["Clash Verge", "v2rayN"])
                        } else if (clash) {
                            launchProxy("Clash")
                        } else if (v2ray) {
                            launchProxy("v2rayN")
                        } else {
                            notify("未找到梯子程序")
                        }
                    }
                }
                AppButton {
                    text: "清空"
                    implicitWidth: 130
                    onClicked: input.text = ""
                }
            }

            // status card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                color: Theme.surface
                radius: 10
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 10
                    Rectangle {
                        width: 10; height: 10; radius: 5
                        color: page.proxyStatus.indexOf("●") >= 0 ? "#4EA86B" : "#D8A84E"
                    }
                    Text {
                        text: "梯子状态：" + page.proxyStatus
                        color: Theme.text
                        font.pixelSize: 14
                    }
                }
            }

            // self-diagnosis output
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: Theme.surface
                radius: 10
                visible: diagText.text.length > 0
                clip: true
                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 8
                    TextArea {
                        id: diagText
                        width: parent.width
                        color: "#B9C6D4"
                        readOnly: true
                        font.pixelSize: 12
                        font.family: "Consolas"
                        wrapMode: TextEdit.Wrap
                        text: ""
                        background: null
                        padding: 6
                    }
                }
            }
        }

        // ---------- RIGHT: 网络工具 ----------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: false
            Layout.alignment: Qt.AlignTop
            spacing: 14

            Text {
                text: "网络工具"
                color: "white"
                font.pixelSize: 20
                font.bold: true
            }
            GridLayout {
                columns: 2
                columnSpacing: 14
                rowSpacing: 14
                Layout.fillWidth: true
                

                ActionCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "节点测速"
                    desc: "检测各站点延迟"
                    onClicked: {
                        notify("节点测速中...")
                        netService.nodeTestAsync()
                    }
                }
                ActionCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "网络测速"
                    desc: "测下载速度"
                    onClicked: {
                        notify("测速中...")
                        netService.speedTestAsync(true, 10 * 1024 * 1024)
                    }
                }
                ActionCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "故障自检"
                    desc: "端口 / DNS / 连通性"
                    onClicked: {
                        notify("开始自检...")
                        netService.diagnoseAsync()
                    }
                }
                ActionCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "下载梯子 Ovo"
                    desc: "打开 Clash Verge 下载页"
                    onClicked: {
                        Qt.openUrlExternally("https://github.com/clash-verge-rev/clash-verge-rev/releases")
                        notify("已打开下载页面")
                    }
                }
                ActionCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "遇到难题找GPT"
                    desc: "自动开梯子并打开 ChatGPT"
                    onClicked: {
                        proxyService.launchAny()
                        Qt.openUrlExternally("https://chatgpt.com/")
                        notify("正在前往 ChatGPT...")
                    }
                }
                ActionCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "刷新状态"
                    desc: "重新检测梯子状态"
                    onClicked: page.refreshStatus()
                }
            }

            // speed test result
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 130
                color: Theme.surface
                radius: 10
                visible: resultText.text.length > 0
                Text {
                    id: resultText
                    anchors.fill: parent
                    anchors.margins: 14
                    color: "#B9C6D4"
                    font.pixelSize: 13
                    wrapMode: Text.Wrap
                    text: ""
                }
            }
        }
    }

    // async results from NetworkService -> update UI without blocking
    Connections {
        target: netService
        function onResult(text) {
            // node test / speed test -> result box; self-diagnosis -> diag box
            if (text.indexOf("google.com") >= 0) {
                resultText.text = "节点延迟：\n" + text
                appCore.showToast("节点测速完成")
            } else if (text.indexOf("=== 进程检测 ===") >= 0) {
                diagText.text = text
                appCore.showToast("自检完成")
            } else if (text.indexOf("Mbps") >= 0) {
                resultText.text = "网络测速（经梯子）：\n" + text
                appCore.showToast("测速完成")
            } else {
                resultText.text = text
            }
        }
    }

    // dynamic island choice toast: pick which proxy to open
    IslandToast {
        id: proxyChoice
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 16
        z: 99
        onActionChosen: function(index) {
            page.launchProxy(index === 0 ? "Clash" : "v2rayN")
        }
    }
}
