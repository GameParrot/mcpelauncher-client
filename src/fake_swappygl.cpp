#include "fake_swappygl.h"

void FakeSwappyGL::initHooks(std::vector<mcpelauncher_hook_t>& hooks) {
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_init", (void*)+[]() -> bool { return true; }});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_destroy", (void*)+[]() -> void {}});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_getFenceTimeoutNS", (void*)+[]() -> uint64_t { return 0; }});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_getRefreshPeriodNanos", (void*)+[]() -> uint64_t { return 0; }});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_getSupportedRefreshPeriodsNS", (void*)+[]() -> void {}});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_getSwapIntervalNS", (void*)+[]() -> void {}});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_getUseAffinity", (void*)+[]() -> void {}});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_isEnabled", (void*)+[]() -> bool { return true; }});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_setBufferStuffingFixWait", (void*)+[]() -> void {}});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_setFenceTimeoutNS", (void*)+[]() -> void {}});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_setSwapIntervalNS", (void*)+[]() -> void {}});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_setUseAffinity", (void*)+[]() -> void {}});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_setWindow", (void*)+[]() -> bool { return true; }});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_enableFramePacing", (void*)+[]() -> void {}});
    hooks.emplace_back(mcpelauncher_hook_t{"SwappyGL_swap", (void*)+[](EGLDisplay display, EGLSurface surface) -> bool { return fake_egl::eglSwapBuffers(display, surface); }});
}