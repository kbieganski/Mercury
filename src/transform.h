#include "primitives.h"
#include <entt/entt.hpp>

struct Graphics;
struct Scripting;

struct Transform {
    Vec3f position;
    Vec3f scale = { 1.0 };
    Quatf rotation;

    static void propagate_transforms(entt::registry& registry, Graphics& graphics);
    static void bind(Scripting& scripting);
};
