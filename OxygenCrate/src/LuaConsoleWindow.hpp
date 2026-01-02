#pragma once

#include "LuaScriptHost.hpp"
#include <imgui.h>
#include <array>
#include <cstring>

class LuaConsoleWindow {
public:
    bool IsVisible() const { return m_IsVisible; }
    void Toggle() { m_IsVisible = !m_IsVisible; }
    void Show() { m_IsVisible = true; }
    void Hide() { m_IsVisible = false; }

    void Render(LuaScriptHost& host);

private:
    bool m_IsVisible = false;
    std::array<char, 256> m_InputBuffer{};
};
