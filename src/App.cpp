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
#include "ZEditorsCommon/FileExplorer.hpp"


#ifdef SFML_SYSTEM_ANDROID
#include <SFML/System/NativeActivity.hpp>
#include <android/native_activity.h>
#endif

glxy::App::App()
    : _colorPicker(window, actionIcons), _toolPicker(toolIcons)
{
    //load languages
    const shared_ptr<InputStream> stream = InternalResource::getResource(ID_RES2);
    if (!stream)
        return;
    string t;
    t.resize(*stream->getSize());
    validate(stream->read(t.data(), t.size()));
    LL::load(t);
}

glxy::App::~App()
{
    if (filesystem::exists("pdo.pdo"))
        filesystem::remove("pdo.pdo");
}

bool glxy::App::MenuItem(const char* label, const uint8_t iconID, const char* shortcut, const bool selected, const bool enabled) const
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->ChannelsSplit(2);

    drawList->ChannelsSetCurrent(1);

    const Vector2f originalPos = ImGui::GetCursorScreenPos();
    const Vector2f itemSize = Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight());

    constexpr int8_t textureColumns = 8;
    constexpr int8_t textureRows = 9;
    const Vector2f topLeft = Vector2f(1.f / textureColumns * (iconID % textureColumns), 1.f / textureRows * (iconID / textureColumns));
    const Vector2f bottomRight = Vector2f(topLeft.x + 1.f / textureColumns, topLeft.y + 1.f / textureRows);

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

bool glxy::App::BeginMenu(const char* label, const uint8_t iconID, const bool enabled) const
{
    const Vector2f originalPos = ImGui::GetCursorScreenPos();
    const Vector2f itemSize = Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight());

    constexpr int8_t textureColumns = 8;
    constexpr int8_t textureRows = 9;
    const Vector2f topLeft = Vector2f(1.f / textureColumns * (iconID % textureColumns), 1.f / textureRows * (iconID / textureColumns));
    const Vector2f bottomRight = Vector2f(topLeft.x + 1.f / textureColumns, topLeft.y + 1.f / textureRows);

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

    config.Load();
    adjEffConfig.Load();
    fileExplorer.Load();

    _toolPicker.windowOpen = config.openWindow.at(0);
    _colorPicker.windowOpen = config.openWindow.at(1);
    _layerPicker.windowOpen = config.openWindow.at(2);

    {

        Image temp;
        if (const shared_ptr<InputStream> stream = InternalResource::getResource(ID_RES3);
            !stream || !temp.loadFromStream(*stream))
            return;
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
        Image temp;
        if (const shared_ptr<InputStream> stream = InternalResource::getResource(ID_RES4);
            !stream || !toolIcons.loadFromStream(*stream))
            return;
        toolIcons.setSmooth(true);

        if (const shared_ptr<InputStream> stream = InternalResource::getResource(ID_RES5);
            !stream || !setupSelect.loadFromStream(*stream))
            return;
        setupSelect.setSmooth(true);

        if (const shared_ptr<InputStream> stream = InternalResource::getResource(ID_RES6);
            !stream || !textIcons.loadFromStream(*stream))
            return;
        textIcons.setSmooth(true);

        if (const shared_ptr<InputStream> stream = InternalResource::getResource(ID_RES8);
            !stream || !actionIcons.loadFromStream(*stream))
            return;
        actionIcons.setSmooth(true);

        if (const shared_ptr<InputStream> stream = InternalResource::getResource(ID_RES8);
            !stream || !fileExplorer.loadTexture(*stream))
            return;

        shared_ptr<InputStream> stream = InternalResource::getResource(ID_RES8);
        Cursors::Setup(*stream);

        stream = InternalResource::getResource(ID_RES7);
        if (!menuIcons.loadFromStream(*stream)) return;
        menuIcons.setSmooth(true);

        const uint8_t iconColumns = 8;
        const array layerIconIDs = {44, 45, 46, 47, 48, 49, 52};
        const Image iconsImage(*stream);
        temp.resize(Vector2u((24 + 2) * layerIconIDs.size(), 24 + 2), Color::Transparent);
        for (int8_t i = 0; i < 7; i++)
            validate(temp.copy(iconsImage, Vector2u(1 + (24 + 2) * i, 1),
                IntRect(Vector2i(24 * (layerIconIDs.at(i) % iconColumns), 24 * (layerIconIDs.at(i) / iconColumns)), Vector2i(24, 24))));
        validate(layerIcons.loadFromImage(temp));
        layerIcons.setSmooth(true);
    }

    validate(ImGui::SFML::Init(window, false));

    {
        const shared_ptr<InputStream> stream = InternalResource::getResource(ID_RES1);
        if (!stream)
            return;
        mainFontData.resize(*stream->getSize());
        validate(stream->read(mainFontData.data(), mainFontData.size()));
        if (!mainFont.openFromMemory(mainFontData.data(), mainFontData.size()))
            return;
    }

    LoadFonts();
    LoadShapes();

    //keep font memory
    ImFontConfig fc;
    fc.FontDataOwnedByAtlas = false;
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(mainFontData.data(), mainFontData.size(), config.GUIScale * 16.f, &fc);
#ifdef SFML_DESKTOP
    ImGui::GetIO().MouseDoubleClickTime = 0.30f;
#else
    ImGui::GetIO().MouseDoubleClickTime = 0.60f;
#endif
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
    ApplyStyle();
    ImGui::GetStyle().WindowTitleAlign = Vector2f(0.005f, 0.5f);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = "windows.ini";

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    this->startUpTimer = &startUpTimer;
    CanvasWorker::Setup();
    app();
    CanvasWorker::ExitThread();

    config.openWindow.at(0) = _toolPicker.windowOpen;
    config.openWindow.at(1) = _colorPicker.windowOpen;
    config.openWindow.at(2) = _layerPicker.windowOpen;

    config.Save();
    adjEffConfig.Save();
    fileExplorer.Save();
    ImGui::SFML::Shutdown();
}

void glxy::App::LoadFonts()
{
    fontPaths.clear();
    fontPaths.emplace_back("(Built-in) Montserrat.ttf");
    if (filesystem::exists(config.fontLocation))
    {
        for (const auto& n : filesystem::directory_iterator(config.fontLocation))
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
    config.fontID = std::clamp(static_cast<int32_t>(config.fontID), 0, static_cast<int32_t>(fontPaths.size() - 1));
    if (config.fontID == 0)
        textFont = std::make_shared<Font>(mainFont);
    else
        textFont = std::make_shared<Font>(fontPaths.at(config.fontID));
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
    window.create(VideoMode({ config.resolution.x, config.resolution.y }), c_AppName, config.fullscreen ? Style::None : Style::Default,
        config.fullscreen ? State::Fullscreen : State::Windowed, { 0U, 0U, config.antialiasing, GLTarget / 10U, GLTarget % 10U});
#else
    window.create(VideoMode({ config.resolution.x, config.resolution.y }), c_AppName, Style::None, State::Fullscreen,
        { 0U, 0U, config.antialiasing, GLTarget / 10U, GLTarget % 10U});
#endif

    window.setMinimumSize(Vector2u(640, 480));
    window.setIcon(Vector2u(windowLogo.getSize().x, windowLogo.getSize().y), windowLogo.getPixelsPtr());
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

void glxy::App::ApplyStyle()
{
    ImGui::GetStyle().WindowMinSize = Vector2f(150, 75) * config.GUIScale;
    ImGui::GetStyle().WindowPadding = Vector2f(8, 8) * config.GUIScale;
    ImGui::GetStyle().FramePadding = Vector2f(6, 4) * config.GUIScale;
    ImGui::GetStyle().FrameRounding = 3.f * config.GUIScale;
    ImGui::GetStyle().ItemSpacing = Vector2f(8, 4) * config.GUIScale;
    ImGui::GetStyle().ItemInnerSpacing = Vector2f(4, 4) * config.GUIScale;
    ImGui::GetStyle().IndentSpacing = 21 * config.GUIScale;
    ImGui::GetStyle().ScrollbarRounding = 9.f * config.GUIScale;
    ImGui::GetStyle().TabRounding = 4.f * config.GUIScale;
    ImGui::GetStyle().SeparatorTextBorderSize = 3.f * config.GUIScale;
    ImGui::GetStyle().GrabRounding = 4.f * config.GUIScale;
    ImGui::GetStyle().SeparatorTextPadding = Vector2f(20, 3) * config.GUIScale;
#ifdef SFML_DESKTOP
    ImGui::GetStyle().ScrollbarSize = 14.f * config.GUIScale;
    ImGui::GetStyle().GrabMinSize = 12.f * config.GUIScale;
#else
    ImGui::GetStyle().ScrollbarSize = 20.f * config.GUIScale;
    ImGui::GetStyle().GrabMinSize = 18.f * config.GUIScale;
#endif
}

void glxy::App::CreateEmptyImage(const Vector2u resolution, const bool infiniteSize, const Color color)
{
    CanvasWorker::waitWork();
    CanvasWorker::AddEditor(infiniteSize);
    _imageEditor.emplace_back(std::make_shared<ImageEditor>(window, popUpState, _colorPicker,
        !_imageEditor.empty() ? _imageEditor.back()->dockID : mainDockID, _toolPicker, _layerPicker, actionIcons, mainFont,
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
    for (auto& n : config.recentFiles)
        if (n.second == file.string())
        {
            config.recentFiles.erase(n);
            config.recentFiles.emplace(time(nullptr), file.string());
            return;
        }
    while (config.recentFiles.size() >= c_maxRecentFiles)
    {
        pair<int64_t, string> smallestID = pair(INT64_MAX, "");
        for (auto& n : config.recentFiles)
        {
            if (n.first < smallestID.first)
                smallestID = n;
        }
        config.recentFiles.erase(smallestID);
    }
    config.recentFiles.emplace(time(nullptr), file.string());
}

void glxy::App::RescaleWindow(const Vector2u size)
{
    config.resolution = size;
    resetPopupWindow = true;
}

void glxy::App::TitleBar()
{
    static Clock clock;
    static Time dfp = seconds(1);
    ImGui::BeginMainMenuBar();

    ImGui::SetWindowPos(Vector2f(0, 0));
    if (ImGui::MenuItem("Galaxy") || Shortcuts()[ActionShortcut::OpenAbout])
    {
        popUpState.push_back(PopUpState::About);
    }
    if (ImGui::BeginMenu("File"_C))
    {
        if (MenuItem("fileMenu[0]"_C, 0, Shortcuts::getName(ActionShortcut::NewImage).c_str()))
            MenuNew();
        if (MenuItem("fileMenu[1]"_C, 1, Shortcuts::getName(ActionShortcut::OpenImage).c_str()))
            MenuOpen();
        if (MenuItem("fileMenu[2]"_C, 2))
            MenuOpenRecent();
        ImGui::BeginDisabled(activeImageEditor == -1 || _imageEditor.at(activeImageEditor)->isInfinite);
        if (MenuItem("fileMenu[3]"_C, 3, Shortcuts::getName(ActionShortcut::SaveImage).c_str()))
            MenuSave();
        if (MenuItem("fileMenu[4]"_C, 4, Shortcuts::getName(ActionShortcut::SaveImageAs).c_str()))
            MenuSaveAs();
        ImGui::EndDisabled();
        if (MenuItem("fileMenu[5]"_C, 5))
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
        ImGui::BeginDisabled(!active || active->chunkManager.getFinalSelectionBounds().expired());
        if (MenuItem("editMenu[0]"_C, 8, Shortcuts::getName(ActionShortcut::Copy).c_str()))
            MenuCopy();
        if (MenuItem("editMenu[1]"_C, 9, Shortcuts::getName(ActionShortcut::Cut).c_str()))
            MenuCut();
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!clipboardImage);
        if (MenuItem("editMenu[2]"_C, 10, Shortcuts::getName(ActionShortcut::Paste).c_str()))
            MenuPaste();
        ImGui::EndDisabled();
        if (BeginMenu("editMenu[3]"_C, 13))
        {
            if (MenuItem("selectSubmenu[0]"_C, 14, Shortcuts::getName(ActionShortcut::SelectAll).c_str()))
                MenuSelectAll();
            if (MenuItem("selectSubmenu[1]"_C, 15))
                MenuSelectLeft();
            if (MenuItem("selectSubmenu[2]"_C, 16))
                MenuSelectRight();
            if (MenuItem("selectSubmenu[3]"_C, 17))
                MenuSelectTop();
            if (MenuItem("selectSubmenu[4]"_C, 18))
                MenuSelectBottom();
            if (MenuItem("selectSubmenu[5]"_C, 19))
                MenuSelectTopLeft();
            if (MenuItem("selectSubmenu[6]"_C, 20))
                MenuSelectTopRight();
            if (MenuItem("selectSubmenu[7]"_C, 21))
                MenuSelectBottomLeft();
            if (MenuItem("selectSubmenu[8]"_C, 22))
                MenuSelectBottomRight();
            ImGui::EndMenu();
        }
        ImGui::BeginDisabled(!active || active->chunkManager.getFinalSelectionBounds().expired());
        if (MenuItem("editMenu[4]"_C, 23, Shortcuts::getName(ActionShortcut::DeselectAll).c_str()))
            MenuDeselectAll();
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!active || active->chunkManager.getFinalSelectionBounds().expired() && !active->chunkManager.anyHasSelectionTempLayer());
        if (MenuItem("editMenu[5]"_C, 24, Shortcuts::getName(ActionShortcut::Delete).c_str()))
            MenuDelete();
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }
    ImGui::EndDisabled();
    if (activeImageEditor >= 0 && !_imageEditor.at(activeImageEditor)->isInfinite)
    {
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Copy] && !_imageEditor.at(activeImageEditor)->chunkManager.getFinalSelectionBounds().expired())
            MenuCopy();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Cut] && !_imageEditor.at(activeImageEditor)->chunkManager.getFinalSelectionBounds().expired())
            MenuCut();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Paste] && clipboardImage)
            MenuPaste();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::SelectAll])
            MenuSelectAll();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::DeselectAll] && !_imageEditor.at(activeImageEditor)->chunkManager.getFinalSelectionBounds().expired())
            MenuDeselectAll();
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Delete] && (!_imageEditor.at(activeImageEditor)->chunkManager.getFinalSelectionBounds().expired() ||
            _imageEditor.at(activeImageEditor)->chunkManager.anyHasSelectionTempLayer()))
            MenuDelete();
    }
    if (ImGui::MenuItem("Settings"_C) || Shortcuts()[ActionShortcut::OpenSettings])
        popUpState.push_back(PopUpState::Settings);
    ImGui::BeginDisabled(activeImageEditor == -1);
    if (ImGui::BeginMenu("View"_C))
    {
        ImageEditor* active = activeImageEditor == -1 ? nullptr : _imageEditor.at(activeImageEditor).get();
        if (MenuItem("viewMenu[0]"_C, 25, Shortcuts::getName(ActionShortcut::ZoomIn).c_str()))
            MenuZoomIn();
        if (MenuItem("viewMenu[1]"_C, 26, Shortcuts::getName(ActionShortcut::ZoomOut).c_str()))
            MenuZoomOut();
        if (BeginMenu("viewMenu[2]"_C, 27))
        {
            const array zoomScales = {
                1, 2, 5, 10, 25, 50, 75, 100, 150, 200, 250, 300, 400, 500, 1000, 2000, 5000
            };
            const float bestFitZoom = active ? active->getBestFitSize() : 0;
            if (ImGui::MenuItem(("Best fit"_S + " (" + to_string(static_cast<int32_t>(bestFitZoom * 100)) + "%)").c_str()))
                active->setNewView(Vector2f(active->getSize()) / 2.f - Vector2f(1, 1) * (c_rulerSize * config.showRuler) * (0.5f / bestFitZoom), 1 / bestFitZoom);
            float bestPixelFit = zoomScales.back() / 100.f;
            for (int8_t i = 1; i < zoomScales.size(); i++)
                if (bestFitZoom < zoomScales.at(i) / 100.f)
                {
                    bestPixelFit = zoomScales.at(i - 1) / 100.f;
                    break;
                }
            if (ImGui::MenuItem(("Best pixel fit"_S + " (" + to_string(static_cast<int32_t>(bestPixelFit * 100)) + "%)").c_str()))
                active->setNewView(Vector2f(active->getSize()) / 2.f - Vector2f(1, 1) * (c_rulerSize * config.showRuler) * (0.5f / bestPixelFit), 1 / bestPixelFit);
            for (int8_t i = 0; i < zoomScales.size(); i++)
            {
                if (ImGui::MenuItem((to_string(zoomScales.at(i)) + "%").c_str()))
                    active->setNewView(Vector2f(active->getSize()) / 2.f, 100.f / zoomScales.at(i));
            }
            ImGui::EndMenu();
        }
        if (BeginMenu("viewMenu[3]"_C, 28))
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
        if (MenuItem("viewMenu[4]"_C, 29, nullptr, config.showGrid))
            MenuGrid();
        if (MenuItem("viewMenu[5]"_C, 30, (to_string(config.gridBold.x) + " x " + to_string(config.gridBold.y)).c_str()))
            MenuGridBold();
        if (MenuItem("viewMenu[6]"_C, 31, nullptr, config.showRuler))
            MenuRuler();
        if (MenuItem("viewMenu[7]"_C, 32))
            MenuActualSize();
        if (MenuItem("viewMenu[8]"_C, 33, nullptr, config.syncViewport))
            MenuSyncViewport();
        ImGui::EndMenu();
    }
    ImGui::BeginDisabled(activeImageEditor == -1 || _imageEditor.at(activeImageEditor)->isInfinite);
    if (ImGui::BeginMenu("Image"_C))
    {
        ImageEditor* active = activeImageEditor == -1 ? nullptr : _imageEditor.at(activeImageEditor).get();
        ImGui::BeginDisabled(!active || active->chunkManager.getFinalSelectionBounds().expired());
        if (MenuItem("imageMenu[0]"_C, 34, Shortcuts::getName(ActionShortcut::Crop).c_str()))
            MenuCrop();
        ImGui::EndDisabled();
        ImGui::Separator();
        if (MenuItem("imageMenu[1]"_C, 35, Shortcuts::getName(ActionShortcut::Resize).c_str()))
            MenuResize();
        if (MenuItem("imageMenu[2]"_C, 36, Shortcuts::getName(ActionShortcut::ResizeCanvas).c_str()))
            MenuResizeCanvas();
        ImGui::Separator();
        if (MenuItem("imageMenu[3]"_C, 37))
            MenuFlipImageHorizontal();
        if (MenuItem("imageMenu[4]"_C, 38))
            MenuFlipImageVertical();
        ImGui::Separator();
        if (MenuItem("imageMenu[5]"_C, 39))
            MenuRotate90CW();
        if (MenuItem("imageMenu[6]"_C, 40))
            MenuRotate90CCW();
        if (MenuItem("imageMenu[7]"_C, 41))
            MenuRotate180();
        ImGui::Separator();
        if (MenuItem("imageMenu[8]"_C, 42, Shortcuts::getName(ActionShortcut::TransformImage).c_str()))
            MenuTransformImage();
        if (MenuItem("imageMenu[9]"_C, 43))
            MenuCircularShift();
        ImGui::EndMenu();
    }
    ImGui::EndDisabled();
    if (activeImageEditor >= 0 && !_imageEditor.at(activeImageEditor)->isInfinite)
    {
        if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Crop] && !_imageEditor.at(activeImageEditor)->chunkManager.getFinalSelectionBounds().expired())
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
        if (MenuItem("layerMenu[0]"_C, 44))
            MenuNewLayer();
        ImGui::BeginDisabled(!active || active->chunkManager.getLayerCount() == 1);
        if (MenuItem("layerMenu[1]"_C, 45))
            MenuDeleteLayer();
        ImGui::EndDisabled();
        if (MenuItem("layerMenu[2]"_C, 46))
            MenuDuplicateLayer();
        ImGui::Separator();
        ImGui::BeginDisabled(!active || _layerPicker.getLayerIDSelected(activeImageEditor) >= active->chunkManager.getLayerCount() - 1);
        if (MenuItem("layerMenu[3]"_C, 47))
            MenuMoveLayerUp();
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!active || _layerPicker.getLayerIDSelected(activeImageEditor) <= 0);
        if (MenuItem("layerMenu[4]"_C, 48))
            MenuMoveLayerDown();
        if (MenuItem("layerMenu[5]"_C, 49))
            MenuMergeLayerDown();
        ImGui::EndDisabled();
        ImGui::Separator();
        if (MenuItem("layerMenu[6]"_C, 50))
            MenuFlipImageHorizontal();
        if (MenuItem("layerMenu[7]"_C, 51))
            MenuFlipImageVertical();
        ImGui::Separator();
        if (MenuItem("layerMenu[8]"_C, 52))
            MenuLayerProperties();
        ImGui::EndMenu();
    }
    ImGui::BeginDisabled(activeImageEditor == -1 || _imageEditor.at(activeImageEditor)->isInfinite);
    if (ImGui::BeginMenu("Adjustments"_C))
    {
        if (MenuItem("adjustMenu[0]"_C, 53))
            MenuAdjustBlackAndWhite();
        if (MenuItem("adjustMenu[1]"_C, 54))
            MenuAdjustBrightnessContrast();
        if (MenuItem("adjustMenu[2]"_C, 55))
            MenuAdjustHSV();
        if (MenuItem("adjustMenu[3]"_C, 56))
            MenuAdjustInvert();
        if (MenuItem("adjustMenu[4]"_C, 57))
            MenuAdjustTint();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Effects"_C))
    {
        if (MenuItem("effectMenu[0]"_C, 58))
            MenuEffectGauss();
        if (MenuItem("effectMenu[1]"_C, 59))
            MenuEffectBox();
        if (MenuItem("effectMenu[2]"_C, 60))
            MenuEffectDirectional();
        ImGui::Separator();
        if (MenuItem("effectMenu[3]"_C, 61))
            MenuEffectWhite();
        if (MenuItem("effectMenu[4]"_C, 62))
            MenuEffectFractal();
        ImGui::Separator();
        if (MenuItem("effectMenu[5]"_C, 63))
            MenuEffectVignette();
        if (MenuItem("effectMenu[6]"_C, 64))
            MenuEffectMandelbrot();
        if (MenuItem("effectMenu[7]"_C, 65))
            MenuEffectSharpening();
        ImGui::EndMenu();
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();

    if (ImGui::BeginMenu("Help"_C))
    {
        if (MenuItem("otherMenu[0]"_C, 66))
            MenuChangelog();
        if (MenuItem("otherMenu[1]"_C, 67))
            MenuFuturePlan();
        if (MenuItem("otherMenu[2]"_C, 68))
            MenuGLScan();
        if (MenuItem("otherMenu[3]"_C, 69, Shortcuts::getName(ActionShortcut::Debug).c_str(), config.debugMode))
            MenuDebug();
        if (MenuItem("otherMenu[4]"_C, 70))
            MenuAbout();
        ImGui::EndMenu();
    }
    if (!GLOBAL.wantInput && Shortcuts()[ActionShortcut::Debug])
        MenuDebug();

    if (dfp < seconds(1))
        ImGui::Text("%s", (" " + "Saved"_S).c_str());
    if (config.showFPS)
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
    ImGui::SetNextWindowSize(Vector2f(ImGui::GetIO().DisplaySize.x, ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, Vector2f(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

    ImGui::Begin("SubTitleBar", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::PushItemWidth(140.f * config.GUIScale);
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
        if (_toolPicker.getTool() == Tool::MoveSelected)
        {
            if (ImGui::BeginCombo("Sampling method"_C, config.transformSamplingSmooth ? "Linear"_C : "Nearest"_C))
            {
                if (ImGui::Selectable("Nearest"_C))
                {
                    config.transformSamplingSmooth = false;
                    const Vector2f scale = Vector2f(static_cast<float>(_imageEditor.at(activeImageEditor)->moveSelectionSize.x) / _imageEditor.at(activeImageEditor)->moveSelectionOriginalSize.x,
                        static_cast<float>(_imageEditor.at(activeImageEditor)->moveSelectionSize.y) / _imageEditor.at(activeImageEditor)->moveSelectionOriginalSize.y);
                    AddWork(CanvasWork::MovePixels{_imageEditor.at(activeImageEditor)->moveSelectionTransform.getTransform(), scale,
                        _layerPicker.getLayerIDSelected(_imageEditor.at(activeImageEditor)->arrayID), config.transformSamplingSmooth});
                }
                if (ImGui::Selectable("Linear"_C))
                {
                    config.transformSamplingSmooth = true;
                    const Vector2f scale = Vector2f(static_cast<float>(_imageEditor.at(activeImageEditor)->moveSelectionSize.x) / _imageEditor.at(activeImageEditor)->moveSelectionOriginalSize.x,
                        static_cast<float>(_imageEditor.at(activeImageEditor)->moveSelectionSize.y) / _imageEditor.at(activeImageEditor)->moveSelectionOriginalSize.y);
                    AddWork(CanvasWork::MovePixels{_imageEditor.at(activeImageEditor)->moveSelectionTransform.getTransform(), scale,
                        _layerPicker.getLayerIDSelected(_imageEditor.at(activeImageEditor)->arrayID), config.transformSamplingSmooth});
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
        }
        ImGui::BeginDisabled(!statement);
        if (ImGui::ImageButton("Cancel"_C, actionIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
            Vector2f(0, 0.5f), Vector2f(0.1f, 0.75f)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
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
        if (ImGui::ImageButton("Finish"_C, actionIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
            Vector2f(0.1f, 0.5f), Vector2f(0.2f, 0.75f)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
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
        int32_t tolerance = config.wandTolerance;
        if (ImGui::SliderInt("Tolerance"_C, &tolerance, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp))
        {
            config.wandTolerance = tolerance;
            auto& target = _imageEditor.at(activeImageEditor);
            if (lock_guard lock(target->common.mtxEditorWorkerCommon);
                target->common.getWandFill())
                AddWork(CanvasWork::WandFill{Vector2u(target->wandFillPosition), config.wandTolerance,
                    _layerPicker.getLayerIDSelected(activeImageEditor)});
        }
        ImGui::SameLine();
        if (activeImageEditor >= 0)
        {
            lock_guard lock(_imageEditor.at(activeImageEditor)->common.mtxEditorWorkerCommon);
            statement = _imageEditor.at(activeImageEditor)->common.getWandFill();
        }
        ImGui::BeginDisabled(!statement);
        if (ImGui::ImageButton("Cancel"_C, actionIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
            Vector2f(0, 0.5f), Vector2f(0.1f, 0.75f)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
        {
            AddWorkAndWait(CanvasWork::Cancel{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Cancel"_C);
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if (ImGui::ImageButton("Finish"_C, actionIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
            Vector2f(0.1f, 0.5f), Vector2f(0.2f, 0.75f)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
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
        ImGui::InputFloat("Radius"_C, &config.brushRadius, 1, 5, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            config.brushRadius = std::clamp(config.brushRadius, 0.5f, 1000.f);
            for (auto& n : _imageEditor)
                n->OptionSetBrushSize(config.brushRadius);
        }
        break;
    case Tool::Eraser:
        ImGui::SameLine();
        ImGui::PushItemWidth(120.f);
        ImGui::InputFloat("Radius"_C, &config.eraserRadius, 1, 5, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            config.eraserRadius = std::clamp(config.eraserRadius, 0.5f, 1000.f);
            for (auto& n : _imageEditor)
                n->OptionSetBrushSize(config.eraserRadius);
        }
        break;
    case Tool::Bucket:
    {
        bool statement = false;
        ImGui::SameLine();
        ImGui::PushItemWidth(200.f);
        int32_t tolerance = config.bucketTolerance;
        if (ImGui::SliderInt("Tolerance"_C, &tolerance, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp))
        {
            config.bucketTolerance = tolerance;
            auto& target = _imageEditor.at(activeImageEditor);
            if (lock_guard lock(target->common.mtxEditorWorkerCommon);
                target->common.getBucketFill())
                AddWork(CanvasWork::BucketFill{target->bucketFillPosition, target->bucketFillColor, config.bucketTolerance,
                    _layerPicker.getLayerIDSelected(activeImageEditor)});
        }
        ImGui::SameLine();
        if (activeImageEditor >= 0)
        {
            lock_guard lock(_imageEditor.at(activeImageEditor)->common.mtxEditorWorkerCommon);
            statement = _imageEditor.at(activeImageEditor)->common.getBucketFill();
        }
        ImGui::BeginDisabled(!statement);
        if (ImGui::ImageButton("Cancel"_C, actionIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
            Vector2f(0, 0.5f), Vector2f(0.1f, 0.75f)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
        {
            AddWorkAndWait(CanvasWork::Cancel{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Cancel"_C);
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if (ImGui::ImageButton("Finish"_C, actionIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
            Vector2f(0.1f, 0.5f), Vector2f(0.2f, 0.75f)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
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
        if (ImGui::ImageButton("Cancel"_C, actionIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
            Vector2f(0, 0.5f), Vector2f(0.1f, 0.75f)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
        {
            AddWorkAndWait(CanvasWork::Cancel{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Cancel"_C);
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if (ImGui::ImageButton("Finish"_C, actionIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
            Vector2f(0.1f, 0.5f), Vector2f(0.2f, 0.75f)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
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
        ImGui::InputFloat("Radius"_C, &config.colorSwapRadius, 1, 5, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            config.colorSwapRadius = std::clamp(config.colorSwapRadius, 0.5f, 200.f);
            for (auto& n : _imageEditor)
                n->OptionSetBrushSize(config.colorSwapRadius);
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(200.f);
        int32_t tolerance = config.colorSwapTolerance;
        if (ImGui::SliderInt("Tolerance"_C, &tolerance, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp))
            config.colorSwapTolerance = tolerance;
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
        if (ImGui::BeginCombo("Shape"_C, LL::ind("shapeName[]", config.shapeID).c_str()))
        {
            const float uvSizeX = 1.f / static_cast<int8_t>(ShapeType::Count);
            for (int8_t i = 0; i < static_cast<int8_t>(ShapeType::Count); i++)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.shapeID == i ? 2.f : 0.f);
                if (ImGui::ImageButton(LL::ind("shapeName[]", i).c_str(), shapeTextures.getNativeHandle(), Vector2f(24, 24) * config.GUIScale,
                    Vector2f(uvSizeX * i, 0), Vector2f(uvSizeX * (i + 1), 1)))
                {
                    config.shapeID = i;
                    if (statement)
                    {
                        RenderShapes::getShape(_imageEditor.at(activeImageEditor)->shape, static_cast<ShapeType>(i),
                            _imageEditor.at(activeImageEditor)->shapeSize, config.shapeRadius);
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
        ImGui::PushItemWidth(80.f * config.GUIScale);
        if (ImGui::BeginCombo("##MoreOptions", "More"_C))
        {
            switch (static_cast<ShapeType>(config.shapeID))
            {
            case ShapeType::RoundedRectangle: case ShapeType::Star3:
            case ShapeType::Star4: case ShapeType::Star5:
                ImGui::BeginDisabled(false);
                break;
            default:
                ImGui::BeginDisabled(true);
                break;
            }
            ImGui::PushItemWidth(110.f * config.GUIScale);
            ImGui::InputFloat("Radius"_C, &config.shapeRadius, 1, 5, "%.1f");
            if (statement && ImGui::IsItemDeactivatedAfterEdit())
            {
                config.shapeRadius = std::max(0, static_cast<int32_t>(config.shapeRadius));
                RenderShapes::getShape(_imageEditor.at(activeImageEditor)->shape, static_cast<ShapeType>(config.shapeID),
                    _imageEditor.at(activeImageEditor)->shapeSize, config.shapeRadius);
                AddWork(CanvasWork::ShapePixels{std::make_shared<ConvexShape>(_imageEditor.at(activeImageEditor)->shape),
                    _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            ImGui::EndDisabled();
            ImGui::PushItemWidth(110.f * config.GUIScale);
            ImGui::InputFloat("Outline thickness"_C, &config.shapeOutlineThickness, 1, 5, "%.2f");
            if (statement && ImGui::IsItemDeactivatedAfterEdit())
            {
                config.shapeOutlineThickness = std::max(0, static_cast<int32_t>(config.shapeOutlineThickness));
                _imageEditor.at(activeImageEditor)->shape.setOutlineThickness(config.shapeOutlineThickness);
                AddWork(CanvasWork::ShapePixels{std::make_shared<ConvexShape>(_imageEditor.at(activeImageEditor)->shape),
                    _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!statement);
        if (ImGui::ImageButton("Cancel"_C, actionIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
            Vector2f(0, 0.5f), Vector2f(0.1f, 0.75f)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Escape) && !GLOBAL.wantInput)
        {
            AddWorkAndWait(CanvasWork::Cancel{});
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("%s", "Cancel"_C);
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if (ImGui::ImageButton("Finish"_C, actionIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
            Vector2f(0.1f, 0.5f), Vector2f(0.2f, 0.75f)) || statement && popUpState.empty() && InputEvent::isKeyHeld(Keyboard::Key::Enter) && !GLOBAL.wantInput)
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
        float textSize = config.textSize;
        ImGui::SameLine();
        if (ImGui::BeginCombo("Font"_C, fontPaths.at(
            std::clamp(static_cast<int32_t>(config.fontID), 0, static_cast<int32_t>(fontPaths.size()) - 1)).filename().string().c_str()))
        {
            for (uint16_t i = 0; i < fontPaths.size(); i++)
            {
                if (ImGui::Selectable(fontPaths.at(i).filename().string().c_str()))
                {
                    config.fontID = i;
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
        ImGui::PushItemWidth(110.f * config.GUIScale);
        if (ImGui::InputFloat("Size"_C, &textSize, 0.25f, 0.25f, "%.0f"))
        {
            const float t = fmod(textSize, 1.f);
            if (t == 0)
            {
                config.textSize = fmax(textSize, 1.f);
                if (statement)
                {
                    _imageEditor.at(activeImageEditor)->text.setCharacterSize(config.textSize);
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
                    if (c_textSizes.at(i) < config.textSize)
                    {
                        idBelow = i;
                        idAbove = i + 1;
                    }
                    if (config.textSize == c_textSizes.at(i))
                    {
                        idBelow = i - 1;
                        idAbove = i + 1;
                        break;
                    }
                }
                if (t > 0.5f)
                {
                    config.textSize = c_textSizes.at(std::max(static_cast<int32_t>(idBelow), 0));
                    if (statement)
                    {
                        _imageEditor.at(activeImageEditor)->text.setCharacterSize(config.textSize);
                        _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                        AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                            _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
                    }
                }
                else if (t < 0.5f)
                {
                    config.textSize = c_textSizes.at(std::min(static_cast<int32_t>(idAbove), static_cast<int32_t>(c_textSizes.size() - 1)));
                    if (statement)
                    {
                        _imageEditor.at(activeImageEditor)->text.setCharacterSize(config.textSize);
                        _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                        AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                            _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
                    }
                }
            }
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(80.f * config.GUIScale);
        if (ImGui::BeginCombo("##MoreOptions", "More"_C))
        {
            const array iconsMain = { "Align left"_S, "Align center"_S, "Align right"_S };
            const array iconsOptional = {"Bold"_S, "Italic"_S, "Underlined"_S, "Strikethrough"_S};
            const float uvSizeX = 1.f / (iconsMain.size() + iconsOptional.size());
            for (int8_t i = 0; i < iconsOptional.size(); i++)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.textStyle >> i & 1 ? 2.f : 0.f);
                if (ImGui::ImageButton(iconsOptional.at(i).c_str(), textIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
                    Vector2f(uvSizeX * i, 0), Vector2f(uvSizeX * (i + 1), 1)))
                {
                    config.textStyle ^= 1 << i;
                    if (statement)
                    {
                        _imageEditor.at(activeImageEditor)->text.setStyle(config.textStyle);
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
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, config.textAlignment == i ? 2.f : 0.f);
                if (ImGui::ImageButton(iconsMain.at(i).c_str(), textIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
                    Vector2f(uvSizeX * (i + iconsOptional.size()), 0), Vector2f(uvSizeX * (i + iconsOptional.size() + 1), 1)))
                {
                    config.textAlignment = i;
                    if (statement)
                    {
                        switch (config.textAlignment)
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
            ImGui::PushItemWidth(110.f * config.GUIScale);
            ImGui::InputFloat("Outline thickness"_C, &config.textOutlineThickness, 1, 5, "%.2f");
            if (statement && ImGui::IsItemDeactivatedAfterEdit())
            {
                config.textOutlineThickness = fabsf(config.textOutlineThickness);
                _imageEditor.at(activeImageEditor)->text.setOutlineThickness(config.textOutlineThickness);
                _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                    _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            ImGui::PushItemWidth(110.f * config.GUIScale);
            ImGui::InputFloat("Letter spacing"_C, &config.letterSpacing, 1, 5, "%.2f");
            if (statement && ImGui::IsItemDeactivatedAfterEdit())
            {
                _imageEditor.at(activeImageEditor)->text.setLetterSpacing(config.letterSpacing);
                _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                    _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            ImGui::PushItemWidth(110.f * config.GUIScale);
            ImGui::InputFloat("Line spacing"_C, &config.lineSpacing, 1, 5, "%.2f");
            if (statement && ImGui::IsItemDeactivatedAfterEdit())
            {
                _imageEditor.at(activeImageEditor)->text.setLineSpacing(config.lineSpacing);
                _imageEditor.at(activeImageEditor)->UpdateTextUIPosition();
                AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(_imageEditor.at(activeImageEditor)->text),
                    _imageEditor.at(activeImageEditor)->text.getGlobalBounds(), _layerPicker.getLayerIDSelected(activeImageEditor)});
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!statement);
        if (ImGui::ImageButton("Cancel"_C, actionIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
            Vector2f(0, 0.5f), Vector2f(0.1f, 0.75f)))
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
        if (ImGui::ImageButton("Finish"_C, actionIcons.getNativeHandle(), Vector2f(15, 15) * config.GUIScale,
            Vector2f(0.1f, 0.5f), Vector2f(0.2f, 0.75f)))
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
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void glxy::App::MainWindow()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(Vector2f(0, 2 * ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - (ImGui::GetFrameHeight() + 2 * ImGui::GetStyle().WindowPadding.y)));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

    ImGui::Begin("MainViewDockspace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

    mainDockID = ImGui::GetID("MainDockspace");

    ImGui::DockSpace(mainDockID, Vector2f(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

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
                AddWork(CanvasWork::BucketFill{n->bucketFillPosition, n->bucketFillColor, config.bucketTolerance,
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
    if (config.syncViewport && hoveredImageEditor >= 0)
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
                n->needsCoreGraphicsUpdate = true;
                n->needsUIGraphicsUpdate = true;
            }
            else if (n->view.getCenter() != center)
            {
                n->view.setCenter(center);
                n->needsCoreGraphicsUpdate = true;
                n->needsUIGraphicsUpdate = true;
            }
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
    bool fullscreenF11 = config.fullscreen && window.getSize() == Vector2u(VideoMode::getDesktopMode().size.x, VideoMode::getDesktopMode().size.y);
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

        if (config.outOfFocus && !InputEvent::WindowHasFocus())
        {
            sleep(milliseconds(100));
            continue;
        }
        if (changeFont)
        {
            ImFontConfig fc;
            fc.FontDataOwnedByAtlas = false;
            ImGui::GetIO().Fonts->Clear();
            ImGui::GetIO().Fonts->AddFontFromMemoryTTF(&mainFontData[0], mainFontData.size(), config.GUIScale * 16.f, &fc);
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
            shared_ptr<ImageEditor>& target = _imageEditor.at(activeImageEditor);
            if (_toolPicker.getTool() != target->currentTool)
                _toolPicker.setTool(target->currentTool, false);
            _toolPicker.setToolsEnabled(target->isInfinite ? c_infiniteToolsEnabled : 0xFFFFFF);
            for (int8_t i = 0; i < 2; i++)
                _colorPicker.setEditorColors(i, target->currentColor.at(i));
        }
        else if (hoveredImageEditor != -1)
        {
            shared_ptr<ImageEditor>& target = _imageEditor.at(hoveredImageEditor);
            if (_toolPicker.getTool() != target->currentTool)
                _toolPicker.setTool(target->currentTool, false);

            Tool newTool = Tool::Count;
            if (config.middleMouseButton >= 0 && InputEvent::isButtonPressed(Mouse::Button::Middle))
                newTool = static_cast<Tool>(config.middleMouseButton);
            if (config.extra1MouseButton >= 0 && InputEvent::isButtonPressed(Mouse::Button::Extra1))
                newTool = static_cast<Tool>(config.extra1MouseButton);
            if (config.extra2MouseButton >= 0 && InputEvent::isButtonPressed(Mouse::Button::Extra2))
                newTool = static_cast<Tool>(config.extra2MouseButton);

            if (target->isInfinite)
            {
                if (!((c_infiniteToolsEnabled >> static_cast<int8_t>(newTool)) & 1))
                    newTool = Tool::Count;
            }

            if (newTool == Tool::Count && target->toolBeforeMouseSwitch != Tool::Count)
            {
                _toolPicker.setTool(target->toolBeforeMouseSwitch, true);
                if (target->currentTool == Tool::Pan || target->currentTool == Tool::Zoom)
                {
                    target->switchBackPanZoomTool = true;
                    target->forceToolNoChange = true;
                }
                target->currentTool = target->toolBeforeMouseSwitch;
                target->toolBeforeMouseSwitch = Tool::Count;

            }
            if (newTool != Tool::Count && newTool != target->currentTool)
            {
                target->toolBeforeMouseSwitch = target->currentTool;
                target->currentTool = newTool;
                _toolPicker.setTool(newTool, true);
                target->switchBackPanZoomTool = false;
            }

            _toolPicker.setToolsEnabled(target->isInfinite ? c_infiniteToolsEnabled : 0xFFFFFF);
            for (int8_t i = 0; i < 2; i++)
                _colorPicker.setEditorColors(i, target->currentColor.at(i));
        }
        else if (activeImageEditor != -1)
        {
            shared_ptr<ImageEditor>& target = _imageEditor.at(activeImageEditor);
            if (_toolPicker.getTool() != target->currentTool)
                _toolPicker.setTool(target->currentTool, false);
            _toolPicker.setToolsEnabled(target->isInfinite ? c_infiniteToolsEnabled : 0xFFFFFF);
            for (int8_t i = 0; i < 2; i++)
                _colorPicker.setEditorColors(i, target->currentColor.at(i));
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
                for (int8_t i = 0; i < 2; i++)
                    _imageEditor.at(activeImageEditor)->currentColor.at(i) = _colorPicker.getColor(i);
            }
            else if (hoveredImageEditor != -1)
            {
                for (int8_t i = 0; i < 2; i++)
                    _imageEditor.at(hoveredImageEditor)->currentColor.at(i) = _colorPicker.getColor(i);
            }
            else if (activeImageEditor != -1)
            {
                for (int8_t i = 0; i < 2; i++)
                    _imageEditor.at(activeImageEditor)->currentColor.at(i) = _colorPicker.getColor(i);
            }
        }
        _toolPicker.Draw();
        if (_toolPicker.wasUserChanged())
        {
            if (!someEditorHovered && activeImageEditor != -1)
            {
                if (lock_guard lock(_imageEditor.at(activeImageEditor)->common.mtxEditorWorkerCommon);
                    _imageEditor.at(activeImageEditor)->common.hasUnfinishedChanges() &&
                    _toolPicker.getTool() != Tool::Pan && _toolPicker.getTool() != Tool::Zoom && !_imageEditor.at(activeImageEditor)->switchBackPanZoomTool)
                {
                    changeToTool = _toolPicker.getTool();
                    AddWork(CanvasWork::Finish{});
                    popUpState.push_back(PopUpState::ToolChanged);
                    popUpState.push_back(PopUpState::ThreadWork);
                }
                else
                    _imageEditor.at(activeImageEditor)->currentTool = _toolPicker.getTool();
                if (_imageEditor.at(activeImageEditor)->switchBackPanZoomTool)
                    _imageEditor.at(activeImageEditor)->forceToolNoChange = true;
                _imageEditor.at(activeImageEditor)->switchBackPanZoomTool = false;
            }
            else if (hoveredImageEditor != -1)
            {
                if (lock_guard lock(_imageEditor.at(hoveredImageEditor)->common.mtxEditorWorkerCommon);
                    _imageEditor.at(hoveredImageEditor)->common.hasUnfinishedChanges() &&
                    _toolPicker.getTool() != Tool::Pan && _toolPicker.getTool() != Tool::Zoom && !_imageEditor.at(activeImageEditor)->switchBackPanZoomTool)
                {
                    changeToTool = _toolPicker.getTool();
                    AddWork(CanvasWork::Finish{});
                    popUpState.push_back(PopUpState::ToolChanged);
                    popUpState.push_back(PopUpState::ThreadWork);
                }
                else
                    _imageEditor.at(hoveredImageEditor)->currentTool = _toolPicker.getTool();
                if (_imageEditor.at(hoveredImageEditor)->switchBackPanZoomTool)
                    _imageEditor.at(hoveredImageEditor)->forceToolNoChange = true;
                _imageEditor.at(hoveredImageEditor)->switchBackPanZoomTool = false;
            }
            else if (activeImageEditor != -1)
            {
                if (lock_guard lock(_imageEditor.at(activeImageEditor)->common.mtxEditorWorkerCommon);
                    _imageEditor.at(activeImageEditor)->common.hasUnfinishedChanges() &&
                    _toolPicker.getTool() != Tool::Pan && _toolPicker.getTool() != Tool::Zoom && !_imageEditor.at(activeImageEditor)->switchBackPanZoomTool)
                {
                    changeToTool = _toolPicker.getTool();
                    AddWork(CanvasWork::Finish{});
                    popUpState.push_back(PopUpState::ToolChanged);
                    popUpState.push_back(PopUpState::ThreadWork);
                }
                else
                    _imageEditor.at(activeImageEditor)->currentTool = _toolPicker.getTool();
                if (_imageEditor.at(activeImageEditor)->switchBackPanZoomTool)
                    _imageEditor.at(activeImageEditor)->forceToolNoChange = true;
                _imageEditor.at(activeImageEditor)->switchBackPanZoomTool = false;
            }
        }

        LayerPicker::Return result = _layerPicker.Draw(window, config.GUIScale, layerIcons, popUpState, activeImageEditor);
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
        default:
            break;
        }

        if (Shortcuts()[ActionShortcut::ToggleFullscreen])
        {
            fullscreenF11 = !fullscreenF11;
            if (fullscreenF11)
            {
                config.resolution = VideoMode::getDesktopMode().size;
                config.fullscreen = true;
            }
            else
            {
                config.resolution = Vector2u(1280, 720);
                config.fullscreen = false;
            }
            RecreateAppWindow();
            RescaleWindow(config.resolution);
            config.Save();
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
    _imageEditor.emplace_back(std::make_shared<ImageEditor>(window, popUpState, _colorPicker,
        _imageEditor.size() ? _imageEditor.back()->dockID : mainDockID, _toolPicker, _layerPicker, actionIcons, mainFont,
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