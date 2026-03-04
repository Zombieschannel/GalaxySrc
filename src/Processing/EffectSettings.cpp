#include "EffectSettings.hpp"
#include "../ZEditorsCommon/ZTB.hpp"

void glxy::EffectSettings::Save() const
{
    JSON file;
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
    file.saveToFile("eff.json");
}

void glxy::EffectSettings::Load()
{
    JSON file;
    if (file.loadFromFile("eff.json"))
    {
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
    }
}
