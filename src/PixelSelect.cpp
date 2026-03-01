#include "PixelSelect.hpp"
#include <SFML/OpenGL.hpp>

#include "Func.hpp"
#include "GlobalClock.hpp"
#include "inc/ZTB.hpp"

const int32_t c_animationLineCount = 80;

glxy::PixelSelect::PixelSelectAnimation::PixelSelectAnimation(const View& view, const Color color)
    : view(view)
{
    arr.setPrimitiveType(PrimitiveType::Triangles);
    const int32_t count = 2 * c_animationLineCount + 1;
    arr.resize(6 * count);
    for (int32_t i = 0; i < count; i++)
    {
        for (int32_t j = 0; j < 6; j++)
            arr[6 * i + j].color = color;

        if (i < c_animationLineCount)
        {
            arr[6 * i + 0].position = Vector2f(-0.5f, i);
            arr[6 * i + 3].position = Vector2f(-0.5f, i + 0.5f);
        }
        else
        {
            arr[6 * i + 0].position = Vector2f(-0.5f + (i - c_animationLineCount), c_animationLineCount);
            arr[6 * i + 3].position = Vector2f(i - c_animationLineCount, c_animationLineCount);
        }

        if (i < c_animationLineCount)
        {
            arr[6 * i + 1].position = Vector2f(i, -0.5f);
            arr[6 * i + 2].position = Vector2f(i + 0.5f, -0.5f);
        }
        else
        {
            arr[6 * i + 1].position = Vector2f(c_animationLineCount, -0.5f + (i - c_animationLineCount));
            arr[6 * i + 2].position = Vector2f(c_animationLineCount, i - c_animationLineCount);
        }

        arr[6 * i + 4].position = arr[6 * i + 0].position;
        arr[6 * i + 5].position = arr[6 * i + 2].position;
    }
}

void glxy::PixelSelect::PixelSelectAnimation::Update()
{
    offset += TimeControl::DeltaReal().asSeconds();
    offset = fmod(offset, 1.f);
}

void glxy::PixelSelect::PixelSelectAnimation::draw(RenderTarget& target, RenderStates states) const
{
    Transform t;
    t.translate({offset * 0.5f, offset * 0.5f});
    states.transform *= t;

    Vector2f size;
    if (view.getSize().x > view.getSize().y)
    {
        size.x = c_animationLineCount;
        size.y = c_animationLineCount * (view.getSize().y / view.getSize().x);
    }
    else
    {
        size.y = c_animationLineCount;
        size.x = c_animationLineCount * (view.getSize().x / view.getSize().y);
    }
    target.setView(View({c_animationLineCount / 2.f, c_animationLineCount / 2.f}, size));
    target.draw(arr, states);
    target.setView(view);
}

glxy::PixelSelect::PixelSelect(const float& windowScale, const View& view, const Tool& currentTool, ChunkManager& chunkManager, const bool& drawSelectionLines)
    : animCenter(view, Color(128, 128, 128, 64)), animOutline(view, Color(0, 0, 0, 255)), shapeSelectType(ShapeSelectType::Box),
      chunkManager(chunkManager), windowScale(windowScale), currentTool(currentTool), drawSelectionLines(drawSelectionLines)
{
}

bool glxy::PixelSelect::hasStartedBoxSelect() const
{
    return started;
}

void glxy::PixelSelect::clear()
{
    chunkManager.deleteSelectionLayer();
    tempSelectedArea.reset();
    finalBounds.reset();
    selectionActive = false;
    started = false;
}

void glxy::PixelSelect::setSize(const Vector2u size)
{
    selectedWand.resize(size, Color::Transparent);
}

void glxy::PixelSelect::selectAll()
{
    boxShapeStart(Vector2u(0, 0), true, ShapeSelectType::Box);
    boxShapeEnd(chunkManager.getSize() - Vector2u(1, 1));
    boxShapeFinish();
}

void glxy::PixelSelect::boxShapeStart(const Vector2u startPos, const bool additive, const ShapeSelectType type)
{
    shapeSelectType = type;
    selectShape = pair(IntRect(Vector2i(startPos), {0, 0}), additive);
    this->startPos = startPos;
    started = true;
    selectionActive = true;
}

void glxy::PixelSelect::boxShapeEnd(const Vector2u endPos)
{
    if (this->endPos == endPos)
        return;
    const uint16_t chunkSizeNative = chunkManager.getChunkSizeNative();
    IntRect areaToUpdate = selectShape.first;

    Vector2i minPos, maxPos;
    minPos.x = min(startPos.x, endPos.x);
    maxPos.x = max(startPos.x, endPos.x);
    minPos.y = min(startPos.y, endPos.y);
    maxPos.y = max(startPos.y, endPos.y);
    selectShape.first = IntRect(minPos, maxPos - minPos + Vector2i(1, 1));

    this->endPos = endPos;

    if (areaToUpdate == IntRect())
        areaToUpdate = selectShape.first;

    for (int32_t x = min(areaToUpdate.position.x, selectShape.first.position.x) / chunkSizeNative;
            x <= (areaToUpdate.position.x + areaToUpdate.size.x - 1) / chunkSizeNative || x <= (selectShape.first.position.x + selectShape.first.size.x - 1) / chunkSizeNative; x++)
        for (int32_t y = min(areaToUpdate.position.y, selectShape.first.position.y) / chunkSizeNative;
                y <= (areaToUpdate.position.y + areaToUpdate.size.y - 1) / chunkSizeNative || y <= (selectShape.first.position.y + selectShape.first.size.y - 1) / chunkSizeNative; y++)
        {
            const int32_t ID = x + y * chunkManager.getChunkCount().x;
            const IntRect imgArea = IntRect(Vector2i(x * chunkSizeNative, y * chunkSizeNative),
                Vector2i(chunkManager.getChunkSize(ID)));
            const std::optional intersection = selectShape.first.findIntersection(imgArea);
            Image img;
            img.resize(Vector2u(imgArea.size), Color::Transparent);
            chunkManager.CopyImage(img, Vector2u(), imgArea, ImageCopyType::Selection);
            if (intersection.has_value())
            {
                if (shapeSelectType == ShapeSelectType::Box)
                {
                    Image box;
                    box.resize(Vector2u(intersection->size), selectShape.second ? Color::Black : Color::Transparent);
                    validate(img.copy(box, Vector2u(intersection->position.x % chunkSizeNative, intersection->position.y % chunkSizeNative), IntRect()));
                }
                else if (shapeSelectType == ShapeSelectType::Circle)
                {
                    Image circle;
                    circle.resize(Vector2u(intersection->size), selectShape.second ? Color::Transparent : Color::Black);
                    for (int32_t ix = 0; ix < intersection->size.x; ix++)
                        for (int32_t iy = 0; iy < intersection->size.y; iy++)
                        {
                            const Vector2f checkPixel = Vector2f((intersection->position.x + ix + 0.5f - selectShape.first.size.x / 2.f - selectShape.first.position.x) / selectShape.first.size.x,
                                (intersection->position.y + iy + 0.5f - selectShape.first.size.y / 2.f - selectShape.first.position.y) / selectShape.first.size.y);
                            if (checkPixel.x * checkPixel.x + checkPixel.y * checkPixel.y < 0.5f * 0.5f)
                                circle.setPixel(Vector2u(ix, iy), selectShape.second ? Color::Black : Color::Transparent);
                        }
                    const Vector2i offset = Vector2i(intersection->position.x % chunkSizeNative, intersection->position.y % chunkSizeNative);
                    for (int32_t ix = 0; ix < circle.getSize().x; ix++)
                        for (int32_t iy = 0; iy < circle.getSize().y; iy++)
                        {
                            const Color targetPixel = circle.getPixel(Vector2u(ix, iy));
                            if (targetPixel == Color::Transparent && selectShape.second || targetPixel == Color::Black && !selectShape.second)
                                continue;
                            img.setPixel(Vector2u(offset + Vector2i(ix, iy)),
                                targetPixel == Color::Black && selectShape.second ? Color::Black : Color::Transparent);
                        }
                }
            }
            chunkManager.updateSelectionTextureFromImage(ID, img);
        }
}

void glxy::PixelSelect::boxShapeFinish()
{
    tempSelectedArea.reset();
    tempSelectedAreaPosition = Vector2i();

    started = false;
    if (shapeSelectType == ShapeSelectType::Box)
    {
        Image temp;
        temp.resize(Vector2u(selectShape.first.size), selectShape.second ? Color::Black : Color::Transparent);
        chunkManager.PasteImage(temp, Vector2u(selectShape.first.position), IntRect(), ImageCopyType::Selection);
    }
    else if (shapeSelectType == ShapeSelectType::Circle)
    {
        Image temp;
        temp.resize(Vector2u(selectShape.first.size), selectShape.second ? Color::Transparent : Color::Black);
        for (int32_t x = 0; x < selectShape.first.size.x; x++)
            for (int32_t y = 0; y < selectShape.first.size.y; y++)
            {
                const Vector2f checkPixel = Vector2f((x + 0.5f - selectShape.first.size.x / 2.f) / selectShape.first.size.x,
                    (y + 0.5f - selectShape.first.size.y / 2.f) / selectShape.first.size.y);
                if (checkPixel.x * checkPixel.x + checkPixel.y * checkPixel.y < 0.5f * 0.5f)
                    temp.setPixel(Vector2u(x, y), selectShape.second ? Color::Black : Color::Transparent);
            }
        for (int32_t x = 0; x < temp.getSize().x; x++)
            for (int32_t y = 0; y < temp.getSize().y; y++)
            {
                const Color targetPixel = temp.getPixel(Vector2u(x, y));
                if (targetPixel == Color::Transparent && selectShape.second || targetPixel == Color::Black && !selectShape.second)
                    continue;
                chunkManager.setPixelSelected(Vector2u(selectShape.first.position + Vector2i(x, y)), targetPixel == Color::Black && selectShape.second);
            }
    }

    finalBounds = make_unique<IntRect>(getUnion(&selectShape.first, finalBounds.get()));
    endPos = Vector2u(-1, -1);
}

void glxy::PixelSelect::wandSelect(const Vector2u pos, const bool additive, const int8_t tolerance)
{
    if (this->endPos == pos)
        return;
    const uint16_t chunkSizeNative = chunkManager.getChunkSizeNative();
    this->endPos = pos;
    const IntRect area = chunkManager.FloodFill(Vector2i(pos), Color::Black, tolerance, &selectedWand, false);

    if (selectWand == IntRect())
        selectWand = area;

    for (int32_t x = min(selectWand.position.x, area.position.x) / chunkSizeNative;
            x <= (selectWand.position.x + selectWand.size.x - 1) / chunkSizeNative || x <= (area.position.x + area.size.x - 2) / chunkSizeNative; x++)
        for (int32_t y = min(selectWand.position.y, area.position.y) / chunkSizeNative;
                y <= (selectWand.position.y + selectWand.size.y - 1) / chunkSizeNative || y <= (area.position.y + area.size.y - 2) / chunkSizeNative; y++)
        {
            const int32_t ID = x + y * chunkManager.getChunkCount().x;
            const IntRect imgArea = IntRect(Vector2i(x * chunkSizeNative, y * chunkSizeNative),
                Vector2i(chunkManager.getChunkSize(ID)));
            Image img;
            img.resize(Vector2u(imgArea.size), Color::Transparent);
            chunkManager.CopyImage(img, Vector2u(), imgArea, ImageCopyType::Selection);
            for (int32_t ix = 0; ix < img.getSize().x; ix++)
                for (int32_t iy = 0; iy < img.getSize().y; iy++)
                {
                    const Color targetPixel = selectedWand.getPixel(Vector2u(ix + x * chunkSizeNative, iy + y * chunkSizeNative));
                    if (targetPixel == Color::Transparent)
                        continue;
                    img.setPixel(Vector2u(Vector2i(ix, iy)),
                        targetPixel == Color::Black ? Color::Black : Color::Transparent);
                }
            chunkManager.updateSelectionTextureFromImage(ID, img);
        }
    selectWand = area;
    selectionActive = true;
}

void glxy::PixelSelect::wandClear()
{
    tempSelectedArea.reset();
    tempSelectedAreaPosition = Vector2i();
    selectedWand.resize(selectedWand.getSize(), Color::Transparent);
    selectWand = IntRect();
    endPos = Vector2u(-1, -1);
}

void glxy::PixelSelect::wandFinish()
{
    tempSelectedArea.reset();
    tempSelectedAreaPosition = Vector2i();

    for (int32_t x = selectWand.position.x; x < selectWand.position.x + selectWand.size.x; x++)
        for (int32_t y = selectWand.position.y; y < selectWand.position.y + selectWand.size.y; y++)
        {
            const Color targetPixel = selectedWand.getPixel(Vector2u(x, y));
            if (targetPixel == Color::Transparent)
                continue;
            chunkManager.setPixelSelected(Vector2u(Vector2i(x, y)), targetPixel == Color::Black);
        }

    finalBounds = make_unique<IntRect>(getUnion(finalBounds.get(), &selectWand));
    selectWand = IntRect();
    endPos = Vector2u(-1, -1);
}

void glxy::PixelSelect::createTempSelection(const Vector2u size)
{
    if (!tempSelectedArea)
    {
        tempSelectedArea = make_unique<Image>();
        tempSelectedArea->resize(size, Color::Transparent);
    }
}

void glxy::PixelSelect::createTempSelectionFromSelected()
{
    const IntRect* bounds = getFinalSelectionBounds();
    assert(bounds);
    if (!tempSelectedArea)
    {
        createTempSelection(Vector2u(bounds->size));
        const std::optional<IntRect> intersect = bounds->findIntersection(IntRect({0, 0}, Vector2i(chunkManager.getSize())));
        if (intersect)
            chunkManager.CopyImage(*tempSelectedArea, Vector2u(intersect->position - bounds->position), *intersect, ImageCopyType::Selection);
    }
    tempSelectedAreaPosition = bounds->position;
}

void glxy::PixelSelect::copyTempSelection(const Image& image) const
{
    tempSelectedArea->resize(image.getSize(), Color::Transparent);
    validate(tempSelectedArea->copy(image, Vector2u()));
}

void glxy::PixelSelect::setTempSelectedAreaPosition(const Vector2i pos)
{
    tempSelectedAreaPosition = pos;
}

void glxy::PixelSelect::revertTempSelection()
{
    if (!tempSelectedArea)
        return;

    chunkManager.clearSelection();
    const std::optional<IntRect> intersect = IntRect(tempSelectedAreaPosition, Vector2i(tempSelectedArea->getSize())).findIntersection(
        IntRect({0, 0}, Vector2i(chunkManager.getSize())));
    if (intersect)
    {
        chunkManager.PasteImage(*tempSelectedArea, Vector2u(intersect->position),
            IntRect(intersect->position - Vector2i(tempSelectedAreaPosition), intersect->size), ImageCopyType::Selection);
    }
    tempSelectedAreaPosition = Vector2i();
}

void glxy::PixelSelect::setSelectColor(const Color selectColor)
{
    this->selectColor = Color(selectColor.r, selectColor.g, selectColor.b, 64);
}

void glxy::PixelSelect::setNewBounds(const IntRect& newBounds)
{
    finalBounds = make_unique<IntRect>(newBounds);
}

IntRect* glxy::PixelSelect::getFinalSelectionBounds() const
{
    return finalBounds.get();
}

IntRect glxy::PixelSelect::getBoxSelectArea() const
{
    return selectShape.first;
}

bool glxy::PixelSelect::withinTempBounds(const Vector2u pos) const
{
    return tempSelectedArea->getPixel(pos).a > 128;
}

void glxy::PixelSelect::copySelectedPixels(Image& dst, Vector2u& location, const int16_t layerID) const
{
    const IntRect* bounds = getFinalSelectionBounds();
    assert(bounds);
    dst.resize(Vector2u(bounds->size), Color::Transparent);
    location = Vector2u(bounds->position);
    for (int32_t x = bounds->position.x; x < bounds->position.x + bounds->size.x; x++)
        for (int32_t y = bounds->position.y; y < bounds->position.y + bounds->size.y; y++)
            if (chunkManager.getPixelSelected(Vector2u(x, y)))
                dst.setPixel(Vector2u(x - bounds->position.x, y - bounds->position.y), chunkManager.getPixelColor(Vector2u(x, y), layerID));
}

void glxy::PixelSelect::clearSelectedPixels() const
{
    const IntRect* bounds = getFinalSelectionBounds();
    assert(bounds);
    for (int32_t x = bounds->position.x; x < bounds->position.x + bounds->size.x; x++)
        for (int32_t y = bounds->position.y; y < bounds->position.y + bounds->size.y; y++)
            if (chunkManager.getPixelSelected(Vector2u(x, y)))
                chunkManager.setPixelColor(Vector2u(x, y), Color::Transparent);
}

const Image* glxy::PixelSelect::getTempMask() const
{
    return tempSelectedArea.get();
}

void glxy::PixelSelect::UpdateTexture(const IntRect& area) const
{
    if (!area.findIntersection(IntRect({0, 0}, Vector2i(chunkManager.getSize()))))
        return;
    const uint16_t chunkSizeNative = chunkManager.getChunkSizeNative();
    for (int32_t x = max(0, area.position.x / chunkSizeNative);
        x < min(static_cast<float>(chunkManager.getChunkCount().x), ceil(static_cast<float>(area.position.x + area.size.x) / chunkSizeNative)); x++)
        for (int32_t y = max(0, area.position.y / chunkSizeNative);
            y < min(static_cast<float>(chunkManager.getChunkCount().y), ceil(static_cast<float>(area.position.y + area.size.y) / chunkSizeNative)); y++)
        {
            chunkManager.updateSelectionTexture(x + y * chunkManager.getChunkCount().x);
        }
}

void glxy::PixelSelect::Update()
{
    animCenter.Update();
    animOutline.Update();
}

void glxy::PixelSelect::draw(RenderTarget& target, RenderStates states) const
{
    if (!chunkManager.hasSelectionLayer() && !getTempMask())
        return;
    const uint16_t chunkSizeNative = chunkManager.getChunkSizeNative();
    const float offset = windowScale * 1;
    target.clearStencil(0x00);
#ifdef GL_ALPHA_TEST
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
#endif
    for (int32_t i = 0; i < chunkManager.getChunkCount().x * chunkManager.getChunkCount().y; i++)
    {
        const Vector2u chunkID = Vector2u(i % chunkManager.getChunkCount().x, i / chunkManager.getChunkCount().x);
        const Vector2u size = chunkManager.getChunkSize(i);
        const std::array v = {
            Vertex{Vector2f(chunkID.x * chunkSizeNative, chunkID.y * chunkSizeNative), Color::White, Vector2f(0, 0)},
            Vertex{Vector2f(chunkID.x * chunkSizeNative + size.x, chunkID.y * chunkSizeNative), Color::White, Vector2f(1, 0)},
            Vertex{Vector2f(chunkID.x * chunkSizeNative + size.x, chunkID.y * chunkSizeNative + size.y), Color::White, Vector2f(1, 1)},
            Vertex{Vector2f(chunkID.x * chunkSizeNative, chunkID.y * chunkSizeNative + size.y), Color::White, Vector2f(0, 1)},
        };

        Transformable t;
        const std::array offsets = {Vector2f(-offset, -offset), Vector2f(offset, -offset), Vector2f(offset, offset), Vector2f(-offset, offset)};
        for (int32_t j = 0; j < offsets.size(); j++)
        {
            t.setPosition(offsets.at(j));
            target.draw(v.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendNone,
                {StencilComparison::Always, StencilUpdateOperation::Increment, StencilValue(0x01), 0xFF, true},
                t.getTransform(), CoordinateType::Normalized, chunkManager.getSelectionTexture(i), nullptr));
        }
    }
#ifdef GL_ALPHA_TEST
    glDisable(GL_ALPHA_TEST);
#endif
    const IntRect bounds = IntRect({}, Vector2i(chunkManager.getSize()));
    const std::array v = {
        Vertex{Vector2f(bounds.position), selectColor},
        Vertex{Vector2f(bounds.position.x + bounds.size.x, bounds.position.y), selectColor},
        Vertex{Vector2f(bounds.position + bounds.size), selectColor},
        Vertex{Vector2f(bounds.position.x, bounds.position.y + bounds.size.y), selectColor},
    };
    const std::array v2 = {
        Vertex{Vector2f(bounds.position), Color::White},
        Vertex{Vector2f(bounds.position.x + bounds.size.x, bounds.position.y), Color::White},
        Vertex{Vector2f(bounds.position + bounds.size), Color::White},
        Vertex{Vector2f(bounds.position.x, bounds.position.y + bounds.size.y), Color::White},
    };
    switch (currentTool)
    {
    case Tool::BoxSelect: case Tool::CircleSelect: case Tool::MagicWand:
        target.draw(v.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendAlpha,
            {StencilComparison::Equal, StencilUpdateOperation::Keep, 0x4, 0x4, false},
            Transform::Identity, CoordinateType::Pixels, nullptr, nullptr));
        break;
    default:
        break;
    }
    if (drawSelectionLines)
    {
        target.draw(animCenter, RenderStates(BlendAlpha,
            {StencilComparison::Equal, StencilUpdateOperation::Keep, 0x4, 0x4, false},
            Transform::Identity, CoordinateType::Pixels, nullptr, nullptr));
    }

    target.draw(v2.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendAlpha,
        {StencilComparison::Less, StencilUpdateOperation::Keep, 0x00, 0x03, false},
        Transform::Identity, CoordinateType::Pixels, nullptr, nullptr));

    target.draw(animOutline, RenderStates(BlendAlpha,
        {StencilComparison::Less, StencilUpdateOperation::Keep, 0x00, 0x03, false},
        Transform::Identity, CoordinateType::Pixels, nullptr, nullptr));

    //render subtractive select box
    if (!selectShape.second && hasStartedBoxSelect())
    {
        target.clearStencil(0x00);
#ifdef GL_ALPHA_TEST
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.5f);
#endif
        const std::array v = {
            Vertex{Vector2f(selectShape.first.position.x, selectShape.first.position.y), Color::White},
            Vertex{Vector2f(selectShape.first.position.x + selectShape.first.size.x, selectShape.first.position.y), Color::White},
            Vertex{Vector2f(selectShape.first.position.x + selectShape.first.size.x, selectShape.first.position.y + selectShape.first.size.y), Color::White},
            Vertex{Vector2f(selectShape.first.position.x, selectShape.first.position.y + selectShape.first.size.y), Color::White},
        };
        Transformable t;
        const std::array offsets = {Vector2f(-offset, -offset), Vector2f(offset, -offset), Vector2f(offset, offset), Vector2f(-offset, offset)};
        for (int32_t j = 0; j < offsets.size(); j++)
        {
            t.setPosition(offsets.at(j));
            target.draw(v.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendNone,
                {StencilComparison::Always, StencilUpdateOperation::Increment, StencilValue(0x01), 0xFF, true},
                t.getTransform(), CoordinateType::Normalized, nullptr, nullptr));
        }
#ifdef GL_ALPHA_TEST
        glDisable(GL_ALPHA_TEST);
#endif
        target.draw(v2.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendAlpha,
            {StencilComparison::Less, StencilUpdateOperation::Keep, 0x00, 0x03, false},
            Transform::Identity, CoordinateType::Pixels, nullptr, nullptr));

        target.draw(animOutline, RenderStates(BlendAlpha,
            {StencilComparison::Less, StencilUpdateOperation::Keep, 0x00, 0x03, false},
            Transform::Identity, CoordinateType::Pixels, nullptr, nullptr));
    }
}
