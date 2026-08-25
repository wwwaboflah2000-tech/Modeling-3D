#ifndef CAR_STUDIO_H
#define CAR_STUDIO_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

class CarStudio : public Node {
    GDCLASS(CarStudio, Node);

protected:
    static void _bind_methods();

public:
    CarStudio();
    ~CarStudio();

    String get_system_info();
    
    // دالة التنعيم السينمائي من Pixar
    Dictionary apply_pixar_subdivision(PackedVector3Array vertices, PackedInt32Array indices, int level);
};

}

#endif
