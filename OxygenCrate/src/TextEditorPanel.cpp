#include "TextEditorPanel.hpp"
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
    InitializeTextEditor();
}

void TextEditorPanel::InitializeTextEditor()
{
    auto lang = TextEditor::LanguageDefinition::CPlusPlus();

    static const char* ppnames[] = { "NULL", "PM_REMOVE", "ZeroMemory", "DXGI_SWAP_EFFECT_DISCARD",
        "D3D_FEATURE_LEVEL", "D3D_DRIVER_TYPE_HARDWARE", "WINAPI", "D3D11_SDK_VERSION", "assert" };
    static const char* ppvalues[] = {
        "#define NULL ((void*)0)",
        "#define PM_REMOVE (0x0001)",
        "Microsoft's own memory zapper function\n(which is a macro actually)\nvoid ZeroMemory(\n\t[in] PVOID  Destination,\n\t[in] SIZE_T Length\n); ",
        "enum DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_DISCARD = 0",
        "enum D3D_FEATURE_LEVEL",
        "enum D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE  = ( D3D_DRIVER_TYPE_UNKNOWN + 1 )",
        "#define WINAPI __stdcall",
        "#define D3D11_SDK_VERSION (7)",
        " #define assert(expression) (void)(                                                  \n"
        "    (!!(expression)) ||                                                              \n"
        "    (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \n"
        " )" };

    for (int i = 0; i < static_cast<int>(std::size(ppnames)); ++i)
    {
        TextEditor::Identifier id;
        id.mDeclaration = ppvalues[i];
        lang.mPreprocIdentifiers.insert(std::make_pair(std::string(ppnames[i]), id));
    }

    static const char* identifiers[] = {
        "HWND", "HRESULT", "LPRESULT", "D3D11_RENDER_TARGET_VIEW_DESC", "DXGI_SWAP_CHAIN_DESC", "MSG", "LRESULT",
        "WPARAM", "LPARAM", "UINT", "LPVOID", "ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D10Blob",
        "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView",
        "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState", "IDXGISwapChain", "ID3D11RenderTargetView",
        "ID3D11Texture2D", "TextEditor" };
    static const char* idecls[] = {
        "typedef HWND_* HWND", "typedef long HRESULT", "typedef long* LPRESULT", "struct D3D11_RENDER_TARGET_VIEW_DESC",
        "struct DXGI_SWAP_CHAIN_DESC", "typedef tagMSG MSG\n * Message structure", "typedef LONG_PTR LRESULT",
        "WPARAM", "LPARAM", "UINT", "LPVOID", "ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D10Blob",
        "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView",
        "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState", "IDXGISwapChain", "ID3D11RenderTargetView",
        "ID3D11Texture2D", "class TextEditor" };

    for (int i = 0; i < static_cast<int>(std::size(identifiers)); ++i)
    {
        TextEditor::Identifier id;
        id.mDeclaration = std::string(idecls[i]);
        lang.mIdentifiers.insert(std::make_pair(std::string(identifiers[i]), id));
    }

    m_TextEditor.SetLanguageDefinition(lang);
    m_TextEditor.SetPalette(TextEditor::GetDarkPalette());

    TextEditor::ErrorMarkers markers;
    markers.insert(std::make_pair<int, std::string>(6, "Example error here:\nInclude file not found: \"TextEditor.h\""));
    markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
    m_TextEditor.SetErrorMarkers(markers);

    m_StatusMessage = "No file loaded";
    m_FileToEdit.clear();
}

void TextEditorPanel::Render()
{
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Text Editor Demo", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar))
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
#ifdef __ANDROID__
    std::filesystem::path base("/storage/emulated/0/Beisent/OxygenCrate");
#else
    std::filesystem::path base;
    if (const char* userProfile = std::getenv("USERPROFILE"))
        base = std::filesystem::path(userProfile) / "Documents" / "OxygenCrate";
    else if (const char* home = std::getenv("HOME"))
        base = std::filesystem::path(home) / "Documents" / "OxygenCrate";
    else
        base = std::filesystem::current_path() / "OxygenCrateFiles";
#endif
    std::error_code ec;
    std::filesystem::create_directories(base, ec);
    return base;
}
