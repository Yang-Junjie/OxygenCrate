#pragma once
#include "../../external/Flux/Flux/Core/src/Layer.hpp"
#include "TextEditorPanel.hpp"
#include "SchedulePanel.hpp"
#include "LuaScriptHost.hpp"
#include "LuaConsoleWindow.hpp"
#include <imgui.h>
#include <string>

class ExampleLayer final : public Flux::Layer {
public:
    ExampleLayer();
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnRenderUI() override;

private:
    void CompileLuaScript();
    void RequestScriptCompile();
    void RenderControlPanel();
    void RenderLuaOutput();

    bool m_ShowDemo = true;
    bool m_PendingScriptCompile = false;
    TextEditorPanel m_TextEditorPanel;
    SchedulePanel m_SchedulePanel;
    LuaScriptHost m_LuaHost;
    LuaConsoleWindow m_LuaConsole;
};
