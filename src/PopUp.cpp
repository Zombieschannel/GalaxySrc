#include "App.hpp"
#include "Func.hpp"
#include "Global.hpp"
#include "Shortcuts.hpp"
#include <SFML/OpenGL.hpp>
#include "Themes.hpp"

#define BUILDNUMBER 2261
void glxy::App::PopUp()
{
    if (popUpState.empty())
        return;
    static bool keyboardFocusHere = false;
    static Vector2i imageNewSize;
    static int32_t scalePercentage;
    static Vector2i canvasResizePoint;
    static Vector2i resolution;
    static Vector2f transformImagePosition = Vector2f();
    static Vector2f transformImageOrigin = Vector2f();
    static Vector2f transformImageScale = Vector2f(1, 1);
    static float transformImageRotation = 0;
    static bool transformImageTile = false;
    bool reRender = false;
    const array popUpSize = {
        Vector2f(550, 450), //Settings
        Vector2f(630, 360), //About
        Vector2f(690, 540), //Changelog
        Vector2f(690, 540), //FuturePlan
        Vector2f(440, 100), //ThreadWork
        Vector2f(530, 350), //GLScan
        Vector2f(440, 240), //New
        Vector2f(500, 200), //Save
        Vector2f(550, 400), //Open
        Vector2f(500, 200), //LayerProperties
        Vector2f(500, 300), //Resize
        Vector2f(500, 275), //ResizeCanvas
        Vector2f(350, 425), //Setup
        Vector2f(500, 240), //TransformImage
        Vector2f(300, 150), //SaveBeforeExit
        Vector2f(300, 130), //SaveBeforeClose
    };
    if (!ImGui::IsPopupOpen(LL::ind("PopUpTitle[]", static_cast<int8_t>(popUpState.back())).c_str()))
    {
        switch (popUpState.back())
        {
        case PopUpState::TransformImage:
            reRender = true;
            if (_imageEditor.at(activeImageEditor)->chunkManager.hasSelectionLayer())
            {
                assert(!_imageEditor.at(activeImageEditor)->pixelSelect.getFinalSelectionBounds().expired());
                const IntRect final = *_imageEditor.at(activeImageEditor)->pixelSelect.getFinalSelectionBounds().lock();
                _imageEditor.at(activeImageEditor)->transformImageOriginalPosition = final.position;
            }
            else
                _imageEditor.at(activeImageEditor)->transformImageOriginalPosition = Vector2i();
            break;
        case PopUpState::Resize:
            imageNewSize = Vector2i(_imageEditor.at(activeImageEditor)->getSize());
            scalePercentage = 100;
            break;
        case PopUpState::ResizeCanvas:
            imageNewSize = Vector2i(_imageEditor.at(activeImageEditor)->getSize());
            scalePercentage = 100;
            canvasResizePoint = Vector2i(1, 1);
            break;
        case PopUpState::New:
            if (clipboardImage)
                resolution = Vector2i(clipboardImage->getSize());
            else
                resolution = Vector2i(800, 600);
            break;
        case PopUpState::Open:
            keyboardFocusHere = true;
            break;
        }
        const Vector2f size = popUpSize.at(static_cast<int8_t>(popUpState.back())) * settings.GUIScale;
        ImGui::SetNextWindowSize(size);
        ImGui::SetWindowPos(LL::ind("PopUpTitle[]", static_cast<int8_t>(popUpState.back())).c_str(),
            Vector2f(window.getSize().x / 2 - size.x / 2, window.getSize().y / 2 - size.y / 2), ImGuiCond_Always);
        ImGui::OpenPopup(LL::ind("PopUpTitle[]", static_cast<int8_t>(popUpState.back())).c_str());
    }
    if (ImGui::BeginPopupModal(LL::ind("PopUpTitle[]", static_cast<int8_t>(popUpState.back())).c_str(), nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings))
    {
        switch (popUpState.back())
        {
        case PopUpState::SaveBeforeClose:
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * settings.GUIScale)))
            {
                ImGui::Text("%s", "glxyWarning2"_C);
                ImGui::BulletText("%s", _imageEditor.at(editorCloseAttempt)->imageName.c_str());
            }
            ImGui::EndChild();
            if (ImGui::Button("Save"_C, Vector2f(ImGui::GetContentRegionAvail().x / 3, ImGui::GetContentRegionAvail().y)))
            {
                popUpState.pop_back();
                if (SaveImage())
                    DeleteEditor(editorCloseAttempt);
            }
            ImGui::SameLine();
            if (ImGui::Button("Don't save"_C, Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)))
            {
                DeleteEditor(editorCloseAttempt);
                editorCloseAttempt = -1;
                popUpState.pop_back();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            {
                editorCloseAttempt = -1;
                popUpState.pop_back();
            }
            break;
        case PopUpState::SaveBeforeExit:
            ImGui::Text("%s:", "Unsaved files"_C);
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * settings.GUIScale)))
            {
                for (auto& n : _imageEditor)
                {
                    if (n->unsavedChanges)
                        ImGui::BulletText("%s", n->imageName.c_str());
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("Don't save"_C, Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)))
            {
                window.close();
                popUpState.pop_back();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
            }
            break;
        case PopUpState::TransformImage:
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * settings.GUIScale)))
            {
                if (ImGui::SliderFloat2("Origin"_C, &transformImageOrigin.x, -1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                    reRender = true;
                if (ImGui::SliderFloat2("Position"_C, &transformImagePosition.x, -1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                    reRender = true;
                if (ImGui::SliderFloat("Rotation"_C, &transformImageRotation, -180, 180, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                    reRender = true;
                if (ImGui::SliderFloat2("Scale"_C, &transformImageScale.x, 0.001f, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
                    reRender = true;
                ImGui::BeginDisabled(_imageEditor.at(activeImageEditor)->chunkManager.hasSelectionLayer());
                if (ImGui::Checkbox("Tileable"_C, &transformImageTile))
                    reRender = true;
                ImGui::EndDisabled();
                if (reRender)
                {
                    ImageEditor* editor = _imageEditor.at(activeImageEditor).get();
                    const Vector2f posOffset = Vector2f(editor->tempImage->getSize()) / 2.f + Vector2f(editor->transformImageOriginalPosition);
                    unique_ptr<Transformable>& tran = editor->transformImageTransform;
                    if (!tran)
                        tran = make_unique<Transformable>();
                    tran->setPosition(Vector2f(transformImagePosition.x * editor->getSize().x, transformImagePosition.y * editor->getSize().y) + posOffset);
                    tran->setScale(transformImageScale);
                    tran->setOrigin(Vector2f(transformImageOrigin.x * editor->tempImage->getSize().x, transformImageOrigin.y * editor->tempImage->getSize().y) +
                        Vector2f(editor->tempImage->getSize()) / 2.f);
                    tran->setRotation(degrees(transformImageRotation));

                    editor->chunkManager.createTempLayer();
                    editor->chunkManager.clearTempLayer(Color::Transparent);
                    if (editor->chunkManager.hasSelectionLayer())
                        editor->chunkManager.clearSelection();

                    const IntRect rect = editor->TransformImage(*tran, transformImageTile);
                    if (editor->chunkManager.hasSelectionLayer())
                    {
                        if (editor->transformImageSelectionArea != IntRect())
                            editor->pixelSelect.UpdateTexture(getUnion(&editor->transformImageSelectionArea, &rect));
                        else
                            editor->pixelSelect.UpdateTexture(rect);
                    }
                    editor->transformImageSelectionArea = rect;
                    editor->chunkManager.RenderChunkArea();
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                _imageEditor.at(activeImageEditor)->OptionFinish();
                popUpState.pop_back();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            {
                _imageEditor.at(activeImageEditor)->OptionCancel();
                popUpState.pop_back();
            }
            break;
        case PopUpState::Resize:
        {
            static bool byPercentage = false;
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * settings.GUIScale)))
            {
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
                if (ImGui::BeginCombo("Resampling method"_C, LL::ind("resamplingMethod[]", settings.resamplingMethod).c_str()))
                {
                    for (uint8_t i = 0; i < c_resamplingMethodCnt; i++)
                        if (ImGui::Selectable(LL::ind("resamplingMethod[]", i).c_str()))
                        {
                            settings.resamplingMethod = i;
                        }
                    ImGui::EndCombo();
                }
                if (ImGui::RadioButton("By percentage"_C, byPercentage))
                    byPercentage = true;

                ImGui::Indent();
                ImGui::BeginDisabled(!byPercentage);
                if (ImGui::InputInt("Percentage"_C, &scalePercentage, 1, 10))
                {
                    scalePercentage = std::clamp(scalePercentage, 1, 10000);
                    imageNewSize = Vector2i(_imageEditor.at(activeImageEditor)->getSize()) * scalePercentage / 100;
                    imageNewSize.x = max(imageNewSize.x, 1);
                    imageNewSize.y = max(imageNewSize.y, 1);
                }
                ImGui::EndDisabled();
                ImGui::Unindent();

                if (ImGui::RadioButton("By absolute size"_C, !byPercentage))
                    byPercentage = false;

                ImGui::Indent();
                ImGui::BeginDisabled(byPercentage);
                ImGui::Checkbox("Maintain aspect ratio"_C, &settings.maintainAspectResize);
                if (ImGui::InputInt("Width"_C, &imageNewSize.x, 1, 10))
                {
                    imageNewSize.x = max(imageNewSize.x, 1);
                    if (settings.maintainAspectResize)
                        imageNewSize.y = _imageEditor.at(activeImageEditor)->getSize().y * (static_cast<float>(imageNewSize.x) / _imageEditor.at(activeImageEditor)->getSize().x);
                }
                if (ImGui::InputInt("Height"_C, &imageNewSize.y, 1, 10))
                {
                    imageNewSize.y = max(imageNewSize.y, 1);
                    if (settings.maintainAspectResize)
                        imageNewSize.x = _imageEditor.at(activeImageEditor)->getSize().x * (static_cast<float>(imageNewSize.y) / _imageEditor.at(activeImageEditor)->getSize().y);
                }
                ImGui::EndDisabled();
                ImGui::Unindent();
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                _imageEditor.at(activeImageEditor)->RescaleCanvas(Vector2u(imageNewSize), static_cast<RescaleMethod>(settings.resamplingMethod));
                popUpState.pop_back();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
            }
            break;
        }
        case PopUpState::ResizeCanvas:
        {
            static bool byPercentage = false;
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * settings.GUIScale)))
            {
                ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
                if (ImGui::RadioButton("By percentage"_C, byPercentage))
                    byPercentage = true;

                ImGui::Indent();
                ImGui::BeginDisabled(!byPercentage);
                ImGui::SetNextItemWidth(150 * settings.GUIScale);
                if (ImGui::InputInt("Percentage"_C, &scalePercentage, 1, 10))
                {
                    scalePercentage = std::clamp(scalePercentage, 1, 10000);
                    imageNewSize = Vector2i(_imageEditor.at(activeImageEditor)->getSize()) * scalePercentage / 100;
                    imageNewSize.x = max(imageNewSize.x, 1);
                    imageNewSize.y = max(imageNewSize.y, 1);
                }
                ImGui::EndDisabled();
                ImGui::Unindent();

                if (ImGui::RadioButton("By absolute size"_C, !byPercentage))
                    byPercentage = false;

                ImGui::Indent();
                ImGui::BeginDisabled(byPercentage);
                ImGui::Checkbox("Maintain aspect ratio"_C, &settings.maintainAspectCanvas);
                ImGui::SetNextItemWidth(150 * settings.GUIScale);
                if (ImGui::InputInt("Width"_C, &imageNewSize.x, 1, 10))
                {
                    imageNewSize.x = max(imageNewSize.x, 1);
                    if (settings.maintainAspectCanvas)
                        imageNewSize.y = _imageEditor.at(activeImageEditor)->getSize().y * (static_cast<float>(imageNewSize.x) / _imageEditor.at(activeImageEditor)->getSize().x);
                }
                ImGui::SetNextItemWidth(150 * settings.GUIScale);
                if (ImGui::InputInt("Height"_C, &imageNewSize.y, 1, 10))
                {
                    imageNewSize.y = max(imageNewSize.y, 1);
                    if (settings.maintainAspectCanvas)
                        imageNewSize.x = _imageEditor.at(activeImageEditor)->getSize().x * (static_cast<float>(imageNewSize.y) / _imageEditor.at(activeImageEditor)->getSize().y);
                }
                ImGui::EndDisabled();
                ImGui::Unindent();
                ImGui::SetCursorPos(Vector2f(320, 50) * settings.GUIScale);
                for (int8_t i = 0; i < 3; i++)
                {
                    for (int8_t j = 0; j < 3; j++)
                    {
                        const Vector2i offset = Vector2i(j, i) - Vector2i(canvasResizePoint);
                        int8_t index = 0;
                        if (offset.x >= -1 && offset.x <= 1 && offset.y >= -1 && offset.y <= 1)
                            index = (offset.x + 1) + (offset.y + 1) * 3 + 1;
                        const float texOffset = index * 0.1f;
                        if (ImGui::ImageButton(("Button" + to_string(i) + to_string(j)).c_str(),
                            canvasIcons.getNativeHandle(), Vector2f(25, 25) * settings.GUIScale,
                            Vector2f(texOffset, 0), Vector2f(texOffset + 0.1f, 1)))
                            canvasResizePoint = Vector2i(j, i);
                        if (j < 2)
                            ImGui::SameLine();
                        else
                            ImGui::SetCursorPosX(320 * settings.GUIScale);
                    }
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                auto& editor = _imageEditor.at(activeImageEditor);
                const Vector2i size = static_cast<Vector2i>(editor->getSize());
                const Vector2i position = -Vector2i((imageNewSize.x - size.x) * canvasResizePoint.x * 0.5f,
                    (imageNewSize.y - size.y) * canvasResizePoint.y * 0.5f);
                editor->ResizeCanvas(IntRect(position, {imageNewSize.x, imageNewSize.y}));
                popUpState.pop_back();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
            }
            break;
        }
        case PopUpState::LayerProperties:
        {
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * settings.GUIScale)))
            {
                ImGui::InputText("Layer name"_C, &_layerPicker.getLayer().name[0], _layerPicker.getLayer().name.size() + 1, ImGuiInputTextFlags_CallbackResize, TextCallback, &_layerPicker.getLayer().name);
                if (ImGui::Checkbox("Enabled"_C, &_layerPicker.getLayer().enabled))
                    _imageEditor.at(activeImageEditor)->chunkManager.RenderChunkArea();
                int32_t value = _layerPicker.getLayer().transparency;
                if (ImGui::SliderInt("Transparency"_C, &value, 0, 255, "%d", ImGuiSliderFlags_AlwaysClamp))
                {
                    _layerPicker.getLayer().transparency = value;
                    _imageEditor.at(activeImageEditor)->chunkManager.RenderChunkArea();
                }
                if (ImGui::BeginCombo("Blend mode"_C, LL::ind("blendModeName[]", _layerPicker.getLayer().blendMode).c_str()))
                {
                    for (uint8_t i = 0; i < c_blendModes.size(); i++)
                        if (ImGui::Selectable(LL::ind("blendModeName[]", i).c_str()))
                        {
                            _layerPicker.getLayer().blendMode = i;
                            _imageEditor.at(activeImageEditor)->chunkManager.RenderChunkArea();
                        }
                    ImGui::EndCombo();
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
            }
            break;
        }
        case PopUpState::Open:
        {
            static string fileName;
            ImGui::SetWindowFocus();
            ImGui::SeparatorText("From file"_C);
            if (keyboardFocusHere)
            {
                ImGui::SetKeyboardFocusHere();
                keyboardFocusHere = false;
            }
            ImGui::InputText("Filename"_C, &fileName[0], fileName.size() + 1, ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_AlwaysOverwrite, TextCallback, &fileName);
            ImGui::SeparatorText("Recent"_C);
            if (ImGui::BeginChild("Scrolling", Vector2f(0, -30 * settings.GUIScale)))
            {
                if (settings.recentFiles.empty())
                    ImGui::Text("%s...", "Nothing here"_C);
                for (auto n = settings.recentFiles.rbegin(); n != settings.recentFiles.rend(); ++n)
                {
                    if (n->second.empty())
                        continue;
                    if (ImGui::Selectable(n->second.c_str()))
                    {
                        if (OpenImage(n->second))
                        {
                            popUpState.pop_back();
                            if (!popUpState.empty() && popUpState.back() == PopUpState::Setup)
                                popUpState.pop_back();
                            break;
                        }
                    }
                }
            }
            ImGui::EndChild();
            ImGui::BeginDisabled(fileName.empty());
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                if (OpenImage(fileName))
                {
                    popUpState.pop_back();
                    if (!popUpState.empty() && popUpState.back() == PopUpState::Setup)
                        popUpState.pop_back();
                }
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
                popUpState.pop_back();
        }
        break;
        case PopUpState::Save:
        {
            static string fileName;
            static int32_t extension = 0;
            static int32_t jpgQuality = 75;
            static bool exists = false;
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * settings.GUIScale)))
            {
                if (ImGui::InputText("Filename"_C, &fileName[0], fileName.size() + 1, ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_EnterReturnsTrue, TextCallback, &fileName))
                {
                    exists = filesystem::exists(fileName + c_imageExtensions.at(extension));
                }
                if (ImGui::BeginCombo("Extension"_C, c_imageExtensions.at(extension).c_str()))
                {
                    for (int32_t i = 0; i < c_imageExtensions.size(); i++)
                    {
                        if (ImGui::Selectable(c_imageExtensions.at(i).c_str()))
                        {
                            extension = i;
                            exists = filesystem::exists(fileName + c_imageExtensions.at(extension));
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::BeginDisabled(extension != 1);
                ImGui::SliderInt("Quality"_C, &jpgQuality, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
                ImGui::EndDisabled();

                if (exists)
                    ImGui::TextColored(Color(255, 200, 0), "%s", "glxyWarning1"_C);
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                _imageEditor.at(activeImageEditor)->imagePath = fileName + c_imageExtensions.at(extension);
                _imageEditor.at(activeImageEditor)->imageName = filesystem::path(fileName + c_imageExtensions.at(extension)).filename().string();
                _imageEditor.at(activeImageEditor)->imageJPGQuality = jpgQuality;
                if (SaveImage())
                {
                    if (editorCloseAttempt >= 0)
                    {
                        DeleteEditor(editorCloseAttempt);
                        editorCloseAttempt = -1;
                    }
                    popUpState.pop_back();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            {
                if (editorCloseAttempt >= 0)
                    editorCloseAttempt = -1;
                popUpState.pop_back();
            }
        }
        break;
        case PopUpState::New:
        {
            static int32_t background = 0;
            const array colors = {
                Color::White,
                Color::Black,
                Color::Transparent,
            };
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * settings.GUIScale)))
            {
                ImGui::DragInt2("Resolution"_C, &resolution.x, 1, 1, 1e5, "%d", ImGuiSliderFlags_AlwaysClamp);
                ImGui::Spacing();
                ImGui::Text("%s:", "Background"_C);
                ImGui::RadioButton("White"_C, &background, 0);
                ImGui::RadioButton("Black"_C, &background, 1);
                ImGui::RadioButton("Transparent"_C, &background, 2);
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                CreateEmptyImage(Vector2u(resolution), colors[background]);
                popUpState.pop_back();
                if (!popUpState.empty() && popUpState.back() == PopUpState::Setup)
                    popUpState.pop_back();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
            }
        }
        break;
        case PopUpState::Setup:
        {
            if (openWithGalaxy)
            {
                openWithGalaxyFile = openWithGalaxyFile.lexically_normal();
                if (filesystem::exists(openWithGalaxyFile))
                {
                    if (OpenImage(openWithGalaxyFile))
                    {
                        popUpState.pop_back();
                        break;
                    }
                }
                openWithGalaxyFile = "";
            }
            ImGui::SetWindowFocus();
            const float width = ImGui::GetContentRegionAvail().x / 2 - ImGui::GetStyle().ChildBorderSize * 3 * 2;
            for (int8_t i = 0; i < 3; i++)
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, Color(255, 255, 255, 32));
                ImGui::BeginChild(("MenuItem" + to_string(i)).c_str(),
                    Vector2f(width, width * 1.1f) +
                    Vector2f(ImGui::GetStyle().ChildBorderSize * 3, ImGui::GetStyle().FramePadding.y * 3), false,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
                const bool selected = ImGui::IsMouseDown(ImGuiMouseButton_Left);
                Color c;
                if (hovered && selected)
                    c = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
                else if (hovered)
                    c = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
                else
                    c = Color(255, 255, 255, 32);
                ImGui::Image(setupSelect.getNativeHandle(), Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x),
                    Vector2f((i % 2) * 0.5f, (i / 2) * 0.5f), Vector2f((i % 2) * 0.5f + 0.5f, (i / 2) * 0.5f + 0.5f), c);
                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, Vector2f(0.5f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, Color(255, 0, 255));
                ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.f);
                ImGui::Selectable(LL::ind("setupOption[]", i).c_str(), hovered, ImGuiSelectableFlags_Disabled, ImGui::GetContentRegionAvail());
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();
                ImGui::EndChild();
                ImGui::PopStyleColor();
                if (i % 2 == 0)
                    ImGui::SameLine();
                if (hovered && (InputEvent::isButtonReleased(Mouse::Button::Left) || InputEvent::isTouchReleased(0)))
                {
                    switch (i)
                    {
                    case 0: popUpState.push_back(PopUpState::New); break;
                    case 1: popUpState.push_back(PopUpState::Open); break;
                    case 2: popUpState.push_back(PopUpState::Settings); break;
                    default: break;
                    }
                }
            }
        }
        break;
        case PopUpState::ThreadWork:
            ImGui::ProgressBar(workerThreadProgress, ImGui::GetContentRegionAvail());
            if (workerThreadProgress >= 1.f)
            {
                workerThread.reset();
                popUpState.pop_back();
            }
            break;
        case PopUpState::GLScan:
        {
            static bool firstScan = true;
            static int32_t printData = 0;
            static vector<string> GLdata;
            if (firstScan)
            {
                for (int8_t i = 0; i < 10; i++)
                {
                    if (window.setActive())
                    {
                        string x;
                        x += reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
                        string y;
                        for (int32_t j = 0; j < x.size(); j++)
                        {
                            if (x.at(j) == ' ')
                            {
                                GLdata.push_back(y);
                                y.clear();
                                continue;
                            }
                            y += x.at(j);
                        }
                        GLdata.push_back("Version"_S + ": " + reinterpret_cast<const char*>(glGetString(GL_VERSION)));
                        GLdata.push_back("Vendor"_S + ": " + reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
                        GLdata.push_back("Renderer"_S + ": " + reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
                        GLdata.push_back("Shaders available"_S + ": " + (Shader::isAvailable() ? "yes"_S : "no"_S));
                        GLdata.push_back("Vertex buffer available"_S + ": " + (VertexBuffer::isAvailable() ? "yes"_S : "no"_S));
                        GLdata.push_back("Vulkan available"_S + ": " + (Vulkan::isAvailable() ? "yes"_S : "no"_S));

                        break;
                    }
                    sleep(milliseconds(30));
                }
                firstScan = false;
            }

            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * settings.GUIScale)))
            {
                for (int32_t i = 0; i < min(printData, static_cast<int32_t>(GLdata.size())); i++)
                    ImGui::Text("%s", GLdata.at(i).c_str());
                if (printData <= GLdata.size())
                    ImGui::SetScrollHereY(1.0f);
                printData++;
            }
            ImGui::EndChild();

            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                printData = 0;
                popUpState.pop_back();
            }
        }
            break;
        case PopUpState::Changelog: case PopUpState::FuturePlan:
        {
            const string& t = popUpState.back() == PopUpState::Changelog ? c_changelog : c_futurePlan;
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * settings.GUIScale)))
            {
                string x;
                for (int32_t i = 0; i < t.size(); i++)
                {
                    if (t.at(i) == '\n')
                    {
                        if (!x.empty())
                        {
                            if (x.front() == '-')
                            {
                                x.erase(0, 1);
                                ImGui::Bullet();
                                ImGui::TextWrapped("%s", x.c_str());
                            }
                            else
                            {
                                ImGui::Spacing();
                                ImGui::SeparatorText(x.c_str());
                            }
                            x.clear();
                        }
                        continue;
                    }
                    x += t.at(i);
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
                popUpState.pop_back();
            break;
        }
        case PopUpState::Settings:
        {
            static array bgColor = { settings.bgColor.r / 255.f, settings.bgColor.g / 255.f, settings.bgColor.b / 255.f };
            static int8_t pressedID = -1;
            static bool editing = false;
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * settings.GUIScale)))
            {
                ImGui::SeparatorText("Graphics"_C);
                if (ImGui::BeginCombo("Resolution"_C, (to_string(settings.resolution.x) + "x" + to_string(settings.resolution.y)).c_str()))
                {
                    const std::vector<VideoMode>& res = VideoMode::getFullscreenModes();
                    for (int32_t i = 0; i < res.size(); i++)
                    {
                        if (res.at(i).size.x < 800 || res[i].size.y < 600)
                            continue;
                        if (find(res.begin(), res.begin() + i, VideoMode{Vector2u(res.at(i).size.x, res.at(i).size.y), 32U}) != res.begin() + i)
                            continue;
                        string t = to_string(res.at(i).size.x) + "x" + to_string(res.at(i).size.y);
                        if (ImGui::Selectable(t.c_str()))
                        {
                            window.setSize(Vector2u(res.at(i).size.x, res.at(i).size.y));
                        }
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::Checkbox("Fullscreen window"_C, &settings.fullscreen))
                    RecreateAppWindow();
                if (ImGui::Checkbox("Vertical sync"_C, &settings.verticalSync))
                {
                    if (settings.verticalSync)
                    {
                        window.setFramerateLimit(0);
                        window.setVerticalSyncEnabled(true);
                    }
                    else
                    {
                        window.setVerticalSyncEnabled(false);
                        window.setFramerateLimit(settings.maxFPS);
                    }
                }
                ToolTip("glxyTooltip1"_S);

                ImGui::BeginDisabled(settings.verticalSync);
                ImGui::InputInt("Max FPS"_C, &settings.maxFPS, 0, 0);
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (settings.maxFPS < 5)
                        settings.maxFPS = 5;
                    else if (settings.maxFPS > 1000)
                        settings.maxFPS = 1000;
                    window.setFramerateLimit(settings.maxFPS);
                }
                ToolTip("glxyTooltip2"_S);
                ImGui::EndDisabled();

                if (ImGui::BeginCombo("Antialiasing"_C, ("x" + to_string(settings.antialiasing)).c_str()))
                {
                    if (ImGui::Selectable("x0"))
                    {
                        settings.antialiasing = 0;
                        for (auto& n : _imageEditor)
                            n->RecreateEditorTexture();
                    }
                    for (int8_t i = 1; i <= 8; i *= 2)
                        if (ImGui::Selectable(("x" + to_string(i)).c_str()))
                        {
                            settings.antialiasing = i;
                            for (auto& n : _imageEditor)
                                n->RecreateEditorTexture();
                        }
                    ImGui::EndCombo();
                }
                ToolTip("glxyTooltip3"_S);

                ImGui::Spacing();
                ImGui::SeparatorText("Visual"_C);
                if (ImGui::BeginCombo("Theme"_C, LL::ind("ThemeType[]", GLOBAL.themeID).c_str()))
                {
                    for (int8_t i = 0; i < 4; i++)
                    {
                        if (ImGui::Selectable(LL::ind("ThemeType[]", i).c_str()))
                        {
                            GLOBAL.themeID = i;
                            switch (GLOBAL.themeID)
                            {
                            case 0: StyleColorsThemeDefault(); break;
                            case 1: StyleColorsThemeRed(); break;
                            case 2: StyleColorsThemeMidnight(); break;
                            case 3: ImGui::StyleColorsDark(); break;
                            default: break;
                            }
                            for (auto& n : _imageEditor)
                                n->setThemeColor();

                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::PushItemWidth(300 * settings.GUIScale);
                ImGui::SliderInt("Font size"_C, &settings.fontSize, 12, 24, "%d", ImGuiSliderFlags_AlwaysClamp);
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("Apply"_C))
                {
                    settings.GUIScale = settings.fontSize / 16.f;
                    changeFont = true;
                }
                if (ImGui::BeginCombo("Language"_C, c_languageNames.at(settings.languageID).c_str()))
                {
                    for (uint8_t i = 0; i < c_languageCnt; i++)
                        if (ImGui::Selectable(c_languageNames.at(i).c_str()))
                        {
                            settings.languageID = i;
                            LL::setLanguageID(settings.languageID);
                        }
                    ImGui::EndCombo();
                }
                ImGui::Checkbox("Show framerate"_C, &settings.showFPS);
                ImGui::Spacing();
                ImGui::SeparatorText("Editor"_C);
                ImGui::Checkbox("Freeze when out of focus"_C, &settings.outOfFocus); ToolTip("glxyTooltip4"_S);
                ImGui::Checkbox("Draw inner selection lines"_C, &settings.drawSelectionLines);
                ImGui::Checkbox("Enable touchpad support"_C, &settings.touchPadSupport);
                if (ImGui::BeginCombo("Pan mouse button"_C, LL::ind("panMouseButton[]", settings.panMouseButton).c_str()))
                {
                    for (int8_t i = 0; i < 3; i++)
                    {
                        if (ImGui::Selectable(LL::ind("panMouseButton[]", i).c_str()))
                            settings.panMouseButton = i;
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::ColorEdit3("Background color"_C, bgColor.data(), ImGuiColorEditFlags_Uint8))
                {
                    settings.bgColor.r = bgColor[0] * 255;
                    settings.bgColor.g = bgColor[1] * 255;
                    settings.bgColor.b = bgColor[2] * 255;
                }

                ImGui::Spacing();
                ImGui::SeparatorText("Animations"_C);
                ImGui::Checkbox("Animate zoom"_C, &settings.animateZoom);
                ImGui::Checkbox("Animate pan"_C, &settings.animatePan);
                ImGui::Spacing();
                ImGui::SeparatorText("Shortcuts"_C);

                for (int8_t i = 0; i < static_cast<int8_t>(ActionShortcut::Count); i++)
                {
                    const Vector2f def = ImGui::GetStyle().SelectableTextAlign;
                    ImGui::GetStyle().SelectableTextAlign = Vector2f(0.5, 0.5);
                    if (editing && pressedID == i)
                        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.f);
                    ImGui::Selectable((LL::ind("shortcut[]", i) + "##Select" + to_string(i)).c_str(), false, ImGuiSelectableFlags_Disabled, Vector2f(ImGui::GetContentRegionAvail().x / 3, 25.f * settings.GUIScale));
                    if (editing && pressedID == i)
                        ImGui::PopStyleVar();
                    ImGui::SameLine();
                    if (ImGui::Button((Shortcuts::getName(static_cast<ActionShortcut>(i)) + "##Button" + to_string(i)).c_str(), Vector2f(ImGui::GetContentRegionAvail().x / 1.5f, 25.f * settings.GUIScale)))
                    {
                        editing = true;
                        pressedID = i;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(("Clear"_S + "##" + to_string(i)).c_str(), Vector2f(ImGui::GetContentRegionAvail().x / 1.3f, 25.f * settings.GUIScale)))
                    {
                        Shortcuts::clear(static_cast<ActionShortcut>(i));
                    }
                    if (editing && pressedID == i)
                    {
                        Shortcuts::captureShortcut(static_cast<ActionShortcut>(i));
                        string name = Shortcuts::getName(static_cast<ActionShortcut>(i));
                        if (!name.empty() && name.back() != ' ')
                            editing = false;
                    }
                    ImGui::GetStyle().SelectableTextAlign = def;
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                editing = false;
                settings.Save();
                popUpState.pop_back();
            }
        }
        break;
        case PopUpState::About:
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * settings.GUIScale)))
            {
                ImGui::TextWrapped("%s", "glxyAbout1"_C);
                ImGui::TextWrapped("%s", "glxyAbout2"_C);
                ImGui::Text("%s", "glxyAbout3"_C);
                ImGui::Bullet();
                ImGui::SameLine();
                ImGui::TextWrapped("%s", "glxyAbout4"_C);
                ImGui::Bullet();
                ImGui::SameLine();
                ImGui::TextWrapped("%s", "glxyAbout5"_C);

                ImGui::TextWrapped("%s", "glxyAbout6"_C);
                ImGui::Spacing();
                ImGui::TextWrapped("glxyAbout7"_C, c_AppVersion.c_str(), BUILDNUMBER, c_SFML_ARCH.c_str());
                ImGui::Indent();
                ImGui::TextWrapped("glxyAbout8"_C, SFML_VERSION_MAJOR, SFML_VERSION_MINOR, SFML_VERSION_PATCH);
                ImGui::TextWrapped("glxyAbout9"_C, ImGui::GetVersion());
                ImGui::Unindent();
                ImGui::TextWrapped("glxyAbout10"_C, GalaxyStartUpTime.asMilliseconds());
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
                popUpState.pop_back();
            ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - windowLogoTexture.getSize().x / 3 * settings.GUIScale);
            ImGui::SetCursorPosY(20 * settings.GUIScale);
            ImGui::Image(windowLogoTexture, Vector2f(windowLogoTexture.getSize()) / 3.f * settings.GUIScale);
            break;
        default:
            break;
        }
        ImGui::EndPopup();
    }
}