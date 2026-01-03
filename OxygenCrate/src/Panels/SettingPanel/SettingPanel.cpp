#include "SettingPanel.hpp"

#include <imgui.h>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>
#include <system_error>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace
{
std::filesystem::path ResolveSettingsDirectory()
{
#ifdef __ANDROID__
    return std::filesystem::path("/storage/emulated/0/Beisent/OxygenCrate/settings");
#else
    auto executableDirectory = []() -> std::filesystem::path {
#if defined(_WIN32)
        wchar_t buffer[MAX_PATH];
        DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (length == 0)
            return std::filesystem::current_path();
        std::filesystem::path exePath(buffer, buffer + length);
        return exePath.parent_path();
#elif defined(__APPLE__)
        char buffer[PATH_MAX];
        uint32_t size = static_cast<uint32_t>(sizeof(buffer));
        if (_NSGetExecutablePath(buffer, &size) == 0)
            return std::filesystem::path(buffer).parent_path();
        std::string dynamicPath(size, '\0');
        if (_NSGetExecutablePath(dynamicPath.data(), &size) == 0)
            return std::filesystem::path(dynamicPath.c_str()).parent_path();
        return std::filesystem::current_path();
#else
        char buffer[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (count > 0)
        {
            buffer[count] = '\0';
            return std::filesystem::path(buffer).parent_path();
        }
        return std::filesystem::current_path();
#endif
    };

    return executableDirectory() / "settings";
#endif
}
} // namespace

SettingPanel::SettingPanel()
{
    if (!LoadFromDisk())
    {
        ClearStatus();
        m_StatusMessage = "Using built-in defaults.";
    }
}

void SettingPanel::Render()
{
    ImGui::Begin("Panel Settings");

    ImGui::TextWrapped("Configure which panels open automatically when the application starts.");
    ImGui::Separator();

    auto renderToggle = [&](const char* label, bool* value) {
        if (ImGui::Checkbox(label, value))
        {
            MarkDirty(std::string("Updated ") + label);
            ApplyAndSave("Preferences saved.");
        }
    };

    renderToggle("Show demo window", &m_Preferences.ShowDemoWindow);
    renderToggle("Show text editor", &m_Preferences.ShowTextEditor);
    renderToggle("Show schedule planner", &m_Preferences.ShowSchedule);
    renderToggle("Show settings panel", &m_Preferences.ShowSettingsPanel);
    renderToggle("Show Lua console", &m_Preferences.ShowLuaConsole);

    if (m_Dirty)
    {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.95f, 0.78f, 0.33f, 1.0f), "Unsaved changes");
    }

    ImGui::Spacing();
    if (ImGui::Button("Apply & Save"))
        ApplyAndSave("Preferences saved.");
    ImGui::SameLine();
    if (ImGui::Button("Reload from disk"))
    {
        if (LoadFromDisk())
        {
            m_PendingApply = true;
            ClearStatus();
            m_StatusMessage = "Settings reloaded.";
        }
    }

    if (!m_StatusMessage.empty())
    {
        ImGui::Spacing();
        ImGui::TextWrapped("%s", m_StatusMessage.c_str());
    }

    ImGui::End();
}

bool SettingPanel::ConsumePendingPreferences(PanelPreferences& outPreferences)
{
    if (!m_PendingApply)
        return false;
    outPreferences = m_Preferences;
    m_PendingApply = false;
    return true;
}

bool SettingPanel::LoadFromDisk()
{
    const std::filesystem::path path = GetStoragePath();
    if (!std::filesystem::exists(path))
        return false;

    try
    {
        YAML::Node root = YAML::LoadFile(path.string());
        auto readBool = [&](const char* key, bool currentValue) {
            const YAML::Node node = root[key];
            if (node && node.IsScalar())
                return node.as<bool>();
            return currentValue;
        };

        m_Preferences.ShowDemoWindow = readBool("show_demo_window", m_Preferences.ShowDemoWindow);
        m_Preferences.ShowTextEditor = readBool("show_text_editor", m_Preferences.ShowTextEditor);
        m_Preferences.ShowSchedule = readBool("show_schedule_panel", m_Preferences.ShowSchedule);
        m_Preferences.ShowSettingsPanel = readBool("show_settings_panel", m_Preferences.ShowSettingsPanel);
        m_Preferences.ShowLuaConsole = readBool("show_lua_console", m_Preferences.ShowLuaConsole);

        m_Dirty = false;
        return true;
    }
    catch (const std::exception& e)
    {
        MarkDirty(std::string("Failed to load settings: ") + e.what());
        return false;
    }
}

bool SettingPanel::SaveToDisk()
{
    const std::filesystem::path path = GetStoragePath();
    try
    {
        if (path.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
        }

        YAML::Node root;
        root["show_demo_window"] = m_Preferences.ShowDemoWindow;
        root["show_text_editor"] = m_Preferences.ShowTextEditor;
        root["show_schedule_panel"] = m_Preferences.ShowSchedule;
        root["show_settings_panel"] = m_Preferences.ShowSettingsPanel;
        root["show_lua_console"] = m_Preferences.ShowLuaConsole;

        std::ofstream stream(path);
        if (!stream.is_open())
        {
            MarkDirty("Failed to open settings file for writing.");
            return false;
        }

        stream << root;
        m_Dirty = false;
        return true;
    }
    catch (const std::exception& e)
    {
        MarkDirty(std::string("Failed to save settings: ") + e.what());
        return false;
    }
}

std::filesystem::path SettingPanel::GetStoragePath() const
{
    std::filesystem::path directory = ResolveSettingsDirectory();
    if (directory.empty())
        directory = std::filesystem::current_path();
    return directory / "settings.yaml";
}

void SettingPanel::MarkDirty(const std::string& reason)
{
    m_Dirty = true;
    if (!reason.empty())
        m_StatusMessage = reason;
}

void SettingPanel::ClearStatus()
{
    m_StatusMessage.clear();
    m_Dirty = false;
}

void SettingPanel::ApplyAndSave(const std::string& successMessage)
{
    if (SaveToDisk())
    {
        m_PendingApply = true;
        ClearStatus();
        m_StatusMessage = successMessage;
    }
}
