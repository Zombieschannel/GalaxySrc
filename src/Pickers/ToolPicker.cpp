#include "ToolPicker.hpp"
#include <imgui.h>
#include "../Namespace.hpp"
#include "../Config.hpp"
#include "../Const.hpp"
#include "../ZEditorsCommon/Languages.hpp"
#include "../Func.hpp"

using namespace sf;

glxy::ToolPicker::ToolPicker(const Texture& toolIcons)
    : toolIcons(toolIcons)
{
}

Tool glxy::ToolPicker::getTool() const
{
    return currentTool;
}

uint32_t glxy::ToolPicker::getToolsEnabled() const
{
    return toolsEnabled;
}

void glxy::ToolPicker::setTool(const Tool currentTool, const bool userChange)
{
    this->currentTool = currentTool;
    this->userChange = userChange;
}

void glxy::ToolPicker::setToolsEnabled(const uint32_t tools)
{
    this->toolsEnabled = tools;
    toolEnabledCount = 0;
    for (int8_t i = 0; i < static_cast<int8_t>(Tool::Count); i++)
        toolEnabledCount += toolsEnabled >> i & 1;
}

bool glxy::ToolPicker::wasUserChanged() const
{
    return toolHasChanged;
}

void glxy::ToolPicker::Draw()
{
    toolHasChanged = false;

    if (userChange)
    {
        toolHasChanged = true;
        userChange = false;
    }

    if (!windowOpen)
        return;

    ImGui::SetNextWindowPos(Vector2f((5 + c_rulerSize * config.showRuler) * config.GUIScale, (95 + c_rulerSize * config.showRuler) * config.GUIScale));

    const ImVec4 t = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    const Vector2f p = ImGui::GetStyle().FramePadding;
    constexpr int8_t columns = 3;
    constexpr int8_t textureColumns = 3;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(t.x, t.y, t.z, 0.8f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Vector2f(4, 4) * config.GUIScale);

    const array toolColors = {
        Color(83, 170, 17),
        Color(170, 163, 17),
        Color(170, 51, 17),
        Color(17, 170, 138),
        Color(170, 135, 17),
        Color(17, 65, 170),
        Color(17, 170, 87),
        Color(130, 170, 17),
        Color(170, 75, 17),
        Color(170, 103, 17),
        Color(113, 17, 170),
        Color(170, 17, 155),
        Color(170, 17, 36),
        Color(170, 17, 64),
        Color(139, 17, 170),
        Color(17, 137, 170),
        Color(55, 17, 170),
    };

    if (!ImGui::Begin("windowName[0]"_C, &windowOpen,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, Vector2f(2, 2) * config.GUIScale);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.f * config.GUIScale);

    uint8_t elementCount = 0;
    for (uint8_t i = 0; i < static_cast<uint8_t>(Tool::Count); i++)
    {
        if (!(toolsEnabled >> i & 1))
            continue;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, Vector2f(10 * config.GUIScale, 3 * config.GUIScale));

        ImGui::PushStyleColor(ImGuiCol_Button, Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 128));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 192));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 255));
        ImGui::PushStyleColor(ImGuiCol_Border, currentTool == static_cast<Tool>(i) ? Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 255) : Color::Transparent);

        const Vector2f topLeft = Vector2f(1.f / textureColumns * (i % textureColumns), 1 / std::ceil(static_cast<float>(Tool::Count) / textureColumns) * (i / textureColumns));
        const Vector2f bottomRight = Vector2f(topLeft.x + 1.f / textureColumns, topLeft.y + 1 / std::ceil(static_cast<float>(Tool::Count) / textureColumns));

        if (ImGui::ImageButton(("Tool" + to_string(i)).c_str(), toolIcons.getNativeHandle(), Vector2f(32 * config.GUIScale, 32 * config.GUIScale),
            topLeft, bottomRight))
        {
            if (currentTool != static_cast<Tool>(i))
                toolHasChanged = true;
            currentTool = static_cast<Tool>(i);
        }
        ImGui::PopStyleColor(4);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, p);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, p);
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", LL::ind("toolName[]", i).c_str());
            ImGui::EndTooltip();
        }
        ImGui::PopStyleVar(3);
        if (elementCount % columns != columns - 1)
            ImGui::SameLine();
        elementCount++;
    }
    ImGui::PopStyleVar(2);
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
