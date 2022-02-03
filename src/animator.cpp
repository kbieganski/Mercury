#include "animator.h"
#include "graphics.h"
#include "mesh.h"
#include "model.h"
#include "scripting.h"

SkeletalAnimation::SkeletalAnimation(ModelHandle _model, AnimationHandle _animation)
    : model(_model), animation(_animation) {
    locals.resize(model->skeleton->num_soa_joints());
    models.resize(model->skeleton->num_joints());
    skinning_matrices.resize(model->mesh->inverse_binds.size());
}

void SkeletalAnimation::update(float dt) {
    time_ratio += dt / animation->duration();
    if (time_ratio > 1)
        time_ratio -= std::floor(time_ratio);
    ozz::animation::SamplingCache cache;
    cache.Resize(model->skeleton->num_joints());
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = animation;
    sampling_job.cache = &cache;
    sampling_job.ratio = time_ratio;
    sampling_job.output = ozz::make_span(locals);
    sampling_job.Run();
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = model->skeleton;
    ltm_job.input = ozz::make_span(locals);
    ltm_job.output = ozz::make_span(models);
    ltm_job.Run();
    for (size_t i = 0; i < models.size(); ++i) {
        // TODO: Get rid of the bone lookup
        auto bone_name = model->skeleton->joint_names()[i];
        auto it = model->mesh->bone_name_to_index.find(bone_name);
        if (it != model->mesh->bone_name_to_index.end()) {
            auto bone_index = it->second;
            skinning_matrices[bone_index] =
                models[i] * model->mesh->inverse_binds[bone_index];
        }
    }
}

void Animator::update(float dt, entt::registry& registry) {
    auto view = registry.view<SkeletalAnimation>();
    for(auto [entity, anim] : view.each())
        anim.update(dt);
}

void Animator::update_renderables(entt::registry& registry, Graphics& graphics) {
    auto view = registry.view<SkeletalAnimation, Renderable>();
    auto& renderable_manager = graphics.engine->getRenderableManager();
    for(auto [entity, anim, renderable] : view.each()) {
        auto renderable_instance = renderable_manager.getInstance(renderable.entity);
        renderable_manager.setBones(renderable_instance,
                reinterpret_cast<filament::math::mat4f*>(anim.skinning_matrices.data()),
                anim.skinning_matrices.size());
    }
}

void Animator::bind(Scripting& scripting) {
    auto& lua = scripting.lua;
    lua.new_usertype<SkeletalAnimation>("SkeletalAnimation", sol::meta_function::construct, [](ModelHandle model, AnimationHandle animation) { return SkeletalAnimation(model, animation); });
}
