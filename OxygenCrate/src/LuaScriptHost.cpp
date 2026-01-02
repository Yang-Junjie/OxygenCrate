#include "LuaScriptHost.hpp"
#include "../../external/Flux/Flux/Core/src/Image.hpp"
#include "LuaGLBindings.hpp"
#include "GLWrappers.hpp"
#include <imgui_internal.h>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
extern android_app* g_AndroidApp;
#endif

#if !defined(__ANDROID__)
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#else
#include <unistd.h>
#include <limits.h>
#endif
#endif

namespace {
constexpr const char* kSampleScriptFileName = "Sample.lua";
constexpr const char* kVec2ModuleName = "Vec2.lua";
}

std::string ReadTextFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::in | std::ios::binary);
    if (!stream.is_open())
        return {};
    return std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}

#if !defined(__ANDROID__)
namespace {
std::filesystem::path GetExecutableDirectory()
{
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
}
} // namespace
#endif

#ifdef __ANDROID__
std::string ReadAssetFile(const std::string& relativePath)
{
    if (!g_AndroidApp || !g_AndroidApp->activity || !g_AndroidApp->activity->assetManager)
        return {};
    AAssetManager* manager = g_AndroidApp->activity->assetManager;
    if (!manager)
        return {};
    AAsset* asset = AAssetManager_open(manager, relativePath.c_str(), AASSET_MODE_BUFFER);
    if (!asset)
        return {};
    const off_t length = AAsset_getLength(asset);
    std::string buffer;
    buffer.resize(static_cast<size_t>(length));
    int64_t read = AAsset_read(asset, buffer.data(), buffer.size());
    if (read < 0)
    {
        buffer.clear();
    }
    else
    {
        buffer.resize(static_cast<size_t>(read));
    }
    AAsset_close(asset);
    return buffer;
}
#else
std::filesystem::path GetDesktopAssetDirectory()
{
    return GetExecutableDirectory() / "assets";
}

std::string ReadAssetFile(const std::string& relativePath)
{
    const std::filesystem::path fullPath = GetDesktopAssetDirectory() / relativePath;
    return ReadTextFile(fullPath);
}
#endif

bool WriteTextFile(const std::filesystem::path& path, const std::string& contents)
{
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream stream(path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!stream.is_open())
        return false;
    stream << contents;
    return stream.good();
}

std::filesystem::path LuaScriptHost::GetModuleDirectory()
{
#ifdef __ANDROID__
    std::filesystem::path dir("/storage/emulated/0/Beisent/OxygenCrate/lua");
#else
    std::filesystem::path base = GetExecutableDirectory();
    std::filesystem::path dir = base / "lua";
#endif
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

static std::filesystem::path GetSampleScriptPath()
{
    return LuaScriptHost::GetModuleDirectory() / kSampleScriptFileName;
}

void LuaScriptHost::EnsureDefaultModulesInstalled()
{
    static bool s_Installed = false;
    if (s_Installed)
        return;

    const std::filesystem::path moduleDir = GetModuleDirectory();
    const std::filesystem::path vec2Path = moduleDir / kVec2ModuleName;
    const std::filesystem::path samplePath = moduleDir / kSampleScriptFileName;

    auto ensureFromAsset = [&](const char* assetRelativePath, const std::filesystem::path& destination) {
        if (std::filesystem::exists(destination))
            return;
        std::string data = ReadAssetFile(assetRelativePath);
        if (data.empty())
            return;
        WriteTextFile(destination, data);
    };

    ensureFromAsset("lua/Vec2.lua", vec2Path);
    ensureFromAsset("lua/Sample.lua", samplePath);

    s_Installed = true;
}

LuaScriptHost::LuaScriptHost()
{
    EnsureDefaultModulesInstalled();
    std::string sample = ReadTextFile(GetSampleScriptPath());
    if (sample.empty())
    {
        sample = "-- Sample.lua not found in the lua directory. Add your own script and press \"Run Lua Script\".";
    }
    m_SampleScript = std::move(sample);
    AppendConsoleLine("Load or type a Lua script, then press \"Run Lua Script\".");
}

bool LuaScriptHost::CompileScript(const std::string& script)
{
    m_LuaError.clear();
    m_LuaDrawFunction = sol::protected_function{};
    m_IsScriptReady = false;

    if (script.empty())
    {
        m_LuaError = "Script editor is empty. Load the sample or write your own Lua code.";
        AppendConsoleLine(std::string("[Error] ") + m_LuaError);
        return false;
    }

    try
    {
        m_LuaState = sol::state{};
        m_LuaState.open_libraries(sol::lib::base,
            sol::lib::math,
            sol::lib::string,
            sol::lib::os,
            sol::lib::package,
            sol::lib::table);

        const std::string moduleDir = GetModuleDirectory().generic_string();
        sol::table packageTable = m_LuaState["package"];
        if (packageTable.valid())
        {
            std::string currentPath = packageTable.get_or("path", std::string{});
            std::string newPath = moduleDir + "/?.lua;" + moduleDir + "/?/init.lua;";
            if (!currentPath.empty())
                newPath += currentPath;
            packageTable["path"] = newPath;
        }

        sol::table imguiTable = m_LuaState.create_named_table("imgui");
        imguiTable.set_function("button", [](const std::string& label)
        {
            return ImGui::Button(label.c_str());
        });
        imguiTable.set_function("text", [](const std::string& text)
        {
            ImGui::TextUnformatted(text.c_str());
        });
        imguiTable.set_function("text_wrapped", [](const std::string& text)
        {
            ImGui::TextWrapped("%s", text.c_str());
        });
        imguiTable.set_function("same_line", []()
        {
            ImGui::SameLine();
        });
        imguiTable.set_function("spacing", []()
        {
            ImGui::Spacing();
        });
        imguiTable.set_function("separator", []()
        {
            ImGui::Separator();
        });
        imguiTable.set_function("begin_window", [](const std::string& title, sol::optional<sol::table> options)
        {
            bool openValue = true;
            bool* openPtr = nullptr;
            ImGuiWindowFlags flags = 0;
            sol::table opts;
            if (options)
            {
                opts = *options;
                openValue = opts.get_or("open", true);
                flags = static_cast<ImGuiWindowFlags>(opts.get_or("flags", 0));
                openPtr = &openValue;
            }
            bool visible = ImGui::Begin(title.c_str(), openPtr, flags);
            if (options && openPtr)
                opts["open"] = openValue;
            return visible;
        });
        imguiTable.set_function("end_window", []()
        {
            ImGui::End();
        });
        imguiTable.set_function("checkbox", [](const std::string& label, bool current)
        {
            bool value = current;
            ImGui::Checkbox(label.c_str(), &value);
            return value;
        });
        imguiTable.set_function("slider_float", [](const std::string& label, float current, float minValue, float maxValue)
        {
            float value = current;
            ImGui::SliderFloat(label.c_str(), &value, minValue, maxValue);
            return value;
        });
        imguiTable.set_function("image", [this](int imageId, float width, float height)
        {
            Flux::Image* image = GetLuaImage(imageId);
            if (!image)
                return;
            ImTextureID textureID = static_cast<ImTextureID>(static_cast<uintptr_t>(image->GetColorAttachment()));
            ImGui::Image(textureID, ImVec2(width, height));
        });
        m_LuaState.create_named_table("opengl");
        m_LuaState.create_named_table("opengles");
        LuaGLBindings::Register(m_LuaState);

        m_LuaState.set_function("log", [this](sol::variadic_args args, sol::this_state thisState)
        {
            sol::state_view lua(thisState);
            sol::function tostring = lua["tostring"];
            std::string line = "[Lua] ";
            bool first = true;
            for (auto value : args)
            {
                sol::object obj(value.lua_state(), value.stack_index());
                std::string part;
                if (tostring.valid())
                {
                    sol::object converted = tostring(obj);
                    part = converted.as<std::string>();
                }
                else if (obj.is<std::string>())
                {
                    part = obj.as<std::string>();
                }
                else
                {
                    part = "<non-printable>";
                }

                if (!first)
                    line += "\t";
                line += part;
                first = false;
            }
            AppendConsoleLine(line);
        });
        m_LuaState.set_function("create_image", [this](uint32_t width, uint32_t height)
        {
            int handle = CreateLuaImage(width, height);
            if (handle < 0)
                AppendConsoleLine("[Error] Failed to create image");
            return handle;
        });
        m_LuaState.set_function("set_image_data", [this](int imageId, sol::as_table_t<std::vector<uint8_t>> pixelData)
        {
            Flux::Image* image = GetLuaImage(imageId);
            if (!image)
                return false;
            auto data = pixelData.value();
            const size_t required = static_cast<size_t>(image->GetWidth()) * static_cast<size_t>(image->GetHeight()) * 4;
            if (data.size() != required)
            {
                AppendConsoleLine("[Error] set_image_data: pixel buffer does not match image size");
                return false;
            }
            for (uint8_t& value : data)
                value = static_cast<uint8_t>(std::clamp<int>(value, 0, 255));
            m_ImageScratchBuffer = std::move(data);
            image->SetData(m_ImageScratchBuffer.data());
            return true;
        });
        sol::table fluxImageTable = m_LuaState.create_named_table("flux_image");
        fluxImageTable.set_function("bind_framebuffer", [this](int imageId)
        {
            Flux::Image* image = GetLuaImage(imageId);
            if (!image)
            {
                AppendConsoleLine("[Error] flux_image.bind_framebuffer: invalid image handle");
                return false;
            }
            Flux::GL::BindFramebuffer(GL_FRAMEBUFFER, image->GetFramebuffer());
            return true;
        });
        fluxImageTable.set_function("unbind_framebuffer", []()
        {
            Flux::GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
        });

        AppendConsoleLine("Running Lua script...");
        sol::protected_function_result result = m_LuaState.safe_script(script, sol::script_pass_on_error);
        if (!result.valid())
        {
            sol::error err = result;
            throw std::runtime_error(err.what());
        }

        sol::object returnValue = result;
        if (!returnValue.is<sol::table>())
            throw std::runtime_error("Script must return a table that contains a draw() function.");

        sol::table uiTable = returnValue.as<sol::table>();
        sol::object drawObj = uiTable["draw"];
        if (!drawObj.valid() || drawObj.get_type() != sol::type::function)
            throw std::runtime_error("Returned table must contain a function named 'draw'.");

        m_LuaDrawFunction = drawObj.as<sol::protected_function>();
        AppendConsoleLine("Lua script compiled successfully.");
        m_IsScriptReady = true;
        return true;
    }
    catch (const std::exception& e)
    {
        m_LuaDrawFunction = sol::protected_function{};
        m_LuaError = e.what();
        AppendConsoleLine(std::string("[Error] ") + m_LuaError);
        return false;
    }
}

void LuaScriptHost::Draw()
{
    if (!m_LuaDrawFunction.valid())
        return;

    sol::protected_function_result callResult = m_LuaDrawFunction();
    if (!callResult.valid())
    {
        sol::error err = callResult;
        m_LuaError = err.what();
        AppendConsoleLine(std::string("[Error] ") + m_LuaError);
        m_LuaDrawFunction = sol::protected_function{};
        m_IsScriptReady = false;
    }
}

void LuaScriptHost::ClearConsole()
{
    m_LuaConsoleLines.clear();
    m_ScrollConsoleToBottom = true;
}

bool LuaScriptHost::ExecuteConsoleCommand(const std::string& command)
{
    if (command.empty())
        return false;

    AppendConsoleLine(std::string(">> ") + command);

    if (!m_IsScriptReady)
    {
        AppendConsoleLine("[Error] Run a Lua script before sending console commands.");
        return false;
    }

    sol::protected_function_result result = m_LuaState.safe_script(command, sol::script_pass_on_error);
    if (!result.valid())
    {
        sol::error err = result;
        AppendConsoleLine(std::string("[Error] ") + err.what());
        return false;
    }

    AppendConsoleLine("[Result] ok");
    return true;
}

void LuaScriptHost::AppendConsoleLine(const std::string& line)
{
    m_LuaConsoleLines.push_back(line);
    while (m_LuaConsoleLines.size() > kMaxConsoleLines)
        m_LuaConsoleLines.erase(m_LuaConsoleLines.begin());
    m_ScrollConsoleToBottom = true;
}

int LuaScriptHost::CreateLuaImage(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return -1;

    auto image = std::make_unique<Flux::Image>(width, height);

    const int handle = m_NextImageId++;
    m_LuaImages[handle] = std::move(image);
    return handle;
}

Flux::Image* LuaScriptHost::GetLuaImage(int imageId)
{
    auto it = m_LuaImages.find(imageId);
    if (it == m_LuaImages.end())
        return nullptr;
    return it->second.get();
}
