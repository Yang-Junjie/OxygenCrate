#pragma once

#include <sol/sol.hpp>
#include "../../external/Flux/Flux/Core/src/Image.hpp"
#include "LuaGLBindings.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Flux { class Image; }

class LuaScriptHost {
public:
    LuaScriptHost();

    bool CompileScript(const std::string& script);
    void Draw();
    void ClearConsole();
    bool ExecuteConsoleCommand(const std::string& command);

    const std::vector<std::string>& GetConsoleLines() const { return m_LuaConsoleLines; }
    bool ShouldScrollConsole() const { return m_ScrollConsoleToBottom; }
    void AcknowledgeConsoleScroll() { m_ScrollConsoleToBottom = false; }
    bool HasDrawFunction() const { return m_LuaDrawFunction.valid(); }
    void Update(float deltaTime);
    const std::string& GetLastError() const { return m_LuaError; }
    const std::string& GetSampleScript() const { return m_SampleScript; }
    bool IsReady() const { return m_IsScriptReady; }
    static std::filesystem::path GetModuleDirectory();
    static void EnsureDefaultModulesInstalled();

private:
    void AppendConsoleLine(const std::string& line);
    int CreateLuaImage(uint32_t width, uint32_t height);
    Flux::Image* GetLuaImage(int imageId);

    sol::state m_LuaState;
    sol::protected_function m_LuaDrawFunction;
    sol::protected_function m_LuaUpdateFunction;
    std::vector<std::string> m_LuaConsoleLines;
    bool m_ScrollConsoleToBottom = false;
    std::string m_LuaError;
    std::string m_SampleScript;
    bool m_IsScriptReady = false;
    std::unordered_map<int, std::unique_ptr<Flux::Image>> m_LuaImages;
    int m_NextImageId = 1;
    std::vector<uint8_t> m_ImageScratchBuffer;
    static constexpr size_t kMaxConsoleLines = 200;
};
