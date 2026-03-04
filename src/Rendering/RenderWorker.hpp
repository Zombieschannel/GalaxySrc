#pragma once
#include <condition_variable>
#include <SFML/Graphics.hpp>
#include "../Namespace.hpp"
#include "../Func.hpp"
#include <mutex>
#include <variant>

using namespace sf;
namespace glxy
{
    class RenderWork
    {
    public:
        struct MergeLayers { LayerID lowerLayerID; LayerID upperLayerID; ChunkID chunkID; };
        struct MergeColorTempLayer { LayerID lowerLayerID; ChunkID chunkID; BlendMode blendMode; };
        struct RenderSelection { IntRect area; ShapeSelectType shapeSelectionType; ChunkID chunkID; bool additive; bool isFinal; };
        struct BrushDraw { Vector2f start, end; float radius; Color color; LayerID layerID; ChunkID chunkID; bool isEraser; };

        RenderWork();

        template<typename T>
        RenderWork(const T& work);

        template<typename T>
        const T* get();

        void setEditorID(EditorID ID);
        EditorID getEditorID() const;

    private:
        std::variant<
            MergeLayers,
            MergeColorTempLayer,
            RenderSelection,
            BrushDraw> work;
        EditorID editorID = 0;
    };
    template <typename T>
    RenderWork::RenderWork(const T& work)
    {
        this->work = work;
    }

    template <typename T>
    const T* RenderWork::get()
    {
        return std::get_if<T>(&work);
    }

    class RenderResult
    {
    public:
        struct Chunk { Image img; };
        struct ExitThread {};

        RenderResult();

        template<typename T>
        RenderResult(const T& work);

        template<typename T>
        const T* get();

    private:
        std::variant<
            Chunk,
            ExitThread> work;
    };
    template <typename T>
    RenderResult::RenderResult(const T& work)
    {
        this->work = work;
    }

    template <typename T>
    const T* RenderResult::get()
    {
        return std::get_if<T>(&work);
    }

    struct RenderWorkBatch
    {
        static const int8_t BatchMaxSize = 5;
        vector<unique_ptr<RenderWork>> work;
    };

    class RenderWorker
    {
        mutable std::mutex mtxWork;
        mutable std::mutex mtxResult;
        vector<unique_ptr<RenderWork>> workTodo;
        vector<unique_ptr<RenderResult>> result;
        EditorID editorID = 0;

        std::condition_variable resultReadyCV;

        static RenderWorker& get()
        {
            static RenderWorker instance;
            return instance;
        }
    public:
        static bool hasWork();
        static int32_t getWorkAmount();
        static void waitWork();
        static void ExitThread();
        static void setEditorID(EditorID editorID);
        static void AddWork(const RenderWork& work);
        static void AddResult(const RenderResult& result);
        static RenderWorkBatch getWorkTodo();
        static unique_ptr<RenderResult> getResult();
        static void RemoveWorkTodo();
        static void RemoveResult();
    };
}
