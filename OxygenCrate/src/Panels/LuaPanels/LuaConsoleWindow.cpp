#include "LuaConsoleWindow.hpp"

void LuaConsoleWindow::Render(LuaScriptHost& host)
{
    if (!m_IsVisible)
        return;

    ImGui::SetNextWindowSize(ImVec2(480.0f, 260.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Lua Console", &m_IsVisible))
    {
        if (ImGui::Button("Clear console"))
            host.ClearConsole();

        ImGui::Separator();
        const float footerHeight = ImGui::GetFrameHeightWithSpacing() * 2.0f;
        ImGui::BeginChild("LuaConsoleOutput", ImVec2(0.0f, -footerHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushTextWrapPos(0.0f);
        for (const std::string& line : host.GetConsoleLines())
            ImGui::TextUnformatted(line.c_str());
        ImGui::PopTextWrapPos();
        if (host.ShouldScrollConsole())
        {
            ImGui::SetScrollHereY(1.0f);
            host.AcknowledgeConsoleScroll();
        }
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::PushItemWidth(-100.0f);
        bool submit = ImGui::InputText("##LuaConsoleInput", m_InputBuffer.data(), m_InputBuffer.size(),
            ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        submit |= ImGui::Button("Send");

        if (submit)
        {
            std::string command = m_InputBuffer.data();
            if (!command.empty())
                host.ExecuteConsoleCommand(command);
            m_InputBuffer.fill(0);
            ImGui::SetKeyboardFocusHere(-1);
        }
    }
    ImGui::End();
}
