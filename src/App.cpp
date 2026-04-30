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

#include "Rendering/RenderShapes.hpp"
#include "UIElements/Cursors.hpp"


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

bool glxy::App::MenuItem(const char* label, const Vector2u iconID, const char* shortcut, const bool selected, const bool enabled) const
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->ChannelsSplit(2);

    drawList->ChannelsSetCurrent(1);

    const Vector2f originalPos = ImGui::GetCursorScreenPos();
    const Vector2f itemSize = Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight());

    constexpr int8_t textureColumns = 8;
    const Vector2f topLeft = Vector2f(1.f / textureColumns * iconID.x, 1.f / textureColumns * iconID.y);
    const Vector2f bottomRight = Vector2f(topLeft.x + 1.f / textureColumns, topLeft.y + 1.f / textureColumns);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, Vector2f(0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Color::Transparent);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, Color::Transparent);
    ImGui::PushStyleColor(ImGuiCol_Button, Color::Transparent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Color::Transparent);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Color::Transparent);

    bool clicked = false;
    bool hovered = false;

    if (ImGui::ImageButton((string(label) + "_ImageButton").c_str(), menuIcons.getNativeHandle(),
        Vector2f(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()), topLeft, bottomRight))
        clicked = true;

    if (ImGui::IsItemHovered())
        hovered = true;

    ImGui::PopStyleVar();
    ImGui::SameLine(0, 1);

    if (ImGui::MenuItem((string(" ") + label).c_str(), shortcut, selected, enabled))
        clicked = true;

    if (ImGui::IsItemHovered())
        hovered = true;

    ImGui::PopStyleColor(5);

    if (hovered)
    {
        drawList->ChannelsSetCurrent(0);

        ImGui::GetWindowDrawList()->AddRectFilled(originalPos, originalPos + itemSize,
            ImGui::GetColorU32(ImGuiCol_HeaderHovered));
    }

    drawList->ChannelsMerge();

    return clicked;
}

bool glxy::App::BeginMenu(const char* label, const Vector2u iconID, const bool enabled) const
{
    const Vector2f originalPos = ImGui::GetCursorScreenPos();
    const Vector2f itemSize = Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight());

    constexpr int8_t textureColumns = 8;
    const Vector2f topLeft = Vector2f(1.f / textureColumns * iconID.x, 1.f / textureColumns * iconID.y);
    const Vector2f bottomRight = Vector2f(topLeft.x + 1.f / textureColumns, topLeft.y + 1.f / textureColumns);

    bool clicked = false;
    bool hovered = false;
    static const char* hoveredLastFrame = nullptr;

    if (hoveredLastFrame && hoveredLastFrame == label)
    {
        ImGui::GetWindowDrawList()->AddRectFilled(originalPos, originalPos + itemSize,
            ImGui::GetColorU32(ImGuiCol_HeaderHovered));
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, Vector2f(0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Color::Transparent);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, Color::Transparent);
    ImGui::PushStyleColor(ImGuiCol_Button, Color::Transparent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Color::Transparent);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Color::Transparent);

    ImGui::Image(menuIcons.getNativeHandle(), Vector2f(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()), topLeft, bottomRight);

    ImGui::PopStyleVar();
    ImGui::SameLine(0, 1);

    if (ImGui::BeginMenu((string(" ") + label).c_str(), enabled))
        clicked = true;

    if (ImGui::IsItemHovered())
        hovered = true;

    ImGui::PopStyleColor(5);

    if (hovered)
        hoveredLastFrame = label;
    if (hoveredLastFrame == label && !hovered)
        hoveredLastFrame = nullptr;

    return clicked;
}

void glxy::App::Start(const filesystem::path& filename, const bool openWithGalaxy, const Clock& startUpTimer)
{
    openWithGalaxyFile = filename;
    this->openWithGalaxy = openWithGalaxy;
    GLOBAL.themeID = 0;

    settings.Load();
    adjSettings.Load();
    effSettings.Load();

    _toolPicker.windowOpen = settings.openWindow.at(0);
    _colorPicker.windowOpen = settings.openWindow.at(1);
    _layerPicker.windowOpen = settings.openWindow.at(2);

    {
        const string data = InternalResource::getResource(ID_RES3, "BINARY");
        Image temp;
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
    }

    RecreateAppWindow();

    window.clear();
    window.display();

    {
        Image temp, temp2;
        string data = InternalResource::getResource(ID_RES4, "BINARY");
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

        data = InternalResource::getResource(ID_RES9, "BINARY");
        if (!textIcons.loadFromMemory(data.data(), data.size())) return;
        textIcons.setSmooth(true);

        data = InternalResource::getResource(ID_RES10, "BINARY");
        if (!finalizeIcons.loadFromMemory(data.data(), data.size())) return;
        finalizeIcons.setSmooth(true);

        data = InternalResource::getResource(ID_RES11, "BINARY");
        MemoryInputStream mis(data.c_str(), data.length());
        Cursors::Setup(mis);

        data = InternalResource::getResource(ID_RES12, "BINARY");
        if (!menuIcons.loadFromMemory(data.data(), data.size())) return;
        menuIcons.setSmooth(true);
    }

    validate(ImGui::SFML::Init(window, false));
    mainFontData = InternalResource::getResource(ID_RES1, "BINARY");
    if (!mainFont.openFromMemory(mainFontData.data(), mainFontData.size()))
        return;

    LoadFonts();
    LoadShapes();

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
    CanvasWorker::ExitThread();

    settings.openWindow.at(0) = _toolPicker.windowOpen;
    settings.openWindow.at(1) = _colorPicker.windowOpen;
    settings.openWindow.at(2) = _layerPicker.windowOpen;

    settings.Save();
    adjSettings.Save();
    effSettings.Save();
    ImGui::SFML::Shutdown();
}

void glxy::App::LoadFonts()
{
    fontPaths.clear();
    fontPaths.push_back("(Built-in) Montserrat.ttf");
    if (filesystem::exists(settings.fontLocation))
    {
        for (const auto& n : filesystem::directory_iterator(settings.fontLocation))
        {
            if (n.is_directory())
            {
                for (const auto& m : filesystem::directory_iterator(n))
                    if (!m.is_directory() && find(c_fontExtensions.begin(), c_fontExtensions.end(), m.path().extension().string()) != c_fontExtensions.end())
                        fontPaths.push_back(m);
            }
            else if (find(c_fontExtensions.begin(), c_fontExtensions.end(), n.path().extension().string()) != c_fontExtensions.end())
                fontPaths.push_back(n);
        }
        std::sort(fontPaths.begin(), fontPaths.end(), [](const filesystem::path& a, const filesystem::path& b)
        {
            return a.filename().string() < b.filename().string();
        });
    }
    settings.fontID = std::clamp(static_cast<int32_t>(settings.fontID), 0, static_cast<int32_t>(fontPaths.size() - 1));
    if (settings.fontID == 0)
        textFont = std::make_shared<Font>(mainFont);
    else
        textFont = std::make_shared<Font>(fontPaths.at(settings.fontID));
    textFont->setSmooth(true);
}

void glxy::App::LoadShapes()
{
    const int8_t shapeSize = 32;
    RenderTexture texture;
    validate(shapeTextures.resize(Vector2u(static_cast<int16_t>(ShapeType::Count) * shapeSize, shapeSize)));
    shapeTextures.setSmooth(true);
    validate(texture.resize(Vector2u(shapeSize, shapeSize), {0U, 0U, 0U}));
    texture.setView(View(FloatRect(Vector2f(0, 0), Vector2f(shapeSize, shapeSize))));
    for (int8_t i = 0; i < static_cast<int8_t>(ShapeType::Count); i++)
    {
        texture.clear(Color::Transparent);
        ConvexShape shape;
        RenderShapes::getShape(shape, static_cast<ShapeType>(i), Vector2i(shapeSize, shapeSize), 4);
        texture.draw(shape);
        texture.display();
        shapeTextures.update(texture.getTexture(), Vector2u(i * shapeSize, 0));
    }
}

void glxy::App::RecreateAppWindow()
{
#if defined(SFML_SYSTEM_WINDOWS) || defined(SFML_SYSTEM_MACOS) || defined(SFML_SYSTEM_LINUX) || defined(SFML_SYSTEM_ANDROID)
    const int8_t GLTarget = 11;
#elif defined(SFML_SYSTEM_EMSCRIPTEN)
    const int8_t GLTarget = 20;
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

void glxy::App::CreateEmptyImage(const Vector2u resolution, const bool infiniteSize, const Color color)
{
    CanvasWorker::waitWork();
    CanvasWorker::AddEditor(infiniteSize);
    _imageEditor.emplace_back(std::make_shared<ImageEditor>(settings, window, popUpState, _colorPicker,
        !_imageEditor.empty() ? _imageEditor.back()->dockID : mainDockID, _toolPicker, _layerPicker, gizmoIcons, mainFont,
        CanvasWorker::getChunkManager(static_cast<EditorID>(_imageEditor.size())),
        CanvasWorker::getCommon(static_cast<EditorID>(_imageEditor.size())),
        static_cast<int16_t>(_imageEditor.size()), infiniteSize, textFont));
    setActiveEditor(_imageEditor.size() - 1);
    _layerPicker.createNewImage(infiniteSize);
    AddWork(CanvasWork::ImageEmpty{resolution, color});
    _imageEditor.back()->imagePath.clear();
}

bool glxy::App::hasUnsavedImages() const
{
    for (auto& n : _imageEditor)
        if (n->unsavedChanges)
            return true;
    return false;
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
        if (MenuItem("fileMenu[0]"_C, Vector2u(0, 0), Shortcuts::getName(ActionShortcut::NewImage).c_str()))
            MenuNew();
        if (MenuItem("fileMenu[1]"_C, Vector2u(1, 0), Shortcuts::getName(ActionShortcut::OpenImage).c_str()))
            MenuOpen();
        ImGui::BeginDisabled(activeImageEditor == -1 || _imageEditor.at(activeImageEditor)->isInfinite);
        if (MenuItem("fileMenu[2]"_C, Vector2u(2, 0), Shortcuts::getName(ActionShortcut::SaveImage).c_str()))
            MenuSave();
        if (MenuItem("fileMenu[3]"_C, Vector2u(3, 0), Shortcuts::getName(ActionShortcut::SaveImageAs).c_str()))
            MenuSaveAs();
        ImGui::EndDisabled();
        if (MenuItem("fileMenu[4]"_C, Vector2u(4, 0)))
            MenuExit(false);
        ImGui::EndMenu();
    }
    if (popUpState.empty())
    {
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::NewImage])
            MenuNew();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::OpenImage])
            MenuOpen();
        if (activeImageEditor >= 0 && !_imageEditor.at(activeImageEditor)->isInfinite)
        {
            if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::SaveImage])
                MenuSave();
            if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::SaveImageAs])
                MenuSaveAs();
        }
    }
    ImGui::BeginDisabled(activeImageEditor == -1 || _imageEditor.at(activeImageEditor)->isInfinite);
    if (ImGui::BeginMenu("Edit"_C))
    {
        ImageEditor* active = activeImageEditor == -1 ? nullptr : _imageEditor.at(activeImageEditor).get();
        ImGui::BeginDisabled(!active || !active->chunkManager.anyHasSelectionLayer());
        if (MenuItem("editMenu[0]"_C, Vector2u(5, 0), Shortcuts::getName(ActionShortcut::Copy).c_str()))
            MenuCopy();
        if (MenuItem("editMenu[1]"_C, Vector2u(6, 0), Shortcuts::getName(ActionShortcut::Cut).c_str()))
            MenuCut();
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!clipboardImage);
        if (MenuItem("editMenu[2]"_C, Vector2u(7, 0), Shortcuts::getName(ActionShortcut::Paste).c_str()))
            MenuPaste();
        ImGui::EndDisabled();
        if (BeginMenu("editMenu[3]"_C, Vector2u(0, 1)))
        {
            if (MenuItem("selectSubmenu[0]"_C, Vector2u(1, 1), Shortcuts::getName(ActionShortcut::SelectAll).c_str()))
                MenuSelectAll();
            if (MenuItem("selectSubmenu[1]"_C, Vector2u(2, 1)))
                MenuSelectLeft();
            if (MenuItem("selectSubmenu[2]"_C, Vector2u(3, 1)))
                MenuSelectRight();
            if (MenuItem("selectSubmenu[3]"_C, Vector2u(4, 1)))
                MenuSelectTop();
            if (MenuItem("selectSubmenu[4]"_C, Vector2u(5, 1)))
                MenuSelectBottom();
            if (MenuItem("selectSubmenu[5]"_C, Vector2u(6, 1)))
                MenuSelectTopLeft();
            if (MenuItem("selectSubmenu[6]"_C, Vector2u(7, 1)))
                MenuSelectTopRight();
            if (MenuItem("selectSubmenu[7]"_C, Vector2u(0, 2)))
                MenuSelectBottomLeft();
            if (MenuItem("selectSubmenu[8]"_C, Vector2u(1, 2)))
                MenuSelectBottomRight();
            ImGui::EndMenu();
        }
        ImGui::BeginDisabled(!active || !active->chunkManager.anyHasSelectionLayer());
        if (MenuItem("editMenu[4]"_C, Vector2u(2, 2), Shortcuts::getName(ActionShortcut::DeselectAll).c_str()))
            MenuDeselectAll();
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!active || !active->chunkManager.anyHasSelectionLayer() && !active->chunkManager.anyHasSelectionTempLayer());
        if (MenuItem("editMenu[5]"_C, Vector2u(3, 2), Shortcuts::getName(ActionShortcut::Delete).c_str()))
            MenuDelete();
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }
    ImGui::EndDisabled();
    if (activeImageEditor >= 0 && !_imageEditor.at(activeImageEditor)->isInfinite)
    {
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Copy] && _imageEditor.at(activeImageEditor)->chunkManager.anyHasSelectionLayer())
            MenuCopy();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Cut] && _imageEditor.at(activeImageEditor)->chunkManager.anyHasSelectionLayer())
            MenuCut();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Paste] && clipboardImage)
            MenuPaste();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::SelectAll])
            MenuSelectAll();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::DeselectAll] && _imageEditor.at(activeImageEditor)->chunkManager.anyHasSelectionLayer())
            MenuDeselectAll();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Delete] && (_imageEditor.at(activeImageEditor)->chunkManager.anyHasSelectionLayer() ||
            _imageEditor.at(activeImageEditor)->chunkManager.anyHasSelectionTempLayer()))
            MenuDelete();
    }
    if (ImGui::MenuItem("Settings"_C) || Shortcuts()[ActionShortcut::OpenSettings])
        popUpState.push_back(PopUpState::Settings);
    ImGui::BeginDisabled(activeImageEditor == -1);
    if (ImGui::BeginMenu("View"_C))
    {
        ImageEditor* active = activeImageEditor == -1 ? nullptr : _imageEditor.at(activeImageEditor).get();
        if (MenuItem("viewMenu[0]"_C, Vector2u(4, 2), Shortcuts::getName(ActionShortcut::ZoomIn).c_str()))
            MenuZoomIn();
        if (MenuItem("viewMenu[1]"_C, Vector2u(5, 2), Shortcuts::getName(ActionShortcut::ZoomOut).c_str()))
            MenuZoomOut();
        if (BeginMenu("viewMenu[2]"_C, Vector2u(6, 2)))
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
        if (BeginMenu("viewMenu[3]"_C, Vector2u(7, 2)))
        {
            if (ImGui::MenuItem("windowName[0]"_C))
                _toolPicker.windowOpen = true;
            if (ImGui::MenuItem("windowName[1]"_C))
                _colorPicker.windowOpen = true;
            if (ImGui::MenuItem("windowName[2]"_C))
                _layerPicker.windowOpen = true;
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (MenuItem("viewMenu[4]"_C, Vector2u(0, 3), nullptr, settings.showGrid))
            MenuGrid();
        if (MenuItem("viewMenu[5]"_C, Vector2u(1, 3), (to_string(settings.gridBold.x) + " x " + to_string(settings.gridBold.y)).c_str()))
            MenuGridBold();
        if (MenuItem("viewMenu[6]"_C, Vector2u(2, 3), nullptr, settings.showRuler))
            MenuRuler();
        if (MenuItem("viewMenu[7]"_C, Vector2u(3, 3)))
            MenuActualSize();
        if (MenuItem("viewMenu[8]"_C, Vector2u(4, 3), nullptr, settings.syncViewport))
            MenuSyncViewport();
        ImGui::EndMenu();
    }
    ImGui::BeginDisabled(activeImageEditor == -1 || _imageEditor.at(activeImageEditor)->isInfinite);
    if (ImGui::BeginMenu("Image"_C))
    {
        ImageEditor* active = activeImageEditor == -1 ? nullptr : _imageEditor.at(activeImageEditor).get();
        ImGui::BeginDisabled(!active || !active->chunkManager.anyHasSelectionLayer());
        if (MenuItem("imageMenu[0]"_C, Vector2u(5, 3), Shortcuts::getName(ActionShortcut::Crop).c_str()))
            MenuCrop();
        ImGui::EndDisabled();
        ImGui::Separator();
        if (MenuItem("imageMenu[1]"_C, Vector2u(6, 3), Shortcuts::getName(ActionShortcut::Resize).c_str()))
            MenuResize();
        if (MenuItem("imageMenu[2]"_C, Vector2u(7, 3), Shortcuts::getName(ActionShortcut::ResizeCanvas).c_str()))
            MenuResizeCanvas();
        ImGui::Separator();
        if (MenuItem("imageMenu[3]"_C, Vector2u(0, 4)))
            MenuFlipImageHorizontal();
        if (MenuItem("imageMenu[4]"_C, Vector2u(1, 4)))
            MenuFlipImageVertical();
        ImGui::Separator();
        if (MenuItem("imageMenu[5]"_C, Vector2u(2, 4)))
            MenuRotate90CW();
        if (MenuItem("imageMenu[6]"_C, Vector2u(3, 4)))
            MenuRotate90CCW();
        if (MenuItem("imageMenu[7]"_C, Vector2u(4, 4)))
            MenuRotate180();
        ImGui::Separator();
        if (MenuItem("imageMenu[8]"_C, Vector2u(5, 4), Shortcuts::getName(ActionShortcut::TransformImage).c_str()))
            MenuTransformImage();
        ImGui::EndMenu();
    }
    ImGui::EndDisabled();
    if (activeImageEditor >= 0 && !_imageEditor.at(activeImageEditor)->isInfinite)
    {
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Crop] && _imageEditor.at(activeImageEditor)->chunkManager.anyHasSelectionLayer())
            MenuCrop();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Resize])
            MenuResize();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::ResizeCanvas])
            MenuResizeCanvas();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::TransformImage])
            MenuTransformImage();
    }
    if (ImGui::BeginMenu("Layers"_C))
    {
        ImageEditor* active = activeImageEditor == -1 ? nullptr : _imageEditor.at(activeImageEditor).get();
        if (MenuItem("layerMenu[0]"_C, Vector2u(6, 4)))
            MenuNewLayer();
        ImGui::BeginDisabled(!active || active->chunkManager.getLayerCount() == 1);
        if (MenuItem("layerMenu[1]"_C, Vector2u(7, 4)))
            MenuDeleteLayer();
        ImGui::EndDisabled();
        if (MenuItem("layerMenu[2]"_C, Vector2u(0, 5)))
            MenuDuplicateLayer();
        ImGui::Separator();
        ImGui::BeginDisabled(!active || _layerPicker.getLayerIDSelected(activeImageEditor) >= active->chunkManager.getLayerCount() - 1);
        if (MenuItem("layerMenu[3]"_C, Vector2u(1, 5)))
            MenuMoveLayerUp();
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!active || _layerPicker.getLayerIDSelected(activeImageEditor) <= 0);
        if (MenuItem("layerMenu[4]"_C, Vector2u(2, 5)))
            MenuMoveLayerDown();
        if (MenuItem("layerMenu[5]"_C, Vector2u(3, 5)))
            MenuMergeLayerDown();
        ImGui::EndDisabled();
        ImGui::Separator();
        if (MenuItem("layerMenu[6]"_C, Vector2u(4, 5)))
            MenuFlipImageHorizontal();
        if (MenuItem("layerMenu[7]"_C, Vector2u(5, 5)))
            MenuFlipImageVertical();
        ImGui::Separator();
        if (MenuItem("layerMenu[8]"_C, Vector2u(6, 5)))
            MenuLayerProperties();
        ImGui::EndMenu();
    }
    ImGui::BeginDisabled(activeImageEditor == -1 || _imageEditor.at(activeImageEditor)->isInfinite);
    if (ImGui::BeginMenu("Adjustments"_C))
    {
        if (MenuItem("adjustMenu[0]"_C, Vector2u(7, 5)))
            MenuAdjustBlackAndWhite();
        if (MenuItem("adjustMenu[1]"_C, Vector2u(0, 6)))
            MenuAdjustBrightnessContrast();
        if (MenuItem("adjustMenu[2]"_C, Vector2u(1, 6)))
            MenuAdjustHSV();
        if (MenuItem("adjustMenu[3]"_C, Vector2u(2, 6)))
            MenuAdjustInvert();
        if (MenuItem("adjustMenu[4]"_C, Vector2u(3, 6)))
            MenuAdjustTint();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Effects"_C))
    {
        if (MenuItem("effectMenu[0]"_C, Vector2u(4, 6)))
            MenuEffectGauss();
        if (MenuItem("effectMenu[1]"_C, Vector2u(5, 6)))
            MenuEffectBox();
        if (MenuItem("effectMenu[2]"_C, Vector2u(6, 6)))
            MenuEffectDirectional();
        ImGui::Separator();
        if (MenuItem("effectMenu[3]"_C, Vector2u(7, 6)))
            MenuEffectWhite();
        if (MenuItem("effectMenu[4]"_C, Vector2u(0, 7)))
            MenuEffectFractal();
        ImGui::EndMenu();
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();

    if (ImGui::BeginMenu("Help"_C))
    {
        if (MenuItem("otherMenu[0]"_C, Vector2u(1, 7)))
            MenuChangelog();
        if (MenuItem("otherMenu[1]"_C, Vector2u(2, 7)))
            MenuFuturePlan();
        if (MenuItem("otherMenu[2]"_C, Vector2u(3, 7)))
            MenuGLScan();
        if (MenuItem("otherMenu[3]"_C, Vector2u(4, 7), Shortcuts::getName(ActionShortcut::Debug).c_str(), settings.debugMode))
            MenuDebug();
        if (MenuItem("otherMenu[4]"_C, Vector2u(5, 7)))
            MenuAbout();
        ImGui::EndMenu();
    }
    if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Debug])
        MenuDebug();

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
            if (!(_toolPicker.getToolsEnabled() >> i & 1))
                continue;
            if (ImGui::Selectable(LL::ind("toolName[]", i).c_str()))
                _toolPicker.setTool(static_cast<Tool>(i));
        }
        ImGui::EndCombo();
    }
    switch (_toolPicker.getTool())
    {
    case Tool::BoxSelect: case Tool::CircleSelect: case Tool::LassoSelect:
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
    case Tool::MoveSelected: case Tool::MoveSelection:
    {
        bool statement = false;
        ImGui::SameLine();
        if (activeImageEditor >= 0)
        {
            lock_guard lock(_imageEditor.at(activeImageEditor)->common.mtxEditorWorkerCommon);
            if (_toolPicker.getTool() == Tool::MoveSelected)
                statement = _imageEditor.at(activeImageEditor)->common.getMoveSelected();
            else
                statement = _imageEditor.at(activeImageEditor)->common.getMoveSelection();
        }
        ImGui::BeginDisabled(!statement);
        if (ImGui::ImageButton("Cancel"_C, finalizeIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
            Vector2f(0, 0), Vector2f(0.5f, 1)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
        {
            _imageEditor.at(activeImageEditor)->ResetMovePixels();
            popUpState.push_back(PopUpState::ThreadWork);
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Cancel"_C);
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if (ImGui::ImageButton("Finish"_C, finalizeIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
            Vector2f(0.5f, 0), Vector2f(1, 1)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
        {
            _imageEditor.at(activeImageEditor)->FinishMovePixels();
            popUpState.push_back(PopUpState::ThreadWork);
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Finish"_C);
            ImGui::EndTooltip();
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
        if (ImGui::ImageButton("Cancel"_C, finalizeIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
            Vector2f(0, 0), Vector2f(0.5f, 1)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
        {
            AddWorkAndWait(CanvasWork::Cancel{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Cancel"_C);
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if (ImGui::ImageButton("Finish"_C, finalizeIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
            Vector2f(0.5f, 0), Vector2f(1, 1)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
        {
            AddWorkAndWait(CanvasWork::Finish{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Finish"_C);
            ImGui::EndTooltip();
        }
        ImGui::EndDisabled();
        break;
    }
    case Tool::Brush:
        ImGui::SameLine();
        ImGui::PushItemWidth(120.f);
        ImGui::InputFloat("Radius"_C, &settings.brushRadius, 1, 5, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            settings.brushRadius = std::clamp(settings.brushRadius, 0.5f, 1000.f);
            for (auto& n : _imageEditor)
                n->OptionSetBrushSize(settings.brushRadius);
        }
        break;
    case Tool::Eraser:
        ImGui::SameLine();
        ImGui::PushItemWidth(120.f);
        ImGui::InputFloat("Radius"_C, &settings.eraserRadius, 1, 5, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            settings.eraserRadius = std::clamp(settings.eraserRadius, 0.5f, 1000.f);
            for (auto& n : _imageEditor)
                n->OptionSetBrushSize(settings.eraserRadius);
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
        if (ImGui::ImageButton("Cancel"_C, finalizeIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
            Vector2f(0, 0), Vector2f(0.5f, 1)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
        {
            AddWorkAndWait(CanvasWork::Cancel{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Cancel"_C);
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if (ImGui::ImageButton("Finish"_C, finalizeIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
            Vector2f(0.5f, 0), Vector2f(1, 1)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
        {
            AddWorkAndWait(CanvasWork::Finish{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Finish"_C);
            ImGui::EndTooltip();
        }
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
        if (ImGui::ImageButton("Cancel"_C, finalizeIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
            Vector2f(0, 0), Vector2f(0.5f, 1)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
        {
            AddWorkAndWait(CanvasWork::Cancel{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Cancel"_C);
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if (ImGui::ImageButton("Finish"_C, finalizeIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
            Vector2f(0.5f, 0), Vector2f(1, 1)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
        {
            AddWorkAndWait(CanvasWork::Finish{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Finish"_C);
            ImGui::EndTooltip();
        }
        ImGui::EndDisabled();
        break;
    }
    case Tool::ColorSwap:
    {
        ImGui::SameLine();
        ImGui::PushItemWidth(120.f);
        ImGui::InputFloat("Radius"_C, &settings.colorSwapRadius, 1, 5, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            settings.colorSwapRadius = std::clamp(settings.colorSwapRadius, 0.5f, 200.f);
            for (auto& n : _imageEditor)
                n->OptionSetBrushSize(settings.colorSwapRadius);
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(200.f);
        int32_t tolerance = settings.colorSwapTolerance;
        if (ImGui::SliderInt("Tolerance"_C, &tolerance, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp))
            settings.colorSwapTolerance = tolerance;
        break;
    }
    case Tool::Shapes:
    {
        ImGui::SameLine();
        bool statement = false;
        if (activeImageEditor >= 0)
        {
            lock_guard lock(_imageEditor.at(activeImageEditor)->common.mtxEditorWorkerCommon);
            statement = _imageEditor.at(activeImageEditor)->common.getShapeDraw();
        }
        if (ImGui::BeginCombo("Shape"_C, LL::ind("shapeName[]", settings.shapeID).c_str()))
        {
            const float uvSizeX = 1.f / static_cast<int8_t>(ShapeType::Count);
            for (int8_t i = 0; i < static_cast<int8_t>(ShapeType::Count); i++)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, settings.shapeID == i ? 2.f : 0.f);
                if (ImGui::ImageButton(LL::ind("shapeName[]", i).c_str(), shapeTextures.getNativeHandle(), Vector2f(24, 24) * settings.GUIScale,
                    Vector2f(uvSizeX * i, 0), Vector2f(uvSizeX * (i + 1), 1)))
                {
                    settings.shapeID = i;
                    if (statement)
                    {
                        RenderShapes::getShape(_imageEditor.at(activeImageEditor)->shape, static_cast<ShapeType>(i),
                            _imageEditor.at(activeImageEditor)->shapeSize, settings.shapeRadius);
                        AddWork(CanvasWork::ShapePixels{std::make_shared<ConvexShape>(_imageEditor.at(activeImageEditor)->shape),
                            _layerPicker.getLayerIDSelected(activeImageEditor)});
                    }
                }
                ImGui::PopStyleVar();
                if (ImGui::BeginItemTooltip())
                {
                    ImGui::Text("%s", LL::ind("shapeName[]", i).c_str());
                    ImGui::EndTooltip();
                }
                if (i % 4 != 3)
                    ImGui::SameLine();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(80.f * settings.GUIScale);
        if (ImGui::BeginCombo("##MoreOptions", "More"_C))
        {
            switch (static_cast<ShapeType>(settings.shapeID))
            {
            case ShapeType::RoundedRectangle: case ShapeType::Star3:
            case ShapeType::Star4: case ShapeType::Star5:
                ImGui::BeginDisabled(false);
                break;
            default:
                ImGui::BeginDisabled(true);
                break;
            }
            ImGui::PushItemWidth(110.f * settings.GUIScale);
            ImGui::InputFloat("Radius"_C, &settings.shapeRadius, 1, 5, "%.1f");
            if (statement && ImGui::IsItemDeactivatedAfterEdit())
            {
                settings.shapeRadius = max(0, static_cast<int32_t>(settings.shapeRadius));
                RenderShapes::getShape(_imageEditor.at(activeImageEditor)->shape, static_cast<ShapeType>(settings.shapeID),
                    _imageEditor.at(activeImageEditor)->shapeSize, settings.shapeRadius);
                AddWork(CanvasWork::ShapePixels{std::make_shared<ConvexShape>(_imageEditor.at(activeImageEditor)->shape),
                    _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            ImGui::EndDisabled();
            ImGui::PushItemWidth(110.f * settings.GUIScale);
            ImGui::InputFloat("Outline thickness"_C, &settings.shapeOutlineThickness, 1, 5, "%.2f");
            if (statement && ImGui::IsItemDeactivatedAfterEdit())
            {
                settings.shapeOutlineThickness = max(0, static_cast<int32_t>(settings.shapeOutlineThickness));
                _imageEditor.at(activeImageEditor)->shape.setOutlineThickness(settings.shapeOutlineThickness);
                AddWork(CanvasWork::ShapePixels{std::make_shared<ConvexShape>(_imageEditor.at(activeImageEditor)->shape),
                    _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!statement);
        if (ImGui::ImageButton("Cancel"_C, finalizeIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
            Vector2f(0, 0), Vector2f(0.5f, 1)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
        {
            AddWorkAndWait(CanvasWork::Cancel{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Cancel"_C);
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if (ImGui::ImageButton("Finish"_C, finalizeIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
            Vector2f(0.5f, 0), Vector2f(1, 1)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
        {
            AddWorkAndWait(CanvasWork::Finish{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Finish"_C);
            ImGui::EndTooltip();
        }
        ImGui::EndDisabled();
        break;
    }
    case Tool::Text:
    {
        bool statement = false;
        if (activeImageEditor >= 0)
        {
            lock_guard lock(_imageEditor.at(activeImageEditor)->common.mtxEditorWorkerCommon);
            statement = _imageEditor.at(activeImageEditor)->common.getTextDraw();
        }
        float textSize = settings.textSize;
        ImGui::SameLine();
        if (ImGui::BeginCombo("Font"_C, fontPaths.at(
            std::clamp(static_cast<int32_t>(settings.fontID), 0, static_cast<int32_t>(fontPaths.size()) - 1)).filename().string().c_str()))
        {
            for (uint16_t i = 0; i < fontPaths.size(); i++)
            {
                if (ImGui::Selectable(fontPaths.at(i).filename().string().c_str()))
                {
                    settings.fontID = i;
                    if (i == 0)
                        textFont = std::make_shared<Font>(mainFont);
                    else
                        textFont = std::make_shared<Font>(fontPaths.at(i));
                    textFont->setSmooth(true);
                    if (statement)
                    {
                        _imageEditor.at(activeImageEditor)->text.setFont(*textFont);
                        _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                        AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                            _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
                    }
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(110.f * settings.GUIScale);
        if (ImGui::InputFloat("Size"_C, &textSize, 0.25f, 0.25f, "%.0f"))
        {
            const float t = fmod(textSize, 1.f);
            if (t == 0)
            {
                settings.textSize = fmax(textSize, 1.f);
                if (statement)
                {
                    _imageEditor.at(activeImageEditor)->text.setCharacterSize(settings.textSize);
                    _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                    AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                        _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
                }
            }
            else
            {
                int8_t idAbove = 0;
                int8_t idBelow = 0;
                for (int8_t i = 0; i < c_textSizes.size(); i++)
                {
                    if (c_textSizes.at(i) < settings.textSize)
                    {
                        idBelow = i;
                        idAbove = i + 1;
                    }
                    if (settings.textSize == c_textSizes.at(i))
                    {
                        idBelow = i - 1;
                        idAbove = i + 1;
                        break;
                    }
                }
                if (t > 0.5f)
                {
                    settings.textSize = c_textSizes.at(max(static_cast<int32_t>(idBelow), 0));
                    if (statement)
                    {
                        _imageEditor.at(activeImageEditor)->text.setCharacterSize(settings.textSize);
                        _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                        AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                            _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
                    }
                }
                else if (t < 0.5f)
                {
                    settings.textSize = c_textSizes.at(min(static_cast<int32_t>(idAbove), static_cast<int32_t>(c_textSizes.size() - 1)));
                    if (statement)
                    {
                        _imageEditor.at(activeImageEditor)->text.setCharacterSize(settings.textSize);
                        _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                        AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                            _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
                    }
                }
            }
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(80.f * settings.GUIScale);
        if (ImGui::BeginCombo("##MoreOptions", "More"_C))
        {
            const array iconsMain = { "Align left"_S, "Align center"_S, "Align right"_S };
            const array iconsOptional = {"Bold"_S, "Italic"_S, "Underlined"_S, "Strikethrough"_S};
            const float uvSizeX = 1.f / (iconsMain.size() + iconsOptional.size());
            for (int8_t i = 0; i < iconsOptional.size(); i++)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, settings.textStyle >> i & 1 ? 2.f : 0.f);
                if (ImGui::ImageButton(iconsOptional.at(i).c_str(), textIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
                    Vector2f(uvSizeX * i, 0), Vector2f(uvSizeX * (i + 1), 1)))
                {
                    settings.textStyle ^= 1 << i;
                    if (statement)
                    {
                        _imageEditor.at(activeImageEditor)->text.setStyle(settings.textStyle);
                        _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                        AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                            _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
                    }
                }
                ImGui::PopStyleVar();
                if (ImGui::BeginItemTooltip())
                {
                    ImGui::Text("%s", iconsOptional.at(i).c_str());
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
            }
            for (int8_t i = 0; i < iconsMain.size(); i++)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, settings.textAlignment == i ? 2.f : 0.f);
                if (ImGui::ImageButton(iconsMain.at(i).c_str(), textIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
                    Vector2f(uvSizeX * (i + iconsOptional.size()), 0), Vector2f(uvSizeX * (i + iconsOptional.size() + 1), 1)))
                {
                    settings.textAlignment = i;
                    if (statement)
                    {
                        switch (settings.textAlignment)
                        {
                        case 0: _imageEditor.at(activeImageEditor)->text.setLineAlignment(Text::LineAlignment::Left); break;
                        case 1: _imageEditor.at(activeImageEditor)->text.setLineAlignment(Text::LineAlignment::Center); break;
                        case 2: _imageEditor.at(activeImageEditor)->text.setLineAlignment(Text::LineAlignment::Right); break;
                        }
                        _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                        AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                            _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
                    }
                }
                ImGui::PopStyleVar();
                if (ImGui::BeginItemTooltip())
                {
                    ImGui::Text("%s", iconsMain.at(i).c_str());
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
            }
            ImGui::Spacing();
            ImGui::PushItemWidth(110.f * settings.GUIScale);
            ImGui::InputFloat("Outline thickness"_C, &settings.textOutlineThickness, 1, 5, "%.2f");
            if (statement && ImGui::IsItemDeactivatedAfterEdit())
            {
                settings.textOutlineThickness = fabsf(settings.textOutlineThickness);
                _imageEditor.at(activeImageEditor)->text.setOutlineThickness(settings.textOutlineThickness);
                _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                    _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            ImGui::PushItemWidth(110.f * settings.GUIScale);
            ImGui::InputFloat("Letter spacing"_C, &settings.letterSpacing, 1, 5, "%.2f");
            if (statement && ImGui::IsItemDeactivatedAfterEdit())
            {
                _imageEditor.at(activeImageEditor)->text.setLetterSpacing(settings.letterSpacing);
                _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                    _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            ImGui::PushItemWidth(110.f * settings.GUIScale);
            ImGui::InputFloat("Line spacing"_C, &settings.lineSpacing, 1, 5, "%.2f");
            if (statement && ImGui::IsItemDeactivatedAfterEdit())
            {
                _imageEditor.at(activeImageEditor)->text.setLineSpacing(settings.lineSpacing);
                _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                    _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!statement);
        if (ImGui::ImageButton("Cancel"_C, finalizeIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
            Vector2f(0, 0), Vector2f(0.5f, 1)))
        {
            _imageEditor.at(activeImageEditor)->textString.clear();
            _imageEditor.at(activeImageEditor)->wantInput = false;
            AddWorkAndWait(CanvasWork::Cancel{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Cancel"_C);
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if (ImGui::ImageButton("Finish"_C, finalizeIcons.getNativeHandle(), Vector2f(15, 15) * settings.GUIScale,
            Vector2f(0.5f, 0), Vector2f(1, 1)))
        {
            _imageEditor.at(activeImageEditor)->textString.clear();
            _imageEditor.at(activeImageEditor)->wantInput = false;
            AddWorkAndWait(CanvasWork::Finish{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Finish"_C);
            ImGui::EndTooltip();
        }
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
            if (lock_guard lock(n->common.mtxEditorWorkerCommon);
                n->common.getShapeDraw())
            {
                _imageEditor.at(activeImageEditor)->shape.setFillColor(_colorPicker.getColor(0));
                _imageEditor.at(activeImageEditor)->shape.setOutlineColor(_colorPicker.getColor(1));
                AddWork(CanvasWork::ShapePixels{ std::make_shared<ConvexShape>(_imageEditor.at(activeImageEditor)->shape),
                    _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            if (lock_guard lock(n->common.mtxEditorWorkerCommon);
                n->common.getTextDraw())
            {
                _imageEditor.at(activeImageEditor)->text.setFillColor(_colorPicker.getColor(0));
                _imageEditor.at(activeImageEditor)->text.setOutlineColor(_colorPicker.getColor(1));
                AddWork(CanvasWork::TextPixels{ textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                    _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
        }
    }
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
                MenuExit(true);
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
                setActiveEditor(i);
            if (_imageEditor.at(i)->windowHovered)
                setHoveredEditor(i);
        }
        if (!someEditorHovered && Cursors::getCursor() != Cursors::Type::Arrow)
            Cursors::setCursor(Cursors::Type::Arrow, window);
        if (!someEditorHovered && activeImageEditor != -1)
        {
            _toolPicker.setTool(_imageEditor.at(activeImageEditor)->currentTool, false);
            _toolPicker.setToolsEnabled(_imageEditor.at(activeImageEditor)->isInfinite ? c_infiniteToolsEnabled : 0xFFFFFF);
            for (int8_t i = 0; i < c_colorCount; i++)
                _colorPicker.setEditorColors(i, _imageEditor.at(activeImageEditor)->currentColor.at(i));
        }
        else if (hoveredImageEditor != -1)
        {
            _toolPicker.setTool(_imageEditor.at(hoveredImageEditor)->currentTool, false);
            _toolPicker.setToolsEnabled(_imageEditor.at(hoveredImageEditor)->isInfinite ? c_infiniteToolsEnabled : 0xFFFFFF);
            for (int8_t i = 0; i < c_colorCount; i++)
                _colorPicker.setEditorColors(i, _imageEditor.at(hoveredImageEditor)->currentColor.at(i));
        }
        else if (activeImageEditor != -1)
        {
            _toolPicker.setTool(_imageEditor.at(activeImageEditor)->currentTool, false);
            _toolPicker.setToolsEnabled(_imageEditor.at(activeImageEditor)->isInfinite ? c_infiniteToolsEnabled : 0xFFFFFF);
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
        if (activeImageEditor >= 0 && Shortcuts()[ActionShortcut::Search] && !GLOBAL.wantInput && popUpState.empty())
        {
            if (lock_guard lock(_imageEditor.at(activeImageEditor)->common.mtxEditorWorkerCommon);
                !_imageEditor.at(activeImageEditor)->common.getTextDraw())
                popUpState.push_back(PopUpState::Search);
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
    CanvasWorker::AddEditor(false);
    _imageEditor.emplace_back(std::make_shared<ImageEditor>(settings, window, popUpState, _colorPicker,
        _imageEditor.size() ? _imageEditor.back()->dockID : mainDockID, _toolPicker, _layerPicker, gizmoIcons, mainFont,
        CanvasWorker::getChunkManager(static_cast<EditorID>(_imageEditor.size())),
        CanvasWorker::getCommon(static_cast<EditorID>(_imageEditor.size())),
        static_cast<int16_t>(_imageEditor.size()), false, textFont));

    setActiveEditor(_imageEditor.size() - 1);
    _layerPicker.createNewImage(false);

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