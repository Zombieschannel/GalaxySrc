#pragma once
#include <SFML/Graphics.hpp>

#include "ChunkManager.hpp"
#include "Namespace.hpp"
#include "ToolPicker.hpp"
#include "ImageChunk.hpp"

using namespace sf;
namespace glxy
{
    enum class ShapeSelectType
    {
        Box,
        Circle
    };
    class PixelSelect : public Drawable
    {
        class PixelSelectAnimation : public Drawable
        {
            Vector2i prevSize;
            VertexArray arr;
            float offset = 0.f;
            const View& view;
        public:
            PixelSelectAnimation(const View& view, Color color);
            void Update();
            void draw(RenderTarget& target, RenderStates states) const override;
        };
        PixelSelectAnimation animCenter;
        PixelSelectAnimation animOutline;

        unique_ptr<Image> tempSelectedArea;
        Vector2i tempSelectedAreaPosition;
        pair<IntRect, bool> selectShape; // bounding box of current shape selection
        IntRect selectWand; //bounding box of wand selection while not finished
        unique_ptr<IntRect> finalBounds; // bounding box of everything
        Image selectedWand;
        ShapeSelectType shapeSelectType;

        bool selectionActive = false;

        Vector2u startPos;
        Color selectColor;
        ChunkManager& chunkManager;
        const float& windowScale;
        const bool& drawSelectionLines;
        const Tool& currentTool;
        bool started = false;
    public:
        Vector2u endPos = Vector2u(-1, -1);
        PixelSelect(const float& windowScale, const View& view, const Tool& currentTool, ChunkManager& chunkManager, const bool& drawSelectionLines);
        bool hasStartedBoxSelect() const;
        IntRect* getFinalSelectionBounds() const;
        IntRect getBoxSelectArea() const;
        bool withinTempBounds(Vector2u pos) const;
        const Image* getTempMask() const;

        void clear();
        void setSize(Vector2u size);

        void selectAll();

        void boxShapeStart(Vector2u startPos, bool additive, ShapeSelectType type);
        void boxShapeEnd(Vector2u endPos);
        void boxShapeFinish();

        void wandSelect(Vector2u pos, bool additive, int8_t tolerance);
        void wandClear();
        void wandFinish();

        void createTempSelection(Vector2u size);
        void createTempSelectionFromSelected();
        void copyTempSelection(const Image& image) const;
        void setTempSelectedAreaPosition(Vector2i pos);
        void revertTempSelection();
        void setSelectColor(Color selectColor);

        void setNewBounds(const IntRect& newBounds);

        void clearSelectedPixels() const;
        void copySelectedPixels(Image& dst, Vector2u& location, int16_t layerID) const;

        void UpdateTexture(const IntRect& area) const;
        void Update();

    private:
        void draw(RenderTarget& target, RenderStates states) const override;
    };
}