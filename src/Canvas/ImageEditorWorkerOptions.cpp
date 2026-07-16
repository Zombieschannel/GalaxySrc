#include "../Func.hpp"
#include "ImageEditorWorker.hpp"

void glxy::ImageEditorWorker::OptionCreateLayer(const LayerID layerID)
{
    OptionFinish();

    chunkManager.lockAllChunks();
    chunkManager.addLayer(layerID + 1);
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionDeleteLayer(const LayerID layerID)
{
    OptionFinish();

    chunkManager.lockAllChunks();
    chunkManager.deleteLayer(layerID);
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionDuplicateLayer(const LayerID layerID)
{
    OptionFinish();

    chunkManager.lockAllChunks();
    chunkManager.duplicateLayer(layerID);
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionMoveLayerUp(const LayerID layerID)
{
    OptionFinish();

    chunkManager.lockAllChunks();
    chunkManager.moveLayerUp(layerID);
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionMoveLayerDown(const LayerID layerID)
{
    OptionFinish();

    chunkManager.lockAllChunks();
    chunkManager.moveLayerDown(layerID);
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionMergeLayerDown(const LayerID layerID)
{
    OptionFinish();
    chunkManager.lockAllChunks();
    chunkManager.mergeLayerDown(layerID);
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionFlipLayerHorizontal(const LayerID layerID)
{
    OptionFinish();

    chunkManager.lockAllChunks();
    chunkManager.flipLayerHorizontal(layerID);
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionFlipLayerVertical(const LayerID layerID)
{
    OptionFinish();

    chunkManager.lockAllChunks();
    chunkManager.flipLayerVertical(layerID);
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionFlipImageHorizontal()
{
    OptionFinish();

    chunkManager.lockAllChunks();
    for (LayerID i = 0; i < chunkManager.getLayerCount(); i++)
        chunkManager.flipLayerHorizontal(i);
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionFlipImageVertical()
{
    OptionFinish();

    chunkManager.lockAllChunks();
    for (LayerID i = 0; i < chunkManager.getLayerCount(); i++)
        chunkManager.flipLayerVertical(i);
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionRotate90CW()
{
    OptionFinish();

    chunkManager.rotate90CW();
}

void glxy::ImageEditorWorker::OptionRotate90CCW()
{
    OptionFinish();

    chunkManager.rotate90CCW();
}

void glxy::ImageEditorWorker::OptionRotate180()
{
    OptionFinish();

    chunkManager.lockAllChunks();
    chunkManager.rotate180();
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionCopyToClipboard(Image& target, Vector2u& location, Image& selectionTarget, const LayerID layerID)
{
    OptionFinish();
    chunkManager.CopySelectedColorPixels(target, location, layerID);
    selectionTarget.resize(target.getSize());
    chunkManager.CopyImage(selectionTarget, Vector2i(), IntRect(Vector2i(location), Vector2i(target.getSize())), ImageLayerType::Selection);
}

void glxy::ImageEditorWorker::OptionSelectArea(const FloatRect& area)
{
    OptionFinish();

    chunkManager.selectArea(area);
}

void glxy::ImageEditorWorker::OptionDeselectAll()
{
    OptionFinish();

    newMoveSelectArea = IntRect();
    chunkManager.resetSelection();
}

void glxy::ImageEditorWorker::OptionDeleteSelected(const LayerID layerID)
{
    OptionFinish();

    if (chunkManager.getFinalSelectionBounds().expired())
        return;

    ForEachPixelInChunkArea(*chunkManager.getFinalSelectionBounds().lock(), true, [&](const Vector2i coord)
    {
        if (chunkManager.getPixelSelection(coord))
            chunkManager.setPixelColor(coord, Color::Transparent, layerID);
    });

    newMoveSelectArea = IntRect();
    chunkManager.resetSelection();
}

void glxy::ImageEditorWorker::OptionPasteFromClipboard(const Image& src, Vector2u location, const Image& selection)
{
    OptionFinish();

    if (location.x + src.getSize().x > getSize().x || location.y + src.getSize().y > getSize().y)
        location = Vector2u();

    chunkManager.lockAllChunks();
    chunkManager.resetSelection();
    chunkManager.ForEachChunkInChunkArea(IntRect(Vector2i(location), Vector2i(src.getSize())),
        [&](const ChunkID chunkID)
    {
        chunkManager.createSelectionLayer(chunkID);
    });
    chunkManager.unlockAllChunks();

    transformImageCache.colorBufferTemp = make_unique<Image>();
    transformImageCache.colorBufferTemp->resize(src.getSize());
    validate(transformImageCache.colorBufferTemp->copy(src, Vector2u()));
    transformImageCache.requiresUpdateColor = true;

    {
        lock_guard lock(mtxEditorWorkerCommon);
        newMoveSelectArea = IntRect(Vector2i(location), Vector2i(transformImageCache.colorBufferTemp->getSize()));
        moveSelected = true;
    }
    transformImageCache.selectionBufferTemp = make_unique<Image>();
    transformImageCache.selectionBufferTemp->resize(src.getSize());
    validate(transformImageCache.selectionBufferTemp->copy(selection, Vector2u()));
    transformImageCache.requiresUpdateSelection = true;
}

void glxy::ImageEditorWorker::OptionCancel()
{
    if (bucketFill)
    {
        chunkManager.lockAllChunks();
        chunkManager.deleteColorTempLayerAll();
        chunkManager.unlockAllChunks();
        lock_guard lock(mtxEditorWorkerCommon);
        bucketFill = false;
    }
    if (wandFill)
    {
        chunkManager.wandCancel();
        lock_guard lock(mtxEditorWorkerCommon);
        wandFill = false;
    }
    if (gradientDraw)
    {
        chunkManager.lockAllChunks();
        chunkManager.deleteColorTempLayerAll();
        chunkManager.unlockAllChunks();
        lock_guard lock(mtxEditorWorkerCommon);
        gradientDraw = false;
    }
    if (moveSelected)
    {
        if (!chunkManager.getFinalSelectionBounds().expired())
            CancelMovePixels(*chunkManager.getFinalSelectionBounds().lock());
        else
        {
            CancelMovePixels(IntRect());
            newMoveSelectArea = IntRect();
            chunkManager.deleteSelectionLayerAll();
        }
        moveSelected = false;
    }
    if (moveSelection)
    {
        assert(!chunkManager.getFinalSelectionBounds().expired());
        CancelMoveSelection(*chunkManager.getFinalSelectionBounds().lock());
        moveSelection = false;
    }
    if (transformImage)
    {
        if (!chunkManager.getFinalSelectionBounds().expired())
            CancelMovePixels(*chunkManager.getFinalSelectionBounds().lock());
        else
            CancelMovePixels(IntRect(Vector2i(0, 0), Vector2i(chunkManager.getSize())));
        transformImageTransform.reset();
        lock_guard lock(mtxEditorWorkerCommon);
        transformImageSelectionArea = IntRect();
        transformImage = false;
    }
    if (circularShift)
    {
        chunkManager.lockAllChunks();
        chunkManager.deleteColorTempLayerAll();
        chunkManager.unlockAllChunks();
        lock_guard lock(mtxEditorWorkerCommon);
        circularShift = false;
    }
    if (shapeDraw)
    {
        chunkManager.lockAllChunks();
        chunkManager.deleteColorTempLayerAll();
        chunkManager.unlockAllChunks();
        prevShapeRenderArea = IntRect();
        lock_guard lock(mtxEditorWorkerCommon);
        shapeDraw = false;
    }
    if (textDraw)
    {
        chunkManager.lockAllChunks();
        chunkManager.deleteColorTempLayerAll();
        chunkManager.unlockAllChunks();
        prevTextRenderArea = IntRect();
        lock_guard lock(mtxEditorWorkerCommon);
        textDraw = false;
    }
}

void glxy::ImageEditorWorker::OptionFinish()
{
    if (bucketFill)
    {
        MergeColorTempLayer(workingLayer, BlendAlpha);
        lock_guard lock(mtxEditorWorkerCommon);
        bucketFill = false;
    }
    if (wandFill)
    {
        chunkManager.wandFinish();
        lock_guard lock(mtxEditorWorkerCommon);
        wandFill = false;
    }
    if (gradientDraw)
    {
        MergeColorTempLayer(workingLayer, BlendAlpha);
        lock_guard lock(mtxEditorWorkerCommon);
        gradientDraw = false;
    }
    if (moveSelected)
    {
        FinishMovePixels(newMoveSelectArea);
        lock_guard lock(mtxEditorWorkerCommon);
        moveSelected = false;
    }
    if (moveSelection)
    {
        FinishMoveSelection(newMoveSelectArea);
        lock_guard lock(mtxEditorWorkerCommon);
        moveSelection = false;
    }
    if (transformImage)
    {
        FinishMovePixels(transformImageSelectionArea);
        transformImageTransform.reset();
        lock_guard lock(mtxEditorWorkerCommon);
        transformImageSelectionArea = IntRect();
        transformImage = false;
    }
    if (circularShift)
    {
        MergeColorTempLayer(workingLayer, BlendNone);
        lock_guard lock(mtxEditorWorkerCommon);
        circularShift = false;
    }
    if (shapeDraw)
    {
        MergeColorTempLayer(workingLayer, BlendAlpha);
        prevShapeRenderArea = IntRect();
        lock_guard lock(mtxEditorWorkerCommon);
        shapeDraw = false;
    }
    if (textDraw)
    {
        MergeColorTempLayer(workingLayer, BlendAlpha);
        prevTextRenderArea = IntRect();
        lock_guard lock(mtxEditorWorkerCommon);
        textDraw = false;
    }
}

void glxy::ImageEditorWorker::OptionSetupTransformImage(const LayerID layerID)
{
    OptionFinish();
    {
        lock_guard lock(mtxEditorWorkerCommon);
        transformImage = true;
    }
    if (!transformImageCache.colorBufferTemp)
        transformImageCache.colorBufferTemp = make_unique<Image>();

    if (lock_guard lock(chunkManager.mtxChunkManager);
        chunkManager.getFinalSelectionBounds().expired())
    {
        transformImageCache.colorBufferTemp->resize(getSize(), Color::Transparent);
        chunkManager.CopyImage(*transformImageCache.colorBufferTemp, Vector2i(), IntRect(), ImageLayerType::Color, layerID);
        transformImageCache.requiresUpdateColor = true;

        Image empty;
        empty.resize(getSize(), Color::Transparent);

        chunkManager.lockAllChunks();
        chunkManager.PasteImage(empty, Vector2i(), IntRect(), ImageLayerType::Color, layerID);
        chunkManager.unlockAllChunks();
    }
    else
        SetupMovePixels();
}

void glxy::ImageEditorWorker::OptionTransformImage(const Vector2f pos, const float rot, const Vector2f scale, const Vector2f origin, const int16_t repeat, const bool smooth)
{
    Vector2f offset;
    if (!chunkManager.getFinalSelectionBounds().expired())
        offset = Vector2f(chunkManager.getFinalSelectionBounds().lock()->position);
    const Vector2f posOffset = Vector2f(transformImageCache.colorBufferTemp->getSize()) / 2.f + offset;
    unique_ptr<Transformable>& tran = transformImageTransform;
    if (!tran)
        tran = make_unique<Transformable>();
    tran->setPosition(Vector2f(pos.x * getSize().x, pos.y * getSize().y) + posOffset);
    tran->setScale(scale);
    tran->setOrigin(Vector2f(origin.x * transformImageCache.colorBufferTemp->getSize().x, origin.y * transformImageCache.colorBufferTemp->getSize().y) +
        Vector2f(transformImageCache.colorBufferTemp->getSize()) / 2.f);
    tran->setRotation(degrees(rot));

    const IntRect rect = TransformImage(tran->getTransform(), repeat, smooth, false);

    lock_guard lock(mtxEditorWorkerCommon);
    if (transformImageSelectionArea != IntRect())
    {
        if (const std::optional<IntRect> area = transformImageSelectionArea.findIntersection(IntRect(Vector2i(0, 0), Vector2i(getSize()))))
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
    transformImageSelectionArea = rect;
}

void glxy::ImageEditorWorker::OptionSetupCircularShift()
{
    OptionFinish();
    chunkManager.lockAllChunks();
    if (!chunkManager.allHaveColorTempLayer())
        chunkManager.createColorTempLayerAll(getBlendMode(BlendAlpha));
    chunkManager.unlockAllChunks();
    {
        lock_guard lock(mtxEditorWorkerCommon);
        circularShift = true;
    }
}

void glxy::ImageEditorWorker::OptionCircularShift(const int32_t shift, const bool horizontal, const int32_t groupSize)
{
    chunkManager.lockAllChunks();
    if (horizontal)
    {
        for (int32_t y = 0; y < getSize().y; y++)
            for (int32_t x = 0; x < getSize().x; x++)
            {
                int64_t targetPixel = x + static_cast<int32_t>(getSize().x) * y - shift;
                targetPixel = modneg(targetPixel, getSize().x * getSize().y);
                const Color color = chunkManager.getPixelColor(Vector2i(targetPixel % getSize().x,
                        modneg(y + (targetPixel / getSize().x - y) * groupSize, getSize().y)), workingLayer);
                chunkManager.setPixelColorTemp(Vector2i(x, y), color);
            }
    }
    else
    {
        for (int32_t x = 0; x < getSize().x; x++)
            for (int32_t y = 0; y < getSize().y; y++)
            {
                int64_t targetPixel = y + static_cast<int32_t>(getSize().y) * x - shift;
                targetPixel = modneg(targetPixel, getSize().x * getSize().y);
                const Color color = chunkManager.getPixelColor(
                    Vector2i(modneg(x + (targetPixel / getSize().y - x) * groupSize, getSize().x),
                    targetPixel % getSize().y), workingLayer);
                chunkManager.setPixelColorTemp(Vector2i(x, y), color);
            }
    }
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionCropSelection()
{
    OptionFinish();

    assert(!chunkManager.getFinalSelectionBounds().expired());
    const IntRect final = *chunkManager.getFinalSelectionBounds().lock();

    vector<Image> temp;
    temp.resize(chunkManager.getLayerCount());
    Vector2u dummy;
    for (LayerID i = 0; i < chunkManager.getLayerCount(); i++)
        chunkManager.CopySelectedColorPixels(temp.at(i), dummy, i);

    AllocateChunks(Vector2u(final.size));

    for (LayerID i = 0; i < chunkManager.getLayerCount(); i++)
        chunkManager.PasteImage(temp.at(i), Vector2i(), IntRect(), ImageLayerType::Color, i);
}
