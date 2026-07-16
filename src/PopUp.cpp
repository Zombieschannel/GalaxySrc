#include "App.hpp"
#include "ImGuiFunc.hpp"
#include "Global.hpp"
#include "ZEditorsCommon/Shortcuts.hpp"
#include "ZEditorsCommon/Themes.hpp"
#include "ZEditorsCommon/Languages.hpp"
#include <SFML/OpenGL.hpp>

#define BUILDNUMBER 4796
void glxy::App::PopUp()
{
    if (popUpState.empty())
        return;
    static bool keyboardFocusHere = false;
    static bool setWindowFocus = false;
    static Vector2i imageNewSize;
    static int32_t scalePercentage;
    static Pivot canvasResizePivot;
    static Vector2i resolution;
    static Vector2f transformImagePosition;
    static Vector2f transformImageOrigin;
    static Vector2f transformImageScale = Vector2f(1, 1);
    static float transformImageRotation = 0;
    static int32_t transformImageRepeat = 1;
    static int32_t circularShift = 0;
    static bool circularHorizontal = true;
    static int32_t circularGroup = 1;
    static int8_t maxOctaves = 1;
    static float workerTodoWork = 0;
    static unique_ptr<AdjustmentData> adjustmentData;
    static unique_ptr<EffectData> effectData;
    static vector<pair<int32_t, int32_t>> searchItems;
    static int8_t searchCount = 7;
    static int32_t searchTargetID = 0;
    bool reRender = false;
    bool popStyle = false;
    constexpr array popUpSize = {
        Vector2f(550, 450), //Settings
        Vector2f(630, 360), //About
        Vector2f(690, 540), //Changelog
        Vector2f(690, 540), //FuturePlan
        Vector2f(440, 100), //ThreadWork
        Vector2f(530, 350), //GLScan
        Vector2f(440, 300), //New
        Vector2f(650, 500), //Save
        Vector2f(650, 500), //Open
        Vector2f(500, 200), //LayerProperties
        Vector2f(500, 300), //Resize
        Vector2f(500, 275), //ResizeCanvas
        Vector2f(350, 425), //Setup
        Vector2f(500, 240), //TransformImage
        Vector2f(300, 150), //SaveBeforeExit
        Vector2f(300, 130), //SaveBeforeClose
        Vector2f(400, 200), //Adjustment
        Vector2f(400, 250), //Effect
        Vector2f(0, 0),     //ToolChanged
        Vector2f(300, 110), //GridBold
        Vector2f(400, 370), //Search
        Vector2f(550, 400), //Recent
        Vector2f(400, 200), //CircularShift
        Vector2f(300, 120), //SaveAdditional
    };
    struct SearchEntry
    {
        string name;
        void (App::*callback)();
        CanvasType type;
    };
    const array searchOptions = {
        SearchEntry{"fileMenu[0]"_S, &App::MenuNew, CanvasType::FixedOrInfinite},
        SearchEntry{"fileMenu[1]"_S, &App::MenuOpen, CanvasType::FixedOrInfinite},
        SearchEntry{"fileMenu[2]"_S, &App::MenuOpenRecent, CanvasType::FixedOrInfinite},
        SearchEntry{"fileMenu[3]"_S, &App::MenuSave, CanvasType::Fixed},
        SearchEntry{"fileMenu[4]"_S, &App::MenuSaveAs, CanvasType::Fixed},
        SearchEntry{"editMenu[0]"_S, &App::MenuCopy, CanvasType::Fixed},
        SearchEntry{"editMenu[1]"_S, &App::MenuCut, CanvasType::Fixed},
        SearchEntry{"editMenu[2]"_S, &App::MenuPaste, CanvasType::Fixed},
        SearchEntry{"editMenu[4]"_S, &App::MenuDeselectAll, CanvasType::Fixed},
        SearchEntry{"editMenu[5]"_S, &App::MenuDelete, CanvasType::Fixed},
        SearchEntry{"viewMenu[0]"_S, &App::MenuZoomIn, CanvasType::FixedOrInfinite},
        SearchEntry{"viewMenu[1]"_S, &App::MenuZoomOut, CanvasType::FixedOrInfinite},
        SearchEntry{"viewMenu[4]"_S, &App::MenuGrid, CanvasType::FixedOrInfinite},
        SearchEntry{"viewMenu[5]"_S, &App::MenuGridBold, CanvasType::FixedOrInfinite},
        SearchEntry{"viewMenu[6]"_S, &App::MenuRuler, CanvasType::FixedOrInfinite},
        SearchEntry{"viewMenu[7]"_S, &App::MenuActualSize, CanvasType::FixedOrInfinite},
        SearchEntry{"viewMenu[8]"_S, &App::MenuSyncViewport, CanvasType::FixedOrInfinite},
        SearchEntry{"imageMenu[0]"_S, &App::MenuCrop, CanvasType::Fixed},
        SearchEntry{"imageMenu[1]"_S, &App::MenuResize, CanvasType::Fixed},
        SearchEntry{"imageMenu[2]"_S, &App::MenuResizeCanvas, CanvasType::Fixed},
        SearchEntry{"imageMenu[3]"_S, &App::MenuFlipImageHorizontal, CanvasType::Fixed},
        SearchEntry{"imageMenu[4]"_S, &App::MenuFlipImageVertical, CanvasType::Fixed},
        SearchEntry{"imageMenu[5]"_S, &App::MenuRotate90CW, CanvasType::Fixed},
        SearchEntry{"imageMenu[6]"_S, &App::MenuRotate90CCW, CanvasType::Fixed},
        SearchEntry{"imageMenu[7]"_S, &App::MenuRotate180, CanvasType::Fixed},
        SearchEntry{"imageMenu[8]"_S, &App::MenuTransformImage, CanvasType::Fixed},
        SearchEntry{"imageMenu[9]"_S, &App::MenuCircularShift, CanvasType::Fixed},
        SearchEntry{"layerMenu[0]"_S, &App::MenuNewLayer, CanvasType::FixedOrInfinite},
        SearchEntry{"layerMenu[1]"_S, &App::MenuDeleteLayer, CanvasType::FixedOrInfinite},
        SearchEntry{"layerMenu[2]"_S, &App::MenuDuplicateLayer, CanvasType::FixedOrInfinite},
        SearchEntry{"layerMenu[3]"_S, &App::MenuMoveLayerUp, CanvasType::FixedOrInfinite},
        SearchEntry{"layerMenu[4]"_S, &App::MenuMoveLayerDown, CanvasType::FixedOrInfinite},
        SearchEntry{"layerMenu[5]"_S, &App::MenuMergeLayerDown, CanvasType::FixedOrInfinite},
        SearchEntry{"layerMenu[6]"_S, &App::MenuFlipLayerHorizontal, CanvasType::Fixed},
        SearchEntry{"layerMenu[7]"_S, &App::MenuFlipLayerVertical, CanvasType::Fixed},
        SearchEntry{"layerMenu[8]"_S, &App::MenuLayerProperties, CanvasType::FixedOrInfinite},
        SearchEntry{"adjustMenu[0]"_S, &App::MenuAdjustBlackAndWhite, CanvasType::Fixed},
        SearchEntry{"adjustMenu[1]"_S, &App::MenuAdjustBrightnessContrast, CanvasType::Fixed},
        SearchEntry{"adjustMenu[2]"_S, &App::MenuAdjustHSV, CanvasType::Fixed},
        SearchEntry{"adjustMenu[3]"_S, &App::MenuAdjustInvert, CanvasType::Fixed},
        SearchEntry{"adjustMenu[4]"_S, &App::MenuAdjustTint, CanvasType::Fixed},
        SearchEntry{"effectMenu[0]"_S, &App::MenuEffectGauss, CanvasType::Fixed},
        SearchEntry{"effectMenu[1]"_S, &App::MenuEffectBox, CanvasType::Fixed},
        SearchEntry{"effectMenu[2]"_S, &App::MenuEffectDirectional, CanvasType::Fixed},
        SearchEntry{"effectMenu[3]"_S, &App::MenuEffectWhite, CanvasType::Fixed},
        SearchEntry{"effectMenu[4]"_S, &App::MenuEffectFractal, CanvasType::Fixed},
        SearchEntry{"effectMenu[5]"_S, &App::MenuEffectVignette, CanvasType::Fixed},
        SearchEntry{"effectMenu[6]"_S, &App::MenuEffectMandelbrot, CanvasType::Fixed},
        SearchEntry{"effectMenu[7]"_S, &App::MenuEffectSharpening, CanvasType::Fixed},
        SearchEntry{"otherMenu[0]"_S, &App::MenuChangelog, CanvasType::FixedOrInfinite},
        SearchEntry{"otherMenu[1]"_S, &App::MenuFuturePlan, CanvasType::FixedOrInfinite},
        SearchEntry{"otherMenu[2]"_S, &App::MenuGLScan, CanvasType::FixedOrInfinite},
        SearchEntry{"otherMenu[3]"_S, &App::MenuDebug, CanvasType::FixedOrInfinite},
        SearchEntry{"otherMenu[4]"_S, &App::MenuAbout, CanvasType::FixedOrInfinite},
        SearchEntry{"selectSubmenu[0]"_S, &App::MenuSelectAll, CanvasType::Fixed},
        SearchEntry{"selectSubmenu[1]"_S, &App::MenuSelectLeft, CanvasType::Fixed},
        SearchEntry{"selectSubmenu[2]"_S, &App::MenuSelectRight, CanvasType::Fixed},
        SearchEntry{"selectSubmenu[3]"_S, &App::MenuSelectTop, CanvasType::Fixed},
        SearchEntry{"selectSubmenu[4]"_S, &App::MenuSelectBottom, CanvasType::Fixed},
        SearchEntry{"selectSubmenu[5]"_S, &App::MenuSelectTopLeft, CanvasType::Fixed},
        SearchEntry{"selectSubmenu[6]"_S, &App::MenuSelectTopRight, CanvasType::Fixed},
        SearchEntry{"selectSubmenu[7]"_S, &App::MenuSelectBottomLeft, CanvasType::Fixed},
        SearchEntry{"selectSubmenu[8]"_S, &App::MenuSelectBottomRight, CanvasType::Fixed},
    };
    if (resetPopupWindow)
    {
        if (ImGui::IsPopupOpen(LL::ind("PopUpTitle[]", static_cast<int8_t>(popUpState.back())).c_str()))
        {
            const Vector2f size = popUpSize.at(static_cast<int8_t>(popUpState.back())) * config.GUIScale;
            ImGui::SetNextWindowSize(size);
            ImGui::SetNextWindowPos(Vector2f(window.getSize().x / 2 - size.x / 2, window.getSize().y / 2 - size.y / 2));
        }
        resetPopupWindow = false;
    }
    if (!ImGui::IsPopupOpen(LL::ind("PopUpTitle[]", static_cast<int8_t>(popUpState.back())).c_str()))
    {
        switch (popUpState.back())
        {
        case PopUpState::Search:
            keyboardFocusHere = true;
            break;
        case PopUpState::ThreadWork:
            workerTodoWork = CanvasWorker::getWorkAmount();
            break;
        case PopUpState::Effect:
            reRender = true;
            setWindowFocus = true;
            maxOctaves = std::min(log2((_imageEditor.at(activeImageEditor)->getSize().x + _imageEditor.at(activeImageEditor)->getSize().y) / 2.f) + 1, 15.f);
            adjEffConfig.fractalOctaves = maxOctaves;
            AddWork(CanvasWork::Finish{});
            break;
        case PopUpState::Adjustment:
            reRender = true;
            setWindowFocus = true;
            AddWork(CanvasWork::Finish{});
            break;
        case PopUpState::CircularShift:
            reRender = true;
            setWindowFocus = true;
            break;
        case PopUpState::TransformImage:
            reRender = true;
            setWindowFocus = true;
            break;
        case PopUpState::Resize:
            setWindowFocus = true;
            imageNewSize = Vector2i(_imageEditor.at(activeImageEditor)->getSize());
            scalePercentage = 100;
            break;
        case PopUpState::ResizeCanvas:
            setWindowFocus = true;
            imageNewSize = Vector2i(_imageEditor.at(activeImageEditor)->getSize());
            scalePercentage = 100;
            canvasResizePivot = Pivot::Center;
            break;
        case PopUpState::New:
            if (clipboardImage)
                resolution = Vector2i(clipboardImage->getSize());
            else
                resolution = Vector2i(800, 600);
            break;
            setWindowFocus = true;
        case PopUpState::Open:
            keyboardFocusHere = true;
            setWindowFocus = true;
            break;
        case PopUpState::Recent:
            keyboardFocusHere = true;
            setWindowFocus = true;
            break;
        case PopUpState::Settings: case PopUpState::Save: case PopUpState::LayerProperties: case PopUpState::GridBold: case PopUpState::Setup:
            setWindowFocus = true;
            break;
        default: break;
        }
        const Vector2f size = popUpSize.at(static_cast<int8_t>(popUpState.back())) * config.GUIScale;
        ImGui::SetNextWindowSize(size);
        ImGui::SetNextWindowPos(Vector2f(window.getSize().x / 2 - size.x / 2, window.getSize().y / 2 - size.y / 2), ImGuiCond_Always);
        ImGui::OpenPopup(LL::ind("PopUpTitle[]", static_cast<int8_t>(popUpState.back())).c_str());
    }
    switch (popUpState.back())
    {
    case PopUpState::Adjustment: case PopUpState::Effect: case PopUpState::LayerProperties: case PopUpState::TransformImage: case PopUpState::CircularShift:
        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, Color::Transparent);
        popStyle = true;
        break;
    default: break;
    }
    if (ImGui::BeginPopupModal(LL::ind("PopUpTitle[]", static_cast<int8_t>(popUpState.back())).c_str(), nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings))
    {
        switch (popUpState.back())
        {
        case PopUpState::SaveAdditional:
        {
            static int32_t jpgQuality = 75;
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                if (setWindowFocus)
                {
                    ImGui::SetWindowFocus();
                    setWindowFocus = false;
                }
                ImGui::SliderInt("Quality"_C, &jpgQuality, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                auto& ie = _imageEditor.at(activeImageEditor);
                ie->imageJPGQuality = jpgQuality;
                ie->Save();
                popUpState.pop_back();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
                popUpState.pop_back();
            break;
        }
        case PopUpState::CircularShift:
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                if (setWindowFocus)
                {
                    ImGui::SetWindowFocus();
                    setWindowFocus = false;
                }
                if (ImGui::DragInt("Shift"_C, &circularShift, 1, -1e8, 1e8, "%d", ImGuiSliderFlags_AlwaysClamp))
                    reRender = true;
                if (ImGui::DragInt("Group size"_C, &circularGroup, 1, 1, circularHorizontal ? _imageEditor.at(activeImageEditor)->getSize().y :
                    _imageEditor.at(activeImageEditor)->getSize().x, "%d", ImGuiSliderFlags_AlwaysClamp))
                    reRender = true;
                if (ImGui::RadioButton("Horizontal"_C, circularHorizontal))
                {
                    circularHorizontal = true;
                    reRender = true;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Vertical"_C, !circularHorizontal))
                {
                    circularHorizontal = false;
                    reRender = true;
                }
                if (circularHorizontal && _imageEditor.at(activeImageEditor)->getSize().y % circularGroup != 0 ||
                    !circularHorizontal && _imageEditor.at(activeImageEditor)->getSize().x % circularGroup != 0)
                {
                    ImGui::TextColored(Color(255, 200, 0), "%s", "glxyWarning3"_C);
                }
                if (reRender)
                    AddWork(CanvasWork::CircularShift{circularShift, circularHorizontal, circularGroup});
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
                _imageEditor.at(activeImageEditor)->unsavedChanges = true;
                AddWorkAndWait(CanvasWork::Finish{});
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
                AddWorkAndWait(CanvasWork::Cancel{});
            }
            break;
        case PopUpState::Recent:
            if (keyboardFocusHere)
            {
                ImGui::SetKeyboardFocusHere();
                keyboardFocusHere = false;
            }
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                if (config.recentFiles.empty())
                    ImGui::Text("%s...", "Nothing here"_C);
                for (auto n = config.recentFiles.rbegin(); n != config.recentFiles.rend(); ++n)
                {
                    if (n->second.empty())
                        continue;
                    if (ImGui::Selectable(n->second.c_str()))
                        OpenImage(n->second);
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
                popUpState.pop_back();
            break;
        case PopUpState::Search:
        {
            static string targetName;
            bool first = false;
            bool isInfinite = _imageEditor.at(activeImageEditor)->isInfinite;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (keyboardFocusHere)
            {
                ImGui::SetKeyboardFocusHere();
                first = true;
                keyboardFocusHere = false;
            }
            if (ImGui::InputText("##Target", targetName.data(), targetName.capacity() + 1, ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_AutoSelectAll, TextCallback, &targetName) || first)
            {
                searchItems.clear();
                auto filter = [&](const string& name)
                {
                    if (name == "editMenu[0]"_S || name == "editMenu[1]"_S || name == "editMenu[4]"_S || name == "editMenu[5]"_S ||
                        name == "imageMenu[0]"_S)
                        return !_imageEditor.at(activeImageEditor)->chunkManager.getFinalSelectionBounds().expired();
                    if (name == "editMenu[2]"_S)
                        return clipboardImage != nullptr;
                    if (name == "layerMenu[1]"_S)
                        return _imageEditor.at(activeImageEditor)->chunkManager.getLayerCount() > 1;
                    if (name == "layerMenu[3]"_S)
                        return _layerPicker.getLayerIDSelected(activeImageEditor) < _imageEditor.at(activeImageEditor)->chunkManager.getLayerCount() - 1;
                    if (name == "layerMenu[4]"_S || name == "layerMenu[5]"_S)
                        return _layerPicker.getLayerIDSelected(activeImageEditor) > 0;
                    return true;
                };
                if (!targetName.empty())
                {
                    if (!first)
                        searchTargetID = 0;
                    auto toLower = [](string data)
                    {
                        std::transform(data.begin(), data.end(), data.begin(), [](unsigned char c){ return tolower(c); });
                        return data;
                    };
                    for (int8_t i = 0; i < searchOptions.size(); i++)
                    {
                        if (isInfinite && searchOptions.at(i).type == CanvasType::Fixed ||
                            !isInfinite && searchOptions.at(i).type == CanvasType::Infinite)
                            continue;
                        if (!filter(searchOptions.at(i).name))
                            continue;
                        string comp = toLower(searchOptions.at(i).name);
                        string targetLower = toLower(targetName);
                        int32_t t = comp.find(targetLower);
                        if (t == string::npos)
                        {
                            t = 50;
                            while (!targetLower.empty())
                            {
                                int32_t tm = comp.find(targetLower.front());
                                if (tm != string::npos)
                                {
                                    if (t > 0)
                                        t--;
                                    comp.erase(tm, 1);
                                }
                                targetLower.erase(0, 1);
                            }
                        }
                        searchItems.emplace_back(pair(t, i));
                    }
                    std::sort(searchItems.begin(), searchItems.end(), std::less());
                }
                else
                {
                    for (int8_t i = 0; i < searchOptions.size(); i++)
                    {
                        if (isInfinite && searchOptions.at(i).type == CanvasType::Fixed ||
                            !isInfinite && searchOptions.at(i).type == CanvasType::Infinite)
                            continue;
                        if (!filter(searchOptions.at(i).name))
                            continue;
                        searchItems.emplace_back(pair(0, i));
                    }
                }
            }
            bool selected = false;
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                searchCount = std::floor(ImGui::GetContentRegionAvail().y / ((15.f + ImGui::GetStyle().ItemSpacing.y) * config.GUIScale));
                for (int8_t i = 0; i < searchCount && i < searchItems.size(); i++)
                {
                    if (ImGui::Selectable((searchOptions.at(searchItems.at(i).second).name +
                        "##search" + to_string(i)).c_str(), false,
                        searchTargetID == i ? ImGuiSelectableFlags_Highlight : ImGuiSelectableFlags_None, Vector2f(0, 15 * config.GUIScale)))
                    {
                        searchTargetID = i;
                        selected = true;
                    }
                }
                ImGui::EndChild();
            }
            if (InputEvent::isKeyHeld(Keyboard::Key::Up))
            {
                searchTargetID--;
                if (searchTargetID < 0)
                    searchTargetID = searchCount - 1;
            }
            if (InputEvent::isKeyHeld(Keyboard::Key::Down))
            {
                searchTargetID++;
                searchTargetID %= searchCount;
            }
            if (selected || InputEvent::isKeyHeld(Keyboard::Key::Enter))
            {
                popUpState.pop_back();
                (this->*searchOptions.at(searchItems.at(searchTargetID).second).callback)();
            }
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, 0)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
            }
            break;
        }
        case PopUpState::GridBold:
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                if (setWindowFocus)
                {
                    ImGui::SetWindowFocus();
                    setWindowFocus = false;
                }
                ImGui::InputInt2("Grid bold size"_C, &config.gridBold.x);
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (config.gridBold.y < 0)
                        config.gridBold.x = 0;
                    if (config.gridBold.y < 0)
                        config.gridBold.y = 0;
                    for (auto& n : _imageEditor)
                        n->OptionGridBold(config.gridBold);
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
                popUpState.pop_back();
            break;
        case PopUpState::Effect:
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                if (setWindowFocus)
                {
                    ImGui::SetWindowFocus();
                    setWindowFocus = false;
                }
                switch (targetEffect)
                {
                case Effects::GaussianBlur:
                    if (ImGui::SliderInt("Radius"_C, &adjEffConfig.gaussBlurRadius, 0, 10, "%d", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (reRender)
                    {
                        effectData = make_unique<EffectGaussBlur>(adjEffConfig.gaussBlurRadius);
                        auto& effect = *reinterpret_cast<EffectGaussBlur*>(effectData.get());
                        effect.values.resize(adjEffConfig.gaussBlurRadius * 2 + 1);
                        for (int32_t i = 0; i <= effect.values.size() / 2; i++)
                            effect.values.at(i) = effect.values.at(effect.values.size() - i - 1) = Binomial(effect.values.size() - 1, i);
                    }
                    break;
                case Effects::BoxBlur:
                    if (ImGui::SliderInt("Radius"_C, &adjEffConfig.boxBlurRadius, 0, 128, "%d", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (reRender)
                        effectData = make_unique<EffectBoxBlur>(adjEffConfig.boxBlurRadius);
                    break;
                case Effects::DirectionalBlur:
                    if (ImGui::SliderInt("Radius"_C, &adjEffConfig.dirBlurRadius, 0, 128, "%d", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::SliderAngle("Angle"_C, &adjEffConfig.dirBlurAngle))
                        reRender = true;
                    if (reRender)
                        effectData = make_unique<EffectDirectionalBlur>(adjEffConfig.dirBlurRadius, adjEffConfig.dirBlurAngle);
                    break;
                case Effects::WhiteNoise:
                    if (ImGui::SliderFloat("Intensity"_C, &adjEffConfig.noiseIntensity, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::SliderFloat("Saturation"_C, &adjEffConfig.noiseSaturation, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::SliderFloat("Frequency"_C, &adjEffConfig.noiseFrequency, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::InputInt("Seed"_C, &adjEffConfig.noiseSeed))
                        reRender = true;
                    if (reRender)
                    {
                        effectData = make_unique<EffectWhiteNoise>(adjEffConfig.noiseIntensity, adjEffConfig.noiseSaturation, adjEffConfig.noiseFrequency, adjEffConfig.noiseSeed);
                    }
                    break;
                case Effects::FractalNoise:
                    if (ImGui::SliderInt("Octaves"_C, &adjEffConfig.fractalOctaves, 1, maxOctaves, "%d", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::SliderFloat("Smoothness"_C, &adjEffConfig.fractalSmoothness, 0.01f, 2, "%.2f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::InputInt("Seed"_C, &adjEffConfig.fractalSeed))
                        reRender = true;
                    if (ImGui::ColorEdit3("Primary color"_C, &adjEffConfig.fractalColor1.r))
                        reRender = true;
                    if (ImGui::ColorEdit3("Secondary color"_C, &adjEffConfig.fractalColor2.r))
                        reRender = true;
                    if (reRender)
                    {
                        effectData = make_unique<EffectFractalNoise>(adjEffConfig.fractalOctaves, adjEffConfig.fractalSmoothness, adjEffConfig.fractalSeed);
                        auto& effect = *reinterpret_cast<EffectFractalNoise*>(effectData.get());
                        effect.size = _imageEditor.at(activeImageEditor)->getSize();
                        effect.color1 = adjEffConfig.fractalColor1;
                        effect.color2 = adjEffConfig.fractalColor2;
                        std::mt19937 generator(adjEffConfig.fractalSeed);
                        effect.values.resize(_imageEditor.at(activeImageEditor)->getSize().x);
                        for (int32_t i = 0; i < effect.values.size(); i++)
                        {
                            effect.values.at(i).resize(_imageEditor.at(activeImageEditor)->getSize().y);
                            for (int32_t j = 0; j < effect.values.at(i).size(); j++)
                                effect.values.at(i).at(j) = static_cast<float>(generator()) / static_cast<float>(generator.max());
                        }
                    }
                    break;
                case Effects::Vignette:
                    if (ImGui::SliderFloat("Intensity"_C, &adjEffConfig.vignetteIntensity, 0.001f, 10, "%.3f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
                        reRender = true;
                    if (ImGui::SliderFloat("Falloff factor"_C, &adjEffConfig.vignetteFalloff, 0.01f, 5, "%.2f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::ColorEdit3("Outer color"_C, &adjEffConfig.vignetteColor.r))
                        reRender = true;
                    if (reRender)
                    {
                        effectData = make_unique<EffectVignette>(adjEffConfig.vignetteIntensity, adjEffConfig.vignetteFalloff);
                        auto& effect = *reinterpret_cast<EffectVignette*>(effectData.get());
                        effect.size = _imageEditor.at(activeImageEditor)->getSize();
                        effect.color = adjEffConfig.vignetteColor;
                    }
                    break;
                case Effects::Mandelbrot:
                    if (ImGui::SliderFloat2("Offset"_C, &adjEffConfig.mandelbrotOffset.x, -1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::SliderFloat("Zoom"_C, &adjEffConfig.mandelbrotZoom, 0.001f, 10, "%.3f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
                        reRender = true;
                    if (ImGui::SliderInt("Iterations"_C, &adjEffConfig.mandelbrotIterations, 1, 1000, "%d", ImGuiSliderFlags_Logarithmic))
                        reRender = true;
                    if (ImGui::ColorEdit3("Center color"_C, &adjEffConfig.mandelbrotCenterColor.r))
                        reRender = true;
                    if (ImGui::ColorEdit3(("Outer color"_S + " 1").c_str(), &adjEffConfig.mandelbrotOuterColor1.r))
                        reRender = true;
                    if (ImGui::ColorEdit3(("Outer color"_S + " 2").c_str(), &adjEffConfig.mandelbrotOuterColor2.r))
                        reRender = true;
                    if (reRender)
                    {
                        effectData = make_unique<EffectMandelbrot>(adjEffConfig.mandelbrotIterations, adjEffConfig.mandelbrotOffset, adjEffConfig.mandelbrotZoom);
                        auto& effect = *reinterpret_cast<EffectMandelbrot*>(effectData.get());
                        effect.size = _imageEditor.at(activeImageEditor)->getSize();
                        effect.color1 = adjEffConfig.mandelbrotOuterColor1;
                        effect.color2 = adjEffConfig.mandelbrotOuterColor2;
                        effect.color3 = adjEffConfig.mandelbrotCenterColor;
                    }
                    break;
                case Effects::Sharpening:
                    if (ImGui::SliderFloat("Intensity"_C, &adjEffConfig.sharpeningIntensity, 0, 20, "%.1f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::SliderInt("Radius"_C, &adjEffConfig.sharpeningRadius, 0, 10, "%d", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::SliderFloat("Threshold"_C, &adjEffConfig.sharpeningThreshold, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
                        reRender = true;
                    if (reRender)
                    {
                        effectData = make_unique<EffectSharpening>(adjEffConfig.sharpeningIntensity, adjEffConfig.sharpeningRadius, adjEffConfig.sharpeningThreshold);
                        auto& effect = *reinterpret_cast<EffectSharpening*>(effectData.get());
                        effect.values.resize(adjEffConfig.sharpeningRadius * 2 + 1);
                        for (int32_t i = 0; i <= effect.values.size() / 2; i++)
                            effect.values.at(i) = effect.values.at(effect.values.size() - i - 1) = Binomial(effect.values.size() - 1, i);
                    }
                }
                if (reRender)
                    AddWork(CanvasWork::Effect{targetEffect, _layerPicker.getLayerIDSelected(activeImageEditor), std::move(effectData)});
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
                _imageEditor.at(activeImageEditor)->unsavedChanges = true;
                AddWorkAndWait(CanvasWork::FinishAdjustmentOrEffect{_layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
                AddWorkAndWait(CanvasWork::CancelAdjustmentOrEffect{});
            }
            break;
        case PopUpState::Adjustment:
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                if (setWindowFocus)
                {
                    ImGui::SetWindowFocus();
                    setWindowFocus = false;
                }
                switch (targetAdjustment)
                {
                case Adjustments::BlackAndWhite: case Adjustments::Invert:
                    popUpState.pop_back();
                    AddWork(CanvasWork::Adjustment{targetAdjustment, _layerPicker.getLayerIDSelected(activeImageEditor), nullptr});
                    AddWorkAndWait(CanvasWork::FinishAdjustmentOrEffect{_layerPicker.getLayerIDSelected(activeImageEditor)});
                    reRender = false;
                    break;
                case Adjustments::BrightnessContrast:
                    if (ImGui::SliderFloat("Brightness"_C, &adjEffConfig.brightness, -1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::SliderFloat("Contrast"_C, &adjEffConfig.contrast, -1, 1, "%.4f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
                        reRender = true;
                    if (reRender)
                        adjustmentData = make_unique<AdjustBrightnessContrast>(adjEffConfig.brightness, adjEffConfig.contrast);
                    break;
                case Adjustments::HSV:
                    if (ImGui::SliderFloat("Hue"_C, &adjEffConfig.hue, 0, 360, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::SliderFloat("Saturation"_C, &adjEffConfig.saturation, -1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::SliderFloat("Value"_C, &adjEffConfig.value, -1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (reRender)
                        adjustmentData = make_unique<AdjustHSV>(std::fmod(adjEffConfig.hue, 360.f), adjEffConfig.saturation, adjEffConfig.value);
                    break;
                case Adjustments::Tint:
                    if (ImGui::SliderFloat("Red"_C, &adjEffConfig.tintRed, -1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::SliderFloat("Green"_C, &adjEffConfig.tintGreen, -1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (ImGui::SliderFloat("Blue"_C, &adjEffConfig.tintBlue, -1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                        reRender = true;
                    if (reRender)
                        adjustmentData = make_unique<AdjustTint>(adjEffConfig.tintRed, adjEffConfig.tintGreen, adjEffConfig.tintBlue);
                    break;
                default: break;
                }
                if (reRender)
                    AddWork(CanvasWork::Adjustment{targetAdjustment, _layerPicker.getLayerIDSelected(activeImageEditor), std::move(adjustmentData)});
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
                _imageEditor.at(activeImageEditor)->unsavedChanges = true;
                AddWorkAndWait(CanvasWork::FinishAdjustmentOrEffect{_layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
                AddWorkAndWait(CanvasWork::CancelAdjustmentOrEffect{});
            }
            break;
        case PopUpState::SaveBeforeClose:
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                ImGui::Text("%s", "glxyWarning2"_C);
                lock_guard lock(_imageEditor.at(editorCloseAttempt)->common.mtxEditorWorkerCommon);
                ImGui::BulletText("%s", _imageEditor.at(editorCloseAttempt)->getImageName().c_str());
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
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                for (auto& n : _imageEditor)
                {
                    lock_guard lock(n->common.mtxEditorWorkerCommon);
                    if (n->unsavedChanges)
                        ImGui::BulletText("%s", n->getImageName().c_str());
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
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                if (setWindowFocus)
                {
                    ImGui::SetWindowFocus();
                    setWindowFocus = false;
                }
                if (ImGui::SliderFloat2("Origin"_C, &transformImageOrigin.x, -1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                    reRender = true;
                if (ImGui::SliderFloat2("Position"_C, &transformImagePosition.x, -1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                    reRender = true;
                if (ImGui::SliderFloat("Rotation"_C, &transformImageRotation, -180, 180, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                    reRender = true;
                if (ImGui::SliderFloat2("Scale"_C, &transformImageScale.x, 0.001f, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
                    reRender = true;
                if (ImGui::BeginCombo("Sampling method"_C, config.transformSamplingSmooth ? "Linear"_C : "Nearest"_C))
                {
                    if (ImGui::Selectable("Nearest"_C))
                    {
                        config.transformSamplingSmooth = false;
                        reRender = true;
                    }
                    if (ImGui::Selectable("Linear"_C))
                    {
                        config.transformSamplingSmooth = true;
                        reRender = true;
                    }
                    ImGui::EndCombo();
                }
                ImGui::BeginDisabled(!_imageEditor.at(activeImageEditor)->chunkManager.getFinalSelectionBounds().expired() ||
                    _imageEditor.at(activeImageEditor)->getSize().x > c_maxChunkWorkableSize ||
                    _imageEditor.at(activeImageEditor)->getSize().y > c_maxChunkWorkableSize);
                if (ImGui::SliderInt("Repeat"_C, &transformImageRepeat, 1, 30000, "%d", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
                    reRender = true;
                ImGui::EndDisabled();

                if (reRender)
                {
                    int16_t repeat = transformImageRepeat;
                    if (!_imageEditor.at(activeImageEditor)->chunkManager.getFinalSelectionBounds().expired() ||
                        _imageEditor.at(activeImageEditor)->getSize().x > c_maxChunkWorkableSize ||
                        _imageEditor.at(activeImageEditor)->getSize().y > c_maxChunkWorkableSize)
                        repeat = 1;
                    AddWork(CanvasWork::TransformImage{transformImagePosition, transformImageRotation, transformImageScale, transformImageOrigin, repeat, config.transformSamplingSmooth});
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
                _imageEditor.at(activeImageEditor)->unsavedChanges = true;
                AddWorkAndWait(CanvasWork::Finish{});
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
                AddWorkAndWait(CanvasWork::Cancel{});
            }
            break;
        case PopUpState::Resize:
        {
            static bool byPercentage = false;
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                if (setWindowFocus)
                {
                    ImGui::SetWindowFocus();
                    setWindowFocus = false;
                }
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
                if (ImGui::BeginCombo("Resampling method"_C, LL::ind("resamplingMethod[]", config.resamplingMethod).c_str()))
                {
                    for (uint8_t i = 0; i < c_resamplingMethodCnt; i++)
                        if (ImGui::Selectable(LL::ind("resamplingMethod[]", i).c_str()))
                        {
                            config.resamplingMethod = i;
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
                    imageNewSize.x = std::max(imageNewSize.x, 1);
                    imageNewSize.y = std::max(imageNewSize.y, 1);
                }
                ImGui::EndDisabled();
                ImGui::Unindent();

                if (ImGui::RadioButton("By absolute size"_C, !byPercentage))
                    byPercentage = false;

                ImGui::Indent();
                ImGui::BeginDisabled(byPercentage);
                ImGui::Checkbox("Maintain aspect ratio"_C, &config.maintainAspectResize);
                if (ImGui::InputInt("Width"_C, &imageNewSize.x, 1, 10))
                {
                    imageNewSize.x = std::max(imageNewSize.x, 1);
                    if (config.maintainAspectResize)
                        imageNewSize.y = _imageEditor.at(activeImageEditor)->getSize().y * (static_cast<float>(imageNewSize.x) / _imageEditor.at(activeImageEditor)->getSize().x);
                }
                if (ImGui::InputInt("Height"_C, &imageNewSize.y, 1, 10))
                {
                    imageNewSize.y = std::max(imageNewSize.y, 1);
                    if (config.maintainAspectResize)
                        imageNewSize.x = _imageEditor.at(activeImageEditor)->getSize().x * (static_cast<float>(imageNewSize.y) / _imageEditor.at(activeImageEditor)->getSize().y);
                }
                ImGui::EndDisabled();
                ImGui::Unindent();
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
                auto& editor = _imageEditor.at(activeImageEditor);
                AddWorkAndWait(CanvasWork::RescaleCanvas{Vector2u(imageNewSize), static_cast<RescaleMethod>(config.resamplingMethod)});
                editor->ClampView();
                editor->unsavedChanges = true;
                editor->gridLines.manualChange = true;
                editor->rulerUI.manualChange = true;
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
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                if (setWindowFocus)
                {
                    ImGui::SetWindowFocus();
                    setWindowFocus = false;
                }
                ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
                if (ImGui::RadioButton("By percentage"_C, byPercentage))
                    byPercentage = true;

                ImGui::Indent();
                ImGui::BeginDisabled(!byPercentage);
                ImGui::SetNextItemWidth(150 * config.GUIScale);
                if (ImGui::InputInt("Percentage"_C, &scalePercentage, 1, 10))
                {
                    scalePercentage = std::clamp(scalePercentage, 1, 10000);
                    imageNewSize = Vector2i(_imageEditor.at(activeImageEditor)->getSize()) * scalePercentage / 100;
                    imageNewSize.x = std::max(imageNewSize.x, 1);
                    imageNewSize.y = std::max(imageNewSize.y, 1);
                }
                ImGui::EndDisabled();
                ImGui::Unindent();

                if (ImGui::RadioButton("By absolute size"_C, !byPercentage))
                    byPercentage = false;

                ImGui::Indent();
                ImGui::BeginDisabled(byPercentage);
                ImGui::Checkbox("Maintain aspect ratio"_C, &config.maintainAspectCanvas);
                ImGui::SetNextItemWidth(150 * config.GUIScale);
                if (ImGui::InputInt("Width"_C, &imageNewSize.x, 1, 10))
                {
                    imageNewSize.x = std::max(imageNewSize.x, 1);
                    if (config.maintainAspectCanvas)
                        imageNewSize.y = _imageEditor.at(activeImageEditor)->getSize().y * (static_cast<float>(imageNewSize.x) / _imageEditor.at(activeImageEditor)->getSize().x);
                }
                ImGui::SetNextItemWidth(150 * config.GUIScale);
                if (ImGui::InputInt("Height"_C, &imageNewSize.y, 1, 10))
                {
                    imageNewSize.y = std::max(imageNewSize.y, 1);
                    if (config.maintainAspectCanvas)
                        imageNewSize.x = _imageEditor.at(activeImageEditor)->getSize().x * (static_cast<float>(imageNewSize.y) / _imageEditor.at(activeImageEditor)->getSize().y);
                }
                ImGui::EndDisabled();
                ImGui::Unindent();
                ImGui::SetCursorPos(Vector2f(320, 50) * config.GUIScale);
                for (int8_t i = 0; i < 3; i++)
                {
                    for (int8_t j = 0; j < 3; j++)
                    {
                        const Vector2i offset = Vector2i(j, i) - Vector2i(static_cast<int8_t>(canvasResizePivot) % 3, static_cast<int8_t>(canvasResizePivot) / 3);
                        int8_t index = 0;
                        if (offset.x >= -1 && offset.x <= 1 && offset.y >= -1 && offset.y <= 1)
                            index = (offset.x + 1) + (offset.y + 1) * 3 + 1;
                        const float texOffset = index * 0.1f;
                        if (ImGui::ImageButton(("Button" + to_string(i) + to_string(j)).c_str(),
                            actionIcons.getNativeHandle(), Vector2f(25, 25) * config.GUIScale,
                            Vector2f(texOffset, 0), Vector2f(texOffset + 0.1f, 0.25f)))
                        {
                            canvasResizePivot = static_cast<Pivot>(j + i * 3);
                        }
                        if (j < 2)
                            ImGui::SameLine();
                        else
                            ImGui::SetCursorPosX(320 * config.GUIScale);
                    }
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                popUpState.pop_back();
                auto& editor = _imageEditor.at(activeImageEditor);
                AddWorkAndWait(CanvasWork::ResizeCanvas{Vector2u(imageNewSize), canvasResizePivot});
                editor->ClampView();
                editor->unsavedChanges = true;
                editor->gridLines.manualChange = true;
                editor->rulerUI.manualChange = true;
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
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                if (setWindowFocus)
                {
                    ImGui::SetWindowFocus();
                    setWindowFocus = false;
                }
                const LayerID layerID = _layerPicker.getLayerIDSelected(activeImageEditor);
                const auto& layer = _layerPicker.getLayer(activeImageEditor, layerID);
                string data = layer.name;
                if (ImGui::InputText("Layer name"_C, data.data(), data.capacity() + 1, ImGuiInputTextFlags_CallbackResize, TextCallback, &data))
                    _layerPicker.setLayerName(activeImageEditor, layerID, data);
                bool enabled = layer.enabled;
                if (ImGui::Checkbox("Enabled"_C, &enabled))
                {
                    _layerPicker.setLayerEnabled(activeImageEditor, layerID, enabled);
                    _imageEditor.at(activeImageEditor)->unsavedChanges = true;
                    AddWork(CanvasWork::LayerPropertyChanged{layerID, layer.enabled, layer.transparency, layer.blendMode});
                }
                int32_t transparency = layer.transparency;
                if (ImGui::SliderInt("Transparency"_C, &transparency, 0, 255, "%d", ImGuiSliderFlags_AlwaysClamp))
                {
                    _layerPicker.setLayerTransparency(activeImageEditor, layerID, transparency);
                    _imageEditor.at(activeImageEditor)->unsavedChanges = true;
                    AddWork(CanvasWork::LayerPropertyChanged{layerID, layer.enabled, layer.transparency, layer.blendMode});
                }
                uint8_t blendMode = layer.blendMode;
                if (ImGui::BeginCombo("Blend mode"_C, LL::ind("blendModeName[]", blendMode).c_str()))
                {
                    for (uint8_t i = 0; i < c_blendModes.size(); i++)
                        if (ImGui::Selectable(LL::ind("blendModeName[]", i).c_str()))
                        {
                            _layerPicker.setLayerBlendMode(activeImageEditor, layerID, i);
                            _imageEditor.at(activeImageEditor)->unsavedChanges = true;
                            AddWork(CanvasWork::LayerPropertyChanged{layerID, layer.enabled, layer.transparency, layer.blendMode});
                        }
                    ImGui::EndCombo();
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
                popUpState.pop_back();
            break;
        }
        case PopUpState::Open:
        {
            bool fileDoubleClicked = false;
            if (ImGui::BeginChild("FileExplorer", Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 30 * config.GUIScale)))
                fileDoubleClicked = fileExplorer.Widget(ExplorerFlags::Open);
            ImGui::EndChild();

            const filesystem::path filePath = fileExplorer.getFileOpenPath() / fileExplorer.getFilenameOpen();
            ImGui::BeginDisabled(filePath.empty() || !filePath.empty() && (!filesystem::exists(filePath) || filesystem::is_directory(filePath)));
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !ImGui::GetIO().WantTextInput || fileDoubleClicked)
                OpenImage(filePath);
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !ImGui::GetIO().WantTextInput)
                popUpState.pop_back();
            break;
        }
        case PopUpState::Save:
        {
            bool fileDoubleClicked = false;
            if (ImGui::BeginChild("FileExplorer", Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 30 * config.GUIScale - ImGui::GetTextLineHeightWithSpacing())))
                fileDoubleClicked = fileExplorer.Widget(ExplorerFlags::Save);
            ImGui::EndChild();
            const filesystem::path filePath = fileExplorer.getFileSavePath() / fileExplorer.getFilenameSave();
            string path = filePath.string() + fileExplorer.getExtensionUsed();
            if (filePath.has_extension())
            {
                string extension = filePath.extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
                if (extension == fileExplorer.getExtensionUsed())
                    path = filePath.string();
            }
            if (filesystem::exists(path))
                ImGui::TextColored(Color(255, 200, 0), "%s", "glxyWarning1"_C);
            else
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetTextLineHeightWithSpacing());
            ImGui::BeginDisabled(fileExplorer.getFilenameSave().empty());
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !ImGui::GetIO().WantTextInput || fileDoubleClicked)
            {
                auto& ie = _imageEditor.at(activeImageEditor);
                ie->imagePath = filePath.string() + fileExplorer.getExtensionUsed();
                if (filePath.has_extension())
                {
                    string extension = filePath.extension().string();
                    std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
                    if (extension == fileExplorer.getExtensionUsed())
                        ie->imagePath = filePath.string();
                }
                popUpState.pop_back();
                if (fileExplorer.getExtensionUsed() == ".jpg")
                    popUpState.push_back(PopUpState::SaveAdditional);
                else
                    ie->Save();
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancel"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Escape) && !ImGui::GetIO().WantTextInput)
            {
                if (editorCloseAttempt >= 0)
                    editorCloseAttempt = -1;
                popUpState.pop_back();
            }
            break;
        }
        break;
        case PopUpState::New:
        {
            static int32_t background = 0;
            static int32_t imageType = 0;
            const array colors = {
                Color::White,
                Color::Black,
                Color::Transparent,
            };
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                if (setWindowFocus)
                {
                    ImGui::SetWindowFocus();
                    setWindowFocus = false;
                }
                ImGui::RadioButton("Fixed size"_C, &imageType, 0);
                ImGui::BeginDisabled(imageType != 0);
                ImGui::Indent();
                ImGui::DragInt2("Resolution"_C, &resolution.x, 1, 1, 1e5, "%d", ImGuiSliderFlags_AlwaysClamp);
                ImGui::Unindent();
                ImGui::EndDisabled();
                ImGui::RadioButton("Infinite"_C, &imageType, 1);
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Text("%s:", "Background"_C);
                ImGui::Indent();
                ImGui::RadioButton("White"_C, &background, 0);
                ImGui::RadioButton("Black"_C, &background, 1);
                ImGui::BeginDisabled(imageType != 0);
                ImGui::RadioButton("Transparent"_C, &background, 2);
                ImGui::EndDisabled();
                ImGui::Unindent();
                if (imageType == 0)
                {
                    const float count = static_cast<float>(resolution.x) * resolution.y;
                    if (count > 3e5f)
                        ImGui::Text("%s: %.1fM, %s: %.1f MB", "Total pixels"_C, count / 1e6f, "Size"_C, count * 4 / 1e6f);
                    else if (count > 3e2f)
                        ImGui::Text("%s: %.1fK, %s: %.1f kB", "Total pixels"_C, count / 1e3f, "Size"_C, count * 4 / 1e3f);
                    else
                        ImGui::Text("%s: %.0f, %s: %.0f B", "Total pixels"_C, count, "Size"_C, count * 4);
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            {
                if (imageType == 1 && background == 2)
                    background = 0;
                CreateEmptyImage(Vector2u(resolution), imageType, colors.at(background));
                popUpState.push_back(PopUpState::ThreadWork);
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
                    OpenImage(openWithGalaxyFile);
                openWithGalaxyFile = "";
            }
            const float width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
            const float height = (ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y) * 0.5f;
            for (int8_t i = 0; i < 4; i++)
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, Color(255, 255, 255, 32));
                ImGui::BeginChild(("MenuItem" + to_string(i)).c_str(),
                    Vector2f(width, height), false,
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
                    case 2: popUpState.push_back(PopUpState::Recent); break;
                    case 3: popUpState.push_back(PopUpState::Settings); break;
                    default: break;
                    }
                }
            }
        }
        break;
        case PopUpState::ThreadWork:
        {
            if (!workerTodoWork)
                ImGui::ProgressBar(1, ImGui::GetContentRegionAvail());
            else
            {
                const float value = 1 - CanvasWorker::getWorkAmount() / workerTodoWork;
                ImGui::ProgressBar(value, ImGui::GetContentRegionAvail());
            }

            if (!CanvasWorker::hasWork())
            {
                popUpState.pop_back();
                CanvasWorker::LockAddingNewWork(false);
                if (!popUpState.empty())
                {
                    switch (popUpState.back())
                    {
                    case PopUpState::New:
                        popUpState.pop_back();
                        if (!popUpState.empty() && popUpState.back() == PopUpState::Setup)
                            popUpState.pop_back();
                        _imageEditor.at(activeImageEditor)->FinishCreation();
                        break;
                    case PopUpState::Open: case PopUpState::Recent:
                        popUpState.pop_back();
                        if (_imageEditor.at(activeImageEditor)->getSize() == Vector2u())
                            DeleteEditor(activeImageEditor);
                        else
                        {
                            _imageEditor.at(activeImageEditor)->FinishCreation();
                            if (!popUpState.empty() && popUpState.back() == PopUpState::Setup)
                                popUpState.pop_back();
                        }
                        break;
                    case PopUpState::Save:
                        if (editorCloseAttempt >= 0)
                        {
                            DeleteEditor(editorCloseAttempt);
                            editorCloseAttempt = -1;
                        }
                        popUpState.pop_back();
                        break;
                    case PopUpState::ToolChanged:
                        popUpState.pop_back();
                        _imageEditor.at(activeImageEditor)->currentTool = changeToTool;
                        _imageEditor.at(activeImageEditor)->forceToolChange = true;
                        break;
                    default:
                        break;
                    }
                }
            }
            break;
        }
        case PopUpState::GLScan:
        {
            static bool firstScan = true;
            static Clock timeToPrint;
            static vector<string> GLData;
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
                                GLData.push_back(y);
                                y.clear();
                                continue;
                            }
                            y += x.at(j);
                        }
                        GLData.push_back("Version"_S + ": " + reinterpret_cast<const char*>(glGetString(GL_VERSION)));
                        GLData.push_back("Vendor"_S + ": " + reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
                        GLData.push_back("Renderer"_S + ": " + reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
                        GLData.push_back("Shaders available"_S + ": " + (Shader::isAvailable() ? "yes"_S : "no"_S));
                        GLData.push_back("Vertex buffer available"_S + ": " + (VertexBuffer::isAvailable() ? "yes"_S : "no"_S));
                        GLData.push_back("Vulkan available"_S + ": " + (Vulkan::isAvailable() ? "yes"_S : "no"_S));
                        break;
                    }
                    sleep(milliseconds(30));
                }
                firstScan = false;
            }

            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                for (int32_t i = 0; i < static_cast<int32_t>(GLData.size()) * std::min(1.f, timeToPrint.getElapsedTime().asSeconds()); i++)
                    ImGui::Text("%s", GLData.at(i).c_str());
                if (timeToPrint.getElapsedTime().asSeconds() < 1.5f)
                    ImGui::SetScrollHereY(1.f);
            }
            ImGui::EndChild();

            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
                popUpState.pop_back();
            ImGui::SameLine();

            if (ImGui::Button("Copy to clipboard"_C, Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)))
            {
                string sum;
                for (int16_t i = 0; i < GLData.size(); i++)
                {
                    sum += GLData.at(i);
                    sum += '\n';
                }
                Clipboard::setString(sum);
            }
        }
            break;
        case PopUpState::Changelog: case PopUpState::FuturePlan:
        {
            const string_view& t = popUpState.back() == PopUpState::Changelog ? c_changelog : c_futurePlan;
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
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
            static array bgColor = { config.bgColor.r / 255.f, config.bgColor.g / 255.f, config.bgColor.b / 255.f };
            static int8_t pressedID = -1;
            static bool editing = false;
            static int32_t fontSize = 16.f * config.GUIScale;
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
            {
                if (setWindowFocus)
                {
                    ImGui::SetWindowFocus();
                    setWindowFocus = false;
                }
                ImGui::SeparatorText("Graphics"_C);
                if (ImGui::BeginCombo("Resolution"_C, (to_string(config.resolution.x) + "x" + to_string(config.resolution.y)).c_str()))
                {
                    const std::vector<VideoMode>& res = VideoMode::getFullscreenModes();
                    for (int32_t i = 0; i < res.size(); i++)
                    {
                        if (res.at(i).size.x < 640 || res.at(i).size.y < 480)
                            continue;
                        if (find(res.begin(), res.begin() + i, VideoMode{res.at(i).size, 32U}) != res.begin() + i)
                            continue;
                        const string t = to_string(res.at(i).size.x) + "x" + to_string(res.at(i).size.y);
                        if (ImGui::Selectable(t.c_str()))
                            window.setSize(res.at(i).size);
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::Checkbox("Fullscreen window"_C, &config.fullscreen))
                    RecreateAppWindow();
                if (ImGui::Checkbox("Vertical sync"_C, &config.verticalSync))
                {
                    if (config.verticalSync)
                    {
                        window.setFramerateLimit(0);
                        window.setVerticalSyncEnabled(true);
                    }
                    else
                    {
                        window.setVerticalSyncEnabled(false);
                        window.setFramerateLimit(config.maxFPS);
                    }
                }
                ToolTip("glxyTooltip1"_S);

                ImGui::BeginDisabled(config.verticalSync);
                ImGui::InputInt("Max FPS"_C, &config.maxFPS, 0, 0);
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (config.maxFPS < 5)
                        config.maxFPS = 5;
                    else if (config.maxFPS > 1000)
                        config.maxFPS = 1000;
                    window.setFramerateLimit(config.maxFPS);
                }
                ToolTip("glxyTooltip2"_S);
                ImGui::EndDisabled();
                ImGui::SetNextItemWidth(250 * config.GUIScale);
                if (ImGui::BeginCombo("Viewport antialiasing"_C, ("x" + to_string(config.antialiasing)).c_str()))
                {
                    if (ImGui::Selectable("x0"))
                    {
                        config.antialiasing = 0;
                        for (auto& n : _imageEditor)
                            n->RecreateEditorTexture();
                    }
                    for (int8_t i = 1; i <= 8; i *= 2)
                        if (ImGui::Selectable(("x" + to_string(i)).c_str()))
                        {
                            config.antialiasing = i;
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
                ImGui::SetNextItemWidth(300 * config.GUIScale);
                ImGui::SliderInt("Font size"_C, &fontSize, 10, 28, "%d", ImGuiSliderFlags_AlwaysClamp);
                ImGui::SameLine();
                if (ImGui::Button("Apply"_C))
                {
                    config.GUIScale = fontSize / 16.f;
                    ApplyStyle();
                    resetPopupWindow = true;
                    changeFont = true;
                }
                if (ImGui::BeginCombo("Language"_C, c_languageNames.at(config.languageID).data()))
                {
                    for (uint8_t i = 0; i < c_languageCnt; i++)
                        if (ImGui::Selectable(c_languageNames.at(i).data()))
                        {
                            config.languageID = i;
                            LL::setLanguageID(config.languageID);
                        }
                    ImGui::EndCombo();
                }
                ImGui::Checkbox("Show framerate"_C, &config.showFPS);
                ImGui::Spacing();
                ImGui::SeparatorText("Editor"_C);
                ImGui::Checkbox("Freeze when out of focus"_C, &config.outOfFocus); ToolTip("glxyTooltip4"_S);
                ImGui::Checkbox("Draw inner selection lines"_C, &config.drawSelectionLines);
                ImGui::Checkbox("Show color picker in triangle style"_C, &config.colorPickerTriangle);
                ImGui::Checkbox("Enable touchpad support"_C, &config.touchPadSupport);
                ImGui::SetNextItemWidth(250 * config.GUIScale);
                ImGui::SliderInt("Thumbnail cache size limit"_C, &config.thumbnailCacheSizeLimit, 0, 100, "%d MB", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic); ToolTip("glxyTooltip5"_S);
                if (ImGui::InputText("Font location"_C, config.fontLocation.data(), config.fontLocation.capacity() + 1,
                    ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_EnterReturnsTrue, TextCallback, &config.fontLocation))
                {
                    LoadFonts();
                }
                if (ImGui::BeginCombo("Middle mouse button"_C, LL::ind("toolName[]", config.middleMouseButton).c_str()))
                {
                    if (ImGui::Selectable("None"_C))
                        config.middleMouseButton = -1;
                    for (int8_t i = 0; i < static_cast<uint8_t>(Tool::Count); i++)
                    {
                        if (ImGui::Selectable(LL::ind("toolName[]", i).c_str()))
                            config.middleMouseButton = i;
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::BeginCombo("Extra1 mouse button"_C, LL::ind("toolName[]", config.extra1MouseButton).c_str()))
                {
                    if (ImGui::Selectable("None"_C))
                        config.middleMouseButton = -1;
                    for (int8_t i = 0; i < static_cast<uint8_t>(Tool::Count); i++)
                    {
                        if (ImGui::Selectable(LL::ind("toolName[]", i).c_str()))
                            config.extra1MouseButton = i;
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::BeginCombo("Extra2 mouse button"_C, LL::ind("toolName[]", config.extra2MouseButton).c_str()))
                {
                    if (ImGui::Selectable("None"_C))
                        config.middleMouseButton = -1;
                    for (int8_t i = 0; i < static_cast<uint8_t>(Tool::Count); i++)
                    {
                        if (ImGui::Selectable(LL::ind("toolName[]", i).c_str()))
                            config.extra2MouseButton = i;
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::ColorEdit3("Background color"_C, bgColor.data(), ImGuiColorEditFlags_Uint8))
                {
                    config.bgColor.r = bgColor[0] * 255;
                    config.bgColor.g = bgColor[1] * 255;
                    config.bgColor.b = bgColor[2] * 255;
                }

                ImGui::Spacing();
                ImGui::SeparatorText("Animations"_C);
                ImGui::Checkbox("Animate zoom"_C, &config.animateZoom);
                ImGui::Checkbox("Animate pan"_C, &config.animatePan);
                ImGui::Spacing();
                ImGui::SeparatorText("Shortcuts"_C);

                for (int8_t i = 0; i < static_cast<int8_t>(ActionShortcut::Count); i++)
                {
                    const Vector2f def = ImGui::GetStyle().SelectableTextAlign;
                    ImGui::GetStyle().SelectableTextAlign = Vector2f(0.5, 0.5);
                    if (editing && pressedID == i)
                        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.f);
                    ImGui::Selectable((LL::ind("shortcut[]", i) + "##Select" + to_string(i)).c_str(), false, ImGuiSelectableFlags_Disabled, Vector2f(ImGui::GetContentRegionAvail().x / 3, 25.f * config.GUIScale));
                    if (editing && pressedID == i)
                        ImGui::PopStyleVar();
                    ImGui::SameLine();
                    if (ImGui::Button((Shortcuts::getName(static_cast<ActionShortcut>(i)) + "##Button" + to_string(i)).c_str(), Vector2f(ImGui::GetContentRegionAvail().x / 1.5f, 25.f * config.GUIScale)))
                    {
                        editing = true;
                        pressedID = i;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(("Clear"_S + "##" + to_string(i)).c_str(), Vector2f(ImGui::GetContentRegionAvail().x / 1.3f, 25.f * config.GUIScale)))
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
                config.Save();
                popUpState.pop_back();
            }
        }
        break;
        case PopUpState::About:
            if (ImGui::BeginChild("Scroll", Vector2f(0, -30 * config.GUIScale)))
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
                ImGui::TextWrapped("glxyAbout7"_C, c_AppVersion.c_str(), BUILDNUMBER, c_SFML_ARCH.data());
                ImGui::Indent();
                ImGui::TextWrapped("glxyAbout8"_C, sf::version().string.data());
                ImGui::TextWrapped("glxyAbout9"_C, ImGui::GetVersion());
                ImGui::Unindent();
                ImGui::TextWrapped("glxyAbout10"_C, GalaxyStartUpTime.asMilliseconds());
            }
            ImGui::EndChild();
            if (ImGui::Button("OK", Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)) || InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
                popUpState.pop_back();
            ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - windowLogoTexture.getSize().x / 3 * config.GUIScale);
            ImGui::SetCursorPosY(20 * config.GUIScale);
            ImGui::Image(windowLogoTexture, Vector2f(windowLogoTexture.getSize()) / 3.f * config.GUIScale);
            break;
        default:
            break;
        }
        ImGui::EndPopup();
    }
    if (popStyle)
        ImGui::PopStyleColor();
}