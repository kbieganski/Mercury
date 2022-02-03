#include <fstream>
#include "entt/entity/fwd.hpp"
#include "graphics.h"
#include "imgui.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/options/options.h"
#include <cereal/cereal.hpp>
#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>
#include <entt/entt.hpp>
#undef Success // X11 defines this
#include "TextEditor.h"
#include <filament/RenderableManager.h>

#include "animator.h"
#include "scripting.h"
#include "transform.h"

void button_callback(GLFWwindow* win, int bt, int action, int mods);
void cursor_callback(GLFWwindow* win, double x, double y);
void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods);
void char_callback(GLFWwindow* win, unsigned int key);
void error_callback(int err, const char* desc);
void resize_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void key_callback(GLFWwindow* win, int key, int scancode, int action,
                  int mods) {
    if (GLFW_RELEASE == action) {
        return;
    }
    switch (key) {
        case GLFW_KEY_ESCAPE: {
            glfwSetWindowShouldClose(win, GL_TRUE);
            break;
        }
    };
}

void error_callback(int err, const char* desc) {
    printf("GLFW error: %s (%d)\n", desc, err);
}

void resize_callback(GLFWwindow* window, int width, int height) {}
void cursor_callback(GLFWwindow* win, double x, double y) {}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {}
void button_callback(GLFWwindow* win, int bt, int action, int mods) {}
void char_callback(GLFWwindow* win, unsigned int key) {}

//#ifdef DOCTEST_CONFIG_DISABLE
int main(int argc, char* argv[]) {
    glfwSetErrorCallback(error_callback);

    Scripting scripting;


    if (!glfwInit()) {
        printf("Error: cannot setup glfw.\n");
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    GLFWwindow* win = NULL;

    win = glfwCreateWindow(1600, 900, "Mercury", NULL, NULL);
    if (!win) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetFramebufferSizeCallback(win, resize_callback);
    glfwSetKeyCallback(win, key_callback);
    glfwSetCharCallback(win, char_callback);
    glfwSetCursorPosCallback(win, cursor_callback);
    glfwSetMouseButtonCallback(win, button_callback);
    glfwSetScrollCallback(win, scroll_callback);

    auto imgui_context = ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(win, true);

    {
        entt::registry registry;
        struct Entity { entt::registry::entity_type id; };

        Graphics graphics(win, imgui_context);
        AssetLibrary assets(*graphics.engine);
        Animator animator;

        scripting.lua.new_usertype<Entity>("Entity",
            sol::meta_function::construct, [&registry]() { return Entity{ registry.create() }; },
            "add", sol::overload(
                // TODO: automate this
                [&registry](Entity entity, Transform& component) -> Transform& { return registry.emplace<Transform>(entity.id, component); },
                [&registry](Entity entity, Renderable& component) -> Renderable& { return registry.emplace<Renderable>(entity.id, component); },
                [&registry](Entity entity, Sun& component) -> Sun& { return registry.emplace<Sun>(entity.id, component); },
                [&registry](Entity entity, DirectionalLight& component) -> DirectionalLight& { return registry.emplace<DirectionalLight>(entity.id, component); },
                [&registry](Entity entity, SkeletalAnimation& component) -> SkeletalAnimation& { return registry.emplace<SkeletalAnimation>(entity.id, component); }
            ));
        Transform::bind(scripting);
        assets.bind(scripting);
        graphics.bind(scripting);
        animator.bind(scripting);
        scripting.load_scripts("assets/scripts");

        auto sun = registry.create();
        registry.emplace<Sun>(sun, graphics);

        auto dir_light = registry.create();
        registry.emplace<DirectionalLight>(dir_light, graphics);

        auto [ov, tx] = graphics.create_offscreen_view(960, 720);

        TextEditor editor;
        editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());

        std::string current_file;
        auto start_time = std::chrono::high_resolution_clock::now();
        auto last_time = std::chrono::high_resolution_clock::now();
        while (!glfwWindowShouldClose(win)) {
            auto new_time = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration_cast<std::chrono::duration<float>>(new_time - last_time).count();
            float elapsed_time = std::chrono::duration_cast<std::chrono::duration<float>>(new_time - start_time).count();
            last_time = new_time;

            animator.update(dt, registry);
            animator.update_renderables(registry, graphics);
            Transform::propagate_transforms(registry, graphics);

            for (auto& view : graphics.offscreen_views)
                view->getCamera().lookAt(
                    filament::math::mat3f::rotation(
                        elapsed_time, filament::math::float3{0, 1, 0}) *
                        Vec3f{20, 0, 0},
                    {0, 0, 0}, {0, 1, 0});

            graphics.render([tx = tx, &current_file, &scripting, &editor]() mutable {
                ImGui_ImplGlfw_NewFrame();
                ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
                ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x,
                                                ImGui::GetIO().DisplaySize.y));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0));
                ImGui::Begin(
                    "##MainWindow", NULL,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoBringToFrontOnFocus);
                ImGui::PopStyleVar(2);
                {
                    if (ImGui::BeginTabBar("")) {
                        if (ImGui::BeginTabItem("Scene")) {
                            ImGui::Image((void*)tx, ImVec2(940, 700));
                            ImGui::EndTabItem();
                        }
                        if (ImGui::BeginTabItem("Scripting")) {
                            ImGui::Columns(2, nullptr, false);
                            ImGui::SetColumnWidth(0, 300);
                            ImGui::ListBoxHeader("##Scripts");
                            for (const auto& script : scripting.loaded) {
                                if (ImGui::Selectable(script.path.c_str(), script.path == current_file)) {
                                    std::ifstream script_file(script.path.c_str());
                                    editor.SetText({std::istreambuf_iterator<char>(script_file), std::istreambuf_iterator<char>()});
                                }
                            }
                            ImGui::ListBoxFooter();
                            ImGui::NextColumn();
                            editor.Render("");
                            ImGui::Columns(1);
                            ImGui::EndTabItem();
                        }
                        ImGui::EndTabBar();
                    }
                }
                ImGui::End();
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            glfwPollEvents();
        }
    }
    glfwTerminate();
    return EXIT_SUCCESS;
}
