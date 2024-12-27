#pragma once

#include <game_window.h>
#include <unordered_map>
#include "jni/jni_support.h"
#include "fake_inputqueue.h"
#include <chrono>
#include <vector>
#include <mutex>
#include "main.h"
#ifdef USE_IMGUI
#include <imgui.h>
#endif
class WindowCallbacks {
private:
    struct GamepadData {
        float axis[6];
        bool button[15];

        GamepadData();
    };
    struct KeyboardInputHook {
        void *user;
        bool (*hook)(void *user, int keyCode, int action);
    };
    struct MouseClickHook {
        void *user;
        bool (*hook)(void *user, double x, double y, int button, int action);
    };
    struct MousePositionHook {
        void *user;
        bool (*hook)(void *user, double x, double y, bool relative);
    };
    struct MouseScrollHook {
        void *user;
        bool (*hook)(void *user, double x, double y, double dx, double dy);
    };

    std::vector<KeyboardInputHook> keyboardHooks;
    std::vector<MouseClickHook> mouseClickHooks;
    std::vector<MousePositionHook> mousePositionHooks;
    std::vector<MouseScrollHook> mouseScrollHooks;
    std::mutex keyboardHooksLock;
    std::mutex mouseClickHooksLock;
    std::mutex mousePositionHooksLock;
    std::mutex mouseScrollHooksLock;

    GameWindow &window;
    JniSupport &jniSupport;
    FakeInputQueue &inputQueue;
    std::unordered_map<int, GamepadData> gamepads;
    int32_t buttonState = 0;
    KeyCode lastKey = (KeyCode)0;
    size_t lastEnabledNo = 0;
    int32_t metaState = 0;
    bool useDirectMouseInput, useDirectKeyboardInput;
    bool modCTRL = false;
    bool needsQueueGamepadInput = true;
    bool sendEvents = false;
    bool cursorLocked = false;
    bool imguiTextInput = false;
    int menubarsize = 0;
    enum class InputMode {
        Touch,
        Mouse,
        Gamepad,
        Unknown,
    };
    int imGuiTouchId = -1;
    bool useRawInput = false;
    InputMode forcedMode = InputMode::Unknown;
    int inputModeSwitchDelay = 100;
    std::chrono::high_resolution_clock::time_point lastUpdated;
    bool hasInputMode(InputMode want = InputMode::Unknown, bool changeMode = true);

    void queueGamepadAxisInputIfNeeded(int gamepad);

public:
    WindowCallbacks(GameWindow &window, JniSupport &jniSupport, FakeInputQueue &inputQueue);

    static void loadGamepadMappings();

    void registerCallbacks();

    void startSendEvents();

    void markRequeueGamepadInput() { needsQueueGamepadInput = true; }

    void onWindowSizeCallback(int w, int h);

    void setCursorLocked(bool locked);

    void onClose();

    void setFullscreen(bool isFs);

    void onMouseButton(double x, double y, int btn, MouseButtonAction action);
    void onMousePosition(double x, double y);
    void onMouseRelativePosition(double x, double y);
    void onMouseScroll(double x, double y, double dx, double dy);
    void onTouchStart(int id, double x, double y);
    void onTouchUpdate(int id, double x, double y);
    void onTouchEnd(int id, double x, double y);
    void onKeyboard(KeyCode key, KeyAction action);
    void onKeyboardText(std::string const &c);
    void onPaste(std::string const &str);
    void onGamepadState(int gamepad, bool connected);
    void onGamepadButton(int gamepad, GamepadButtonId btn, bool pressed);
    void onGamepadAxis(int gamepad, GamepadAxisId ax, float value);

    void addKeyboardHook(void *user, bool (*hook)(void *user, int keyCode, int action));
    void addMouseClickHook(void *user, bool (*hook)(void *user, double x, double y, int button, int action));
    void addMousePositionHook(void *user, bool (*hook)(void *user, double x, double y, bool relative));
    void addMouseScrollHook(void *user, bool (*hook)(void *user, double x, double y, double dx, double dy));

    static int mapMouseButtonToAndroid(int btn);
    static int mapMinecraftToAndroidKey(KeyCode code);
    static int mapGamepadToAndroidKey(GamepadButtonId btn);

    InputMode inputMode = InputMode::Unknown;
#ifdef USE_IMGUI
    static ImGuiKey mapImGuiKey(KeyCode code);
#endif
};
