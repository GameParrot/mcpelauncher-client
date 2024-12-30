#pragma once
#include <vector>
#include <mutex>
#define __ANDROID__
#include <EGL/egl.h>
#undef __ANDROID__
namespace fake_egl {

void *eglGetProcAddress(const char *name);

}

struct FakeEGL {
    static void setProcAddrFunction(void *(*fn)(const char *));

    static void installLibrary();

    static void setupGLOverrides();

    static bool enableTexturePatch;

    struct SwapBuffersCallback {
        void *user;
        void (*callback)(void *user, EGLDisplay display, EGLSurface surface);
    };
    static std::vector<SwapBuffersCallback> swapBuffersCallbacks;
    static std::mutex swapBuffersCallbacksLock;
};
