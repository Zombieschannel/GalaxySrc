#include "App.hpp"
#include "Func.hpp"
#include "CliTools.hpp"

#ifdef SFML_SYSTEM_WINDOWS
#include <Windows.h>
#elif defined(SFML_SYSTEM_ANDROID)
#include <SFML/System/NativeActivity.hpp>
#include <android/native_activity.h>
#endif

#include <filesystem>

#if !defined(SFML_SYSTEM_EMSCRIPTEN) && !defined(SFML_SYSTEM_ANDROID)
#include "../whereami/whereami.h"
void setWorkingDirectory()
{
    int32_t dirName_length;
    int32_t length = wai_getExecutablePath(nullptr, 0, &dirName_length);
    if (length > 0)
    {
        string path;
        path.resize(length);
        wai_getExecutablePath(&path[0], length, &dirName_length);
        path.resize(dirName_length);

#ifdef SFML_SYSTEM_MACOS
        filesystem::current_path(path + "/../../..");
#else
        filesystem::current_path(path);
#endif

    }
}
#else
void setWorkingDirectory()
{

}
#endif

#ifdef SFML_SYSTEM_LINUX
namespace LinuxStuff
{
#include <X11/Xlib.h>
}
#endif

int32_t main(int32_t argc, char* argv[])
{
#ifdef SFML_SYSTEM_LINUX
    LinuxStuff::XInitThreads();
#endif

    filesystem::path startParams = "";
    bool openWithGalaxy = false;
    if (argc == 2)
    {
        if (filesystem::exists(string(argv[0])) && filesystem::exists(string(argv[1])))
        {
            filesystem::path p(argv[1]);
            if (find(c_imageExtensions.begin(), c_imageExtensions.end(), p.extension()) != c_imageExtensions.end())
            {
                openWithGalaxy = true;
                startParams = p;
            }
        }
    }
    if (!openWithGalaxy && argc > 1)
    {
#ifdef SFML_SYSTEM_WINDOWS
        FreeConsole();
        AllocConsole();
        freopen("CONOUT$", "w", stderr);
#endif

        vector<string> arguments;
        arguments.resize(argc);
        for (int32_t i = 0; i < argc; i++)
            arguments.at(i) = string(argv[i]);
        glxy::CLITools cli;
        return cli.Start(arguments);
    }

    Clock startUpTime;
#ifdef SFML_SYSTEM_ANDROID
    ANativeActivity* NA = getNativeActivity();
    filesystem::current_path(NA->externalDataPath);
#else
    setWorkingDirectory();
#endif

    //Prevent double open
    if (filesystem::exists("pdo.pdo"))
    {
        if (time(nullptr) - to_time_t(filesystem::last_write_time("pdo.pdo")) <= 5)
        {
            ofstream flag;
            flag.open("pdo");
            flag.close();
            return 0;
        }
    }
    else
    {
        ofstream flag;
        flag.open("pdo.pdo");
        flag.close();
    }

    glxy::App app;
    app.Start(startParams, openWithGalaxy, startUpTime);
    cout << "Exit" << endl;
}