#pragma once
#include <SFML/Graphics.hpp>
#include "../AppSettings.hpp"
#include "../Pickers/ColorPicker.hpp"
#include "../Pickers/LayerPicker.hpp"
#include "../Pickers/ToolPicker.hpp"
#include "../Namespace.hpp"
#include "../PopUpState.hpp"
#include "ChunkManager.hpp"
#include "ImageEditorWorkerCommon.hpp"
#include "../Processing/ImageEffects.hpp"
#include "../Rendering/RenderWorker.hpp"
#include <functional>

using namespace sf;

namespace glxy
{
    class ImageEditorWorker : public ImageEditorWorkerCommon
    {
    public:
        ChunkManager chunkManager;

        LayerID workingLayer = -1;
        Vector2f pixelSelectStart;

        unique_ptr<Image> colorBufferTemp;
        unique_ptr<Image> selectionBufferTemp;

        bool transformImage = false;
        IntRect transformImageSelectionArea;
        unique_ptr<Transformable> transformImageTransform;

        ImageEditorWorker();
        void Empty(Vector2u resolution, Color color);
        bool Open(const filesystem::path& target);

        void ForEachChunkInChunkArea(const IntRect& area, const std::function<void(Vector2i)>& func) const;
        void ForEachPixelInChunkArea(const IntRect& area, bool lockChunks, const std::function<void(Vector2i)>& func) const;

        void OptionCreateLayer(LayerID layerID);
        void OptionDeleteLayer(LayerID layerID);
        void OptionDuplicateLayer(LayerID layerID);
        void OptionMoveLayerUp(LayerID layerID);
        void OptionMoveLayerDown(LayerID layerID);
        void OptionMergeLayerDown(LayerID layerID);
        void OptionFlipLayerHorizontal(LayerID layerID);
        void OptionFlipLayerVertical(LayerID layerID);
        void OptionFlipImageHorizontal();
        void OptionFlipImageVertical();
        void OptionRotate90CW();
        void OptionRotate90CCW();
        void OptionRotate180();
        void OptionCopyToClipboard(Image& target, Vector2u& location, LayerID layerID);
        void OptionPasteFromClipboard(const Image& src, Vector2u location);
        void OptionSelectAll();
        void OptionDeselectAll();
        void OptionDeleteSelected(LayerID layerID);
        void OptionCancel();
        void OptionFinish();
        void OptionSetupTransformImage(LayerID layerID);
        void OptionTransformImage(Vector2f pos, float rot, Vector2f scale, Vector2f origin, bool tile);
        void OptionCropSelection();

        Vector2u getSize() const;

        void BeginSelect(Vector2f pos, SelectMode selectMode, ShapeSelectType type, bool keepAspect);
        void EndSelect(Vector2f pos);

        void SetupMovePixels();
        void MovePixels(const Transform& transform);
        void CancelMovePixels(const IntRect& area);
        void FinishMovePixels(const IntRect& area);

        IntRect TransformImage(const Transform& transformable, bool tile);

        static void InterpolatePixelLine(Vector2f start, Vector2f end, vector<Vector2i>& out);
        void DrawPixels(Vector2f pos, Vector2f prev, Color color, LayerID layerID);
        void BrushPixels(Vector2f start, Vector2f end, float radius, Color color, LayerID layerID, bool eraser);
        bool FillPixels(Vector2i pos, Color color, int8_t tolerance, LayerID layerID);
        void GradientPixels(Vector2f startPos, Vector2f endPos, Color startColor, Color endColor);
        void ResizeCanvas(Vector2u size, Pivot pivot);
        void RescaleCanvas(Vector2u newSize, RescaleMethod method);
        void Adjustment(Adjustments adjustment, LayerID layerID, const AdjustmentData* data, ChunkID chunkID);
        void Effect(Effects effect, LayerID layerID, const EffectData* data, ChunkID chunkID);
        void CancelAdjustmentOrEffect();

        void MergeColorTempLayer(LayerID layerID, const BlendMode& blendMode);
        void AllocateChunks(Vector2u size, Color color);

        uint32_t getChunkIDFromPixel(Vector2u pixel) const;

    };
}