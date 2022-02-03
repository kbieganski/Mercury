#ifndef MATERIAL_H_
#define MATERIAL_H_
#include "asset_library.h"

struct Material {
    filament::MaterialInstance* instance;
    ShaderHandle shader;
};

#endif // MATERIAL_H_
