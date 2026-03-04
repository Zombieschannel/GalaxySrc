#include "ImageEditorWorker.hpp"
#include "../Const.hpp"
#include "../Func.hpp"
#include "../ZEditorsCommon/ZTB.hpp"
#include <set>

namespace stb
{
    #define STB_IMAGE_RESIZE_IMPLEMENTATION
    #include <stb_image_resize2.h>
}

glxy::ImageEditorWorker::ImageEditorWorker()
{
}

void glxy::ImageEditorWorker::Empty(const Vector2u resolution, const Color color)
{
    lock_guard lock(chunkManager.mtxChunkManager);
    AllocateChunks(resolution, color);
}

bool glxy::ImageEditorWorker::Open(const filesystem::path& target)
{
    Image image;
    if (image.loadFromFile(target))
    {
        lock_guard lock(chunkManager.mtxChunkManager);
        AllocateChunks(image.getSize(), Color::Transparent);
        chunkManager.lockAllChunks();
        chunkManager.PasteImage(image, Vector2u(), IntRect(), ImageLayerType::Color);
        chunkManager.unlockAllChunks();
        return true;
    }
    return false;
}

void glxy::ImageEditorWorker::ForEachChunkInChunkArea(const IntRect& area, const std::function<void(Vector2i)>& func) const
{
    chunkManager.ForEachChunkInChunkArea(area, func);
}

void glxy::ImageEditorWorker::ForEachPixelInChunkArea(const IntRect& area, const bool lockChunks, const std::function<void(Vector2i)>& func) const
{
    for (int32_t i = area.position.x / chunkManager.getChunkSize();
        i < ceil(static_cast<float>(area.position.x + area.size.x) / chunkManager.getChunkSize()); i++)
        for (int32_t j = area.position.y / chunkManager.getChunkSize();
            j < ceil(static_cast<float>(area.position.y + area.size.y) / chunkManager.getChunkSize()); j++)
        {
            if (lockChunks)
                chunkManager.getChunkMutex(Vector2u(i, j)).lock();
            const std::optional<IntRect> intersection = area.findIntersection(
                IntRect(Vector2i(i * chunkManager.getChunkSize(), j * chunkManager.getChunkSize()),
                    Vector2i(chunkManager.getChunkSize(Vector2u(i, j)))));
            const IntRect rect = *intersection;
            for (int32_t x = rect.position.x; x < rect.position.x + rect.size.x; x++)
                for (int32_t y = rect.position.y; y < rect.position.y + rect.size.y; y++)
                {
                    func(Vector2i(x, y));
                }
            if (lockChunks)
                chunkManager.getChunkMutex(Vector2u(i, j)).unlock();
        }
}

Vector2u glxy::ImageEditorWorker::getSize() const
{
    return chunkManager.getSize();
}

void glxy::ImageEditorWorker::BeginSelect(const Vector2f pos, const SelectMode selectMode, const ShapeSelectType type, const bool keepAspect)
{
    lock_guard lock(chunkManager.mtxChunkManager);
    chunkManager.lockAllChunks();
    if (!chunkManager.hasStartedBoxSelect())
    {
        if (selectMode == SelectMode::Single)
            chunkManager.resetSelection();
        chunkManager.boxShapeStart(Vector2u(pos), selectMode != SelectMode::Subtractive, type);
        pixelSelectStart = pos;
    }
    else
    {
        const Vector2i pixelSelectStartInt = Vector2i(pixelSelectStart);
        if (pos.x - pixelSelectStart.x != 0.f && pos.y - pixelSelectStart.y != 0.f)
        {
            if (keepAspect)
            {
                Vector2f vec = pos - pixelSelectStart;
                float limitSize = 0.f;
                if (fabs(pos.x - pixelSelectStart.x) > fabs(pos.y - pixelSelectStart.y))
                {
                    vec.y = fabs(pos.x - pixelSelectStart.x) * (pos.y - pixelSelectStart.y) / fabs(pos.y - pixelSelectStart.y);
                    limitSize = vec.y > 0.f ? getSize().y - pixelSelectStartInt.y - 1 : pixelSelectStartInt.y;
                }
                else
                {
                    vec.x = fabs(pos.y - pixelSelectStart.y) * (pos.x - pixelSelectStart.x) / fabs(pos.x - pixelSelectStart.x);
                    limitSize = vec.x > 0.f ? getSize().x - pixelSelectStartInt.x - 1 : pixelSelectStartInt.x;
                }
                vec.x = std::clamp(vec.x, -limitSize, limitSize);
                vec.y = std::clamp(vec.y, -limitSize, limitSize);
                chunkManager.boxShapeEnd(Vector2u(vec.x + pixelSelectStartInt.x, vec.y + pixelSelectStartInt.y));
            }
            else
                chunkManager.boxShapeEnd(Vector2u(pos));
        }
    }
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::EndSelect(const Vector2f pos)
{
    lock_guard lock(chunkManager.mtxChunkManager);
    if (!chunkManager.hasStartedBoxSelect())
        return;
    chunkManager.lockAllChunks();
    if (Distance::Point_Point(pixelSelectStart, pos) < 0.2f)
        chunkManager.resetSelection();
    else
        chunkManager.boxShapeFinish();
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::SetupMovePixels()
{
    assert(!chunkManager.getFinalSelectionBounds().expired());
    const IntRect bounds = *chunkManager.getFinalSelectionBounds().lock();
    if (!colorBufferTemp)
        colorBufferTemp = make_unique<Image>();
    colorBufferTemp->resize(Vector2u(bounds.size), Color::Transparent);
    if (!selectionBufferTemp)
        selectionBufferTemp = make_unique<Image>();
    selectionBufferTemp->resize(Vector2u(bounds.size), Color::Transparent);

    const std::optional<IntRect> intersection = bounds.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize())));
    if (intersection)
    {
        chunkManager.lockAllChunks();
        Vector2u dummy;
        chunkManager.CopySelectedColorPixels(*colorBufferTemp, dummy, workingLayer);
        ForEachPixelInChunkArea(*chunkManager.getFinalSelectionBounds().lock(), false, [&](const Vector2i coord)
        {
            if (chunkManager.getPixelSelection(Vector2u(coord)))
                chunkManager.setPixelColor(Vector2u(coord), Color::Transparent, workingLayer);
        });
        chunkManager.CopyImage(*selectionBufferTemp, Vector2u(intersection->position - bounds.position), *intersection, ImageLayerType::Selection);
        chunkManager.unlockAllChunks();
    }
}

void glxy::ImageEditorWorker::MovePixels(const Transform& transform)
{
    if (!colorBufferTemp || !selectionBufferTemp)
        return;
    {
        lock_guard lock(chunkManager.mtxChunkManager);
        chunkManager.lockAllChunks();
        if (!chunkManager.hasColorTempLayer(0))
            chunkManager.createColorTempLayerAll(getBlendMode(BlendAlpha));
        else
            chunkManager.clearColorTempLayerAll(Color::Transparent);
        chunkManager.clearSelectionLayer(false);
        chunkManager.unlockAllChunks();
    }
    const IntRect rect = TransformImage(transform, false);

    lock_guard lock(mtxEditorWorkerCommon);
    newMoveSelectionArea = rect;
}

void glxy::ImageEditorWorker::CancelMovePixels(const IntRect& area)
{
    const std::optional<IntRect> intersection = area.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize())));
    if (intersection)
    {
        chunkManager.lockAllChunks();
        if (chunkManager.hasSelectionLayer(0))
        {
            chunkManager.clearSelectionLayer(false);
            chunkManager.PasteImage(*selectionBufferTemp, Vector2u(intersection->position), IntRect(intersection->position - area.position, intersection->size), ImageLayerType::Selection);
            chunkManager.PasteSelectedColorPixels(*colorBufferTemp, workingLayer);
        }
        else
            chunkManager.PasteImage(*colorBufferTemp, Vector2u(), IntRect(), ImageLayerType::Color, workingLayer);
        chunkManager.unlockAllChunks();
    }
    chunkManager.lockAllChunks();
    chunkManager.deleteColorTempLayer();
    chunkManager.unlockAllChunks();

    selectionBufferTemp.reset();
    colorBufferTemp.reset();
}

void glxy::ImageEditorWorker::FinishMovePixels(const IntRect& area)
{
    chunkManager.lockAllChunks();
    for (int32_t x = area.position.x; x < area.position.x + area.size.x; x++)
        for (int32_t y = area.position.y; y < area.position.y + area.size.y; y++)
        {
            if (x < 0 || y < 0 || x >= getSize().x || y >= getSize().y)
                continue;
            if (chunkManager.hasSelectionLayer(0) && !chunkManager.getPixelSelection(Vector2u(x, y)))
                continue;
            chunkManager.setPixelColor(Vector2u(x, y), chunkManager.getPixelColorTemp(Vector2u(x, y)), workingLayer);
        }
    if (chunkManager.hasSelectionLayer(0))
        chunkManager.setNewSelectionBounds(area);
    chunkManager.deleteColorTempLayer();
    chunkManager.unlockAllChunks();

    selectionBufferTemp.reset();
    colorBufferTemp.reset();
}

IntRect glxy::ImageEditorWorker::TransformImage(const Transform& transformable, const bool tile)
{
    const FloatRect area = FloatRect(Vector2f(), Vector2f(colorBufferTemp->getSize()));
    FloatRect transformedArea;
    if (!tile)
        transformedArea = transformable.transformRect(area);
    else
        transformedArea = FloatRect({}, Vector2f(colorBufferTemp->getSize()));
    const Transform inverse = transformable.getInverse();

    chunkManager.lockAllChunks();
    const bool selectAll = !chunkManager.hasSelectionLayer(0) && !chunkManager.hasSelectionTempLayer(0);

    for (int32_t x = max(roundf(transformedArea.position.x), 0.f);
        x < min(roundf(transformedArea.position.x + transformedArea.size.x), static_cast<float>(getSize().x)); x++)
        for (int32_t y = max(roundf(transformedArea.position.y), 0.f);
            y < min(roundf(transformedArea.position.y + transformedArea.size.y), static_cast<float>(getSize().y)); y++)
        {
            if (x < 0 || y < 0 || x >= getSize().x || y >= getSize().y)
                continue;
            const Vector2f pos = inverse.transformPoint(Vector2f(x + 0.5f, y + 0.5f));
            if (!selectAll && (pos.x < 0 || pos.y < 0 || pos.x >= colorBufferTemp->getSize().x || pos.y >= colorBufferTemp->getSize().y || selectionBufferTemp->getPixel(Vector2u(pos)) == Color::Transparent))
                continue;
            Color col = Color::Transparent;
            if (tile && selectAll)
            {
                const Vector2f tex = Vector2f(pos.x / colorBufferTemp->getSize().x, pos.y / colorBufferTemp->getSize().y);
                const Vector2u coords = Vector2u((tex.x - floor(tex.x)) * colorBufferTemp->getSize().x, (tex.y - floor(tex.y)) * colorBufferTemp->getSize().y);
                col = colorBufferTemp->getPixel(coords);
            }
            else if (!(pos.x < 0 || pos.y < 0 || pos.x >= colorBufferTemp->getSize().x || pos.y >= colorBufferTemp->getSize().y))
                col = colorBufferTemp->getPixel(Vector2u(pos.x, pos.y));
            chunkManager.setPixelColorTemp(Vector2u(x, y), col);
            if (!selectAll)
                chunkManager.setPixelSelection(Vector2u(x, y), true);
        }
    chunkManager.unlockAllChunks();
    return IntRect(Vector2i(round(transformedArea.position.x), round(transformedArea.position.y)), Vector2i(round(transformedArea.size.x), round(transformedArea.size.y)));
}

void glxy::ImageEditorWorker::InterpolatePixelLine(const Vector2f start, const Vector2f end, vector<Vector2i>& out)
{
    const Vector2f diff = Vector2f(end.x - start.x, end.y - start.y);
    out.clear();
    if (diff.x == 0 && diff.y == 0)
        out.push_back(Vector2i(end.x, end.y));
    else if (fabs(diff.x) > fabs(diff.y))
    {
        const float offsetY = static_cast<float>(diff.y) / fabs(diff.x);
        for (int32_t i = 0; i < fabs(diff.x); i++)
            out.push_back(Vector2i(start.x + i * (diff.x / fabs(diff.x)), start.y + offsetY * i));
    }
    else
    {
        const float offsetX = static_cast<float>(diff.x) / fabs(diff.y);
        for (int32_t i = 0; i < fabs(diff.y); i++)
            out.push_back(Vector2i(start.x + offsetX * i, start.y + i * (diff.y / fabs(diff.y))));
    }
}

void glxy::ImageEditorWorker::DrawPixels(const Vector2f pos, const Vector2f prev, const Color color, const LayerID layerID)
{
    const FloatRect canvasBB = FloatRect({0.f, 0.f}, Vector2f(getSize().x, getSize().y));

    if (!canvasBB.findIntersection(FloatRect(Vector2f(pos.x - 0.5f, pos.y - 0.5f), Vector2f(2, 2))) &&
        !canvasBB.findIntersection(FloatRect(Vector2f(prev.x - 0.5f, prev.y - 0.5f), Vector2f(2, 2))))
        return;

    vector<Vector2i> targetPixels;
    InterpolatePixelLine(prev, pos, targetPixels);
    lock_guard lock(chunkManager.mtxChunkManager);
    chunkManager.lockAllChunks();
    for (const auto& n : targetPixels)
    {
        if (!canvasBB.contains(Vector2f(n.x + 0.5f, n.y + 0.5f)))
            continue;
        if (chunkManager.getPixelColor(Vector2u(n), layerID) != color && (!chunkManager.hasSelectionLayer(0) || chunkManager.getPixelSelection(Vector2u(n))))
            chunkManager.setPixelColor(Vector2u(n), color, layerID);
    }
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::BrushPixels(const Vector2f start, const Vector2f end, const float radius, const Color color, const LayerID layerID, const bool eraser)
{
    const float left = min(start.x, end.x);
    const float top = min(start.y, end.y);
    const float right = max(start.x, end.x);
    const float bottom = max(start.y, end.y);
    const FloatRect rect = FloatRect(Vector2f(left, top), Vector2f(right - left, bottom - top));
    const IntRect rectShape = IntRect(Vector2i(rect.position.x - radius, rect.position.y - radius),
        Vector2i(rect.size.x + radius * 2, rect.size.y + radius * 2));
    const std::optional<IntRect> intersect = rectShape.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize())));
    if (!intersect)
        return;

    ForEachChunkInChunkArea(*intersect, [&](const Vector2i chunkID)
    {
        RenderWorker::AddWork(RenderWork::BrushDraw{start, end, radius, color, layerID, chunkManager.getChunkID(Vector2u(chunkID)), eraser});
    });
    ForEachChunkInChunkArea(*intersect, [&](const Vector2i chunkID)
    {
        const unique_ptr<RenderResult> result = RenderWorker::getResult();
        const auto v = result->get<RenderResult::Chunk>();
        if (!v) return;
        chunkManager.getChunkMutex(Vector2u(chunkID)).lock();

        if (!chunkManager.hasColorTempLayer(chunkManager.getChunkID(Vector2u(chunkID))))
        {
            chunkManager.createColorTempLayer(getBlendMode(eraser ? BlendMultiply : BlendAlpha), chunkManager.getChunkID(Vector2u(chunkID)));
            if (eraser)
                chunkManager.clearColorTempLayer(Color::White, chunkManager.getChunkID(Vector2u(chunkID)));
        }

        chunkManager.PasteImage(v->img, Vector2u(chunkID.x * chunkManager.getChunkSize(), chunkID.y * chunkManager.getChunkSize()),
            IntRect(), ImageLayerType::ColorTemp, layerID);
        chunkManager.getChunkMutex(Vector2u(chunkID)).unlock();
        RenderWorker::RemoveResult();
    });
}

bool glxy::ImageEditorWorker::FillPixels(const Vector2i pos, const Color color, const int8_t tolerance, const LayerID layerID)
{
    if (pos.x < 0 || pos.y < 0 || pos.x >= getSize().x || pos.y >= getSize().y)
        return false;
    lock_guard lock(chunkManager.mtxChunkManager);
    chunkManager.lockAllChunks();
    chunkManager.createColorTempLayerAll(getBlendMode(BlendAlpha));
    chunkManager.clearColorTempLayerAll(Color::Transparent);
    chunkManager.FloodFill(ImageLayerType::ColorTemp, pos, color, tolerance, chunkManager.hasSelectionLayer(0), layerID);
    chunkManager.unlockAllChunks();
    return true;
}

void glxy::ImageEditorWorker::GradientPixels(const Vector2f startPos, const Vector2f endPos, const Color startColor, const Color endColor)
{
    chunkManager.mtxChunkManager.lock();
    chunkManager.createColorTempLayerAll(getBlendMode(BlendAlpha));
    IntRect area;
    if (!chunkManager.hasSelectionLayer(0))
        area = IntRect({}, Vector2i(getSize()));
    else
    {
        assert(!chunkManager.getFinalSelectionBounds().expired());
        const IntRect final = *chunkManager.getFinalSelectionBounds().lock();
        area = final;
    }

    if (startPos == endPos)
    {
        chunkManager.mtxChunkManager.unlock();
        ForEachPixelInChunkArea(area, true, [&](const Vector2i pos)
        {
            if (chunkManager.hasSelectionLayer(0) && !chunkManager.getPixelSelection(Vector2u(pos)))
                return;
            chunkManager.setPixelColorTemp(Vector2u(pos), startColor);
        });
        return;
    }
    const float dist = Distance::Point_Point(startPos, endPos);
    chunkManager.mtxChunkManager.unlock();

    ForEachPixelInChunkArea(area, true, [&](const Vector2i pos)
    {
        if (chunkManager.hasSelectionLayer(0) && !chunkManager.getPixelSelection(Vector2u(pos.x, pos.y)))
            return;
        const Vector2f s = Vector2f(pos.x + 0.5f - startPos.x, pos.y + 0.5f - startPos.y);
        const Vector2f e = Vector2f(pos.x + 0.5f - endPos.x, pos.y + 0.5f - endPos.y);
        if (s.x * s.x + s.y * s.y - e.x * e.x - e.y * e.y > dist * dist)
            chunkManager.setPixelColorTemp(Vector2u(pos.x, pos.y), endColor);
        else if (s.x * s.x + s.y * s.y - e.x * e.x - e.y * e.y < -dist * dist)
            chunkManager.setPixelColorTemp(Vector2u(pos.x, pos.y), startColor);
        else
        {
            const Color c = LerpColor(startColor, endColor, (s.x * s.x + s.y * s.y - e.x * e.x - e.y * e.y) / (dist * dist) / 2 + 0.5f);
            chunkManager.setPixelColorTemp(Vector2u(pos.x, pos.y), c);
        }
    });
}

void glxy::ImageEditorWorker::ResizeCanvas(const Vector2u size, const Pivot pivot)
{
    const Vector2i canvasResizePoint = Vector2i(static_cast<int32_t>(pivot) % 3, static_cast<int32_t>(pivot) / 3);
    const Vector2i imageSize = static_cast<Vector2i>(getSize());
    const Vector2i position = -Vector2i((static_cast<int32_t>(size.x) - imageSize.x) * canvasResizePoint.x * 0.5f,
        (static_cast<int32_t>(size.y) - imageSize.y) * canvasResizePoint.y * 0.5f);
    const IntRect newSize = IntRect(position, Vector2i(size.x, size.y));

    vector<Image> layers;
    const Vector2u oldSize = getSize();
    const int16_t layerCount = chunkManager.getLayerCount();
    for (LayerID i = 0; i < layerCount; i++)
    {
        layers.emplace_back();
        layers.back().resize(getSize(), Color::Transparent);
        chunkManager.CopyImage(layers.back(), Vector2u(), IntRect(), ImageLayerType::Color, i);
    }
    lock_guard lock(chunkManager.mtxChunkManager);
    AllocateChunks(Vector2u(newSize.size), Color::Transparent);
    chunkManager.lockAllChunks();
    for (LayerID i = 0; i < layerCount; i++)
    {
        chunkManager.PasteImage(layers.at(i), Vector2u(-min(newSize.position.x, 0), -min(newSize.position.y, 0)),
            IntRect(Vector2i(max(newSize.position.x, 0), max(newSize.position.y, 0)),
                Vector2i(min(newSize.size.x, static_cast<int32_t>(oldSize.x)),
                    min(newSize.size.y, static_cast<int32_t>(oldSize.y)))), ImageLayerType::Color, i);
    }
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::RescaleCanvas(const Vector2u newSize, RescaleMethod method)
{
    const array filters = {
        stb::STBIR_FILTER_TRIANGLE, stb::STBIR_FILTER_BOX, stb::STBIR_FILTER_CATMULLROM, stb::STBIR_FILTER_MITCHELL, stb::STBIR_FILTER_CUBICBSPLINE, stb::STBIR_FILTER_POINT_SAMPLE
    };
    vector<Image> layers;
    const int16_t layerCount = chunkManager.getLayerCount();
    for (LayerID i = 0; i < layerCount; i++)
    {
        layers.emplace_back();
        layers.back().resize(getSize(), Color::Transparent);
        chunkManager.CopyImage(layers.back(), Vector2u(), IntRect(), ImageLayerType::Color, i);
    }
    lock_guard lock(chunkManager.mtxChunkManager);
    AllocateChunks(Vector2u(newSize), Color::Transparent);
    chunkManager.lockAllChunks();
    for (LayerID i = 0; i < layerCount; i++)
    {
        Image t;
        t.resize(Vector2u(newSize.x, newSize.y));
        stb::stbir_resize(layers.at(i).getPixelsPtr(), layers.at(i).getSize().x, layers.at(i).getSize().y, 0,
            const_cast<unsigned char*>(t.getPixelsPtr()), newSize.x, newSize.y, 0,
            stb::STBIR_RGBA, stb::STBIR_TYPE_UINT8_SRGB, stb::STBIR_EDGE_CLAMP, filters.at(static_cast<int8_t>(method)));
        chunkManager.PasteImage(t, Vector2u(), IntRect(), ImageLayerType::Color, i);
    }
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::Adjustment(const Adjustments adjustment, const LayerID layerID, const AdjustmentData* data, const ChunkID chunkID)
{
    const Vector2i chunkXY = Vector2i(chunkID % chunkManager.getChunkCount().x, chunkID / chunkManager.getChunkCount().x);
    IntRect rect = IntRect(chunkXY * static_cast<int32_t>(chunkManager.getChunkSize()), Vector2i(chunkManager.getChunkSize(chunkID)));
    {
        if (chunkManager.hasSelectionLayer(0))
        {
            assert(!chunkManager.getFinalSelectionBounds().expired());
            const std::optional<IntRect> intersect = chunkManager.getFinalSelectionBounds().lock()->findIntersection(rect);
            if (intersect)
                rect = *intersect;
        }

        lock_guard lock(chunkManager.mtxChunkManager);
        chunkManager.createColorTempLayerAll(getBlendMode(BlendAlpha));
    }
    chunkManager.Adjust(adjustment, rect, layerID, data);
}

void glxy::ImageEditorWorker::Effect(const Effects effect, const LayerID layerID, const EffectData* data, const ChunkID chunkID)
{
    const Vector2i chunkXY = Vector2i(chunkID % chunkManager.getChunkCount().x, chunkID / chunkManager.getChunkCount().x);
    IntRect rect = IntRect(chunkXY * static_cast<int32_t>(chunkManager.getChunkSize()), Vector2i(chunkManager.getChunkSize(chunkID)));
    {
        if (chunkManager.hasSelectionLayer(0))
        {
            assert(!chunkManager.getFinalSelectionBounds().expired());
            const std::optional<IntRect> intersect = chunkManager.getFinalSelectionBounds().lock()->findIntersection(rect);
            if (intersect)
                rect = *intersect;
        }

        lock_guard lock(chunkManager.mtxChunkManager);
        chunkManager.createColorTempLayerAll(getBlendMode(BlendAlpha));
    }
    chunkManager.Effect(effect, rect, layerID, data);
}

void glxy::ImageEditorWorker::CancelAdjustmentOrEffect()
{
    lock_guard lock(chunkManager.mtxChunkManager);
    chunkManager.deleteColorTempLayer();
}

void glxy::ImageEditorWorker::MergeColorTempLayer(const LayerID layerID, const BlendMode& blendMode)
{
    lock_guard lock(chunkManager.mtxChunkManager);
    chunkManager.MergeColorTempLayer(layerID, blendMode);
    chunkManager.deleteColorTempLayer();
}

void glxy::ImageEditorWorker::AllocateChunks(const Vector2u size, const Color color)
{
    if (chunkManager.hasSelectionLayer(0))
        chunkManager.resetSelection();
    chunkManager.AllocateChunks(size, color);
}

uint32_t glxy::ImageEditorWorker::getChunkIDFromPixel(const Vector2u pixel) const
{
    lock_guard lock(chunkManager.mtxChunkManager);
    return pixel.x / chunkManager.getChunkSize() + pixel.y / chunkManager.getChunkSize() * ceil(static_cast<float>(getSize().x) / chunkManager.getChunkSize());
}