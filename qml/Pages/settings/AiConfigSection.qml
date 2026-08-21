import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../Components"

// AI 配置：OpenAI-compatible endpoint (base url / model / key) + quick presets.
// Emits notify(message) so the hosting page can surface inline feedback.
SettingsSectionCard {
    id: section

    signal notify(string message)

    title: "AI 配置"

    Text {
        Layout.fillWidth: true
        text: "支持任意 OpenAI 兼容的 API 接入（DeepSeek / Claude / GPT / 自建中转）"
        color: Theme.textDim
        font.pixelSize: 11
        wrapMode: Text.Wrap
    }

    Text { text: "Base URL"; color: Theme.textDim; font.pixelSize: 12; Layout.topMargin: 4 }
    ThemedTextField {
        id: editBaseUrl
        placeholderText: "https://api.deepseek.com/v1"
        text: aiService.apiBaseUrl()
    }
    Text { text: "Model"; color: Theme.textDim; font.pixelSize: 12 }
    ThemedTextField {
        id: editModel
        placeholderText: "deepseek-chat"
        text: aiService.apiModel()
    }
    Text { text: "API Key"; color: Theme.textDim; font.pixelSize: 12 }
    ThemedTextField {
        id: editApiKey
        placeholderText: "sk-..."
        text: aiService.apiKey()
        echoMode: TextInput.Password
    }

    // ---- quick preset buttons ----
    Text {
        text: "快速预设"
        color: Theme.text
        font.pixelSize: 13
        font.bold: true
        Layout.topMargin: 8
    }
    Text {
        Layout.fillWidth: true
        text: "一键填入常用 API 配置（需自行填写 Key）"
        color: Theme.textDim
        font.pixelSize: 11
        wrapMode: Text.Wrap
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        rowSpacing: 8
        columnSpacing: 8

        AppButton {
            text: "DeepSeek"
            variant: "ghost"
            Layout.fillWidth: true
            implicitHeight: 32
            onClicked: {
                editBaseUrl.text = "https://api.deepseek.com/v1"
                editModel.text = "deepseek-chat"
                editApiKey.text = aiService.apiKeyFor(editBaseUrl.text)
                appCore.showToast("已填入 DeepSeek 配置")
            }
        }
        AppButton {
            text: "OpenAI"
            variant: "ghost"
            Layout.fillWidth: true
            implicitHeight: 32
            onClicked: {
                editBaseUrl.text = "https://api.openai.com/v1"
                editModel.text = "gpt-4o"
                editApiKey.text = aiService.apiKeyFor(editBaseUrl.text)
                appCore.showToast("已填入 OpenAI 配置")
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10
        AppButton {
            text: "保存 AI 配置"
            Layout.fillWidth: true
            implicitHeight: 36
            onClicked: {
                var url = editBaseUrl.text.trim()
                var key = editApiKey.text.trim()
                if (url.length > 0 && key.length > 0)
                    aiService.rememberApiKeyFor(url, key)
                aiService.setApiBaseUrl(url)
                aiService.setApiModel(editModel.text.trim())
                aiService.setApiKey(key)
                section.notify("AI 配置已保存~")
                appCore.showToast("AI 配置已保存~")
            }
        }
    }
}
