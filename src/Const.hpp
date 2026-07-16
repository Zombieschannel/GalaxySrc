#pragma once
#include <SFML/Graphics.hpp>
#include "Namespace.hpp"

using namespace sf;

constexpr array<uint8_t, 3> c_versionNumber = {0, 4, 0};

#if defined(__x86_64__) || defined(_M_X64)
constexpr string_view c_SFML_ARCH = "x64";
#elif defined(__i386__) || defined(_M_IX86)
constexpr string_view c_SFML_ARCH = "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
constexpr string_view c_SFML_ARCH = "ARM64";
#elif defined(__arm__) || defined(_M_ARM)
constexpr string_view c_SFML_ARCH = "ARM";
#elif defined(__riscv) || defined(__riscv_xlen)
constexpr string_view c_SFML_ARCH = "RISC-V";
#else
constexpr string_view c_SFML_ARCH = "Unknown";
#endif

static const string c_AppVersion = "v." + to_string(c_versionNumber.at(0)) + "." + to_string(c_versionNumber.at(1)) + "." + to_string(c_versionNumber.at(2));

static const string c_AppName = "Galaxy " + c_AppVersion;

constexpr int8_t c_fileMenuCnt = 5;
constexpr int8_t c_languageCnt = 2;
constexpr int8_t c_resamplingMethodCnt = 6;
constexpr uint16_t c_minChunkSize = 128;
constexpr uint16_t c_maxChunkSize = 1024;
constexpr uint16_t c_infChunkSize = 256;
constexpr uint16_t c_maxChunkWorkableSize = 1024;
constexpr int8_t c_mediumQualityChunkFactor = 4;
constexpr int8_t c_lowQualityChunkFactor = 16;
constexpr uint8_t c_layerPreviewTextureSize = 100;
constexpr float c_chunkSwitchSmooth = 0.75f;
constexpr float c_chunkSwitchMedium = 4.f;
constexpr float c_chunkSwitchLow = 16.f;
constexpr float c_UIElementSize = 0.01f;
constexpr float c_rulerSize = 15.f;
constexpr int8_t c_maxRecentFiles = 15;
constexpr int16_t c_maxLayers = 10000;
constexpr uint32_t c_noChangeReturnColor = 0x505050FF;

constexpr uint8_t c_keyViewMove = 30;
constexpr uint8_t c_edgeViewMove = 100;
constexpr uint8_t c_edgeViewMoveFaster = 200;
constexpr float c_touchPadViewMove = 50.f;

constexpr uint32_t c_infiniteToolsEnabled = 0x000E38;

constexpr array c_textSizes = { 8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 24, 26, 30, 36, 42, 48, 56, 72, 96, 144, 288 };

constexpr float c_PI = 3.14159265f;
constexpr float c_PI_2 = 1.57079633f;

typedef Vector2i ChunkID;
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

constexpr array<string_view, 5> c_imageExtensions = {".png", ".jpg", ".bmp", ".tga", ".qoi"};
constexpr array<string_view, 2> c_fontExtensions = {".ttf", ".otf"};

constexpr string_view c_futurePlan =
R"(Future plans - everything below is subject to change
-Full undo/redo [0.5.0]
-CPU multi thread acceleration [0.6.0]
-More infinite canvas tools [0.6.0]
-Custom file format for saving projects [0.7.0]
-More functionality for CLI [0.7.0]
-Metadata integration [0.8.0]
-Full release [1.0.0]
Random ideas for after 1.0
-Vector graphics?
-RAW files support?
-Android/iOS support?
-Web support?
-Multi user editing over network?
)";

constexpr string_view c_changelog =
R"(The lightspeed update - 0.4.0 (15. Aug 2026)
-Added a file browser for opening and saving files
-Added a circular shifter feature
-Added three new effects: vignette, Mandelbrot set and sharpening
-All assets are now packed within the executable for Linux and MacOS as well, improves startup performance
-Changed chunk rendering strategy to now prioritize chunks that were not updated in a while instead of going from top left
-Changed how brush/eraser allocates chunks which increases performance mainly in infinite canvas mode
-Improved move pixels/move selection/transform image performance
-Gradient now utilizes the GPU which increases the speed drastically
-Greatly improved bucket fill speed
-Color layers are now not created (internally) until something is actually drawn onto them reducing memory usage
-Selection layer now generally creates less chunks slightly reducing memory usage and performance on larger images
-Main canvas window now split into UI and graphics, only updates when something changes for less resource usage when idle
-Pencil now behaves like brush, pixels drawn with it are overlaid instead of overwritten
-Instead of mapping pan tool to a button on the mouse, now you map mouse buttons (Middle, Extra1, Extra2) to tools
-Reduced minimum window requirement from 800x600 to 640x480
-Reduced minimum font size from 12 to 10 and increased maximum font size from 24 to 28
-Changed zoom tool to use click and drag instead of left/right click
-Added custom cursors for brush and eraser, pan cursor changed to a hand
-Slightly redesigned UI (black background, no canvas title bar until at least 2 canvases open, smaller padding on windows)
-Padding, spacing, rounding should be consistent across different font sizes
-Added icon to swap primary and secondary color
-Layer icons swapped with ones used in the menu bar
-Merged adjustments and effects into a single JSON save file
-Separated open and recent menus
-Fixed a crash on MacOS due to unavailable cursor
-Fixed move selection canceling when switching tools instead of finishing like other tools
-"Fixed" chunks not updating properly if text alignment is set to center/right due to a bug in SFML 3.1.0, significant performance hit unfortunately
-Fixed shapes tool drag circles not behaving properly after rotating previous shape
-Added total pixels and size in memory for when creating new image
-Recent menu now checks if recently opened files still exist

A bit of usual bug fixing - 0.3.1 (16. Jul 2026)
-Added even lower texture quality when zoomed out far enough to reduce VRAM usage on giant canvas sizes
-Limited fractal noise to max 15 octaves and minimum 0.01 smoothness as things start to break on giant canvases due to float precision
-Fixed rotate 180° not working on canvas sizes with more than 4 billion pixels
-Fixed noise generation sometimes going out of bounds
-Fixed shape move icon not moving along with the shape

To infinity and beyond - 0.3.0 (12. Jul 2026)
-Added infinite canvas mode
-Added text rendering
-Added shape rendering
-Added lasso select tool
-Added move selection tool
-Added color swap tool
-Added editor features search for better keyboard only navigation
-Added rotation for selection and selected pixels
-Added icons to options in the main menu bar
-Updated to SFML 3.1.0
-Added options to select area of the image
-Improved general performance for brush/pixel select/merge layers
-Improved performance while using box/circle pixel select
-Added the ability to close color/tool/layer windows
-Custom cursor icons for some tools
-Replaced cancel and finish buttons with icons
-Added - and + icons to brush sizes
-Added support for .qoi image format
-Added Ctrl + Shift + X shortcut for image crop
-Added bold grid functionality
-Added copy to clipboard for OpenGL info
-Redesigned tool picker window (sharper icons and 3 columns)
-Fixed aspect lock when selecting sometimes being 1 pixel of on one of the axes
-Fixed HSV adjustment affecting grayscale pixels
-Fixed not being able to pixel select on edges of image
-Fixed rendering low quality textures before being generated
-Fixed default texture allocation for unused textures
-Fixed bucket tolerance not being saved to disk

Another small bugfix update - 0.2.2 (1. May 2026)
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
