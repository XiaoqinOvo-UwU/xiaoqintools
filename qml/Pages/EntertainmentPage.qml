import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

Page {
    id: root
    padding: 0
    background: Rectangle { color: Theme.bg }

    property string result: ""
    property bool prankOn: false

    function randomInt(min, max) {
        return Math.floor(Math.random() * (max - min + 1)) + min
    }

    function luckyValue() {
        var d = new Date()
        var seed = d.getFullYear() * 10000 + (d.getMonth() + 1) * 100 + d.getDate()
        var x = Math.sin(seed * 999) * 10000
        return Math.floor((x - Math.floor(x)) * 100) + 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        PageHeader {
            title: "娱乐 · UwU"
            subtitle: "今日运势、抽签和一点小乐趣"
        }
        GridLayout {
            columns: 3
            columnSpacing: 14
            rowSpacing: 14
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillHeight: false

            ActionCard {
                Layout.fillWidth: true
                title: "今日幸运值"
                desc: "看看今天的欧气"
                onClicked: {
                    var v = root.luckyValue()
                    var verdict = ""
                    if (v >= 90) verdict = "欧气爆棚!! 去买彩票吧 Ovo"
                    else if (v >= 70) verdict = "今天运气不错~"
                    else if (v >= 40) verdict = "平平淡淡才是真"
                    else if (v >= 20) verdict = "有点小霉运，注意脚下 QwQ"
                    else verdict = "低调点……别立flag"
                    root.result = "今日幸运值：" + v + "/100\n" + verdict
                }
            }
            ActionCard {
                Layout.fillWidth: true
                title: "今日宜忌"
                desc: "泉此方钦定"
                onClicked: {
                    var yi = ["出行","会友","交易","祈福","求财","学习","运动","表白","搬家","装修"]
                    var ji = ["熬夜","借贷","争吵","远行","动土","发誓","冒险","拖延","暴饮暴食","冲动消费"]
                    root.result = "今日宜：" + yi[root.randomInt(0, yi.length-1)] + "\n今日忌：" + ji[root.randomInt(0, ji.length-1)]
                }
            }
            ActionCard {
                Layout.fillWidth: true
                title: "抽签"
                desc: "一签定吉凶"
                onClicked: {
                    var signs = ["大吉","中吉","小吉","吉","末吉","凶","大凶"]
                    var tips = ["心想事成 Ovo","运气不错~","小有福气","平平顺顺","稍安勿躁","宜低调 QwQ","宜躺平"]
                    var i = root.randomInt(0, signs.length-1)
                    root.result = "抽签结果：" + signs[i] + "\n" + tips[i]
                }
            }
            ActionCard {
                Layout.fillWidth: true
                title: "抽卡模拟器"
                desc: "SSR 保佑 Ovo"
                onClicked: {
                    var roll = root.randomInt(1, 1000)
                    var rarity = ""
                    if (roll <= 6) rarity = "SSR ★★★★★"
                    else if (roll <= 46) rarity = "SR ★★★★"
                    else if (roll <= 196) rarity = "R ★★★"
                    else rarity = "N ★★"
                    root.result = "抽卡结果：" + rarity
                    statsService.record("gacha", rarity)
                }
            }
            ActionCard {
                Layout.fillWidth: true
                title: "播放音乐"
                desc: "这么可爱真是抱歉.mp3"
                onClicked: appCore.showToast("音乐模块（后续版本）")
            }
            ActionCard {
                Layout.fillWidth: true
                title: "心情日记"
                desc: "选一个心情记下来"
                onClicked: moodDialog.open()
            }
            ActionCard {
                Layout.fillWidth: true
                title: "整蛊模式"
                desc: root.prankOn ? "运行中 ●" : "随机弹「该休息了」"
                onClicked: {
                    root.prankOn = !root.prankOn
                    if (root.prankOn) prankTimer.start()
                    else prankTimer.stop()
                    appCore.showToast(root.prankOn ? "整蛊模式已开启~" : "整蛊模式已关闭~")
                }
            }
            ActionCard {
                Layout.fillWidth: true
                title: "严肃观看"
                desc: "开梯子前往 pixiv"
                onClicked: {
                    proxyService.launchAny()
                    Qt.openUrlExternally("https://www.pixiv.net/")
                }
            }
        }

        Dialog {
            id: moodDialog
            width: 380
            modal: true
            padding: 16
            anchors.centerIn: Overlay.overlay
            background: Rectangle {
                color: "#2A2333"
                radius: 16
                border.color: Qt.rgba(255,180,220,0.25)
                border.width: 1
            }
            header: Item {
                height: 40
                Text {
                    anchors.centerIn: parent
                    text: "今天的心情是……？"
                    color: "#FFD3E8"
                    font.pixelSize: 16
                    font.bold: true
                }
                // X close button
                AppButton {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 28; height: 28
                    text: "✕"
                    implicitHeight: 28
                    glassColor: "transparent"
                    glassHover: Theme.glassHover
                    glassPress: Theme.glassPress
                    font.pixelSize: 14
                    onClicked: moodDialog.close()
                }
            }
            contentItem: ScrollView {
                width: parent.width
                height: 320
                clip: true
                Column {
                    width: parent.width
                    spacing: 8
                    Repeater {
                        model: moodService.moodOptions()
                        delegate: Rectangle {
                            width: parent.width
                            height: 44
                            radius: 22
                            color: mouse.hovered ? Qt.rgba(255,180,210,0.25) : Qt.rgba(255,180,210,0.10)
                            border.color: Qt.rgba(255,200,225,0.25)
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: "#FFE4F0"
                                font.pixelSize: 14
                                width: parent.width - 24
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                            }
                            MouseArea {
                                id: mouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    moodService.recordMood(modelData)
                                statsService.record("mood", modelData)
                                    root.result = "心情已记下~\n" + modelData
                                    moodDialog.close()
                                    appCore.showToast("心情已记下~ " + modelData)
                                }
                            }
                        }
                    }
                    // history viewer
                    Rectangle {
                        width: parent.width
                        height: 38
                        radius: 19
                        color: Qt.rgba(255,220,240,0.08)
                        border.color: Qt.rgba(255,200,225,0.20)
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "📖 查看心情历史"
                            color: "#FFD3E8"
                            font.pixelSize: 13
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                appCore.showToast(moodService.history())
                            }
                        }
                    }
                }
            }
        }

        Timer {
            id: prankTimer
            interval: 60000
            repeat: true
            onTriggered: {
                if (root.prankOn) appCore.showToast("泉此方提示你该休息了~")
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            color: Theme.surface
            radius: 10
            visible: root.result.length > 0
            Text {
                anchors.fill: parent
                anchors.margins: 14
                color: "#E6EEF5"
                font.pixelSize: 15
                wrapMode: Text.Wrap
                text: root.result
            }
        }

        Item { Layout.fillHeight: true }
    }
}
