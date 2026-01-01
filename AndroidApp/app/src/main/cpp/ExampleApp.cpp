#include "Application.hpp"
#include "ExampleLayer.hpp"
#include <android_native_app_glue.h>

extern android_app* g_AndroidApp;

namespace Flux {

std::unique_ptr<Application> CreateApplication() {
    ApplicationSpecification spec;
    spec.Name = "ExampleApp";
    spec.PlatformContext = g_AndroidApp;
    spec.ImGuiUIScale = 2.5f;

    auto app = std::make_unique<Application>(spec);
    app->PushLayer(std::make_unique<ExampleLayer>());
    return app;
}

} // namespace Flux
