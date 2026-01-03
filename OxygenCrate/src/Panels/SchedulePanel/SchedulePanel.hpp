#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

class SchedulePanel
{
public:
    SchedulePanel();
    void Render();

private:
    struct ScheduleEntry
    {
        std::array<char, 64> Course{};
        std::array<char, 16> Day{};
        std::array<char, 16> Start{};
        std::array<char, 16> End{};
        std::array<char, 64> Location{};
    };

    void RenderStatusLine();
    void RenderToolbar();
    void RenderScheduleTable();
    bool DrawScheduleRow(int index, ScheduleEntry& entry);

    void PopulateDefaultEntries();
    bool LoadFromDisk();
    void SaveToDisk();
    std::filesystem::path GetStoragePath() const;
    void MarkDirty(const std::string& reason);
    void ClearStatus();
    void SortEntries();

private:
    std::vector<ScheduleEntry> m_ScheduleEntries;
    std::string m_StatusMessage;
    bool m_ShowPanel = true;
    bool m_Dirty = false;
};
