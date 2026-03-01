#include "App.hpp"
#include "Func.hpp"
#include "GlobalClock.hpp"
#include "Shortcuts.hpp"
#include "Global.hpp"
#include "Themes.hpp"
#include "InternalResource.hpp"
#include <set>

#ifdef SFML_SYSTEM_ANDROID
#include <SFML/System/NativeActivity.hpp>
#include <android/native_activity.h>
#endif

glxy::App::App()
    : _colorPicker(window, settings.GUIScale, settings.showRuler),
    _toolPicker(settings.GUIScale, settings.showRuler, toolIcons),
    _layerPicker(window, settings.GUIScale, activeImageEditor, layerIcons)
{
    //load languages
    LL::load(InternalResource::getResource(ID_RES2, "BINARY"));

}

glxy::App::~App()
{
    if (filesystem::exists("pdo.pdo"))
        filesystem::remove("pdo.pdo");
}

void glxy::App::Start(const filesystem::path& filename, const bool openWithGalaxy, const Clock& startUpTimer)
{
    openWithGalaxyFile = filename;
    this->openWithGalaxy = openWithGalaxy;
    GLOBAL.themeID = 0;

    settings.Load();
    {
        auto data = InternalResource::getResource(ID_RES3, "BINARY");
        Image temp, temp2;
        if (!temp.loadFromMemory(data.data(), data.size())) return;
        validate(windowLogoTexture.loadFromImage(temp));
        windowLogoTexture.setSmooth(true);
#ifdef SFML_SYSTEM_WINDOWS
        windowLogo.resize(Vector2u(400, 400), Color::Transparent);
#else
        windowLogo.resize(Vector2u(512, 512), Color::Transparent);
#endif
        validate(windowLogo.copy(temp, Vector2u((windowLogo.getSize().x - temp.getSize().x) / 2,
            (windowLogo.getSize().y - temp.getSize().y) / 2)));

        data = InternalResource::getResource(ID_RES4, "BINARY");
        if (!toolIcons.loadFromMemory(data.data(), data.size())) return;
        toolIcons.setSmooth(true);

        data = InternalResource::getResource(ID_RES5, "BINARY");
        if (!canvasIcons.loadFromMemory(data.data(), data.size())) return;
        canvasIcons.setSmooth(true);

        data = InternalResource::getResource(ID_RES6, "BINARY");
        if (!setupSelect.loadFromMemory(data.data(), data.size())) return;
        setupSelect.setSmooth(true);

        data = InternalResource::getResource(ID_RES7, "BINARY");
        if (!gizmoIcons.loadFromMemory(data.data(), data.size())) return;
        gizmoIcons.setSmooth(true);

        data = InternalResource::getResource(ID_RES8, "BINARY");
        if (!temp.loadFromMemory(data.data(), data.size())) return;
        temp2.resize(Vector2u((temp.getSize().y + 2) * 7, temp.getSize().y + 2), Color::Transparent);
        for (int8_t i = 0; i < 7; i++)
            validate(temp2.copy(temp, Vector2u(1 + (temp.getSize().y + 2) * i, 1),
                IntRect(Vector2i(temp.getSize().y * i, 0), Vector2i(temp.getSize().y, temp.getSize().y))));
        validate(layerIcons.loadFromImage(temp2));
        layerIcons.setSmooth(true);

    }

    RecreateAppWindow();

    window.clear();
    window.display();

    validate(ImGui::SFML::Init(window, false));
    settings.fontSize = settings.GUIScale * 16.f;
    mainFontData = InternalResource::getResource(ID_RES1, "BINARY");
    if (!mainFont.openFromMemory(mainFontData.data(), mainFontData.size()))
        return;

#ifdef SFML_DESKTOP
    cursor = make_unique<Cursor>(Cursor::Type::Arrow);
    window.setMouseCursor(*cursor);
#endif

    cursorType = Cursor::Type::Arrow;

    //keep font memory
    ImFontConfig fc;
    fc.FontDataOwnedByAtlas = false;
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(&mainFontData[0], mainFontData.size(), settings.fontSize, &fc);

    validate(ImGui::SFML::UpdateFontTexture());
    switch (GLOBAL.themeID)
    {
    case 0: StyleColorsThemeDefault(); break;
    case 1: StyleColorsThemeRed(); break;
    case 2: StyleColorsThemeMidnight(); break;
    case 3: ImGui::StyleColorsDark(); break;
    default: break;
    }
    ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;
    ImGui::GetStyle().WindowTitleAlign = Vector2f(0.005f, 0.5f);
    ImGui::GetStyle().WindowMinSize = Vector2f(150, 75);
    ImGui::GetStyle().FramePadding = Vector2f(6, 4);
    ImGui::GetStyle().FrameRounding = 3.f;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = "windows.ini";

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    this->startUpTimer = &startUpTimer;
    app();

    settings.Save();
    ImGui::SFML::Shutdown();
}

void glxy::App::RecreateAppWindow()
{
#ifdef SFML_SYSTEM_MACOS
    const int8_t GLTarget = 21;
#elif defined(SFML_SYSTEM_ANDROID)
    const int8_t GLTarget = 31;
#else
    const int8_t GLTarget = 42;
#endif

    window.create(VideoMode({ settings.resolution.x, settings.resolution.y }), c_AppName, settings.fullscreen ? Style::None : Style::Default,
        settings.fullscreen ? State::Fullscreen : State::Windowed, { 0U, 0U, settings.antialiasing, GLTarget / 10U, GLTarget % 10U});

#ifdef SFML_SYSTEM_ANDROID
    sleep(milliseconds(50));
#endif

    window.setMinimumSize(Vector2u(800, 600));
    window.setIcon(Vector2u(windowLogo.getSize().x, windowLogo.getSize().y), windowLogo.getPixelsPtr());
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

void glxy::App::CreateEmptyImage(const Vector2u resolution, const Color color)
{
    static uint16_t name = 0;
    name++;
    _imageEditor.emplace_back(make_unique<ImageEditor>(settings, window, popUpState, cursor, cursorType, _colorPicker,
        !_imageEditor.empty() ? _imageEditor.back()->dockID : mainDockID, _toolPicker, _layerPicker, gizmoIcons, mainFont));
    setActiveEditor(_imageEditor.size() - 1);
    _layerPicker.createNewImage();
    _imageEditor.back()->Empty("Untitled " + to_string(name), resolution, color);
}

bool glxy::App::hasUnsavedImages() const
{
    for (auto& n : _imageEditor)
        if (n->unsavedChanges)
            return true;
    return false;
}

void glxy::App::ExitApp(bool windowClose)
{
    if (find(popUpState.begin(), popUpState.end(), PopUpState::Setup) != popUpState.end())
        window.close();
    if (popUpState.empty() || windowClose)
    {
        if (hasUnsavedImages())
            popUpState.push_back(PopUpState::SaveBeforeExit);
        else
            window.close();
    }
}

void glxy::App::DeleteEditor(const int32_t ID)
{
    _imageEditor.erase(_imageEditor.begin() + ID);
    _layerPicker.deleteImage(ID);
    if (activeImageEditor >= ID)
        activeImageEditor--;
    if (hoveredImageEditor >= ID)
        hoveredImageEditor--;
}

void glxy::App::AddRecentFile(const filesystem::path& file)
{
    for (auto& n : settings.recentFiles)
        if (n.second == file.string())
        {
            settings.recentFiles.erase(n);
            settings.recentFiles.emplace(time(nullptr), file.string());
            return;
        }
    while (settings.recentFiles.size() >= c_maxRecentFiles)
    {
        pair<int64_t, string> smallestID = pair(INT64_MAX, "");
        for (auto& n : settings.recentFiles)
        {
            if (n.first < smallestID.first)
                smallestID = n;
        }
        settings.recentFiles.erase(smallestID);
    }
    settings.recentFiles.emplace(time(nullptr), file.string());
}

void glxy::App::setCursorType(Cursor::Type cursorType)
{
#ifndef SFML_DESKTOP
    return;
#endif
    this->cursorType = cursorType;
    auto t = Cursor::createFromSystem(this->cursorType);
    if (t)
    {
        *cursor = std::move(*t);
        window.setMouseCursor(*cursor);
    }
}

void glxy::App::RescaleWindow(const Vector2u size)
{
    settings.resolution = size;
}

void glxy::App::TitleBar()
{
    static Clock clock;
    static Time dfp = seconds(1);
    ImGui::BeginMainMenuBar();

    ImGui::SetWindowPos(Vector2f(0, 0));
    ImGui::SetWindowSize(Vector2f(window.getSize().x, window.getSize().y / 18.f));
    if (ImGui::MenuItem("Galaxy") || Shortcuts()[ActionShortcut::OpenAbout])
    {
        popUpState.push_back(PopUpState::About);
    }
    if (ImGui::BeginMenu("File"_C))
    {
        const array<string, c_fileMenuCnt> shortcuts = {
            Shortcuts::getName(ActionShortcut::NewImage), Shortcuts::getName(ActionShortcut::OpenImage),
            Shortcuts::getName(ActionShortcut::SaveImage), Shortcuts::getName(ActionShortcut::SaveImageAs), ""
        };
        for (int8_t i = 0; i < c_fileMenuCnt; i++)
        {
            ImGui::BeginDisabled(activeImageEditor == -1 && (i == 2 || i == 3));
            if (ImGui::MenuItem(LL::ind("fileMenu[]", i).c_str(), shortcuts.at(i).c_str()))
            {
                switch (i)
                {
                case 0:
                    popUpState.push_back(PopUpState::New);
                    break;
                case 1:
                    popUpState.push_back(PopUpState::Open);
                    break;
                case 2:
                    SaveImage();
                    break;
                case 3:
                    if (activeImageEditor >= 0 && activeImageEditor < _imageEditor.size())
                        popUpState.push_back(PopUpState::Save);
                    break;
                case 4:
                    ExitApp(false);
                    break;
                default: break;
                }
            }
            ImGui::EndDisabled();
        }
        ImGui::EndMenu();
    }
    if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::NewImage])
        popUpState.push_back(PopUpState::New);
    if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::OpenImage])
        popUpState.push_back(PopUpState::Open);
    if (activeImageEditor >= 0)
    {
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::SaveImage])
            SaveImage();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::SaveImageAs])
            if (activeImageEditor < _imageEditor.size())
                popUpState.push_back(PopUpState::Save);
    }
    ImGui::BeginDisabled(activeImageEditor == -1);
    if (ImGui::BeginMenu("Edit"_C))
    {
        ImageEditor* active = activeImageEditor == -1 ? nullptr : _imageEditor.at(activeImageEditor).get();
        ImGui::BeginDisabled(!active || !active->chunkManager.hasSelectionLayer());
        if (ImGui::MenuItem("editMenu[0]"_C, Shortcuts::getName(ActionShortcut::Copy).c_str()))
        {
            clipboardImage = make_unique<Image>();
            active->OptionCopyToClipboard(*clipboardImage, clipboardLocation);
        }
        if (ImGui::MenuItem("editMenu[1]"_C, Shortcuts::getName(ActionShortcut::Cut).c_str()))
        {
            clipboardImage = make_unique<Image>();
            active->OptionCopyToClipboard(*clipboardImage, clipboardLocation);
            active->OptionDeleteSelected();
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!clipboardImage);
        if (ImGui::MenuItem("editMenu[2]"_C, Shortcuts::getName(ActionShortcut::Paste).c_str()))
        {
            active->OptionPasteFromClipboard(*clipboardImage, clipboardLocation);
        }
        ImGui::EndDisabled();

        if (ImGui::MenuItem("editMenu[3]"_C, Shortcuts::getName(ActionShortcut::SelectAll).c_str()))
        {
            active->currentTool = Tool::BoxSelect;
            active->OptionSelectAll();
        }

        ImGui::BeginDisabled(!active || !active->chunkManager.hasSelectionLayer());
        if (ImGui::MenuItem("editMenu[4]"_C, Shortcuts::getName(ActionShortcut::DeselectAll).c_str()))
        {
            active->OptionDeselectAll();
        }
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!active || !active->chunkManager.hasSelectionLayer());
        if (ImGui::MenuItem("editMenu[5]"_C, Shortcuts::getName(ActionShortcut::Delete).c_str()))
        {
            active->OptionDeleteSelected();
        }
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }
    ImGui::EndDisabled();
    if (activeImageEditor >= 0)
    {
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Copy] && _imageEditor.at(activeImageEditor)->chunkManager.hasSelectionLayer())
        {
            clipboardImage = make_unique<Image>();
            _imageEditor.at(activeImageEditor)->OptionCopyToClipboard(*clipboardImage, clipboardLocation);
        }
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Cut] && _imageEditor.at(activeImageEditor)->chunkManager.hasSelectionLayer())
        {
            clipboardImage = make_unique<Image>();
            _imageEditor.at(activeImageEditor)->OptionCopyToClipboard(*clipboardImage, clipboardLocation);
            _imageEditor.at(activeImageEditor)->OptionDeleteSelected();
        }
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Paste] && clipboardImage)
        {
            _imageEditor.at(activeImageEditor)->currentTool = Tool::MoveSelection;
            _imageEditor.at(activeImageEditor)->OptionPasteFromClipboard(*clipboardImage, clipboardLocation);
        }
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::SelectAll])
        {
            _imageEditor.at(activeImageEditor)->currentTool = Tool::BoxSelect;
            _imageEditor.at(activeImageEditor)->OptionSelectAll();
        }
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::DeselectAll] && _imageEditor.at(activeImageEditor)->chunkManager.hasSelectionLayer())
        {
            _imageEditor.at(activeImageEditor)->OptionDeselectAll();
        }
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Delete] && _imageEditor.at(activeImageEditor)->chunkManager.hasSelectionLayer())
            _imageEditor.at(activeImageEditor)->OptionDeleteSelected();
    }
    if (ImGui::MenuItem("Settings"_C) || Shortcuts()[ActionShortcut::OpenSettings])
    {
        popUpState.push_back(PopUpState::Settings);
    }
    ImGui::BeginDisabled(activeImageEditor == -1);
    if (ImGui::BeginMenu("View"_C))
    {
        ImageEditor* active = activeImageEditor == -1 ? nullptr : _imageEditor.at(activeImageEditor).get();
        if (ImGui::MenuItem("viewMenu[0]"_C, Shortcuts::getName(ActionShortcut::ZoomIn).c_str()))
            active->OptionZoomIn(false);
        if (ImGui::MenuItem("viewMenu[1]"_C, Shortcuts::getName(ActionShortcut::ZoomOut).c_str()))
            active->OptionZoomOut(false);
        if (ImGui::BeginMenu("viewMenu[2]"_C))
        {
            const array zoomScales = {
                1, 2, 5, 10, 25, 50, 75, 100, 150, 200, 250, 300, 400, 500, 1000, 2000, 5000
            };
            const float bestFitZoom = active ? active->getBestFitSize() : 0;
            if (ImGui::MenuItem(("Best fit"_S + " (" + to_string(static_cast<int32_t>(bestFitZoom * 100)) + "%)").c_str()))
                active->setNewView(Vector2f(active->getSize()) / 2.f - Vector2f(1, 1) * (c_rulerSize * settings.showRuler) * (0.5f / bestFitZoom), 1 / bestFitZoom);
            float bestPixelFit = zoomScales.back() / 100.f;
            for (int8_t i = 1; i < zoomScales.size(); i++)
                if (bestFitZoom < zoomScales.at(i) / 100.f)
                {
                    bestPixelFit = zoomScales.at(i - 1) / 100.f;
                    break;
                }
            if (ImGui::MenuItem(("Best pixel fit"_S + " (" + to_string(static_cast<int32_t>(bestPixelFit * 100)) + "%)").c_str()))
                active->setNewView(Vector2f(active->getSize()) / 2.f - Vector2f(1, 1) * (c_rulerSize * settings.showRuler) * (0.5f / bestPixelFit), 1 / bestPixelFit);
            for (int8_t i = 0; i < zoomScales.size(); i++)
            {
                if (ImGui::MenuItem((to_string(zoomScales.at(i)) + "%").c_str()))
                    active->setNewView(Vector2f(active->getSize()) / 2.f, 100.f / zoomScales.at(i));
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("viewMenu[3]"_C, nullptr, settings.showGrid))
        {
            settings.showGrid = !settings.showGrid;
            for (auto& n : _imageEditor)
                n->OptionGrid(settings.showGrid);
        }
        if (ImGui::MenuItem("viewMenu[4]"_C, nullptr, settings.showRuler))
        {
            settings.showRuler = !settings.showRuler;
            for (auto& n : _imageEditor)
                n->OptionRuler(settings.showRuler);
        }
        if (ImGui::MenuItem("viewMenu[5]"_C))
            active->OptionActualSize();
        if (ImGui::MenuItem("viewMenu[6]"_C, nullptr, settings.syncViewport))
            active->OptionSyncViewport();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Image"_C))
    {
        ImageEditor* active = activeImageEditor == -1 ? nullptr : _imageEditor.at(activeImageEditor).get();
        ImGui::BeginDisabled(!active || !active->chunkManager.hasSelectionLayer());
        if (ImGui::MenuItem("imageMenu[0]"_C))
            active->OptionCropSelection();
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("imageMenu[1]"_C, Shortcuts::getName(ActionShortcut::Resize).c_str()))
            popUpState.push_back(PopUpState::Resize);
        if (ImGui::MenuItem("imageMenu[2]"_C, Shortcuts::getName(ActionShortcut::ResizeCanvas).c_str()))
            popUpState.push_back(PopUpState::ResizeCanvas);
        ImGui::Separator();
        if (ImGui::MenuItem("imageMenu[3]"_C))
            active->OptionFlipImageHorizontal();
        if (ImGui::MenuItem("imageMenu[4]"_C))
            active->OptionFlipImageVertical();
        ImGui::Separator();
        if (ImGui::MenuItem("imageMenu[5]"_C))
            active->OptionRotate90CW();
        if (ImGui::MenuItem("imageMenu[6]"_C))
            active->OptionRotate90CCW();
        if (ImGui::MenuItem("imageMenu[7]"_C))
            active->OptionRotate180();
        ImGui::Separator();
        if (ImGui::MenuItem("imageMenu[8]"_C, Shortcuts::getName(ActionShortcut::TransformImage).c_str()))
        {
            popUpState.push_back(PopUpState::TransformImage);
            active->OptionSetupTransformImage();
        }
        ImGui::EndMenu();
    }
    if (activeImageEditor >= 0)
    {
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Resize])
            popUpState.push_back(PopUpState::Resize);
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::ResizeCanvas])
            popUpState.push_back(PopUpState::ResizeCanvas);
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::TransformImage])
        {
            popUpState.push_back(PopUpState::TransformImage);
            _imageEditor.at(activeImageEditor)->OptionSetupTransformImage();
        }
    }
    if (ImGui::BeginMenu("Layers"_C))
    {
        ImageEditor* active = activeImageEditor == -1 ? nullptr : _imageEditor.at(activeImageEditor).get();
        if (ImGui::MenuItem("layerMenu[0]"_C))
            active->OptionCreateLayer();
        ImGui::BeginDisabled(!active || _layerPicker.getLayerCount() == 1);
        if (ImGui::MenuItem("layerMenu[1]"_C))
            active->OptionDeleteLayer();
        ImGui::EndDisabled();
        if (ImGui::MenuItem("layerMenu[2]"_C))
            active->OptionDuplicateLayer();
        ImGui::Separator();
        ImGui::BeginDisabled(!active || _layerPicker.getLayerIDSelected() >= _layerPicker.getLayerCount() - 1);
        if (ImGui::MenuItem("layerMenu[3]"_C))
            active->OptionMoveLayerUp();
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!active || _layerPicker.getLayerIDSelected() <= 0);
        if (ImGui::MenuItem("layerMenu[4]"_C))
            active->OptionMoveLayerDown();
        if (ImGui::MenuItem("layerMenu[5]"_C))
            active->OptionMergeLayerDown();
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("layerMenu[6]"_C))
            active->OptionFlipLayerHorizontal();
        if (ImGui::MenuItem("layerMenu[7]"_C))
            active->OptionFlipLayerVertical();
        ImGui::Separator();
        if (ImGui::MenuItem("layerMenu[8]"_C))
        {
            popUpState.push_back(PopUpState::LayerProperties);
        }
        ImGui::EndMenu();
    }
    ImGui::EndDisabled();

    if (ImGui::BeginMenu("Help"_C))
    {
        for (int8_t i = 0; i < c_helpCnt; i++)
            if (ImGui::MenuItem(LL::ind("OtherType[]", i).c_str()))
            {
                switch (i)
                {
                case 0:
                    popUpState.push_back(PopUpState::Changelog);
                    break;
                case 1:
                    popUpState.push_back(PopUpState::FuturePlan);
                    break;
                case 2:
                    popUpState.push_back(PopUpState::GLScan);
                    break;
                case 3:
                    popUpState.push_back(PopUpState::About);
                    break;
                default: break;
                }
            }
        ImGui::EndMenu();
    }
    if (dfp < seconds(1))
        ImGui::Text("%s", (" " + "Saved"_S).c_str());
    if (settings.showFPS)
    {
        const string fps = " FPS: " + to_string(static_cast<int>(1.f / TimeControl::DeltaReal().asSeconds())) + "\t";
        dfp += clock.restart();
        ImGui::Text("%s", fps.c_str());
    }

    ImGui::EndMainMenuBar();
}

void glxy::App::SubTitleBar()
{
    ImGui::SetNextWindowPos(Vector2f(0, ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(Vector2f(ImGui::GetIO().DisplaySize.x, subTitleBarHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, Vector2f(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

    ImGui::Begin("SubTitleBar", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::PushItemWidth(150.f);
    if (ImGui::BeginCombo("Tool"_C, LL::ind("toolName[]", static_cast<uint8_t>(_toolPicker.getTool())).c_str()))
    {
        for (uint8_t i = 0; i < static_cast<uint8_t>(Tool::Count); i++)
        {
            if (ImGui::Selectable(LL::ind("toolName[]", i).c_str()))
            {
                _toolPicker.setTool(static_cast<Tool>(i));
            }
        }
        ImGui::EndCombo();
    }
    switch (_toolPicker.getTool())
    {
    case Tool::BoxSelect: case Tool::CircleSelect:
        ImGui::SameLine();
        ImGui::PushItemWidth(100.f);
        if (ImGui::BeginCombo("Select mode"_C, LL::ind("boxSelectMode[]", _toolPicker.selectMode).c_str()))
        {
            for (uint8_t i = 0; i < 3; i++)
            {
                if (ImGui::Selectable(LL::ind("boxSelectMode[]", i).c_str()))
                    _toolPicker.selectMode = i;
            }
            ImGui::EndCombo();
        }
        break;
    case Tool::Pan: case Tool::Zoom:
        break;
    case Tool::MoveSelection:
        ImGui::SameLine();
        ImGui::BeginDisabled(!_imageEditor.at(activeImageEditor)->moveSelection);
        if (ImGui::Button("Cancel"_C) || popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            _imageEditor.at(activeImageEditor)->OptionCancel();
        ImGui::SameLine();
        if (ImGui::Button("Finish"_C) || popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            _imageEditor.at(activeImageEditor)->OptionFinish();
        ImGui::EndDisabled();
        break;
    case Tool::MagicWand:
    {
        ImGui::SameLine();
        ImGui::PushItemWidth(200.f);
        int32_t tolerance = settings.wandTolerance;
        if (ImGui::SliderInt("Tolerance"_C, &tolerance, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp))
        {
            settings.wandTolerance = tolerance;
            auto& target = _imageEditor.at(activeImageEditor);
            if (target->wandFill)
                target->pixelSelect.wandSelect(Vector2u(target->wandFillPosition), tolerance);
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!_imageEditor.at(activeImageEditor)->wandFill);
        if (ImGui::Button("Cancel"_C) || popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            _imageEditor.at(activeImageEditor)->OptionCancel();
        ImGui::SameLine();
        if (ImGui::Button("Finish"_C) || popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            _imageEditor.at(activeImageEditor)->OptionFinish();
        ImGui::EndDisabled();
        break;
    }
    case Tool::Brush:
        ImGui::SameLine();
        ImGui::PushItemWidth(50.f);
        ImGui::InputFloat("Radius"_C, &_toolPicker.brushRadius, 0, 0, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            _toolPicker.brushRadius = std::clamp(_toolPicker.brushRadius, 1.f, 1000.f);
            for (auto& n : _imageEditor)
                n->OptionSetBrushSize(_toolPicker.brushRadius);
        }
        break;
    case Tool::Eraser:
        ImGui::SameLine();
        ImGui::PushItemWidth(50.f);
        ImGui::InputFloat("Radius"_C, &_toolPicker.eraserRadius, 0, 0, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            _toolPicker.eraserRadius = std::clamp(_toolPicker.eraserRadius, 1.f, 1000.f);
            for (auto& n : _imageEditor)
                n->OptionSetBrushSize(_toolPicker.eraserRadius);
        }
        break;
    case Tool::Bucket:
    {
        ImGui::SameLine();
        ImGui::PushItemWidth(200.f);
        int32_t tolerance = settings.bucketTolerance;
        if (ImGui::SliderInt("Tolerance"_C, &tolerance, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp))
        {
            settings.bucketTolerance = tolerance;
            auto& target = _imageEditor.at(activeImageEditor);
            if (target->bucketFill)
                target->FillPixels(target->bucketFillPosition, target->bucketFillColor, settings.bucketTolerance);
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!_imageEditor.at(activeImageEditor)->bucketFill);
        if (ImGui::Button("Cancel"_C) || popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
        {
            _imageEditor.at(activeImageEditor)->OptionCancel();
        }
        ImGui::SameLine();
        if (ImGui::Button("Finish"_C) || popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
        {
            _imageEditor.at(activeImageEditor)->OptionFinish();
        }
        ImGui::EndDisabled();
        break;
    }
    case Tool::Gradient:
        ImGui::SameLine();
        ImGui::BeginDisabled(!_imageEditor.at(activeImageEditor)->gradientDraw);
        if (ImGui::Button("Cancel"_C) || popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
        {
            _imageEditor.at(activeImageEditor)->OptionCancel();
        }
        ImGui::SameLine();
        if (ImGui::Button("Finish"_C) || popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
        {
            _imageEditor.at(activeImageEditor)->OptionFinish();
        }
        ImGui::EndDisabled();
        break;
    default:
        break;
    }
    subTitleBarHeight = ImGui::GetCursorPosY();
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void glxy::App::MainWindow()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(Vector2f(0, ImGui::GetFrameHeight() + subTitleBarHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - subTitleBarHeight));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

    ImGui::Begin("MainViewDockspace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

    mainDockID = ImGui::GetID("MainDockspace");

    ImGui::DockSpace(mainDockID, Vector2f());

    ImGui::End();

    ImGui::PopStyleVar(2);
    for (int16_t i = 0; i < _imageEditor.size(); i++)
    {
        if (!_imageEditor.at(i)->windowOpen)
        {
            if (_imageEditor.at(i)->unsavedChanges)
            {
                editorCloseAttempt = i;
                _imageEditor.at(i)->windowOpen = true;
                popUpState.push_back(PopUpState::SaveBeforeClose);
            }
            else
            {
                DeleteEditor(i);
                i--;
            }
        }
    }
    for (auto& n : _imageEditor)
    {
        n->Update();
        if (_colorPicker.hasColorChanged())
        {
            if (n->bucketFill)
            {
                n->bucketFillColor = _colorPicker.getEditingColor();
                n->FillPixels(n->bucketFillPosition, n->bucketFillColor, settings.bucketTolerance);
            }
            if (n->gradientDraw)
            {
                n->GradientPixels(n->gradientStart.getPosition(), n->gradientEnd.getPosition(), _colorPicker.getColor(0), _colorPicker.getColor(1));
            }
        }
    }
    if (settings.syncViewport && activeImageEditor >= 0)
    {
        const Vector2f center = _imageEditor.at(activeImageEditor)->view.getCenter();
        const float scale = _imageEditor.at(activeImageEditor)->windowScale;
        for (auto& n : _imageEditor)
        {
            if (_imageEditor.at(activeImageEditor) == n)
                continue;
            if (n->windowScale != scale)
            {
                n->windowScale = scale;
                n->cameraOriginalPos = _imageEditor.at(activeImageEditor)->cameraOriginalPos;
                n->cameraOriginalSize = n->view.getSize();
                n->cameraAnimation = _imageEditor.at(activeImageEditor)->cameraAnimation;
                n->view.setCenter(center);
                n->view.setSize({ n->viewArea.size.x * n->windowScale, n->viewArea.size.y * n->windowScale });
                n->cameraTargetPos = _imageEditor.at(activeImageEditor)->cameraTargetPos;
                n->cameraTargetSize = n->view.getSize();
            }
            else
                n->view.setCenter(center);
        }
    }
}

void glxy::App::setActiveEditor(const int32_t ID)
{
    activeImageEditor = ID;
}

void glxy::App::setHoveredEditor(const int32_t ID)
{
    hoveredImageEditor = ID;
}

void glxy::App::app()
{
    bool fullscreenF11 = settings.fullscreen && window.getSize() == Vector2u(VideoMode::getDesktopMode().size.x, VideoMode::getDesktopMode().size.y);
    bool firstFrame = true;
    while (window.isOpen())
    {
        InputEvent::OnceUpdate();
        while (const std::optional<Event> event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<Event::Closed>())
                ExitApp(true);
            else if (const auto n = event->getIf<Event::Resized>())
                RescaleWindow(n->size);
            InputEvent::EventUpdate(*event);
        }

        if (pdo.getElapsedTime().asSeconds() > 1)
        {
            pdo.restart();
            if (filesystem::exists("pdo"))
            {
                filesystem::remove("pdo");
                window.requestFocus();
            }
            ofstream flag;
            flag.open("pdo.pdo");
            flag.close();
        }

        if (settings.outOfFocus && !InputEvent::WindowHasFocus())
        {
            sleep(milliseconds(100));
            continue;
        }
        if (changeFont)
        {
            ImFontConfig fc;
            fc.FontDataOwnedByAtlas = false;
            ImGui::GetIO().Fonts->Clear();
            ImGui::GetIO().Fonts->AddFontFromMemoryTTF(&mainFontData[0], mainFontData.size(), settings.fontSize, &fc);
            validate(ImGui::SFML::UpdateFontTexture());
            changeFont = false;
        }

        ImGui::SFML::Update(window, TimeControl::DeltaReal());

        GLOBAL.wantInput = ImGui::GetIO().WantTextInput;
        bool someEditorFocused = false;
        bool someEditorHovered = false;
        for (auto& n : _imageEditor)
        {
            if (n->windowFocused)
                someEditorFocused = true;
            if (n->windowHovered)
                someEditorHovered = true;
        }

        ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, someEditorFocused);

        for (uint16_t i = 0; i < _imageEditor.size(); i++)
        {
            if (_imageEditor.at(i)->windowFocused)
            {
                setActiveEditor(i);
            }
            if (_imageEditor.at(i)->windowHovered)
            {
                setHoveredEditor(i);
            }
        }
        if (!someEditorHovered && cursorType != Cursor::Type::Arrow)
        {
            setCursorType(Cursor::Type::Arrow);
        }
        if (!someEditorHovered && activeImageEditor != -1)
        {
            _toolPicker.setTool(_imageEditor.at(activeImageEditor)->currentTool, false);
            for (int8_t i = 0; i < c_colorCount; i++)
                _colorPicker.setEditorColors(i, _imageEditor.at(activeImageEditor)->currentColor.at(i));
        }
        else if (hoveredImageEditor != -1)
        {
            _toolPicker.setTool(_imageEditor.at(hoveredImageEditor)->currentTool, false);
            for (int8_t i = 0; i < c_colorCount; i++)
                _colorPicker.setEditorColors(i, _imageEditor.at(hoveredImageEditor)->currentColor.at(i));
        }
        else if (activeImageEditor != -1)
        {
            _toolPicker.setTool(_imageEditor.at(activeImageEditor)->currentTool, false);
            for (int8_t i = 0; i < c_colorCount; i++)
                _colorPicker.setEditorColors(i, _imageEditor.at(activeImageEditor)->currentColor.at(i));
        }

        TitleBar();
        SubTitleBar();
        MainWindow();
        _colorPicker.Draw();
        if (_colorPicker.hasColorChanged())
        {
            if (!someEditorHovered && activeImageEditor != -1)
            {
                for (int8_t i = 0; i < c_colorCount; i++)
                    _imageEditor.at(activeImageEditor)->currentColor.at(i) = _colorPicker.getColor(i);
            }
            else if (hoveredImageEditor != -1)
            {
                for (int8_t i = 0; i < c_colorCount; i++)
                    _imageEditor.at(hoveredImageEditor)->currentColor.at(i) = _colorPicker.getColor(i);
            }
            else if (activeImageEditor != -1)
            {
                for (int8_t i = 0; i < c_colorCount; i++)
                    _imageEditor.at(activeImageEditor)->currentColor.at(i) = _colorPicker.getColor(i);
            }
        }
        _toolPicker.Draw();
        if (_toolPicker.wasUserChanged())
        {
            if (!someEditorHovered && activeImageEditor != -1)
                _imageEditor.at(activeImageEditor)->currentTool = _toolPicker.getTool();
            else if (hoveredImageEditor != -1)
                _imageEditor.at(hoveredImageEditor)->currentTool = _toolPicker.getTool();
            else if (activeImageEditor != -1)
                _imageEditor.at(activeImageEditor)->currentTool = _toolPicker.getTool();
        }
        LayerPicker::Return result = _layerPicker.Draw(popUpState);
        switch (result)
        {
        case LayerPicker::Return::Visibility: _imageEditor.at(activeImageEditor)->chunkManager.RenderChunkArea(); break;
        case LayerPicker::Return::AddLayer: _imageEditor.at(activeImageEditor)->OptionCreateLayer(); break;
        case LayerPicker::Return::DeleteLayer: _imageEditor.at(activeImageEditor)->OptionDeleteLayer(); break;
        case LayerPicker::Return::DuplicateLayer: _imageEditor.at(activeImageEditor)->OptionDuplicateLayer(); break;
        case LayerPicker::Return::MergeLayerDown: _imageEditor.at(activeImageEditor)->OptionMergeLayerDown(); break;
        case LayerPicker::Return::MoveLayerUp: _imageEditor.at(activeImageEditor)->OptionMoveLayerUp(); break;
        case LayerPicker::Return::MoveLayerDown: _imageEditor.at(activeImageEditor)->OptionMoveLayerDown(); break;
        }
        
        if (Shortcuts()[ActionShortcut::ToggleFullscreen])
        {
            fullscreenF11 = !fullscreenF11;
            if (fullscreenF11)
            {
                settings.resolution = VideoMode::getDesktopMode().size;
                settings.fullscreen = true;
            }
            else
            {
                settings.resolution = Vector2u(1280, 720);
                settings.fullscreen = false;
            }
            RecreateAppWindow();
            RescaleWindow(settings.resolution);
            settings.Save();
        }
        PopUp();

        ImGui::PopItemFlag();

        if (!window.isOpen())
            break;

        window.clear();
        ImGui::SFML::Render(window);
        window.display();

        if (firstFrame)
        {
            GalaxyStartUpTime = startUpTimer->getElapsedTime();
            firstFrame = false;
        }
        TimeControl::Update();
    }
}

bool glxy::App::OpenImage(const filesystem::path& fileName)
{
    _imageEditor.emplace_back(make_unique<ImageEditor>(settings, window, popUpState, cursor, cursorType, _colorPicker,
        _imageEditor.size() ? _imageEditor.back()->dockID : mainDockID, _toolPicker, _layerPicker, gizmoIcons, mainFont));
    const int16_t temp = activeImageEditor;
    setActiveEditor(_imageEditor.size() - 1);
    _layerPicker.createNewImage();
    const bool result = _imageEditor.back()->Open(fileName);
    if (!result)
    {
        _imageEditor.pop_back();
        _layerPicker.deleteImage(_imageEditor.size());
        setActiveEditor(temp);
        return false;
    }
    AddRecentFile(_imageEditor.at(activeImageEditor)->imagePath);
    return true;
}

bool glxy::App::SaveImage()
{
    if (activeImageEditor >= 0 && activeImageEditor < _imageEditor.size())
    {
        if (_imageEditor.at(activeImageEditor)->imagePath.empty())
        {
            popUpState.push_back(PopUpState::Save);
            return false;
        }
        if (_imageEditor.at(activeImageEditor)->Save())
            AddRecentFile(_imageEditor.at(activeImageEditor)->imagePath);
        else
            return false;
    }
    return true;
}