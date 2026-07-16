#include "LayerPicker.hpp"
#include "../Const.hpp"
#include "../Func.hpp"
#include "../PopUpState.hpp"
#include "../ZEditorsCommon/ZTB.hpp"
#include "../ZEditorsCommon/Languages.hpp"
#include <imgui.h>

LayerID LayerPicker::getLayerIDSelected(const EditorID arrayID) const
{
    return imageLayers.at(arrayID).layerIDSelected;
}

const LayerPicker::Layer& LayerPicker::getLayer(const EditorID arrayID, const LayerID layerID)
{
    return imageLayers.at(arrayID).layers.at(layerID);
}

void LayerPicker::createNewImage(const bool infinite)
{
    imageLayers.emplace_back(ImageLayer{infinite});
    imageLayers.back().layers.emplace_back();
    imageLayers.back().layers.back().name = "Layer 1";
    if (!infinite)
    {
        validate(imageLayers.back().layers.back().texture.loadFromImage(Image(Vector2u(c_layerPreviewTextureSize, c_layerPreviewTextureSize), Color::White)));
        imageLayers.back().layers.back().texture.setSmooth(true);
    }
}

void LayerPicker::deleteLayer(const EditorID editorID)
{
    auto& layers = imageLayers.at(editorID);
    layers.layers.erase(
        layers.layers.begin() + layers.layerIDSelected);
    if (layers.layerIDSelected > 0)
        layers.layerIDSelected--;
}

void LayerPicker::duplicateLayer(const EditorID editorID)
{
    auto& layers = imageLayers.at(editorID);
    layers.layers.emplace(layers.layers.begin() + layers.layerIDSelected + 1);
    const auto& oldLayer = layers.layers.at(layers.layerIDSelected);
    auto& newLayer = layers.layers.at(layers.layerIDSelected + 1);
    newLayer.name = oldLayer.name + " Copy";
    newLayer.texture = oldLayer.texture;
    layers.layerIDSelected++;
}

void LayerPicker::moveLayerUp(const EditorID editorID)
{
    auto& layers = imageLayers.at(editorID);
    std::swap(layers.layers.at(layers.layerIDSelected), layers.layers.at(layers.layerIDSelected + 1));
    layers.layerIDSelected++;
}

void LayerPicker::moveLayerDown(const EditorID editorID)
{
    auto& layers = imageLayers.at(editorID);
    std::swap(layers.layers.at(layers.layerIDSelected), layers.layers.at(layers.layerIDSelected - 1));
    layers.layerIDSelected--;
}

void LayerPicker::setLayerName(const EditorID editorID, const LayerID layerID, const string& name)
{
    imageLayers.at(editorID).layers.at(layerID).name = name;
}

void LayerPicker::setLayerEnabled(const EditorID editorID, const LayerID layerID, const bool state)
{
    imageLayers.at(editorID).layers.at(layerID).enabled = state;
}

void LayerPicker::setLayerBlendMode(const EditorID editorID, const LayerID layerID, const uint8_t blendMode)
{
    imageLayers.at(editorID).layers.at(layerID).blendMode = blendMode;
}

void LayerPicker::setLayerTransparency(const EditorID editorID, const LayerID layerID, const uint8_t transparency)
{
    imageLayers.at(editorID).layers.at(layerID).transparency = transparency;
}

void LayerPicker::updateLayerPreview(const Image& src, const EditorID editorID, const LayerID layerID)
{
    imageLayers.at(editorID).layers.at(layerID).texture.update(src);
}

void LayerPicker::deleteImage(const EditorID editorID)
{
    imageLayers.erase(imageLayers.begin() + editorID);
}

void LayerPicker::createNewLayer(const EditorID editorID)
{
    if (imageLayers.at(editorID).layers.size() >= c_maxLayers)
        return;
    auto& layers = imageLayers.at(editorID);
    layers.layers.emplace(layers.layers.begin() + layers.layerIDSelected + 1);
    layers.layers.at(layers.layerIDSelected + 1).name = "Layer " + to_string(layers.layers.size());
    validate(layers.layers.at(layers.layerIDSelected + 1).texture.loadFromImage(
        Image(Vector2u(c_layerPreviewTextureSize, c_layerPreviewTextureSize), Color::Transparent)));
    layers.layers.at(layers.layerIDSelected + 1).texture.setSmooth(true);
    layers.layerIDSelected++;
}

LayerPicker::Return LayerPicker::Draw(const Window& window, const float& GUIScale, const Texture& layerIcons,
    vector<PopUpState>& popUpState, const EditorID editorID)
{
    if (!windowOpen)
        return Return::None;

    Return result = Return::None;

    static LayerID lastHoveredLayer = -1;
    const LayerID hoveredLayer = lastHoveredLayer;
    lastHoveredLayer = -1;

    static LayerID lastClickedLayer = -1;
    const LayerID clickedLayer = lastClickedLayer;
    lastClickedLayer = -1;

    const Vector2f windowSize = Vector2f(200.f, 250.f) * GUIScale;
    ImGui::SetNextWindowPos(Vector2f(window.getSize().x - 20 * GUIScale - windowSize.x, window.getSize().y - 40 * GUIScale - windowSize.y));
    ImGui::SetNextWindowSize(windowSize);

    const Color32f t = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Color32f(t.r, t.g, t.b, 0.8f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Vector2f(5, 5) * GUIScale);

    if (!ImGui::Begin("windowName[2]"_C, &windowOpen,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        return result;
    }

    if (ImGui::BeginChild("Scroll", Vector2f(ImGui::GetContentRegionAvail().x, -25 * GUIScale - ImGui::GetStyle().ItemSpacing.y)) && editorID >= 0)
    {
        const Color layerActive = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
        const Color layerClicked = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
        const Color layerHovered = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        const Color layerOther = Color(32, 32, 32);
        LayerID j = 0;
        auto& layers = imageLayers.at(editorID);
        for (LayerID i = layers.layers.size() - 1; i >= 0; i--)
        {
            if (clickedLayer == i)
                ImGui::PushStyleColor(ImGuiCol_ChildBg, layerClicked);
            else if (hoveredLayer == i)
                ImGui::PushStyleColor(ImGuiCol_ChildBg, layerActive);
            else if (layers.layerIDSelected == i)
                ImGui::PushStyleColor(ImGuiCol_ChildBg, layerHovered);
            else
                ImGui::PushStyleColor(ImGuiCol_ChildBg, layerOther);

            const int32_t layerBoxSize = layers.infinite ? 40 : 60;

            if (ImGui::BeginChild(("LayerID" + to_string(j++)).c_str(), Vector2f(ImGui::GetContentRegionAvail().x, layerBoxSize * GUIScale), ImGuiChildFlags_AlwaysUseWindowPadding))
            {
                const float width = ImGui::GetContentRegionAvail().x;
                if (!layers.infinite)
                {
                    ImGui::Image(layers.layers.at(i).texture.getNativeHandle(),
                        Vector2f(ImGui::GetContentRegionAvail().y - 2, ImGui::GetContentRegionAvail().y - 2),
                        Vector2f(0, 0), Vector2f(1, 1), Color::White, Color(128, 128, 128));
                }

                ImGui::SetCursorPos(Vector2f(layerBoxSize * GUIScale, layerBoxSize * GUIScale / 3));

                if (layers.layers.at(i).name.size() > 10)
                    ImGui::Text("%s...", layers.layers.at(i).name.substr(0, 7).c_str());
                else
                    ImGui::Text("%s", layers.layers.at(i).name.c_str());
                ImGui::SameLine();

                ImGui::SetCursorPos(Vector2f(width - (layerBoxSize / 2) * GUIScale, layerBoxSize * GUIScale / 2 - 24 * GUIScale / 2));

                if (ImGui::Checkbox(("##LayerEnabled" + to_string(j)).c_str(), &layers.layers.at(i).enabled))
                    result = Return::Visibility;

                if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
                    lastHoveredLayer = i;
                if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
                        (InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isTouchPressed(0)))
                    lastClickedLayer = i;
                if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
                        (InputEvent::isButtonReleased(Mouse::Button::Left) || InputEvent::isTouchReleased(0)))
                    layers.layerIDSelected = i;
                if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)
                || InputEvent::isButtonReleased(Mouse::Button::Right)))
                    popUpState.push_back(PopUpState::LayerProperties);
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    }
    ImGui::EndChild();
    const uint8_t count = 7;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, Vector2f(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, Vector2f((ImGui::GetContentRegionAvail().x - 25 * GUIScale) / (count - 1) - 25 * GUIScale, 0));
    const array toolColors = {
        Color(17, 170, 33), Color(170, 17, 36), Color(170, 163, 17),
        Color(17, 137, 170), Color(17, 170, 138), Color(17, 170, 87),
        Color(170, 75, 17),
    };
    for (uint8_t i = 0; i < count; i++)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 128));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 192));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 255));

        const int16_t layerCount = editorID == -1 ? 0 : imageLayers.at(editorID).layers.size();
        const LayerID layerSelected = editorID == -1 ? 0 : imageLayers.at(editorID).layerIDSelected;

        ImGui::BeginDisabled(editorID == -1 ||
            (i == 0 && layerCount == c_maxLayers) ||
            (i == 1 && layerCount == 1 ||
            i == 3 && layerSelected >= layerCount - 1 ||
            (i == 4 || i == 5) && layerSelected <= 0));
        if (ImGui::ImageButton(("LayerButton" + to_string(i)).c_str(), layerIcons.getNativeHandle(), Vector2f(25, 25) * GUIScale,
            Vector2f(1.f / count * i, 0), Vector2f(1.f / count * (i + 1), 1)))
        {
            switch (i)
            {
            case 0: result = Return::AddLayer; break;
            case 1: result = Return::DeleteLayer; break;
            case 2: result = Return::DuplicateLayer; break;
            case 3: result = Return::MoveLayerUp; break;
            case 4: result = Return::MoveLayerDown; break;
            case 5: result = Return::MergeLayerDown; break;
            case 6: popUpState.push_back(PopUpState::LayerProperties); break;
            default: break;
            }
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", LL::ind("layerToolTip[]", i).c_str());
            ImGui::EndTooltip();
        }
        ImGui::EndDisabled();
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
    }
    ImGui::PopStyleVar(2);
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    return result;
}
