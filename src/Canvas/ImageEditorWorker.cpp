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
        i < ceil(static_cast<float>(area.position.x + area.size.x) / chunkManager.getChunkSize()); i++)
        for (int32_t j = area.position.y / chunkManager.getChunkSize();
            j < ceil(static_cast<float>(area.position.y + area.size.y) / chunkManager.getChunkSize()); j++)
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
            chunkManager.resetSelection();
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
                if (abs(vec.x) > abs(vec.y))
                {
                    vec.y = abs(vec.x) * vec.y / abs(vec.y);
                    limitSize = vec.y > 0.f ? getSize().y - pixelSelectStartInt.y - 1 : pixelSelectStartInt.y;
                }
                else
                {
                    vec.x = abs(vec.y) * vec.x / abs(vec.x);
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

void glxy::ImageEditorWorker::EndSelect(const Vector2f pos, const ShapeSelectType type)
{
    lock_guard lock(chunkManager.mtxChunkManager);
    if (!chunkManager.hasStartedSelect())
        return;
    chunkManager.lockAllChunks();
    if (Distance::Point_Point(pixelSelectStart, pos) < 0.2f)
        chunkManager.resetSelection();
    else
        chunkManager.selectFinish();
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
            if (chunkManager.getPixelSelection(coord))
                chunkManager.setPixelColor(coord, Color::Transparent, workingLayer);
        });
        chunkManager.CopyImage(*selectionBufferTemp, Vector2i(intersection->position - bounds.position), *intersection, ImageLayerType::Selection);
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
        if (!chunkManager.allHaveColorTempLayer())
            chunkManager.createColorTempLayerAll(getBlendMode(BlendAlpha));
        else
            chunkManager.clearColorTempLayerAll(Color::Transparent);
        chunkManager.clearSelectionLayerAll(false);
        chunkManager.unlockAllChunks();
    }
    const IntRect rect = TransformImage(transform, false, false);

    lock_guard lock(mtxEditorWorkerCommon);
    newMoveSelectArea = rect;
}

void glxy::ImageEditorWorker::CancelMovePixels(const IntRect& area)
{
    const std::optional<IntRect> intersection = area.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize())));
    if (intersection)
    {
        chunkManager.lockAllChunks();
        if (chunkManager.anyHasSelectionLayer())
        {
            chunkManager.clearSelectionLayerAll(false);
            chunkManager.PasteImage(*selectionBufferTemp, Vector2i(intersection->position), IntRect(intersection->position - area.position, intersection->size), ImageLayerType::Selection);
            chunkManager.PasteSelectedColorPixels(*colorBufferTemp, workingLayer);
        }
        else
            chunkManager.PasteImage(*colorBufferTemp, Vector2i(), IntRect(), ImageLayerType::Color, workingLayer);
        chunkManager.unlockAllChunks();
    }
    chunkManager.lockAllChunks();
    chunkManager.deleteColorTempLayerAll();
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
            if (chunkManager.anyHasSelectionLayer() && !chunkManager.getPixelSelection(Vector2i(x, y)))
                continue;
            chunkManager.setPixelColor(Vector2i(x, y), chunkManager.getPixelColorTemp(Vector2i(x, y)), workingLayer);
        }
    if (chunkManager.anyHasSelectionLayer())
        chunkManager.setNewSelectionBounds(area);
    chunkManager.deleteColorTempLayerAll();
    chunkManager.unlockAllChunks();

    selectionBufferTemp.reset();
    colorBufferTemp.reset();
}

void glxy::ImageEditorWorker::SetupMoveSelection()
{
    assert(!chunkManager.getFinalSelectionBounds().expired());
    const IntRect bounds = *chunkManager.getFinalSelectionBounds().lock();
    if (!selectionBufferTemp)
        selectionBufferTemp = make_unique<Image>();
    selectionBufferTemp->resize(Vector2u(bounds.size), Color::Transparent);

    const std::optional<IntRect> intersection = bounds.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize())));
    if (intersection)
    {
        chunkManager.lockAllChunks();
        chunkManager.CopyImage(*selectionBufferTemp, Vector2i(intersection->position - bounds.position), *intersection, ImageLayerType::Selection);
        chunkManager.unlockAllChunks();
    }
}

void glxy::ImageEditorWorker::MoveSelection(const Transform& transform)
{
    if (!selectionBufferTemp)
        return;
    {
        lock_guard lock(chunkManager.mtxChunkManager);
        chunkManager.lockAllChunks();
        chunkManager.clearSelectionLayerAll(false);
        chunkManager.unlockAllChunks();
    }
    const IntRect rect = TransformImage(transform, false, true);

    lock_guard lock(mtxEditorWorkerCommon);
    newMoveSelectArea = rect;
}

void glxy::ImageEditorWorker::CancelMoveSelection(const IntRect& area)
{
    const std::optional<IntRect> intersection = area.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize())));
    if (intersection)
    {
        chunkManager.lockAllChunks();
        chunkManager.clearSelectionLayerAll(false);
        chunkManager.PasteImage(*selectionBufferTemp, Vector2i(intersection->position), IntRect(intersection->position - area.position, intersection->size), ImageLayerType::Selection);
        chunkManager.unlockAllChunks();
    }

    selectionBufferTemp.reset();
}

void glxy::ImageEditorWorker::FinishMoveSelection(const IntRect& area)
{
    chunkManager.lockAllChunks();
    chunkManager.setNewSelectionBounds(area);
    chunkManager.unlockAllChunks();

    selectionBufferTemp.reset();
}

IntRect glxy::ImageEditorWorker::TransformImage(const Transform& transform, const bool tile, const bool selectionOnly)
{
    const Vector2u sourceSize = selectionOnly ? selectionBufferTemp->getSize() : colorBufferTemp->getSize();
    const FloatRect area = FloatRect(Vector2f(), Vector2f(sourceSize));
    FloatRect transformedArea;
    if (!tile)
        transformedArea = transform.transformRect(area);
    else
        transformedArea = area;
    const Transform inverse = transform.getInverse();

    chunkManager.lockAllChunks();
    const bool selectAll = !chunkManager.anyHasSelectionLayer() && !chunkManager.anyHasSelectionTempLayer();

    for (int32_t x = max(roundf(transformedArea.position.x), 0.f);
        x < min(roundf(transformedArea.position.x + transformedArea.size.x), static_cast<float>(getSize().x)); x++)
        for (int32_t y = max(roundf(transformedArea.position.y), 0.f);
            y < min(roundf(transformedArea.position.y + transformedArea.size.y), static_cast<float>(getSize().y)); y++)
        {
            if (x < 0 || y < 0 || x >= getSize().x || y >= getSize().y)
                continue;
            const Vector2f pos = inverse.transformPoint(Vector2f(x + 0.5f, y + 0.5f));
            if (!selectAll && (pos.x < 0 || pos.y < 0 || pos.x >= sourceSize.x || pos.y >= sourceSize.y || selectionBufferTemp->getPixel(Vector2u(pos)) == Color::Transparent))
                continue;
            if (!selectionOnly)
            {
                Color col = Color::Transparent;
                if (tile && selectAll)
                {
                    const Vector2f tex = Vector2f(pos.x / sourceSize.x, pos.y / sourceSize.y);
                    const Vector2u coords = Vector2u((tex.x - floor(tex.x)) * sourceSize.x, (tex.y - floor(tex.y)) * sourceSize.y);
                    col = colorBufferTemp->getPixel(coords);
                }
                else if (!(pos.x < 0 || pos.y < 0 || pos.x >= sourceSize.x || pos.y >= sourceSize.y))
                    col = colorBufferTemp->getPixel(Vector2u(pos.x, pos.y));
                chunkManager.setPixelColorTemp(Vector2i(x, y), col);
            }
            if (!selectAll)
                chunkManager.setPixelSelection(Vector2i(x, y), true);
        }
    chunkManager.unlockAllChunks();
    return IntRect(Vector2i(round(transformedArea.position.x), round(transformedArea.position.y)), Vector2i(round(transformedArea.size.x), round(transformedArea.size.y)));
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
        RenderLayerToTexture(*chunkManager.getChunkImageSelection(chunkID), chunkSize, 255, RenderStates(StencilMode{
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
        out.push_back(Vector2i(floorf(end.x), floorf(end.y)));
    else if (fabs(diff.x) > fabs(diff.y))
    {
        const float offsetY = static_cast<float>(diff.y) / fabs(diff.x);
        for (int32_t i = 0; i < fabs(diff.x); i++)
            out.push_back(Vector2i(floorf(start.x + i * (diff.x / fabs(diff.x))), floorf(start.y + offsetY * i)));
    }
    else
    {
        const float offsetX = static_cast<float>(diff.x) / fabs(diff.y);
        for (int32_t i = 0; i < fabs(diff.y); i++)
            out.push_back(Vector2i(floorf(start.x + offsetX * i), floorf(start.y + i * (diff.y / fabs(diff.y)))));
    }
}

void glxy::ImageEditorWorker::PencilPixels(const Vector2f pos, const Vector2f prev, const Color color, const LayerID layerID)
{
    const FloatRect canvasBB = FloatRect({0.f, 0.f}, Vector2f(getSize().x, getSize().y));

    if (!chunkManager.isInfinite() && !canvasBB.findIntersection(FloatRect(Vector2f(pos.x - 0.5f, pos.y - 0.5f), Vector2f(2, 2))) &&
        !canvasBB.findIntersection(FloatRect(Vector2f(prev.x - 0.5f, prev.y - 0.5f), Vector2f(2, 2))))
        return;

    std::set<pair<int32_t, int32_t>> chunksToLock;
    vector<Vector2i> targetPixels;
    InterpolatePixelLine(prev, pos, targetPixels);
    if (chunkManager.isInfinite())
    {
        for (const Vector2i& n : targetPixels)
            chunksToLock.emplace(static_cast<int32_t>(floor(static_cast<float>(n.x) / chunkManager.getChunkSize())),
                static_cast<int32_t>(floor(static_cast<float>(n.y) / chunkManager.getChunkSize())));
        for (const pair<int32_t, int32_t>& n : chunksToLock)
        {
            if (chunkManager.chunkExists(ChunkID(n.first, n.second)))
                continue;
            chunkManager.AllocateChunkInfinite(ChunkID(n.first, n.second));
        }
    }

    lock_guard lock(chunkManager.mtxChunkManager);
    chunkManager.lockAllChunks();
    for (const auto& n : targetPixels)
    {
        if (!chunkManager.isInfinite() && !canvasBB.contains(Vector2f(n.x + 0.5f, n.y + 0.5f)))
            continue;
        if (chunkManager.getPixelColor(n, layerID) != color && (!chunkManager.anyHasSelectionLayer() || chunkManager.getPixelSelection(n)))
            chunkManager.setPixelColor(n, color, layerID);
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
    if (!chunkManager.isInfinite() && !intersect)
        return;

    IntRect result;
    if (!chunkManager.isInfinite())
        result = *intersect;
    if (chunkManager.isInfinite())
    {
        result = rectShape;
        ForEachChunkInChunkArea(result, [&](const ChunkID chunkID)
        {
            if (chunkManager.chunkExists(chunkID))
                return;
            chunkManager.AllocateChunkInfinite(chunkID);
        });
    }

    ForEachChunkInChunkArea(result, [&](const ChunkID chunkID)
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
            RenderLayerToTexture(*chunkManager.getChunkImageColorTemp(chunkID), chunkSize, 255, BlendNone, renderTexture);

        SetupMaskedRendering(renderTexture, chunkID);

        renderTexture.setView(View(FloatRect(Vector2f(chunkID.x * chunkGeneralSize, chunkID.y * chunkGeneralSize),
            Vector2f(chunkSize))));

        CircleShape shape;
        shape.setFillColor(color);

        RenderStates states;
        states.blendMode = BlendNone;
        states.stencilMode = {chunkManager.hasSelectionLayer(chunkID) ? StencilComparison::Equal : StencilComparison::Always,
            StencilUpdateOperation::Keep, 0x01, 0xFF, false};

        shape.setPointCount(fmax(2.f * c_PI * sqrtf(radius), 30.f));
        shape.setRadius(radius);
        shape.setOrigin(Vector2f(radius, radius));

        const Vector2f startPos = Vector2f(Vector2i(floor(start.x), floor(start.y))) + Vector2f(0.5f, 0.5f);
        const Vector2f endPos = Vector2f(Vector2i(floor(end.x), floor(end.y))) + Vector2f(0.5f, 0.5f);

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
                                IntRect(), ImageLayerType::ColorTemp, layerID);
        chunkManager.getChunkMutex(chunkID).unlock();
    });
}

void glxy::ImageEditorWorker::ColorSwapPixels(const Vector2f start, const Vector2f end, const float radius, const Color color1, const Color color2, const int8_t tol, const LayerID layerID)
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

    ForEachChunkInChunkArea(*intersect, [&](const ChunkID chunkID)
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

        shape.setPointCount(fmax(2.f * c_PI * sqrtf(radius), 30.f));
        shape.setRadius(radius);
        shape.setOrigin(Vector2f(radius, radius));

        const Vector2f startPos = Vector2f(Vector2i(floor(start.x), floor(start.y))) + Vector2f(0.5f, 0.5f);
        const Vector2f endPos = Vector2f(Vector2i(floor(end.x), floor(end.y))) + Vector2f(0.5f, 0.5f);

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
    chunkManager.FloodFill(ImageLayerType::ColorTemp, pos, color, tolerance, chunkManager.anyHasSelectionLayer(), layerID);
    chunkManager.unlockAllChunks();
    return true;
}

void glxy::ImageEditorWorker::GradientPixels(const Vector2f startPos, const Vector2f endPos, const Color startColor, const Color endColor)
{
    chunkManager.mtxChunkManager.lock();
    chunkManager.createColorTempLayerAll(getBlendMode(BlendAlpha));
    IntRect area;
    if (!chunkManager.anyHasSelectionLayer())
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
            if (chunkManager.anyHasSelectionLayer() && !chunkManager.getPixelSelection(pos))
                return;
            chunkManager.setPixelColorTemp(pos, startColor);
        });
        return;
    }
    const float dist = Distance::Point_Point(startPos, endPos);
    chunkManager.mtxChunkManager.unlock();

    ForEachPixelInChunkArea(area, true, [&](const Vector2i pos)
    {
        if (chunkManager.anyHasSelectionLayer() && !chunkManager.getPixelSelection(Vector2i(pos.x, pos.y)))
            return;
        const Vector2f s = Vector2f(pos.x + 0.5f - startPos.x, pos.y + 0.5f - startPos.y);
        const Vector2f e = Vector2f(pos.x + 0.5f - endPos.x, pos.y + 0.5f - endPos.y);
        if (s.x * s.x + s.y * s.y - e.x * e.x - e.y * e.y > dist * dist)
            chunkManager.setPixelColorTemp(Vector2i(pos.x, pos.y), endColor);
        else if (s.x * s.x + s.y * s.y - e.x * e.x - e.y * e.y < -dist * dist)
            chunkManager.setPixelColorTemp(Vector2i(pos.x, pos.y), startColor);
        else
        {
            const Color c = LerpColor(startColor, endColor, (s.x * s.x + s.y * s.y - e.x * e.x - e.y * e.y) / (dist * dist) / 2 + 0.5f);
            chunkManager.setPixelColorTemp(Vector2i(pos.x, pos.y), c);
        }
    });
}

void glxy::ImageEditorWorker::ShapePixels(const shared_ptr<ConvexShape>& shape, const LayerID layerID)
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
                IntRect(), ImageLayerType::ColorTemp, layerID);

        chunkManager.getChunkMutex(chunkID).unlock();
    });
}

void glxy::ImageEditorWorker::TextPixels(const shared_ptr<Text>& text, const FloatRect& globalBounds, const LayerID layerID)
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
                IntRect(), ImageLayerType::ColorTemp, layerID);

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
        chunkManager.PasteImage(layers.at(i), Vector2i(-min(newSize.position.x, 0), -min(newSize.position.y, 0)),
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
        if (chunkManager.anyHasSelectionLayer())
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
    IntRect rect = IntRect(chunkID * static_cast<int32_t>(chunkManager.getChunkSize()), Vector2i(chunkManager.getChunkSize(chunkID)));
    {
        if (chunkManager.anyHasSelectionLayer())
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
    if (chunkManager.anyHasSelectionLayer())
        chunkManager.resetSelection();
    chunkManager.AllocateChunksFixed(size);
}

uint32_t glxy::ImageEditorWorker::getChunkIDFromPixel(const Vector2u pixel) const
{
    lock_guard lock(chunkManager.mtxChunkManager);
    return pixel.x / chunkManager.getChunkSize() + pixel.y / chunkManager.getChunkSize() * ceil(static_cast<float>(getSize().x) / chunkManager.getChunkSize());
}