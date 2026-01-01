#pragma once
#include "../../external/Flux/Flux/Core/src/Layer.hpp"
#include "TextEditorPanel.hpp"
#include <imgui.h>

class ExampleLayer final : public Flux::Layer {
public:
    void OnRenderUI() override;

private:
    bool m_ShowDemo = true;
    TextEditorPanel m_TextEditorPanel;
};
