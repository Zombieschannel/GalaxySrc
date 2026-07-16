#include "ImageEditorWorker.hpp"
#include "../Const.hpp"
#include "../Func.hpp"
#include "../ZEditorsCommon/ZTB.hpp"
#include <set>
#include <SFML/OpenGL.hpp>

namespace stb
{
    #define STB_IMAGE_RESIZE_IMPLEMENTATION
    #include <stb_image_resize2.h>
}

glxy::ImageEditorWorker::ImageEditorWorker(const bool infinite)
    : chunkManager(infinite)
{
}

void glxy::ImageEditorWorker::Empty(const Vector2u resolution, const Color color)
{
    lock_guard lock(chunkManager.mtxChunkManager);
    chunkManager.setBackgroundColor(color);
    AllocateChunks(resolution);
}

bool glxy::ImageEditorWorker::Open(const filesystem::path& target)
{
    Image image;
    if (image.loadFromFile(target))
    {
        lock_guard lock(chunkManager.mtxChunkManager);
        AllocateChunks(image.getSize());
        chunkManager.lockAllChunks();
        chunkManager.PasteImage(image, Vector2i(), IntRect(), ImageLayerType::Color);
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
        i < std::ceil(static_cast<float>(area.position.x + area.size.x) / chunkManager.getChunkSize()); i++)
        for (int32_t j = area.position.y / chunkManager.getChunkSize();
            j < std::ceil(static_cast<float>(area.position.y + area.size.y) / chunkManager.getChunkSize()); j++)
        {
            if (lockChunks)
                chunkManager.getChunkMutex(Vector2i(i, j)).lock();
            const std::optional<IntRect> intersection = area.findIntersection(
                IntRect(Vector2i(i * chunkManager.getChunkSize(), j * chunkManager.getChunkSize()),
                    Vector2i(chunkManager.getChunkSize(Vector2i(i, j)))));
            const IntRect rect = *intersection;
            for (int32_t x = rect.position.x; x < rect.position.x + rect.size.x; x++)
                for (int32_t y = rect.position.y; y < rect.position.y + rect.size.y; y++)
                    func(Vector2i(x, y));
            if (lockChunks)
                chunkManager.getChunkMutex(Vector2i(i, j)).unlock();
        }
}

Vector2u glxy::ImageEditorWorker::getSize() const
{
    return chunkManager.getSize();
}

void glxy::ImageEditorWorker::BeginSelect(const Vector2f pos, const SelectMode selectMode, const ShapeSelectType type, const bool keepAspect)
{
    const Vector2f clamped = Vector2f(std::clamp(pos.x, 0.f, static_cast<float>(getSize().x - 1)),
                    std::clamp(pos.y, 0.f, static_cast<float>(getSize().y - 1)));
    lock_guard lock(chunkManager.mtxChunkManager);
    chunkManager.lockAllChunks();
    if (!chunkManager.hasStartedSelect())
    {
        if (selectMode == SelectMode::Single)
        {
            newMoveSelectArea = IntRect();
            chunkManager.resetSelection();
        }
        switch (type)
        {
        case ShapeSelectType::Box: case ShapeSelectType::Circle:
            chunkManager.boxSelectStart(Vector2u(clamped), selectMode != SelectMode::Subtractive, type);
            break;
        case ShapeSelectType::Lasso:
            chunkManager.lassoSelectStart(clamped, selectMode != SelectMode::Subtractive);
            break;
        }
        pixelSelectStart = pos;
    }
    else
    {
        if (pos.x - pixelSelectStart.x != 0.f && pos.y - pixelSelectStart.y != 0.f)
        {
            if (keepAspect && (type == ShapeSelectType::Box || type == ShapeSelectType::Circle))
            {
                const Vector2f clampedStart = Vector2f(std::clamp(pixelSelectStart.x, 0.f, static_cast<float>(getSize().x - 1)),
                    std::clamp(pixelSelectStart.y, 0.f, static_cast<float>(getSize().y - 1)));
                const Vector2i pixelSelectStartInt = Vector2i(clampedStart);
                Vector2i vec = Vector2i(clamped) - pixelSelectStartInt;
                if (vec.x == 0)
                    vec.x = 1;
                if (vec.y == 0)
                    vec.y = 1;
                int32_t limitSize = 0;
                if (std::abs(vec.x) > std::abs(vec.y))
                {
                    vec.y = std::abs(vec.x) * vec.y / std::abs(vec.y);
                    limitSize = vec.y > 0.f ? getSize().y - pixelSelectStartInt.y - 1 : pixelSelectStartInt.y;
                }
                else
                {
                    vec.x = std::abs(vec.y) * vec.x / std::abs(vec.x);
                    limitSize = vec.x > 0.f ? getSize().x - pixelSelectStartInt.x - 1 : pixelSelectStartInt.x;
                }
                vec.x = std::clamp(vec.x, -limitSize, limitSize);
                vec.y = std::clamp(vec.y, -limitSize, limitSize);
                chunkManager.boxSelectEnd(Vector2u(vec.x + pixelSelectStartInt.x, vec.y + pixelSelectStartInt.y));
            }
            else
            {
                switch (type)
                {
                case ShapeSelectType::Box: case ShapeSelectType::Circle:
                    chunkManager.boxSelectEnd(Vector2u(clamped));
                    break;
                case ShapeSelectType::Lasso:
                    chunkManager.lassoSelectEnd(clamped);
                    break;
                }
            }
        }
    }
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::EndSelect(const Vector2f pos)
{
    lock_guard lock(chunkManager.mtxChunkManager);
    if (!chunkManager.hasStartedSelect())
        return;
    chunkManager.lockAllChunks();
    if (Distance::Point_Point(pixelSelectStart, pos) < 0.5f)
    {
        newMoveSelectArea = IntRect();
        chunkManager.resetSelection();
    }
    else
        chunkManager.selectFinish();
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::SetupMovePixels()
{
    assert(!chunkManager.getFinalSelectionBounds().expired());
    const IntRect bounds = *chunkManager.getFinalSelectionBounds().lock();
    if (!transformImageCache.colorBufferTemp)
        transformImageCache.colorBufferTemp = make_unique<Image>();
    transformImageCache.colorBufferTemp->resize(Vector2u(bounds.size), Color::Transparent);
    if (!transformImageCache.selectionBufferTemp)
        transformImageCache.selectionBufferTemp = make_unique<Image>();
    transformImageCache.selectionBufferTemp->resize(Vector2u(bounds.size), Color::Transparent);

    const std::optional<IntRect> intersection = bounds.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize())));
    if (intersection)
    {
        chunkManager.lockAllChunks();
        Vector2u dummy;
        chunkManager.CopySelectedColorPixels(*transformImageCache.colorBufferTemp, dummy, workingLayer);
        transformImageCache.requiresUpdateColor = true;

        ForEachPixelInChunkArea(*chunkManager.getFinalSelectionBounds().lock(), false, [&](const Vector2i coord)
        {
            if (chunkManager.getPixelSelection(coord))
                chunkManager.setPixelColor(coord, Color::Transparent, workingLayer);
        });
        chunkManager.CopyImage(*transformImageCache.selectionBufferTemp, Vector2i(intersection->position - bounds.position), *intersection, ImageLayerType::Selection);
        transformImageCache.requiresUpdateSelection = true;
        chunkManager.unlockAllChunks();
    }
}

void glxy::ImageEditorWorker::MovePixels(const Transform& transform, const bool smooth)
{
    if (!transformImageCache.colorBufferTemp || !transformImageCache.selectionBufferTemp)
        return;
    const IntRect rect = TransformImage(transform, 1, smooth, false);

    lock_guard lock(mtxEditorWorkerCommon);
    if (newMoveSelectArea != IntRect())
    {
        if (const std::optional<IntRect> area = newMoveSelectArea.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize()))))
        {
            ForEachChunkInChunkArea(*area, [&](const ChunkID chunkID)
            {
                if (IntRect(Vector2i(chunkID.x * chunkManager.getChunkSize(), chunkID.y * chunkManager.getChunkSize()),
                    Vector2i(chunkManager.getChunkSize(chunkID))).findIntersection(rect))
                    return;

                chunkManager.getChunkMutex(chunkID).lock();
                if (chunkManager.hasSelectionLayer(chunkID))
                    chunkManager.clearSelectionLayer(false, chunkID);
                if (chunkManager.hasColorTempLayer(chunkID))
                    chunkManager.clearColorTempLayer(Color::Transparent, chunkID);
                chunkManager.getChunkMutex(chunkID).unlock();
            });
        }
    }
    newMoveSelectArea = rect;
}

void glxy::ImageEditorWorker::CancelMovePixels(const IntRect& area)
{
    const std::optional<IntRect> intersection = area.findIntersection(
        IntRect(Vector2i(0, 0), Vector2i(getSize())));
    if (intersection)
    {
        chunkManager.lockAllChunks();
        if (!chunkManager.getFinalSelectionBounds().expired())
        {
            chunkManager.ForEachChunkID([&](const ChunkID chunkID)
            {
                if (!chunkManager.hasSelectionLayer(chunkID))
                    chunkManager.createSelectionLayer(chunkID);
                else
                    chunkManager.clearSelectionLayer(false, chunkID);
            });
            chunkManager.PasteImage(*transformImageCache.selectionBufferTemp, Vector2i(intersection->position), IntRect(intersection->position - area.position, intersection->size), ImageLayerType::Selection);
            chunkManager.PasteSelectedColorPixels(*transformImageCache.colorBufferTemp, workingLayer);
        }
        else
            chunkManager.PasteImage(*transformImageCache.colorBufferTemp, Vector2i(), IntRect(), ImageLayerType::Color, workingLayer);
        chunkManager.unlockAllChunks();
    }
    chunkManager.lockAllChunks();
    chunkManager.deleteColorTempLayerAll();
    chunkManager.unlockAllChunks();

    transformImageCache.selectionBufferTemp.reset();
    transformImageCache.colorBufferTemp.reset();
}

void glxy::ImageEditorWorker::FinishMovePixels(const IntRect& area)
{
    chunkManager.lockAllChunks();
    for (int32_t x = area.position.x; x < area.position.x + area.size.x; x++)
        for (int32_t y = area.position.y; y < area.position.y + area.size.y; y++)
        {
            if (x < 0 || y < 0 || x >= getSize().x || y >= getSize().y)
                continue;
            if ((newMoveSelectArea != IntRect() || !chunkManager.getFinalSelectionBounds().expired()) && !chunkManager.getPixelSelection(Vector2i(x, y)))
                continue;
            chunkManager.setPixelColor(Vector2i(x, y), chunkManager.getPixelColorTemp(Vector2i(x, y)), workingLayer);
        }
    if (!chunkManager.getFinalSelectionBounds().expired() || newMoveSelectArea != IntRect())
        chunkManager.setNewSelectionBounds(area);
    chunkManager.deleteColorTempLayerAll();
    chunkManager.unlockAllChunks();

    transformImageCache.selectionBufferTemp.reset();
    transformImageCache.colorBufferTemp.reset();
}

void glxy::ImageEditorWorker::SetupMoveSelection()
{
    assert(!chunkManager.getFinalSelectionBounds().expired());
    const IntRect bounds = *chunkManager.getFinalSelectionBounds().lock();
    if (!transformImageCache.selectionBufferTemp)
        transformImageCache.selectionBufferTemp = make_unique<Image>();
    transformImageCache.selectionBufferTemp->resize(Vector2u(bounds.size), Color::Transparent);

    const std::optional<IntRect> intersection = bounds.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize())));
    if (intersection)
    {
        chunkManager.lockAllChunks();
        chunkManager.CopyImage(*transformImageCache.selectionBufferTemp, Vector2i(intersection->position - bounds.position), *intersection, ImageLayerType::Selection);
        transformImageCache.requiresUpdateSelection = true;
        chunkManager.unlockAllChunks();
    }
}

void glxy::ImageEditorWorker::MoveSelection(const Transform& transform)
{
    if (!transformImageCache.selectionBufferTemp)
        return;
    const IntRect rect = TransformImage(transform, 1, false, true);

    lock_guard lock(mtxEditorWorkerCommon);
    if (newMoveSelectArea != IntRect())
    {
        if (const std::optional<IntRect> area = newMoveSelectArea.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize()))))
        {
            ForEachChunkInChunkArea(*area, [&](const ChunkID chunkID)
            {
                if (IntRect(Vector2i(chunkID.x * chunkManager.getChunkSize(), chunkID.y * chunkManager.getChunkSize()),
                    Vector2i(chunkManager.getChunkSize(chunkID))).findIntersection(rect))
                    return;

                chunkManager.getChunkMutex(chunkID).lock();
                if (chunkManager.hasSelectionLayer(chunkID))
                    chunkManager.clearSelectionLayer(false, chunkID);
                chunkManager.getChunkMutex(chunkID).unlock();
            });
        }
    }
    newMoveSelectArea = rect;
}

void glxy::ImageEditorWorker::CancelMoveSelection(const IntRect& area)
{
    const std::optional<IntRect> intersection = area.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize())));
    if (intersection)
    {
        chunkManager.lockAllChunks();
        chunkManager.ForEachChunkID([&](const ChunkID chunkID)
        {
            if (!chunkManager.hasSelectionLayer(chunkID))
                chunkManager.createSelectionLayer(chunkID);
            else
                chunkManager.clearSelectionLayer(false, chunkID);
        });
        chunkManager.PasteImage(*transformImageCache.selectionBufferTemp, Vector2i(intersection->position), IntRect(intersection->position - area.position, intersection->size), ImageLayerType::Selection);
        chunkManager.unlockAllChunks();
    }

    transformImageCache.selectionBufferTemp.reset();
}

void glxy::ImageEditorWorker::FinishMoveSelection(const IntRect& area)
{
    chunkManager.lockAllChunks();
    chunkManager.setNewSelectionBounds(area);
    chunkManager.unlockAllChunks();

    transformImageCache.selectionBufferTemp.reset();
    transformImageCache.requiresUpdateSelection = true;
}

IntRect glxy::ImageEditorWorker::TransformImage(const Transform& transform, const int16_t repeat, const bool smooth, const bool selectionOnly)
{
    const Vector2u sourceSize = selectionOnly ? transformImageCache.selectionBufferTemp->getSize() : transformImageCache.colorBufferTemp->getSize();
    const FloatRect area = FloatRect(Vector2f(), Vector2f(sourceSize.x, sourceSize.y));
    Transform temp = transform;
    const FloatRect transformedArea = temp.scale(Vector2f(repeat, repeat), Vector2f(sourceSize) / 2.f).transformRect(area);

    const IntRect chunkArea = IntRect(Vector2i(
        std::max(std::round(transformedArea.position.x), 0.f),
        std::max(std::round(transformedArea.position.y), 0.f)),
        Vector2i(std::min(std::round(transformedArea.size.x), getSize().x - std::max(std::round(transformedArea.position.x), 0.f)),
            std::min(std::round(transformedArea.size.y), getSize().y - std::max(std::round(transformedArea.position.y), 0.f))));

    constexpr int8_t c_cornerOffset = 4;
    constexpr uint16_t c_bufferChunkSize = c_maxChunkWorkableSize - c_cornerOffset;

    if (transformImageCache.colorBufferTemp)
    {
        if (transformImageCache.requiresUpdateColor)
        {
            const Vector2u range = Vector2u(std::ceil(static_cast<float>(transformImageCache.colorBufferTemp->getSize().x) / (c_bufferChunkSize)),
                std::ceil(static_cast<float>(transformImageCache.colorBufferTemp->getSize().y) / c_bufferChunkSize));
            transformImageCache.colorBufferChunks.clear();
            for (uint16_t x = 0; x < range.x; x++)
                for (uint16_t y = 0; y < range.y; y++)
                {
                    transformImageCache.colorBufferChunks.emplace_back();
                    transformImageCache.colorBufferChunks.back().first.position = Vector2i(x * c_bufferChunkSize, y * c_bufferChunkSize);
                    transformImageCache.colorBufferChunks.back().first.size = Vector2i(c_bufferChunkSize, c_bufferChunkSize);
                    if (x == range.x - 1)
                        transformImageCache.colorBufferChunks.back().first.size.x = sourceSize.x % c_bufferChunkSize;
                    if (y == range.y - 1)
                        transformImageCache.colorBufferChunks.back().first.size.y = sourceSize.y % c_bufferChunkSize;

                    const IntRect area = IntRect(
                        transformImageCache.colorBufferChunks.back().first.position - Vector2i(c_cornerOffset / 2, c_cornerOffset / 2),
                        transformImageCache.colorBufferChunks.back().first.size + Vector2i(c_cornerOffset, c_cornerOffset));

                    validate(transformImageCache.colorBufferChunks.back().second.loadFromImage(*transformImageCache.colorBufferTemp, false,
                        *area.findIntersection(IntRect(Vector2i(0, 0), Vector2i(sourceSize)))));
                }
            transformImageCache.requiresUpdateColor = false;
        }
        for (auto& n : transformImageCache.colorBufferChunks)
        {
            n.second.setRepeated(repeat > 1);
            n.second.setSmooth(smooth);
        }
    }
    if (transformImageCache.selectionBufferTemp && transformImageCache.requiresUpdateSelection)
    {
        const Vector2u range = Vector2u(std::ceil(static_cast<float>(transformImageCache.selectionBufferTemp->getSize().x) / c_bufferChunkSize),
            std::ceil(static_cast<float>(transformImageCache.selectionBufferTemp->getSize().y) / c_bufferChunkSize));
        transformImageCache.selectionBufferChunks.clear();
        for (uint16_t x = 0; x < range.x; x++)
            for (uint16_t y = 0; y < range.y; y++)
            {
                transformImageCache.selectionBufferChunks.emplace_back();
                transformImageCache.selectionBufferChunks.back().first.position = Vector2i(x * c_bufferChunkSize, y * c_bufferChunkSize);
                transformImageCache.selectionBufferChunks.back().first.size = Vector2i(c_bufferChunkSize, c_bufferChunkSize);
                if (x == range.x - 1)
                    transformImageCache.selectionBufferChunks.back().first.size.x = sourceSize.x % c_bufferChunkSize;
                if (y == range.y - 1)
                    transformImageCache.selectionBufferChunks.back().first.size.y = sourceSize.y % c_bufferChunkSize;

                const IntRect area = IntRect(
                    transformImageCache.selectionBufferChunks.back().first.position - Vector2i(c_cornerOffset / 2, c_cornerOffset / 2),
                    transformImageCache.selectionBufferChunks.back().first.size + Vector2i(c_cornerOffset, c_cornerOffset));

                validate(transformImageCache.selectionBufferChunks.back().second.loadFromImage(*transformImageCache.selectionBufferTemp, false,
                    *area.findIntersection(IntRect(Vector2i(0, 0), Vector2i(sourceSize)))));
            }
        transformImageCache.requiresUpdateSelection = false;
    }

    RenderTexture renderTextureColor;
    RenderTexture renderTextureSelect;
    ForEachChunkInChunkArea(chunkArea, [&](const ChunkID chunkID)
    {
        const Vector2u chunkSize = chunkManager.getChunkSize(chunkID);
        const uint16_t chunkGeneralSize = chunkManager.getChunkSize();

        if (renderTextureColor.getSize() != chunkSize)
            validate(renderTextureColor.resize(chunkSize, {0U, 0U, 0U}));
        renderTextureColor.setView(View(FloatRect(Vector2f(chunkID.x * chunkGeneralSize, chunkID.y * chunkGeneralSize),
            Vector2f(chunkSize))));

        const auto renderAllChunks = [&](RenderTexture& target, const vector<pair<IntRect, Texture>>& chunks, RenderStates& states)
        {
            for (const auto& n : chunks)
            {
                if (!FloatRect(Vector2f(chunkID.x * chunkGeneralSize, chunkID.y * chunkGeneralSize), Vector2f(chunkSize)).findIntersection(
                    transform.transformRect(FloatRect(n.first))))
                {
                    continue;
                }

                const std::optional<IntRect> areaToDraw = IntRect(n.first.position - Vector2i(c_cornerOffset / 2, c_cornerOffset / 2), n.first.size + Vector2i(c_cornerOffset, c_cornerOffset)).
                    findIntersection(IntRect(Vector2i(0, 0), Vector2i(sourceSize)));

                if (!areaToDraw)
                {
                    continue;
                }

                states.texture = &n.second;
                const array quad = {
                    Vertex{Vector2f(areaToDraw->position), Color::White, Vector2f(0, 0)},
                    Vertex{Vector2f(areaToDraw->position) + Vector2f(areaToDraw->size.x, 0), Color::White, Vector2f(repeat, 0)},
                    Vertex{Vector2f(areaToDraw->position) + Vector2f(areaToDraw->size), Color::White, Vector2f(repeat, repeat)},
                    Vertex{Vector2f(areaToDraw->position) + Vector2f(0, areaToDraw->size.y), Color::White, Vector2f(0, repeat)},
                };
                target.draw(quad.data(), 4, PrimitiveType::TriangleFan, states);
            }
        };

        if (!selectionOnly && transformImageCache.colorBufferTemp)
        {
            renderTextureColor.clear(Color::Transparent);

            RenderStates states;
            states.blendMode = BlendAlpha;
            states.transform = transform;
            states.coordinateType = CoordinateType::Normalized;
            states.transform.scale(Vector2f(repeat, repeat), Vector2f(sourceSize) / 2.f);

            renderAllChunks(renderTextureColor, transformImageCache.colorBufferChunks, states);
            renderTextureColor.display();
        }

        if (renderTextureSelect.getSize() != chunkSize)
            validate(renderTextureSelect.resize(chunkSize, {0U, 0U, 0U}));

        renderTextureSelect.setView(View(FloatRect(Vector2f(chunkID.x * chunkGeneralSize, chunkID.y * chunkGeneralSize), Vector2f(chunkSize))));

        if (transformImageCache.selectionBufferTemp)
        {
            renderTextureSelect.clear(Color::Transparent);

            RenderStates states;
            states.blendMode = BlendAlpha;
            states.transform = transform;
            states.coordinateType = CoordinateType::Normalized;

            renderAllChunks(renderTextureSelect, transformImageCache.selectionBufferChunks, states);
            renderTextureSelect.display();
        }

        chunkManager.getChunkMutex(chunkID).lock();
        if (!selectionOnly && transformImageCache.colorBufferTemp)
        {
            if (!chunkManager.hasColorTempLayer(chunkID))
                chunkManager.createColorTempLayer(getBlendMode(BlendAlpha), chunkID);

            chunkManager.PasteImage(renderTextureColor.getTexture().copyToImage(), Vector2i(chunkID.x * chunkManager.getChunkSize(), chunkID.y * chunkManager.getChunkSize()),
                IntRect(), ImageLayerType::ColorTemp);
        }
        if (transformImageCache.selectionBufferTemp)
        {
            if (!chunkManager.hasSelectionLayer(chunkID))
                chunkManager.createSelectionLayer(chunkID);

            chunkManager.PasteImage(renderTextureSelect.getTexture().copyToImage(), Vector2i(chunkID.x * chunkManager.getChunkSize(), chunkID.y * chunkManager.getChunkSize()),
                IntRect(), ImageLayerType::Selection);

        }
        chunkManager.getChunkMutex(chunkID).unlock();
    });
    return IntRect(Vector2i(std::round(transformedArea.position.x), std::round(transformedArea.position.y)),
        Vector2i(std::round(transformedArea.size.x), std::round(transformedArea.size.y)));
}

void glxy::ImageEditorWorker::SetupMaskedRendering(RenderTexture& renderTexture, const ChunkID chunkID) const
{
    const Vector2u chunkSize = chunkManager.getChunkSize(chunkID);

    renderTexture.clearStencil(0x00);

    if (chunkManager.hasSelectionLayer(chunkID))
    {
#ifdef GL_ALPHA_TEST
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.5f);
#endif
        RenderLayerToTexture(chunkManager.getChunkImageSelection(chunkID), chunkSize, 255, RenderStates(StencilMode{
                StencilComparison::Always, StencilUpdateOperation::Increment, 0x01, 0xFF, true}), renderTexture);
#ifdef GL_ALPHA_TEST
        glDisable(GL_ALPHA_TEST);
#endif
    }
}

void glxy::ImageEditorWorker::InterpolatePixelLine(const Vector2f start, const Vector2f end, vector<Vector2i>& out)
{
    const Vector2f diff = Vector2f(end.x - start.x, end.y - start.y);
    out.clear();
    if (diff.x == 0 && diff.y == 0)
        out.push_back(Vector2i(std::floor(end.x), std::floor(end.y)));
    else if (std::abs(diff.x) > std::abs(diff.y))
    {
        const float offsetY = static_cast<float>(diff.y) / std::abs(diff.x);
        for (int32_t i = 0; i < std::abs(diff.x); i++)
            out.push_back(Vector2i(std::floor(start.x + i * (diff.x / std::abs(diff.x))), std::floor(start.y + offsetY * i)));
    }
    else
    {
        const float offsetX = static_cast<float>(diff.x) / fabs(diff.y);
        for (int32_t i = 0; i < std::abs(diff.y); i++)
            out.push_back(Vector2i(std::floor(start.x + offsetX * i), std::floor(start.y + i * (diff.y / std::abs(diff.y)))));
    }
}

void glxy::ImageEditorWorker::ChunksInBrushLine(const Vector2f start, const Vector2f end, const float radius, vector<ChunkID>& out) const
{
    const float left = std::min(start.x, end.x);
    const float top = std::min(start.y, end.y);
    const float right = std::max(start.x, end.x);
    const float bottom = std::max(start.y, end.y);

    std::set<pair<int32_t, int32_t>> chunksToCheck;
    const float distance = Distance::Point_Point(start, end);
    Vector2f startPos = start;
    const Vector2f dir = end - start;
    if (distance == 0)
    {
        for (int32_t x = std::floor((left - radius) / chunkManager.getChunkSize());
            x <= std::floor((right + radius) / chunkManager.getChunkSize()); x++)
        {
            if (!chunkManager.isInfinite() && (x < 0 || x >= chunkManager.getChunkCount().x))
                continue;
            for (int32_t y = std::floor((top - radius) / chunkManager.getChunkSize());
                y <= std::floor((bottom + radius) / chunkManager.getChunkSize()); y++)
            {
                if (!chunkManager.isInfinite() && (y < 0 || y >= chunkManager.getChunkCount().y))
                    continue;

                if (chunkManager.chunkExists(ChunkID(x, y)) && !chunkManager.hasSelectionLayer(Vector2i(x, y)) &&
                    (!chunkManager.getFinalSelectionBounds().expired() || newMoveSelectArea != IntRect()))
                    continue;

                chunksToCheck.emplace(x, y);
            }
        }
    }
    else
    {
        for (int32_t i = 0; i <= distance / chunkManager.getChunkSize() * 2; i++)
        {
            const Vector2f nextStart = (i == static_cast<int32_t>(distance / chunkManager.getChunkSize() * 2)) ?
                end : startPos + dir.normalized() * (chunkManager.getChunkSize() / 2.f);
            const float newLeft = std::min(startPos.x, nextStart.x) - radius;
            const float newTop = std::min(startPos.y, nextStart.y) - radius;
            const float newRight = std::max(startPos.x, nextStart.x) + radius;
            const float newBottom = std::max(startPos.y, nextStart.y) + radius;

            for (int32_t x = std::floor(newLeft / chunkManager.getChunkSize()); x <= std::floor(newRight / chunkManager.getChunkSize()); x++)
            {
                if (!chunkManager.isInfinite() && (x < 0 || x >= chunkManager.getChunkCount().x))
                    continue;
                for (int32_t y = std::floor(newTop / chunkManager.getChunkSize()); y <= std::floor(newBottom / chunkManager.getChunkSize()); y++)
                {
                    if (!chunkManager.isInfinite() && (y < 0 || y >= chunkManager.getChunkCount().y))
                        continue;

                    if (chunkManager.chunkExists(ChunkID(x, y)) && !chunkManager.hasSelectionLayer(Vector2i(x, y)) &&
                        (!chunkManager.getFinalSelectionBounds().expired() || newMoveSelectArea != IntRect()))
                        continue;

                    chunksToCheck.emplace(x, y);
                }
            }
            startPos = nextStart;
        }
    }
    for (const auto& n : chunksToCheck)
        out.emplace_back(n.first, n.second);
}

void glxy::ImageEditorWorker::PencilPixels(const Vector2f pos, const Vector2f prev, const Color color)
{
    const FloatRect canvasBB = FloatRect({0.f, 0.f}, Vector2f(getSize().x, getSize().y));

    if (!chunkManager.isInfinite() && !canvasBB.findIntersection(FloatRect(Vector2f(pos.x - 0.5f, pos.y - 0.5f), Vector2f(2, 2))) &&
        !canvasBB.findIntersection(FloatRect(Vector2f(prev.x - 0.5f, prev.y - 0.5f), Vector2f(2, 2))))
        return;

    std::set<pair<int32_t, int32_t>> chunksToLock;
    vector<Vector2i> targetPixels;
    InterpolatePixelLine(prev, pos, targetPixels);

    if (!chunkManager.isInfinite())
        for (int32_t i = 0; i < targetPixels.size(); i++)
        {
            const ChunkID chunkID = ChunkID(std::floor(static_cast<float>(targetPixels.at(i).x) / chunkManager.getChunkSize()),
                std::floor(static_cast<float>(targetPixels.at(i).y) / chunkManager.getChunkSize()));

            if (targetPixels.at(i).x >= 0 && targetPixels.at(i).x < getSize().x && targetPixels.at(i).y >= 0 && targetPixels.at(i).y < getSize().y &&
                (chunkManager.hasSelectionLayer(chunkID) || (chunkManager.getFinalSelectionBounds().expired() && newMoveSelectArea == IntRect())))
                continue;

            targetPixels.erase(targetPixels.begin() + i);
            i--;
        }

    for (const Vector2i& n : targetPixels)
    {
        const ChunkID chunkID = ChunkID(std::floor(static_cast<float>(n.x) / chunkManager.getChunkSize()),
            std::floor(static_cast<float>(n.y) / chunkManager.getChunkSize()));
        chunksToLock.emplace(chunkID.x, chunkID.y);
    }

    if (chunksToLock.empty())
        return;

    for (const pair<int32_t, int32_t>& n : chunksToLock)
    {
        if (chunkManager.isInfinite())
        {
            if (chunkManager.chunkExists(ChunkID(n.first, n.second)))
            {
                if (!chunkManager.hasColorTempLayer(ChunkID(n.first, n.second)))
                    chunkManager.createColorTempLayer(getBlendMode(BlendAlpha), ChunkID(n.first, n.second));
                continue;
            }
            chunkManager.AllocateChunkInfinite(ChunkID(n.first, n.second));
            chunkManager.createColorTempLayer(getBlendMode(BlendAlpha), ChunkID(n.first, n.second));
        }
        else
        {
            if (!chunkManager.hasColorTempLayer(ChunkID(n.first, n.second)))
                chunkManager.createColorTempLayer(getBlendMode(BlendAlpha), ChunkID(n.first, n.second));
        }
    }


    lock_guard lock(chunkManager.mtxChunkManager);
    chunkManager.lockAllChunks();
    for (const auto& n : targetPixels)
    {
        if (!chunkManager.isInfinite() && !canvasBB.contains(Vector2f(n.x + 0.5f, n.y + 0.5f)))
            continue;
        if (chunkManager.getFinalSelectionBounds().expired() || chunkManager.getPixelSelection(n))
            chunkManager.setPixelColorTemp(n, color);
    }
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::BrushPixels(const Vector2f start, const Vector2f end, const float radius, const Color color, const bool eraser)
{
    const float left = std::min(start.x, end.x);
    const float top = std::min(start.y, end.y);
    const float right = std::max(start.x, end.x);
    const float bottom = std::max(start.y, end.y);
    const FloatRect rect = FloatRect(Vector2f(left, top), Vector2f(right - left, bottom - top));
    const IntRect rectShape = IntRect(Vector2i(rect.position.x - radius, rect.position.y - radius),
        Vector2i(rect.size.x + radius * 2, rect.size.y + radius * 2));
    const std::optional<IntRect> intersect = rectShape.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize())));
    if (!chunkManager.isInfinite() && !intersect)
        return;

    vector<ChunkID> chunksToCheck;
    ChunksInBrushLine(start, end, radius, chunksToCheck);

    if (chunkManager.isInfinite())
    {
        for (const ChunkID n : chunksToCheck)
        {
            if (chunkManager.chunkExists(n))
                continue;
            chunkManager.AllocateChunkInfinite(n);
        }
    }

    for (const ChunkID chunkID : chunksToCheck)
    {
        const Vector2u chunkSize = chunkManager.getChunkSize(chunkID);
        const uint16_t chunkGeneralSize = chunkManager.getChunkSize();

        RenderTexture renderTexture;

        validate(renderTexture.resize(chunkSize, {0U, 8U, 0U}));

        renderTexture.clear(eraser ? Color::White : Color::Transparent);

#ifdef GL_ALPHA_TEST
        renderTexture.resetGLStates(); // workaround for mixing SFML with OpenGL
#endif

        if (chunkManager.hasColorTempLayer(chunkID))
            RenderLayerToTexture(chunkManager.getChunkImageColorTemp(chunkID), chunkSize, 255, BlendNone, renderTexture);

        SetupMaskedRendering(renderTexture, chunkID);

        renderTexture.setView(View(FloatRect(Vector2f(chunkID.x * chunkGeneralSize, chunkID.y * chunkGeneralSize),
            Vector2f(chunkSize))));

        CircleShape shape;
        shape.setFillColor(color);

        RenderStates states;
        states.blendMode = BlendNone;
        states.stencilMode = {chunkManager.hasSelectionLayer(chunkID) ? StencilComparison::Equal : StencilComparison::Always,
            StencilUpdateOperation::Keep, 0x01, 0xFF, false};

        shape.setPointCount(std::max(2.f * c_PI * sqrtf(radius), 30.f));
        shape.setRadius(radius);
        shape.setOrigin(Vector2f(radius, radius));

        const Vector2f startPos = Vector2f(Vector2i(std::floor(start.x), std::floor(start.y))) + Vector2f(0.5f, 0.5f);
        const Vector2f endPos = Vector2f(Vector2i(std::floor(end.x), std::floor(end.y))) + Vector2f(0.5f, 0.5f);

        shape.setPosition(startPos);
        renderTexture.draw(shape, states);

        shape.setPosition(endPos);
        renderTexture.draw(shape, states);

        if (startPos != endPos)
        {
            const Vector2f diff = endPos - startPos;
            const Vector2f perp = diff.perpendicular().normalized();
            const array connect = {
                Vertex{Vector2f(startPos + perp * radius), color},
                Vertex{Vector2f(startPos + -perp * radius), color},
                Vertex{Vector2f(endPos + -perp * radius), color},
                Vertex{Vector2f(endPos + perp * radius), color}
            };
            renderTexture.draw(connect.data(), 4, PrimitiveType::TriangleFan, states);
        }
        renderTexture.display();

        chunkManager.getChunkMutex(chunkID).lock();

        if (!chunkManager.hasColorTempLayer(chunkID))
        {
            chunkManager.createColorTempLayer(getBlendMode(eraser ? BlendMultiply : BlendAlpha), chunkID);
            if (eraser)
                chunkManager.clearColorTempLayer(Color::White, chunkID);
        }

        chunkManager.PasteImage(renderTexture.getTexture().copyToImage(), Vector2i(chunkID.x * chunkManager.getChunkSize(), chunkID.y * chunkManager.getChunkSize()),
                                IntRect(), ImageLayerType::ColorTemp);
        chunkManager.getChunkMutex(chunkID).unlock();
    }
}

void glxy::ImageEditorWorker::ColorSwapPixels(const Vector2f start, const Vector2f end, const float radius, const Color color1, const Color color2, const int8_t tol, const LayerID layerID)
{
    const float left = std::min(start.x, end.x);
    const float top = std::min(start.y, end.y);
    const float right = std::max(start.x, end.x);
    const float bottom = std::max(start.y, end.y);
    const FloatRect rect = FloatRect(Vector2f(left, top), Vector2f(right - left, bottom - top));
    const IntRect rectShape = IntRect(Vector2i(rect.position.x - radius, rect.position.y - radius),
        Vector2i(rect.size.x + radius * 2, rect.size.y + radius * 2));
    const std::optional<IntRect> intersect = rectShape.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize())));

    if (!intersect)
        return;

    vector<ChunkID> chunksToCheck;
    ChunksInBrushLine(start, end, radius, chunksToCheck);

    for (const ChunkID chunkID : chunksToCheck)
    {
        const Vector2u chunkSize = chunkManager.getChunkSize(chunkID);
        const uint16_t chunkGeneralSize = chunkManager.getChunkSize();

        RenderTexture renderTexture;

        validate(renderTexture.resize(chunkSize, {0U, 8U, 0U}));

        renderTexture.clear(Color::Transparent);

#ifdef GL_ALPHA_TEST
        renderTexture.resetGLStates(); // workaround for mixing SFML with OpenGL
#endif

        SetupMaskedRendering(renderTexture, chunkID);

        renderTexture.setView(View(FloatRect(Vector2f(chunkID.x * chunkGeneralSize, chunkID.y * chunkGeneralSize),
            Vector2f(chunkSize))));

        CircleShape shape;
        shape.setFillColor(Color::Black);

        RenderStates states;
        states.blendMode = BlendNone;
        states.stencilMode = {chunkManager.hasSelectionLayer(chunkID) ? StencilComparison::Equal : StencilComparison::Always,
            StencilUpdateOperation::Keep, 0x01, 0xFF, false};

        shape.setPointCount(std::max(2.f * c_PI * sqrtf(radius), 30.f));
        shape.setRadius(radius);
        shape.setOrigin(Vector2f(radius, radius));

        const Vector2f startPos = Vector2f(Vector2i(std::floor(start.x), std::floor(start.y))) + Vector2f(0.5f, 0.5f);
        const Vector2f endPos = Vector2f(Vector2i(std::floor(end.x), std::floor(end.y))) + Vector2f(0.5f, 0.5f);

        shape.setPosition(startPos);
        renderTexture.draw(shape, states);

        shape.setPosition(endPos);
        renderTexture.draw(shape, states);

        if (startPos != endPos)
        {
            const Vector2f diff = endPos - startPos;
            const Vector2f perp = diff.perpendicular().normalized();
            const array connect = {
                Vertex{Vector2f(startPos + perp * radius), Color::Black},
                Vertex{Vector2f(startPos + -perp * radius), Color::Black},
                Vertex{Vector2f(endPos + -perp * radius), Color::Black},
                Vertex{Vector2f(endPos + perp * radius), Color::Black}
            };
            renderTexture.draw(connect.data(), 4, PrimitiveType::TriangleFan, states);
        }
        renderTexture.display();

        const Image modifyMask = renderTexture.getTexture().copyToImage();
        Image toModify(modifyMask.getSize());
        chunkManager.CopyImage(toModify, Vector2i(), IntRect(Vector2i(chunkID.x * chunkManager.getChunkSize(), chunkID.y * chunkManager.getChunkSize()),
            Vector2i(chunkManager.getChunkSize(chunkID))), ImageLayerType::Color, layerID);

        chunkManager.getChunkMutex(chunkID).lock();

        for (int32_t x = 0; x < toModify.getSize().x; x++)
            for (int32_t y = 0; y < toModify.getSize().y; y++)
            {
                if (modifyMask.getPixel(Vector2u(x, y)).a < 128)
                    continue;
                const Color readColor = toModify.getPixel(Vector2u(x, y));
                if (!SameColor(readColor, color1, tol))
                    continue;
                toModify.setPixel(Vector2u(x, y), color2);
            }

        chunkManager.PasteImage(toModify, Vector2i(chunkID.x * chunkManager.getChunkSize(), chunkID.y * chunkManager.getChunkSize()),
                                IntRect(), ImageLayerType::Color, layerID);

        chunkManager.getChunkMutex(chunkID).unlock();
    }
}

bool glxy::ImageEditorWorker::FillPixels(const Vector2i pos, const Color color, const int8_t tolerance, const LayerID layerID)
{
    if (pos.x < 0 || pos.y < 0 || pos.x >= getSize().x || pos.y >= getSize().y)
        return false;
    lock_guard lock(chunkManager.mtxChunkManager);
    chunkManager.lockAllChunks();
    if (!chunkManager.allHaveColorTempLayer())
        chunkManager.createColorTempLayerAll(getBlendMode(BlendAlpha));
    chunkManager.clearColorTempLayerAll(Color::Transparent);
    chunkManager.FloodFill(ImageLayerType::ColorTemp, pos, color, tolerance, !chunkManager.getFinalSelectionBounds().expired(), layerID);
    chunkManager.unlockAllChunks();
    return true;
}

void glxy::ImageEditorWorker::GradientPixels(const Vector2f startPos, const Vector2f endPos, const Color startColor, const Color endColor)
{
    chunkManager.mtxChunkManager.lock();
    chunkManager.createColorTempLayerAll(getBlendMode(BlendAlpha));
    chunkManager.mtxChunkManager.unlock();

    IntRect area;
    if (chunkManager.getFinalSelectionBounds().expired())
        area = IntRect({}, Vector2i(getSize()));
    else
    {
        const IntRect final = *chunkManager.getFinalSelectionBounds().lock();
        area = final;
    }

    ForEachChunkInChunkArea(area, [&](const ChunkID chunkID)
    {
        const Vector2u chunkSize = chunkManager.getChunkSize(chunkID);
        const uint16_t chunkGeneralSize = chunkManager.getChunkSize();

        RenderTexture renderTexture;

        validate(renderTexture.resize(chunkSize, {0U, 8U, 0U}));

        renderTexture.clear(Color::Transparent);

#ifdef GL_ALPHA_TEST
        renderTexture.resetGLStates(); // workaround for mixing SFML with OpenGL
#endif

        SetupMaskedRendering(renderTexture, chunkID);

        renderTexture.setView(View(FloatRect(Vector2f(chunkID.x * chunkGeneralSize, chunkID.y * chunkGeneralSize),
            Vector2f(chunkSize))));

        RenderStates states;
        states.blendMode = BlendAlpha;
        states.stencilMode = {chunkManager.hasSelectionLayer(chunkID) ? StencilComparison::Equal : StencilComparison::Always,
            StencilUpdateOperation::Keep, 0x01, 0xFF, false};

        if (startPos == endPos)
        {
            const array quad = {
                Vertex{Vector2f(chunkID.x * chunkGeneralSize, chunkID.y * chunkGeneralSize), startColor},
                Vertex{Vector2f(chunkID.x * chunkGeneralSize + chunkSize.x, chunkID.y * chunkGeneralSize), startColor},
                Vertex{Vector2f(chunkID.x * chunkGeneralSize + chunkSize.x, chunkID.y * chunkGeneralSize + chunkSize.y), startColor},
                Vertex{Vector2f(chunkID.x * chunkGeneralSize, chunkID.y * chunkGeneralSize + chunkSize.y), startColor}
            };
            renderTexture.draw(quad.data(), 4, PrimitiveType::TriangleFan, states);
        }
        else
        {
            const Vector2f dir = (endPos - startPos).normalized();
            const Vector2f perp = dir.perpendicular();
            const array quadCenter = {
                Vertex{Vector2f(startPos + perp * 1e6f), startColor},
                Vertex{Vector2f(startPos - perp * 1e6f), startColor},
                Vertex{Vector2f(endPos - perp * 1e6f), endColor},
                Vertex{Vector2f(endPos + perp * 1e6f), endColor},
            };
            const array quadStart = {
                Vertex{Vector2f(startPos - perp * 1e6f), startColor},
                Vertex{Vector2f(startPos + perp * 1e6f), startColor},
                Vertex{Vector2f(startPos + perp * 1e6f - dir * 1e6f), startColor},
                Vertex{Vector2f(startPos - perp * 1e6f - dir * 1e6f), startColor},
            };
            const array quadEnd = {
                Vertex{Vector2f(endPos - perp * 1e6f), endColor},
                Vertex{Vector2f(endPos + perp * 1e6f), endColor},
                Vertex{Vector2f(endPos + perp * 1e6f + dir * 1e6f), endColor},
                Vertex{Vector2f(endPos - perp * 1e6f + dir * 1e6f), endColor},
            };
            renderTexture.draw(quadStart.data(), 4, PrimitiveType::TriangleFan, states);
            renderTexture.draw(quadEnd.data(), 4, PrimitiveType::TriangleFan, states);
            renderTexture.draw(quadCenter.data(), 4, PrimitiveType::TriangleFan, states);
        }
        renderTexture.display();

        chunkManager.getChunkMutex(chunkID).lock();

        if (!chunkManager.hasColorTempLayer(chunkID))
            chunkManager.createColorTempLayer(getBlendMode(BlendAlpha), chunkID);

        chunkManager.PasteImage(renderTexture.getTexture().copyToImage(), Vector2i(chunkID.x * chunkManager.getChunkSize(), chunkID.y * chunkManager.getChunkSize()),
            IntRect(), ImageLayerType::ColorTemp);

        chunkManager.getChunkMutex(chunkID).unlock();
    });
}

void glxy::ImageEditorWorker::ShapePixels(const shared_ptr<ConvexShape>& shape)
{
    IntRect area = IntRect(Vector2i(shape->getGlobalBounds().position), Vector2i(shape->getGlobalBounds().size));
    if (prevShapeRenderArea != IntRect())
        area = getUnion(&prevShapeRenderArea, &area);

    const std::optional<IntRect> intersect = area.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize())));

    if (!intersect)
        return;

    prevShapeRenderArea = IntRect(Vector2i(shape->getGlobalBounds().position), Vector2i(shape->getGlobalBounds().size));

    ForEachChunkInChunkArea(*intersect, [&](const ChunkID chunkID)
    {
        if (!chunkManager.hasSelectionLayer(chunkID) && (!chunkManager.getFinalSelectionBounds().expired() || newMoveSelectArea != IntRect()))
            return;

        const Vector2u chunkSize = chunkManager.getChunkSize(chunkID);
        const uint16_t chunkGeneralSize = chunkManager.getChunkSize();

        RenderTexture renderTexture;

        validate(renderTexture.resize(chunkSize, {0U, 8U, 0U}));

        if (shape->getOutlineThickness() > 0)
            renderTexture.clear(Color(shape->getOutlineColor().r, shape->getOutlineColor().g, shape->getOutlineColor().b, 0));
        else
            renderTexture.clear(Color(shape->getFillColor().r, shape->getFillColor().g, shape->getFillColor().b, 0));

#ifdef GL_ALPHA_TEST
        renderTexture.resetGLStates(); // workaround for mixing SFML with OpenGL
#endif

        SetupMaskedRendering(renderTexture, chunkID);

        renderTexture.setView(View(FloatRect(Vector2f(chunkID.x * chunkGeneralSize, chunkID.y * chunkGeneralSize),
            Vector2f(chunkSize))));

        RenderStates states;
        states.blendMode = BlendAlpha;
        states.stencilMode = {chunkManager.hasSelectionLayer(chunkID) ? StencilComparison::Equal : StencilComparison::Always,
            StencilUpdateOperation::Keep, 0x01, 0xFF, false};

        renderTexture.draw(*shape, states);
        renderTexture.display();

        chunkManager.getChunkMutex(chunkID).lock();

        if (!chunkManager.hasColorTempLayer(chunkID))
            chunkManager.createColorTempLayer(getBlendMode(BlendAlpha), chunkID);

        chunkManager.PasteImage(renderTexture.getTexture().copyToImage(), Vector2i(chunkID.x * chunkManager.getChunkSize(), chunkID.y * chunkManager.getChunkSize()),
                IntRect(), ImageLayerType::ColorTemp);

        chunkManager.getChunkMutex(chunkID).unlock();
    });
}

void glxy::ImageEditorWorker::TextPixels(const shared_ptr<Text>& text, const FloatRect& globalBounds)
{
    IntRect area = IntRect(Vector2i(globalBounds.position), Vector2i(globalBounds.size));
    if (prevTextRenderArea != IntRect())
        area = getUnion(&prevTextRenderArea, &area);

    const std::optional<IntRect> intersect = area.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize())));

    if (!intersect)
        return;

    prevTextRenderArea = IntRect(Vector2i(globalBounds.position), Vector2i(globalBounds.size));

    ForEachChunkInChunkArea(*intersect, [&](const ChunkID chunkID)
    {
        if (!chunkManager.hasSelectionLayer(chunkID) && (!chunkManager.getFinalSelectionBounds().expired() || newMoveSelectArea != IntRect()))
            return;

        const Vector2u chunkSize = chunkManager.getChunkSize(chunkID);
        const uint16_t chunkGeneralSize = chunkManager.getChunkSize();

        RenderTexture renderTexture;

        validate(renderTexture.resize(chunkSize, {0U, 8U, 0U}));

        if (text->getOutlineThickness() > 0)
            renderTexture.clear(Color(text->getOutlineColor().r, text->getOutlineColor().g, text->getOutlineColor().b, 0));
        else
            renderTexture.clear(Color(text->getFillColor().r, text->getFillColor().g, text->getFillColor().b, 0));

#ifdef GL_ALPHA_TEST
        renderTexture.resetGLStates(); // workaround for mixing SFML with OpenGL
#endif

        SetupMaskedRendering(renderTexture, chunkID);

        renderTexture.setView(View(FloatRect(Vector2f(chunkID.x * chunkGeneralSize, chunkID.y * chunkGeneralSize),
            Vector2f(chunkSize))));

        RenderStates states;
        states.blendMode = BlendAlpha;
        states.stencilMode = {chunkManager.hasSelectionLayer(chunkID) ? StencilComparison::Equal : StencilComparison::Always,
            StencilUpdateOperation::Keep, 0x01, 0xFF, false};

        renderTexture.draw(*text, states);
        renderTexture.display();

        chunkManager.getChunkMutex(chunkID).lock();

        if (!chunkManager.hasColorTempLayer(chunkID))
            chunkManager.createColorTempLayer(getBlendMode(BlendAlpha), chunkID);

        chunkManager.PasteImage(renderTexture.getTexture().copyToImage(), Vector2i(chunkID.x * chunkManager.getChunkSize(), chunkID.y * chunkManager.getChunkSize()),
                IntRect(), ImageLayerType::ColorTemp);

        chunkManager.getChunkMutex(chunkID).unlock();
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
        chunkManager.CopyImage(layers.back(), Vector2i(), IntRect(), ImageLayerType::Color, i);
    }
    lock_guard lock(chunkManager.mtxChunkManager);
    AllocateChunks(Vector2u(newSize.size));
    chunkManager.lockAllChunks();
    for (LayerID i = 0; i < layerCount; i++)
    {
        chunkManager.PasteImage(layers.at(i), Vector2i(-std::min(newSize.position.x, 0), -std::min(newSize.position.y, 0)),
                                IntRect(Vector2i(std::max(newSize.position.x, 0), std::max(newSize.position.y, 0)),
                                        Vector2i(std::min(newSize.size.x, static_cast<int32_t>(oldSize.x)),
                                                 std::min(newSize.size.y, static_cast<int32_t>(oldSize.y)))), ImageLayerType::Color, i);
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
        chunkManager.CopyImage(layers.back(), Vector2i(), IntRect(), ImageLayerType::Color, i);
    }
    lock_guard lock(chunkManager.mtxChunkManager);
    AllocateChunks(Vector2u(newSize));
    chunkManager.lockAllChunks();
    for (LayerID i = 0; i < layerCount; i++)
    {
        Image t;
        t.resize(Vector2u(newSize.x, newSize.y));
        stb::stbir_resize(layers.at(i).getPixelsPtr(), layers.at(i).getSize().x, layers.at(i).getSize().y, 0,
            const_cast<unsigned char*>(t.getPixelsPtr()), newSize.x, newSize.y, 0,
            stb::STBIR_RGBA, stb::STBIR_TYPE_UINT8_SRGB, stb::STBIR_EDGE_CLAMP, filters.at(static_cast<int8_t>(method)));
        chunkManager.PasteImage(t, Vector2i(), IntRect(), ImageLayerType::Color, i);
    }
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::Adjustment(const Adjustments adjustment, const LayerID layerID, const AdjustmentData* data, const ChunkID chunkID)
{
    IntRect rect = IntRect(chunkID * static_cast<int32_t>(chunkManager.getChunkSize()), Vector2i(chunkManager.getChunkSize(chunkID)));
    {
        if (!chunkManager.getFinalSelectionBounds().expired())
        {
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
    IntRect rect = IntRect(chunkID * static_cast<int32_t>(chunkManager.getChunkSize()), Vector2i(chunkManager.getChunkSize(chunkID)));
    {
        if (!chunkManager.getFinalSelectionBounds().expired())
        {
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
    chunkManager.lockAllChunks();
    chunkManager.deleteColorTempLayerAll();
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::MergeColorTempLayer(const LayerID layerID, const BlendMode& blendMode)
{
    lock_guard lock(chunkManager.mtxChunkManager);
    chunkManager.MergeColorTempLayer(layerID, blendMode);
    chunkManager.lockAllChunks();
    chunkManager.deleteColorTempLayerAll();
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::AllocateChunks(const Vector2u size)
{
    if (!chunkManager.getFinalSelectionBounds().expired())
    {
        newMoveSelectArea = IntRect();
        chunkManager.resetSelection();
    }
    chunkManager.AllocateChunksFixed(size);
}

uint32_t glxy::ImageEditorWorker::getChunkIDFromPixel(const Vector2u pixel) const
{
    lock_guard lock(chunkManager.mtxChunkManager);
    return pixel.x / chunkManager.getChunkSize() + pixel.y / chunkManager.getChunkSize() * std::ceil(static_cast<float>(getSize().x) / chunkManager.getChunkSize());
}