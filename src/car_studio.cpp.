#include "car_studio.h"
#include <godot_cpp/core/class_db.hpp>
#include "../libs/meshoptimizer/src/meshoptimizer.h"

#define CGLTF_IMPLEMENTATION
#include "../libs/cgltf.h"

using namespace godot;

void CarStudio::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_system_info"), &CarStudio::get_system_info);
}

CarStudio::CarStudio() {}
CarStudio::~CarStudio() {}

String CarStudio::get_system_info() {
    return "🔥 Car Studio 3D Engine is ACTIVE (MeshOptimizer + C++ GDExtension on Android)!";
}
