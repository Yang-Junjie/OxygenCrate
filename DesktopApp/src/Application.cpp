#include "Application.hpp"
#include "ExampleLayer.hpp"
#include <imgui.h>

class MyApp : public Flux::Application
{
public:
    MyApp(const Flux::ApplicationSpecification& spec): Flux::Application(spec)
    {
        PushLayer(std::make_unique<ExampleLayer>());
        SetMenubarCallback([this]()
                           {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit"))
                    {
                        Close();          
                    }
                    ImGui::EndMenu();
            } });
    }

    virtual ~MyApp() override = default;
};
std::unique_ptr<Flux::Application> Flux::CreateApplication()
{
    Flux::ApplicationSpecification spec;
    spec.Name = "MyApp";
    spec.Width = 1280;
    spec.Height = 720;
    return std::make_unique<MyApp>(spec);
}
