#pragma once
#include "ChunkMapper.hpp"
#include "../Processing/ImageAdjustments.hpp"
#include "../Processing/ImageEffects.hpp"
#include <mutex>
#include <functional>

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
        ChunkMapper<ImageChunk> imageChunks;

        vector<Vector2f> selectLasso;
        IntRect selectShape; // bounding box of current shape selection
        IntRect selectWand; //bounding box of wand selection while not finished
        shared_ptr<IntRect> finalBounds; // bounding box of everything
        unique_ptr<pair<Vector2u, int8_t>> wandCache;
        ShapeSelectType shapeSelectType;

        Vector2u selectStartPos;
        Vector2u selectEndPos = Vector2u(-1, -1);
        Vector2f lassoStartPos;
        Vector2f lassoEndPos = Vector2f(-1, -1);
        bool selectStarted = false;
        bool additiveSelection = true;

        uint8_t tempLayerBlendMode = getBlendMode(BlendAlpha);
        Color backgroundColor = Color::Transparent;


    public:
        mutable std::mutex mtxChunkVector;
        mutable std::mutex mtxChunkManager;

        ChunkManager(bool infinite);

        void ForEachChunkID(const std::function<void(ChunkID)>& func) const;
        void ForEachChunkInChunkArea(const IntRect& area, const std::function<void(Vector2i)>& func) const;

        Color getBackgroundColor() const;

        ChunkID getChunkFromCoord(Vector2i coord) const;

        Color getPixelColor(Vector2i coord, LayerID layerID) const;
        Color getPixelColorTemp(Vector2i coord) const;
        bool getPixelSelection(Vector2i coord) const;
        bool getPixelSelectionTemp(Vector2i coord) const;

        uint32_t getLastUpdated(ChunkID chunkID) const;
        void setLastUpdated(ChunkID chunkID, uint32_t time) const;

        Vector2u getChunkCount() const;
        uint32_t getChunkCountTotal() const;
        Vector2u getChunkSize(ChunkID chunkID) const;
        uint16_t getChunkSize() const;
        Vector2i getChunkPosition(ChunkID chunkID) const;
        Vector2u getSize() const;
        bool isInfinite() const;
        bool chunkExists(ChunkID chunkID) const;

        int16_t getLayerCount() const;
        uint8_t getLayerBlendMode(LayerID layerID) const;
        uint8_t getLayerTransparency(LayerID layerID) const;
        uint8_t getTempLayerBlendMode() const;
        bool isLayerEnabled(LayerID layerID) const;

        std::mutex& getChunkMutex(ChunkID chunkID) const;

        bool hasSelectionLayer(ChunkID chunkID) const;
        bool allHaveSelectionLayer() const;
        bool hasSelectionTempLayer(ChunkID chunkID) const;
        bool anyHasSelectionTempLayer() const;
        bool hasColorTempLayer(ChunkID chunkID) const;
        bool allHaveColorTempLayer() const;

        bool needsUpdateColorLow(ChunkID chunkID) const;
        bool needsUpdateColorMedium(ChunkID chunkID) const;
        bool needsUpdateColorNative(ChunkID chunkID) const;
        bool needsUpdateSelection(ChunkID chunkID) const;
        bool needsUpdateSelectionTemp(ChunkID chunkID) const;

        const Image* getChunkImageColor(ChunkID chunkID, LayerID layerID) const;
        const Image* getChunkImageSelection(ChunkID chunkID) const;
        const Image* getChunkImageColorTemp(ChunkID chunkID) const;
        const Image* getChunkImageSelectionTemp(ChunkID chunkID) const;

        bool hasStartedSelect() const;
        weak_ptr<const IntRect> getFinalSelectionBounds() const;
        IntRect getBoxSelectArea() const;
        ShapeSelectType getShapeSelectType() const;
        bool isSelectionAdditive() const;
        Vector2f getLassoSelectPoint(int32_t index) const;
        int32_t getLassoSelectPointCount() const;

        void resetSelection();
        void selectArea(const FloatRect& area);

        void boxSelectStart(Vector2u startPos, bool additive, ShapeSelectType type);
        void boxSelectEnd(Vector2u endPos);
        void lassoSelectStart(Vector2f startPos, bool additive);
        void lassoSelectEnd(Vector2f endPos);
        void selectFinish();


        void wandSelect(Vector2u pos, int8_t tolerance, LayerID layerID);
        void wandCancel();
        void wandFinish();

        void setNewSelectionBounds(const IntRect& newBounds);

        void CopySelectedColorPixels(Image& dst, Vector2u& location, LayerID layerID) const;
        void PasteSelectedColorPixels(const Image& src, LayerID layerID);

        void lockAllChunks() const;
        void unlockAllChunks() const;

        void setPixelColor(Vector2i coord, Color color, LayerID layerID);
        void setPixelColorTemp(Vector2i coord, Color color);
        void setPixelSelection(Vector2i coord, bool selected);
        void setPixelSelectionTemp(Vector2i coord, bool selected);

        void setLayerEnabled(LayerID layerID, bool enabled);
        void setLayerTransparency(LayerID layerID, uint8_t transparency);
        void setLayerBlendMode(LayerID layerID, uint8_t blendMode);

        void createColorTempLayerAll(uint8_t blendMode);
        void createColorTempLayer(uint8_t blendMode, ChunkID chunkID);
        void clearColorTempLayerAll(Color color);
        void clearColorTempLayer(Color color, ChunkID chunkID);
        void deleteColorTempLayerAll();


        void createSelectionLayer(ChunkID chunkID);
        void clearSelectionLayer(bool state, ChunkID chunkID) const;
        void deleteSelectionLayerAll();

        void createSelectionTempLayerAll();
        void clearSelectionTempLayerAll();
        void deleteSelectionTempLayerAll();

        void addLayer(LayerID layerID);
        void clearLayer(LayerID layerID, Color color = Color::Transparent);
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
        void AllocateChunksFixed(Vector2u size);
        void AllocateChunkInfinite(Vector2i chunkID);
        void MergeColorTempLayer(LayerID layerID, const BlendMode& blendMode);

        void CopyImage(Image& target, Vector2i dest, const IntRect& area, ImageLayerType type, LayerID layerID = 0) const;
        void PasteImage(const Image& src, Vector2i dest, const IntRect& area, ImageLayerType type, LayerID layerID = 0);

        void setBackgroundColor(Color color);
        void CopyImageInternal(ImageLayerType src, ImageLayerType dst, const IntRect& area, LayerID layerSrc = 0, LayerID layerDst = 0);

        void Adjust(Adjustments adjustment, const IntRect& area, LayerID layerID, const AdjustmentData* data);
        void Effect(Effects effect, const IntRect& area, LayerID layerID, const EffectData* data);
        void InvalidateColorTextures();
        void setUpdatedColorNative(ChunkID chunkID) const;
        void setUpdatedColorMedium(ChunkID chunkID) const;
        void setUpdatedColorLow(ChunkID chunkID) const;
        void setUpdatedSelection(ChunkID chunkID) const;
        void setUpdatedSelectionTemp(ChunkID chunkID) const;
    };
}
