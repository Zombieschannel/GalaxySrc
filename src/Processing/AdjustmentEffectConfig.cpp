#include "AdjustmentEffectConfig.hpp"
#include "../ZEditorsCommon/ZTB.hpp"

glxy::AdjustmentEffectConfig& glxy::AdjustmentEffectConfig::get()
{
    static AdjustmentEffectConfig config;
    return config;
}

void glxy::AdjustmentEffectConfig::Save() const
{
    JSON file;
    //Adjustments
    file.setValue("brightness", brightness);
    file.setValue("contrast", contrast);
    file.setValue("hue", hue);
    file.setValue("saturation", saturation);
    file.setValue("value", value);
    file.setValue("tintRed", tintRed);
    file.setValue("tintGreen", tintGreen);
    file.setValue("tintBlue", tintBlue);

    //Effects
    file.setValue("boxBlurRadius", boxBlurRadius);
    file.setValue("gaussBlurRadius", gaussBlurRadius);
    file.setValue("dirBlurRadius", dirBlurRadius);
    file.setValue("dirBlurAngle", dirBlurAngle);
    file.setValue("noiseFrequency", noiseFrequency);
    file.setValue("noiseIntensity", noiseIntensity);
    file.setValue("noiseSaturation", noiseSaturation);
    file.setValue("noiseSeed", noiseSeed);
    file.setValue("fractalOctaves", fractalOctaves);
    file.setValue("fractalSeed", fractalSeed);
    file.setValue("fractalSmoothness", fractalSmoothness);
    file.setValue("fractalColor1R", fractalColor1.r);
    file.setValue("fractalColor1G", fractalColor1.g);
    file.setValue("fractalColor1B", fractalColor1.b);
    file.setValue("fractalColor2R", fractalColor2.r);
    file.setValue("fractalColor2G", fractalColor2.g);
    file.setValue("fractalColor2B", fractalColor2.b);
    file.setValue("mandelbrotIterations", mandelbrotIterations);
    file.setValue("mandelbrotZoom", mandelbrotZoom);
    file.setValue("mandelbrotOffsetX", mandelbrotOffset.x);
    file.setValue("mandelbrotOffsetY", mandelbrotOffset.y);
    file.setValue("mandelbrotCenterColorR", mandelbrotCenterColor.r);
    file.setValue("mandelbrotCenterColorG", mandelbrotCenterColor.g);
    file.setValue("mandelbrotCenterColorB", mandelbrotCenterColor.b);
    file.setValue("mandelbrotOuterColor1R", mandelbrotOuterColor1.r);
    file.setValue("mandelbrotOuterColor1G", mandelbrotOuterColor1.g);
    file.setValue("mandelbrotOuterColor1B", mandelbrotOuterColor1.b);
    file.setValue("mandelbrotOuterColor2R", mandelbrotOuterColor2.r);
    file.setValue("mandelbrotOuterColor2G", mandelbrotOuterColor2.g);
    file.setValue("mandelbrotOuterColor2B", mandelbrotOuterColor2.b);
    file.setValue("sharpeningQuality", sharpeningRadius);
    file.setValue("sharpeningIntensity", sharpeningIntensity);
    file.setValue("sharpeningThreshold", sharpeningThreshold);
    file.saveToFile("adjeff.json");
}

void glxy::AdjustmentEffectConfig::Load()
{
    JSON file;
    if (file.loadFromFile("adjeff.json"))
    {
        //Adjustments
        file.loadValue("brightness", brightness);
        file.loadValue("contrast", contrast);
        file.loadValue("hue", hue);
        file.loadValue("saturation", saturation);
        file.loadValue("value", value);
        file.loadValue("tintRed", tintRed);
        file.loadValue("tintGreen", tintGreen);
        file.loadValue("tintBlue", tintBlue);

        //Effects
        file.loadValue("boxBlurRadius", boxBlurRadius);
        file.loadValue("gaussBlurRadius", gaussBlurRadius);
        file.loadValue("dirBlurRadius", dirBlurRadius);
        file.loadValue("dirBlurAngle", dirBlurAngle);
        file.loadValue("noiseFrequency", noiseFrequency);
        file.loadValue("noiseIntensity", noiseIntensity);
        file.loadValue("noiseSaturation", noiseSaturation);
        file.loadValue("noiseSeed", noiseSeed);
        file.loadValue("fractalOctaves", fractalOctaves);
        file.loadValue("fractalSeed", fractalSeed);
        file.loadValue("fractalSmoothness", fractalSmoothness);
        file.loadValue("fractalColor1R", fractalColor1.r);
        file.loadValue("fractalColor1G", fractalColor1.g);
        file.loadValue("fractalColor1B", fractalColor1.b);
        file.loadValue("fractalColor2R", fractalColor2.r);
        file.loadValue("fractalColor2G", fractalColor2.g);
        file.loadValue("fractalColor2B", fractalColor2.b);
        file.loadValue("mandelbrotIterations", mandelbrotIterations);
        file.loadValue("mandelbrotZoom", mandelbrotZoom);
        file.loadValue("mandelbrotOffsetX", mandelbrotOffset.x);
        file.loadValue("mandelbrotOffsetY", mandelbrotOffset.y);
        file.loadValue("mandelbrotCenterColorR", mandelbrotCenterColor.r);
        file.loadValue("mandelbrotCenterColorG", mandelbrotCenterColor.g);
        file.loadValue("mandelbrotCenterColorB", mandelbrotCenterColor.b);
        file.loadValue("mandelbrotOuterColor1R", mandelbrotOuterColor1.r);
        file.loadValue("mandelbrotOuterColor1G", mandelbrotOuterColor1.g);
        file.loadValue("mandelbrotOuterColor1B", mandelbrotOuterColor1.b);
        file.loadValue("mandelbrotOuterColor2R", mandelbrotOuterColor2.r);
        file.loadValue("mandelbrotOuterColor2G", mandelbrotOuterColor2.g);
        file.loadValue("mandelbrotOuterColor2B", mandelbrotOuterColor2.b);
        file.loadValue("sharpeningQuality", sharpeningRadius);
        file.loadValue("sharpeningIntensity", sharpeningIntensity);
        file.loadValue("sharpeningThreshold", sharpeningThreshold);
    }
}
