#pragma once
#include <SFML/Graphics.hpp>
#include "Namespace.hpp"

using namespace sf;

const array<uint8_t, 3> c_versionNumber = {0, 2, 2};

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
static const int8_t c_colorCount = 2;
static const uint32_t c_noChangeReturnColor = 0x505050FF;

static const uint8_t c_keyViewMove = 30;
static const uint8_t c_edgeViewMove = 100;
static const uint8_t c_edgeViewMoveFaster = 200;
static const float c_touchPadViewMove = 50.f;

typedef int16_t ChunkID;
typedef int16_t LayerID;
typedef int16_t EditorID;
typedef int8_t ColorID;

static const array c_blendModes = {
        BlendMode(BlendMode::Factor::One, BlendMode::Factor::Zero),
        BlendMode(BlendMode::Factor::SrcAlpha, BlendMode::Factor::One, BlendMode::Equation::Add,
                BlendMode::Factor::One, BlendMode::Factor::One, BlendMode::Equation::Add),
        BlendMode(BlendMode::Factor::SrcAlpha, BlendMode::Factor::OneMinusSrcAlpha, BlendMode::Equation::Add,
                BlendMode::Factor::One, BlendMode::Factor::OneMinusSrcAlpha, BlendMode::Equation::Add),
        BlendMode(BlendMode::Factor::DstColor, BlendMode::Factor::Zero)
};

static const array<string, 4> c_imageExtensions = {".png", ".jpg", ".bmp", ".tga"};

static const string c_futurePlan =
R"(Future plans - everything below is subject to change
-Text and shape rendering [0.3.0]
-Custom cursor icons for tools [0.3.0]
-Quick shortcut icons in GUI [0.3.0]
-Stability and performance improvements [0.4.0]
-Full undo/redo [0.5.0]
-CPU multi thread acceleration [0.6.0]
-Full release [1.0.0]
)";

static const string c_changelog =
R"(Another small bugfix update - 0.2.2 (1. May 2026)
-Fixed copy and cut working only on the bottom layer
-Fixed gradient working only on the bottom layer
-Fixed transform image working only on the bottom layer
-Fixed move selection not working properly after selecting with wand and not finishing selection

Small bugfix update - 0.2.1 (5. Apr 2026)
-Settings window now resizes after changing font size
-Popup windows now recenter after the app window resizes
-Fixed font size not being saved and restored properly

The threads and effects update - 0.2.0 (1. Apr 2026)
-Basic multithreading: separated canvas and rendering/UI threads
-Added 5 adjustments: black and white, brightness/contrast, HSV, invert colors, tint
-Added 5 effects: Gaussian blur, box blur, directional blur, white noise, fractal noise
-Added CLI tools for unit testing in the future
-Added holding Ctrl to switch to additive selection, holding Shift to have square aspect ratio
-Brush no longer replaces pixels if there is transparency
-Added explicit option to show color picker in triangle style rather than using mouse right click
-Left color and right color renamed to primary and secondary
-Added debug mode

Small bugfix update - 0.1.1 (3. Mar 2026)
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