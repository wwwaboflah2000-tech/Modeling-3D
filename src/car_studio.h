#ifndef CAR_STUDIO_H
#define CAR_STUDIO_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <vector>

namespace godot {

struct QuadFace {
    int v[4];
};

class CarStudio : public Node {
    GDCLASS(CarStudio, Node);

private:
    std::vector<Vector3> m_vertices;
    std::vector<QuadFace> m_faces;
    int m_selected_face;

protected:
    static void _bind_methods();

public:
    CarStudio();
    ~CarStudio();

    String get_system_info();
    
    // دوال إنشاء الأشكال الأولية (Primitives)
    void create_cube(float size);
    void create_plane(float size);
    
    // دوال النمذجة الأساسية (Core Modeling Operations)
    void select_face(int face_index);
    int get_selected_face() const;
    int get_face_count() const;
    int get_vertex_count() const;
    
    bool extrude_selected_face(float distance);
    bool inset_selected_face(float amount);
    bool move_selected_face(Vector3 offset);
    bool delete_selected_face();
    
    // ربط Pixar OpenSubdiv
    Dictionary apply_pixar_subdivision(int level);
    
    // توليد الميش النهائي للعرض في جودوت
    Ref<ArrayMesh> generate_godot_mesh();
};

}

#endif
