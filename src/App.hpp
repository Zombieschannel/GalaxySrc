#pragma once
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <filesystem>
#include <set>
#include <thread>
#include "AppSettings.hpp"
#include "Const.hpp"
#include "inc/ZTB.hpp"
#include "ImageEditor.hpp"
#include "ColorPicker.hpp"
#include "LayerPicker.hpp"
#include "ToolPicker.hpp"

using namespace sf;

namespace glxy
{
    struct App
    {
        RenderWindow window;
        vector<PopUpState> popUpState = { PopUpState::Setup };
        vector<unique_ptr<ImageEditor>> _imageEditor;
        int16_t activeImageEditor = -1;
        int16_t hoveredImageEditor = -1;

        unique_ptr<Image> clipboardImage;
        Vector2u clipboardLocation;

        ImGuiID mainDockID = 0;
        float subTitleBarHeight = 0;
        int16_t editorCloseAttempt = -1;

        ColorPicker _colorPicker;
        ToolPicker _toolPicker;
        LayerPicker _layerPicker;

        unique_ptr<std::thread> workerThread;
        float workerThreadProgress = 0;
        unique_ptr<Cursor> cursor;
        Cursor::Type cursorType = Cursor::Type::Arrow;
        AppSettings settings;

        string mainFontData;
        Font mainFont;

        bool changeFont = false;
        Image windowLogo;
        Texture windowLogoTexture;
        Clock pdo;
        Texture canvasIcons;
        Texture layerIcons;
        Texture toolIcons;
        Texture setupSelect;
        Texture gizmoIcons;

        filesystem::path openWithGalaxyFile = "";
        bool openWithGalaxy = false;

        const Clock* startUpTimer = nullptr;
        Time GalaxyStartUpTime;

        App();
        ~App();
        bool hasUnsavedImages() const;
        void ExitApp(bool windowClose);
        void DeleteEditor(int32_t ID);
        void AddRecentFile(const filesystem::path& file);
        void setCursorType(Cursor::Type cursorType);

        void Start(const filesystem::path& filename, bool openWithGalaxy, const Clock& startUpTimer);
        void RecreateAppWindow();
        void CreateEmptyImage(Vector2u resolution, Color color);
        void RescaleWindow(Vector2u size);
        void TitleBar();
        void SubTitleBar();
        void PopUp();
        void MainWindow();
        void setActiveEditor(int32_t ID);
        void setHoveredEditor(int32_t ID);
        void app();
        bool OpenImage(const filesystem::path& fileName);
        bool SaveImage();
    };
}