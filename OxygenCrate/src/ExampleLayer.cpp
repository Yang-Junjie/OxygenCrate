#include "ExampleLayer.hpp"

ExampleLayer::ExampleLayer() = default;

void ExampleLayer::OnAttach()
{
    m_TextEditorPanel.SetText(m_LuaHost.GetSampleScript());
    m_LuaConsole.Hide();
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
    if (m_PendingScriptCompile)
    {
        CompileLuaScript();
        m_PendingScriptCompile = false;
    }

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

    m_TextEditorPanel.Render();
    RenderControlPanel();
    m_LuaConsole.Render(m_LuaHost);
}

void ExampleLayer::RenderControlPanel()
{
    ImGui::Begin("Example Layer");
    ImGui::Text("Hello from ExampleLayer!");
    if (ImGui::Button("Toggle Demo"))
        m_ShowDemo = !m_ShowDemo;

    ImGui::Separator();
    ImGui::TextUnformatted("Lua integration");
    if (ImGui::Button("Run Lua Script"))
        RequestScriptCompile();
    ImGui::SameLine();
    if (ImGui::Button("Load sample script"))
        m_TextEditorPanel.SetText(m_LuaHost.GetSampleScript());
    ImGui::SameLine();
    const bool consoleVisible = m_LuaConsole.IsVisible();
    const char* consoleButtonLabel = consoleVisible ? "Hide console" : "Show console";
    if (ImGui::Button(consoleButtonLabel))
        m_LuaConsole.Toggle();

    const std::string& luaError = m_LuaHost.GetLastError();
    if (!luaError.empty())
    {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", luaError.c_str());
    }

    RenderLuaOutput();

    ImGui::End();
}

void ExampleLayer::RenderLuaOutput()
{
    ImGui::Separator();
    ImGui::TextUnformatted("Lua UI output");
    ImGui::PushID("LuaUIArea");
    if (m_LuaHost.HasDrawFunction())
        m_LuaHost.Draw();
    else
        ImGui::TextWrapped("No Lua UI is active. Load/enter a script and press \"Run Lua Script\".");
    ImGui::PopID();
}
