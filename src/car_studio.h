#ifndef CAR_STUDIO_H
#define CAR_STUDIO_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <pmp/surface_mesh.h>
#include <pmp/algorithms/subdivision.h>

namespace godot {

class CarStudio : public Node {
    GDCLASS(CarStudio, Node);

private:
    pmp::SurfaceMesh m_mesh;
    int m_selected_face;

protected:
    static void _bind_methods();

public:
    CarStudio();
    ~CarStudio();

    String get_system_info();
    
    void create_cube(float size);
    void select_face(int face_index);
    int get_selected_face() const;
    int get_face_count() const;
    int get_vertex_count() const;
    Vector3 get_selected_face_center() const;
    
    // دالة اصطياد وتحديد الوجه الملموس بدقة بالغة مع عزل الأوجه الخلفية
    int raycast_face(Vector3 ray_from, Vector3 ray_dir);
    
    bool extrude_selected_face(float distance);
    bool delete_selected_face();
    bool apply_subdivision();
    bool move_selected_face(Vector3 offset);
    
    Ref<ArrayMesh> generate_godot_mesh();
};

}

#endif
