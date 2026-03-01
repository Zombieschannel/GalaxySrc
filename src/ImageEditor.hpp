#pragma once
#include <SFML/Graphics.hpp>
#include "AppSettings.hpp"
#include "ColorPicker.hpp"
#include "GridLines.hpp"
#include "imgui.h"
#include "LayerPicker.hpp"
#include "Namespace.hpp"
#include "PopUpState.hpp"
#include "ToolPicker.hpp"
#include "PixelSelect.hpp"
#include "EditorUiElement.hpp"
#include "ChunkManager.hpp"
#include "RulerUi.hpp"

using namespace sf;

namespace glxy
{
    enum class RescaleMethod
    {
        Triangle,
        Box,
        Catmullrom,
        Mitchell,
        CubicBSpline,
        PointSample
    };
    class ImageEditor
    {
    public:
        uint16_t editorID;
        const ImGuiID& dockID;

        View view;
        View viewUI;
        RenderTexture texture;
        string imageName;
        filesystem::path imagePath;
        GridLines gridLines;
        RulerUI rulerUI;

        const ToolPicker& _toolPicker;
        Tool currentTool = Tool::Pencil;
        array<ImVec4, c_colorCount> currentColor;

        LayerPicker& _layerPicker;
        PixelSelect pixelSelect;
        Vector2f pixelSelectStart;

        bool needUpdateLayerPreview = false;

        bool bucketFill = false;
        ImVec4 bucketFillColor;
        Vector2i bucketFillPosition;
        EditorUIElement bucketFillMove;

        bool wandFill = false;
        Vector2i wandFillPosition;
        EditorUIElement wandMove;

        bool gradientDraw = false;
        bool gradientSetup = false;
        EditorUIElement gradientStart;
        EditorUIElement gradientEnd;
        EditorUIElement gradientMove;

        CircleShape brushInnerOutline;
        CircleShape brushOuterOutline;

        RectangleShape squareInnerOutline;
        RectangleShape squareOuterOutline;

        bool transformImage = false;
        IntRect transformImageSelectionArea;
        Vector2i transformImageOriginalPosition;
        unique_ptr<Transformable> transformImageTransform;

        bool moveSelection = false;
        unique_ptr<Image> tempImage;
        IntRect moveSelectionArea;
        unique_ptr<Vector2i> moveSelectionOriginalPosition;
        unique_ptr<Transformable> moveSelectionTransform;
        vector<EditorUIElement> moveSelectionPoints;
        EditorUIElement moveSelectionMove;
        EditorUIElement moveSelectionMoveArea;

        ChunkManager chunkManager;
        Texture transparentLayer;

        bool viewHovered = false;
        bool windowFocused = false;
        bool windowHovered = false;
        bool wasHoveredUponAction = false;
        bool windowOpen = true;
        bool unsavedChanges = false;
        FloatRect viewArea;
        FloatRect windowArea;
        float windowScale = 0;
        Vector2f windowSize;

        AppSettings& settings;
        vector<PopUpState>& popUpState;
        Window& window;
        unique_ptr<Cursor>& cursor;
        Cursor::Type& cursorType;
        const Texture& gizmoIcons;

        Vector2f scrollBarScroll = Vector2f(0, 1);
        Vector2i prevPanPos;
        Vector2f cameraOriginalPos;
        Vector2f cameraTargetPos;
        Vector2f cameraOriginalSize;
        Vector2f cameraTargetSize;
        bool cameraAnimationRunning = false;
        Time cameraAnimation;
        Time moveViewHitRate;
        Vector2f mousePosPrevFrame = Vector2f();
        int32_t imageJPGQuality = 0;
        IntRect lastFillArea;

        ImageEditor(AppSettings& settings, Window& window, vector<PopUpState>& popUpState, unique_ptr<Cursor>& cursor,
                    Cursor::Type& cursorType, const ColorPicker& colorPicker, const ImGuiID& dockID,
                    const ToolPicker& toolPicker, LayerPicker& layerPicker, const Texture& gizmoIcons, const Font& mainFont);
        void Empty(const string& name, Vector2u resolution, Color color);
        bool Open(const filesystem::path& target);
        bool Save();
        void UpdateZoom();
        void UpdateTool();
        void Update();
        void Draw();

        void OptionZoomIn(bool basedOnMouse);
        void OptionZoomOut(bool basedOnMouse);
        void OptionGrid(bool state);
        void OptionRuler(bool state);
        void OptionActualSize();
        void OptionCreateLayer();
        void OptionDeleteLayer();
        void OptionDuplicateLayer();
        void OptionMoveLayerUp();
        void OptionMoveLayerDown();
        void OptionMergeLayerDown();
        void OptionFlipLayerHorizontal();
        void OptionFlipLayerVertical();
        void OptionFlipImageHorizontal();
        void OptionFlipImageVertical();
        void OptionRotate90CW();
        void OptionRotate90CCW();
        void OptionRotate180();
        void OptionCopyToClipboard(Image& target, Vector2u& location);
        void OptionPasteFromClipboard(const Image& src, Vector2u location);
        void OptionSelectAll();
        void OptionDeselectAll();
        void OptionDeleteSelected();
        void OptionCancel();
        void OptionFinish();
        void OptionSyncViewport() const;
        void OptionSetBrushSize(float radius);
        void OptionSetupTransformImage();
        void OptionCropSelection();

        Vector2u getSize() const;
        float getBestFitSize() const;
        bool anyEditorUIElementSelected() const;

        void setThemeColor();
        void SetupMovePixelsUI();
        void SetupMovePixels();
        void GenerateMovePixels();
        void FinishMovePixels();

        IntRect TransformImage(const Transformable& transformable, bool tile) const;

        bool ReadPixel(Vector2f pos, Color& color) const;
        bool ReadPixel(Vector2f pos, ImVec4& color) const;
        void InterpolatePixelLine(Vector2f start, Vector2f end, vector<Vector2i>& out) const;
        void DrawPixels(Vector2f pos, Color color, float radius);
        bool FillPixels(Vector2i pos, Color color, int8_t tolerance);
        void GradientPixels(Vector2f startPos, Vector2f endPos, Color startColor, Color endColor);
        void ResizeCanvas(IntRect newSize);
        void RescaleCanvas(Vector2u newSize, RescaleMethod method);

        void RecreateEditorTexture();
        void MergeTempLayer();
        void AllocateChunks(Vector2u size);

        void setCursorType(Cursor::Type cursorType) const;
        void updateLayerPreview(int16_t layerID) const;
        uint32_t getChunkIDFromPixel(Vector2u pixel) const;
        void setNewView(Vector2f center, float scale);
        void MoveView(Vector2f offset);
        void setViewPosition(Vector2f position);
        void setViewPositionX(float position);
        void setViewPositionY(float position);
        void ClampView();
    };
}