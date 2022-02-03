#ifndef ASSET_LIBRARY_H
#define ASSET_LIBRARY_H
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

template <typename Asset> struct Library {
    struct Wrapper {
        Asset* asset;
        mutable size_t ref_count = 0;
    };

    struct Handle {
        Handle() : wrapper(nullptr) {}
        Handle(Wrapper* _wrapper) : wrapper(_wrapper) { ++wrapper->ref_count; }
        ~Handle() { --wrapper->ref_count; }

        operator bool() const { return wrapper; }
        operator Asset*() { return wrapper->asset; }
        operator const Asset*() const { return wrapper->asset; }
        Asset* operator->() { return wrapper->asset; }
        const Asset* operator->() const { return wrapper->asset; }
        Asset& operator*() { return *wrapper->asset; }
        const Asset& operator*() const { return *wrapper->asset; }

        Wrapper* wrapper;
    };

    ~Library() {
        for (auto a : assets)
            unload(a.second->asset);
    }

    Handle operator[](const std::string& name) {
        auto it = assets.find(name);
        if (it != assets.end())
            return {it->second};
        auto wrapper = assets[name] = new Wrapper;
        wrapper->asset = load(name);
        return {wrapper};
    }

    void release_unused() {
        std::vector<
            typename std::unordered_map<std::string, Wrapper*>::iterator>
            unused;
        for (auto it = assets.begin(); it != assets.end(); ++it) {
            if (it->second->ref_count == 0) {
                unload(it->second->asset);
                unused.push_back(it);
            }
        }
        for (auto it : unused)
            assets.erase(it);
    }

    std::function<Asset*(const std::string&)> load;
    std::function<void(Asset*)> unload;

    std::unordered_map<std::string, Wrapper*> assets;
};

namespace ozz {
    namespace animation {
        class Animation;
        class Skeleton;
    }
}

namespace filament {
    class Engine;
    class Material;
    class MaterialInstance;
}

using Animation = ozz::animation::Animation;
using AnimationHandle = Library<Animation>::Handle;

struct Material;
using MaterialHandle = Library<Material>::Handle;

struct Mesh;
using MeshHandle = Library<Mesh>::Handle;

struct Model;
using ModelHandle = Library<Model>::Handle;

using Shader = filament::Material;
using ShaderHandle = Library<Shader>::Handle;

using Skeleton = ozz::animation::Skeleton;
using SkeletonHandle = Library<Skeleton>::Handle;

struct Scripting;

struct AssetLibrary {
    AssetLibrary(filament::Engine& engine);

    Library<Animation> animations;
    Library<Mesh> meshes;
    Library<Model> models;
    Library<Shader> shaders;
    Library<Material> materials;
    Library<Skeleton> skeletons;

    void release_unused();
    void bind(Scripting& scripting);
};

#endif
