#pragma once
#include <QString>
#include <QJsonObject>
#include <QtGlobal>

// Load/save app config to a JSON file in the user config dir.
// Ported concept from the WinForms AppConfig (base_url / model / api_key / proxy paths).
class ConfigService
{
public:
    static ConfigService &instance();

    QString baseUrl() const { return m_baseUrl; }
    QString model() const { return m_model; }
    QString apiKey() const { return m_apiKey; }

    // per-provider key memory: baseUrl -> last used api key (preset switching)
    QString apiKeyFor(const QString &baseUrl) const { return m_apiKeys.value(baseUrl).toString(); }
    void rememberApiKeyFor(const QString &baseUrl, const QString &key) { m_apiKeys.insert(baseUrl, key); save(); }

    QString clashPath() const { return m_clashPath; }
    QString v2rayPath() const { return m_v2rayPath; }

    // user profile
    QString userName() const { return m_userName; }
    QString avatarChar() const { return m_avatarChar; }
    QString aiName() const { return m_aiName; }
    QString aiPersonality() const { return m_aiPersonality; }

    // privacy toggles (default ON)
    bool allowStateRead() const { return m_allowStateRead; }
    bool allowTimeRecord() const { return m_allowTimeRecord; }
    bool allowLongTermMemory() const { return m_allowLongTermMemory; }
    void setAllowStateRead(bool v) { m_allowStateRead = v; save(); }
    void setAllowTimeRecord(bool v) { m_allowTimeRecord = v; save(); }
    void setAllowLongTermMemory(bool v) { m_allowLongTermMemory = v; save(); }

    // wallpaper blur (default on, radius 24)
    bool wallpaperBlurEnabled() const { return m_wallpaperBlurEnabled; }
    int  wallpaperBlurRadius() const { return m_wallpaperBlurRadius; }
    void setWallpaperBlurEnabled(bool v) { m_wallpaperBlurEnabled = v; save(); }
    void setWallpaperBlurRadius(int v) { m_wallpaperBlurRadius = qBound(0, v, 40); save(); }

    // wallpaper average luminance (0=dark .. 1=bright) — drives the dark overlay
    double wallpaperBrightness() const { return m_wallpaperBrightness; }
    void setWallpaperBrightness(double v) { m_wallpaperBrightness = qBound(0.0, v, 1.0); save(); }

    // appearance mode: "" = 默认深色 | "glass" = 壁纸玻璃
    QString appearanceMode() const { return m_appearanceMode; }
    double  wallpaperGlassOpacity() const { return m_wallpaperGlassOpacity; }
    void setAppearanceMode(const QString &v) { m_appearanceMode = v; save(); }
    void setWallpaperGlassOpacity(double v) { m_wallpaperGlassOpacity = qBound(0.05, v, 0.20); save(); }

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
    QJsonObject m_apiKeys;             // baseUrl -> last used key
    QString m_clashPath;
    QString m_v2rayPath;
    QString m_userName = "用户";
    QString m_avatarChar = "用";
    QString m_aiName = "AI助手";
    QString m_aiPersonality = "温柔、可爱、像朋友";
    bool m_allowStateRead = true;
    bool m_allowTimeRecord = true;
    bool m_allowLongTermMemory = true;
    bool m_wallpaperBlurEnabled = true;
    int  m_wallpaperBlurRadius = 12;   // subtle frosted by default
    double m_wallpaperBrightness = 0.5;
    QString m_appearanceMode;          // "" | "glass"
    double m_wallpaperGlassOpacity = 0.10; // wallpaper layer opacity in glass mode
};
