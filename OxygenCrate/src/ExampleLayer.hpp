#pragma once
#include "../../external/Flux/Flux/Core/src/Layer.hpp"
#include "TextEditorPanel.hpp"
#include "LuaScriptHost.hpp"
#include "LuaConsoleWindow.hpp"
#include <imgui.h>
#include <string>

class ExampleLayer final : public Flux::Layer {
public:
    ExampleLayer();
    void OnRenderUI() override;

private:
    void CompileLuaScript();

    bool m_ShowDemo = true;
    TextEditorPanel m_TextEditorPanel;
    LuaScriptHost m_LuaHost;
    LuaConsoleWindow m_LuaConsole;
};
