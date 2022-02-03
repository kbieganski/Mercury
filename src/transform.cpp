#include "transform.h"
#include "graphics.h"
#include "scripting.h"

void Transform::propagate_transforms(entt::registry& registry, Graphics& graphics) {
    auto view = registry.view<Transform, Renderable>();
    auto& transform_manager = graphics.engine->getTransformManager();
    for(auto [entity, transform, renderable] : view.each()) {
        auto transform_instance = transform_manager.getInstance(renderable.entity);
        transform_manager.setTransform(transform_instance, Mat4f::scaling(transform.scale)
                * Mat4f(transform.rotation) * Mat4f::translation(transform.position));
    }
}

void Transform::bind(Scripting& scripting) {
    auto& lua = scripting.lua;
    lua.new_usertype<Vec2f>("Vec2", "x", &Vec2f::x, "y", &Vec2f::y);
    lua.new_usertype<Vec3f>("Vec3", "x", &Vec3f::x, "y", &Vec3f::y, "z", &Vec3f::z);
    lua.new_usertype<Vec4f>("Vec4", "x", &Vec4f::x, "y", &Vec4f::y, "z", &Vec4f::z, "w", &Vec4f::w);
    lua.new_usertype<Quatf>("Quat", "x", &Quatf::x, "y", &Quatf::y, "z", &Quatf::z, "w", &Quatf::w);
    lua.new_usertype<Transform>("Transform", "position", &Transform::position, "scale", &Transform::scale, "rotation", &Transform::rotation);
}
