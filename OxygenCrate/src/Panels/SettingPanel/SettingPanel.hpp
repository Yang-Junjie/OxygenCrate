#pragma once

#include <filesystem>
#include <string>

class SettingPanel
{
public:
    struct PanelPreferences
    {
        bool ShowDemoWindow = false;
        bool ShowTextEditor = true;
        bool ShowSchedule = true;
        bool ShowSettingsPanel = false;
        bool ShowLuaConsole = false;
    };

    SettingPanel();

    void Render();

    const PanelPreferences& GetPreferences() const { return m_Preferences; }
    bool ConsumePendingPreferences(PanelPreferences& outPreferences);

private:
    bool LoadFromDisk();
    bool SaveToDisk();
    std::filesystem::path GetStoragePath() const;
    void MarkDirty(const std::string& reason);
    void ClearStatus();
    void ApplyAndSave(const std::string& successMessage);

private:
    PanelPreferences m_Preferences{};
    bool m_Dirty = false;
    bool m_PendingApply = false;
    std::string m_StatusMessage;
};
