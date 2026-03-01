#include "Func.hpp"
#include "ImageEditor.hpp"

void glxy::ImageEditor::OptionZoomIn(const bool basedOnMouse)
{
    if (windowScale > 0.005f)
    {
        windowScale *= 1 / 1.3f;
        const Vector2f pos = getSFMLViewMousePos(viewArea, view);
        if (settings.animateZoom)
        {
            cameraOriginalPos = view.getCenter();
            cameraOriginalSize = view.getSize();
            cameraAnimation = Time::Zero;
            cameraAnimationRunning = true;
        }

        view.setSize({ viewArea.size.x * windowScale, viewArea.size.y * windowScale });
        if (basedOnMouse)
        {
            view.move(pos - getSFMLViewMousePos(viewArea, view));
            ClampView();
        }

        if (settings.animateZoom)
        {
            cameraTargetPos = view.getCenter();
            cameraTargetSize = view.getSize();
        }
    }
}

void glxy::ImageEditor::OptionZoomOut(const bool basedOnMouse)
{
    if (windowScale < 50.f)
    {
        windowScale *= 1.3f;
        const Vector2f pos = getSFMLViewMousePos(viewArea, view);
        if (settings.animateZoom)
        {
            cameraOriginalPos = view.getCenter();
            cameraOriginalSize = view.getSize();
            cameraAnimation = Time::Zero;
            cameraAnimationRunning = true;
        }

        view.setSize({ viewArea.size.x * windowScale, viewArea.size.y * windowScale });
        if (basedOnMouse)
        {
            view.move(pos - getSFMLViewMousePos(viewArea, view));
            ClampView();
        }
        if (settings.animateZoom)
        {
            cameraTargetPos = view.getCenter();
            cameraTargetSize = view.getSize();
        }
    }
}

void glxy::ImageEditor::OptionGrid(const bool state)
{
    gridLines.setEnabled(state);
}

void glxy::ImageEditor::OptionRuler(const bool state)
{
    rulerUI.setEnabled(state);
}

void glxy::ImageEditor::OptionActualSize()
{
    windowScale = 1.f;
    view.setSize({ viewArea.size.x, viewArea.size.y});
    view.setCenter(Vector2f(getSize().x / 2.f, getSize().y / 2.f));
}

void glxy::ImageEditor::OptionCreateLayer()
{
    OptionFinish();

    chunkManager.addLayer(_layerPicker.getLayerIDSelected() + 1);
    _layerPicker.createNewLayer();
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionDeleteLayer()
{
    OptionFinish();

    chunkManager.deleteLayer(_layerPicker.getLayerIDSelected());
    _layerPicker.deleteLayer();
    chunkManager.RenderChunkArea();
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionDuplicateLayer()
{
    OptionFinish();

    chunkManager.duplicateLayer(_layerPicker.getLayerIDSelected());
    _layerPicker.duplicateLayer();
    chunkManager.RenderChunkArea();
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionMoveLayerUp()
{
    OptionFinish();

    chunkManager.moveLayerUp(_layerPicker.getLayerIDSelected());
    _layerPicker.moveLayerUp();
    chunkManager.RenderChunkArea();
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionMoveLayerDown()
{
    OptionFinish();

    chunkManager.moveLayerDown(_layerPicker.getLayerIDSelected());
    _layerPicker.moveLayerDown();
    chunkManager.RenderChunkArea();
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionMergeLayerDown()
{
    OptionFinish();

    chunkManager.mergeLayerDown(_layerPicker.getLayerIDSelected());
    _layerPicker.deleteLayer();
    updateLayerPreview(_layerPicker.getLayerIDSelected());
    chunkManager.RenderChunkArea();
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionFlipLayerHorizontal()
{
    OptionFinish();

    chunkManager.flipLayerHorizontal(_layerPicker.getLayerIDSelected());
    chunkManager.RenderChunkArea();
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionFlipLayerVertical()
{
    OptionFinish();

    chunkManager.flipLayerVertical(_layerPicker.getLayerIDSelected());
    chunkManager.RenderChunkArea();
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionFlipImageHorizontal()
{
    OptionFinish();

    for (int16_t i = 0; i < _layerPicker.getLayerCount(); i++)
        chunkManager.flipLayerHorizontal(i);
    chunkManager.RenderChunkArea();
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionFlipImageVertical()
{
    OptionFinish();

    for (int16_t i = 0; i < _layerPicker.getLayerCount(); i++)
        chunkManager.flipLayerVertical(i);
    chunkManager.RenderChunkArea();
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionRotate90CW()
{
    OptionFinish();

    chunkManager.rotate90CW();
    chunkManager.RenderChunkArea();
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionRotate90CCW()
{
    OptionFinish();

    chunkManager.rotate90CCW();
    chunkManager.RenderChunkArea();
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionRotate180()
{
    OptionFinish();

    chunkManager.rotate180();
    chunkManager.RenderChunkArea();
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionCopyToClipboard(Image& target, Vector2u& location)
{
    OptionFinish();

    pixelSelect.copySelectedPixels(target, location, _layerPicker.getLayerIDSelected());
}

void glxy::ImageEditor::OptionSelectAll()
{
    OptionFinish();

    pixelSelect.selectAll();
}

void glxy::ImageEditor::OptionDeselectAll()
{
    OptionFinish();

    pixelSelect.clear();
}

void glxy::ImageEditor::OptionDeleteSelected()
{
    OptionFinish();

    assert(!pixelSelect.getFinalSelectionBounds().expired());
    const IntRect area = *pixelSelect.getFinalSelectionBounds().lock();
    pixelSelect.clearSelectedPixels();
    pixelSelect.clear();
    updateLayerPreview(_layerPicker.getLayerIDSelected());

    chunkManager.RenderChunkArea(area);
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionPasteFromClipboard(const Image& src, Vector2u location)
{
    OptionFinish();

    if (location.x + src.getSize().x > getSize().x || location.y + src.getSize().y > getSize().y)
        location = Vector2u();

    pixelSelect.clear();
    chunkManager.createSelectionLayer();

    tempImage = make_unique<Image>();
    tempImage->resize(src.getSize());
    validate(tempImage->copy(src, Vector2u()));

    moveSelectionArea = IntRect(Vector2i(location), Vector2i(tempImage->getSize()));
    moveSelection = true;
    pixelSelect.createTempSelection(src.getSize());
    pixelSelect.setTempSelectedAreaPosition(Vector2i(location));
    pixelSelect.setNewBounds(IntRect(Vector2i(location), Vector2i(tempImage->getSize())));
    pixelSelect.copyTempSelection(Image(src.getSize(), Color::Black));

    moveSelectionTransform = make_unique<Transformable>();
    moveSelectionTransform->setPosition(Vector2f(location));
    moveSelectionOriginalPosition.reset();

    SetupMovePixelsUI();
    GenerateMovePixels();
    chunkManager.RenderChunkArea(moveSelectionArea);
    unsavedChanges = true;
}

void glxy::ImageEditor::OptionCancel()
{
    if (bucketFill)
    {
        chunkManager.deleteTempLayer();
        chunkManager.RenderChunkArea(lastFillArea);
        bucketFill = false;
        lastFillArea = IntRect();
    }
    if (wandFill)
    {
        pixelSelect.wandCancel();
        if (!pixelSelect.getFinalSelectionBounds().expired())
            chunkManager.deleteSelectionLayer();
        else
            pixelSelect.UpdateTexture(IntRect({}, Vector2i(getSize())));
        wandFill = false;
    }
    if (gradientDraw)
    {
        chunkManager.deleteTempLayer();
        chunkManager.RenderChunkArea();
        gradientDraw = false;
    }
    if (moveSelection)
    {
        if (moveSelectionOriginalPosition)
        {
            const Vector2i loc = *moveSelectionOriginalPosition;
            chunkManager.MergeImageWithMask(*tempImage, *pixelSelect.getTempMask(), loc);
            pixelSelect.setNewBounds(IntRect(loc, Vector2i(tempImage->getSize())));
            pixelSelect.revertTempSelection();
            const IntRect rect = IntRect(loc, Vector2i(tempImage->getSize()));
            pixelSelect.UpdateTexture(getUnion(&rect, &moveSelectionArea));

            moveSelectionTransform = make_unique<Transformable>();
            moveSelectionTransform->setPosition(Vector2f(loc));
            moveSelectionOriginalPosition.reset();
            tempImage.reset();

            SetupMovePixelsUI();
        }
        else
        {
            pixelSelect.UpdateTexture(moveSelectionArea);
            tempImage.reset();
            moveSelectionTransform.reset();
            pixelSelect.clear();
        }
        chunkManager.deleteTempLayer();
        moveSelection = false;
        chunkManager.RenderChunkArea();
    }
    if (transformImage)
    {
        const Vector2i loc = transformImageOriginalPosition;
        if (chunkManager.hasSelectionLayer())
        {
            chunkManager.MergeImageWithMask(*tempImage, *pixelSelect.getTempMask(), loc);
            pixelSelect.setNewBounds(IntRect(loc, Vector2i(tempImage->getSize())));
            pixelSelect.revertTempSelection();
            const IntRect rect = IntRect(loc, Vector2i(tempImage->getSize()));
            pixelSelect.UpdateTexture(getUnion(&rect, &transformImageSelectionArea));
        }
        else
            chunkManager.PasteImage(*tempImage, Vector2u(), IntRect(), ImageCopyType::Color, _layerPicker.getLayerIDSelected());

        chunkManager.deleteTempLayer();
        tempImage.reset();
        transformImageTransform.reset();
        chunkManager.RenderChunkArea();
        transformImageSelectionArea = IntRect();
        transformImage = false;
    }
}

void glxy::ImageEditor::OptionFinish()
{
    if (bucketFill)
    {
        MergeTempLayer();
        bucketFill = false;
        lastFillArea = IntRect();
        unsavedChanges = true;
    }
    if (wandFill)
    {
        pixelSelect.wandFinish();
        wandFill = false;
    }
    if (gradientDraw)
    {
        MergeTempLayer();
        gradientDraw = false;
        unsavedChanges = true;
    }
    if (moveSelection)
    {
        const std::optional<IntRect> intersect = moveSelectionArea.findIntersection(IntRect({0, 0}, Vector2i(getSize())));
        if (intersect)
            chunkManager.MergeTempLayerWithMask(intersect->position, *intersect);
        FinishMovePixels();
        chunkManager.deleteTempLayer();
        tempImage.reset();

        moveSelectionTransform = make_unique<Transformable>();
        moveSelectionTransform->setPosition(Vector2f(moveSelectionArea.position));
        moveSelectionOriginalPosition = make_unique<Vector2i>(moveSelectionArea.position);

        moveSelection = false;
        SetupMovePixelsUI();
        updateLayerPreview(_layerPicker.getLayerIDSelected());
        if (intersect)
            chunkManager.RenderChunkArea(*intersect);
        unsavedChanges = true;
    }
    if (transformImage)
    {
        if (chunkManager.hasSelectionLayer())
        {
            const std::optional<IntRect> intersect = transformImageSelectionArea.findIntersection(IntRect({0, 0}, Vector2i(getSize())));
            if (intersect)
                chunkManager.MergeTempLayerWithMask(intersect->position, *intersect);

            Image temp;
            const FloatRect area = FloatRect(Vector2f(), Vector2f(tempImage->getSize()));
            const FloatRect transformedArea = transformImageTransform->getTransform().transformRect(area);
            const Transform inverse = transformImageTransform->getInverseTransform();
            temp.resize(Vector2u(round(transformedArea.size.x), round(transformedArea.size.y)), Color::Transparent);

            for (int32_t x = round(transformedArea.position.x); x < round(transformedArea.position.x + transformedArea.size.x); x++)
                for (int32_t y = round(transformedArea.position.y); y < round(transformedArea.position.y + transformedArea.size.y); y++)
                {
                    const Vector2f pos = inverse.transformPoint(Vector2f(x + 0.5f, y + 0.5f));
                    if (pos.x < 0 || pos.y < 0 || pos.x >= tempImage->getSize().x || pos.y >= tempImage->getSize().y)
                        continue;
                    if (!pixelSelect.withinTempBounds(Vector2u(pos)))
                        continue;
                    temp.setPixel(Vector2u(x - transformedArea.position.x, y - transformedArea.position.y), Color::Black);
                }
            pixelSelect.copyTempSelection(temp);
            pixelSelect.setNewBounds(transformImageSelectionArea);
            pixelSelect.UpdateTexture(transformImageSelectionArea);
        }
        else
            chunkManager.SwapLayerWithTemp(_layerPicker.getLayerIDSelected());

        chunkManager.deleteTempLayer();
        tempImage.reset();
        transformImageTransform.reset();
        updateLayerPreview(_layerPicker.getLayerIDSelected());
        chunkManager.RenderChunkArea(transformImageSelectionArea);
        transformImageSelectionArea = IntRect();
        transformImage = false;
        unsavedChanges = true;
    }
}

void glxy::ImageEditor::OptionSyncViewport() const
{
    settings.syncViewport = !settings.syncViewport;
}

void glxy::ImageEditor::OptionSetBrushSize(const float radius)
{
    if (radius == 1.f)
    {
        brushInnerOutline.setRadius(0.5f);
        brushInnerOutline.setOrigin({brushInnerOutline.getRadius(), brushInnerOutline.getRadius()});
        brushOuterOutline.setRadius(0.5f);
        brushOuterOutline.setOrigin({brushOuterOutline.getRadius(), brushOuterOutline.getRadius()});
    }
    else
    {
        brushInnerOutline.setRadius(radius);
        brushInnerOutline.setOrigin({brushInnerOutline.getRadius(), brushInnerOutline.getRadius()});
        brushOuterOutline.setRadius(radius);
        brushOuterOutline.setOrigin({brushOuterOutline.getRadius(), brushOuterOutline.getRadius()});
    }
}

void glxy::ImageEditor::OptionSetupTransformImage()
{
    OptionFinish();
    transformImage = true;
    if (!tempImage)
        tempImage = make_unique<Image>();

    if (!chunkManager.hasSelectionLayer())
    {
        tempImage->resize(getSize(), Color::Transparent);
        chunkManager.CopyImage(*tempImage, Vector2u(), IntRect(), ImageCopyType::Color, _layerPicker.getLayerIDSelected());

        Image empty;
        empty.resize(getSize(), Color::Transparent);
        chunkManager.PasteImage(empty, Vector2u(), IntRect(), ImageCopyType::Color, _layerPicker.getLayerIDSelected());
    }
    else
    {
        pixelSelect.createTempSelectionFromSelected();

        assert(!pixelSelect.getFinalSelectionBounds().expired());
        const IntRect bounds = *pixelSelect.getFinalSelectionBounds().lock();
        tempImage->resize(Vector2u(bounds.size), Color::Transparent);
        for (int32_t x = 0; x < bounds.size.x; x++)
            for (int32_t y = 0; y < bounds.size.y; y++)
            {
                if (!pixelSelect.withinTempBounds(Vector2u(x, y)))
                    continue;
                const Vector2i pos = Vector2i(x, y) + bounds.position;
                if (pos.x < 0 || pos.y < 0 || pos.x >= getSize().x || pos.y >= getSize().y)
                    continue;
                tempImage->setPixel(Vector2u(x, y), chunkManager.getPixelColor(Vector2u(pos)));
                chunkManager.setPixelColor(Vector2u(pos), Color::Transparent);
            }
    }
}

void glxy::ImageEditor::OptionCropSelection()
{
    OptionFinish();

    assert(!pixelSelect.getFinalSelectionBounds().expired());
    const IntRect final = *pixelSelect.getFinalSelectionBounds().lock();

    vector<Image> temp;
    temp.resize(_layerPicker.getLayerCount());
    Vector2u dummy;
    for (int16_t i = 0; i < _layerPicker.getLayerCount(); i++)
        pixelSelect.copySelectedPixels(temp.at(i), dummy, i);

    AllocateChunks(Vector2u(final.size));
    for (int16_t i = 0; i < _layerPicker.getLayerCount(); i++)
        chunkManager.addLayer(0);

    for (int16_t i = 0; i < _layerPicker.getLayerCount(); i++)
    {
        chunkManager.PasteImage(temp.at(i), Vector2u(), IntRect(), ImageCopyType::Color, i);
        updateLayerPreview(i);
    }
    chunkManager.RenderChunkArea();
    ClampView();
    gridLines.manualChange = true;
    rulerUI.manualChange = true;
    unsavedChanges = true;
}
