#pragma once
#include <SFML/Graphics.hpp>
#include <set>
using namespace sf;

namespace glxy
{
    struct AppSettings
    {
        Vector2u resolution = Vector2u(1280, 720);
        bool fullscreen = false;
        float GUIScale = 1.f;
        int32_t fontSize = 16;
        bool verticalSync = true;
        bool outOfFocus = true;
        Color bgColor = Color(16, 16, 16);
        uint8_t antialiasing = 0;
        int32_t maxFPS = 60;
        int32_t languageID = 0;
        bool touchPadSupport = false;
        bool animateZoom = true;
        bool animatePan = true;
        bool syncViewport = false;
        bool showFPS = false;
        bool showGrid = true;
        bool showRuler = true;
        bool drawSelectionLines = true;
        uint8_t panMouseButton = 0;
        std::set<std::pair<int64_t, std::string>> recentFiles;

        bool maintainAspectResize = true;
        bool maintainAspectCanvas = true;
        uint8_t resamplingMethod = 0;

        int8_t bucketTolerance = 50;
        int8_t wandTolerance = 50;
        void Save() const;
        void Load();
    };
}