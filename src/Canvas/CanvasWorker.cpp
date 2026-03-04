#include "CanvasWorker.hpp"

glxy::CanvasWork::CanvasWork()
{
}

void glxy::CanvasWork::setEditorID(const EditorID ID)
{
    this->editorID = ID;
}

EditorID glxy::CanvasWork::getEditorID() const
{
    return editorID;
}

glxy::CanvasExtendedWork::CanvasExtendedWork()
{

}

glxy::CanvasExtendedWork::CanvasExtendedWork(const CanvasWork& other)
{
    this->work = other.work;
    this->editorID = other.editorID;
}

bool glxy::CanvasWorker::hasWork()
{
    lock_guard lock(get().mtxWork);
    return !get().workTodo.empty();
}

float glxy::CanvasWorker::getWorkAmount()
{
    lock_guard lock(get().mtxWork);
    return get().workTodo.size() - get().workDone;
}

const glxy::ChunkManager& glxy::CanvasWorker::getChunkManager(const EditorID editorID)
{
    lock_guard lock(get().mtxEditor);
    return get().imageEditorWorker.at(editorID)->chunkManager;
}

const glxy::ImageEditorWorkerCommon& glxy::CanvasWorker::getCommon(const EditorID editorID)
{
    lock_guard lock(get().mtxEditor);
    return *get().imageEditorWorker.at(editorID);
}

void glxy::CanvasWorker::waitWork()
{
    while (hasWork())
        sleep(milliseconds(10));
}

void glxy::CanvasWorker::ExitThread()
{
    AddWork(CanvasWork::ExitThread{}, 0);
    if (get().worker->joinable())
        get().worker->join();
    get().worker.reset();
}

void glxy::CanvasWorker::LockAddingNewWork(const bool state)
{
    lock_guard lock(get().mtxWork);
    get().lockWork = state;
}

void glxy::CanvasWorker::AddWork(const CanvasWork& work, const EditorID editorID)
{
    lock_guard lock(get().mtxWork);
    if (get().lockWork)
        return;
    get().workTodo.push_back(work);
    get().workTodo.back().setEditorID(editorID);
    {
        std::lock_guard lockFlag(get().mtxWorkFlag);
        get().newWorkFlag = true;
    }
    get().newWorkCV.notify_one();
}

void glxy::CanvasWorker::AddEditor()
{
    lock_guard lock(get().mtxEditor);
    get().imageEditorWorker.emplace_back(std::make_shared<ImageEditorWorker>());
}

void glxy::CanvasWorker::RemoveEditor(const EditorID editorID)
{
    lock_guard lock(get().mtxEditor);
    get().imageEditorWorker.erase(get().imageEditorWorker.begin() + editorID);
}

void glxy::CanvasWorker::Setup()
{
    get().worker = make_unique<std::thread>(&CanvasWorker::Main, &get());
}

void glxy::CanvasWorker::Main()
{
    CanvasExtendedWork work;
    bool sameWork = false;
    while (true)
    {
        std::unique_lock workLock(mtxWork);
        if (!workTodo.empty())
        {
            do
            {
                if (workTodo.size() == 1)
                    break;
                if (workTodo.front().get<CanvasWork::ExitThread>())
                    return;
                if (workTodo.front().get<CanvasWork::Adjustment>() && workTodo.at(1).get<CanvasWork::Adjustment>() ||
                    workTodo.front().get<CanvasWork::Effect>() && workTodo.at(1).get<CanvasWork::Effect>() ||
                    workTodo.front().get<CanvasWork::BucketFill>() && workTodo.at(1).get<CanvasWork::BucketFill>() ||
                    workTodo.front().get<CanvasWork::WandFill>() && workTodo.at(1).get<CanvasWork::WandFill>() ||
                    workTodo.front().get<CanvasWork::GradientPixels>() && workTodo.at(1).get<CanvasWork::GradientPixels>() ||
                    workTodo.front().get<CanvasWork::BeginSelect>() && workTodo.at(1).get<CanvasWork::BeginSelect>() ||
                    workTodo.front().get<CanvasWork::EndSelect>() && workTodo.at(1).get<CanvasWork::EndSelect>() ||
                    workTodo.front().get<CanvasWork::MovePixels>() && workTodo.at(1).get<CanvasWork::MovePixels>() ||
                    workTodo.front().get<CanvasWork::TransformImage>() && workTodo.at(1).get<CanvasWork::TransformImage>())
                {
                    workTodo.erase(workTodo.begin());
                    sameWork = false;
                    continue;
                }
                if (workTodo.size() > 5 &&
                    (workTodo.front().get<CanvasWork::BrushPixels>() && workTodo.at(1).get<CanvasWork::BrushPixels>() ||
                    workTodo.front().get<CanvasWork::EraserPixels>() && workTodo.at(1).get<CanvasWork::EraserPixels>()))
                {
                    const array<int8_t, 3> delCount = { 5, 10, 15 };
                    int8_t i = 0;
                    for (i = delCount.size() - 1; i >= 0; i--)
                        if (delCount.at(i) <= workTodo.size())
                            break;

                    for (; i >= 0; i--)
                    {
                        if (workTodo.front().get<CanvasWork::BrushPixels>())
                        {
                            const auto* src = workTodo.front().get<CanvasWork::BrushPixels>();
                            auto* dst = workTodo.at(1).modify<CanvasWork::BrushPixels>();
                            if (!src || !dst)
                                break;
                            dst->end = src->end;
                        }
                        else if (workTodo.front().get<CanvasWork::EraserPixels>())
                        {
                            const auto* src = workTodo.front().get<CanvasWork::EraserPixels>();
                            auto* dst = workTodo.at(1).modify<CanvasWork::EraserPixels>();
                            if (!src || !dst)
                                break;
                            dst->end = src->end;
                        }
                        workTodo.erase(workTodo.begin());
                    }
                    sameWork = false;
                    break;
                }
                if (workTodo.front().get<CanvasWork::Adjustment>() || workTodo.front().get<CanvasWork::Effect>())
                {
                    for (int32_t i = 0; i < workTodo.size(); i++)
                    {
                        if (workTodo.at(i).get<CanvasWork::CancelAdjustmentOrEffect>())
                        {
                            for (int32_t j = 0; j < i; j++)
                                workTodo.erase(workTodo.begin());
                            sameWork = false;
                            break;
                        }
                    }
                }
                break;
            } while (true);
            if (!sameWork)
            {
                workDone = 0.f;
                work = workTodo.front();
            }
        }
        else
        {
            workLock.unlock();
            std::unique_lock lock(mtxWorkFlag);
            newWorkCV.wait(lock, [&]{ return newWorkFlag; });
            newWorkFlag = false;
            continue;
        }
        workLock.unlock();

        sameWork = false;

        {
            lock_guard editorLock(mtxEditor);
            if (work.get<CanvasWork::ExitThread>())
                break;

            auto& ie = imageEditorWorker.at(work.getEditorID());
            RenderWorker::setEditorID(work.getEditorID());

            if (const auto v = work.get<CanvasWork::CreateLayer>())
                ie->OptionCreateLayer(v->layerID);
            else if (const auto v = work.get<CanvasWork::DeleteLayer>())
                ie->OptionDeleteLayer(v->layerID);
            else if (const auto v = work.get<CanvasWork::DuplicateLayer>())
                ie->OptionDuplicateLayer(v->layerID);
            else if (const auto v = work.get<CanvasWork::MoveLayerUp>())
                ie->OptionMoveLayerUp(v->layerID);
            else if (const auto v = work.get<CanvasWork::MoveLayerDown>())
                ie->OptionMoveLayerDown(v->layerID);
            else if (const auto v = work.get<CanvasWork::MergeLayerDown>())
                ie->OptionMergeLayerDown(v->layerID);
            else if (const auto v = work.get<CanvasWork::FlipLayerHorizontal>())
                ie->OptionFlipLayerHorizontal(v->layerID);
            else if (const auto v = work.get<CanvasWork::FlipLayerVertical>())
                ie->OptionFlipLayerVertical(v->layerID);
            else if (work.get<CanvasWork::FlipImageHorizontal>())
                ie->OptionFlipImageHorizontal();
            else if (work.get<CanvasWork::FlipImageVertical>())
                ie->OptionFlipImageVertical();
            else if (work.get<CanvasWork::Rotate90CW>())
                ie->OptionRotate90CW();
            else if (work.get<CanvasWork::Rotate90CCW>())
                ie->OptionRotate90CCW();
            else if (work.get<CanvasWork::Rotate180>())
                ie->OptionRotate180();
            else if (work.get<CanvasWork::Cancel>())
                ie->OptionCancel();
            else if (work.get<CanvasWork::Finish>())
                ie->OptionFinish();
            else if (work.get<CanvasWork::FinishBrush>())
                ie->MergeColorTempLayer(ie->workingLayer, BlendAlpha);
            else if (work.get<CanvasWork::FinishEraser>())
                ie->MergeColorTempLayer(ie->workingLayer, BlendMultiply);
            else if (const auto v = work.get<CanvasWork::ImageEmpty>())
                ie->Empty(v->resolution, v->color);
            else if (const auto v = work.get<CanvasWork::ImageOpen>())
                ie->Open(v->path);
            else if (const auto v = work.get<CanvasWork::ClipboardCopy>())
                ie->OptionCopyToClipboard(*v->image, *v->location, v->layerID);
            else if (const auto v = work.get<CanvasWork::ClipboardPaste>())
            {
                ie->workingLayer = v->layerID;
                ie->OptionPasteFromClipboard(*v->image, *v->location);
                ie->MovePixels(v->transform);
            }
            else if (work.get<CanvasWork::SelectAll>())
                ie->OptionSelectAll();
            else if (work.get<CanvasWork::DeselectAll>())
                ie->OptionDeselectAll();
            else if (const auto v = work.get<CanvasWork::DeleteSelected>())
                ie->OptionDeleteSelected(v->layerID);
            else if (work.get<CanvasWork::CropSelection>())
                ie->OptionCropSelection();
            else if (const auto v = work.get<CanvasWork::TransformImageSetup>())
            {
                ie->workingLayer = v->layerID;
                ie->OptionSetupTransformImage(v->layerID);
            }
            else if (const auto v = work.get<CanvasWork::Adjustment>())
            {
                ie->Adjustment(v->adjustment, v->layerID, v->data.get(), work.chunkID);
                work.chunkID++;
                if (work.chunkID < ie->chunkManager.getChunkCount().x * ie->chunkManager.getChunkCount().y)
                    sameWork = true;
                workDone = static_cast<float>(work.chunkID) / (ie->chunkManager.getChunkCount().x * ie->chunkManager.getChunkCount().y);
            }
            else if (const auto v = work.get<CanvasWork::Effect>())
            {
                ie->Effect(v->effect, v->layerID, v->data.get(), work.chunkID);
                work.chunkID++;
                if (work.chunkID < ie->chunkManager.getChunkCount().x * ie->chunkManager.getChunkCount().y)
                    sameWork = true;
                workDone = static_cast<float>(work.chunkID) / (ie->chunkManager.getChunkCount().x * ie->chunkManager.getChunkCount().y);
            }
            else if (work.get<CanvasWork::CancelAdjustmentOrEffect>())
                ie->CancelAdjustmentOrEffect();
            else if (const auto v = work.get<CanvasWork::FinishAdjustmentOrEffect>())
                ie->MergeColorTempLayer(v->layerID, BlendAlpha);
            else if (const auto v = work.get<CanvasWork::RescaleCanvas>())
                ie->RescaleCanvas(v->size, v->method);
            else if (const auto v = work.get<CanvasWork::ResizeCanvas>())
                ie->ResizeCanvas(v->size, v->pivot);
            else if (const auto v = work.get<CanvasWork::TransformImage>())
                ie->OptionTransformImage(v->pos, v->rot, v->scale, v->origin, v->tile);
            else if (const auto v = work.get<CanvasWork::DrawPixels>())
            {
                ie->workingLayer = v->layerID;
                ie->DrawPixels(v->start, v->end, v->color, v->layerID);
            }
            else if (const auto v = work.get<CanvasWork::BrushPixels>())
            {
                ie->workingLayer = v->layerID;
                ie->BrushPixels(v->start, v->end, v->radius, v->color, v->layerID, false);
            }
            else if (const auto v = work.get<CanvasWork::EraserPixels>())
            {
                ie->workingLayer = v->layerID;
                ie->BrushPixels(v->start, v->end, v->radius, Color::Transparent, v->layerID, true);
            }
            else if (const auto v = work.get<CanvasWork::GradientPixels>())
            {
                ie->workingLayer = v->layerID;
                ie->GradientPixels(v->start, v->end, v->color1, v->color2);
            }
            else if (const auto v = work.get<CanvasWork::BeginSelect>())
                ie->BeginSelect(v->pos, v->selectMode, v->type, v->keepAspect);
            else if (const auto v = work.get<CanvasWork::EndSelect>())
                ie->EndSelect(v->pos);
            else if (const auto v = work.get<CanvasWork::BucketFill>())
            {
                {
                    lock_guard lock(ie->mtxEditorWorkerCommon);
                    ie->bucketFill = true;
                }
                ie->workingLayer = v->layerID;
                ie->FillPixels(v->pos, v->col, v->tol, v->layerID);
            }
            else if (const auto v = work.get<CanvasWork::WandFill>())
            {
                {
                    lock_guard lock(ie->mtxEditorWorkerCommon);
                    ie->wandFill = true;
                }
                ie->chunkManager.wandSelect(v->pos, v->tol, v->layerID);
            }
            else if (const auto v = work.get<CanvasWork::LayerPropertyChanged>())
            {
                ie->chunkManager.setLayerBlendMode(v->layerID, v->blendMode);
                ie->chunkManager.setLayerEnabled(v->layerID, v->enabled);
                ie->chunkManager.setLayerTransparency(v->layerID, v->transparency);
                ie->chunkManager.InvalidateColorTextures();
            }
            else if (work.get<CanvasWork::GradientSetup>())
            {
                lock_guard lock(ie->mtxEditorWorkerCommon);
                ie->gradientDraw = true;
                ie->gradientSetup = true;
            }
            else if (work.get<CanvasWork::GradientFinishSetup>())
            {
                lock_guard lock(ie->mtxEditorWorkerCommon);
                ie->gradientSetup = false;
            }
            else if (const auto v = work.get<CanvasWork::MovePixels>())
            {
                ie->mtxEditorWorkerCommon.lock();
                const bool moveSelection = ie->moveSelection;
                ie->mtxEditorWorkerCommon.unlock();
                ie->workingLayer = v->layerID;

                if (!moveSelection)
                {
                    assert(!ie->chunkManager.getFinalSelectionBounds().expired());
                    const IntRect bounds = *ie->chunkManager.getFinalSelectionBounds().lock();
                    ie->SetupMovePixels();
                    lock_guard lock(ie->mtxEditorWorkerCommon);
                    ie->newMoveSelectionArea = bounds;
                    ie->moveSelection = true;
                }

                ie->MovePixels(v->transform);
            }
        }

        if (sameWork)
            continue;

        workLock.lock();
        workTodo.erase(workTodo.begin());
    }
}
