#pragma once
#include "math/mat2.h"
#include "math/mat3.h"
#include "math/mat4.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

using Vec2i = filament::math::vec2<int>;
using Vec2f = filament::math::vec2<float>;
using Vec3i = filament::math::vec3<int>;
using Vec3f = filament::math::vec3<float>;
using Vec4i = filament::math::vec4<int>;
using Vec4us = filament::math::vec4<uint16_t>;
using Vec4f = filament::math::vec4<float>;
using Quatf = filament::math::quatf;
using Mat2f = filament::math::mat2f;
using Mat3f = filament::math::mat3f;
using Mat4f = filament::math::mat4f;

#define IM_VEC2_CLASS_EXTRA                                                    \
    ImVec2(const Vec2f& v) : x(v.x), y(v.y) {}                                 \
    ImVec2& operator=(const Vec2f& v) {                                        \
        x = v.x;                                                               \
        y = v.y;                                                               \
        return *this;                                                          \
    }                                                                          \
    operator Vec2f() { return {x, y}; }
