#include "RenderWorker.hpp"

glxy::RenderWork::RenderWork()
{
}

void glxy::RenderWork::setEditorID(const EditorID ID)
{
    editorID = ID;
}

EditorID glxy::RenderWork::getEditorID() const
{
    return editorID;
}

bool glxy::RenderWorker::hasWork()
{
    lock_guard lock(get().mtxWork);
    return !get().workTodo.empty();
}

int32_t glxy::RenderWorker::getWorkAmount()
{
    lock_guard lock(get().mtxWork);
    return get().workTodo.size();
}

void glxy::RenderWorker::waitWork()
{
    while (hasWork())
        sleep(milliseconds(10));
}

void glxy::RenderWorker::ExitThread()
{
    const int32_t workCount = getWorkAmount();
    for (int32_t i = 0; i < workCount; i++)
    {
        AddResult(RenderResult::ExitThread{});
        RemoveWorkTodo();
    }
}

void glxy::RenderWorker::setEditorID(const EditorID editorID)
{
    get().editorID = editorID;
}

void glxy::RenderWorker::AddWork(const RenderWork& work)
{
    lock_guard lock(get().mtxWork);
    get().workTodo.push_back(make_unique<RenderWork>(work));
    get().workTodo.back()->setEditorID(get().editorID);
}

void glxy::RenderWorker::AddResult(const RenderResult& result)
{
    lock_guard lock(get().mtxResult);
    get().result.push_back(make_unique<RenderResult>(result));
    get().resultReadyCV.notify_one();
}

glxy::RenderWorkBatch glxy::RenderWorker::getWorkTodo()
{
    lock_guard lock(get().mtxWork);
    RenderWorkBatch batch;

    for (int8_t i = 0; i < 5 && i < get().workTodo.size(); i++)
        batch.work.push_back(std::move(get().workTodo.at(i)));

    return batch;
}

unique_ptr<glxy::RenderResult> glxy::RenderWorker::getResult()
{
    std::unique_lock lock(get().mtxResult);
    get().resultReadyCV.wait(lock, [&]{ return !get().result.empty(); });
    if (get().result.front()->get<RenderResult::ExitThread>())
        return nullptr;
    return std::move(get().result.front());
}

void glxy::RenderWorker::RemoveWorkTodo()
{
    lock_guard lock(get().mtxWork);
    assert(!get().workTodo.front());
    get().workTodo.erase(get().workTodo.begin());
}

void glxy::RenderWorker::RemoveResult()
{
    lock_guard lock(get().mtxResult);
    assert(!get().result.front());
    get().result.erase(get().result.begin());
}
