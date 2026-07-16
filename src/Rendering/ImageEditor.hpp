#pragma once
#include <SFML/Graphics.hpp>
#include "../Config.hpp"
#include "../Canvas/CanvasWorker.hpp"
#include "../Namespace.hpp"
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
#include <imgui.h>

namespace glxy
{
    class ImageEditor
    {
    public:
        EditorID editorID;
        int16_t arrayID;
        const ImGuiID& dockID;
        const bool isInfinite;
        filesystem::path imagePath;
        int32_t imageJPGQuality = 0;

        View view;
        float windowScale = 0;
        View viewUI;
        RenderTexture coreTexture;
        RenderTexture UITexture;
        GridLines gridLines;
        RulerUI rulerUI;
        ChunkTextureManager chunkTextureManager;

        const ToolPicker& _toolPicker;
        LayerPicker& _layerPicker;
        const ColorPicker& _colorPicker;
        Tool currentTool = Tool::Pencil;
        Tool toolBeforeMouseSwitch = Tool::Count;
        bool switchBackPanZoomTool = false;
        array<Color32f, 2> currentColor;
        Texture transparentLayer;

        bool viewHovered = false;
        bool windowFocused = false;
        bool windowHovered = false;
        bool wasHoveredUponAction = false;
        bool windowOpen = true;
        bool initComplete = false;
        bool unsavedChanges = false;
        bool chunksUpToDate = false;
        bool needsCoreGraphicsUpdate = true;
        bool needsUIGraphicsUpdate = true;

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

        EditorUIElement shapeMove;
        EditorUIElement shapeRotate;
        EditorUIElement shapeMoveArea;
        ConvexShape shape;
        vector<EditorUIElement> shapeSizePoints;
        Vector2i shapeStartPosition;
        Vector2i shapeEndPosition;
        Vector2i shapeSize;

        EditorUIElement textMove;
        EditorUIElement textRotate;
        String textString;
        Text text;

        Vector2i moveSelectionSize;
        Vector2i moveSelectionOriginalSize;
        Transformable moveSelectionTransform;
        EditorUIElement moveSelectionMove;
        EditorUIElement moveSelectionRotate;
        EditorUIElement moveSelectionMoveArea;
        vector<EditorUIElement> moveSelectionPoints;

        const Config& config = Config::get();
        const vector<PopUpState>& popUpState;
        Window& window;
        const Texture& gizmoIcons;
        const ChunkManager& chunkManager;
        const ImageEditorWorkerCommon& common;
        const shared_ptr<Font>& textFont;

        Vector2i cacheShapeSelection;
        Vector2f cacheLassoSelection;
        Vector2f cacheBrushPosition;
        Vector2f cachePencilPosition;
        Vector2f cacheColorSwapPosition;
        Vector2f cacheGradientPosition;
        bool hasStartedZoom = false;
        bool hasStartedPencil = false;
        bool hasStartedBrush = false;
        bool hasStartedBucket = false;
        bool hasStartedWand = false;
        bool hasStartedColorSwap = false;
        bool hasStartedShape = false;
        bool hasStartedText = false;
        bool lastWorkerToolState = false;
        int32_t lassoSelectVertexCount = 0;
        VertexArray lassoSelectArea;
        Vector2f scrollBarScroll = Vector2f(0, 1);
        Vector2f cameraOriginalPos;
        Vector2f cameraTargetPos;
        Vector2f cameraOriginalSize;
        Vector2f cameraTargetSize;
        bool cameraAnimationRunning = false;
        Time cameraAnimation;
        Time moveViewHitRate;
        std::optional<Vector2f> mousePosPrevFrame;
        std::optional<Vector2i> mouseScreenPosPrevFrame;
        int32_t zoomMouseStartPosX;
        bool forceToolChange = false;
        bool forceToolNoChange = false;
        bool wantInput = false;

        ImageEditor(Window& window, const vector<PopUpState>& popUpState,
                    const ColorPicker& colorPicker, const ImGuiID& dockID, const ToolPicker& toolPicker,
                    LayerPicker& layerPicker, const Texture& gizmoIcons, const Font& mainFont,
                    const ChunkManager& chunkManager, const ImageEditorWorkerCommon& common, int16_t arrayID,
                    bool infiniteSize, const shared_ptr<Font>& textFont);

        void FinishCreation();
        bool Save();

        void onResize();
        void UpdateZoom();
        void UpdateTool();
        void UpdateEditorTextures();
        void Update();
        void Draw();
        void DrawUI();
        void DrawChunkManager(RenderTarget& target);
        void DrawPixelSelect(RenderTarget& target) const;
        template <typename T>
        void AddWork(const T& work);

        void OptionZoomIn(bool basedOnMouse);
        void OptionZoomOut(bool basedOnMouse);
        void OptionGrid(bool state);
        void OptionGridBold(Vector2i size);
        void OptionRuler(bool state);
        void OptionActualSize();
        void OptionSetBrushSize(float radius);

        Vector2u getSize() const;

        float getBestFitSize() const;
        bool anyEditorUIElementHovered() const;
        string getImageName() const;
        Color32f getUsedColor() const;

        bool ReadPixel(Vector2f pos, Color& color) const;
        bool ReadPixel(Vector2f pos, Color32f& color) const;

        void ResetMovePixels();
        void FinishMovePixels();

        void SetupMovePixelsUI(Vector2f size);
        void UpdateShapeUIPosition();
        void UpdateTextUIPosition();

        void setThemeColor();

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
