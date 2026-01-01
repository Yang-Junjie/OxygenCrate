#include "ExampleLayer.hpp"
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <iterator>

namespace
{
    std::filesystem::path ResolveRepoRelativePath(const std::filesystem::path& relativePath)
    {
        auto current = std::filesystem::current_path();
        for (int i = 0; i < 6; ++i)
        {
            const auto candidate = current / relativePath;
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec) && !ec)
                return candidate;
            if (!current.has_parent_path())
                break;
            current = current.parent_path();
        }
        return {};
    }

    std::string ReadFileToString(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (!file.good())
            return {};

        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
}

ExampleLayer::ExampleLayer()
{
    InitializeTextEditor();
}

void ExampleLayer::InitializeTextEditor()
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

void ExampleLayer::OnRenderUI()
{
    if (m_ShowDemo)
        ImGui::ShowDemoWindow(&m_ShowDemo);

    ShowTextEditorWindow();

    ImGui::Begin("Example Layer");
    ImGui::Text("Hello from ExampleLayer!");
    if (ImGui::Button("Toggle Demo"))
        m_ShowDemo = !m_ShowDemo;
    ImGui::End();
}

void ExampleLayer::ShowTextEditorWindow()
{
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Text Editor Demo", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar))
    {
        const auto cpos = m_TextEditor.GetCursorPosition();

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
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
}

void ExampleLayer::TrySaveFile()
{
    if (m_FileToEdit.empty())
    {
        m_StatusMessage = "No file selected";
        return;
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
