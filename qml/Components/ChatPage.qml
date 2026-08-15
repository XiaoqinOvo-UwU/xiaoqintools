import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.LocalStorage
import "../Components"

// Full-screen chat page (QQ/WeChat style), opened from the sidebar AI card.
// Conversation is persisted per contact (SQLite) — reopening keeps the same chat.
// INDEPENDENT chat overlay: owns its own near-opaque background so business
// pages never show through. The wallpaper is only faintly felt behind it.
Rectangle {
    id: chatPage
    color: Theme.chatBg
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
            var hist = []
            for (var i = 0; i < rs.rows.length; i++) {
                var isAi = rs.rows.item(i).isAi === 1
                var msg = rs.rows.item(i).msg
                msgModel.append({ "isAi": isAi, "msg": msg })
                hist.push((isAi ? aiService.aiName() : (aiService.userName() || "用户")) + ": " + msg)
            }
            // seed AI context with this conversation so it can see past messages
            aiService.setChatHistory(hist.join("\n"))
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
        NumberAnimation { target: chatPage; property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.OutCubic }
        NumberAnimation { target: chatPage; property: "x"; from: -chatPage.width; to: 0; duration: 200; easing.type: Easing.OutCubic }
    }

    // closing animation: slide right, then notify parent to hide
    function closeWithAnim() {
        closeAnim.restart()
    }
    ParallelAnimation {
        id: closeAnim
        NumberAnimation { target: chatPage; property: "opacity"; to: 0; duration: 160; easing.type: Easing.InCubic }
        NumberAnimation { target: chatPage; property: "x"; to: chatPage.width; duration: 200; easing.type: Easing.InCubic }
        onFinished: chatPage.closeFinished()
    }

    signal backRequested()
    signal closeFinished()
    signal aiProfileRequested()

    // return the last N messages as "角色: 内容" lines (newest last)
    function recentMessages(n) {
        var out = []
        var start = Math.max(0, msgModel.count - n)
        for (var i = start; i < msgModel.count; i++) {
            var m = msgModel.get(i)
            var who = m.isAi ? aiService.aiName() : (aiService.userName() || "用户")
            out.push(who + ": " + m.msg)
        }
        return out
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // header: back button + AI info
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.chatPanelBg
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
                    // presence: colored dot + status (dot turns amber while typing)
                    Row {
                        spacing: 5
                        Rectangle {
                            id: statusDot
                            width: 7; height: 7; radius: 3.5
                            anchors.verticalCenter: parent.verticalCenter
                            color: Theme.ok
                            Behavior on color { ColorAnimation { duration: Theme.durMid; easing.type: Easing.OutCubic } }
                        }
                        Text {
                            id: headerStatus
                            text: "在线"
                            color: Theme.ok
                            font.pixelSize: Theme.fsCaption
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }

            // emotion badge: single character glyph + accent dot (project rule:
            // no emoji icons). Pops over the AI avatar, never touches name/status.
            Rectangle {
                id: emotionBadge
                width: 26; height: 26
                radius: 13
                color: Theme.surface
                border.color: Theme.glassBorder
                border.width: 1
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.topMargin: 6
                anchors.leftMargin: 52
                visible: emotionEmoji.length > 0
                opacity: 0
                scale: 0.4
                Rectangle {
                    width: 5; height: 5; radius: 2.5
                    anchors.top: parent.top
                    anchors.topMargin: 3
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: emotionColor
                }
                Text {
                    id: emotionText
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: 2
                    text: emotionEmoji
                    color: emotionColor
                    font.pixelSize: 13
                    font.bold: true
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
            // track whether the user is pinned to the bottom (so we don't yank
            // the view away while they scroll up through history)
            property bool stickToBottom: true
            onContentYChanged: {
                // if user scrolled away from bottom, stop auto-following
                var dist = contentHeight - contentY - height
                stickToBottom = dist <= 40
            }
            // whenever the content grows (new message or text wrapping to more
            // lines), follow the bottom if the user is pinned there. Deferring
            // twice lets the delegate height settle before we jump.
            onContentHeightChanged: {
                if (stickToBottom) {
                    Qt.callLater(function() { positionViewAtEnd() })
                    Qt.callLater(function() { Qt.callLater(function() { positionViewAtEnd() }) })
                }
            }
            // a new row appended: jump immediately (before it wraps), then again
            // once the content height settles
            onCountChanged: {
                if (stickToBottom) {
                    Qt.callLater(function() { positionViewAtEnd() })
                    Qt.callLater(function() { Qt.callLater(function() { positionViewAtEnd() }) })
                }
            }
            delegate: Item {
                id: delegateRoot
                width: msgView.width
                height: Math.max(44, bubbleRow.height) + 14

                // TG-style message enter animation: slide up + fade in + subtle pop
                transform: Translate { id: msgTrans; y: 16 }
                Component.onCompleted: {
                    delegateRoot.opacity = 0
                    msgTrans.y = 16
                    animIn.start()
                }
                SequentialAnimation {
                    id: animIn
                    NumberAnimation { target: delegateRoot; property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.OutCubic }
                    NumberAnimation { target: msgTrans; property: "y"; from: 16; to: 0; duration: 240; easing.type: Easing.OutCubic }
                }

                Item {
                    id: bubbleRow
                    width: msgView.width
                    height: bubble.height
                    anchors.top: parent.top
                    anchors.topMargin: 8

                    // avatars: AI left, user right (36px — readable next to bubbles)
                    Avatar {
                        id: aiAv
                        size: 36
                        source: chatPage.aiAvatarSource
                        charText: aiService.aiName().length > 0 ? aiService.aiName().charAt(0) : "A"
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.top: parent.top
                        visible: model.isAi
                    }
                    Avatar {
                        id: userAv
                        size: 36
                        source: chatPage.userAvatarSource
                        charText: aiService.userName().length > 0 ? aiService.userName().charAt(0) : "我"
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        anchors.top: parent.top
                        visible: !model.isAi
                    }

                    MessageBubble {
                        id: bubble
                        isAi: model.isAi
                        text: model.msg
                        typing: model.msg === "..."     // animated three-dot reply state

                        // invisible center line: each side shares half the width,
                        // text wraps automatically once it would cross the line.
                        // The floor must stay BELOW the real half-width — a larger
                        // floor caps short messages early AND makes bubbles cross
                        // the center in narrow windows.
                        readonly property real centerLimit: Math.max(120, msgView.width / 2 - 56)
                        // +4 safety buffer: prevents exact-fit text (especially
                        // messages containing a space) from wrapping a glyph over
                        // to a second line.
                        width: model.msg === "..."
                                   ? 64
                                   : Math.min(bubble.contentWidth + 28 + 4, bubble.centerLimit)

                        anchors.left: model.isAi ? aiAv.right : undefined
                        anchors.leftMargin: model.isAi ? 8 : 0
                        anchors.right: model.isAi ? undefined : userAv.left
                        anchors.rightMargin: model.isAi ? 0 : 8
                        anchors.top: parent.top

                        // reveal pop when the typing placeholder turns into text
                        property string prevText: ""
                        onTextChanged: {
                            if (text !== "..." && text !== prevText) {
                                prevText = text
                                bubblePop.start()
                            }
                        }
                        transform: Scale { id: bubbleScale; xScale: 1; yScale: 1 }
                        ParallelAnimation {
                            id: bubblePop
                            NumberAnimation { target: bubbleScale; property: "xScale"; from: 0.97; to: 1.0; duration: 220; easing.type: Easing.OutBack }
                            NumberAnimation { target: bubbleScale; property: "yScale"; from: 0.97; to: 1.0; duration: 220; easing.type: Easing.OutBack }
                        }

                        // multi-line replies: keep the view pinned to the bottom
                        onHeightChanged: {
                            if (msgView && msgView.stickToBottom) {
                                Qt.callLater(function() { if (msgView) msgView.positionViewAtEnd() })
                                Qt.callLater(function() { Qt.callLater(function() { if (msgView) msgView.positionViewAtEnd() }) })
                            }
                        }
                    }
                }
            }
        }
        // empty chat state: avatar + invite (never a blank panel)
        Column {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.topMargin: 56
            visible: msgModel.count === 0
            spacing: Theme.sp3
            Avatar {
                size: 56
                source: chatPage.aiAvatarSource
                charText: aiService.aiName().length > 0 ? aiService.aiName().charAt(0) : "A"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "还没有消息"
                color: Theme.textDim
                font.pixelSize: Theme.fsSmall
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "在下方输入框说点什么，开始和 " + aiService.aiName() + " 聊天吧"
                color: Theme.textMuted
                font.pixelSize: Theme.fsCaption
            }
        }
        ListModel { id: msgModel }

        // input bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: Theme.chatPanelBg
            // no outer border box — the input field itself carries the focus ring
            border.color: "transparent"
            border.width: 0

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    radius: 16
                    color: Theme.inputFill
                    // visible focus ring when the input is active (a11y)
                    border.color: chatInput.activeFocus ? Theme.focusRing : "transparent"
                    border.width: 1
                    TextField {
                        id: chatInput
                        anchors.fill: parent
                        color: Theme.text
                        placeholderText: "输入消息..."
                        placeholderTextColor: Theme.textDim
                        background: null
                        font.pixelSize: Theme.fsBody
                        leftPadding: 16
                        rightPadding: 16
                        topPadding: 0
                        bottomPadding: 0
                        verticalAlignment: Text.AlignVCenter
                        onAccepted: sendMsg()
                        onTextChanged: {
                            if (text.length > 0) appCore.setStatus("用户输入中...")
                            else appCore.setStatus("在线")
                        }
                    }
                }
                // fixed-width send button (no layout shift while replying)
                AppButton {
                    text: replyBusy ? "回复中" : "发送"
                    variant: "primary"
                    implicitWidth: 76
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
        headerStatus.color = (s === "在线") ? Theme.ok : Theme.warn
        statusDot.color = headerStatus.color
    }

    // human-like reply delay: wait "typing time" proportional to text length,
    // then reveal the full reply at once. Each reply is processed one after
    // another; rapid consecutive messages never cancel an earlier reply.
    // Multi-line replies are split into separate short bubbles (like chat apps).
    function appendAi(text) {
        // split into lines (trim empty), each becomes its own bubble
        var parts = text.split(/\r?\n/).map(function(s) { return s.trim() }).filter(function(s) { return s.length > 0 })
        if (parts.length === 0) parts = [text]

        // for each part: ensure a placeholder bubble, then queue it
        for (var p = 0; p < parts.length; p++) {
            // find an existing "..." placeholder bubble to reuse
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
            var ms = Math.round(parts[p].length * 120)
            ms = Math.max(1500, Math.min(ms, 12000))
            replyQueue.push({ "text": parts[p], "ms": ms })
        }
        setHeaderStatus(aiService.aiName() + " 正在输入...")
        pumpReplies()
    }

    // direct AI message insert (e.g. morning greeting): shows immediately
    // (no typing queue) AND persists to the chat DB so it appears in history
    function insertAiMessage(text) {
        if (!text || text.trim().length === 0) return
        msgModel.append({ "isAi": true, "msg": text })
        Qt.callLater(function() { msgView.positionViewAtEnd() })
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

    // ---- AI emotion state (character glyphs — project rule: no emoji icons) ----
    property string emotionEmoji: ""          // single CJK glyph, e.g. "喜"
    property color emotionColor: Theme.ok
    property var emotionChars: ({
        "happy": "喜", "sad": "忧", "angry": "怒", "think": "思",
        "surprised": "惊", "awkward": "尬", "question": "疑",
        "curious": "奇", "neutral": "平", "love": "爱", "tired": "倦"
    })
    property var emotionColors: ({
        "happy": "#5FA87A", "sad": "#C55A5A", "angry": "#C55A5A", "think": "#C9A15A",
        "surprised": "#C9A15A", "awkward": "#9A9A9A", "question": "#C9A15A",
        "curious": "#C9A15A", "neutral": "#9A9A9A", "love": "#C55A5A", "tired": "#C9A15A"
    })

    function playEmotion(name, intensity) {
        var c = emotionChars[name]
        if (!c) return
        emotionEmoji = c
        emotionColor = emotionColors[name] || Theme.ok
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
