#include "mesh.h"
#include "math/norm.h"
#include "primitives.h"
#include <iostream>

Mesh::Mesh(filament::Engine& engine, const std::string& path) {
    Assimp::Importer importer;
    auto scene = importer.ReadFile(
        path, aiProcess_LimitBoneWeights | aiProcess_Triangulate |
                  aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

    for (size_t i = 0; i < scene->mNumMeshes; i++) {
        auto mesh = scene->mMeshes[i];
        for (size_t j = 0; j < mesh->mNumBones; j++) {
            auto bone = mesh->mBones[j];
            aiMatrix4x4 inverse_bind = bone->mOffsetMatrix;
            inverse_bind.Transpose();
            if (bone_name_to_index.find(bone->mName.C_Str()) ==
                bone_name_to_index.end()) {
                bone_name_to_index.insert(
                    std::make_pair(bone->mName.C_Str(), inverse_binds.size()));
                inverse_binds.push_back(
                    reinterpret_cast<ozz::math::Float4x4&>(inverse_bind));
            }
        }
    }

    parts.resize(scene->mNumMeshes);
    for (size_t i = 0; i < scene->mNumMeshes; i++) {
        auto mesh = scene->mMeshes[i];
        parts[i].index_buffer =
            filament::IndexBuffer::Builder()
                .indexCount(mesh->mNumFaces * 3)
                .bufferType(filament::IndexBuffer::IndexType::UINT)
                .build(engine);

        for (size_t j = 0; j < mesh->mNumFaces; j++) {
            auto& face = mesh->mFaces[j];
            parts[i].indices.push_back(face.mIndices[0]);
            parts[i].indices.push_back(face.mIndices[1]);
            parts[i].indices.push_back(face.mIndices[2]);
        }
        parts[i].index_buffer->setBuffer(
            engine, filament::backend::BufferDescriptor(
                        parts[i].indices.data(),
                        sizeof(uint32_t) * parts[i].indices.size()));

        for (size_t j = 0; j < mesh->mNumVertices; j++) {
            auto& vertex = mesh->mVertices[j];
            parts[i].positions.push_back({vertex.x, vertex.y, vertex.z});
        }
        Vec3f* tangents = reinterpret_cast<Vec3f*>(mesh->mTangents);
        Vec3f* bitangents = reinterpret_cast<Vec3f*>(mesh->mBitangents);
        Vec3f* normals = reinterpret_cast<Vec3f*>(mesh->mNormals);
        for (size_t j = 0; j < mesh->mNumVertices; j++) {
            Vec3f normal = normals[j];
            Vec3f tangent;
            Vec3f bitangent;
            if (!tangents) {
                bitangent = normalize(cross(normal, Vec3f{1.0, 0.0, 0.0}));
                tangent = normalize(cross(normal, bitangent));
            } else {
                tangent = tangents[j];
                bitangent = bitangents[j];
            }
            filament::math::quatf q =
                filament::math::details::TMat33<float>::packTangentFrame(
                    {tangent, bitangent, normal});
            parts[i].tangents.push_back(filament::math::packSnorm16(q.xyzw));
        }
        if (mesh->mNumBones > 0) {
            parts[i].bone_indices.resize(mesh->mNumVertices, {0, 0, 0, 0});
            parts[i].bone_weights.resize(mesh->mNumVertices, {0, 0, 0, 0});
            for (size_t j = 0; j < mesh->mNumBones; j++) {
                auto bone = mesh->mBones[j];
                for (size_t k = 0; k < bone->mNumWeights; k++) {
                    auto vertex_index = bone->mWeights[k].mVertexId;
                    auto bone_index =
                        bone_name_to_index.at(bone->mName.C_Str());
                    auto& bi = parts[i].bone_indices[vertex_index];
                    auto& bw = parts[i].bone_weights[vertex_index];
                    float weight = bone->mWeights[k].mWeight;
                    if (weight > bw.w) {
                        bw.w = weight;
                        bi.w = bone_index;
                    }
                    if (bw.w > bw.z) {
                        std::swap(bw.w, bw.z);
                        std::swap(bi.w, bi.z);
                    }
                    if (bw.z > bw.y) {
                        std::swap(bw.z, bw.y);
                        std::swap(bi.z, bi.y);
                    }
                    if (bw.y > bw.x) {
                        std::swap(bw.y, bw.x);
                        std::swap(bi.y, bi.x);
                    }
                }
            }
            for (auto& bw : parts[i].bone_weights) {
                float sum = bw.x + bw.y + bw.z + bw.w;
                if (sum < 0.99 || sum > 1.01)
                    bw = bw * 1 / sum;
            }
        }

        filament::VertexBuffer::Builder vb_builder;
        vb_builder.vertexCount(mesh->mNumVertices)
            .bufferCount(mesh->mNumBones > 0 ? 4 : 2)
            .attribute(filament::VertexAttribute::POSITION, 0,
                       filament::VertexBuffer::AttributeType::FLOAT3, 0,
                       sizeof(Vec3f))
            .attribute(filament::VertexAttribute::TANGENTS, 1,
                       filament::VertexBuffer::AttributeType::SHORT4)
            .normalized(filament::VertexAttribute::TANGENTS);
        if (mesh->mNumBones > 0) {
            vb_builder
                .attribute(filament::VertexAttribute::BONE_INDICES, 2,
                           filament::VertexBuffer::AttributeType::USHORT4, 0,
                           sizeof(filament::math::ushort4))
                .attribute(filament::VertexAttribute::BONE_WEIGHTS, 3,
                           filament::VertexBuffer::AttributeType::FLOAT4, 0,
                           sizeof(Vec4f));
        }
        parts[i].vertex_buffer = vb_builder.build(engine);

        parts[i].vertex_buffer->setBufferAt(
            engine, 0,
            filament::backend::BufferDescriptor(parts[i].positions.data(),
                                                sizeof(Vec3f) *
                                                    parts[i].positions.size()));
        parts[i].vertex_buffer->setBufferAt(
            engine, 1,
            filament::backend::BufferDescriptor(parts[i].tangents.data(),
                                                sizeof(filament::math::short4) *
                                                    parts[i].tangents.size()));
        if (mesh->mNumBones > 0) {
            parts[i].vertex_buffer->setBufferAt(
                engine, 2,
                filament::backend::BufferDescriptor(
                    parts[i].bone_indices.data(),
                    sizeof(filament::math::ushort4) *
                        parts[i].bone_indices.size()));
            parts[i].vertex_buffer->setBufferAt(
                engine, 3,
                filament::backend::BufferDescriptor(
                    parts[i].bone_weights.data(),
                    sizeof(Vec4f) * parts[i].bone_weights.size()));
        }
    }
}
