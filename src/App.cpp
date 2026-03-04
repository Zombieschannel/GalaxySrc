#include "App.hpp"
#include "Func.hpp"
#include "ZEditorsCommon/GlobalClock.hpp"
#include "ZEditorsCommon/Shortcuts.hpp"
#include "ZEditorsCommon/Themes.hpp"
#include "ZEditorsCommon/InternalResource.hpp"
#include "ZEditorsCommon/Languages.hpp"
#include "Global.hpp"
#include <SFML/OpenGL.hpp>
#include <set>


#ifdef SFML_SYSTEM_ANDROID
#include <SFML/System/NativeActivity.hpp>
#include <android/native_activity.h>
#endif

glxy::App::App()
    : _colorPicker(window, settings.GUIScale, settings.showRuler, settings.colorPickerTriangle),
    _toolPicker(settings.GUIScale, settings.showRuler, toolIcons)
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
    adjSettings.Load();
    effSettings.Load();
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
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(&mainFontData[0], mainFontData.size(), settings.GUIScale * 16.f, &fc);

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
#ifdef SFML_DESKTOP
    ImGui::GetStyle().ScrollbarSize = 14.f;
#else
    ImGui::GetStyle().ScrollbarSize = 20.f;
#endif

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = "windows.ini";

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    this->startUpTimer = &startUpTimer;
    CanvasWorker::Setup();
    app();
    RenderWorker::ExitThread();
    CanvasWorker::ExitThread();
    settings.Save();
    adjSettings.Save();
    effSettings.Save();
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

#ifdef SFML_DESKTOP
    window.create(VideoMode({ settings.resolution.x, settings.resolution.y }), c_AppName, settings.fullscreen ? Style::None : Style::Default,
        settings.fullscreen ? State::Fullscreen : State::Windowed, { 0U, 0U, settings.antialiasing, GLTarget / 10U, GLTarget % 10U});
#else
    window.create(VideoMode({ settings.resolution.x, settings.resolution.y }), c_AppName, Style::None, State::Fullscreen,
        { 0U, 0U, settings.antialiasing, GLTarget / 10U, GLTarget % 10U});
#endif

#ifdef SFML_SYSTEM_ANDROID
    sleep(milliseconds(200));
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
    CanvasWorker::waitWork();
    CanvasWorker::AddEditor();
    _imageEditor.emplace_back(std::make_shared<ImageEditor>(settings, window, popUpState, cursor, cursorType, _colorPicker,
        !_imageEditor.empty() ? _imageEditor.back()->dockID : mainDockID, _toolPicker, _layerPicker, gizmoIcons, mainFont,
        CanvasWorker::getChunkManager(static_cast<EditorID>(_imageEditor.size())),
        CanvasWorker::getCommon(static_cast<EditorID>(_imageEditor.size())),
        static_cast<int16_t>(_imageEditor.size())));
    setActiveEditor(_imageEditor.size() - 1);
    _layerPicker.createNewImage();
    AddWork(CanvasWork::ImageEmpty{resolution, color});
    _imageEditor.back()->imagePath.clear();
}

bool glxy::App::hasUnsavedImages() const
{
    for (auto& n : _imageEditor)
    {
        if (n->unsavedChanges)
            return true;
    }
    return false;
}

void glxy::App::ExitApp(const bool windowClose)
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

void glxy::App::DeleteEditor(const EditorID ID)
{
    CanvasWorker::waitWork();
    _imageEditor.erase(_imageEditor.begin() + ID);
    CanvasWorker::RemoveEditor(ID);
    for (EditorID i = ID; i < _imageEditor.size(); i++)
        _imageEditor.at(i)->arrayID--;

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
    resetPopupWindow = true;
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
            Shortcuts::getName(ActionShortcut::NewImage),
            Shortcuts::getName(ActionShortcut::OpenImage),
            Shortcuts::getName(ActionShortcut::SaveImage),
            Shortcuts::getName(ActionShortcut::SaveImageAs),
            ""
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
        ImGui::BeginDisabled(!active || !active->chunkManager.hasSelectionLayer(0));
        if (ImGui::MenuItem("editMenu[0]"_C, Shortcuts::getName(ActionShortcut::Copy).c_str()))
        {
            clipboardImage = make_unique<Image>();
            AddWorkAndWait(CanvasWork::ClipboardCopy{clipboardImage.get(), &clipboardLocation, _layerPicker.getLayerIDSelected(activeImageEditor)});
        }
        if (ImGui::MenuItem("editMenu[1]"_C, Shortcuts::getName(ActionShortcut::Cut).c_str()))
        {
            clipboardImage = make_unique<Image>();
            AddWork(CanvasWork::ClipboardCopy{clipboardImage.get(), &clipboardLocation, _layerPicker.getLayerIDSelected(activeImageEditor)});
            AddWorkAndWait(CanvasWork::DeleteSelected{_layerPicker.getLayerIDSelected(activeImageEditor)});
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!clipboardImage);
        if (ImGui::MenuItem("editMenu[2]"_C, Shortcuts::getName(ActionShortcut::Paste).c_str()))
        {
            _imageEditor.at(activeImageEditor)->currentTool = Tool::MoveSelection;
            _imageEditor.at(activeImageEditor)->ClipboardPaste(clipboardImage.get(), &clipboardLocation);
            AddWorkAndWait(CanvasWork::ClipboardPaste{clipboardImage.get(), &clipboardLocation,
                _layerPicker.getLayerIDSelected(activeImageEditor), _imageEditor.at(activeImageEditor)->moveSelectionTransform.getTransform()});
        }
        ImGui::EndDisabled();

        if (ImGui::MenuItem("editMenu[3]"_C, Shortcuts::getName(ActionShortcut::SelectAll).c_str()))
        {
            active->currentTool = Tool::BoxSelect;
            AddWorkAndWait(CanvasWork::SelectAll{});
        }

        ImGui::BeginDisabled(!active || !active->chunkManager.hasSelectionLayer(0));
        if (ImGui::MenuItem("editMenu[4]"_C, Shortcuts::getName(ActionShortcut::DeselectAll).c_str()))
            AddWorkAndWait(CanvasWork::DeselectAll{});
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!active || !active->chunkManager.hasSelectionLayer(0));
        if (ImGui::MenuItem("editMenu[5]"_C, Shortcuts::getName(ActionShortcut::Delete).c_str()))
            AddWorkAndWait(CanvasWork::DeleteSelected{_layerPicker.getLayerIDSelected(activeImageEditor)});
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }
    ImGui::EndDisabled();
    if (activeImageEditor >= 0)
    {
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Copy] && _imageEditor.at(activeImageEditor)->chunkManager.hasSelectionLayer(0))
        {
            clipboardImage = make_unique<Image>();
            AddWorkAndWait(CanvasWork::ClipboardCopy{clipboardImage.get(), &clipboardLocation, _layerPicker.getLayerIDSelected(activeImageEditor)});
        }
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Cut] && _imageEditor.at(activeImageEditor)->chunkManager.hasSelectionLayer(0))
        {
            clipboardImage = make_unique<Image>();
            _imageEditor.at(activeImageEditor)->unsavedChanges = true;
            AddWork(CanvasWork::ClipboardCopy{clipboardImage.get(), &clipboardLocation, _layerPicker.getLayerIDSelected(activeImageEditor)});
            AddWorkAndWait(CanvasWork::DeleteSelected{_layerPicker.getLayerIDSelected(activeImageEditor)});
        }
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Paste] && clipboardImage)
        {
            _imageEditor.at(activeImageEditor)->currentTool = Tool::MoveSelection;
            _imageEditor.at(activeImageEditor)->unsavedChanges = true;
            _imageEditor.at(activeImageEditor)->ClipboardPaste(clipboardImage.get(), &clipboardLocation);
            AddWorkAndWait(CanvasWork::ClipboardPaste{clipboardImage.get(), &clipboardLocation,
                _layerPicker.getLayerIDSelected(activeImageEditor), _imageEditor.at(activeImageEditor)->moveSelectionTransform.getTransform()});
        }
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::SelectAll])
        {
            _imageEditor.at(activeImageEditor)->currentTool = Tool::BoxSelect;
            AddWorkAndWait(CanvasWork::SelectAll{});
        }
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::DeselectAll] && _imageEditor.at(activeImageEditor)->chunkManager.hasSelectionLayer(0))
            AddWorkAndWait(CanvasWork::DeselectAll{});
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Delete] && _imageEditor.at(activeImageEditor)->chunkManager.hasSelectionLayer(0))
        {
            _imageEditor.at(activeImageEditor)->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::DeleteSelected{_layerPicker.getLayerIDSelected(activeImageEditor)});
        }
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
        ImGui::Separator();
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
            settings.syncViewport = !settings.syncViewport;
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Image"_C))
    {
        ImageEditor* active = activeImageEditor == -1 ? nullptr : _imageEditor.at(activeImageEditor).get();
        ImGui::BeginDisabled(!active || !active->chunkManager.hasSelectionLayer(0));
        if (ImGui::MenuItem("imageMenu[0]"_C))
        {
            AddWorkAndWait(CanvasWork::CropSelection{});
            active->ClampView();
            active->unsavedChanges = true;
            active->gridLines.manualChange = true;
            active->rulerUI.manualChange = true;
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("imageMenu[1]"_C, Shortcuts::getName(ActionShortcut::Resize).c_str()))
            popUpState.push_back(PopUpState::Resize);
        if (ImGui::MenuItem("imageMenu[2]"_C, Shortcuts::getName(ActionShortcut::ResizeCanvas).c_str()))
            popUpState.push_back(PopUpState::ResizeCanvas);
        ImGui::Separator();
        if (ImGui::MenuItem("imageMenu[3]"_C))
        {
            active->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::FlipImageHorizontal{});
        }
        if (ImGui::MenuItem("imageMenu[4]"_C))
        {
            active->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::FlipImageVertical{});
        }
        ImGui::Separator();
        if (ImGui::MenuItem("imageMenu[5]"_C))
        {
            active->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::Rotate90CW{});
        }
        if (ImGui::MenuItem("imageMenu[6]"_C))
        {
            active->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::Rotate90CCW{});
        }
        if (ImGui::MenuItem("imageMenu[7]"_C))
        {
            active->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::Rotate180{});
        }
        ImGui::Separator();
        if (ImGui::MenuItem("imageMenu[8]"_C, Shortcuts::getName(ActionShortcut::TransformImage).c_str()))
        {
            _imageEditor.at(activeImageEditor)->currentTool = Tool::BoxSelect;
            popUpState.push_back(PopUpState::TransformImage);
            AddWork(CanvasWork::TransformImageSetup{_layerPicker.getLayerIDSelected(activeImageEditor)});
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
            _imageEditor.at(activeImageEditor)->currentTool = Tool::BoxSelect;
            popUpState.push_back(PopUpState::TransformImage);
            AddWork(CanvasWork::TransformImageSetup{_layerPicker.getLayerIDSelected(activeImageEditor)});
        }
    }
    if (ImGui::BeginMenu("Layers"_C))
    {
        ImageEditor* active = activeImageEditor == -1 ? nullptr : _imageEditor.at(activeImageEditor).get();
        if (ImGui::MenuItem("layerMenu[0]"_C))
        {
            active->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::CreateLayer{_layerPicker.getLayerIDSelected(activeImageEditor)});
            _layerPicker.createNewLayer(activeImageEditor);
        }
        ImGui::BeginDisabled(!active || active->chunkManager.getLayerCount() == 1);
        if (ImGui::MenuItem("layerMenu[1]"_C))
        {
            active->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::DeleteLayer{_layerPicker.getLayerIDSelected(activeImageEditor)});
            _layerPicker.deleteLayer(activeImageEditor);
        }
        ImGui::EndDisabled();
        if (ImGui::MenuItem("layerMenu[2]"_C))
        {
            active->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::DuplicateLayer{_layerPicker.getLayerIDSelected(activeImageEditor)});
            _layerPicker.duplicateLayer(activeImageEditor);
        }
        ImGui::Separator();
        ImGui::BeginDisabled(!active || _layerPicker.getLayerIDSelected(activeImageEditor) >= active->chunkManager.getLayerCount() - 1);
        if (ImGui::MenuItem("layerMenu[3]"_C))
        {
            active->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::MoveLayerUp{_layerPicker.getLayerIDSelected(activeImageEditor)});
            _layerPicker.moveLayerUp(activeImageEditor);
        }
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!active || _layerPicker.getLayerIDSelected(activeImageEditor) <= 0);
        if (ImGui::MenuItem("layerMenu[4]"_C))
        {
            active->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::MoveLayerDown{_layerPicker.getLayerIDSelected(activeImageEditor)});
            _layerPicker.moveLayerDown(activeImageEditor);
        }
        if (ImGui::MenuItem("layerMenu[5]"_C))
        {
            active->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::MergeLayerDown{_layerPicker.getLayerIDSelected(activeImageEditor)});
            _layerPicker.deleteLayer(activeImageEditor);
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("layerMenu[6]"_C))
        {
            active->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::FlipLayerHorizontal{_layerPicker.getLayerIDSelected(activeImageEditor)});
        }
        if (ImGui::MenuItem("layerMenu[7]"_C))
        {
            active->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::FlipLayerVertical{_layerPicker.getLayerIDSelected(activeImageEditor)});
        }
        ImGui::Separator();
        if (ImGui::MenuItem("layerMenu[8]"_C))
        {
            popUpState.push_back(PopUpState::LayerProperties);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Adjustments"_C))
    {
        if (ImGui::MenuItem("adjustMenu[0]"_C))
        {
            popUpState.push_back(PopUpState::Adjustment);
            targetAdjustment = Adjustments::BlackAndWhite;
        }
        if (ImGui::MenuItem("adjustMenu[1]"_C))
        {
            popUpState.push_back(PopUpState::Adjustment);
            targetAdjustment = Adjustments::BrightnessContrast;
        }
        if (ImGui::MenuItem("adjustMenu[2]"_C))
        {
            popUpState.push_back(PopUpState::Adjustment);
            targetAdjustment = Adjustments::HSV;
        }
        if (ImGui::MenuItem("adjustMenu[3]"_C))
        {
            popUpState.push_back(PopUpState::Adjustment);
            targetAdjustment = Adjustments::Invert;
        }
        if (ImGui::MenuItem("adjustMenu[4]"_C))
        {
            popUpState.push_back(PopUpState::Adjustment);
            targetAdjustment = Adjustments::Tint;
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Effects"_C))
    {
        if (ImGui::MenuItem("effectMenu[0]"_C))
        {
            popUpState.push_back(PopUpState::Effect);
            targetEffect = Effects::GaussianBlur;
        }
        if (ImGui::MenuItem("effectMenu[1]"_C))
        {
            popUpState.push_back(PopUpState::Effect);
            targetEffect = Effects::BoxBlur;
        }
        if (ImGui::MenuItem("effectMenu[2]"_C))
        {
            popUpState.push_back(PopUpState::Effect);
            targetEffect = Effects::DirectionalBlur;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("effectMenu[3]"_C))
        {
            popUpState.push_back(PopUpState::Effect);
            targetEffect = Effects::WhiteNoise;
        }
        if (ImGui::MenuItem("effectMenu[4]"_C))
        {
            popUpState.push_back(PopUpState::Effect);
            targetEffect = Effects::FractalNoise;
        }
        ImGui::EndMenu();
    }
    ImGui::EndDisabled();

    if (ImGui::BeginMenu("Help"_C))
    {
        if (ImGui::MenuItem("OtherType[0]"_C))
            popUpState.push_back(PopUpState::Changelog);
        if (ImGui::MenuItem("OtherType[1]"_C))
            popUpState.push_back(PopUpState::FuturePlan);
        if (ImGui::MenuItem("OtherType[2]"_C))
            popUpState.push_back(PopUpState::GLScan);
        if (ImGui::MenuItem("OtherType[3]"_C, Shortcuts::getName(ActionShortcut::Debug).c_str(), settings.debugMode))
            settings.debugMode = !settings.debugMode;
        if (ImGui::MenuItem("OtherType[4]"_C))
            popUpState.push_back(PopUpState::About);
        ImGui::EndMenu();
    }
    if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Debug])
        settings.debugMode = !settings.debugMode;

    if (dfp < seconds(1))
        ImGui::Text("%s", (" " + "Saved"_S).c_str());
    if (settings.showFPS)
    {
        const string fps = " FPS: " + to_string(static_cast<int32_t>(1.f / TimeControl::DeltaReal().asSeconds())) + "\t";
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
        if (ImGui::BeginCombo("Select mode"_C, LL::ind("boxSelectMode[]", static_cast<int32_t>(_toolPicker.selectMode)).c_str()))
        {
            for (uint8_t i = 0; i < 3; i++)
            {
                if (ImGui::Selectable(LL::ind("boxSelectMode[]", i).c_str()))
                    _toolPicker.selectMode = static_cast<SelectMode>(i);
            }
            ImGui::EndCombo();
        }
        break;
    case Tool::Pan: case Tool::Zoom:
        break;
    case Tool::MoveSelection:
    {
        bool statement = false;
        ImGui::SameLine();
        if (activeImageEditor >= 0)
        {
            lock_guard lock(_imageEditor.at(activeImageEditor)->common.mtxEditorWorkerCommon);
            statement = _imageEditor.at(activeImageEditor)->common.getMoveSelection();
        }
        ImGui::BeginDisabled(!statement);
        if (ImGui::Button("Cancel"_C) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
        {
            _imageEditor.at(activeImageEditor)->ResetMovePixels();
            popUpState.push_back(PopUpState::ThreadWork);
        }
        ImGui::SameLine();
        if (ImGui::Button("Finish"_C) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
        {
            _imageEditor.at(activeImageEditor)->FinishMovePixels();
            popUpState.push_back(PopUpState::ThreadWork);
        }
        ImGui::EndDisabled();
        break;
    }
    case Tool::MagicWand:
    {
        bool statement = false;
        ImGui::SameLine();
        ImGui::PushItemWidth(200.f);
        int32_t tolerance = settings.wandTolerance;
        if (ImGui::SliderInt("Tolerance"_C, &tolerance, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp))
        {
            settings.wandTolerance = tolerance;
            auto& target = _imageEditor.at(activeImageEditor);
            if (lock_guard lock(target->common.mtxEditorWorkerCommon);
                target->common.getWandFill())
                AddWork(CanvasWork::WandFill{Vector2u(target->wandFillPosition), settings.wandTolerance,
                    _layerPicker.getLayerIDSelected(activeImageEditor)});
        }
        ImGui::SameLine();
        if (activeImageEditor >= 0)
        {
            lock_guard lock(_imageEditor.at(activeImageEditor)->common.mtxEditorWorkerCommon);
            statement = _imageEditor.at(activeImageEditor)->common.getWandFill();
        }
        ImGui::BeginDisabled(!statement);
        if (ImGui::Button("Cancel"_C) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            AddWorkAndWait(CanvasWork::Cancel{});
        ImGui::SameLine();
        if (ImGui::Button("Finish"_C) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            AddWorkAndWait(CanvasWork::Finish{});
        ImGui::EndDisabled();
        break;
    }
    case Tool::Brush:
        ImGui::SameLine();
        ImGui::PushItemWidth(50.f);
        ImGui::InputFloat("Radius"_C, &_toolPicker.brushRadius, 0, 0, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            _toolPicker.brushRadius = std::clamp(_toolPicker.brushRadius, 0.5f, 1000.f);
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
            _toolPicker.eraserRadius = std::clamp(_toolPicker.eraserRadius, 0.5f, 1000.f);
            for (auto& n : _imageEditor)
                n->OptionSetBrushSize(_toolPicker.eraserRadius);
        }
        break;
    case Tool::Bucket:
    {
        bool statement = false;
        ImGui::SameLine();
        ImGui::PushItemWidth(200.f);
        int32_t tolerance = settings.bucketTolerance;
        if (ImGui::SliderInt("Tolerance"_C, &tolerance, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp))
        {
            settings.bucketTolerance = tolerance;
            auto& target = _imageEditor.at(activeImageEditor);
            if (lock_guard lock(target->common.mtxEditorWorkerCommon);
                target->common.getBucketFill())
                AddWork(CanvasWork::BucketFill{target->bucketFillPosition, target->bucketFillColor, settings.bucketTolerance,
                    _layerPicker.getLayerIDSelected(activeImageEditor)});
        }
        ImGui::SameLine();
        if (activeImageEditor >= 0)
        {
            lock_guard lock(_imageEditor.at(activeImageEditor)->common.mtxEditorWorkerCommon);
            statement = _imageEditor.at(activeImageEditor)->common.getBucketFill();
        }
        ImGui::BeginDisabled(!statement);
        if (ImGui::Button("Cancel"_C) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            AddWorkAndWait(CanvasWork::Cancel{});
        ImGui::SameLine();
        if (ImGui::Button("Finish"_C) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            AddWorkAndWait(CanvasWork::Finish{});
        ImGui::EndDisabled();
        break;
    }
    case Tool::Gradient:
    {
        bool statement = false;
        ImGui::SameLine();
        if (activeImageEditor >= 0)
        {
            lock_guard lock(_imageEditor.at(activeImageEditor)->common.mtxEditorWorkerCommon);
            statement = _imageEditor.at(activeImageEditor)->common.getGradientDraw();
        }
        ImGui::BeginDisabled(!statement);
        if (ImGui::Button("Cancel"_C) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
            AddWorkAndWait(CanvasWork::Cancel{});
        ImGui::SameLine();
        if (ImGui::Button("Finish"_C) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
            AddWorkAndWait(CanvasWork::Finish{});
        ImGui::EndDisabled();
        break;
    }
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
            if (lock_guard lock(n->common.mtxEditorWorkerCommon);
                n->common.getBucketFill())
            {
                n->bucketFillColor = _colorPicker.getEditingColor();
                AddWork(CanvasWork::BucketFill{n->bucketFillPosition, n->bucketFillColor, settings.bucketTolerance,
                    _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            if (lock_guard lock(n->common.mtxEditorWorkerCommon);
                n->common.getGradientDraw())
            {
                AddWork(CanvasWork::GradientPixels{n->gradientStart.getPosition(), n->gradientEnd.getPosition(),
                    _colorPicker.getColor(0), _colorPicker.getColor(1), _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
        }
    }
    UpdateRenderWorker();
    if (settings.syncViewport && hoveredImageEditor >= 0)
    {
        const Vector2f center = _imageEditor.at(hoveredImageEditor)->view.getCenter();
        const float scale = _imageEditor.at(hoveredImageEditor)->windowScale;
        for (auto& n : _imageEditor)
        {
            if (_imageEditor.at(hoveredImageEditor) == n)
                continue;
            if (n->windowScale != scale)
            {
                n->windowScale = scale;
                n->cameraOriginalPos = _imageEditor.at(hoveredImageEditor)->cameraOriginalPos;
                n->cameraOriginalSize = n->view.getSize();
                n->cameraAnimation = _imageEditor.at(hoveredImageEditor)->cameraAnimation;
                n->cameraAnimationRunning = _imageEditor.at(hoveredImageEditor)->cameraAnimationRunning;

                n->view.setCenter(center);
                n->view.setSize({ n->viewArea.size.x * n->windowScale, n->viewArea.size.y * n->windowScale });

                n->cameraTargetPos = _imageEditor.at(hoveredImageEditor)->cameraTargetPos;
                n->cameraTargetSize = n->view.getSize();
            }
            else
                n->view.setCenter(center);
        }
    }
}

void glxy::App::UpdateRenderWorker() const
{
    if (!RenderWorker::hasWork())
        return;
    const int32_t todoWork = RenderWorker::getWorkAmount();
    const array<pair<int32_t, Time>, 4> cutoff = {
        pair{10, milliseconds(5)},
        pair{20, milliseconds(10)},
        pair{30, milliseconds(15)},
        pair{40, milliseconds(20)},
    };
    Time maxTime = cutoff.back().second;
    for (int32_t i = 0; i < cutoff.size(); i++)
    {
        if (todoWork < cutoff.at(i).first)
        {
            maxTime = cutoff.at(i).second;
            break;
        }
    }
    const Clock workTime;
    do
    {
        if (!RenderWorker::hasWork())
            break;
        RenderWorkBatch batch = RenderWorker::getWorkTodo();
        array<RenderTexture, RenderWorkBatch::BatchMaxSize> textures;

        for (int8_t i = 0; i < batch.work.size(); i++)
        {
            const unique_ptr<RenderWork> work = std::move(batch.work.at(i));
            RenderTexture& renderTexture = textures.at(i);

            auto& ie = _imageEditor.at(work->getEditorID());
            if (const auto v = work->get<RenderWork::MergeLayers>())
            {
                const Vector2u chunkSize = ie->chunkManager.getChunkSize(v->chunkID);
                validate(renderTexture.resize(chunkSize, {0U, 0U, 0U, 0U, 0U}));

                renderTexture.clear(Color::Transparent);
                renderTexture.setView(View(FloatRect({0.f, 0.f}, Vector2f(chunkSize))));

                ImageChunkTexture::RenderLayerToTexture(*ie->chunkManager.getChunkImageColor(v->chunkID, v->lowerLayerID),
                    chunkSize, 255, c_blendModes.at(ie->chunkManager.getLayerBlendMode(v->lowerLayerID)), renderTexture);
                ImageChunkTexture::RenderLayerToTexture(*ie->chunkManager.getChunkImageColor(v->chunkID, v->upperLayerID),
                    chunkSize, ie->chunkManager.getLayerTransparency(v->upperLayerID), c_blendModes.at(ie->chunkManager.getLayerBlendMode(v->upperLayerID)), renderTexture);
            }
            else if (const auto v = work->get<RenderWork::MergeColorTempLayer>())
            {
                const Vector2u chunkSize = ie->chunkManager.getChunkSize(v->chunkID);
                validate(renderTexture.resize(chunkSize, {0U, 0U, 0U, 0U, 0U}));

                renderTexture.clear(Color::Transparent);
                renderTexture.setView(View(FloatRect({0.f, 0.f}, Vector2f(chunkSize))));

                ImageChunkTexture::RenderLayerToTexture(*ie->chunkManager.getChunkImageColor(v->chunkID, v->lowerLayerID),
                    chunkSize, 255, BlendNone, renderTexture);
                ImageChunkTexture::RenderLayerToTexture(*ie->chunkManager.getChunkImageColorTemp(v->chunkID),
                    chunkSize, 255, v->blendMode, renderTexture);
            }
            else if (const auto v = work->get<RenderWork::RenderSelection>())
            {
                const Vector2u chunkSize = ie->chunkManager.getChunkSize(v->chunkID);
                const uint16_t chunkGeneralSize = ie->chunkManager.getChunkSize();
                const Vector2u chunkCount = ie->chunkManager.getChunkCount();
                validate(renderTexture.resize(chunkSize, {0U, 0U, 0U, 0U, 0U}));

                const Vector2u chunk = Vector2u(v->chunkID % chunkCount.x, v->chunkID / chunkCount.x);

                renderTexture.clear(Color::Transparent);

                if (v->isFinal)
                {
                    renderTexture.setView(View(FloatRect(Vector2f(0, 0), Vector2f(chunkSize))));
                    ImageChunkTexture::RenderLayerToTexture(*ie->chunkManager.getChunkImageSelection(v->chunkID),
                        chunkSize, 255, BlendNone, renderTexture);
                }

                renderTexture.setView(View(FloatRect(Vector2f(chunk.x * chunkGeneralSize, chunk.y * chunkGeneralSize), Vector2f(chunkSize))));
                if (v->shapeSelectionType == ShapeSelectType::Box)
                {
                    RectangleShape shape;
                    shape.setFillColor(Color::Black);
                    if (v->isFinal && !v->additive)
                        shape.setFillColor(Color::Transparent);

                    shape.setPosition(Vector2f(v->area.position));
                    shape.setScale(Vector2f(v->area.size));
                    shape.setSize(Vector2f(1, 1));

                    renderTexture.draw(shape, BlendNone);
                }
                else if (v->shapeSelectionType == ShapeSelectType::Circle)
                {
                    CircleShape shape;
                    shape.setFillColor(Color::Black);
                    if (v->isFinal && !v->additive)
                        shape.setFillColor(Color::Transparent);

                    shape.setPointCount(fmax(2.f * 3.14159265f * sqrtf(fmax(v->area.size.x, v->area.size.y)), 20.f));
                    shape.setRadius(0.5f);
                    shape.setPosition(Vector2f(v->area.position));
                    shape.setScale(Vector2f(v->area.size));

                    renderTexture.draw(shape, BlendNone);
                }
            }
            else if (const auto v = work->get<RenderWork::BrushDraw>())
            {
                const Vector2u chunkSize = ie->chunkManager.getChunkSize(v->chunkID);
                const Vector2u chunkCount = ie->chunkManager.getChunkCount();
                const Vector2u chunk = Vector2u(v->chunkID % chunkCount.x, v->chunkID / chunkCount.x);
                const uint16_t chunkGeneralSize = ie->chunkManager.getChunkSize();

                validate(renderTexture.resize(chunkSize, {0U, 8U, 0U, 0U, 0U}));

                renderTexture.clear(v->isEraser ? Color::White : Color::Transparent);
                renderTexture.clearStencil(0x00);

#ifdef GL_ALPHA_TEST
                renderTexture.resetGLStates(); // workaround for mixing SFML with OpenGL
#endif

                if (ie->chunkManager.hasColorTempLayer(v->chunkID))
                {
                    ImageChunkTexture::RenderLayerToTexture(*ie->chunkManager.getChunkImageColorTemp(v->chunkID),
                        chunkSize, 255, BlendNone, renderTexture);
                }

                if (ie->chunkManager.hasSelectionLayer(v->chunkID))
                {
#ifdef GL_ALPHA_TEST
                    glEnable(GL_ALPHA_TEST);
                    glAlphaFunc(GL_GREATER, 0.5f);
#endif
                    ImageChunkTexture::RenderLayerToTexture(*ie->chunkManager.getChunkImageSelection(v->chunkID),
                        chunkSize, 255, RenderStates(StencilMode{
                            StencilComparison::Always, StencilUpdateOperation::Increment, 0x01, 0xFF, true}), renderTexture);
#ifdef GL_ALPHA_TEST
                    glDisable(GL_ALPHA_TEST);
#endif
                }

                renderTexture.setView(View(FloatRect(Vector2f(chunk.x * chunkGeneralSize, chunk.y * chunkGeneralSize), Vector2f(chunkSize))));

                CircleShape shape;
                shape.setFillColor(v->color);

                RenderStates states;
                states.blendMode = BlendNone;
                states.stencilMode = {ie->chunkManager.hasSelectionLayer(v->chunkID) ? StencilComparison::Equal : StencilComparison::Always,
                    StencilUpdateOperation::Keep, 0x01, 0xFF, false};

                shape.setPointCount(fmax(2.f * 3.14159265f * sqrtf(v->radius), 20.f));
                shape.setRadius(v->radius);
                shape.setOrigin(Vector2f(v->radius, v->radius));

                shape.setPosition(Vector2f(Vector2i(v->start)) + Vector2f(0.5f, 0.5f));
                renderTexture.draw(shape, states);

                shape.setPosition(Vector2f(Vector2i(v->end)) + Vector2f(0.5f, 0.5f));
                renderTexture.draw(shape, states);

                if (v->start != v->end)
                {
                    const Vector2f diff = v->end - v->start;
                    const Vector2f perp = diff.perpendicular().normalized();
                    const array<Vertex, 4> connect = {
                        Vertex{Vector2f(v->start + perp * v->radius), v->color},
                        Vertex{Vector2f(v->start + -perp * v->radius), v->color},
                        Vertex{Vector2f(v->end + -perp * v->radius), v->color},
                        Vertex{Vector2f(v->end + perp * v->radius), v->color}
                    };
                    renderTexture.draw(connect.data(), 4, PrimitiveType::TriangleFan, states);
                }
            }
            renderTexture.display();
        }

        for (int8_t i = 0; i < batch.work.size(); i++)
        {
            RenderWorker::AddResult(RenderResult::Chunk{textures.at(i).getTexture().copyToImage()});
            RenderWorker::RemoveWorkTodo();
        }

    } while (workTime.getElapsedTime() <= maxTime);
}

void glxy::App::setActiveEditor(const EditorID ID)
{
    activeImageEditor = ID;
}

void glxy::App::setHoveredEditor(const EditorID ID)
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
            ImGui::GetIO().Fonts->AddFontFromMemoryTTF(&mainFontData[0], mainFontData.size(), settings.GUIScale * 16.f, &fc);
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

        for (EditorID i = 0; i < _imageEditor.size(); i++)
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

        if (popUpState.empty() && CanvasWorker::getWorkAmount() > 100)
        {
            popUpState.push_back(PopUpState::ThreadWork);
            CanvasWorker::LockAddingNewWork(true);
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
        if (_toolPicker.wasUserChanged() && activeImageEditor >= 0)
        {
            if (lock_guard lock(_imageEditor.at(activeImageEditor)->common.mtxEditorWorkerCommon);
                _imageEditor.at(activeImageEditor)->common.hasUnfinishedChanges())
            {
                changeToTool = _toolPicker.getTool();
                AddWork(CanvasWork::Finish{});
                popUpState.push_back(PopUpState::ToolChanged);
                popUpState.push_back(PopUpState::ThreadWork);
            }
            else
                _imageEditor.at(activeImageEditor)->currentTool = _toolPicker.getTool();
        }
        LayerPicker::Return result = _layerPicker.Draw(window, settings.GUIScale, layerIcons, popUpState, activeImageEditor);
        switch (result)
        {
        case LayerPicker::Return::Visibility:
        {
            const auto& layer = _layerPicker.getLayer(activeImageEditor, _layerPicker.getLayerIDSelected(activeImageEditor));
            _imageEditor.at(activeImageEditor)->unsavedChanges = true;
            AddWork(CanvasWork::LayerPropertyChanged{_layerPicker.getLayerIDSelected(activeImageEditor),
                layer.enabled, layer.transparency, layer.blendMode});
            break;
        }
        case LayerPicker::Return::AddLayer:
            _imageEditor.at(activeImageEditor)->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::CreateLayer{_layerPicker.getLayerIDSelected(activeImageEditor)});
            _layerPicker.createNewLayer(activeImageEditor);
            break;
        case LayerPicker::Return::DeleteLayer:
            _imageEditor.at(activeImageEditor)->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::DeleteLayer{_layerPicker.getLayerIDSelected(activeImageEditor)});
            _layerPicker.deleteLayer(activeImageEditor);
            break;
        case LayerPicker::Return::DuplicateLayer:
            _imageEditor.at(activeImageEditor)->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::DuplicateLayer{_layerPicker.getLayerIDSelected(activeImageEditor)});
            _layerPicker.duplicateLayer(activeImageEditor);
            break;
        case LayerPicker::Return::MergeLayerDown:
            _imageEditor.at(activeImageEditor)->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::MergeLayerDown{_layerPicker.getLayerIDSelected(activeImageEditor)});
            _layerPicker.deleteLayer(activeImageEditor);
            break;
        case LayerPicker::Return::MoveLayerUp:
            _imageEditor.at(activeImageEditor)->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::MoveLayerUp{_layerPicker.getLayerIDSelected(activeImageEditor)});
            _layerPicker.moveLayerUp(activeImageEditor);
            break;
        case LayerPicker::Return::MoveLayerDown:
            _imageEditor.at(activeImageEditor)->unsavedChanges = true;
            AddWorkAndWait(CanvasWork::MoveLayerDown{_layerPicker.getLayerIDSelected(activeImageEditor)});
            _layerPicker.moveLayerDown(activeImageEditor);
            break;
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

void glxy::App::OpenImage(const filesystem::path& fileName)
{
    CanvasWorker::waitWork();
    CanvasWorker::AddEditor();
    _imageEditor.emplace_back(std::make_shared<ImageEditor>(settings, window, popUpState, cursor, cursorType, _colorPicker,
        _imageEditor.size() ? _imageEditor.back()->dockID : mainDockID, _toolPicker, _layerPicker, gizmoIcons, mainFont,
        CanvasWorker::getChunkManager(static_cast<EditorID>(_imageEditor.size())),
        CanvasWorker::getCommon(static_cast<EditorID>(_imageEditor.size())),
        static_cast<int16_t>(_imageEditor.size())));

    setActiveEditor(_imageEditor.size() - 1);
    _layerPicker.createNewImage();

    AddWorkAndWait(CanvasWork::ImageOpen{fileName});
    _imageEditor.back()->imagePath = fileName;
    AddRecentFile(fileName);
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
        _imageEditor.at(activeImageEditor)->Save();
        AddRecentFile(_imageEditor.at(activeImageEditor)->imagePath);
    }
    return true;
}