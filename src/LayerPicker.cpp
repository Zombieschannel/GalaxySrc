#include "LayerPicker.hpp"
#include "Const.hpp"
#include "Func.hpp"
#include "PopUpState.hpp"
#include "inc/ZTB.hpp"

LayerPicker::LayerPicker(const Window& window, const float& GUIScale, const int16_t& activeImageEditor, const Texture& layerIcons)
    : window(window), GUIScale(GUIScale), activeImageEditor(activeImageEditor), layerIcons(layerIcons)
{

}

int16_t LayerPicker::getLayerIDSelected() const
{
    return imageLayers.at(activeImageEditor).layerIDSelected;
}

int16_t LayerPicker::getLayerCount() const
{
    return imageLayers.at(activeImageEditor).layers.size();
}

LayerPicker::Layer& LayerPicker::getLayer()
{
    return imageLayers.at(activeImageEditor).layers.at(imageLayers.at(activeImageEditor).layerIDSelected);
}

uint8_t LayerPicker::getLayerBlendMode(const int16_t layerID) const
{
    return imageLayers.at(activeImageEditor).layers.at(layerID).blendMode;
}

uint8_t LayerPicker::getLayerTransparency(const int16_t layerID) const
{
    return imageLayers.at(activeImageEditor).layers.at(layerID).transparency;
}

bool LayerPicker::isLayerEnabled(const int16_t layerID) const
{
    return imageLayers.at(activeImageEditor).layers.at(layerID).enabled;
}

void LayerPicker::createNewImage()
{
    imageLayers.emplace_back();
    imageLayers.back().layers.emplace_back();
    imageLayers.back().layers.back().name = "Layer 1";
    validate(imageLayers.back().layers.back().texture.loadFromImage(Image(Vector2u(c_layerPreviewTextureSize, c_layerPreviewTextureSize), Color::White)));
    imageLayers.back().layers.back().texture.setSmooth(true);
    imageLayers.back().layers.back().enabled = true;
}

void LayerPicker::deleteLayer()
{
    auto& layers = imageLayers.at(activeImageEditor);
    layers.layers.erase(
        layers.layers.begin() + layers.layerIDSelected);
    if (layers.layerIDSelected > 0)
        layers.layerIDSelected--;
}

void LayerPicker::duplicateLayer()
{
    auto& layers = imageLayers.at(activeImageEditor);
    layers.layers.emplace(layers.layers.begin() + layers.layerIDSelected + 1);
    const auto& oldLayer = layers.layers.at(layers.layerIDSelected);
    auto& newLayer = layers.layers.at(layers.layerIDSelected + 1);
    newLayer.name = oldLayer.name + " Copy";
    newLayer.enabled = oldLayer.enabled;
    newLayer.transparency = oldLayer.transparency;
    newLayer.blendMode = oldLayer.blendMode;
    newLayer.texture = oldLayer.texture;
    layers.layerIDSelected++;
}

void LayerPicker::moveLayerUp()
{
    auto& layers = imageLayers.at(activeImageEditor);
    std::swap(layers.layers.at(layers.layerIDSelected), layers.layers.at(layers.layerIDSelected + 1));
    layers.layerIDSelected++;
}

void LayerPicker::moveLayerDown()
{
    auto& layers = imageLayers.at(activeImageEditor);
    std::swap(layers.layers.at(layers.layerIDSelected), layers.layers.at(layers.layerIDSelected - 1));
    layers.layerIDSelected--;
}

void LayerPicker::updateLayerPreview(const Image& src, const int16_t layerID)
{
    imageLayers.at(activeImageEditor).layers.at(layerID).texture.update(src);
}

void LayerPicker::deleteImage(const uint32_t index)
{
    imageLayers.erase(imageLayers.begin() + index);
}

void LayerPicker::createNewLayer()
{
    if (imageLayers.at(activeImageEditor).layers.size() >= c_maxLayers)
        return;
    auto& layers = imageLayers.at(activeImageEditor);
    layers.layers.emplace(layers.layers.begin() + layers.layerIDSelected + 1);
    layers.layers.at(layers.layerIDSelected + 1).name = "Layer " + to_string(layers.layers.size());
    validate(layers.layers.at(layers.layerIDSelected + 1).texture.loadFromImage(
        Image(Vector2u(c_layerPreviewTextureSize, c_layerPreviewTextureSize), Color::Transparent)));
    layers.layers.at(layers.layerIDSelected + 1).texture.setSmooth(true);
    layers.layerIDSelected++;
}

LayerPicker::Return LayerPicker::Draw(vector<PopUpState>& popUpState)
{
    Return result = Return::None;

    static int16_t lastHoveredLayer = -1;
    const int16_t hoveredLayer = lastHoveredLayer;
    lastHoveredLayer = -1;

    static int16_t lastClickedLayer = -1;
    const int16_t clickedLayer = lastClickedLayer;
    lastClickedLayer = -1;

    const Vector2f windowSize = Vector2f(200.f, 250.f) * GUIScale;
    ImGui::SetNextWindowPos(Vector2f(window.getSize().x - 25 * GUIScale - windowSize.x, window.getSize().y - 40 * GUIScale - windowSize.y));
    ImGui::SetNextWindowSize(windowSize);

    const ImVec4 t = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(t.x, t.y, t.z, 0.8f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Vector2f(5, 5));
    if (ImGui::Begin("Layers"_C, nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        if (ImGui::BeginChild("Scroll", Vector2f(ImGui::GetContentRegionAvail().x, -25 * GUIScale - ImGui::GetStyle().ItemSpacing.y)) && activeImageEditor >= 0)
        {
            const Color layerActive = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
            const Color layerClicked = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
            const Color layerHovered = ImGui::GetStyleColorVec4(ImGuiCol_Button);
            const Color layerOther = Color(32, 32, 32);
            int16_t j = 0;
            auto& layers = imageLayers.at(activeImageEditor);
            for (int16_t i = layers.layers.size() - 1; i >= 0; i--)
            {
                if (clickedLayer == i)
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, layerClicked);
                else if (hoveredLayer == i)
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, layerActive);
                else if (layers.layerIDSelected == i)
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, layerHovered);
                else
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, layerOther);

                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Vector2f(5, 5));
                if (ImGui::BeginChild(("LayerID" + to_string(j++)).c_str(), Vector2f(ImGui::GetContentRegionAvail().x, 60 * GUIScale), ImGuiChildFlags_AlwaysUseWindowPadding))
                {
                    const float width = ImGui::GetContentRegionAvail().x;
                    ImGui::Image(layers.layers.at(i).texture.getNativeHandle(),
                        Vector2f(ImGui::GetContentRegionAvail().y - 2, ImGui::GetContentRegionAvail().y - 2),
                        Vector2f(0, 0), Vector2f(1, 1), Color::White, Color(128, 128, 128));
                    ImGui::SetCursorPos(Vector2f(60 * GUIScale + 10, 60 * GUIScale / 3));
                    if (layers.layers.at(i).name.size() > 10)
                        ImGui::Text("%s...", layers.layers.at(i).name.substr(0, 7).c_str());
                    else
                        ImGui::Text("%s", layers.layers.at(i).name.c_str());
                    ImGui::SameLine();
                    ImGui::SetCursorPos(Vector2f(width - 30 * GUIScale, 60 * GUIScale / 2 - 24 * GUIScale / 2));
                    if (ImGui::Checkbox(("##LayerEnabled" + to_string(j)).c_str(), &layers.layers.at(i).enabled))
                        result = Return::Visibility;

                    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
                        lastHoveredLayer = i;
                    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && InputEvent::isButtonPressed(Mouse::Button::Left))
                        lastClickedLayer = i;
                    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && InputEvent::isButtonReleased(Mouse::Button::Left))
                        layers.layerIDSelected = i;
                    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) || InputEvent::isButtonReleased(Mouse::Button::Right)))
                        popUpState.push_back(PopUpState::LayerProperties);
                }
                ImGui::EndChild();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
            }
        }
        ImGui::EndChild();
        const uint8_t count = 7;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, Vector2f(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, Vector2f((ImGui::GetContentRegionAvail().x - 25 * GUIScale) / (count - 1) - 25 * GUIScale, 0));
        const std::array toolColors = {
            Color(83, 170, 17), Color(170, 17, 36), Color(170, 163, 17),
            Color(17, 170, 87), Color(17, 170, 138), Color(17, 65, 170),
            Color(170, 75, 17),
        };
        for (uint8_t i = 0; i < count; i++)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 128));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 192));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Color(toolColors.at(i).r, toolColors.at(i).g, toolColors.at(i).b, 255));

            ImGui::BeginDisabled(activeImageEditor == -1 ||
                (i == 0 && getLayerCount() == c_maxLayers) ||
                (i == 1 && getLayerCount() == 1 ||
                i == 3 && getLayerIDSelected() >= getLayerCount() - 1 ||
                (i == 4 || i == 5) && getLayerIDSelected() <= 0));
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
    }
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    return result;
}
