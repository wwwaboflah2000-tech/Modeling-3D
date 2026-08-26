#ifndef CAR_STUDIO_H
#define CAR_STUDIO_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <pmp/surface_mesh.h>
#include <pmp/algorithms/subdivision.h>
#include <vector>

namespace godot {

enum SelectionMode {
    MODE_VERTEX = 0,
    MODE_EDGE = 1,
    MODE_FACE = 2,
    MODE_OBJECT = 3
};

class CarStudio : public Node {
    GDCLASS(CarStudio, Node);

private:
    pmp::SurfaceMesh m_mesh;
    int m_mode; // 0=Vertex, 1=Edge, 2=Face, 3=Object
    int m_selected_idx;

    // الرؤوس النشطة التابعة للعنصر المختار (للتحريك المنعزل)
    std::vector<pmp::Vertex> m_active_vertices;

protected:
    static void _bind_methods();

public:
    CarStudio();
    ~CarStudio();

    String get_system_info();
    
    void create_cube(float size);
    
    // إدارة الأوضاع والتحديد
    void set_selection_mode(int mode);
    int get_selection_mode() const;
    void set_selected_index(int index);
    int get_selected_index() const;
    
    int get_face_count() const;
    int get_vertex_count() const;
    int get_edge_count() const;
    
    Vector3 get_selection_center() const;
    
    // اصطياد العناصر بالأشعة (Raycasting)
    int pick_element(Vector3 ray_from, Vector3 ray_dir);
    
    // عمليات النمذجة (Blender BMesh Engine)
    bool extrude_selected(float distance);
    bool delete_selected();
    bool apply_subdivision();
    bool move_selected(Vector3 offset);
    
    Ref<ArrayMesh> generate_godot_mesh();
};

}

#endif
