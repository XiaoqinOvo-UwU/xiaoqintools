import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.LocalStorage
import "../Components"

// Full-screen chat page (QQ/WeChat style), opened from the sidebar AI card.
// Conversation is persisted per contact (SQLite) — reopening keeps the same chat.
Rectangle {
    id: chatPage
    color: Theme.bg
    anchors.fill: parent

    // enter: slide in from LEFT with a bouncy OutBack; exit: slide out to RIGHT
    opacity: 0
    x: -width

    // ---- persistent per-contact chat (same conversation every time) ----
    function chatDb() {
        var db = LocalStorage.openDatabaseSync("XiaoQinChat", "1.0", "chat history", 8*1024*1024)
        return db
    }

    function loadChat(contactId) {
        var db = chatDb()
        db.transaction(function(tx) {
            tx.executeSql("CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY AUTOINCREMENT, contact TEXT, isAi INTEGER, msg TEXT)")
            var rs = tx.executeSql("SELECT isAi, msg FROM messages WHERE contact=? ORDER BY id", [contactId])
            msgModel.clear()
            for (var i = 0; i < rs.rows.length; i++)
                msgModel.append({ "isAi": rs.rows.item(i).isAi === 1, "msg": rs.rows.item(i).msg })
        })
        // if this contact has no history yet, show a greeting bubble
        if (msgModel.count === 0)
            msgModel.append({ "isAi": true, "msg": "你好，我是" + aiService.aiName() + "。" })
        Qt.callLater(function() { msgView.positionViewAtEnd() })
    }

    function saveMsg(contactId, isAi, msg) {
        var db = chatDb()
        db.transaction(function(tx) {
            tx.executeSql("CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY AUTOINCREMENT, contact TEXT, isAi INTEGER, msg TEXT)")
            tx.executeSql("INSERT INTO messages (contact, isAi, msg) VALUES (?,?,?)", [contactId, isAi ? 1 : 0, msg])
            // keep history bounded (last 400)
            tx.executeSql("DELETE FROM messages WHERE id NOT IN (SELECT id FROM messages WHERE contact=? ORDER BY id DESC LIMIT 400)", [contactId])
        })
        chatPage.messageSaved(contactId, isAi, msg)
    }

    signal messageSaved(string contactId, bool isAi, string msg)

    property string currentContactId: ""

    // switch to a contact: load its conversation (no reset)
    function openContact(id) {
        currentContactId = id
        loadChat(id)
    }

    onVisibleChanged: {
        if (visible) {
            x = -width
            opacity = 0
            openAnim.restart()
            // load the current contact's history whenever the page shows
            var cid = contactService.currentId()
            if (currentContactId !== cid) {
                currentContactId = cid
                loadChat(cid)
            }
        } else {
            // reset so the next open always starts from the left
            x = -width
            opacity = 0
        }
    }
    property string lastGreetedId: ""

    // open animation (explicit object — no Behavior races)
    ParallelAnimation {
        id: openAnim
        NumberAnimation { target: chatPage; property: "opacity"; from: 0; to: 1; duration: 180; easing.type: Easing.OutQuad }
        NumberAnimation { target: chatPage; property: "x"; from: -chatPage.width; to: 0; duration: 340; easing.type: Easing.OutBack; easing.overshoot: 1.15 }
    }

    // closing animation: slide right, then notify parent to hide
    function closeWithAnim() {
        closeAnim.restart()
    }
    ParallelAnimation {
        id: closeAnim
        NumberAnimation { target: chatPage; property: "opacity"; to: 0; duration: 180; easing.type: Easing.InQuad }
        NumberAnimation { target: chatPage; property: "x"; to: chatPage.width; duration: 300; easing.type: Easing.InCubic }
        onFinished: chatPage.closeFinished()
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
                anchors.leftMargin: 8
                anchors.rightMargin: 10
                spacing: 8

                AppButton {
                    text: "‹"
                    implicitWidth: 32
                    implicitHeight: 32
                    font.pixelSize: 18
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

                Column {
                    Layout.fillWidth: true
                    spacing: 0
                    Text {
                        text: aiService.aiName()
                        color: Theme.text
                        font.pixelSize: 15
                        font.bold: true
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        id: headerStatus
                        text: "在线"
                        color: Theme.ok
                        font.pixelSize: 11
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            // AIRI-style emotion badge: pops over the AI avatar (overlay, never
            // touches the name/status area)
            Rectangle {
                id: emotionBadge
                width: 24; height: 24
                radius: 12
                color: Theme.surface
                border.color: Theme.glassBorder
                border.width: 1
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.topMargin: 4
                anchors.leftMargin: 56
                visible: emotionEmoji.length > 0
                opacity: 0
                scale: 0.4
                Text {
                    id: emotionText
                    anchors.centerIn: parent
                    text: emotionEmoji
                    font.pixelSize: 13
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
                            if (text.length > 0) appCore.setStatus("用户输入中...")
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
        // critical path first: clear input + send to AI, so a DB hiccup never
        // blocks the message or leaves the input text stuck
        chatInput.text = ""
        msgModel.append({ "isAi": false, "msg": t })
        msgModel.append({ "isAi": true, "msg": "..." })
        Qt.callLater(function() { msgView.positionViewAtEnd() })
        setHeaderStatus(aiService.aiName() + " 正在输入...")
        aiService.sendMessage(t)
        // persistence is best-effort; never let it break the chat
        try {
            var cid = currentContactId.length > 0 ? currentContactId : contactService.currentId()
            if (cid.length > 0) saveMsg(cid, false, t)
        } catch (e) { }
    }

    function setHeaderStatus(s) {
        headerStatus.text = s
        headerStatus.color = s === "在线" ? Theme.ok : Theme.warn
    }

    // human-like reply delay: wait "typing time" proportional to text length,
    // then reveal the full reply at once. Each reply is processed one after
    // another; rapid consecutive messages never cancel an earlier reply.
    function appendAi(text) {
        // one placeholder per reply: only add a new "..." bubble when there is
        // no pending placeholder to fill, otherwise consecutive AI messages
        // (e.g. proactive idle chats) would overwrite each other.
        var hasPlaceholder = false
        for (var i = 0; i < msgModel.count; i++) {
            if (msgModel.get(i).isAi && msgModel.get(i).msg === "...") {
                hasPlaceholder = true
                break
            }
        }
        if (!hasPlaceholder) {
            msgModel.append({ "isAi": true, "msg": "..." })
            Qt.callLater(function() { msgView.positionViewAtEnd() })
        }
        // typing speed ~ 120ms per char, clamp to 1.5s ~ 12s
        var ms = Math.round(text.length * 120)
        ms = Math.max(1500, Math.min(ms, 12000))
        setHeaderStatus(aiService.aiName() + " 正在输入...")
        replyQueue.push({ "text": text, "ms": ms })
        pumpReplies()
    }

    property var replyQueue: []
    property bool replyBusy: false

    function pumpReplies() {
        // nothing pending, nothing running -> done
        if (replyQueue.length === 0) {
            if (!replyBusy) setHeaderStatus("在线")
            return
        }
        // a reply is currently being typed out; it will pump the next one
        if (replyBusy) return
        replyBusy = true
        var item = replyQueue.shift()
        chatPage.pendingReply = item.text
        replyTimer.interval = item.ms
        replyTimer.start()
    }

    Timer {
        id: replyTimer
        repeat: false
        onTriggered: {
            replyBusy = false
            // reveal this reply in the first pending "..." placeholder bubble
            var text = chatPage.pendingReply
            var filled = false
            for (var i = 0; i < msgModel.count; i++) {
                if (msgModel.get(i).isAi && msgModel.get(i).msg === "...") {
                    msgModel.set(i, { "isAi": true, "msg": text })
                    filled = true
                    break
                }
            }
            if (!filled)
                msgModel.append({ "isAi": true, "msg": text })
            Qt.callLater(function() { msgView.positionViewAtEnd() })
            // persistence (best-effort)
            try {
                var cid = chatPage.currentContactId.length > 0 ? chatPage.currentContactId : contactService.currentId()
                if (cid.length > 0) {
                    var db = chatDb()
                    db.transaction(function(tx) {
                        tx.executeSql("CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY AUTOINCREMENT, contact TEXT, isAi INTEGER, msg TEXT)")
                        tx.executeSql("INSERT INTO messages (contact, isAi, msg) VALUES (?,1,?)", [cid, text])
                    })
                    chatPage.messageSaved(cid, true, text)
                }
            } catch (e) { }
            // process the next queued reply (if any)
            chatPage.pumpReplies()
        }
    }

    property string pendingReply: ""

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
