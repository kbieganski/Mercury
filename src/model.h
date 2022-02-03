#ifndef MODEL_H_
#define MODEL_H_
#include "asset_library.h"
#include <filament/MaterialInstance.h>

struct Model {
    MeshHandle mesh;
    MaterialHandle material;
    SkeletonHandle skeleton;
    std::vector<AnimationHandle> animations;
};

#endif
