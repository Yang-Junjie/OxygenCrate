#pragma once

#include "../../external/ImGuiTextEditor/TextEditor.h"
#include <imgui.h>
#include <array>
#include <filesystem>
#include <string>

class TextEditorPanel
{
public:
    TextEditorPanel();

    void Render();

private:
    void InitializeTextEditor();
    void TrySaveFile();
    void BeginNewFile();
    void CreateNewFileAtPath(const std::string& path);
    void OpenFileAtPath(const std::string& path);
    void DrawOpenFilePopup();
    void DrawNewFilePopup();
    std::filesystem::path GetDefaultDirectory() const;

private:
    TextEditor m_TextEditor;
    std::string m_FileToEdit;
    std::string m_StatusMessage;
    std::array<char, 512> m_OpenPathBuffer{};
    std::array<char, 256> m_NewFilePathBuffer{};
    bool m_OpenPopupRequested = false;
    bool m_NewPopupRequested = false;
    bool m_CloseOpenPopup = false;
};
