#include <filament/Engine.h>
#include <filament/InstanceBuffer.h>
#include <filament/RenderableManager.h>
#include <filament/Renderer.h>
#include <imgui.h>

// Link-only caller: the runner never executes this program or initializes a GPU.
int main(int argc, char**)
{
    if (argc > 1)
    {
        auto* engine = filament::Engine::create(filament::Engine::Backend::VULKAN);
        if (engine == nullptr)
        {
            return 1;
        }
        auto* renderer = engine->createRenderer();
        auto* instances = filament::InstanceBuffer::Builder(1).build(*engine);
        const auto capacity = engine->getMaxAutomaticInstances();
        filament::RenderableManager::Builder(1).instances(capacity);
        auto* context = ImGui::CreateContext();
        ImGui::DestroyContext(context);
        engine->destroy(instances);
        engine->destroy(renderer);
        filament::Engine::destroy(&engine);
    }
    return 0;
}
