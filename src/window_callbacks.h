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
    struct KeyboardInputCallback {
        void *user;
        bool (*callback)(void *user, int keyCode, int action);
    };
    struct MouseButtonCallback {
        void *user;
        bool (*callback)(void *user, double x, double y, int button, int action);
    };
    struct MousePositionCallback {
        void *user;
        bool (*callback)(void *user, double x, double y, bool relative);
    };
    struct MouseScrollCallback {
        void *user;
        bool (*callback)(void *user, double x, double y, double dx, double dy);
    };

    struct GamepadButtonCallback {
        void *user;
        bool (*callback)(void *user, int btn, bool pressed);
    };
    struct GamepadAxisCallback {
        void *user;
        bool (*callback)(void *user, int ax, float value);
    };

    struct TouchEventCallback {
        void *user;
        bool (*callback)(void *user, int id, double x, double y);
    };

    std::vector<KeyboardInputCallback> keyboardCallbacks;
    std::vector<MouseButtonCallback> mouseButtonCallbacks;
    std::vector<MousePositionCallback> mousePositionCallbacks;
    std::vector<MouseScrollCallback> mouseScrollCallbacks;

    std::vector<GamepadButtonCallback> gamepadButtonCallbacks;
    std::vector<GamepadAxisCallback> gamepadAxisCallbacks;

    std::vector<TouchEventCallback> touchStartCallbacks;
    std::vector<TouchEventCallback> touchUpdateCallbacks;
    std::vector<TouchEventCallback> touchEndCallbacks;

    std::mutex keyboardCallbacksLock;
    std::mutex mouseButtonCallbacksLock;
    std::mutex mousePositionCallbacksLock;
    std::mutex mouseScrollCallbacksLock;

    std::mutex gamepadButtonCallbacksLock;
    std::mutex gamepadAxisCallbacksLock;

    std::mutex touchStartCallbacksLock;
    std::mutex touchUpdateCallbacksLock;
    std::mutex touchEndCallbacksLock;

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
    InputMode inputMode = InputMode::Unknown;
    InputMode forcedMode = InputMode::Unknown;
    int inputModeSwitchDelay = 100;
    std::chrono::high_resolution_clock::time_point lastUpdated;
    bool hasInputMode(InputMode want = InputMode::Unknown, bool changeMode = true);

    void queueGamepadAxisInputIfNeeded(int gamepad);

    void addTouchEventCallback(std::vector<TouchEventCallback> &callbacks, std::mutex &lock, void *user, bool (*callback)(void *user, int id, double x, double y));
    bool callTouchEventCallbacks(std::vector<TouchEventCallback> &callbacks, std::mutex &lock, int id, double x, double y);

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

    InputMode getInputMode();

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

    void addKeyboardCallback(void *user, bool (*callback)(void *user, int keyCode, int action));
    void addMouseButtonCallback(void *user, bool (*callback)(void *user, double x, double y, int button, int action));
    void addMousePositionCallback(void *user, bool (*callback)(void *user, double x, double y, bool relative));
    void addMouseScrollCallback(void *user, bool (*callback)(void *user, double x, double y, double dx, double dy));

    void addGamepadButtonCallback(void *user, bool (*callback)(void *user, int btn, bool pressed));
    void addGamepadAxisCallback(void *user, bool (*callback)(void *user, int ax, float value));

    void addTouchStartCallback(void *user, bool (*callback)(void *user, int id, double x, double y)) {
        addTouchEventCallback(touchStartCallbacks, touchStartCallbacksLock, user, callback);
    }
    void addTouchUpdateCallback(void *user, bool (*callback)(void *user, int id, double x, double y)) {
        addTouchEventCallback(touchUpdateCallbacks, touchUpdateCallbacksLock, user, callback);
    }
    void addTouchEndCallback(void *user, bool (*callback)(void *user, int id, double x, double y)) {
        addTouchEventCallback(touchEndCallbacks, touchEndCallbacksLock, user, callback);
    }

    static int mapMouseButtonToAndroid(int btn);
    static int mapMinecraftToAndroidKey(KeyCode code);
    static int mapGamepadToAndroidKey(GamepadButtonId btn);
#ifdef USE_IMGUI
    static ImGuiKey mapImGuiKey(KeyCode code);
#endif
};
