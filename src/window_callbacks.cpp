#include "window_callbacks.h"
#include "symbols.h"

#include <mcpelauncher/minecraft_version.h>
#include <game_window_manager.h>
#include <log.h>
#include <mcpelauncher/path_helper.h>
#include <cstdlib>
#include <string>
#include "settings.h"
#include "util.h"

WindowCallbacks::WindowCallbacks(GameWindow& window, JniSupport& jniSupport, FakeInputQueue& inputQueue) : window(window), jniSupport(jniSupport), inputQueue(inputQueue) {
    useDirectMouseInput = Mouse::feed;
    useDirectKeyboardInput = (Keyboard::_states && (Keyboard::_inputs || Keyboard::_inputsLegacy) && Keyboard::_gameControllerId);
    if(Settings::fullscreen) {
        window.setFullscreen(true);
    }
    useRawInput = ReadEnvFlag("MCPELAUNCHER_CLIENT_RAW_INPUT");
    forcedMode = (InputMode)ReadEnvInt("MCPELAUNCHER_CLIENT_FORCED_INPUT_MODE", (int)forcedMode);
    inputModeSwitchDelay = ReadEnvInt("MCPELAUNCHER_CLIENT_INPUT_SWITCH_DELAY", inputModeSwitchDelay);
}

void WindowCallbacks::registerCallbacks() {
    using namespace std::placeholders;
    window.setWindowSizeCallback(std::bind(&WindowCallbacks::onWindowSizeCallback, this, _1, _2));
    window.setCloseCallback(std::bind(&WindowCallbacks::onClose, this));

    window.setMouseButtonCallback(std::bind(&WindowCallbacks::onMouseButton, this, _1, _2, _3, _4));
    window.setMousePositionCallback(std::bind(&WindowCallbacks::onMousePosition, this, _1, _2));
    window.setMouseRelativePositionCallback(std::bind(&WindowCallbacks::onMouseRelativePosition, this, _1, _2));
    window.setMouseScrollCallback(std::bind(&WindowCallbacks::onMouseScroll, this, _1, _2, _3, _4));
    window.setTouchStartCallback(std::bind(&WindowCallbacks::onTouchStart, this, _1, _2, _3));
    window.setTouchUpdateCallback(std::bind(&WindowCallbacks::onTouchUpdate, this, _1, _2, _3));
    window.setTouchEndCallback(std::bind(&WindowCallbacks::onTouchEnd, this, _1, _2, _3));
    window.setKeyboardCallback(std::bind(&WindowCallbacks::onKeyboard, this, _1, _2, _3));
    window.setKeyboardTextCallback(std::bind(&WindowCallbacks::onKeyboardText, this, _1));
    window.setDropCallback(std::bind(&WindowCallbacks::onDrop, this, _1));
    window.setPasteCallback(std::bind(&WindowCallbacks::onPaste, this, _1));
    window.setGamepadStateCallback(std::bind(&WindowCallbacks::onGamepadState, this, _1, _2));
    window.setGamepadButtonCallback(std::bind(&WindowCallbacks::onGamepadButton, this, _1, _2, _3));
    window.setGamepadAxisCallback(std::bind(&WindowCallbacks::onGamepadAxis, this, _1, _2, _3));
}

void WindowCallbacks::startSendEvents() {
    if(!sendEvents) {
        sendEvents = true;
        for(auto&& gp : gamepads) {
            jniSupport.setGameControllerConnected(gp.first, true);
        }
    }
    if(Settings::menubarsize != menubarsize) {
        menubarsize = Settings::menubarsize;
        int w, h;
        window.getWindowSize(w, h);
        onWindowSizeCallback(w, h);
    }
    if(delayedPaste > 0) {
        delayedPaste--;
        if(delayedPaste == 0) {
            jniSupport.getTextInputHandler().onTextInput("\x08");
            jniSupport.getTextInputHandler().onTextInput(lastPasteStr);
        }
    }
}

void WindowCallbacks::onWindowSizeCallback(int w, int h) {
    jniSupport.onWindowResized(w, h - Settings::menubarsize);
}

void WindowCallbacks::setCursorLocked(bool locked) {
    cursorLocked = locked;
    if(hasInputMode(InputMode::Mouse, false))
        window.setCursorDisabled(locked);
}

void WindowCallbacks::onClose() {
    jniSupport.onWindowClosed();
}

void WindowCallbacks::setFullscreen(bool isFs) {
    if(Settings::fullscreen != isFs) {
        window.setFullscreen(isFs);
        Settings::fullscreen = isFs;
        Settings::save();
    }
}

WindowCallbacks::InputMode WindowCallbacks::getInputMode() {
    return inputMode;
}

bool WindowCallbacks::hasInputMode(WindowCallbacks::InputMode want, bool changeMode) {
    if(!sendEvents) {
        return false;
    }
    if(useRawInput) {
        return true;
    }
    if(forcedMode != InputMode::Unknown) {
        return want == forcedMode;
    }
    auto now = std::chrono::high_resolution_clock::now();
    if(inputMode == want || (changeMode && ((int)want < (int)inputMode || (now - lastUpdated) > std::chrono::milliseconds(inputModeSwitchDelay)))) {
        if(inputMode != want) {
#ifndef NDEBUG
            printf("Input Mode changed to %d\n", (int)want);
#endif
            if(want == InputMode::Mouse) {
                window.setCursorDisabled(cursorLocked);
            } else {
                window.setCursorDisabled(true);
            }
        }
        inputMode = want;
        lastUpdated = now;
        return true;
    }
    return false;
}

void WindowCallbacks::onMouseButton(double x, double y, int btn, MouseButtonAction action) {
    if(hasInputMode(InputMode::Mouse)) {
        if(mouseButtonCallbacksLock.try_lock()) {
            for(size_t i = 0; i < mouseButtonCallbacks.size(); i++) {
                if(mouseButtonCallbacks[i].callback(mouseButtonCallbacks[i].user, x, y, (int)btn, (int)action)) {
                    mouseButtonCallbacksLock.unlock();
                    return;
                }
            }
            mouseButtonCallbacksLock.unlock();
        }
        if(btn < 1)
            return;
#ifdef USE_IMGUI
        if(ImGui::GetCurrentContext() && btn >= 1 && btn <= 3) {
            // High Mouse Buttons let this code crash
            ImGuiIO& io = ImGui::GetIO();
            io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
            io.AddMouseButtonEvent(btn - 1, action != MouseButtonAction::RELEASE);
            if(io.WantTextInput != imguiTextInput) {
                imguiTextInput = io.WantTextInput;
                if(io.WantTextInput) {
                    window.startTextInput();
                } else {
                    window.stopTextInput();
                }
            }
            if(io.WantCaptureMouse && !window.getCursorDisabled()) {
                return;
            }
        }
#endif
        if(options.emulateTouch) {
            if(jniSupport.isGameActivityVersion()) {
                sendTouchEvent(0, action == MouseButtonAction::PRESS ? AMOTION_EVENT_ACTION_DOWN : AMOTION_EVENT_ACTION_UP, x, y - Settings::menubarsize);
            } else {
                inputQueue.addEvent(FakeMotionEvent(AINPUT_SOURCE_TOUCHSCREEN, action == MouseButtonAction::PRESS ? AMOTION_EVENT_ACTION_DOWN : AMOTION_EVENT_ACTION_UP, 0, x, y - Settings::menubarsize));
            }
            return;
        }
        if(btn > 3) {
            // Seems to get recognized same as regular Mousebuttons as Button4 or higher, but ignored from mouse
            return onKeyboard((KeyCode)btn, action == MouseButtonAction::PRESS ? KeyAction::PRESS : KeyAction::RELEASE, 0);
        }
        if(useDirectMouseInput)
            Mouse::feed((char)btn, (char)(action == MouseButtonAction::PRESS ? 1 : 0), (short)x, (short)(y - Settings::menubarsize), 0, 0);
        else if(!jniSupport.isGameActivityVersion()) {
            if(action == MouseButtonAction::PRESS) {
                buttonState |= mapMouseButtonToAndroid(btn);
                inputQueue.addEvent(FakeMotionEvent(AINPUT_SOURCE_MOUSE, AMOTION_EVENT_ACTION_BUTTON_PRESS, 0, x, y - Settings::menubarsize, buttonState, 0));
            } else if(action == MouseButtonAction::RELEASE) {
                buttonState = buttonState & ~mapMouseButtonToAndroid(btn);
                inputQueue.addEvent(FakeMotionEvent(AINPUT_SOURCE_MOUSE, AMOTION_EVENT_ACTION_BUTTON_RELEASE, 0, x, y - Settings::menubarsize, buttonState, 0));
            }
        } else {
            if(action == MouseButtonAction::PRESS) {
                buttonState |= mapMouseButtonToAndroid(btn);
            } else {
                buttonState = buttonState & ~mapMouseButtonToAndroid(btn);
            }
            sendMouseEvent(AINPUT_SOURCE_MOUSE, 0, (action == MouseButtonAction::PRESS) ? AMOTION_EVENT_ACTION_BUTTON_PRESS : AMOTION_EVENT_ACTION_BUTTON_RELEASE, buttonState, x, y - Settings::menubarsize, 0);
        }
    }
}
void WindowCallbacks::onMousePosition(double x, double y) {
    if(hasInputMode(InputMode::Mouse)) {
        if(mousePositionCallbacksLock.try_lock()) {
            for(size_t i = 0; i < mousePositionCallbacks.size(); i++) {
                if(mousePositionCallbacks[i].callback(mousePositionCallbacks[i].user, x, y, false)) {
                    mousePositionCallbacksLock.unlock();
                    return;
                }
            }
            mousePositionCallbacksLock.unlock();
        }
#ifdef USE_IMGUI
        if(ImGui::GetCurrentContext()) {
            ImGuiIO& io = ImGui::GetIO();
            io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
            io.AddMousePosEvent(x, y);
            if(io.WantCaptureMouse && !window.getCursorDisabled()) {
                return;
            }
        }
#endif
        if(options.emulateTouch) {
            if(jniSupport.isGameActivityVersion()) {
                sendTouchEvent(0, AMOTION_EVENT_ACTION_MOVE, x, y - Settings::menubarsize);
            } else {
                inputQueue.addEvent(FakeMotionEvent(AINPUT_SOURCE_TOUCHSCREEN, AMOTION_EVENT_ACTION_MOVE, 0, x, y - Settings::menubarsize));
            }
            return;
        }
        if(useDirectMouseInput)
            Mouse::feed(0, 0, (short)x, (short)(y - Settings::menubarsize), 0, 0);
        else if(jniSupport.isGameActivityVersion()) {
            sendMouseEvent(AINPUT_SOURCE_MOUSE, 0, AMOTION_EVENT_ACTION_HOVER_MOVE, buttonState, x, y - Settings::menubarsize, 0);
        } else
            inputQueue.addEvent(FakeMotionEvent(AINPUT_SOURCE_MOUSE, AMOTION_EVENT_ACTION_HOVER_MOVE, 0, x, y - Settings::menubarsize, buttonState, 0));
    }
}
void WindowCallbacks::onMouseRelativePosition(double x, double y) {
    if(hasInputMode(InputMode::Mouse, std::abs(x) > 10 || std::abs(y) > 10)) {
        if(mousePositionCallbacksLock.try_lock()) {
            for(size_t i = 0; i < mousePositionCallbacks.size(); i++) {
                if(mousePositionCallbacks[i].callback(mousePositionCallbacks[i].user, x, y, true)) {
                    mousePositionCallbacksLock.unlock();
                    return;
                }
            }
            mousePositionCallbacksLock.unlock();
        }
        if(useDirectMouseInput)
            Mouse::feed(0, 0, 0, 0, (short)x, (short)y);
        else if(jniSupport.isGameActivityVersion()) {
            GameActivityMotionEvent event = {};
            sendMouseEvent(AINPUT_SOURCE_MOUSE_RELATIVE, 0, AMOTION_EVENT_ACTION_HOVER_MOVE, buttonState, x, y, 0);
        } else
            inputQueue.addEvent(FakeMotionEvent(AINPUT_SOURCE_MOUSE_RELATIVE, AMOTION_EVENT_ACTION_HOVER_MOVE, 0, x, y, buttonState, 0));
    }
}
void WindowCallbacks::onMouseScroll(double x, double y, double dx, double dy) {
    if(hasInputMode(InputMode::Mouse)) {
        if(mouseScrollCallbacksLock.try_lock()) {
            for(size_t i = 0; i < mouseScrollCallbacks.size(); i++) {
                if(mouseScrollCallbacks[i].callback(mouseScrollCallbacks[i].user, x, y, dx, dy)) {
                    mouseScrollCallbacksLock.unlock();
                    return;
                }
            }
            mouseScrollCallbacksLock.unlock();
        }
#ifdef USE_IMGUI
        if(ImGui::GetCurrentContext()) {
            ImGuiIO& io = ImGui::GetIO();
            io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
            io.AddMouseWheelEvent(dx, dy);
            if(io.WantCaptureMouse && !window.getCursorDisabled()) {
                return;
            }
        }
#endif
#ifdef __APPLE__
        signed char cdy = (signed char)std::max(std::min((dx + dy) * 127.0, 127.0), -127.0);
#else
        signed char cdy = (signed char)std::max(std::min(dy * 127.0, 127.0), -127.0);
#endif
        if(useDirectMouseInput)
            Mouse::feed(4, (char&)cdy, 0, 0, (short)x, (short)y - Settings::menubarsize);
        else if(jniSupport.isGameActivityVersion())
            sendMouseEvent(AINPUT_SOURCE_MOUSE, 0, AMOTION_EVENT_ACTION_SCROLL, buttonState, x, y - Settings::menubarsize, cdy);
        else
            inputQueue.addEvent(FakeMotionEvent(AINPUT_SOURCE_MOUSE, AMOTION_EVENT_ACTION_SCROLL, 0, x, y - Settings::menubarsize, buttonState, cdy));
    }
}

void WindowCallbacks::sendMouseEvent(int32_t source, int32_t deviceId, int32_t action, int32_t buttonState, float x, float y, float scrollY) {
    GameActivityMotionEvent event = {};
    event.source = source;
    event.deviceId = deviceId;
    event.action = action;
    event.buttonState = buttonState;
    event.precisionX = x;
    event.precisionY = y;
    event.pointerCount = 2;
    event.pointers[0].axisValues[AMOTION_EVENT_AXIS_X] = x;
    event.pointers[0].axisValues[AMOTION_EVENT_AXIS_Y] = y;
    event.pointers[0].rawX = x;
    event.pointers[0].rawY = x;
    event.pointers[0].axisValues[AMOTION_EVENT_AXIS_VSCROLL] = scrollY;

    jniSupport.sendMotionEvent(&event);
}

void WindowCallbacks::onTouchStart(int id, double x, double y) {
    if(hasInputMode(InputMode::Touch)) {
#ifdef USE_IMGUI
        if(ImGui::GetCurrentContext() && imGuiTouchId == -1) {
            imGuiTouchId = id;
            ImGuiIO& io = ImGui::GetIO();
            io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
            io.AddMousePosEvent(x, y);
            io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
            if(io.WantCaptureMouse) {
                return;
            }
        }
#endif
        if(jniSupport.isGameActivityVersion()) {
            sendTouchEvent(id, AMOTION_EVENT_ACTION_DOWN, x, y - Settings::menubarsize);
        } else {
            inputQueue.addEvent(FakeMotionEvent(AINPUT_SOURCE_TOUCHSCREEN, AMOTION_EVENT_ACTION_DOWN, id, x, y - Settings::menubarsize));
        }
    }
}
void WindowCallbacks::onTouchUpdate(int id, double x, double y) {
    if(hasInputMode(InputMode::Touch)) {
#ifdef USE_IMGUI
        if(ImGui::GetCurrentContext() && imGuiTouchId == id) {
            ImGuiIO& io = ImGui::GetIO();
            io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
            io.AddMousePosEvent(x, y);
            return;
        }
#endif
        if(jniSupport.isGameActivityVersion()) {
            sendTouchEvent(id, AMOTION_EVENT_ACTION_MOVE, x, y - Settings::menubarsize);
        } else {
            inputQueue.addEvent(FakeMotionEvent(AINPUT_SOURCE_TOUCHSCREEN, AMOTION_EVENT_ACTION_MOVE, id, x, y - Settings::menubarsize));
        }
    }
}
void WindowCallbacks::onTouchEnd(int id, double x, double y) {
    if(hasInputMode(InputMode::Touch)) {
#ifdef USE_IMGUI
        if(ImGui::GetCurrentContext() && imGuiTouchId == id) {
            imGuiTouchId = -1;
            ImGuiIO& io = ImGui::GetIO();
            io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
            io.AddMousePosEvent(x, y);
            io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
            return;
        }
#endif
        if(jniSupport.isGameActivityVersion()) {
            sendTouchEvent(id, AMOTION_EVENT_ACTION_UP, x, y - Settings::menubarsize);
        } else {
            inputQueue.addEvent(FakeMotionEvent(AINPUT_SOURCE_TOUCHSCREEN, AMOTION_EVENT_ACTION_UP, id, x, y - Settings::menubarsize));
        }
    }
}

void WindowCallbacks::sendTouchEvent(int32_t pointerId, int32_t action, float x, float y) {
    GameActivityMotionEvent ev = {};
    ev.source = AINPUT_SOURCE_TOUCHSCREEN;
    ev.action = action;
    ev.pointerCount = 1;
    ev.deviceId = 0;
    ev.pointers[0].id = pointerId;
    ev.pointers[0].axisValues[AMOTION_EVENT_AXIS_X] = x;
    ev.pointers[0].axisValues[AMOTION_EVENT_AXIS_Y] = y;
    ev.pointers[0].rawX = x;
    ev.pointers[0].rawY = y;
    jniSupport.sendMotionEvent(&ev);
}

static bool deadKey(KeyCode key) {
    switch(WindowCallbacks::mapMinecraftToAndroidKey(key)) {
    case AKEYCODE_DEL:
    case AKEYCODE_FORWARD_DEL:
    case AKEYCODE_SHIFT_LEFT:
    case AKEYCODE_SHIFT_RIGHT:
    case AKEYCODE_ALT_LEFT:
    case AKEYCODE_ALT_RIGHT:
    case AKEYCODE_CTRL_LEFT:
    case AKEYCODE_CTRL_RIGHT:
    case AKEYCODE_CAPS_LOCK:
    case AKEYCODE_META_LEFT:
    case AKEYCODE_META_RIGHT:
    case AKEYCODE_ESCAPE:
    case AKEYCODE_ENTER:
    case AKEYCODE_VOLUME_UP:
    case AKEYCODE_VOLUME_DOWN:
    case AKEYCODE_VOLUME_MUTE:
    case AKEYCODE_DPAD_LEFT:
    case AKEYCODE_DPAD_RIGHT:
    case AKEYCODE_DPAD_UP:
    case AKEYCODE_DPAD_UP_LEFT:
    case AKEYCODE_DPAD_UP_RIGHT:
    case AKEYCODE_DPAD_DOWN:
    case AKEYCODE_DPAD_DOWN_LEFT:
    case AKEYCODE_DPAD_DOWN_RIGHT:
    case AKEYCODE_UNKNOWN:
        return true;
    }
    return false;
}

#ifdef USE_IMGUI
ImGuiKey WindowCallbacks::mapImGuiKey(KeyCode code) {
    if(code >= KeyCode::NUM_0 && code <= KeyCode::NUM_9)
        return (ImGuiKey)((int)code - (int)KeyCode::NUM_0 + ImGuiKey_0);
    if(code >= KeyCode::NUMPAD_0 && code <= KeyCode::NUMPAD_9)
        return (ImGuiKey)((int)code - (int)KeyCode::NUMPAD_0 + ImGuiKey_Keypad0);
    if(code >= KeyCode::A && code <= KeyCode::Z)
        return (ImGuiKey)((int)code - (int)KeyCode::A + ImGuiKey_A);
    if(code >= KeyCode::FN1 && code <= KeyCode::FN12)
        return (ImGuiKey)((int)code - (int)KeyCode::FN1 + ImGuiKey_F1);
    switch(code) {
    case KeyCode::BACK:
        return ImGuiKey_AppBack;
    case KeyCode::BACKSPACE:
        return ImGuiKey_Backspace;
    case KeyCode::TAB:
        return ImGuiKey_Tab;
    case KeyCode::ENTER:
        return ImGuiKey_Enter;
    case KeyCode::LEFT_SHIFT:
        return ImGuiKey_LeftShift;
    case KeyCode::RIGHT_SHIFT:
        return ImGuiKey_RightShift;
    case KeyCode::LEFT_CTRL:
        return ImGuiKey_LeftCtrl;
    case KeyCode::RIGHT_CTRL:
        return ImGuiKey_RightCtrl;
    case KeyCode::PAUSE:
        return ImGuiKey_Pause;
    case KeyCode::CAPS_LOCK:
        return ImGuiKey_CapsLock;
    case KeyCode::ESCAPE:
        return ImGuiKey_Escape;
    case KeyCode::SPACE:
        return ImGuiKey_Space;
    case KeyCode::PAGE_UP:
        return ImGuiKey_PageUp;
    case KeyCode::PAGE_DOWN:
        return ImGuiKey_PageDown;
    case KeyCode::END:
        return ImGuiKey_End;
    case KeyCode::HOME:
        return ImGuiKey_Home;
    case KeyCode::LEFT:
        return ImGuiKey_LeftArrow;
    case KeyCode::UP:
        return ImGuiKey_UpArrow;
    case KeyCode::RIGHT:
        return ImGuiKey_RightArrow;
    case KeyCode::DOWN:
        return ImGuiKey_DownArrow;
    case KeyCode::INSERT:
        return ImGuiKey_Insert;
    case KeyCode::DELETE:
        return ImGuiKey_Delete;
    case KeyCode::NUM_LOCK:
        return ImGuiKey_NumLock;
    case KeyCode::SCROLL_LOCK:
        return ImGuiKey_ScrollLock;
    case KeyCode::SEMICOLON:
        return ImGuiKey_Semicolon;
    case KeyCode::EQUAL:
        return ImGuiKey_Equal;
    case KeyCode::COMMA:
        return ImGuiKey_Comma;
    case KeyCode::MINUS:
        return ImGuiKey_Minus;
    case KeyCode::NUMPAD_ADD:
        return ImGuiKey_KeypadAdd;
    case KeyCode::NUMPAD_SUBTRACT:
        return ImGuiKey_KeypadSubtract;
    case KeyCode::NUMPAD_MULTIPLY:
        return ImGuiKey_KeypadMultiply;
    case KeyCode::NUMPAD_DIVIDE:
        return ImGuiKey_KeypadDivide;
    case KeyCode::PERIOD:
        return ImGuiKey_Period;
    case KeyCode::NUMPAD_DECIMAL:
        return ImGuiKey_KeypadDecimal;
    case KeyCode::SLASH:
        return ImGuiKey_Slash;
    case KeyCode::GRAVE:
        return ImGuiKey_GraveAccent;
    case KeyCode::LEFT_BRACKET:
        return ImGuiKey_LeftBracket;
    case KeyCode::BACKSLASH:
        return ImGuiKey_Backslash;
    case KeyCode::RIGHT_BRACKET:
        return ImGuiKey_RightBracket;
    case KeyCode::APOSTROPHE:
        return ImGuiKey_Apostrophe;
    case KeyCode::MENU:
        return ImGuiKey_Menu;
    case KeyCode::LEFT_ALT:
        return ImGuiKey_LeftAlt;
    case KeyCode::RIGHT_ALT:
        return ImGuiKey_RightAlt;
    default:
        return ImGuiKey_None;
    }
}

static ImGuiKey mapImGuiModKey(KeyCode code) {
    switch(code) {
    case KeyCode::LEFT_SHIFT:
    case KeyCode::RIGHT_SHIFT:
        return ImGuiMod_Shift;
    case KeyCode::LEFT_CTRL:
    case KeyCode::RIGHT_CTRL:
        return ImGuiMod_Ctrl;
    case KeyCode::LEFT_ALT:
        return ImGuiMod_Alt;
    default:
        return ImGuiKey_None;
    }
}
#endif

void WindowCallbacks::onKeyboard(KeyCode key, KeyAction action, int mods) {
    if(hasInputMode(InputMode::Mouse)) {
        if(keyboardCallbacksLock.try_lock()) {
            for(size_t i = 0; i < keyboardCallbacks.size(); i++) {
                if(keyboardCallbacks[i].callback(keyboardCallbacks[i].user, (int)key, (int)action)) {
                    keyboardCallbacksLock.unlock();
                    return;
                }
            }
            keyboardCallbacksLock.unlock();
        }
#ifdef USE_IMGUI
        if(ImGui::GetCurrentContext()) {
            ImGuiIO& io = ImGui::GetIO();
            io.AddKeyEvent(mapImGuiModKey(key), action != KeyAction::RELEASE);
            io.AddKeyEvent(mapImGuiKey(key), action != KeyAction::RELEASE);
            if(io.WantTextInput != imguiTextInput) {
                imguiTextInput = io.WantTextInput;
                if(io.WantTextInput) {
                    window.startTextInput();
                } else {
                    window.stopTextInput();
                }
            }
            if(io.WantCaptureKeyboard || io.WantTextInput) {
                return;
            }
        }
#endif
// return onKeyboard((KeyCode) 4, KeyAction::PRESS);
// key = (KeyCode) 0x21;
#ifdef __APPLE__
        int modCTRL = mods & KEY_MOD_SUPER;
#else
        int modCTRL = mods & KEY_MOD_CTRL;
#endif
        modCTRL = (action != KeyAction::RELEASE);

        if(modCTRL && key == KeyCode::C && jniSupport.getTextInputHandler().getCopyText() != "") {
            window.setClipboardText(jniSupport.getTextInputHandler().getCopyText());
        } else {
            jniSupport.getTextInputHandler().onKeyPressed(key, action, mods);
        }

        if(key == KeyCode::FN11 && action == KeyAction::PRESS)
            setFullscreen(!Settings::fullscreen);

        if(useDirectKeyboardInput && (action == KeyAction::PRESS || action == KeyAction::RELEASE)) {
            if(Keyboard::useLegacyKeyboard) {
                Keyboard::LegacyInputEvent evData{};
                evData.key = (unsigned int)key & 0xff;
                evData.event = (action == KeyAction::PRESS ? 1 : 0);
                evData.controllerId = *Keyboard::_gameControllerId;
                Keyboard::_inputsLegacy->push_back(evData);
                Keyboard::_states[(int)key & 0xff] = evData.event;
            } else {
                Keyboard::InputEvent evData{};
                evData.modShift = Keyboard::_states[16];
                evData.modCtrl = Keyboard::_states[17];
                evData.modAlt = Keyboard::_states[18];
                evData.key = (unsigned int)key & 0xff;
                evData.event = (action == KeyAction::PRESS ? 1 : 0);
                evData.controllerId = *Keyboard::_gameControllerId;
                Keyboard::_inputs->push_back(evData);
                Keyboard::_states[(int)key & 0xff] = evData.event;
            }
            return;
        }

        int32_t state = 0;

        if(mods & KEY_MOD_SHIFT) {
            state |= AMETA_SHIFT_ON;
        }
        if(mods & KEY_MOD_ALT) {
            state |= AMETA_ALT_ON;
        }
        if(mods & KEY_MOD_CTRL) {
            state |= AMETA_CTRL_ON;
        }
        if(mods & KEY_MOD_SUPER) {
            state |= AMETA_META_ON;
        }
        if(mods & KEY_MOD_CAPSLOCK) {
            state |= AMETA_CAPS_LOCK_ON;
        }
        if(mods & KEY_MOD_NUMLOCK) {
            state |= AMETA_NUM_LOCK_ON;
        }

        if(jniSupport.isGameActivityVersion()) {
            GameActivityKeyEvent event = {};
            event.deviceId = 0;
            event.source = AINPUT_SOURCE_KEYBOARD;
            event.action = (action == KeyAction::PRESS) ? AKEY_EVENT_ACTION_DOWN : AKEY_EVENT_ACTION_UP;
            event.metaState = state;
            event.keyCode = mapMinecraftToAndroidKey(key);
            if(action == KeyAction::PRESS)
                jniSupport.sendKeyDown(&event);
            else if(action == KeyAction::RELEASE)
                jniSupport.sendKeyUp(&event);
        } else {
            if(action == KeyAction::PRESS)
                inputQueue.addEvent(FakeKeyEvent(AKEY_EVENT_ACTION_DOWN, mapMinecraftToAndroidKey(key), state));
            else if(action == KeyAction::RELEASE)
                inputQueue.addEvent(FakeKeyEvent(AKEY_EVENT_ACTION_UP, mapMinecraftToAndroidKey(key), state));
        }
    }
}
void WindowCallbacks::onKeyboardText(std::string const& c) {
#ifdef USE_IMGUI
    if(ImGui::GetCurrentContext()) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddInputCharactersUTF8(c.data());
        if(io.WantCaptureKeyboard) {
            return;
        }
    }
#endif
    if(c == "\n" && !jniSupport.getTextInputHandler().isMultiline())
        jniSupport.onReturnKeyPressed();
    else
        jniSupport.getTextInputHandler().onTextInput(c);
}
void WindowCallbacks::onDrop(std::string const& path) {
    jniSupport.importFile(path);
}
void WindowCallbacks::onPaste(std::string const& str) {
#ifdef USE_IMGUI
    Settings::clipboard = str;
#endif
    if(Settings::enable_keyboard_autofocus_paste_patches_1_20_60) {
        lastPasteStr = str;
    }
    jniSupport.getTextInputHandler().onTextInput(str);
}
void WindowCallbacks::onGamepadState(int gamepad, bool connected) {
    Log::trace("WindowCallbacks", "Gamepad %s #%i", connected ? "connected" : "disconnected", gamepad);
    if(connected)
        gamepads.insert({gamepad, GamepadData()});
    else
        gamepads.erase(gamepad);

    if(sendEvents) {
        // This crashs the game 1.16.210+ during init, but works after loading
        // We block sendEvents before the game starts polling the looper, to avoid the crash
        // 1.19.60+ requires calling this method, otherwise the game ignores the gamepad input
        jniSupport.setGameControllerConnected(gamepad, connected);
    }
}

void WindowCallbacks::queueGamepadAxisInputIfNeeded(int gamepad) {
    if(!needsQueueGamepadInput)
        return;
    if(jniSupport.isGameActivityVersion()) {
        auto gpi = gamepads.find(gamepad);
        if(gpi == gamepads.end())
            return;
        auto& gp = gpi->second;

        GameActivityMotionEvent ev = {};
        ev.source = AINPUT_SOURCE_GAMEPAD;
        ev.deviceId = gamepad;
        ev.action = AMOTION_EVENT_ACTION_MOVE;
        ev.pointerCount = 1;
        ev.pointers[0].id = 0;
        ev.pointers[0].axisValues[AMOTION_EVENT_AXIS_X] = gp.axis[(int)GamepadAxisId::LEFT_X];
        ev.pointers[0].axisValues[AMOTION_EVENT_AXIS_Y] = gp.axis[(int)GamepadAxisId::LEFT_Y];
        ev.pointers[0].axisValues[AMOTION_EVENT_AXIS_RX] = gp.axis[(int)GamepadAxisId::RIGHT_X];
        ev.pointers[0].axisValues[AMOTION_EVENT_AXIS_RY] = gp.axis[(int)GamepadAxisId::RIGHT_Y];
        ev.pointers[0].axisValues[AMOTION_EVENT_AXIS_BRAKE] = gp.axis[(int)GamepadAxisId::LEFT_TRIGGER];
        ev.pointers[0].axisValues[AMOTION_EVENT_AXIS_GAS] = gp.axis[(int)GamepadAxisId::RIGHT_TRIGGER];

        float hatX = 0;
        if(gp.button[(int)GamepadButtonId::DPAD_LEFT])
            hatX = -1.f;
        if(gp.button[(int)GamepadButtonId::DPAD_RIGHT])
            hatX = 1.f;
        ev.pointers[0].axisValues[AMOTION_EVENT_AXIS_HAT_X] = hatX;

        float hatY = 0;
        if(gp.button[(int)GamepadButtonId::DPAD_UP])
            hatY = -1.f;
        if(gp.button[(int)GamepadButtonId::DPAD_DOWN])
            hatY = 1.f;
        ev.pointers[0].axisValues[AMOTION_EVENT_AXIS_HAT_Y] = hatY;

        jniSupport.sendMotionEvent(&ev);
    } else {
        inputQueue.addEvent(FakeMotionEvent(AINPUT_SOURCE_GAMEPAD, gamepad, AMOTION_EVENT_ACTION_MOVE, 0, 0.f, 0.f,
                                            [this, gamepad](int axis) {
                                                auto gpi = gamepads.find(gamepad);
                                                if(gpi == gamepads.end())
                                                    return 0.f;
                                                auto& gp = gpi->second;
                                                if(axis == AMOTION_EVENT_AXIS_X)
                                                    return gp.axis[(int)GamepadAxisId::LEFT_X];
                                                if(axis == AMOTION_EVENT_AXIS_Y)
                                                    return gp.axis[(int)GamepadAxisId::LEFT_Y];
                                                if(axis == AMOTION_EVENT_AXIS_RX)
                                                    return gp.axis[(int)GamepadAxisId::RIGHT_X];
                                                if(axis == AMOTION_EVENT_AXIS_RY)
                                                    return gp.axis[(int)GamepadAxisId::RIGHT_Y];
                                                if(axis == AMOTION_EVENT_AXIS_BRAKE)
                                                    return gp.axis[(int)GamepadAxisId::LEFT_TRIGGER];
                                                if(axis == AMOTION_EVENT_AXIS_GAS)
                                                    return gp.axis[(int)GamepadAxisId::RIGHT_TRIGGER];
                                                if(axis == AMOTION_EVENT_AXIS_HAT_X) {
                                                    if(gp.button[(int)GamepadButtonId::DPAD_LEFT])
                                                        return -1.f;
                                                    if(gp.button[(int)GamepadButtonId::DPAD_RIGHT])
                                                        return 1.f;
                                                    return 0.f;
                                                }
                                                if(axis == AMOTION_EVENT_AXIS_HAT_Y) {
                                                    if(gp.button[(int)GamepadButtonId::DPAD_UP])
                                                        return -1.f;
                                                    if(gp.button[(int)GamepadButtonId::DPAD_DOWN])
                                                        return 1.f;
                                                    return 0.f;
                                                }
                                                return 0.f;
                                            }));
    }
    needsQueueGamepadInput = false;
}

void WindowCallbacks::onGamepadButton(int gamepad, GamepadButtonId btn, bool pressed) {
    if(hasInputMode(InputMode::Gamepad)) {
        auto gpi = gamepads.find(gamepad);
        if(gpi == gamepads.end())
            return;
        auto& gp = gpi->second;
        if((int)btn < 0 || (int)btn >= 15)
            throw std::runtime_error("bad button id");
        if(gp.button[(int)btn] == pressed)
            return;
        gp.button[(int)btn] = pressed;

        if(btn == GamepadButtonId::DPAD_UP || btn == GamepadButtonId::DPAD_DOWN || btn == GamepadButtonId::DPAD_LEFT || btn == GamepadButtonId::DPAD_RIGHT) {
            queueGamepadAxisInputIfNeeded(gamepad);
            return;
        }

        if(jniSupport.isGameActivityVersion()) {
            GameActivityKeyEvent event = {};
            event.deviceId = gamepad;
            event.source = AINPUT_SOURCE_GAMEPAD;
            event.action = pressed ? AKEY_EVENT_ACTION_DOWN : AKEY_EVENT_ACTION_UP;
            event.keyCode = mapGamepadToAndroidKey(btn);
            if(pressed)
                jniSupport.sendKeyDown(&event);
            else
                jniSupport.sendKeyUp(&event);
        } else {
            if(pressed)
                inputQueue.addEvent(FakeKeyEvent(AINPUT_SOURCE_GAMEPAD, gamepad, AKEY_EVENT_ACTION_DOWN, mapGamepadToAndroidKey(btn)));
            else
                inputQueue.addEvent(FakeKeyEvent(AINPUT_SOURCE_GAMEPAD, gamepad, AKEY_EVENT_ACTION_UP, mapGamepadToAndroidKey(btn)));
        }
    }
}

void WindowCallbacks::onGamepadAxis(int gamepad, GamepadAxisId ax, float value) {
    if(hasInputMode(InputMode::Gamepad, std::abs(value) > 0.4f)) {
        auto gpi = gamepads.find(gamepad);
        if(gpi == gamepads.end())
            return;
        auto& gp = gpi->second;
        if((int)ax < 0 || (int)ax >= 6)
            throw std::runtime_error("bad axis id");
        gp.axis[(int)ax] = value;
        queueGamepadAxisInputIfNeeded(gamepad);
    }
}

void WindowCallbacks::addKeyboardCallback(void* user, bool (*callback)(void* user, int keyCode, int action)) {
    keyboardCallbacksLock.lock();
    keyboardCallbacks.emplace_back(KeyboardInputCallback{.user = user, .callback = callback});
    keyboardCallbacksLock.unlock();
}

void WindowCallbacks::addMouseButtonCallback(void* user, bool (*callback)(void* user, double x, double y, int button, int action)) {
    mouseButtonCallbacksLock.lock();
    mouseButtonCallbacks.emplace_back(MouseButtonCallback{.user = user, .callback = callback});
    mouseButtonCallbacksLock.unlock();
}

void WindowCallbacks::addMousePositionCallback(void* user, bool (*callback)(void* user, double x, double y, bool relative)) {
    mousePositionCallbacksLock.lock();
    mousePositionCallbacks.emplace_back(MousePositionCallback{.user = user, .callback = callback});
    mousePositionCallbacksLock.unlock();
}

void WindowCallbacks::addMouseScrollCallback(void* user, bool (*callback)(void* user, double x, double y, double dx, double dy)) {
    mouseScrollCallbacksLock.lock();
    mouseScrollCallbacks.emplace_back(MouseScrollCallback{.user = user, .callback = callback});
    mouseScrollCallbacksLock.unlock();
}

void WindowCallbacks::setDelayedPaste() {
    delayedPaste = 2;
}

void WindowCallbacks::loadGamepadMappings() {
    auto windowManager = GameWindowManager::getManager();
    std::vector<std::string> controllerDbPaths;
    PathHelper::findAllDataFiles("gamecontrollerdb.txt", [&controllerDbPaths](std::string const& path) {
        controllerDbPaths.push_back(path);
    });
    // Bugfix: allow users to change internal gamepad layouts
    std::reverse(controllerDbPaths.begin(), controllerDbPaths.end());
    for(std::string const& path : controllerDbPaths) {
        Log::trace("Launcher", "Loading gamepad mappings: %s", path.c_str());
        windowManager->addGamepadMappingFile(path);
    }
}

WindowCallbacks::GamepadData::GamepadData() {
    for(int i = 0; i < 6; i++)
        axis[i] = 0.f;
    memset(button, 0, sizeof(button));
}

int WindowCallbacks::mapMouseButtonToAndroid(int btn) {
    switch(btn) {
    case 1:
        return AMOTION_EVENT_BUTTON_PRIMARY;
    case 2:
        return AMOTION_EVENT_BUTTON_SECONDARY;
    case 3:
        return AMOTION_EVENT_BUTTON_TERTIARY;
    case 8:
        return AMOTION_EVENT_BUTTON_BACK;
    case 9:
        return AMOTION_EVENT_BUTTON_FORWARD;
    }
    return btn;
}

int WindowCallbacks::mapMinecraftToAndroidKey(KeyCode code) {
    if(code >= KeyCode::NUM_0 && code <= KeyCode::NUM_9)
        return (int)code - (int)KeyCode::NUM_0 + AKEYCODE_0;
    if(code >= KeyCode::NUMPAD_0 && code <= KeyCode::NUMPAD_9)
        return (int)code - (int)KeyCode::NUMPAD_0 + AKEYCODE_NUMPAD_0;
    if(code >= KeyCode::A && code <= KeyCode::Z)
        return (int)code - (int)KeyCode::A + AKEYCODE_A;
    if(code >= KeyCode::FN1 && code <= KeyCode::FN12)
        return (int)code - (int)KeyCode::FN1 + AKEYCODE_F1;
    switch(code) {
    case KeyCode::BACK:
        return AKEYCODE_BACK;
    case KeyCode::BACKSPACE:
        return AKEYCODE_DEL;
    case KeyCode::TAB:
        return AKEYCODE_TAB;
    case KeyCode::ENTER:
        return AKEYCODE_ENTER;
    case KeyCode::LEFT_SHIFT:
        return AKEYCODE_SHIFT_LEFT;
    case KeyCode::RIGHT_SHIFT:
        return AKEYCODE_SHIFT_RIGHT;
    case KeyCode::LEFT_CTRL:
        return AKEYCODE_CTRL_LEFT;
    case KeyCode::RIGHT_CTRL:
        return AKEYCODE_CTRL_RIGHT;
    case KeyCode::PAUSE:
        return AKEYCODE_BREAK;
    case KeyCode::CAPS_LOCK:
        return AKEYCODE_CAPS_LOCK;
    case KeyCode::ESCAPE:
        return AKEYCODE_ESCAPE;
    case KeyCode::SPACE:
        return AKEYCODE_SPACE;
    case KeyCode::PAGE_UP:
        return AKEYCODE_PAGE_UP;
    case KeyCode::PAGE_DOWN:
        return AKEYCODE_PAGE_DOWN;
    case KeyCode::END:
        return AKEYCODE_MOVE_END;
    case KeyCode::HOME:
        return AKEYCODE_MOVE_HOME;
    case KeyCode::LEFT:
        return AKEYCODE_DPAD_LEFT;
    case KeyCode::UP:
        return AKEYCODE_DPAD_UP;
    case KeyCode::RIGHT:
        return AKEYCODE_DPAD_RIGHT;
    case KeyCode::DOWN:
        return AKEYCODE_DPAD_DOWN;
    case KeyCode::INSERT:
        return AKEYCODE_INSERT;
    case KeyCode::DELETE:
        return AKEYCODE_FORWARD_DEL;
    case KeyCode::NUM_LOCK:
        return AKEYCODE_NUM_LOCK;
    case KeyCode::SCROLL_LOCK:
        return AKEYCODE_SCROLL_LOCK;
    case KeyCode::SEMICOLON:
        return AKEYCODE_SEMICOLON;
    case KeyCode::EQUAL:
        return AKEYCODE_EQUALS;
    case KeyCode::COMMA:
        return AKEYCODE_COMMA;
    case KeyCode::MINUS:
        return AKEYCODE_MINUS;
    case KeyCode::NUMPAD_ADD:
        return AKEYCODE_NUMPAD_ADD;
    case KeyCode::NUMPAD_SUBTRACT:
        return AKEYCODE_NUMPAD_SUBTRACT;
    case KeyCode::NUMPAD_MULTIPLY:
        return AKEYCODE_NUMPAD_MULTIPLY;
    case KeyCode::NUMPAD_DIVIDE:
        return AKEYCODE_NUMPAD_DIVIDE;
    case KeyCode::PERIOD:
        return AKEYCODE_PERIOD;
    case KeyCode::NUMPAD_DECIMAL:
        return AKEYCODE_NUMPAD_DOT;
    case KeyCode::SLASH:
        return AKEYCODE_SLASH;
    case KeyCode::GRAVE:
        return AKEYCODE_GRAVE;
    case KeyCode::LEFT_BRACKET:
        return AKEYCODE_LEFT_BRACKET;
    case KeyCode::BACKSLASH:
        return AKEYCODE_BACKSLASH;
    case KeyCode::RIGHT_BRACKET:
        return AKEYCODE_RIGHT_BRACKET;
    case KeyCode::APOSTROPHE:
        return AKEYCODE_APOSTROPHE;
    case KeyCode::MENU:
        return AKEYCODE_MENU;
    case KeyCode::LEFT_SUPER:
        return AKEYCODE_META_LEFT;
    case KeyCode::RIGHT_SUPER:
        return AKEYCODE_META_RIGHT;
    case KeyCode::LEFT_ALT:
        return AKEYCODE_ALT_LEFT;
    case KeyCode::RIGHT_ALT:
        return AKEYCODE_ALT_RIGHT;
    default:
        return AKEYCODE_UNKNOWN;
    }
}

int WindowCallbacks::mapGamepadToAndroidKey(GamepadButtonId btn) {
    switch(btn) {
    case GamepadButtonId::A:
        return AKEYCODE_BUTTON_A;
    case GamepadButtonId::B:
        return AKEYCODE_BUTTON_B;
    case GamepadButtonId::X:
        return AKEYCODE_BUTTON_X;
    case GamepadButtonId::Y:
        return AKEYCODE_BUTTON_Y;
    case GamepadButtonId::LB:
        return AKEYCODE_BUTTON_L1;
    case GamepadButtonId::RB:
        return AKEYCODE_BUTTON_R1;
    case GamepadButtonId::BACK:
        return AKEYCODE_BUTTON_SELECT;
    case GamepadButtonId::START:
        return AKEYCODE_BUTTON_START;
    case GamepadButtonId::GUIDE:
        return AKEYCODE_BUTTON_MODE;
    case GamepadButtonId::LEFT_STICK:
        return AKEYCODE_BUTTON_THUMBL;
    case GamepadButtonId::RIGHT_STICK:
        return AKEYCODE_BUTTON_THUMBR;
    case GamepadButtonId::DPAD_UP:
        return AKEYCODE_DPAD_UP;
    case GamepadButtonId::DPAD_RIGHT:
        return AKEYCODE_DPAD_RIGHT;
    case GamepadButtonId::DPAD_DOWN:
        return AKEYCODE_DPAD_DOWN;
    case GamepadButtonId::DPAD_LEFT:
        return AKEYCODE_DPAD_LEFT;
    default:
        return -1;
    }
}
