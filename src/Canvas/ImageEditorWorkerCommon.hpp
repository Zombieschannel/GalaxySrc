#pragma once
#include <mutex>
#include <SFML/Graphics.hpp>
#include "../Namespace.hpp"
#include "imgui.h"

using namespace sf;
namespace glxy
{
    class ImageEditorWorkerCommon
    {
    friend class CanvasWorker;
    protected:

        bool bucketFill = false;
        bool wandFill = false;
        bool gradientDraw = false;
        bool gradientSetup = false;

        bool moveSelection = false;
        IntRect newMoveSelectionArea;

    public:
        mutable std::mutex mtxEditorWorkerCommon;

        bool getBucketFill() const;
        bool getWandFill() const;

        bool getGradientDraw() const;
        bool getGradientSetup() const;

        bool getMoveSelection() const;
        IntRect getNewMoveSelectionArea() const;

        bool hasUnfinishedChanges() const;
    };
}