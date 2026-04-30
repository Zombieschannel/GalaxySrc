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

    chunkManager.resetSelection();
}

void glxy::ImageEditorWorker::OptionPasteFromClipboard(const Image& src, Vector2u location, const Image& selection)
{
    OptionFinish();

    if (location.x + src.getSize().x > getSize().x || location.y + src.getSize().y > getSize().y)
        location = Vector2u();

    chunkManager.lockAllChunks();
    chunkManager.resetSelection();
    chunkManager.createSelectionLayerAll();
    chunkManager.unlockAllChunks();

    colorBufferTemp = make_unique<Image>();
    colorBufferTemp->resize(src.getSize());
    validate(colorBufferTemp->copy(src, Vector2u()));

    {
        lock_guard lock(mtxEditorWorkerCommon);
        newMoveSelectArea = IntRect(Vector2i(location), Vector2i(colorBufferTemp->getSize()));
        moveSelected = true;
    }
    selectionBufferTemp = make_unique<Image>();
    selectionBufferTemp->resize(src.getSize());
    validate(selectionBufferTemp->copy(selection, Vector2u()));
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
        if (chunkManager.anyHasSelectionLayer())
        {
            assert(!chunkManager.getFinalSelectionBounds().expired());
            CancelMovePixels(*chunkManager.getFinalSelectionBounds().lock());
        }
        else
            CancelMovePixels(IntRect(Vector2i(0, 0), Vector2i(chunkManager.getSize())));
        transformImageTransform.reset();
        lock_guard lock(mtxEditorWorkerCommon);
        transformImageSelectionArea = IntRect();
        transformImage = false;
    }
    if (shapeDraw)
    {
        chunkManager.lockAllChunks();
        chunkManager.deleteColorTempLayerAll();
        chunkManager.unlockAllChunks();
        lock_guard lock(mtxEditorWorkerCommon);
        shapeDraw = false;
    }
    if (textDraw)
    {
        chunkManager.lockAllChunks();
        chunkManager.deleteColorTempLayerAll();
        chunkManager.unlockAllChunks();
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
    if (shapeDraw)
    {
        MergeColorTempLayer(workingLayer, BlendAlpha);
        lock_guard lock(mtxEditorWorkerCommon);
        shapeDraw = false;
    }
    if (textDraw)
    {
        MergeColorTempLayer(workingLayer, BlendAlpha);
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
    if (!colorBufferTemp)
        colorBufferTemp = make_unique<Image>();

    if (lock_guard lock(chunkManager.mtxChunkManager);
        !chunkManager.anyHasSelectionLayer())
    {
        colorBufferTemp->resize(getSize(), Color::Transparent);
        chunkManager.lockAllChunks();
        chunkManager.CopyImage(*colorBufferTemp, Vector2i(), IntRect(), ImageLayerType::Color, layerID);

        Image empty;
        empty.resize(getSize(), Color::Transparent);
        chunkManager.PasteImage(empty, Vector2i(), IntRect(), ImageLayerType::Color, layerID);
        chunkManager.unlockAllChunks();
    }
    else
        SetupMovePixels();
}

void glxy::ImageEditorWorker::OptionTransformImage(const Vector2f pos, const float rot, const Vector2f scale, const Vector2f origin, const bool tile)
{
    Vector2f offset;
    if (!chunkManager.getFinalSelectionBounds().expired())
        offset = Vector2f(chunkManager.getFinalSelectionBounds().lock()->position);
    const Vector2f posOffset = Vector2f(colorBufferTemp->getSize()) / 2.f + offset;
    unique_ptr<Transformable>& tran = transformImageTransform;
    if (!tran)
        tran = make_unique<Transformable>();
    tran->setPosition(Vector2f(pos.x * getSize().x, pos.y * getSize().y) + posOffset);
    tran->setScale(scale);
    tran->setOrigin(Vector2f(origin.x * colorBufferTemp->getSize().x, origin.y * colorBufferTemp->getSize().y) +
        Vector2f(colorBufferTemp->getSize()) / 2.f);
    tran->setRotation(degrees(rot));

    {
        lock_guard lock(chunkManager.mtxChunkManager);
        chunkManager.lockAllChunks();
        if (!chunkManager.allHaveColorTempLayer())
            chunkManager.createColorTempLayerAll(getBlendMode(BlendAlpha));
        else
            chunkManager.clearColorTempLayerAll(Color::Transparent);
        if (chunkManager.anyHasSelectionLayer())
            chunkManager.clearSelectionLayerAll(false);
        chunkManager.unlockAllChunks();
    }

    const IntRect rect = TransformImage(tran->getTransform(), tile, false);
    lock_guard lock(mtxEditorWorkerCommon);
    transformImageSelectionArea = rect;
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
