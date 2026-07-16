#pragma once
#include "../Namespace.hpp"
#include "ImageEditorWorker.hpp"
#include <mutex>
#include <thread>
#include <condition_variable>

using namespace sf;
namespace glxy
{
    class CanvasWork
    {
    friend class CanvasExtendedWork;
    public:
        struct CreateLayer { LayerID layerID; };
        struct DeleteLayer { LayerID layerID; };
        struct DuplicateLayer { LayerID layerID; };
        struct MoveLayerUp { LayerID layerID; };
        struct MoveLayerDown { LayerID layerID; };
        struct MergeLayerDown { LayerID layerID; };
        struct FlipLayerHorizontal { LayerID layerID; };
        struct FlipLayerVertical { LayerID layerID; };
        struct FlipImageHorizontal {};
        struct FlipImageVertical {};
        struct Rotate90CW {};
        struct Rotate90CCW {};
        struct Rotate180 {};
        struct Cancel {};
        struct Finish {};
        struct FinishPencil {};
        struct FinishBrush {};
        struct FinishEraser {};
        struct ImageEmpty { Vector2u resolution; Color color; };
        struct ImageOpen { filesystem::path path; };
        struct ClipboardCopy { Image* image; Vector2u* location; Image* selection; LayerID layerID; };
        struct ClipboardPaste { Image* image; Vector2u* location; Image* selection; LayerID layerID; Transform transform; bool smooth; };
        struct SelectArea { FloatRect area; };
        struct DeselectAll {};
        struct DeleteSelected { LayerID layerID; };
        struct CropSelection {};
        struct TransformImageSetup { LayerID layerID; };
        struct CircularShiftSetup { LayerID layerID; };
        struct Adjustment { Adjustments adjustment; LayerID layerID; shared_ptr<AdjustmentData> data; };
        struct Effect { Effects effect; LayerID layerID; shared_ptr<EffectData> data; };
        struct CancelAdjustmentOrEffect {};
        struct FinishAdjustmentOrEffect { LayerID layerID; };
        struct RescaleCanvas { Vector2u size; RescaleMethod method; };
        struct ResizeCanvas { Vector2u size; Pivot pivot; };
        struct TransformImage { Vector2f pos; float rot; Vector2f scale; Vector2f origin; int16_t repeat; bool smooth; };
        struct CircularShift { int32_t shift; bool horizontal; int32_t groupSize; };
        struct PencilPixels { Vector2f start, end; Color color; LayerID layerID; };
        struct BrushPixels { Vector2f start, end; Color color; float radius; LayerID layerID; };
        struct EraserPixels { Vector2f start, end; float radius; LayerID layerID; };
        struct ColorSwapPixels { Vector2f start, end; Color color1, color2; float radius; int8_t tol; LayerID layerID; };
        struct GradientPixels { Vector2f start, end; Color color1, color2; LayerID layerID; };
        struct ShapePixels { shared_ptr<ConvexShape> shape; LayerID layerID; };
        struct TextPixels { shared_ptr<Font> font; shared_ptr<Text> text; FloatRect globalBounds; LayerID layerID; };
        struct BeginSelect { Vector2f pos; SelectMode selectMode; ShapeSelectType type; bool keepAspect; };
        struct EndSelect { Vector2f pos; };
        struct BucketFill { Vector2i pos; Color col; int8_t tol; LayerID layerID; };
        struct WandFill { Vector2u pos; int8_t tol; LayerID layerID; };
        struct LayerPropertyChanged { LayerID layerID; bool enabled; uint8_t transparency; uint8_t blendMode; };
        struct GradientSetup {};
        struct GradientFinishSetup {};
        struct MovePixels { Transform transform; Vector2f scale; LayerID layerID; bool smooth; };
        struct MoveSelection { Transform transform; Vector2f scale; LayerID layerID; };
        struct ExitThread {};

        CanvasWork();

        template<typename T>
        CanvasWork(const T& work);

        template<typename T>
        const T* get();

        template<typename T>
        T* modify();

        void setEditorID(EditorID ID);
        EditorID getEditorID() const;

    protected:
        std::variant<
            CreateLayer,
            DeleteLayer,
            DuplicateLayer,
            MoveLayerUp,
            MoveLayerDown,
            MergeLayerDown,
            FlipLayerHorizontal,
            FlipLayerVertical,
            FlipImageHorizontal,
            FlipImageVertical,
            Rotate90CW,
            Rotate90CCW,
            Rotate180,
            Cancel,
            Finish,
            FinishPencil,
            FinishBrush,
            FinishEraser,
            ImageEmpty,
            ImageOpen,
            ClipboardCopy,
            ClipboardPaste,
            SelectArea,
            DeselectAll,
            DeleteSelected,
            CropSelection,
            TransformImageSetup,
            CircularShiftSetup,
            Adjustment,
            Effect,
            CancelAdjustmentOrEffect,
            FinishAdjustmentOrEffect,
            RescaleCanvas,
            ResizeCanvas,
            TransformImage,
            CircularShift,
            PencilPixels,
            BrushPixels,
            EraserPixels,
            ColorSwapPixels,
            GradientPixels,
            ShapePixels,
            TextPixels,
            BeginSelect,
            EndSelect,
            BucketFill,
            WandFill,
            LayerPropertyChanged,
            GradientSetup,
            GradientFinishSetup,
            MovePixels,
            MoveSelection,
            ExitThread> work;
        EditorID editorID = 0;
    };

    template <typename T>
    CanvasWork::CanvasWork(const T& work)
    {
        this->work = work;
    }

    template <typename T>
    const T* CanvasWork::get()
    {
        return std::get_if<T>(&work);
    }

    template <typename T>
    T* CanvasWork::modify()
    {
        return std::get_if<T>(&work);
    }

    class CanvasExtendedWork : public CanvasWork
    {
    public:
        int32_t chunkID = 0;

        CanvasExtendedWork();

        template<typename T>
        CanvasExtendedWork(const T& work);

        CanvasExtendedWork(const CanvasWork& other);
    };

    template <typename T>
    CanvasExtendedWork::CanvasExtendedWork(const T& work)
    {
        this->work = work;
    }

    class CanvasWorker
    {
        bool lockWork = false;
        bool newWorkFlag = false;
        float workDone = 0;
        vector<shared_ptr<ImageEditorWorker>> imageEditorWorker;
        std::unique_ptr<std::thread> worker;

        mutable std::mutex mtxWork;
        mutable std::mutex mtxEditor;
        std::mutex mtxWorkFlag;
        std::condition_variable newWorkCV;
        vector<CanvasWork> workTodo;
        static CanvasWorker& get()
        {
            static CanvasWorker instance;
            return instance;
        }
        void Main();
    public:
        static bool hasWork();
        static float getWorkAmount();
        static const ChunkManager& getChunkManager(EditorID editorID);
        static const ImageEditorWorkerCommon& getCommon(EditorID editorID);
        static void waitWork();
        static void ExitThread();
        static void LockAddingNewWork(bool lock);
        static void AddWork(const CanvasWork& work, EditorID editorID);
        static void AddEditor(bool infinite);
        static void RemoveEditor(EditorID editorID);
        static void Setup();
    };
}
