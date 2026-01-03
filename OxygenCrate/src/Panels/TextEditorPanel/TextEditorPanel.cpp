#include "TextEditorPanel.hpp"
#include "Panels/LuaPanels/LuaScriptHost.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>
#include <cstdlib>
#include <cstdio>

#ifdef __ANDROID__
#include "../Platform/Android/FilePicker.hpp"
#else
#include "../Platform/Desktop/FileDialog.hpp"
#endif

namespace
{
    std::string ReadFileToString(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (!file.good())
            return {};

        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
}

TextEditorPanel::TextEditorPanel()
{
    LuaScriptHost::EnsureDefaultModulesInstalled();
    InitializeTextEditor();
}

void TextEditorPanel::InitializeTextEditor()
{
    auto lang = TextEditor::LanguageDefinition::Lua();

    static const char* hostIdentifiers[] = {
        "log", "create_image", "set_image_data",
        "imgui", "opengl", "opengles", "flux_image",
        "set_clear_color"
    };
    for (const char* name : hostIdentifiers)
    {
        TextEditor::Identifier id;
        id.mDeclaration = "Host API";
        lang.mIdentifiers.insert(std::make_pair(std::string(name), id));
    }

    m_TextEditor.SetLanguageDefinition(lang);
    m_TextEditor.SetPalette(TextEditor::GetDarkPalette());

    m_StatusMessage = "No file loaded";
    m_FileToEdit.clear();
}

std::string TextEditorPanel::GetText() const
{
    return m_TextEditor.GetText();
}

void TextEditorPanel::SetText(const std::string& text)
{
    m_TextEditor.SetText(text);
}

void TextEditorPanel::Render()
{
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Lua Script Editor", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar))
    {
        const auto cpos = m_TextEditor.GetCursorPosition();

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "Ctrl+N"))
                    BeginNewFile();
                if (ImGui::MenuItem("Open...", "Ctrl+O"))
                {
                    auto suggested = !m_FileToEdit.empty() ? m_FileToEdit : GetDefaultDirectory().string();
                    std::snprintf(m_OpenPathBuffer.data(), m_OpenPathBuffer.size(), "%s", suggested.c_str());
                    m_OpenPopupRequested = true;
                }
                if (ImGui::MenuItem("Save", "Ctrl+S", false, !m_TextEditor.IsReadOnly()))
                    TrySaveFile();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                bool readOnly = m_TextEditor.IsReadOnly();
                if (ImGui::MenuItem("Read-only mode", nullptr, &readOnly))
                    m_TextEditor.SetReadOnly(readOnly);

                ImGui::Separator();
                if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !readOnly && m_TextEditor.CanUndo()))
                    m_TextEditor.Undo();
                if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !readOnly && m_TextEditor.CanRedo()))
                    m_TextEditor.Redo();

                ImGui::Separator();
                if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, m_TextEditor.HasSelection()))
                    m_TextEditor.Copy();
                if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !readOnly && m_TextEditor.HasSelection()))
                    m_TextEditor.Cut();
                if (ImGui::MenuItem("Delete", "Del", nullptr, !readOnly && m_TextEditor.HasSelection()))
                    m_TextEditor.Delete();
                if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !readOnly && ImGui::GetClipboardText() != nullptr))
                    m_TextEditor.Paste();

                ImGui::Separator();
                if (ImGui::MenuItem("Select all", nullptr))
                    m_TextEditor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(m_TextEditor.GetTotalLines(), 0));

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Dark palette"))
                    m_TextEditor.SetPalette(TextEditor::GetDarkPalette());
                if (ImGui::MenuItem("Light palette"))
                    m_TextEditor.SetPalette(TextEditor::GetLightPalette());
                if (ImGui::MenuItem("Retro blue palette"))
                    m_TextEditor.SetPalette(TextEditor::GetRetroBluePalette());
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s",
            cpos.mLine + 1, cpos.mColumn + 1, m_TextEditor.GetTotalLines(),
            m_TextEditor.IsOverwrite() ? "Ovr" : "Ins",
            m_TextEditor.CanUndo() ? "*" : " ",
            m_TextEditor.GetLanguageDefinition().mName.c_str());

        if (!m_StatusMessage.empty())
            ImGui::TextUnformatted(m_StatusMessage.c_str());

        m_TextEditor.Render("TextEditor");
    }
    ImGui::End();

    DrawOpenFilePopup();
    DrawNewFilePopup();

#ifdef __ANDROID__
    {
        const std::string polledPath = OxygenCrate::Platform::PollSelectedFile();
        if (!polledPath.empty())
        {
            std::snprintf(m_OpenPathBuffer.data(), m_OpenPathBuffer.size(), "%s", polledPath.c_str());
            OpenFileAtPath(polledPath);
            m_CloseOpenPopup = true;
        }
    }
#endif
}

void TextEditorPanel::TrySaveFile()
{
    if (m_FileToEdit.empty())
    {
        auto defaultDir = GetDefaultDirectory();
        std::error_code ec;
        std::filesystem::create_directories(defaultDir, ec);
        auto fallback = defaultDir / "untitled.txt";
        m_FileToEdit = fallback.string();
    }

    std::ofstream file(m_FileToEdit, std::ios::out | std::ios::trunc);
    if (!file.good())
    {
        m_StatusMessage = "Failed to save file: " + m_FileToEdit;
        return;
    }

    file << m_TextEditor.GetText();
    m_StatusMessage = "Saved to " + m_FileToEdit;
}

void TextEditorPanel::BeginNewFile()
{
    auto defaultPath = GetDefaultDirectory() / "untitled.txt";
    std::snprintf(m_NewFilePathBuffer.data(), m_NewFilePathBuffer.size(), "%s", defaultPath.string().c_str());
    m_NewPopupRequested = true;
}

void TextEditorPanel::CreateNewFileAtPath(const std::string& path)
{
    if (path.empty())
    {
        m_StatusMessage = "File name required";
        return;
    }

    std::filesystem::path filePath = std::filesystem::absolute(path);
    auto parent = filePath.parent_path();
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);

    {
        std::ofstream file(m_FileToEdit, std::ios::out | std::ios::trunc);
    }

    m_TextEditor.SetText("");
    m_FileToEdit = filePath.string();
    m_StatusMessage = "New file: " + m_FileToEdit;
}

void TextEditorPanel::OpenFileAtPath(const std::string& path)
{
    if (path.empty())
    {
        m_StatusMessage = "No path provided";
        return;
    }

    std::filesystem::path filePath = path;
    auto absolutePath = std::filesystem::absolute(filePath);
    if (!std::filesystem::exists(absolutePath))
    {
        m_StatusMessage = "File not found: " + absolutePath.string();
        return;
    }

    auto text = ReadFileToString(absolutePath);
    m_TextEditor.SetText(text);
    m_FileToEdit = absolutePath.string();
    m_StatusMessage = "Opened " + m_FileToEdit;
}

void TextEditorPanel::DrawOpenFilePopup()
{
    if (m_OpenPopupRequested)
    {
        ImGui::OpenPopup("Open File");
        m_OpenPopupRequested = false;
    }

    if (ImGui::BeginPopupModal("Open File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Path", m_OpenPathBuffer.data(), m_OpenPathBuffer.size());
#ifdef __ANDROID__
        ImGui::SameLine();
        if (ImGui::Button("Browse##AndroidFilePicker"))
            OxygenCrate::Platform::RequestFilePicker();
#else
        ImGui::SameLine();
        if (ImGui::Button("Browse##DesktopFilePicker"))
        {
            const std::string selected = OxygenCrate::Platform::OpenFileDialog();
            if (!selected.empty())
            {
                std::snprintf(m_OpenPathBuffer.data(), m_OpenPathBuffer.size(), "%s", selected.c_str());
                OpenFileAtPath(selected);
                m_CloseOpenPopup = true;
            }
        }
#endif

        if (ImGui::Button("Open"))
        {
            OpenFileAtPath(m_OpenPathBuffer.data());
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();

        if (m_CloseOpenPopup)
        {
            ImGui::CloseCurrentPopup();
            m_CloseOpenPopup = false;
        }

        ImGui::EndPopup();
    }
}

void TextEditorPanel::DrawNewFilePopup()
{
    if (m_NewPopupRequested)
    {
        ImGui::OpenPopup("Create New File");
        m_NewPopupRequested = false;
    }

    if (ImGui::BeginPopupModal("Create New File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("File Path", m_NewFilePathBuffer.data(), m_NewFilePathBuffer.size());

        if (ImGui::Button("Create"))
        {
            CreateNewFileAtPath(m_NewFilePathBuffer.data());
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

std::filesystem::path TextEditorPanel::GetDefaultDirectory() const
{
    std::filesystem::path moduleDir = LuaScriptHost::GetModuleDirectory();
    std::error_code ec;
    std::filesystem::create_directories(moduleDir, ec);
    return moduleDir;
}
