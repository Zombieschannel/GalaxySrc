#pragma once
#include <functional>

#include "ImageChunk.hpp"
#include "../Processing/ImageAdjustments.hpp"
#include "../Processing/ImageEffects.hpp"
#include <mutex>

namespace glxy
{
    class ChunkManager
    {
        struct Layer
        {
            bool enabled = true;
            uint8_t transparency = 255;
            uint8_t blendMode = 2;
        };
        vector<Layer> layers;

        uint16_t chunkSize;
        Vector2u imageSize;
        Vector2u chunkCount;
        vector<ImageChunk> imageChunks;

        IntRect selectShape; // bounding box of current shape selection
        IntRect selectWand; //bounding box of wand selection while not finished
        shared_ptr<IntRect> finalBounds; // bounding box of everything
        unique_ptr<pair<Vector2u, int8_t>> wandCache;
        ShapeSelectType shapeSelectType;

        Vector2u selectStartPos;
        Vector2u selectEndPos = Vector2u(-1, -1);
        bool selectStarted = false;
        bool additiveSelection = true;

        uint8_t tempLayerBlendMode = getBlendMode(BlendAlpha);

    public:
        mutable std::mutex mtxChunkVector;
        mutable std::mutex mtxChunkManager;

        ChunkManager();

        void ForEachChunkInChunkArea(const IntRect& area, const std::function<void(Vector2i)>& func) const;

        Color getPixelColor(Vector2u coord, LayerID layerID) const;
        Color getPixelColorTemp(Vector2u coord) const;
        bool getPixelSelection(Vector2u coord) const;
        bool getPixelSelectionTemp(Vector2u coord) const;


        Vector2u getChunkCount() const;
        Vector2u getChunkSize(ChunkID chunkID) const;
        Vector2u getChunkSize(Vector2u chunkID) const;
        uint16_t getChunkSize() const;
        Vector2u getSize() const;
        ChunkID getChunkID(Vector2u chunk) const;

        int16_t getLayerCount() const;
        uint8_t getLayerBlendMode(LayerID layerID) const;
        uint8_t getLayerTransparency(LayerID layerID) const;
        uint8_t getTempLayerBlendMode() const;
        bool isLayerEnabled(LayerID layerID) const;

        std::mutex& getChunkMutex(Vector2u chunkID) const;
        std::mutex& getChunkMutex(ChunkID chunkID) const;

        bool hasSelectionLayer(ChunkID chunkID) const;
        bool hasSelectionTempLayer(ChunkID chunkID) const;
        bool hasColorTempLayer(ChunkID chunkID) const;

        bool needsUpdateColorLow(ChunkID chunkID) const;
        bool needsUpdateColorNative(ChunkID chunkID) const;
        bool needsUpdateSelection(ChunkID chunkID) const;
        bool needsUpdateSelectionTemp(ChunkID chunkID) const;

        const Image* getChunkImageColor(ChunkID chunkID, LayerID layerID) const;
        const Image* getChunkImageSelection(ChunkID chunkID) const;
        const Image* getChunkImageColorTemp(ChunkID chunkID) const;
        const Image* getChunkImageSelectionTemp(ChunkID chunkID) const;

        bool hasStartedBoxSelect() const;
        weak_ptr<const IntRect> getFinalSelectionBounds() const;
        IntRect getBoxSelectArea() const;
        ShapeSelectType getShapeSelectType() const;
        bool isSelectionAdditive() const;

        void resetSelection();
        void selectAll();

        void boxShapeStart(Vector2u startPos, bool additive, ShapeSelectType type);
        void boxShapeEnd(Vector2u endPos);
        void boxShapeFinish();

        void wandSelect(Vector2u pos, int8_t tolerance, LayerID layerID);
        void wandCancel();
        void wandFinish();

        void setNewSelectionBounds(const IntRect& newBounds);

        void CopySelectedColorPixels(Image& dst, Vector2u& location, LayerID layerID) const;
        void PasteSelectedColorPixels(const Image& src, LayerID layerID);

        void lockAllChunks() const;
        void unlockAllChunks() const;

        void setPixelColor(Vector2u coord, Color color, LayerID layerID);
        void setPixelColorTemp(Vector2u coord, Color color);
        void setPixelSelection(Vector2u coord, bool selected);
        void setPixelSelectionTemp(Vector2u coord, bool selected);

        void setLayerEnabled(LayerID layerID, bool enabled);
        void setLayerTransparency(LayerID layerID, uint8_t transparency);
        void setLayerBlendMode(LayerID layerID, uint8_t blendMode);

        void createColorTempLayerAll(uint8_t blendMode);
        void createColorTempLayer(uint8_t blendMode, ChunkID chunkID);
        void clearColorTempLayerAll(Color color);
        void clearColorTempLayer(Color color, ChunkID chunkID);
        void deleteColorTempLayer();

        void createSelectionLayer();
        void clearSelectionLayer(bool state) const;
        void deleteSelectionLayer();

        void createSelectionTempLayer();
        void clearSelectionTempLayer();
        void deleteSelectionTempLayer();

        void addLayer(LayerID layerID, Color color = Color::Transparent);
        void duplicateLayer(LayerID layerID);
        void deleteLayer(LayerID layerID);
        void moveLayerUp(LayerID layerID);
        void moveLayerDown(LayerID layerID);
        void mergeLayerDown(LayerID layerID);
        void flipLayerHorizontal(LayerID layerID);
        void flipLayerVertical(LayerID layerID);
        void rotate90CW();
        void rotate90CCW();
        void rotate180();

        IntRect FloodFill(ImageLayerType layer, Vector2i pos, Color color, int8_t tolerance, bool mask, LayerID layerID);
        void AllocateChunks(Vector2u size, Color color);
        void MergeColorTempLayer(LayerID layerID, const BlendMode& blendMode);

        void CopyImage(Image& target, Vector2u dest, const IntRect& area, ImageLayerType type, LayerID layerID = 0) const;
        void PasteImage(const Image& src, Vector2u dest, const IntRect& area, ImageLayerType type, LayerID layerID = 0);

        void CopyImageInternal(ImageLayerType src, ImageLayerType dst, const IntRect& area, LayerID layerSrc = 0, LayerID layerDst = 0);

        void Adjust(Adjustments adjustment, const IntRect& area, LayerID layerID, const AdjustmentData* data);
        void Effect(Effects effect, const IntRect& area, LayerID layerID, const EffectData* data);
        void InvalidateColorTextures();
        void setUpdatedColorNative(ChunkID chunkID) const;
        void setUpdatedColorLow(ChunkID chunkID) const;
        void setUpdatedSelection(ChunkID chunkID) const;
        void setUpdatedSelectionTemp(ChunkID chunkID) const;
    };
}
