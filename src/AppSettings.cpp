#include "AppSettings.hpp"
#include "Global.hpp"
#include "Languages.hpp"
#include "Namespace.hpp"
#include "Shortcuts.hpp"
#include "inc/ZTB.hpp"

void glxy::AppSettings::Save() const
{
    JSON file;
    file.setValue("theme", GLOBAL.themeID);
    file.setValue("resX", resolution.x);
    file.setValue("resY", resolution.y);
    file.setValue("fullscreen", fullscreen);
    file.setValue("menuScale", GUIScale);
    file.setValue("verticalSync", verticalSync);
    file.setValue("outOfFocus", outOfFocus);
    file.setValue("bgColorR", bgColor.r);
    file.setValue("bgColorG", bgColor.g);
    file.setValue("bgColorB", bgColor.b);
    file.setValue("antialiasing", antialiasing);
    file.setValue("maxFPS", maxFPS);
    file.setValue("language", languageID);
    file.setValue("touchPadSupport", touchPadSupport);
    file.setValue("animateZoom", animateZoom);
    file.setValue("animatePan", animatePan);
    file.setValue("aspectResize", maintainAspectResize);
    file.setValue("aspectCanvas", maintainAspectCanvas);
    file.setValue("resamplingMethod", resamplingMethod);
    file.setValue("bucketTolerance", bucketTolerance);
    file.setValue("wandTolerance", wandTolerance);
    file.setValue("syncViewport", syncViewport);
    file.setValue("showFPS", showFPS);
    file.setValue("showGrid", showGrid);
    file.setValue("showRuler", showRuler);
    file.setValue("drawSelectionLines", drawSelectionLines);
    file.setValue("panMouseButton", panMouseButton);
    file.setValue("recentFileCount", static_cast<int32_t>(recentFiles.size()));
    int8_t counter = 0;
    for (auto& n : recentFiles)
    {
        file.setValueStr("recentFile_" + to_string(counter), n.second);
        file.setValue("recentFile_" + to_string(counter) + "_ID", n.first);
        counter++;
    }
    for (int8_t i = 0; i < static_cast<int32_t>(ActionShortcut::Count); i++)
        file.setValue(("shortcut_" + to_string(i)), Shortcuts::getShortcut(static_cast<ActionShortcut>(i)));
    file.saveToFile("sett.json");
}

void glxy::AppSettings::Load()
{
    JSON file;
    if (file.loadFromFile("sett.json"))
    {
        file.loadValue("theme", GLOBAL.themeID);
        file.loadValue("resX", resolution.x);
        file.loadValue("resY", resolution.y);
        file.loadValue("fullscreen", fullscreen);
        file.loadValue("menuScale", GUIScale);
        file.loadValue("verticalSync", verticalSync);
        file.loadValue("outOfFocus", outOfFocus);
        file.loadValue("bgColorR", bgColor.r);
        file.loadValue("bgColorG", bgColor.g);
        file.loadValue("bgColorB", bgColor.b);
        file.loadValue("antialiasing", antialiasing);
        file.loadValue("maxFPS", maxFPS);
        file.loadValue("language", languageID);
        file.loadValue("touchPadSupport", touchPadSupport);
        file.loadValue("animateZoom", animateZoom);
        file.loadValue("animatePan", animatePan);
        file.loadValue("aspectResize", maintainAspectResize);
        file.loadValue("aspectCanvas", maintainAspectCanvas);
        file.loadValue("resamplingMethod", resamplingMethod);
        file.loadValue("bucketTolerange", bucketTolerance);
        file.loadValue("wandTolerance", wandTolerance);
        file.loadValue("syncViewport", syncViewport);
        file.loadValue("showFPS", showFPS);
        file.loadValue("showGrid", showGrid);
        file.loadValue("showRuler", showRuler);
        file.loadValue("drawSelectionLines", drawSelectionLines);
        file.loadValue("panMouseButton", panMouseButton);

        int8_t recentCount = 0;
        file.loadValue("recentFileCount", recentCount);
        recentFiles.clear();
        for (int8_t i = 0; i < recentCount; i++)
        {
            int64_t ID;
            string data;
            file.loadValue("recentFile_" + to_string(i), data);
            file.loadValue("recentFile_" + to_string(i) + "_ID", ID);
            recentFiles.emplace(ID, data);
        }

        for (int8_t i = 0; i < static_cast<int32_t>(ActionShortcut::Count); i++)
        {
            uint32_t val = -1;
            file.loadValue(("shortcut_" + to_string(i)), val);
            if (val != -1)
                Shortcuts::setShortcut(static_cast<ActionShortcut>(i), val);
        }
        LL::setLanguageID(languageID);
    }
}