#pragma once
#include <SFML/Graphics.hpp>
#include "../AppSettings.hpp"
#include "../Canvas/CanvasWorker.hpp"
#include "../Namespace.hpp"
#include "imgui.h"
#include "../UIElements/EditorUiElement.hpp"
#include "../UIElements/RulerUi.hpp"
#include "../UIElements/GridLines.hpp"
#include "../PopUpState.hpp"
#include "../Pickers/ColorPicker.hpp"
#include "../Pickers/LayerPicker.hpp"
#include "../Pickers/ToolPicker.hpp"
#include "../Canvas/ChunkManager.hpp"
#include "ChunkTextureManager.hpp"
#include "../Canvas/ImageEditorWorkerCommon.hpp"
#include "../UIElements/PixelSelectAnimation.hpp"

namespace glxy
{
    class ImageEditor
    {
    public:
        EditorID editorID;
        int16_t arrayID;
        const ImGuiID& dockID;
        filesystem::path imagePath;
        int32_t imageJPGQuality = 0;

        View view;
        float windowScale = 0;
        View viewUI;
        RenderTexture texture;
        GridLines gridLines;
        RulerUI rulerUI;
        ChunkTextureManager chunkTextureManager;

        const ToolPicker& _toolPicker;
        LayerPicker& _layerPicker;
        const ColorPicker& _colorPicker;
        Tool currentTool = Tool::Pencil;
        array<ImVec4, c_colorCount> currentColor;

        Texture transparentLayer;
        bool viewHovered = false;
        bool windowFocused = false;
        bool windowHovered = false;
        bool wasHoveredUponAction = false;
        bool windowOpen = true;
        bool initComplete = false;
        bool unsavedChanges = false;
        bool chunksUpToDate = false;

        FloatRect viewArea;
        FloatRect windowArea;
        Vector2f windowSize;

        PixelSelectAnimation animCenter;
        PixelSelectAnimation animOutline;

        Color bucketFillColor;
        Vector2i bucketFillPosition;
        EditorUIElement bucketFillMove;

        Vector2i wandFillPosition;
        EditorUIElement wandMove;

        CircleShape brushInnerOutline;
        CircleShape brushOuterOutline;

        RectangleShape squareInnerOutline;
        RectangleShape squareOuterOutline;

        EditorUIElement gradientStart;
        EditorUIElement gradientEnd;
        EditorUIElement gradientMove;

        IntRect moveSelectionArea;
        Vector2i moveSelectionOriginalSize;
        Transformable moveSelectionTransform;
        EditorUIElement moveSelectionMove;
        EditorUIElement moveSelectionMoveArea;
        vector<EditorUIElement> moveSelectionPoints;

        const AppSettings& settings;
        const vector<PopUpState>& popUpState;
        Window& window;
        unique_ptr<Cursor>& cursor;
        Cursor::Type& cursorType;
        const Texture& gizmoIcons;
        const ChunkManager& chunkManager;
        const ImageEditorWorkerCommon& common;

        Vector2i cacheShapeSelection;
        Vector2f cacheBrushPosition;
        bool hasStartedBrush = false;
        bool hasStartedBucket = false;
        bool hasStartedWand = false;
        Vector2f scrollBarScroll = Vector2f(0, 1);
        Vector2i prevPanPos;
        Vector2f cameraOriginalPos;
        Vector2f cameraTargetPos;
        Vector2f cameraOriginalSize;
        Vector2f cameraTargetSize;
        bool cameraAnimationRunning = false;
        Time cameraAnimation;
        Time moveViewHitRate;
        Time lastRenderPass;
        Vector2f mousePosPrevFrame = Vector2f();
        bool forceToolChange = false;

        ImageEditor(const AppSettings& settings, Window& window, const vector<PopUpState>& popUpState, unique_ptr<Cursor>& cursor,
                    Cursor::Type& cursorType, const ColorPicker& colorPicker, const ImGuiID& dockID,
                    const ToolPicker& toolPicker, LayerPicker& layerPicker, const Texture& gizmoIcons, const Font& mainFont,
                    const ChunkManager& chunkManager, const ImageEditorWorkerCommon& common, int16_t arrayID);

        void FinishCreation();
        bool Save();

        void UpdateZoom();
        void UpdateTool();
        void UpdateEditorTextures();
        void Update();
        void Draw();
        void DrawChunkManager(RenderTarget& target) const;
        void DrawPixelSelect(RenderTarget& target) const;
        template <typename T>
        void AddWork(const T& work);

        void OptionZoomIn(bool basedOnMouse);
        void OptionZoomOut(bool basedOnMouse);
        void OptionGrid(bool state);
        void OptionRuler(bool state);
        void OptionActualSize();
        void OptionSetBrushSize(float radius);

        Vector2u getSize() const;

        float getBestFitSize() const;
        bool anyEditorUIElementSelected() const;
        string getImageName() const;
        ImVec4 getUsedColor() const;

        bool ReadPixel(Vector2f pos, Color& color) const;
        bool ReadPixel(Vector2f pos, ImVec4& color) const;

        void ResetMovePixels();
        void FinishMovePixels();
        void SetupMovePixelsUI(Vector2i size);

        void setThemeColor();
        void setCursorType(Cursor::Type cursorType) const;
        void RecreateEditorTexture();
        void UpdateLayerPreview() const;
        void setNewView(Vector2f center, float scale);
        void MoveView(Vector2f offset);
        void setViewPosition(Vector2f position);
        void setViewPositionX(float position);
        void setViewPositionY(float position);
        void ClampView();
        void ClipboardPaste(const Image* image, const Vector2u* location);
    };

    template <typename T>
    void ImageEditor::AddWork(const T& work)
    {
        CanvasWorker::AddWork(work, arrayID);
    }
}
