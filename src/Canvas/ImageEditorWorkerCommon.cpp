#include "ImageEditorWorkerCommon.hpp"

bool glxy::ImageEditorWorkerCommon::getBucketFill() const
{
    return bucketFill;
}

bool glxy::ImageEditorWorkerCommon::getWandFill() const
{
    return wandFill;
}

bool glxy::ImageEditorWorkerCommon::getShapeDraw() const
{
    return shapeDraw;
}

bool glxy::ImageEditorWorkerCommon::getTextDraw() const
{
    return textDraw;
}

bool glxy::ImageEditorWorkerCommon::getGradientDraw() const
{
    return gradientDraw;
}

bool glxy::ImageEditorWorkerCommon::getGradientSetup() const
{
    return gradientSetup;
}

bool glxy::ImageEditorWorkerCommon::getMoveSelected() const
{
    return moveSelected;
}

IntRect glxy::ImageEditorWorkerCommon::getNewMoveSelectArea() const
{
    return newMoveSelectArea;
}

bool glxy::ImageEditorWorkerCommon::getCircularShift() const
{
    return circularShift;
}

bool glxy::ImageEditorWorkerCommon::getMoveSelection() const
{
    return moveSelection;
}

bool glxy::ImageEditorWorkerCommon::hasUnfinishedChanges() const
{
    return bucketFill || wandFill || gradientDraw || moveSelected || moveSelection || shapeDraw || textDraw;
}
