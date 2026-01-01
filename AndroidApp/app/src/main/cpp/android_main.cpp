#include "Application.hpp"
#include "imgui_impl_android.h"
#include <android_native_app_glue.h>

struct android_app* g_AndroidApp = nullptr;

static void WaitForWindow(struct android_app* state) {
    int events;
    struct android_poll_source* source;
    while (!state->window) {
        int result = ALooper_pollOnce(-1, nullptr, &events, (void**)&source);
        if (source) {
            source->process(state, source);
        }
        if (result == ALOOPER_POLL_ERROR || state->destroyRequested) {
            return;
        }
    }
}

static int32_t HandleInputEvent(struct android_app* app, AInputEvent* event) {
    return ImGui_ImplAndroid_HandleInputEvent(event);
}

void android_main(struct android_app* state) {
    g_AndroidApp = state;
    state->onInputEvent = HandleInputEvent;

    WaitForWindow(state);
    if (!state->window || state->destroyRequested) {
        return;
    }

    auto app = Flux::CreateApplication();
    if (app)
        app->Run();
}
