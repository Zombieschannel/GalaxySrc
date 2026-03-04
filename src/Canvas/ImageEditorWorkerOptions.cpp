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

    chunkManager.lockAllChunks();
    chunkManager.rotate90CW();
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionRotate90CCW()
{
    OptionFinish();

    chunkManager.lockAllChunks();
    chunkManager.rotate90CCW();
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionRotate180()
{
    OptionFinish();

    chunkManager.lockAllChunks();
    chunkManager.rotate180();
    chunkManager.unlockAllChunks();
}

void glxy::ImageEditorWorker::OptionCopyToClipboard(Image& target, Vector2u& location, const LayerID layerID)
{
    OptionFinish();

    chunkManager.CopySelectedColorPixels(target, location, layerID);
}

void glxy::ImageEditorWorker::OptionSelectAll()
{
    OptionFinish();

    chunkManager.selectAll();
}

void glxy::ImageEditorWorker::OptionDeselectAll()
{
    OptionFinish();

    chunkManager.resetSelection();
}

void glxy::ImageEditorWorker::OptionDeleteSelected(const LayerID layerID)
{
    OptionFinish();

    assert(!chunkManager.getFinalSelectionBounds().expired());

    ForEachPixelInChunkArea(*chunkManager.getFinalSelectionBounds().lock(), true, [&](const Vector2i coord)
    {
        if (chunkManager.getPixelSelection(Vector2u(coord)))
            chunkManager.setPixelColor(Vector2u(coord), Color::Transparent, layerID);
    });

    chunkManager.resetSelection();
}

void glxy::ImageEditorWorker::OptionPasteFromClipboard(const Image& src, Vector2u location)
{
    OptionFinish();

    if (location.x + src.getSize().x > getSize().x || location.y + src.getSize().y > getSize().y)
        location = Vector2u();

    chunkManager.resetSelection();
    chunkManager.createSelectionLayer();
    colorBufferTemp = make_unique<Image>();
    colorBufferTemp->resize(src.getSize());
    validate(colorBufferTemp->copy(src, Vector2u()));

    {
        lock_guard lock(mtxEditorWorkerCommon);
        newMoveSelectionArea = IntRect(Vector2i(location), Vector2i(colorBufferTemp->getSize()));
        moveSelection = true;
    }
    selectionBufferTemp = make_unique<Image>();
    selectionBufferTemp->resize(src.getSize(), Color::Black);
}

void glxy::ImageEditorWorker::OptionCancel()
{
    if (bucketFill)
    {
        chunkManager.deleteColorTempLayer();
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
        chunkManager.deleteColorTempLayer();
        lock_guard lock(mtxEditorWorkerCommon);
        gradientDraw = false;
    }
    if (moveSelection)
    {
        if (!chunkManager.getFinalSelectionBounds().expired())
            CancelMovePixels(*chunkManager.getFinalSelectionBounds().lock());
        else
        {
            CancelMovePixels(IntRect());
            chunkManager.deleteSelectionLayer();
        }
        moveSelection = false;
    }
    if (transformImage)
    {
        if (chunkManager.hasSelectionLayer(0))
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
    if (moveSelection)
    {
        FinishMovePixels(newMoveSelectionArea);
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
        !chunkManager.hasSelectionLayer(0))
    {
        colorBufferTemp->resize(getSize(), Color::Transparent);
        chunkManager.lockAllChunks();
        chunkManager.CopyImage(*colorBufferTemp, Vector2u(), IntRect(), ImageLayerType::Color, layerID);

        Image empty;
        empty.resize(getSize(), Color::Transparent);
        chunkManager.PasteImage(empty, Vector2u(), IntRect(), ImageLayerType::Color, layerID);
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
        if (!chunkManager.hasColorTempLayer(0))
            chunkManager.createColorTempLayerAll(getBlendMode(BlendAlpha));
        else
            chunkManager.clearColorTempLayerAll(Color::Transparent);
        if (chunkManager.hasSelectionLayer(0))
            chunkManager.clearSelectionLayer(false);
        chunkManager.unlockAllChunks();
    }

    const IntRect rect = TransformImage(tran->getTransform(), tile);
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

    AllocateChunks(Vector2u(final.size), Color::Transparent);

    for (LayerID i = 0; i < chunkManager.getLayerCount(); i++)
        chunkManager.PasteImage(temp.at(i), Vector2u(), IntRect(), ImageLayerType::Color, i);
}
