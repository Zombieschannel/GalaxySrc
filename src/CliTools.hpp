#pragma once
#include "Namespace.hpp"
#include "Pickers/LayerPicker.hpp"
#include "Rendering/ChunkTextureManager.hpp"
#include "Processing/ImageAdjustments.hpp"
#include "Processing/ImageEffects.hpp"

namespace glxy
{
    class CLITools
    {
        enum class CLIAction : int8_t
        {
            Help,
            Empty,
            Input,
            Output,
            Resize,
            Rescale,
            Rotate90CW,
            Rotate90CCW,
            Rotate180,
            ImageHorizontal,
            ImageVertical,
            Count
        };
        enum class ErrorType : int8_t
        {
            InvalidArguments,
            NegativeArguments,
            InvalidPath,
            InvalidMethod,
            InvalidPivot,
        };
        struct Command
        {
            string prefix;
            string shortPrefix;
            vector<string> arguments;
        };

        const unordered_map<CLIAction, Command> commands = {
            {CLIAction::Help, Command{"--help", "-h", {}}},
            {CLIAction::Empty, Command{"--empty", "-E", {"Width", "Height"}}},
            {CLIAction::Input, Command{"--input", "-I", {"Input path"}}},
            {CLIAction::Output, Command{"--output", "-O", {"Output path"}}},
            {CLIAction::Resize, Command{"--resize", "-R", {"Width", "Height", "Pivot"}}},
            {CLIAction::Rescale, Command{"--rescale", "-r", {"Width", "Height", "Method"}}},
            {CLIAction::Rotate90CW, Command{"--rotate90CW", "-R90CW", {}}},
            {CLIAction::Rotate90CCW, Command{"--rotate90CCW", "-R90CCW", {}}},
            {CLIAction::Rotate180, Command{"--rotate180", "-R180", {}}},
            {CLIAction::ImageHorizontal, Command{"--flipImgHor", "-FIH", {}}},
            {CLIAction::ImageVertical, Command{"--flipImgVer", "-FIV", {}}},
        };
        const unordered_map<RescaleMethod, string> rescaleMethods = {
            {RescaleMethod::Triangle, "Triangle"},
            {RescaleMethod::Box, "Box"},
            {RescaleMethod::Catmullrom, "Catmullrom"},
            {RescaleMethod::Mitchell, "Mitchell"},
            {RescaleMethod::CubicBSpline, "CubicBSpline"},
            {RescaleMethod::PointSample, "PointSample"},
        };
        const unordered_map<Pivot, string> pivots = {
            {Pivot::LeftTop, "LeftTop"},
            {Pivot::MiddleTop, "MiddleTop"},
            {Pivot::RightTop, "RightTop"},
            {Pivot::LeftMiddle, "LeftMiddle"},
            {Pivot::Center, "Center"},
            {Pivot::RightMiddle, "RightMiddle"},
            {Pivot::LeftBottom, "LeftBottom"},
            {Pivot::MiddleBottom, "MiddleBottom"},
        };

        int8_t activeEditor = -1;
        int8_t editorCount = 0;
        LayerPicker layerPicker;

        static void Error(CLIAction action, ErrorType error);
        void Help();
        int32_t ActionHandler(CLIAction action, const vector<string>& argv, int32_t offset, bool verify);
    public:
        int32_t Start(const vector<string>& arguments);
    };
}
