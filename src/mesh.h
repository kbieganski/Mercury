#ifndef MODEL_LOADER_H_
#define MODEL_LOADER_H_
#include <vector>

#include "asset_library.h"
#include "filament/IndexBuffer.h"
#include "filament/RenderableManager.h"
#include "filament/VertexBuffer.h"
#include "ozz/base/maths/simd_math.h"
#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <unordered_map>

#include "primitives.h"

struct Mesh {
    Mesh(filament::Engine& engine, const std::string& path);

    std::vector<ozz::math::Float4x4> inverse_binds;
    std::unordered_map<std::string, uint16_t> bone_name_to_index;

    struct Part {
        filament::IndexBuffer* index_buffer;
        filament::VertexBuffer* vertex_buffer;
        std::vector<uint32_t> indices;
        std::vector<Vec3f> positions;
        std::vector<filament::math::short4> tangents;
        std::vector<filament::math::ushort4> bone_indices;
        std::vector<Vec4f> bone_weights;
    };
    std::vector<Part> parts;
};

#endif // MODEL_LOADER_H_
