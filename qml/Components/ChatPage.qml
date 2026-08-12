import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

// Full-screen chat page (QQ/WeChat style), opened from the sidebar AI card.
Rectangle {
    id: chatPage
    color: Theme.bg
    visible: false
    anchors.fill: parent

    // enter: slide in from LEFT; exit: slide out to RIGHT
    opacity: 0
    x: -width
    Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
    Behavior on x { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
    onVisibleChanged: {
        if (visible) {
            x = -width
            opacity = 0
            x = 0
            opacity = 1
            if (msgModel.count === 0)
                msgModel.append({ "isAi": true, "msg": "你好，我是" + aiService.aiName() + "。" })
        } else {
            // slide out to the right before hiding (handled by parent timer)
        }
    }

    // closing animation: slide to right then notify parent to hide
    function closeWithAnim() {
        opacity = 0
        x = width
        hideTimer.start()
    }
    Timer {
        id: hideTimer
        interval: 250
        onTriggered: chatPage.closeFinished()
    }

    signal backRequested()
    signal closeFinished()
    signal aiProfileRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // header: back button + AI info
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.surface
            border.color: Theme.glassBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                AppButton {
                    text: "‹"
                    implicitWidth: 36
                    implicitHeight: 36
                    font.pixelSize: 20
                    glassColor: Theme.glass
                    glassHover: Theme.glassHover
                    glassPress: Theme.glassPress
                    onClicked: chatPage.backRequested()
                }

                // AI avatar (image or char) — click opens the AI profile dialog
                Rectangle {
                    width: 40; height: 40
                    radius: 20
                    color: Theme.accent
                    clip: true
                    scale: aiAvatarBtn.pressed ? 0.92 : 1.0
                    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                    Image {
                        anchors.fill: parent
                        visible: chatPage.aiAvatarSource.length > 0
                        source: chatPage.aiAvatarSource
                        fillMode: Image.PreserveAspectCrop
                    }
                    Text {
                        anchors.centerIn: parent
                        text: chatPage.aiAvatarSource.length > 0 ? "" : (aiService.aiName().length > 0 ? aiService.aiName().charAt(0) : "A")
                        color: "white"
                        font.pixelSize: 16
                        font.bold: true
                    }
                    MouseArea {
                        id: aiAvatarBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: chatPage.aiProfileRequested()
                    }
                }

                // AIRI-style emotion badge: pops next to the AI avatar when ACT tokens play
                Rectangle {
                    id: emotionBadge
                    width: 26; height: 26
                    radius: 13
                    color: Theme.surface
                    border.color: Theme.glassBorder
                    border.width: 1
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.topMargin: -8
                    anchors.leftMargin: 30
                    visible: emotionEmoji.length > 0
                    opacity: 0
                    scale: 0.4
                    Text {
                        id: emotionText
                        anchors.centerIn: parent
                        text: emotionEmoji
                        font.pixelSize: 14
                    }
                    ParallelAnimation {
                        id: emotionPop
                        NumberAnimation { target: emotionBadge; property: "opacity"; from: 0; to: 1; duration: 180; easing.type: Easing.OutBack }
                        NumberAnimation { target: emotionBadge; property: "scale"; from: 0.4; to: 1.0; duration: 220; easing.type: Easing.OutBack }
                    }
                    SequentialAnimation {
                        id: emotionFade
                        PauseAnimation { duration: 2400 }
                        ParallelAnimation {
                            NumberAnimation { target: emotionBadge; property: "opacity"; to: 0; duration: 300; easing.type: Easing.InCubic }
                            NumberAnimation { target: emotionBadge; property: "scale"; to: 0.6; duration: 300; easing.type: Easing.InCubic }
                        }
                    }
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 1
                    Text {
                        text: aiService.aiName()
                        color: Theme.text
                        font.pixelSize: 15
                        font.bold: true
                    }
                    Text {
                        text: "在线"
                        color: Theme.ok
                        font.pixelSize: 11
                    }
                }
            }
        }

        // message list
        ListView {
            id: msgView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: msgModel
            delegate: Rectangle {
                id: delegateRoot
                width: msgView.width
                height: bubbleRect.height + 12
                color: "transparent"

                // TG-style message enter animation: slide up + fade in
                transform: Translate { id: msgTrans; y: 14 }
                Component.onCompleted: {
                    delegateRoot.opacity = 0
                    msgTrans.y = 14
                    animIn.start()
                }
                SequentialAnimation {
                    id: animIn
                    NumberAnimation { target: delegateRoot; property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.OutCubic }
                    NumberAnimation { target: msgTrans; property: "y"; from: 14; to: 0; duration: 220; easing.type: Easing.OutCubic }
                }

                // message bubble: AI left, user right (anchor-based, reliable sizing)
                Rectangle {
                    id: bubbleRect
                    width: Math.min(msgText.implicitWidth + 24, msgView.width * 0.7)
                    height: Math.max(38, msgText.implicitHeight + 18)
                    radius: 10
                    color: model.isAi ? Theme.glassHover : Theme.accent
                    anchors.top: parent.top
                    anchors.topMargin: 6
                    anchors.left: model.isAi ? parent.left : undefined
                    anchors.right: model.isAi ? undefined : parent.right
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    Text {
                        id: msgText
                        anchors.fill: parent
                        anchors.margins: 10
                        text: model.msg
                        color: Theme.text
                        font.pixelSize: 13
                        wrapMode: Text.Wrap
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                    }
                }
            }
        }
        ListModel { id: msgModel }

        // input bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: Theme.surface
            border.color: Theme.glassBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    radius: 19
                    color: Theme.inputBg
                    TextField {
                        id: chatInput
                        anchors.fill: parent
                        color: Theme.text
                        placeholderText: "输入消息..."
                        placeholderTextColor: Theme.textDim
                        background: null
                        font.pixelSize: 13
                        padding: 12
                        verticalAlignment: Text.AlignVCenter
                        onAccepted: sendMsg()
                        onTextChanged: {
                            if (text.length > 0) appCore.setStatus("正在输入...")
                            else appCore.setStatus("在线")
                        }
                    }
                }
                AppButton {
                    text: "发送"
                    implicitWidth: 64
                    implicitHeight: 38
                    onClicked: sendMsg()
                }
            }
        }
    }

    function sendMsg() {
        var t = chatInput.text.trim()
        if (t.length === 0) return
        msgModel.append({ "isAi": false, "msg": t })
        chatInput.text = ""
        // placeholder AI bubble with typing indicator, replaced in-place by the reply (TG style)
        msgModel.append({ "isAi": true, "msg": "..." })
        msgView.positionViewAtEnd()
        appCore.setStatus(aiService.aiName() + " 正在输入...")
        aiService.sendMessage(t)
    }

    function appendAi(text) {
        // replace the placeholder bubble's content in place (no flicker, TG style)
        if (msgModel.count > 0) {
            var last = msgModel.get(msgModel.count-1)
            if (last.isAi)
                msgModel.set(msgModel.count-1, { "isAi": true, "msg": text })
            else
                msgModel.append({ "isAi": true, "msg": text })
        } else {
            msgModel.append({ "isAi": true, "msg": text })
        }
        msgView.positionViewAtEnd()
        appCore.setStatus("在线")
    }

    // exposed: AI avatar image path (empty = char avatar)
    property string aiAvatarSource: ""
    property string userAvatarSource: ""

    // ---- AIRI-style emotion state ----
    property string emotionEmoji: ""
    property var emotionMap: ({
        "happy": "😊", "sad": "😢", "angry": "😠", "think": "🤔",
        "surprised": "😲", "awkward": "😅", "question": "🤔",
        "curious": "🤔", "neutral": "😐", "love": "🥰", "tired": "😪"
    })

    function playEmotion(name, intensity) {
        var e = emotionMap[name]
        if (!e) return
        emotionEmoji = e
        emotionPop.stop()
        emotionFade.stop()
        emotionPop.start()
        emotionFade.start()
    }

    Connections {
        target: aiService
        function onEmotionSignal(name, intensity) {
            chatPage.playEmotion(name, intensity)
        }
    }
}
