#include "asset_library.h"
#include "material.h"
#include "mesh.h"
#include "model.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/log.h"

#include <filament/Engine.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <fstream>
#undef assert_invariant
#include <nlohmann/json.hpp>
#include "primitives.h"
#include "scripting.h"

AssetLibrary::AssetLibrary(filament::Engine& engine) {
    animations.load = [](auto name) {
        auto filename = "assets/animations/" + name + ".ozz";
        ozz::io::File file(filename.c_str(), "rb");
        if (!file.opened()) {
            ozz::log::Err() << "Failed to open animation file '" << filename
                            << "''." << std::endl;
            std::exit(1);
        }
        ozz::io::IArchive archive(&file);
        if (!archive.TestTag<ozz::animation::Animation>()) {
            ozz::log::Err() << "Failed to load animation instance from file '"
                            << filename << "'." << std::endl;
            std::exit(1);
        }
        auto animation = new ozz::animation::Animation;
        archive >> *animation;
        return animation;
    };
    animations.unload = [](auto animation) { delete animation; };
    materials.load = [this](auto name) {
        auto filename = "assets/materials/" + name + ".json";
        std::ifstream file(filename);
        nlohmann::json j;
        file >> j;
        auto shader = shaders[j["shader"]];
        auto instance = shader->createInstance(name.c_str());
        auto param_count = shader->getParameterCount();
        std::vector<filament::Material::ParameterInfo> param_info(param_count);
        shader->getParameters(param_info.data(), param_count);
        for (auto& pi : param_info) {
            auto json_value = j[pi.name];
            switch (pi.type) {
            case filament::Material::ParameterType::FLOAT:
                instance->setParameter(pi.name, json_value.get<float>());
                break;
            case filament::Material::ParameterType::FLOAT2: {
                Vec2f value{json_value.at(0).get<float>(),
                            json_value.at(1).get<float>()};
                instance->setParameter(pi.name, value);
                break;
            }
            case filament::Material::ParameterType::FLOAT3: {
                // XXX instance->setParameter("baseColor",
                // filament::RgbType::sRGB, Value3f(0.8, 0, 0));
                Vec3f value{json_value.at(0).get<float>(),
                            json_value.at(1).get<float>(),
                            json_value.at(2).get<float>()};
                instance->setParameter(pi.name, value);
                break;
            }
            case filament::Material::ParameterType::FLOAT4: {
                Vec4f value{json_value.at(0).get<float>(),
                            json_value.at(1).get<float>(),
                            json_value.at(2).get<float>(),
                            json_value.at(3).get<float>()};
                instance->setParameter(pi.name, value);
                break;
            }
            case filament::Material::ParameterType::BOOL:
            case filament::Material::ParameterType::BOOL2:
            case filament::Material::ParameterType::BOOL3:
            case filament::Material::ParameterType::BOOL4:

            case filament::Material::ParameterType::INT:
            case filament::Material::ParameterType::INT2:
            case filament::Material::ParameterType::INT3:
            case filament::Material::ParameterType::INT4:
            case filament::Material::ParameterType::UINT:
            case filament::Material::ParameterType::UINT2:
            case filament::Material::ParameterType::UINT3:
            case filament::Material::ParameterType::UINT4:
            case filament::Material::ParameterType::MAT3:
            case filament::Material::ParameterType::MAT4:
            default:
                std::exit(1);
            }
        }
        return new Material{instance, shader};
    };
    materials.unload = [&engine](auto material) {
        engine.destroy(material->instance);
    };
    meshes.load = [&engine](auto name) {
        auto filename = "assets/meshes/" + name + ".glb";
        return new Mesh(engine, filename);
    };
    meshes.unload = [&engine](auto mesh) {
        for (auto& part : mesh->parts) {
            engine.destroy(part.index_buffer);
            engine.destroy(part.vertex_buffer);
        }
        delete mesh;
    };
    models.load = [this](auto name) {
        auto filename = "assets/models/" + name + ".json";
        std::ifstream file(filename);
        nlohmann::json json;
        file >> json;
        auto mesh = meshes[json["mesh"]];
        auto material = materials[json["material"]];
        auto skeleton = skeletons[json["skeleton"]];
        std::vector<AnimationHandle> model_anims;
        for (auto anim : json["animations"]) {
            model_anims.push_back(animations[anim]);
        }
        return new Model{mesh, material, skeleton, model_anims};
    };
    models.unload = [](auto model) { delete model; };
    shaders.load = [&engine](auto name) {
        auto filename = "assets/shaders/" + name + ".filamat";
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size))
            return filament::Material::Builder()
                .package(buffer.data(), buffer.size())
                .build(engine);
        else
            std::exit(1);
    };
    shaders.unload = [&engine](auto shader) { engine.destroy(shader); };
    skeletons.load =
        [](auto name) {
            auto filename = "assets/skeletons/" + name + ".ozz";
            ozz::io::File file(filename.c_str(), "rb");
            if (!file.opened()) {
                ozz::log::Err() << "Failed to open skeleton file '" << filename
                                << "''." << std::endl;
                std::exit(1);
            }
            ozz::io::IArchive archive(&file);
            if (!archive.TestTag<ozz::animation::Skeleton>()) {
                ozz::log::Err()
                    << "Failed to load skeleton instance from file '"
                    << filename << "'." << std::endl;
                std::exit(1);
            }
            auto skeleton = new ozz::animation::Skeleton;
            archive >> *skeleton;
            return skeleton;
        },
    skeletons.unload = [](auto skeleton) { delete skeleton; };
}

void AssetLibrary::bind(Scripting& scripting) {
    auto& lua = scripting.lua;
    lua["assets"] = lua.create_table();

    lua.new_usertype<AnimationHandle>("Animation");
    auto animations_table = lua["assets"]["animations"] = lua.create_table();
    auto animations_meta = animations_table[sol::metatable_key] = lua.create_table();
    animations_meta[sol::meta_method::index] = [this](sol::table, const std::string& name) { return animations[name]; };

    lua.new_usertype<MeshHandle>("Mesh");
    auto meshes_table = lua["assets"]["meshes"] = lua.create_table();
    auto meshes_meta = meshes_table[sol::metatable_key] = lua.create_table();
    meshes_meta[sol::meta_method::index] = [this](sol::table, const std::string& name) { return meshes[name]; };

    lua.new_usertype<ModelHandle>("Model");
    auto models_table = lua["assets"]["models"] = lua.create_table();
    auto models_meta = models_table[sol::metatable_key] = lua.create_table();
    models_meta[sol::meta_method::index] = [this](sol::table, const std::string& name) { return models[name]; };

    lua.new_usertype<ShaderHandle>("Shader");
    auto shaders_table = lua["assets"]["shaders"] = lua.create_table();
    auto shaders_meta = shaders_table[sol::metatable_key] = lua.create_table();
    shaders_meta[sol::meta_method::index] = [this](sol::table, const std::string& name) { return shaders[name]; };

    lua.new_usertype<MaterialHandle>("Material");
    auto materials_table = lua["assets"]["materials"] = lua.create_table();
    auto materials_meta = materials_table[sol::metatable_key] = lua.create_table();
    materials_meta[sol::meta_method::index] = [this](sol::table, const std::string& name) { return materials[name]; };

    lua.new_usertype<SkeletonHandle>("Skeleton");
    auto skeletons_table = lua["assets"]["skeletons"] = lua.create_table();
    auto skeletons_meta = skeletons_table[sol::metatable_key] = lua.create_table();
    skeletons_meta[sol::meta_method::index] = [this](sol::table, const std::string& name) { return skeletons[name]; };
}
