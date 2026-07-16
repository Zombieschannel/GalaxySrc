#pragma once
#include <SFML/Graphics.hpp>
#include "../Config.hpp"
#include "../Pickers/ColorPicker.hpp"
#include "../Pickers/LayerPicker.hpp"
#include "../Pickers/ToolPicker.hpp"
#include "../Namespace.hpp"
#include "../PopUpState.hpp"
#include "ChunkManager.hpp"
#include "ImageEditorWorkerCommon.hpp"
#include "../Processing/ImageEffects.hpp"
#include <functional>

using namespace sf;

namespace glxy
{
    class ImageEditorWorker : public ImageEditorWorkerCommon
    {
        struct TransformImageCache
        {
            vector<pair<IntRect, Texture>> colorBufferChunks;
            vector<pair<IntRect, Texture>> selectionBufferChunks;
            unique_ptr<Image> colorBufferTemp;
            unique_ptr<Image> selectionBufferTemp;
            bool requiresUpdateColor = false;
            bool requiresUpdateSelection = false;
        };
    public:
        ChunkManager chunkManager;

        LayerID workingLayer = -1;
        Vector2f pixelSelectStart;

        bool transformImage = false;
        IntRect transformImageSelectionArea;
        unique_ptr<Transformable> transformImageTransform;

        IntRect prevTextRenderArea;
        TransformImageCache transformImageCache;

        ImageEditorWorker(bool infinite);
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
        void OptionCopyToClipboard(Image& target, Vector2u& location, Image& selectionTarget, LayerID layerID);
        void OptionPasteFromClipboard(const Image& src, Vector2u location, const Image& selection);
        void OptionSelectArea(const FloatRect& area);
        void OptionDeselectAll();
        void OptionDeleteSelected(LayerID layerID);
        void OptionCancel();
        void OptionFinish();
        void OptionSetupTransformImage(LayerID layerID);
        void OptionTransformImage(Vector2f pos, float rot, Vector2f scale, Vector2f origin, int16_t repeat, bool smooth);
        void OptionSetupCircularShift();
        void OptionCircularShift(int32_t shift, bool horizontal, int32_t groupSize);
        void OptionCropSelection();

        Vector2u getSize() const;

        void BeginSelect(Vector2f pos, SelectMode selectMode, ShapeSelectType type, bool keepAspect);
        void EndSelect(Vector2f pos);

        void SetupMovePixels();
        void MovePixels(const Transform& transform, bool smooth);
        void CancelMovePixels(const IntRect& area);
        void FinishMovePixels(const IntRect& area);

        void SetupMoveSelection();
        void MoveSelection(const Transform& transform);
        void CancelMoveSelection(const IntRect& area);
        void FinishMoveSelection(const IntRect& area);

        IntRect TransformImage(const Transform& transform, int16_t repeat, bool smooth, bool selectionOnly);

        void SetupMaskedRendering(RenderTexture& renderTexture, ChunkID chunkID) const;
        static void InterpolatePixelLine(Vector2f start, Vector2f end, vector<Vector2i>& out);
        void ChunksInBrushLine(Vector2f start, Vector2f end, float radius, vector<ChunkID>& out) const;
        void PencilPixels(Vector2f pos, Vector2f prev, Color color);
        void BrushPixels(Vector2f start, Vector2f end, float radius, Color color, bool eraser);
        void ColorSwapPixels(Vector2f start, Vector2f end, float radius, Color color1, Color color2, int8_t tol, LayerID layerID);
        bool FillPixels(Vector2i pos, Color color, int8_t tolerance, LayerID layerID);
        void GradientPixels(Vector2f startPos, Vector2f endPos, Color startColor, Color endColor);
        void ShapePixels(const shared_ptr<ConvexShape>& shape);
        void TextPixels(const shared_ptr<Text>& text, const FloatRect& globalBounds);

        void ResizeCanvas(Vector2u size, Pivot pivot);
        void RescaleCanvas(Vector2u newSize, RescaleMethod method);
        void Adjustment(Adjustments adjustment, LayerID layerID, const AdjustmentData* data, ChunkID chunkID);
        void Effect(Effects effect, LayerID layerID, const EffectData* data, ChunkID chunkID);
        void CancelAdjustmentOrEffect();

        void MergeColorTempLayer(LayerID layerID, const BlendMode& blendMode);
        void AllocateChunks(Vector2u size);

        uint32_t getChunkIDFromPixel(Vector2u pixel) const;

    };
}