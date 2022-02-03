#include "graphics.h"
#include "filament/RenderTarget.h"
#include "filament/Texture.h"
#include "primitives.h"
#include <fstream>
#undef Success // macro from X11/X.h
#include "filament/LightManager.h"
#include "filament/RenderableManager.h"
#include "material.h"
#include "math/TVecHelpers.h"
#include "math/norm.h"
#include "mesh.h"
#include "model.h"
#include "scripting.h"

Renderable::Renderable(Graphics& graphics, ModelHandle model) {
    auto builder =
        filament::RenderableManager::Builder(model->mesh->parts.size());
    builder.boundingBox({{0, 0, 0}, {1, 1, 1}});
    for (size_t i = 0; i < model->mesh->parts.size(); i++) {
        auto& part = model->mesh->parts[i];
        builder.material(i, model->material->instance)
            .geometry(i, filament::RenderableManager::PrimitiveType::TRIANGLES,
                      part.vertex_buffer, part.index_buffer);
    }
    entity = utils::EntityManager::get().create();
    builder.skinning(model->mesh->inverse_binds.size()).build(*graphics.engine, entity);
    graphics.scene->addEntity(entity);
}

Sun::Sun(Graphics& graphics) {
    entity = utils::EntityManager::get().create();
    graphics.scene->addEntity(entity);
    filament::LightManager::Builder(filament::LightManager::Type::SUN)
        .color({0.98, 0.92, 0.89})
        .intensity(100.0)
        .direction({0.6, -1.0, -0.8})
        .sunAngularRadius(1.9)
        .sunHaloSize(10.0)
        .sunHaloFalloff(80.0)
        .build(*graphics.engine, entity);
}

DirectionalLight::DirectionalLight(Graphics& graphics) {
    entity = utils::EntityManager::get().create();
    graphics.scene->addEntity(entity);
    filament::LightManager::Builder(filament::LightManager::Type::DIRECTIONAL)
        .direction({-1, 0, 1})
        .intensity(50.0)
        .build(*graphics.engine, entity);
}

Graphics::Graphics(GLFWwindow* win, ImGuiContext* imgui_context) {
    int w, h;
    glfwGetWindowSize(win, &w, &h);
    uint32_t width = w, height = h;
    engine = filament::Engine::create(filament::backend::Backend::VULKAN,
                                      nullptr, nullptr);

    void* native_window = nullptr;
#if defined(__linux)
    native_window = (void*)glfwGetX11Window(win);
#elif defined(_WIN32)
    native_window = (void*)glfwGetWin32Window(win);
#endif

    swap_chain = engine->createSwapChain(native_window);
    renderer = engine->createRenderer();
    scene = engine->createScene();
    ui_view = engine->createView();
    ui_view->setViewport({0, 0, width, height});
    imgui_helper = std::make_shared<filagui::ImGuiHelper>(
        engine, ui_view, "ext/imgui/misc/fonts/DroidSans.ttf", imgui_context);
    imgui_helper->setDisplaySize(width, height, 1, 1);

    renderer->setClearOptions(
        {.clearColor = {0.6f, 0.8f, 1.0f, 1.0f}, .clear = true});
}

filament::View* Graphics::create_view() {
    const auto size = ui_view->getViewport();
    auto cam = engine->createCamera(utils::EntityManager::get().create());
    cam->setExposure(16.0f, 1 / 125.0f, 100.0f);
    cam->setExposure(100.0f);
    cam->setProjection(45.0f, float(size.width) / size.height, 0.1f, 100.0f);
    cam->lookAt({0, 0, 10.0}, {0, 0, 0}, {0, 1, 0});
    auto view = engine->createView();
    view->setViewport({0, 0, size.width, size.height});
    view->setScene(scene);
    view->setCamera(cam);
    views.push_back(view);
    return view;
}

std::tuple<filament::View*, filament::Texture*>
Graphics::create_offscreen_view(uint32_t width, uint32_t height) {
    auto view = engine->createView();
    view->setScene(scene);
    view->setPostProcessingEnabled(false);
    auto colorTexture = filament::Texture::Builder()
                            .width(width)
                            .height(height)
                            .levels(1)
                            .usage(filament::Texture::Usage::COLOR_ATTACHMENT |
                                   filament::Texture::Usage::SAMPLEABLE)
                            .format(filament::Texture::InternalFormat::RGBA8)
                            .build(*engine);
    auto depthTexture = filament::Texture::Builder()
                            .width(width)
                            .height(height)
                            .levels(1)
                            .usage(filament::Texture::Usage::DEPTH_ATTACHMENT)
                            .format(filament::Texture::InternalFormat::DEPTH24)
                            .build(*engine);
    auto renderTarget =
        filament::RenderTarget::Builder()
            .texture(filament::RenderTarget::AttachmentPoint::COLOR,
                     colorTexture)
            .texture(filament::RenderTarget::AttachmentPoint::DEPTH,
                     depthTexture)
            .build(*engine);
    view->setRenderTarget(renderTarget);
    view->setViewport({0, 0, width, height});
    auto camera = engine->createCamera(utils::EntityManager::get().create());
    camera->setExposure(16.0f, 1 / 125.0f, 100.0f);
    camera->setExposure(100.0f);
    camera->setProjection(45.0f, float(width) / height, 0.1f, 1000.0f);
    camera->lookAt({0, 0, 10}, {0, 0, 0}, {0, 1, 0});
    view->setCamera(camera);
    offscreen_views.push_back(view);
    offscreen_targets.push_back(renderTarget);
    textures.push_back(colorTexture);
    textures.push_back(depthTexture);
    return {view, colorTexture};
}

utils::Entity Graphics::create_entity(ModelHandle model) {
    auto builder =
        filament::RenderableManager::Builder(model->mesh->parts.size());
    builder.boundingBox({{0, 0, 0}, {1, 1, 1}});
    for (size_t i = 0; i < model->mesh->parts.size(); i++) {
        auto& part = model->mesh->parts[i];
        builder.material(i, model->material->instance)
            .geometry(i, filament::RenderableManager::PrimitiveType::TRIANGLES,
                      part.vertex_buffer, part.index_buffer);
    }
    auto entity = utils::EntityManager::get().create();
    builder.skinning(model->mesh->inverse_binds.size()).build(*engine, entity);
    this->scene->addEntity(entity);
    return entity;
}

void Graphics::render(std::function<void()> imgui_cmds) {
    if (renderer->beginFrame(swap_chain)) {
        for (auto view : offscreen_views)
            renderer->render(view);
        imgui_helper->render(0.016, [imgui_cmds](auto, auto) { imgui_cmds(); });
        for (auto view : views)
            renderer->render(view);
        renderer->render(ui_view);
        renderer->endFrame();
    }
}

Graphics::~Graphics() {
    for (auto view : views)
        engine->destroy(view->getCamera().getEntity());
    for (auto view : views)
        engine->destroy(view);
    for (auto view : offscreen_views)
        engine->destroy(view->getCamera().getEntity());
    for (auto view : offscreen_views)
        engine->destroy(view);
    for (auto target : offscreen_targets)
        engine->destroy(target);
    for (auto texture : textures)
        engine->destroy(texture);
    engine->destroy(ui_view);
    engine->destroy(scene);
    engine->destroy(renderer);
    engine->destroy(swap_chain);
    imgui_helper.reset();
    filament::Engine::destroy(&engine);
}

void Graphics::bind(Scripting& scripting) {
    auto& lua = scripting.lua;
    lua.new_usertype<Renderable>("Renderable", sol::meta_function::construct, [this](ModelHandle model) { return Renderable(*this, model); });
    lua.new_usertype<Sun>("Sun", sol::meta_function::construct, [this](ModelHandle model) { return Sun(*this); });
    lua.new_usertype<DirectionalLight>("DirectionalLight", sol::meta_function::construct, [this](ModelHandle model) { return DirectionalLight(*this); });
}
