#pragma once
#include <SFML/Graphics.hpp>
#include "inc/ZTB.hpp"
#include <imgui.h>
#include "Namespace.hpp"

#ifdef NDEBUG
#define validate(x) static_cast<void>(x)
#else
#define validate(x) assert(x)
#endif

using namespace sf;
static Vector2f getSFMLViewMousePos(const FloatRect& mainView, const View& view)
{
    Vector2f msPos = static_cast<Vector2f>(InputEvent::getMousePosition());
    msPos.x -= mainView.position.x;
    msPos.y -= mainView.position.y;
    msPos.x /= mainView.size.x;
    msPos.y /= mainView.size.y;
    const FloatRect viewRect = FloatRect({ view.getCenter().x - view.getSize().x / 2, view.getCenter().y - view.getSize().y / 2 }, { view.getSize().x, view.getSize().y });
    return Vector2f(viewRect.position.x, viewRect.position.y) + Vector2f(viewRect.size.x * msPos.x, viewRect.size.y * msPos.y);
}
static time_t to_time_t(filesystem::file_time_type tp)
{
	const auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(tp - filesystem::file_time_type::clock::now()
		+ chrono::system_clock::now());
	return chrono::system_clock::to_time_t(sctp);
}
static int TextCallback(ImGuiInputTextCallbackData* data)
{
    switch (data->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackResize:
    {
        string* str = static_cast<string*>(data->UserData);
        if (str->size() != data->BufTextLen)
        {
            str->resize(data->BufTextLen);
            data->Buf = &(*str)[0];
        }
    }
    break;
    case ImGuiInputTextFlags_CallbackCompletion:
    {
        string* str = static_cast<string*>(data->UserData);
        filesystem::path t = *str;
        vector<filesystem::path> filenames;
        if (filesystem::exists(t.parent_path()))
            for (auto& n : filesystem::directory_iterator(t.parent_path()))
                if (n.path().filename().string().find(t.filename().string()) == 0)
                    filenames.push_back(n);
        if (filenames.size() == 1)
            data->InsertChars(str->size(), filenames.front().filename().string().substr(t.filename().string().size()).c_str());
        else if (filenames.size() > 1)
        {
            string append;
            for (int32_t j = t.filename().string().size();; j++)
            {
                bool sameChar = true;
                for (int32_t i = 0; i < filenames.size() - 1; i++)
                {
                    if (filenames.at(i).filename().string().size() <= j || filenames.at(i).filename().string().size() <= j ||
                        filenames.at(i).filename().string().at(j) != filenames.at(i + 1).filename().string().at(j))
                    {
                        sameChar = false;
                        break;
                    }
                }
                if (sameChar)
                    append += filenames.front().filename().string().at(j);
                else
                    break;
            }
            if (!append.empty())
                data->InsertChars(str->size(), append.c_str());
        }
    }
    break;
    }
    return 0;
}
static void ToolTip(const string& text)
{
    ImGui::SameLine();
    ImGui::TextColored(Color(192, 192, 192, 128), "[?]");
    ImGui::SetNextWindowSize(Vector2f(400, 0));
    if (ImGui::BeginItemTooltip())
    {
        ImGui::TextWrapped("%s", text.c_str());
        ImGui::EndTooltip();
    }
}

template<typename T>
Rect<T> getUnion(const Rect<T>* rect1, const Rect<T>* rect2)
{
    if (rect1 && rect2)
    {
        Vector2i minPos, maxPos;
        minPos.x = min(rect1->position.x, rect2->position.x);
        minPos.y = min(rect1->position.y, rect2->position.y);
        maxPos.x = max(rect1->position.x + rect1->size.x, rect2->position.x + rect2->size.x);
        maxPos.y = max(rect1->position.y + rect1->size.y, rect2->position.y + rect2->size.y);
        return Rect<T>(minPos, maxPos - minPos);
    }
    if (rect1)
        return *rect1;
    if (rect2)
        return *rect2;
    return Rect<T>();
}

static bool SameColor(const Color color1, const Color color2, const int8_t tolerance)
{
    if (tolerance == 0)
        return color1 == color2;
    if (tolerance == 100)
        return true;
    const ImVec4 diff = ImVec4(color1.r / 255.f - color2.r / 255.f, color1.g / 255.f - color2.g / 255.f, color1.b / 255.f - color2.b / 255.f, color1.a / 255.f - color2.a / 255.f);
    return (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z + diff.w * diff.w) / 4.f < tolerance / 100.f * tolerance / 100.f;
}

static Color LerpColor(const Color color1, const Color color2, float value)
{
    value = std::clamp(value, 0.f, 1.f);
    const ImVec4 diff = {color2.r / 255.f - color1.r / 255.f, color2.g / 255.f - color1.g / 255.f, color2.b / 255.f - color1.b / 255.f, color2.a / 255.f - color1.a / 255.f};
    return Color(color1.r + diff.x * 255 * value, color1.g + diff.y * 255 * value, color1.b + diff.z * 255 * value, color1.a + diff.w * 255 * value);
}