#include "AdjustmentSettings.hpp"
#include "../ZEditorsCommon/ZTB.hpp"

void glxy::AdjustmentSettings::Save() const
{
    JSON file;
    file.setValue("brightness", brightness);
    file.setValue("contrast", contrast);
    file.setValue("hue", hue);
    file.setValue("saturation", saturation);
    file.setValue("value", value);
    file.setValue("tintRed", tintRed);
    file.setValue("tintGreen", tintGreen);
    file.setValue("tintBlue", tintBlue);
    file.saveToFile("adj.json");
}

void glxy::AdjustmentSettings::Load()
{
    JSON file;
    if (file.loadFromFile("adj.json"))
    {
        file.loadValue("brightness", brightness);
        file.loadValue("contrast", contrast);
        file.loadValue("hue", hue);
        file.loadValue("saturation", saturation);
        file.loadValue("value", value);
        file.loadValue("tintRed", tintRed);
        file.loadValue("tintGreen", tintGreen);
        file.loadValue("tintBlue", tintBlue);
    }
}
