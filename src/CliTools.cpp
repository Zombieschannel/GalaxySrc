#include "CliTools.hpp"
#include <SFML/Graphics.hpp>

#include "Canvas/CanvasWorker.hpp"
#include "Rendering/ChunkTextureManager.hpp"
#include "ZEditorsCommon/InternalResource.hpp"

using namespace sf;

namespace stb
{
    #include <stb_image_write.h>
}

void glxy::CLITools::Error(const CLIAction action, const ErrorType error)
{
    switch (action)
    {
    case CLIAction::Empty: err() << "Empty: "; break;
    case CLIAction::Input: err() << "Input: "; break;
    case CLIAction::Resize: err() << "Resize: "; break;
    case CLIAction::Rescale: err() << "Rescale: "; break;
    default: break;
    }
    switch (error)
    {
    case ErrorType::InvalidArguments: err() << "Invalid arguments"; break;
    case ErrorType::NegativeArguments: err() << "Arguments negative or zero"; break;
    case ErrorType::InvalidPath: err() << "Invalid path"; break;
    case ErrorType::InvalidMethod: err() << "Invalid method"; break;
    case ErrorType::InvalidPivot: err() << "Invalid pivot"; break;
    default: break;
    }
    err() << " (-h for help)" << endl;
}

void glxy::CLITools::Help()
{
    cout << "Galaxy CLI mode - order of arguments is important!" << endl;

    for (int8_t i = 0; i < 70; i++)
        cout << '=';
    cout << endl;

    cout << "Arguments:     Arg-Short:   Properties:" << endl;
    const string tab = "               ";
    const string tabShort = "            ";
    for (int8_t i = 0; i < commands.size(); i++)
    {
        const CLIAction action = static_cast<CLIAction>(i);
        cout << commands.at(action).prefix << tab.substr(commands.at(action).prefix.size()) <<
            commands.at(action).shortPrefix << tabShort.substr(commands.at(action).shortPrefix.size()) << " ";
        cout << commands.at(action).arguments.size() << (commands.at(action).arguments.size() ? " - " : "");
        for (int8_t j = 0; j < commands.at(action).arguments.size(); j++)
        {
            cout << commands.at(action).arguments.at(j);
            if (j != commands.at(action).arguments.size() - 1)
                cout << ", ";
        }
        cout << endl;
    }

    for (int8_t i = 0; i < 70; i++)
        cout << '=';
    cout << endl;

    cout << "Rescale methods:" << endl;
    for (int8_t i = 0; i < rescaleMethods.size(); i++)
    {
        const RescaleMethod method = static_cast<RescaleMethod>(i);
        cout << rescaleMethods.at(method) << tab.substr(rescaleMethods.at(method).size()) << rescaleMethods.at(method) << endl;
    }

    for (int8_t i = 0; i < 70; i++)
        cout << '=';
    cout << endl;

    cout << "Pivots:" << endl;
    for (int8_t i = 0; i < pivots.size(); i++)
    {
        const Pivot pivot = static_cast<Pivot>(i);
        cout << pivots.at(pivot) << endl;
    }
}

int32_t glxy::CLITools::ActionHandler(const CLIAction action, const vector<string>& argv, const int32_t offset, const bool verify)
{
    if (verify)
    {
        switch (action)
        {
        case CLIAction::Rotate90CW: case CLIAction::Rotate90CCW: case CLIAction::Rotate180: case CLIAction::ImageHorizontal:
        case CLIAction::ImageVertical:
            return 0;
        default:
            break;
        }
    }

    Vector2i vec2i;
    string str;
    filesystem::path fs;
    RescaleMethod method = RescaleMethod::Count;
    Pivot pivot = Pivot::Count;

    vector<string> args;
    for (int8_t i = 0; i < commands.at(action).arguments.size(); i++)
        args.push_back(argv.at(i + offset));

    switch (action)
    {
    case CLIAction::Help:
        Help();
        return -1;
    case CLIAction::Empty:
        try
        {
            vec2i.x = stoi(args.at(0));
            vec2i.y = stoi(args.at(1));
        }
        catch (std::invalid_argument& e)
        {
            Error(action, ErrorType::InvalidArguments);
            return -1;
        }
        if (vec2i.x <= 0 || vec2i.y <= 0)
        {
            Error(action, ErrorType::NegativeArguments);
            return -1;
        }
        if (verify)
            return 0;
        CanvasWorker::waitWork();
        CanvasWorker::AddEditor(false);
        editorCount++;
        activeEditor = editorCount - 1;
        layerPicker.createNewImage(false);
        CanvasWorker::AddWork(CanvasWork::ImageEmpty{Vector2u(vec2i), Color::White}, activeEditor);
        break;
    case CLIAction::Input:
        str = args.at(0);
        if (!filesystem::exists(str))
        {
            Error(action, ErrorType::InvalidPath);
            return -1;
        }
        if (verify)
            return 0;
        CanvasWorker::waitWork();
        CanvasWorker::AddEditor(false);
        editorCount++;
        activeEditor = editorCount - 1;
        layerPicker.createNewImage(false);
        CanvasWorker::AddWork(CanvasWork::ImageOpen{str}, activeEditor);
        break;
    case CLIAction::Output:
    {
        fs = args.at(0);
        if (verify)
            return 0;

        CanvasWorker::waitWork();
        const Image result = ChunkTextureManager::renderWholeImage(CanvasWorker::getChunkManager(activeEditor));

        if (fs.extension() == ".png")
            stb::stbi_write_png(fs.string().c_str(), result.getSize().x, result.getSize().y, 4, result.getPixelsPtr(), 0);
        if (fs.extension() == ".jpg")
            stb::stbi_write_jpg(fs.string().c_str(), result.getSize().x, result.getSize().y, 4, result.getPixelsPtr(), 80);
        if (fs.extension() == ".bmp")
            stb::stbi_write_bmp(fs.string().c_str(), result.getSize().x, result.getSize().y, 4, result.getPixelsPtr());
        if (fs.extension() == ".tga")
            stb::stbi_write_tga(fs.string().c_str(), result.getSize().x, result.getSize().y, 4, result.getPixelsPtr());
        break;
    }
    case CLIAction::Rescale:
        try
        {
            vec2i.x = stoi(args.at(0));
            vec2i.y = stoi(args.at(1));
            for (int8_t i = 0; i < static_cast<int8_t>(RescaleMethod::Count); i++)
            {
                if (rescaleMethods.at(static_cast<RescaleMethod>(i)) == args.at(2))
                {
                    method = static_cast<RescaleMethod>(i);
                    break;
                }
            }
        }
        catch (std::invalid_argument& e)
        {
            Error(action, ErrorType::InvalidArguments);
            return -1;
        }
        if (vec2i.x <= 0 || vec2i.y <= 0)
        {
            Error(action, ErrorType::NegativeArguments);
            return -1;
        }
        if (method == RescaleMethod::Count)
        {
            Error(action, ErrorType::InvalidMethod);
            return -1;
        }
        if (verify)
            return 0;
        CanvasWorker::AddWork(CanvasWork::RescaleCanvas{Vector2u(vec2i), method}, activeEditor);
        break;
    case CLIAction::Resize:
        try
        {
            vec2i.x = stoi(args.at(0));
            vec2i.y = stoi(args.at(1));
            for (int8_t i = 0; i < static_cast<int8_t>(Pivot::Count); i++)
            {
                if (pivots.at(static_cast<Pivot>(i)) == args.at(2))
                {
                    pivot = static_cast<Pivot>(i);
                    break;
                }
            }
        }
        catch (std::invalid_argument& e)
        {
            Error(action, ErrorType::InvalidArguments);
            return -1;
        }
        if (vec2i.x <= 0 || vec2i.y <= 0)
        {
            Error(action, ErrorType::NegativeArguments);
            return -1;
        }
        if (pivot == Pivot::Count)
        {
            Error(action, ErrorType::InvalidPivot);
            return -1;
        }
        if (verify)
            return 0;
        CanvasWorker::AddWork(CanvasWork::ResizeCanvas{Vector2u(vec2i), pivot}, activeEditor);
        break;
    case CLIAction::Rotate90CW: CanvasWorker::AddWork(CanvasWork::Rotate90CW{}, activeEditor); break;
    case CLIAction::Rotate90CCW: CanvasWorker::AddWork(CanvasWork::Rotate90CCW{}, activeEditor); break;
    case CLIAction::Rotate180: CanvasWorker::AddWork(CanvasWork::Rotate180{}, activeEditor); break;
    case CLIAction::ImageHorizontal: CanvasWorker::AddWork(CanvasWork::FlipImageHorizontal{}, activeEditor); break;
    case CLIAction::ImageVertical: CanvasWorker::AddWork(CanvasWork::FlipImageVertical{}, activeEditor); break;
    default: break;
    }
    return 0;
}

int32_t glxy::CLITools::Start(const vector<string>& arguments)
{
    Clock counter;
    vector<CLIAction> actions;
    int32_t actionResult = 0;
    int32_t argumentOffset = 1;
    //Verify commands
    for (int32_t i = 1; i < arguments.size(); i++)
    {
        bool commandFound = false;
        for (int8_t j = 0; j < static_cast<int8_t>(CLIAction::Count); j++)
            if (arguments.at(i) == commands.at(static_cast<CLIAction>(j)).prefix || arguments.at(i) == commands.at(static_cast<CLIAction>(j)).shortPrefix)
            {
                actionResult = ActionHandler(static_cast<CLIAction>(j), arguments, i + 1, true);
                if (actionResult == -1)
                    return -1;
                actions.push_back(static_cast<CLIAction>(j));
                i += commands.at(static_cast<CLIAction>(j)).arguments.size();
                commandFound = true;
            }
        if (!commandFound)
        {
            err() << "Command not found: " << arguments.at(i) << " (-h for help)" << endl;
            return -1;
        }
    }
    CanvasWorker::Setup();
    //Exectute commands
    const int8_t barSize = 50;
    for (int32_t i = 0; i < actions.size(); i++)
    {
        if (i)
            cout << "\r";
        cout << "[";
        for (int8_t j = 0; j < barSize; j++)
        {
            if (static_cast<float>(i) / actions.size() > static_cast<float>(j) / barSize)
                cout << "=";
            else
                cout << " ";
        }
        cout << "]" << std::flush;
        ActionHandler(actions.at(i), arguments, argumentOffset + 1, false);
        argumentOffset += commands.at(actions.at(i)).arguments.size() + 1;
    }
    CanvasWorker::ExitThread();
    cout << "\r[";
    for (int8_t j = 0; j < barSize; j++)
        cout << "=";
    cout << "]" << endl;
    cout << "Finished in " << counter.getElapsedTime().asSeconds() << "s" << endl;
    return 0;
}
