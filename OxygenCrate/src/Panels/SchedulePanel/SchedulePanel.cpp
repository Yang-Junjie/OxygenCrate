#include "SchedulePanel.hpp"

#include <imgui.h>
#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <system_error>
#include <ctime>

#include <yaml-cpp/yaml.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace detail
{
template <size_t N>
void CopyString(std::array<char, N>& destination, const std::string& source)
{
    std::snprintf(destination.data(), destination.size(), "%s", source.c_str());
}

template <size_t N>
void CopyString(std::array<char, N>& destination, const char* source)
{
    std::snprintf(destination.data(), destination.size(), "%s", source ? source : "");
}

struct DateComponents
{
    int Year = 2026;
    int Month = 1;
    int Day = 1;
};

struct TimeComponents
{
    int Hour = 9;
    int Minute = 0;
};

DateComponents GetCurrentDate()
{
    DateComponents result;
    std::time_t now = std::time(nullptr);
    if (now == static_cast<std::time_t>(-1))
        return result;
    std::tm timeInfo{};
#if defined(_WIN32)
    localtime_s(&timeInfo, &now);
#elif defined(__ANDROID__) || defined(__linux__)
    localtime_r(&now, &timeInfo);
#else
    std::tm* converted = std::localtime(&now);
    if (converted)
        timeInfo = *converted;
#endif
    result.Year = timeInfo.tm_year + 1900;
    result.Month = timeInfo.tm_mon + 1;
    result.Day = timeInfo.tm_mday;
    return result;
}

TimeComponents GetCurrentTime()
{
    TimeComponents result;
    std::time_t now = std::time(nullptr);
    if (now == static_cast<std::time_t>(-1))
        return result;
    std::tm timeInfo{};
#if defined(_WIN32)
    localtime_s(&timeInfo, &now);
#elif defined(__ANDROID__) || defined(__linux__)
    localtime_r(&now, &timeInfo);
#else
    std::tm* converted = std::localtime(&now);
    if (converted)
        timeInfo = *converted;
#endif
    result.Hour = timeInfo.tm_hour;
    result.Minute = timeInfo.tm_min;
    return result;
}

int DaysInMonth(int year, int month)
{
    static const int kDaysPerMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month < 1 || month > 12)
        return 30;
    int days = kDaysPerMonth[month - 1];
    const bool leapYear = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
    if (month == 2 && leapYear)
        days = 29;
    return days;
}

DateComponents ParseDate(const std::array<char, 16>& value)
{
    DateComponents result = GetCurrentDate();
    int y = 0, m = 0, d = 0;
    if (std::sscanf(value.data(), "%d-%d-%d", &y, &m, &d) == 3)
    {
        result.Year = std::clamp(y, 2000, 2100);
        result.Month = std::clamp(m, 1, 12);
        result.Day = std::clamp(d, 1, DaysInMonth(result.Year, result.Month));
    }
    return result;
}

void WriteDate(const DateComponents& components, std::array<char, 16>& destination)
{
    std::snprintf(destination.data(), destination.size(), "%04d-%02d-%02d",
        components.Year, components.Month, components.Day);
}

TimeComponents ParseTime(const std::array<char, 16>& value)
{
    TimeComponents result = GetCurrentTime();
    int hour = 0, minute = 0;
    if (std::sscanf(value.data(), "%d:%d", &hour, &minute) == 2)
    {
        result.Hour = std::clamp(hour, 0, 23);
        result.Minute = std::clamp(minute, 0, 59);
    }
    return result;
}

void WriteTime(const TimeComponents& components, std::array<char, 16>& destination)
{
    std::snprintf(destination.data(), destination.size(), "%02d:%02d",
        std::clamp(components.Hour, 0, 23),
        std::clamp(components.Minute, 0, 59));
}

bool ParseLegacyRange(const std::string& legacy, TimeComponents& begin, TimeComponents& end)
{
    if (legacy.empty())
        return false;
    int sh = 0, sm = 0, eh = 0, em = 0;
    if (std::sscanf(legacy.c_str(), "%d:%d - %d:%d", &sh, &sm, &eh, &em) == 4 ||
        std::sscanf(legacy.c_str(), "%d:%d-%d:%d", &sh, &sm, &eh, &em) == 4)
    {
        begin.Hour = std::clamp(sh, 0, 23);
        begin.Minute = std::clamp(sm, 0, 59);
        end.Hour = std::clamp(eh, 0, 23);
        end.Minute = std::clamp(em, 0, 59);
        return true;
    }
    return false;
}

constexpr const char* kWeekdays[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

bool DrawDatePicker(const char* popupId, std::array<char, 16>& destination)
{
    bool changed = false;
    const char* displayText = destination[0] ? destination.data() : "Select date";
    if (ImGui::Button(displayText, ImVec2(-FLT_MIN, 0.0f)))
        ImGui::OpenPopup(popupId);

    if (ImGui::BeginPopup(popupId))
    {
        DateComponents date = ParseDate(destination);
        int year = date.Year;
        if (ImGui::InputInt("Year", &year))
        {
            date.Year = std::clamp(year, 2000, 2100);
            date.Day = std::clamp(date.Day, 1, DaysInMonth(date.Year, date.Month));
            WriteDate(date, destination);
            changed = true;
        }

        int month = date.Month;
        if (ImGui::SliderInt("Month", &month, 1, 12))
        {
            date.Month = month;
            date.Day = std::clamp(date.Day, 1, DaysInMonth(date.Year, date.Month));
            WriteDate(date, destination);
            changed = true;
        }

        int maxDay = DaysInMonth(date.Year, date.Month);
        int day = std::clamp(date.Day, 1, maxDay);
        if (ImGui::SliderInt("Day", &day, 1, maxDay))
        {
            date.Day = day;
            WriteDate(date, destination);
            changed = true;
        }

        if (ImGui::Button("Today"))
        {
            WriteDate(GetCurrentDate(), destination);
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            destination[0] = '\0';
            changed = true;
        }

        ImGui::EndPopup();
    }
    return changed;
}

bool DrawTimePicker(const char* popupId, std::array<char, 16>& destination)
{
    bool changed = false;
    const char* displayText = destination[0] ? destination.data() : "HH:MM";
    if (ImGui::Button(displayText, ImVec2(-FLT_MIN, 0.0f)))
        ImGui::OpenPopup(popupId);

    if (ImGui::BeginPopup(popupId))
    {
        TimeComponents time = ParseTime(destination);
        int hour = time.Hour;
        if (ImGui::SliderInt("Hour", &hour, 0, 23))
        {
            time.Hour = hour;
            WriteTime(time, destination);
            changed = true;
        }
        int minute = time.Minute;
        if (ImGui::SliderInt("Minute", &minute, 0, 59))
        {
            time.Minute = minute;
            WriteTime(time, destination);
            changed = true;
        }

        if (ImGui::Button("Now"))
        {
            WriteTime(GetCurrentTime(), destination);
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            destination[0] = '\0';
            changed = true;
        }

        ImGui::EndPopup();
    }
    return changed;
}

bool DrawWeekdaySelector(std::array<char, 16>& destination)
{
    bool changed = false;
    const char* preview = destination[0] ? destination.data() : "Select day";
    if (ImGui::BeginCombo("##DayCombo", preview))
    {
        for (const char* option : kWeekdays)
        {
            const bool selected = std::strncmp(destination.data(), option, destination.size()) == 0;
            if (ImGui::Selectable(option, selected))
            {
                CopyString(destination, option);
                changed = true;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::Separator();
        if (ImGui::InputText("Custom", destination.data(), destination.size()))
            changed = true;
        ImGui::EndCombo();
    }
    return changed;
}

} // namespace detail

SchedulePanel::SchedulePanel()
{
    if (!LoadFromDisk())
    {
        PopulateDefaultEntries();
        MarkDirty("Loaded default sample schedule");
    }
}

void SchedulePanel::Render()
{
    if (!m_ShowPanel)
        return;

    if (!ImGui::Begin("Schedule Planner", &m_ShowPanel))
    {
        ImGui::End();
        return;
    }

    RenderStatusLine();
    RenderToolbar();
    RenderScheduleTable();

    ImGui::End();
}

void SchedulePanel::RenderStatusLine()
{
    const std::string storagePath = GetStoragePath().string();
    ImGui::TextUnformatted("Plan courses, assign dates, and store them in YAML.");
    ImGui::TextWrapped("Storage file: %s", storagePath.c_str());
    if (!m_StatusMessage.empty())
    {
        const ImVec4 color = m_Dirty ? ImVec4(0.95f, 0.78f, 0.33f, 1.0f) : ImVec4(0.6f, 0.9f, 0.6f, 1.0f);
        ImGui::TextColored(color, "%s", m_StatusMessage.c_str());
    }
}

void SchedulePanel::RenderToolbar()
{
    if (ImGui::Button("Add Course"))
    {
        m_ScheduleEntries.emplace_back();
        MarkDirty("Added new course entry");
    }
    ImGui::SameLine();
    const bool hasEntries = !m_ScheduleEntries.empty();
    if (!hasEntries)
        ImGui::BeginDisabled();
    if (ImGui::Button("Clear All"))
    {
        m_ScheduleEntries.clear();
        MarkDirty("Cleared schedule");
    }
    if (!hasEntries)
        ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Reload"))
    {
        if (!LoadFromDisk())
            PopulateDefaultEntries();
    }

    ImGui::SameLine();
    if (ImGui::Button("Save"))
        SaveToDisk();
}

void SchedulePanel::RenderScheduleTable()
{
    const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;

    if (!ImGui::BeginTable("ScheduleTable", 7, tableFlags))
        return;

    ImGui::TableSetupColumn("Course");
    ImGui::TableSetupColumn("Date");
    ImGui::TableSetupColumn("Day");
    ImGui::TableSetupColumn("Start");
    ImGui::TableSetupColumn("End");
    ImGui::TableSetupColumn("Location");
    ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableHeadersRow();

    int rowToRemove = -1;
    for (int idx = 0; idx < static_cast<int>(m_ScheduleEntries.size()); ++idx)
    {
        ImGui::PushID(idx);
        ImGui::TableNextRow();
        if (DrawScheduleRow(idx, m_ScheduleEntries[idx]))
            rowToRemove = idx;
        ImGui::PopID();
    }

    if (rowToRemove >= 0 && rowToRemove < static_cast<int>(m_ScheduleEntries.size()))
        m_ScheduleEntries.erase(m_ScheduleEntries.begin() + rowToRemove);

    ImGui::EndTable();
}

bool SchedulePanel::DrawScheduleRow(int index, ScheduleEntry& entry)
{
    (void)index;
    ImGui::TableSetColumnIndex(0);
    ImGui::SetNextItemWidth(-FLT_MIN);
    bool dirty = ImGui::InputText("##Course", entry.Course.data(), entry.Course.size());

    ImGui::TableSetColumnIndex(1);
    ImGui::PushID("Date");
    ImGui::SetNextItemWidth(-FLT_MIN);
    dirty = detail::DrawDatePicker("DatePicker", entry.Date) || dirty;
    ImGui::PopID();

    ImGui::TableSetColumnIndex(2);
    ImGui::PushID("Day");
    ImGui::SetNextItemWidth(-FLT_MIN);
    dirty = detail::DrawWeekdaySelector(entry.Day) || dirty;
    ImGui::PopID();

    ImGui::TableSetColumnIndex(3);
    ImGui::PushID("Start");
    ImGui::SetNextItemWidth(-FLT_MIN);
    dirty = detail::DrawTimePicker("StartPicker", entry.Start) || dirty;
    ImGui::PopID();

    ImGui::TableSetColumnIndex(4);
    ImGui::PushID("End");
    ImGui::SetNextItemWidth(-FLT_MIN);
    dirty = detail::DrawTimePicker("EndPicker", entry.End) || dirty;
    ImGui::PopID();

    ImGui::TableSetColumnIndex(5);
    ImGui::SetNextItemWidth(-FLT_MIN);
    dirty = ImGui::InputText("##Location", entry.Location.data(), entry.Location.size()) || dirty;

    if (dirty)
        MarkDirty("Entry modified");

    ImGui::TableSetColumnIndex(6);
    if (ImGui::Button("Delete"))
    {
        MarkDirty("Entry removed");
        return true;
    }
    return false;
}

void SchedulePanel::PopulateDefaultEntries()
{
    m_ScheduleEntries.clear();
    m_ScheduleEntries.resize(3);

    detail::CopyString(m_ScheduleEntries[0].Course, "Graphics 101");
    detail::CopyString(m_ScheduleEntries[0].Date, "2026-03-01");
    detail::CopyString(m_ScheduleEntries[0].Day, "Mon");
    detail::CopyString(m_ScheduleEntries[0].Start, "09:00");
    detail::CopyString(m_ScheduleEntries[0].End, "10:30");
    detail::CopyString(m_ScheduleEntries[0].Location, "Room A1");

    detail::CopyString(m_ScheduleEntries[1].Course, "Linear Algebra");
    detail::CopyString(m_ScheduleEntries[1].Date, "2026-03-03");
    detail::CopyString(m_ScheduleEntries[1].Day, "Wed");
    detail::CopyString(m_ScheduleEntries[1].Start, "14:00");
    detail::CopyString(m_ScheduleEntries[1].End, "15:30");
    detail::CopyString(m_ScheduleEntries[1].Location, "Room D2");

    detail::CopyString(m_ScheduleEntries[2].Course, "Game Dev Lab");
    detail::CopyString(m_ScheduleEntries[2].Date, "2026-03-05");
    detail::CopyString(m_ScheduleEntries[2].Day, "Fri");
    detail::CopyString(m_ScheduleEntries[2].Start, "10:00");
    detail::CopyString(m_ScheduleEntries[2].End, "12:00");
    detail::CopyString(m_ScheduleEntries[2].Location, "Lab 3");
}

bool SchedulePanel::LoadFromDisk()
{
    const std::filesystem::path path = GetStoragePath();
    if (!std::filesystem::exists(path))
    {
        m_ScheduleEntries.clear();
        MarkDirty("Schedule file not found. Edit entries and click Save.");
        return false;
    }

    try
    {
        YAML::Node root = YAML::LoadFile(path.string());
        std::vector<ScheduleEntry> loaded;
        if (root && root.IsSequence())
        {
            for (const YAML::Node& node : root)
            {
                ScheduleEntry entry{};
                auto getScalar = [&node](const char* key) -> std::string {
                    const YAML::Node valueNode = node[key];
                    if (valueNode && valueNode.IsScalar())
                        return valueNode.as<std::string>();
                    return {};
                };
                auto copyValue = [&](auto& dest, const char* newKey, const char* legacyKey) {
                    std::string value = getScalar(newKey);
                    if (value.empty() && legacyKey)
                        value = getScalar(legacyKey);
                    detail::CopyString(dest, value.c_str());
                };

                detail::CopyString(entry.Course, getScalar("course").c_str());
                copyValue(entry.Date, "date", nullptr);
                copyValue(entry.Day, "day", nullptr);
                copyValue(entry.Start, "start", nullptr);
                copyValue(entry.End, "end", nullptr);

                std::string legacyTime = getScalar("time");
                if (!legacyTime.empty())
                {
                    detail::TimeComponents legacyStart = detail::GetCurrentTime();
                    detail::TimeComponents legacyEnd = detail::GetCurrentTime();
                    if (detail::ParseLegacyRange(legacyTime, legacyStart, legacyEnd))
                    {
                        detail::WriteTime(legacyStart, entry.Start);
                        detail::WriteTime(legacyEnd, entry.End);
                    }
                }

                copyValue(entry.Location, "location", nullptr);
                loaded.push_back(entry);
            }
        }

        m_ScheduleEntries = std::move(loaded);
        ClearStatus();
        m_StatusMessage = "Loaded schedule from " + path.string();
        return true;
    }
    catch (const std::exception& e)
    {
        MarkDirty(std::string("Failed to load schedule: ") + e.what());
        return false;
    }
}

void SchedulePanel::SaveToDisk()
{
    const std::filesystem::path path = GetStoragePath();
    try
    {
        if (path.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
        }

        YAML::Node root(YAML::NodeType::Sequence);
        for (const auto& entry : m_ScheduleEntries)
        {
            YAML::Node node;
            node["course"] = std::string(entry.Course.data());
            node["date"] = std::string(entry.Date.data());
            node["day"] = std::string(entry.Day.data());
            node["start"] = std::string(entry.Start.data());
            node["end"] = std::string(entry.End.data());
            node["location"] = std::string(entry.Location.data());
            root.push_back(node);
        }

        std::ofstream stream(path);
        if (!stream.is_open())
        {
            m_StatusMessage = "Failed to open " + path.string() + " for writing.";
            return;
        }

        stream << root;
        ClearStatus();
        m_StatusMessage = "Saved schedule to " + path.string();
    }
    catch (const std::exception& e)
    {
        MarkDirty(std::string("Failed to save schedule: ") + e.what());
    }
}

std::filesystem::path SchedulePanel::GetStoragePath() const
{
    auto resolveBaseDirectory = []() -> std::filesystem::path {
#ifdef __ANDROID__
        return std::filesystem::path("/storage/emulated/0/Beisent/OxygenCrate/schedule");
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

        return executableDirectory() / "schedule";
#endif
    };

    std::filesystem::path baseDir = resolveBaseDirectory();
    if (baseDir.empty())
        baseDir = std::filesystem::current_path();
    return baseDir / "schedule.yaml";
}

void SchedulePanel::MarkDirty(const std::string& reason)
{
    m_Dirty = true;
    if (!reason.empty())
        m_StatusMessage = reason;
    else if (m_StatusMessage.empty())
        m_StatusMessage = "Unsaved changes";
}

void SchedulePanel::ClearStatus()
{
    m_Dirty = false;
    m_StatusMessage.clear();
}
