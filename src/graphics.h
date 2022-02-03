#include "filament/RenderableManager.h"
#if defined(__linux)
#define GLFW_EXPOSE_NATIVE_X11
#endif

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WGL
#endif

#include "backends/imgui_impl_glfw.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <filagui/ImGuiHelper.h>
#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/SwapChain.h>
#include <filament/TransformManager.h>
#include <filament/View.h>
#include <filament/Viewport.h>
#include <filameshio/MeshReader.h>
#include <memory>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <utils/EntityManager.h>
#include <utils/Path.h>

#include "asset_library.h"

struct Graphics;

struct Renderable {
    Renderable(Graphics& graphics, ModelHandle model);

    utils::Entity entity;
};

struct Sun {
    Sun(Graphics& graphics);

    utils::Entity entity;
};

struct DirectionalLight {
    DirectionalLight(Graphics& graphics);

    utils::Entity entity;
};

struct Graphics {
    Graphics(GLFWwindow* window, ImGuiContext* context);
    filament::View* create_view();
    std::tuple<filament::View*, filament::Texture*>
    create_offscreen_view(uint32_t width, uint32_t height);
    void render(std::function<void()> imgui_commands);
    utils::Entity create_entity(ModelHandle model);
    ~Graphics();

    void bind(Scripting& scripting);

    filamesh::MeshReader::MaterialRegistry material_registry;
    filament::Engine* engine;
    filament::SwapChain* swap_chain;
    filament::Renderer* renderer;
    filament::Scene* scene;
    std::vector<filament::View*> views;
    std::vector<filament::View*> offscreen_views;
    std::vector<filament::RenderTarget*> offscreen_targets;
    std::vector<filament::Texture*> textures;
    filament::View* ui_view;
    std::shared_ptr<filagui::ImGuiHelper> imgui_helper;
};
