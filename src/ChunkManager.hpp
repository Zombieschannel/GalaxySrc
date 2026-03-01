#pragma once
#include "ImageChunk.hpp"

namespace glxy
{
    enum class ImageCopyType
    {
        Color,
        Selection
    };
    class ChunkManager : public Drawable
    {
        vector<ImageChunk> imageChunks;
        Vector2u chunkCount;
        Vector2u imageSize;
        uint16_t chunkSizeNative;
        uint16_t chunkSizeLow;
        const Tool& currentTool;
        const LayerPicker& layerPicker;
        const View& view;
        const float& windowScale;

    public:
        ChunkManager(const Tool& currentTool, const LayerPicker& layerPicker, const View& view, const float& windowScale);

        Color getPixelColor(Vector2u coord) const;
        Color getPixelColor(Vector2u coord, int16_t layerID) const;
        Color getPixelTemp(Vector2u coord) const;
        bool getPixelSelected(Vector2u coord) const;
        Vector2u getChunkCount() const;
        Vector2u getChunkSize(int16_t chunkID) const;
        Vector2u getChunkSize(Vector2u chunkID) const;
        Vector2u getSize() const;
        uint16_t getChunkSizeNative() const;
        uint16_t getChunkSizeLow() const;
        Image renderWholeImage() const;
        bool hasSelectionLayer() const;

        void setPixelColor(Vector2u coord, Color color);
        void setPixelColor(Vector2u coord, Color color, int16_t layerID);
        void setPixelTemp(Vector2u coord, Color color) const;
        void setPixelSelected(Vector2u coord, bool selected) const;

        void createTempLayer();
        void clearTempLayer(Color color) const;
        void deleteTempLayer();

        void createSelectionLayer();
        void clearSelection() const;
        void updateSelectionTexture(int16_t ID) const;
        void updateSelectionTextureFromImage(int16_t ID, const Image& src) const;
        const Texture* getSelectionTexture(int16_t chunkID) const;
        void deleteSelectionLayer();

        void addLayer(int16_t index, Color color = Color::Transparent);
        void duplicateLayer(int16_t index);
        void deleteLayer(int16_t index);
        void moveLayerUp(int16_t index);
        void moveLayerDown(int16_t index);
        void mergeLayerDown(int16_t index);
        void flipLayerHorizontal(int16_t layerID);
        void flipLayerVertical(int16_t layerID);
        void rotate90CW();
        void rotate90CCW();
        void rotate180();

        IntRect FloodFill(Vector2i pos, Color color, int8_t tolerance, Image* dst, bool mask) const;
        void AllocateChunks(Vector2u size);
        void RenderLQChunk(int16_t chunkID);
        void RenderNQChunk(int16_t chunkID);
        void UpdateLQChunks();
        void UpdateNQChunks();
        void RenderChunkArea(const IntRect& area = IntRect());
        void MergeTempLayer();
        void CopyImage(Image& target, Vector2u dest, const IntRect& area, ImageCopyType type, int16_t layerID = 0) const;
        void PasteImage(const Image& src, Vector2u dest, const IntRect& area, ImageCopyType type, int16_t layerID = 0);
        void MergeImageWithMask(const Image& src, const Image& mask, Vector2i dest, const IntRect& area = IntRect());
        void MergeTempLayerWithMask(Vector2i dest, const IntRect& area = IntRect());
        void SwapLayerWithTemp(int16_t layerID);

    private:
        void draw(RenderTarget& target, RenderStates states) const override;
    };
}