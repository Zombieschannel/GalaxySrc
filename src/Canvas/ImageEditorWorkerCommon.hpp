#pragma once
#include <mutex>
#include <SFML/Graphics.hpp>
#include "../Namespace.hpp"

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

        bool shapeDraw = false;
        IntRect prevShapeRenderArea;

        bool textDraw = false;
        IntRect prevTextRenderArea;

        bool moveSelected = false;
        bool moveSelection = false;
        IntRect newMoveSelectArea;


    public:
        mutable std::mutex mtxEditorWorkerCommon;

        bool getBucketFill() const;
        bool getWandFill() const;
        bool getShapeDraw() const;
        bool getTextDraw() const;

        bool getGradientDraw() const;
        bool getGradientSetup() const;

        bool getMoveSelected() const;
        bool getMoveSelection() const;
        IntRect getNewMoveSelectArea() const;


        bool hasUnfinishedChanges() const;
    };
}