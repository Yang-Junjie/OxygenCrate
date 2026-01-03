#include "ExampleLayer.hpp"

ExampleLayer::ExampleLayer() = default;

void ExampleLayer::OnAttach()
{
    ApplyPanelPreferences(m_SettingsPanel.GetPreferences());
    m_PendingScriptCompile = false;
}

void ExampleLayer::OnDetach()
{
    m_LuaConsole.Hide();
    m_LuaHost.ClearConsole();
    m_PendingScriptCompile = false;
}

void ExampleLayer::OnUpdate(float dt)
{
    SettingPanel::PanelPreferences pendingPrefs;
    if (m_SettingsPanel.ConsumePendingPreferences(pendingPrefs))
        ApplyPanelPreferences(pendingPrefs);

    if (m_PendingScriptCompile)
    {
        CompileLuaScript();
        m_PendingScriptCompile = false;
    }

    m_LuaHost.Render(dt);
    m_LuaHost.Update(dt);
}

void ExampleLayer::CompileLuaScript()
{
    const std::string script = m_TextEditorPanel.GetText();
    m_LuaHost.CompileScript(script);
}

void ExampleLayer::RequestScriptCompile()
{
    m_PendingScriptCompile = true;
}

void ExampleLayer::OnRenderUI()
{
    if (m_ShowDemo)
        ImGui::ShowDemoWindow(&m_ShowDemo);

    if (m_ShowTextEditorPanel)
        m_TextEditorPanel.Render();
    RenderControlPanel();
    if (m_ShowSettingsPanel)
        m_SettingsPanel.Render();
    if (m_ShowSchedulePanel)
        m_SchedulePanel.Render();
    m_LuaConsole.Render(m_LuaHost);
}

void ExampleLayer::RenderControlPanel()
{
    ImGui::Begin("Example Layer");
    ImGui::TextWrapped("Quickly toggle panels and manage Lua scripting utilities from this hub.");

    ImGui::SeparatorText("Panels");
    ImGui::Checkbox("Demo window", &m_ShowDemo);
    ImGui::Checkbox("Text editor", &m_ShowTextEditorPanel);
    ImGui::Checkbox("Schedule planner", &m_ShowSchedulePanel);
    ImGui::Checkbox("Settings panel", &m_ShowSettingsPanel);
    bool consoleVisible = m_LuaConsole.IsVisible();
    if (ImGui::Checkbox("Lua console", &consoleVisible))
    {
        if (consoleVisible)
            m_LuaConsole.Show();
        else
            m_LuaConsole.Hide();
    }

    ImGui::SeparatorText("Lua integration");
    if (ImGui::Button("Run script"))
        RequestScriptCompile();
    ImGui::SameLine();
    if (ImGui::Button("Load sample"))
        m_TextEditorPanel.SetText(m_LuaHost.GetSampleScript());

    const std::string& luaError = m_LuaHost.GetLastError();
    if (!luaError.empty())
    {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", luaError.c_str());
    }

    RenderLuaOutput();

    ImGui::End();
}

void ExampleLayer::RenderLuaOutput()
{
    
    ImGui::TextUnformatted("Lua UI output");
    ImGui::PushID("LuaUIArea");
    if (m_LuaHost.HasDrawFunction())
        m_LuaHost.Draw();
    else
        ImGui::TextWrapped("No Lua UI is active. Load/enter a script and press \"Run Lua Script\".");
    ImGui::PopID();
    ImGui::Separator();
}

void ExampleLayer::ApplyPanelPreferences(const SettingPanel::PanelPreferences& prefs)
{
    m_ShowDemo = prefs.ShowDemoWindow;
    m_ShowTextEditorPanel = prefs.ShowTextEditor;
    m_ShowSchedulePanel = prefs.ShowSchedule;
    m_ShowSettingsPanel = prefs.ShowSettingsPanel;
    if (prefs.ShowLuaConsole)
        m_LuaConsole.Show();
    else
        m_LuaConsole.Hide();
}
