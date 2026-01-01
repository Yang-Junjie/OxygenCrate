#pragma once
#include "../../external/Flux/Flux/Core/src/Layer.hpp"
#include "../../external/ImGuiTextEditor/TextEditor.h"
#include <imgui.h>
#include <string>

class ExampleLayer final : public Flux::Layer {
public:
    ExampleLayer();
    void OnRenderUI() override;

private:
    void InitializeTextEditor();
    void ShowTextEditorWindow();
    void TrySaveFile();

private:
    bool m_ShowDemo = true;
    TextEditor m_TextEditor;
    std::string m_FileToEdit;
    std::string m_StatusMessage;
};
