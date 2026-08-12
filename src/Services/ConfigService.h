#pragma once
#include <QString>
#include <QJsonObject>

// Load/save app config to a JSON file in the user config dir.
// Ported concept from the WinForms AppConfig (base_url / model / api_key / proxy paths).
class ConfigService
{
public:
    static ConfigService &instance();

    QString baseUrl() const { return m_baseUrl; }
    QString model() const { return m_model; }
    QString apiKey() const { return m_apiKey; }
    QString clashPath() const { return m_clashPath; }
    QString v2rayPath() const { return m_v2rayPath; }

    // user profile
    QString userName() const { return m_userName; }
    QString avatarChar() const { return m_avatarChar; }
    QString aiName() const { return m_aiName; }
    QString aiPersonality() const { return m_aiPersonality; }

    void setBaseUrl(const QString &v) { m_baseUrl = v; save(); }
    void setModel(const QString &v) { m_model = v; save(); }
    void setApiKey(const QString &v) { m_apiKey = v; save(); }
    void setUserName(const QString &v) { m_userName = v; save(); }
    void setAvatarChar(const QString &v) { m_avatarChar = v; save(); }
    void setAiName(const QString &v) { m_aiName = v; save(); }
    void setAiPersonality(const QString &v) { m_aiPersonality = v; save(); }

    QString configDir() const;
    QString configPath() const;

    void load();
    void save();

    // auto-detect proxy install paths (ported from FindClashExe/FindV2rayExe)
    QString detectClashExe() const;
    QString detectV2rayExe() const;

private:
    ConfigService() { load(); }
    QString m_baseUrl = "https://api.deepseek.com/v1";
    QString m_model = "deepseek-chat";
    QString m_apiKey;
    QString m_clashPath;
    QString m_v2rayPath;
    QString m_userName = "小钦";
    QString m_avatarChar = "钦";
    QString m_aiName = "AI助手";
    QString m_aiPersonality = "温柔、可爱、像朋友";
};
