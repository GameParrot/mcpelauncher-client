#pragma once
#include <string>
#include <optional>

struct Settings {
    static std::optional<bool> enable_imgui;
    static int menubarsize;
    static std::string clipboard;
    static bool enable_keyboard_tab_patches_1_20_60;
    static bool enable_keyboard_autofocus_patches_1_20_60;
    static bool enable_menubar;
    static int fps_hud_location;
    static int keystroke_mouse_hud_location;
    static std::string videoMode;
    static float scale;
    static std::string menubarFocusKey;

    static bool fullscreen;

    static std::string getPath();
    static void load();
    static void save();
};

struct GameOptions {
    static char leftKey;
    static char downKey;
    static char rightKey;
    static char upKey;

    static char leftKeyFullKeyboard;
    static char downKeyFullKeyboard;
    static char rightKeyFullKeyboard;
    static char upKeyFullKeyboard;

    static bool fullKeyboard;
};