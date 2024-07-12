#pragma once

#include <game_window.h>

struct LauncherOptions {
    int windowWidth, windowHeight;
    bool useStdinImport;
    GraphicsApi graphicsApi;
    std::string importFilePath;
    std::string sendUri;
};
extern LauncherOptions options;
