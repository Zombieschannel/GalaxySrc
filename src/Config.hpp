#pragma once
#include <SFML/Graphics.hpp>
#include "Namespace.hpp"
#include <set>
using namespace sf;

namespace glxy
{
    class Config
    {
        Config() = default;
    public:
        Vector2u resolution = Vector2u(1280, 720);
        bool fullscreen = false;
#ifdef SFML_DESKTOP
        float GUIScale = 1.f;
#else
        float GUIScale = 1.25f;
#endif
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
#ifdef SFML_DESKTOP
        bool showRuler = true;
#else
        bool showRuler = false;
#endif
        bool drawSelectionLines = true;
        bool colorPickerTriangle = false;
        bool debugMode = false;
        int8_t middleMouseButton = 4;
        int8_t extra1MouseButton = 6;
        int8_t extra2MouseButton = 0;
        int32_t thumbnailCacheSizeLimit = 3;
        std::array<bool, 3> openWindow = { true, true, true };
        std::set<std::pair<int64_t, std::string>> recentFiles;
        Vector2i gridBold = Vector2i(16, 16);

        bool maintainAspectResize = true;
        bool maintainAspectCanvas = true;
        uint8_t resamplingMethod = 0;
        bool transformSamplingSmooth = false;

        float brushRadius = 5.f;
        float eraserRadius = 5.f;
        float colorSwapRadius = 5.f;
        int8_t bucketTolerance = 50;
        int8_t wandTolerance = 50;
        int8_t colorSwapTolerance = 50;
        uint8_t shapeID = 0;
        float shapeRadius = 5.f;
        float shapeOutlineThickness = 0;

        uint8_t textStyle = 0;
        int8_t textAlignment = 0;
        float letterSpacing = 1;
        float lineSpacing = 1;
        float textOutlineThickness = 0;
        uint16_t textSize = 16;
        uint16_t fontID = 0;

        std::string fontLocation =
#if defined(SFML_SYSTEM_WINDOWS)
        "C:/Windows/Fonts";
#elif defined(SFML_SYSTEM_MACOS)
        "/System/Library/Fonts";
#elif defined(SFML_SYSTEM_LINUX)
        "/usr/share/fonts";
#elif defined(SFML_SYSTEM_ANDROID)
        "";
#elif defined(SFML_SYSTEM_EMSCRIPTEN)
        "";
#endif

        static Config& get();
        void Save() const;
        void Load();
    };
}