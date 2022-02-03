#ifndef ANIMATOR_H
#define ANIMATOR_H
#include "asset_library.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/options/options.h"
#include <entt/entt.hpp>
#include "primitives.h"

struct SkeletalAnimation {
    SkeletalAnimation(ModelHandle model, AnimationHandle animation);
    void update(float dt);

    std::vector<ozz::math::SoaTransform> locals;
    std::vector<ozz::math::Float4x4> models;
    std::vector<ozz::math::Float4x4> skinning_matrices;

    float time_ratio = 0;
    ModelHandle model;
    AnimationHandle animation;
};

struct Graphics;

struct Animator {
    void update(float dt, entt::registry& registry);
    void update_renderables(entt::registry& registry, Graphics& graphics);

    void bind(Scripting& scripting);
};

#endif
