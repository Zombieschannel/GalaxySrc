#pragma once
#include <SFML/System.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include "../Namespace.hpp"
#include "../Config.hpp"
#include "../ImGuiFunc.hpp"
#include "Languages.hpp"
#include "ZTB.hpp"
#include <stack>
namespace stb
{
    #include <stb_image_resize2.h>
    #include <stb_image_write.h>
}
using namespace sf;
namespace ExplorerFlags
{
    constexpr uint8_t Open = 0;
    constexpr uint8_t Save = 1;
    constexpr uint8_t PNG = 1 << 1;
    constexpr uint8_t JPG = 1 << 2;
    constexpr uint8_t BMP = 1 << 3;
    constexpr uint8_t TGA = 1 << 4;
    constexpr uint8_t QOI = 1 << 5;
}
class FileExplorer
{
    const array<pair<string, string>, 5> fileTypeExtensions = {
        pair("Portable Network Graphics", ".png"),
        pair("Joint Photographic Experts Group", ".jpg"),
        pair("Bitmap", ".bmp"),
        pair("Truevision Advanced Raster Graphics Adapter", ".tga"),
        pair("Quite OK Image Format", ".qoi"),
    };

    array<pair<string, string>, 7> defaultBookmarkedPaths = {
        pair("Home", "~"),
        pair("Desktop", "~/Desktop"),
        pair("Documents", "~/Documents"),
        pair("Music", "~/Music"),
        pair("Pictures", "~/Pictures"),
        pair("Videos", "~/Videos"),
        pair("Downloads", "~/Downloads"),
    };

    struct FileData
    {
        filesystem::path path;
        bool isDirectory;
        bool textureTooLarge;
        bool cannotRead;
        unique_ptr<Texture> texture;
    };

    std::stack<filesystem::path> undo;
    std::stack<filesystem::path> redo;

    filesystem::path filePathOpen;
    filesystem::path filePathSave;

    bool listView = true;
    uint8_t extensionID = 0;
    uint8_t filterID = 0xFE;

    string fileNameOpen;
    string fileNameSave;
    uint8_t lastFrameFlags = 0xFF;
    bool firstRefresh = true;
    bool doubleClickStart = false;
    string newFolderName;
    bool namingNewFolder = false;
    vector<FileData> fileDataOpen;
    vector<FileData> fileDataSave;

    uint32_t currentPathFolders = 0;
    uint32_t currentPathFiles = 0;
    int32_t filesTodo = 0;
    Texture icons;
    Texture iconsSmooth;

    static string DJB2(const std::string& input)
    {
        uint32_t result = 5381;
        for (const char n : input)
            result = ((result << 5) + result) + n;
        std::stringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0') << result;
        return ss.str();
    }
    static filesystem::path getHome()
    {
        static filesystem::path home;
        static bool once = false;
        if (once)
            return home;
#if defined(SFML_SYSTEM_WINDOWS)
        const char* homeDir = getenv("USERPROFILE");
        if (homeDir)
            home = homeDir;
        else
            err() << "Failed to find home directory!" << endl;
#elif defined(SFML_SYSTEM_MACOS) || defined(SFML_SYSTEM_LINUX)
        const char* homeDir = getenv("HOME");
        if (homeDir)
            home = homeDir;
        else
            err() << "Failed to find home directory!" << endl;
#endif
        once = true;
        return home;
    }

public:
    FileExplorer()
    {
        const filesystem::path home = getHome();
        for (auto& n : defaultBookmarkedPaths)
        {
            n.second.replace(n.second.find("~"), 1, home.string());
#ifdef SFML_SYSTEM_WINDOWS
            if (n.second.find("/") != string::npos)
                n.second.replace(n.second.find("/"), 1, "\\");
#endif
            if (!filesystem::exists(n.second))
                n.second.clear();
        }
        filePathOpen = home;
        filePathSave = home;

        if (!filesystem::exists("Cache"))
            filesystem::create_directory("Cache");
        if (!filesystem::exists("Cache/Thumbnails"))
            filesystem::create_directory("Cache/Thumbnails");
    }
    void Load()
    {
        JSON file;
        if (file.loadFromFile("explorerConfig.json"))
        {
            string temp;
            file.loadValue("filePathOpen", temp);
            filePathOpen = temp;
            file.loadValue("filePathSave", temp);
            filePathSave = temp;
            file.loadValue("listView", listView);
            file.loadValue("extensionID", extensionID);
            file.loadValue("filterID", filterID);
        }
    }
    void Save()
    {
        JSON file;
        file.setValueStr("filePathOpen", filePathOpen.string());
        file.setValueStr("filePathSave", filePathSave.string());
        file.setValue("listView", listView);
        file.setValue("extensionID", extensionID);
        file.setValue("filterID", filterID);
        file.saveToFile("explorerConfig.json");
    }
    bool loadTexture(InputStream& stream)
    {
        Image img;
        if (!img.loadFromStream(stream))
            return false;
        if (!icons.loadFromImage(img))
            return false;
        if (!iconsSmooth.loadFromImage(img))
            return false;
        iconsSmooth.setSmooth(true);
        return true;
    }
    const filesystem::path& getFileOpenPath()
    {
        return filePathOpen;
    }
    const filesystem::path& getFileSavePath()
    {
        return filePathSave;
    }
    const string& getFilenameOpen()
    {
        return fileNameOpen;
    }
    const string& getFilenameSave()
    {
        return fileNameSave;
    }
    const string& getExtensionUsed()
    {
        return fileTypeExtensions.at(extensionID).second;
    }
    bool Widget(const uint8_t explorerFlags)
    {
        const glxy::Config& config = glxy::Config::get();
        const bool saveMode = explorerFlags & 1;
        const uint8_t thumbnailSize = 100;
        filesystem::path& filePath = saveMode ? filePathSave : filePathOpen;
        vector<FileData>& fileData = saveMode ? fileDataSave : fileDataOpen;
        string& fileName = saveMode ? fileNameSave : fileNameOpen;

        string fullFilePath = filePath.string();
        bool folderChanged = explorerFlags ^ lastFrameFlags;
        bool fileDoubleClicked = false;

        ImGui::BeginDisabled(undo.empty());
        if (ImGui::ArrowButton("Back", ImGuiDir_Left))
        {
            namingNewFolder = false;
            redo.push(filePath);
            filePath = undo.top();
            undo.pop();
            folderChanged = true;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(redo.empty());
        if (ImGui::ArrowButton("Forward", ImGuiDir_Right))
        {
            namingNewFolder = false;
            undo.push(filePath);
            filePath = redo.top();
            redo.pop();
            folderChanged = true;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::ArrowButton("Up", ImGuiDir_Up))
        {
            namingNewFolder = false;
            if (filePath != filePath.parent_path())
            {
                undo.push(filePath);
                while (!redo.empty()) redo.pop();
                filePath = filePath.parent_path();
            }
            folderChanged = true;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x / 3 * 2);
        if (ImGui::InputText("##FileExplorerFilepath", fullFilePath.data(), fullFilePath.capacity() + 1, ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_EnterReturnsTrue, TextCallback, &fullFilePath))
        {
            if (filesystem::exists(fullFilePath) && filesystem::is_directory(fullFilePath))
            {
                if (filePath != fullFilePath)
                {
                    undo.push(filePath);
                    while (!redo.empty()) redo.pop();
                    filePath = fullFilePath;
                }
                folderChanged = true;
            }
            else
                fullFilePath = filePath.string();
        }
        ImGui::SameLine();
        if (ImGui::ImageButton("List type", iconsSmooth.getNativeHandle(), Vector2f(16, 16) * config.GUIScale,
            Vector2f(0.3f + 0.1f * listView, 0.5f), Vector2f(0.4f + 0.1f * listView, 0.75f)))
        {
            listView = !listView;
        }
        ImGui::SameLine();

        if (ImGui::ImageButton("Refresh", iconsSmooth.getNativeHandle(), Vector2f(16, 16) * config.GUIScale,
            Vector2f(0.5f, 0.5f), Vector2f(0.6f, 0.75f)))
        {
            namingNewFolder = false;
            folderChanged = true;
        }

        ImGui::SameLine();

        if (ImGui::ImageButton("New folder", iconsSmooth.getNativeHandle(), Vector2f(16, 16) * config.GUIScale,
            Vector2f(0.8f, 0.5f), Vector2f(0.9f, 0.75f)))
        {
            namingNewFolder = !namingNewFolder;
        }

        if (currentPathFiles > 0 && filesTodo > 0 && !listView)
        {
            ImGui::SameLine();
            ImGui::ProgressBar(1 - static_cast<float>(filesTodo) / currentPathFiles);
        }

        const Vector2f availableArea = ImGui::GetContentRegionAvail();
        if (ImGui::BeginChild("FileExplorerSidebar", Vector2f(availableArea.x * 0.2f, availableArea.y)))
        {
            const float frameHeight = ImGui::GetFrameHeight();
            if (ImGui::BeginChild("FileExplorerBookmarks", Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 2.f * frameHeight)))
            {
                uint8_t ID = 0;
                for (const auto& n : defaultBookmarkedPaths)
                {
                    ID++;
                    if (n.second.empty())
                        continue;
                    if (ImGui::Selectable((n.first + "##FileExplorerBookmark" + to_string(ID)).c_str()))
                    {
                        if (filePath != n.second)
                        {
                            undo.push(filePath);
                            while (!redo.empty()) redo.pop();
                            filePath = n.second;
                        }
                        folderChanged = true;
                    }
                }
                ID++;
                if (ImGui::Selectable(("Galaxy##FileExplorerBookmark" + to_string(ID)).c_str()))
                {
                    if (filePath != filesystem::current_path())
                    {
                        undo.push(filePath);
                        while (!redo.empty()) redo.pop();
                        filePath = filesystem::current_path();
                    }
                    folderChanged = true;
                }
            }
            ImGui::EndChild();
            ImGui::Text("%s", ("Files"_S + ": " + to_string(currentPathFiles)).c_str());
            ImGui::Text("%s", ("Folders"_S + ": " + to_string(currentPathFolders)).c_str());
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        if (ImGui::BeginChild("FileExplorerMain", Vector2f(ImGui::GetContentRegionAvail().x, availableArea.y)))
        {
            if (namingNewFolder)
            {
                ImGui::Image(icons.getNativeHandle(), Vector2f(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()),
                    Vector2f(0.8f, 0.5f), Vector2f(0.9f, 0.75f));
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x / 3 * 2);
                ImGui::InputText("##FileExplorerNewFolder", newFolderName.data(), newFolderName.capacity() + 1, ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_AlwaysOverwrite, TextCallback, &newFolderName);
                ImGui::SameLine();
                if (ImGui::Button("Create"_C))
                {
                    namingNewFolder = false;
                    if (!filesystem::exists(filesystem::path(filePath) / newFolderName))
                    {
                        filesystem::create_directory(filesystem::path(filePath) / newFolderName);
                        folderChanged = true;
                    }
                }
            }
            if (ImGui::BeginChild("FileExplorerEntries", Vector2f(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 32 * config.GUIScale)))
            {
                const int8_t perRow = ImGui::GetContentRegionAvail().x / (thumbnailSize * config.GUIScale);
                uint8_t thumbnailCounter = 0;
                uint32_t ID = 0;
                for (const auto& n : fileData)
                {
                    ID++;
                    if (listView)
                    {
                        if (ID % 2 == 0)
                            ImGui::SameLine();
                        if (ImGui::Selectable((n.path.filename().string() + "##FileExplorerFile" + to_string(ID)).c_str(), false, (fileName == n.path.filename() ? ImGuiSelectableFlags_Highlight : ImGuiSelectableFlags_None) | ImGuiSelectableFlags_AllowDoubleClick,
                            Vector2f(ID % 2 ? ImGui::GetContentRegionAvail().x / 2 : ImGui::GetContentRegionAvail().x, 0)))
                        {
                            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                                doubleClickStart = true;
                            fileName = n.path.filename().string();
                        }
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, Color(255, 255, 255, 32));
                        ImGui::BeginChild(("FileExplorerFileChild" + to_string(ID)).c_str(),
                            Vector2f(thumbnailSize, thumbnailSize * 1.2f) * config.GUIScale +
                            Vector2f(ImGui::GetStyle().ChildBorderSize * 2, ImGui::GetStyle().FramePadding.y * 2), false,
                            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                        const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
                        const bool selected = ImGui::IsMouseDown(ImGuiMouseButton_Left);
                        FloatRect t;
                        Texture* tex = &icons;
                        if (n.isDirectory)
                            t = FloatRect(Vector2f(0.7f, 0.5f), Vector2f(0.1f, 0.25f));
                        else
                        {
                            if (!n.textureTooLarge && n.texture)
                            {
                                t = FloatRect(Vector2f(0, 0), Vector2f(1, 1));
                                tex = n.texture.get();
                            }
                            else
                                t = FloatRect(Vector2f(0.6f, 0.5f), Vector2f(0.1f, 0.25f));
                        }
                        Color c;
                        if (hovered && selected)
                            c = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
                        else if (hovered || fileName == n.path.filename())
                            c = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
                        else
                            c = Color(255, 255, 255, 16);
                        ImGui::Image(tex->getNativeHandle(), Vector2f(thumbnailSize, thumbnailSize) * config.GUIScale,
                            t.position, t.position + t.size, Color::White, c);

                        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, Vector2f(0.5f, 0.5f));
                        ImGui::PushStyleColor(ImGuiCol_TextDisabled, Color(255, 0, 255));
                        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.9f);
                        ImGui::Selectable((n.path.filename().string() + "##FileExplorerFile" + to_string(ID)).c_str(), hovered || fileName == n.path.filename(), ImGuiSelectableFlags_Disabled, ImGui::GetContentRegionAvail());
                        ImGui::PopStyleVar();
                        ImGui::PopStyleColor();
                        ImGui::PopStyleVar();
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                        if (hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                            fileName = n.path.filename().string();
                        if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                            doubleClickStart = true;
                        if (ImGui::GetWindowSize().x > thumbnailSize && ImGui::GetWindowSize().y > 0)
                        {
                            if (thumbnailCounter < perRow - 1)
                            {
                                ImGui::SameLine();
                                thumbnailCounter++;
                            }
                            else
                                thumbnailCounter = 0;
                        }
                    }
                }
            }
            ImGui::EndChild();
            if (doubleClickStart && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                if (filesystem::is_directory(filePath / fileName))
                {
                    undo.push(filePath);
                    while (!redo.empty()) redo.pop();
                    filePath = filePath / fileName;
                    folderChanged = true;
                }
                else
                    fileDoubleClicked = true;
                doubleClickStart = false;
            }
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x / 5 * 3);
            ImGui::InputText("##FileExplorerFilename", fileName.data(), fileName.capacity() + 1,
                !saveMode ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_AlwaysOverwrite, TextCallback, &fileName);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::BeginCombo("##FileExplorerFilter",
                    saveMode ? fileTypeExtensions.at(extensionID).second.c_str() :
                    filterID == 0xFF ? "All file types"_C : filterID == 0xFE ? "All compatible types"_C : fileTypeExtensions.at(filterID).second.c_str()))
            {
                for (uint8_t i = 0; i < fileTypeExtensions.size(); i++)
                {
                    const auto& n = fileTypeExtensions.at(i);
                    if (ImGui::Selectable((n.second + " (" + n.first + ")").c_str()))
                    {
                        if (saveMode)
                            extensionID = i;
                        else
                            filterID = i;
                        if (!saveMode)
                            folderChanged = true;
                    }
                }
                if (!saveMode)
                {
                    if (ImGui::Selectable("All compatible types"_C))
                    {
                        filterID = 0xFE;
                        folderChanged = true;
                    }
                    if (ImGui::Selectable("All file types"_C))
                    {
                        filterID = 0xFF;
                        folderChanged = true;
                    }
                }
                ImGui::EndCombo();
            }
        }
        ImGui::EndChild();
        if (folderChanged)
        {
            currentPathFiles = 0;
            currentPathFolders = 0;
            filesTodo = 0;
            fileName.clear();
            fileData.clear();
            std::error_code ec;
            for (const auto& n : filesystem::directory_iterator(filePath, ec))
            {
                if (ec)
                    continue;
                if (!filesystem::is_regular_file(n) && !filesystem::is_directory(n))
                    continue;
                if (!saveMode && filterID != 0xFF && !filesystem::is_directory(n))
                {
                    string lowercase = n.path().extension().string();
                    std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), tolower);
                    if (filterID == 0xFE)
                    {
                        if (std::find(c_imageExtensions.begin(), c_imageExtensions.end(), lowercase) == c_imageExtensions.end())
                            continue;
                    }
                    else if (lowercase != fileTypeExtensions.at(filterID).second)
                        continue;
                }
                if (filesystem::is_directory(n))
                {
                    currentPathFolders++;
                    fileData.push_back(FileData{n.path(), true});
                }
                else
                {
                    currentPathFiles++;
                    if (filesystem::file_size(n.path()) < config.thumbnailCacheSizeLimit * 1e6)
                    {
                        fileData.emplace_back(FileData{n.path(), false, false, false});
                        string lowercase = n.path().extension().string();
                        std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), tolower);
                        if (std::find(c_imageExtensions.begin(), c_imageExtensions.end(), lowercase) != c_imageExtensions.end())
                            filesTodo++;
                    }
                    else
                        fileData.emplace_back(FileData{n.path(), false, true, false});
                }
            }
            std::sort(fileData.begin(), fileData.end(),
                [](const FileData& a, const FileData& b)
                {
                    string A = a.path.filename().string();
                    string B = b.path.filename().string();
                    std::transform(A.begin(), A.end(), A.begin(), tolower);
                    std::transform(B.begin(), B.end(), B.begin(), tolower);
                    if (a.isDirectory == b.isDirectory)
                        return A < B;
                    if (a.isDirectory && !b.isDirectory)
                        return true;
                    return false;
                });
        }
        if (!listView)
        {
            bool missing = false;
            for (const auto& n : fileData)
                if (!n.isDirectory && !n.textureTooLarge && !n.texture)
                {
                    missing = true;
                    break;
                }

            if (missing)
            {
                Clock timer;
                for (int32_t i = 0; i < fileData.size() && timer.getElapsedTime().asMilliseconds() < 10; i++)
                {
                    if (fileData.at(i).isDirectory || fileData.at(i).textureTooLarge || fileData.at(i).cannotRead || fileData.at(i).texture)
                        continue;

                    string lowercase = fileData.at(i).path.extension().string();
                    std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), tolower);
                    if (std::find(c_imageExtensions.begin(), c_imageExtensions.end(), lowercase) == c_imageExtensions.end())
                        continue;

                    const uint64_t ts = filesystem::last_write_time(fileData.at(i).path).time_since_epoch().count();

                    const filesystem::path cached = "Cache/Thumbnails/" + DJB2((fileData.at(i).path / to_string(ts)).string()) + ".jpg";
                    Image img;
                    if (filesystem::exists(cached))
                    {
                        if (!img.loadFromFile(cached))
                            fileData.at(i).cannotRead = true;
                        else
                        {
                            fileData.at(i).texture = make_unique<Texture>();
                            if (!fileData.at(i).texture->loadFromImage(img))
                                fileData.at(i).cannotRead = true;
                        }
                        filesTodo--;
                        continue;
                    }
                    if (!filesystem::exists(fileData.at(i).path) || !img.loadFromFile(fileData.at(i).path))
                    {
                        fileData.at(i).cannotRead = true;
                        filesTodo--;
                        continue;
                    }
                    Image low, lowCenter;
                    Vector2u copyDest;
                    if (img.getSize().x > img.getSize().y)
                    {
                        low.resize(Vector2u(thumbnailSize * 1.5f, std::max(1.f, thumbnailSize * 1.5f * img.getSize().y / img.getSize().x)));
                        copyDest = Vector2u(0, (low.getSize().x - low.getSize().y) / 2);
                    }
                    else
                    {
                        low.resize(Vector2u(std::max(1.f, thumbnailSize * 1.5f * img.getSize().x / img.getSize().y), thumbnailSize * 1.5f));
                        copyDest = Vector2u((low.getSize().y - low.getSize().x) / 2, 0);
                    }
                    stb::stbir_resize(img.getPixelsPtr(), img.getSize().x, img.getSize().y, 0,
                                const_cast<unsigned char*>(low.getPixelsPtr()), low.getSize().x, low.getSize().y, 0,
                                stb::STBIR_RGBA, stb::STBIR_TYPE_UINT8_SRGB, stb::STBIR_EDGE_CLAMP, stb::stbir_filter::STBIR_FILTER_POINT_SAMPLE);

                    lowCenter.resize(Vector2u(thumbnailSize * 1.5f, thumbnailSize * 1.5f), Color::Black);
                    validate(lowCenter.copy(low, copyDest, IntRect(), true));

                    fileData.at(i).texture = make_unique<Texture>();
                    if (!fileData.at(i).texture->loadFromImage(lowCenter))
                    {
                        fileData.at(i).cannotRead = true;
                        filesTodo--;
                        continue;
                    }
                    fileData.at(i).texture->setSmooth(true);
                    if (!stb::stbi_write_jpg(("Cache/Thumbnails/" + DJB2((fileData.at(i).path / to_string(ts)).string()) + ".jpg").c_str(),
                        lowCenter.getSize().x, lowCenter.getSize().y, 4, lowCenter.getPixelsPtr(), 0))
                    {
                        fileData.at(i).cannotRead = true;
                    }
                    filesTodo--;
                }
            }
        }
        lastFrameFlags = explorerFlags;
        return fileDoubleClicked;
    }
};
