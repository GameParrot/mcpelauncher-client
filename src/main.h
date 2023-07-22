#pragma once

#include <game_window.h>

struct LauncherOptions {
    int windowWidth, windowHeight;
    bool useStdinImport;
    bool fullscreen;
    GraphicsApi graphicsApi;
    std::string importFilePath;
    std::string alsaDev;
    int alsaLat;
};
extern LauncherOptions options;
