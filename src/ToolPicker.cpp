#include "ToolPicker.hpp"
#include "imgui.h"
#include "imgui-SFML.h"
#include "Namespace.hpp"
#include "AppSettings.hpp"
#include "Const.hpp"
#include "Languages.hpp"
#include "Func.hpp"

using namespace sf;

ToolPicker::ToolPicker(const float& GUIScale, const bool& rulerEnabled, const Texture& toolIcons)
    : GUIScale(GUIScale), rulerEnabled(rulerEnabled), toolIcons(toolIcons)
{
}

Tool ToolPicker::getTool() const
{
    return currentTool;
}

void ToolPicker::setTool(const Tool currentTool, const bool userChange)
{
    this->currentTool = currentTool;
    this->userChange = userChange;
}

bool ToolPicker::wasUserChanged() const
{
    return toolHasChanged;
}

void ToolPicker::Draw()
{
    ImGui::SetNextWindowPos(Vector2f((11 + c_rulerSize * rulerEnabled) * GUIScale, (45 + c_rulerSize * rulerEnabled) * GUIScale + 50));
    ImGui::SetNextWindowSize(Vector2f(120, 310) * GUIScale);

    const ImVec4 t = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    const Vector2f p = ImGui::GetStyle().FramePadding;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, Vector2f(10, 10));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(t.x, t.y, t.z, 0.8f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Vector2f(3, 2));

    const std::array toolColors = {
        Color(83, 170, 17), Color(170, 163, 17), Color(17, 170, 138), Color(170, 135, 17),
        Color(17, 170, 87), Color(170, 75, 17), Color(170, 103, 17), Color(17, 65, 170),
        Color(113, 17, 170), Color(170, 17, 155), Color(170, 17, 36), Color(170, 17, 64)
    };

    toolHasChanged = false;

    if (userChange)
    {
        toolHasChanged = true;
        userChange = false;
    }

    if (ImGui::Begin("Tool Picker"_C, nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, Vector2f(2, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
        const float itemSizeX = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.f;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, Vector2f((itemSizeX - 35 * GUIScale) / 2.f, 5 * GUIScale));

        for (uint8_t i = 0; i < static_cast<uint8_t>(Tool::Count); i++)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 128));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 192));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 255));
            ImGui::PushStyleColor(ImGuiCol_Border, currentTool == static_cast<Tool>(i) ? Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 255) : Color::Transparent);

            const Vector2f topLeft = Vector2f(1 / 2.f * (i % 2), 1 / (static_cast<uint8_t>(Tool::Count) / 2.f) * (i / 2));
            const Vector2f bottomRight = Vector2f(topLeft.x + 1 / 2.f, topLeft.y + 1 / (static_cast<uint8_t>(Tool::Count) / 2.f));
            if (ImGui::ImageButton(("Tool" + to_string(i)).c_str(), toolIcons.getNativeHandle(), Vector2f(35 * GUIScale, 35 * GUIScale),
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
            ImGui::PopStyleVar(2);
            if (i % 2 == 0)
                ImGui::SameLine();
        }
        ImGui::PopStyleVar(3);
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}
