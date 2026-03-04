#pragma once
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <filesystem>
#include <set>
#include <thread>
#include "Processing/AdjustmentSettings.hpp"
#include "Processing/EffectSettings.hpp"
#include "AppSettings.hpp"
#include "Canvas/CanvasWorker.hpp"
#include "Const.hpp"
#include "ZEditorsCommon/ZTB.hpp"
#include "Rendering/ImageEditor.hpp"
#include "Pickers/ColorPicker.hpp"
#include "Pickers/LayerPicker.hpp"
#include "Pickers/ToolPicker.hpp"

using namespace sf;

namespace glxy
{
    struct App
    {
        RenderWindow window;
        vector<PopUpState> popUpState = { PopUpState::Setup };
        vector<shared_ptr<ImageEditor>> _imageEditor;
        EditorID activeImageEditor = -1;
        EditorID hoveredImageEditor = -1;

        unique_ptr<Image> clipboardImage;
        Vector2u clipboardLocation;

        ImGuiID mainDockID = 0;
        float subTitleBarHeight = 0;
        EditorID editorCloseAttempt = -1;
        Adjustments targetAdjustment;
        Effects targetEffect;

        ColorPicker _colorPicker;
        ToolPicker _toolPicker;
        LayerPicker _layerPicker;

        unique_ptr<Cursor> cursor;
        Cursor::Type cursorType = Cursor::Type::Arrow;
        AppSettings settings;
        AdjustmentSettings adjSettings;
        EffectSettings effSettings;

        string mainFontData;
        Font mainFont;

        bool changeFont = false;
        Tool changeToTool = Tool::Count;
        bool resetPopupWindow = false;
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
        void DeleteEditor(EditorID ID);
        void AddRecentFile(const filesystem::path& file);
        void setCursorType(Cursor::Type cursorType);
        template <typename T>
        void AddWork(const T& work);
        template <typename T>
        void AddWorkAndWait(const T& work);

        void Start(const filesystem::path& filename, bool openWithGalaxy, const Clock& startUpTimer);
        void RecreateAppWindow();
        void CreateEmptyImage(Vector2u resolution, Color color);
        void RescaleWindow(Vector2u size);
        void TitleBar();
        void SubTitleBar();
        void PopUp();
        void MainWindow();
        void UpdateRenderWorker() const;
        void setActiveEditor(EditorID ID);
        void setHoveredEditor(EditorID ID);
        void app();
        void OpenImage(const filesystem::path& fileName);
        bool SaveImage();
    };

    template <typename T>
    void App::AddWork(const T& work)
    {
        CanvasWorker::AddWork(work, activeImageEditor);
    }

    template <typename T>
    void App::AddWorkAndWait(const T& work)
    {
        CanvasWorker::AddWork(work, activeImageEditor);
        popUpState.push_back(PopUpState::ThreadWork);
    }
}
