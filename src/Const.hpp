#pragma once
#include <SFML/Graphics.hpp>
#include "Languages.hpp"
#include "Namespace.hpp"
using namespace sf;

const array<uint8_t, 3> c_versionNumber = {0, 1, 1};

#ifdef SFML_X86
static const string c_SFML_ARCH = "x86";
#elif defined(SFML_X64)
static const string c_SFML_ARCH = "x64";
#elif defined(SFML_ARM64)
static const string c_SFML_ARCH = "ARM64";
#elif defined(SFML_SYSTEM_LINUX)
static const string c_SFML_ARCH = "x64";
#elif defined(SFML_SYSTEM_MACOS)
static const string c_SFML_ARCH = "ARM64";
#else
static const string c_SFML_ARCH = "";
#endif

static const string c_AppVersion = "v." + to_string(c_versionNumber.at(0)) + "." + to_string(c_versionNumber.at(1)) + "." + to_string(c_versionNumber.at(2));

static const string c_AppName = "Galaxy " + c_AppVersion;

static const int8_t c_helpCnt = 4;
static const int8_t c_fileMenuCnt = 5;
static const int8_t c_languageCnt = 2;
static const int8_t c_resamplingMethodCnt = 6;
static const uint16_t c_minChunkSize = 128;
static const uint16_t c_maxChunkSize = 1024;
static const int8_t c_lowQualityChunkFactor = 4;
static const uint8_t c_layerPreviewTextureSize = 100;
static const float c_chunkSwitchSmooth = 0.75f;
static const float c_chunkSwitch = 3.f;
static const float c_UIElementSize = 0.01f;
static const float c_rulerSize = 15.f;
static const int8_t c_maxRecentFiles = 15;
static const int16_t c_maxLayers = 10000;
#ifdef SFML_DESKTOP
static const int8_t c_colorCount = 2;
#else
static const int8_t c_colorCount = 1;
#endif
static const uint32_t c_noChangeReturnColor = 0x505050FF;

static const uint8_t c_keyViewMove = 30;
static const uint8_t c_edgeViewMove = 100;
static const uint8_t c_edgeViewMoveFaster = 200;
static const float c_touchPadViewMove = 50.f;

static const array c_blendModes = {
        BlendMode(BlendMode::Factor::One, BlendMode::Factor::Zero),
        BlendMode(BlendMode::Factor::SrcAlpha, BlendMode::Factor::One, BlendMode::Equation::Add, BlendMode::Factor::One, BlendMode::Factor::One, BlendMode::Equation::Add),
        BlendMode(BlendMode::Factor::SrcAlpha, BlendMode::Factor::OneMinusSrcAlpha, BlendMode::Equation::Add, BlendMode::Factor::One, BlendMode::Factor::OneMinusSrcAlpha, BlendMode::Equation::Add),
        BlendMode(BlendMode::Factor::DstColor, BlendMode::Factor::Zero)
};

static const array<string, 4> c_imageExtensions = {".png", ".jpg", ".bmp", ".tga"};

static const string c_futurePlan =
R"(Future plans - everything below is subject to change
-CPU separate canvas and UI threads [0.2.0]
-Basic adjustments and effects [0.2.0]
-Text and shape rendering [0.3.0]
-Custom cursor icons for tools [0.3.0]
-Quick shortcut icons in GUI [0.3.0]
-Stability and performance improvements [0.4.0]
-Full undo/redo [0.5.0]
-CPU multi thread acceleration [0.6.0]
-Full release [1.0.0]
)";

static const string c_changelog =
R"(Small bugfix update - 0.1.1 (3. Mar 2026)
-Escape key can be used for canceling operations in the subtitle bar
-Wand fill automatically finishes after selecting new position similar to bucket fill
-Fixed incorrect blending when pasting copied image
-Fixed selection chunks not being updated with wand select after changing tolerance or position
-Fixed crash on copy image after resizing canvas in any way
-Fixed crash on delete selected/copy image with selection out of bounds
-Fixed crash on select all

The initial version - 0.1.0 (1. Mar 2026)
-No changelog available
)";