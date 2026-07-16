#pragma once
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <iostream>
#include <string>
#include "Namespace.hpp"
#include "Color32f.hpp"

using namespace sf;

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

static int32_t TextCallback(ImGuiInputTextCallbackData* data)
{
    switch (data->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackResize:
        {
            string* str = static_cast<string*>(data->UserData);
            str->resize(data->BufTextLen);
            data->Buf = str->data();
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