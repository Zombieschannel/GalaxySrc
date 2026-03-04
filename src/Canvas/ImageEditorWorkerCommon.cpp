#include "ImageEditorWorkerCommon.hpp"

bool glxy::ImageEditorWorkerCommon::getBucketFill() const
{
    return bucketFill;
}

bool glxy::ImageEditorWorkerCommon::getWandFill() const
{
    return wandFill;
}

bool glxy::ImageEditorWorkerCommon::getGradientDraw() const
{
    return gradientDraw;
}

bool glxy::ImageEditorWorkerCommon::getGradientSetup() const
{
    return gradientSetup;
}

bool glxy::ImageEditorWorkerCommon::getMoveSelection() const
{
    return moveSelection;
}

IntRect glxy::ImageEditorWorkerCommon::getNewMoveSelectionArea() const
{
    return newMoveSelectionArea;
}

bool glxy::ImageEditorWorkerCommon::hasUnfinishedChanges() const
{
    return bucketFill || wandFill || gradientDraw || moveSelection;
}
