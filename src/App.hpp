#pragma once
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <filesystem>
#include <set>
#include <thread>
#include "Processing/AdjustmentEffectConfig.hpp"
#include "Config.hpp"
#include "Canvas/CanvasWorker.hpp"
#include "Const.hpp"
#include "ZEditorsCommon/ZTB.hpp"
#include "Rendering/ImageEditor.hpp"
#include "Pickers/ColorPicker.hpp"
#include "Pickers/LayerPicker.hpp"
#include "Pickers/ToolPicker.hpp"
#include "ZEditorsCommon/FileExplorer.hpp"

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
        unique_ptr<Image> clipboardSelection;
        Vector2u clipboardLocation;

        ImGuiID mainDockID = 0;
        EditorID editorCloseAttempt = -1;
        Adjustments targetAdjustment;
        Effects targetEffect;
        FileExplorer fileExplorer;

        ColorPicker _colorPicker;
        ToolPicker _toolPicker;
        LayerPicker _layerPicker;

        Config& config = Config::get();
        AdjustmentEffectConfig& adjEffConfig = AdjustmentEffectConfig::get();

        string mainFontData;
        Font mainFont;

        shared_ptr<Font> textFont;
        string textString;

        bool changeFont = false;
        Tool changeToTool = Tool::Count;
        bool resetPopupWindow = false;
        Image windowLogo;
        Texture windowLogoTexture;
        Clock pdo;
        Texture actionIcons;
        Texture layerIcons;
        Texture textIcons;
        Texture toolIcons;
        Texture setupSelect;
        Texture shapeTextures;
        Texture menuIcons;

        vector<filesystem::path> fontPaths;

        filesystem::path openWithGalaxyFile = "";
        bool openWithGalaxy = false;

        const Clock* startUpTimer = nullptr;
        Time GalaxyStartUpTime;

        App();
        ~App();
        bool hasUnsavedImages() const;
        void DeleteEditor(EditorID ID);
        void AddRecentFile(const filesystem::path& file);
        template <typename T>
        void AddWork(const T& work);
        template <typename T>
        void AddWorkAndWait(const T& work);

        bool MenuItem(const char* label, uint8_t iconID, const char* shortcut = nullptr, bool selected = false, bool enabled = true) const;
        bool BeginMenu(const char* label, uint8_t iconID, bool enabled = true) const;

        void Start(const filesystem::path& filename, bool openWithGalaxy, const Clock& startUpTimer);
        void LoadFonts();
        void LoadShapes();
        void RecreateAppWindow();
        void ApplyStyle();
        void CreateEmptyImage(Vector2u resolution, bool infiniteSize, Color color);
        void RescaleWindow(Vector2u size);
        void TitleBar();
        void SubTitleBar();
        void PopUp();
        void MainWindow();
        void setActiveEditor(EditorID ID);
        void setHoveredEditor(EditorID ID);
        void app();
        void OpenImage(const filesystem::path& fileName);
        bool SaveImage();

        void MenuNew();
        void MenuOpen();
        void MenuOpenRecent();
        void MenuSave();
        void MenuSaveAs();
        void MenuExit(bool windowClose);
        void MenuCopy();
        void MenuCut();
        void MenuPaste();
        void MenuSelectAll();
        void MenuSelectLeft();
        void MenuSelectRight();
        void MenuSelectTop();
        void MenuSelectBottom();
        void MenuSelectTopLeft();
        void MenuSelectTopRight();
        void MenuSelectBottomLeft();
        void MenuSelectBottomRight();
        void MenuDeselectAll();
        void MenuDelete();
        void MenuZoomIn();
        void MenuZoomOut();
        void MenuGrid();
        void MenuGridBold();
        void MenuRuler();
        void MenuActualSize();
        void MenuSyncViewport();
        void MenuCrop();
        void MenuResize();
        void MenuResizeCanvas();
        void MenuFlipImageHorizontal();
        void MenuFlipImageVertical();
        void MenuRotate90CW();
        void MenuRotate90CCW();
        void MenuRotate180();
        void MenuTransformImage();
        void MenuCircularShift();
        void MenuNewLayer();
        void MenuDeleteLayer();
        void MenuDuplicateLayer();
        void MenuMoveLayerUp();
        void MenuMoveLayerDown();
        void MenuMergeLayerDown();
        void MenuFlipLayerHorizontal();
        void MenuFlipLayerVertical();
        void MenuLayerProperties();
        void MenuAdjustBlackAndWhite();
        void MenuAdjustBrightnessContrast();
        void MenuAdjustHSV();
        void MenuAdjustInvert();
        void MenuAdjustTint();
        void MenuEffectGauss();
        void MenuEffectBox();
        void MenuEffectDirectional();
        void MenuEffectWhite();
        void MenuEffectFractal();
        void MenuEffectVignette();
        void MenuEffectMandelbrot();
        void MenuEffectSharpening();
        void MenuChangelog();
        void MenuFuturePlan();
        void MenuGLScan();
        void MenuDebug();
        void MenuAbout();
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
