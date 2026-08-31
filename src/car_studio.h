#ifndef CAR_STUDIO_H
#define CAR_STUDIO_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <vector>
#include <utility>

namespace godot {

enum SelectionMode {
    MODE_VERTEX = 0,
    MODE_EDGE = 1,
    MODE_FACE = 2,
    MODE_OBJECT = 3
};

struct BMeshVertex {
    Vector3 pos;
    bool deleted = false;
};

struct BMeshFace {
    std::vector<int> verts;
    bool deleted = false;
};

struct BMeshEdge {
    int v0, v1;
    bool deleted = false;
};

class CarStudio : public Node {
    GDCLASS(CarStudio, Node);

private:
    std::vector<BMeshVertex> m_vertices;
    std::vector<BMeshFace> m_faces;
    std::vector<BMeshEdge> m_edges;

    int m_mode;
    int m_selected_idx;
    std::vector<int> m_active_vertex_indices;

    void rebuild_edges();
    Vector3 calculate_face_normal(const BMeshFace& f) const;

protected:
    static void _bind_methods();

public:
    CarStudio();
    ~CarStudio();

    String get_system_info();
    void create_cube(float size);
    
    void set_selection_mode(int mode);
    int get_selection_mode() const;
    void set_selected_index(int index);
    int get_selected_index() const;
    
    int get_face_count() const;
    int get_vertex_count() const;
    int get_edge_count() const;
    
    Vector3 get_selection_center() const;
    Vector3 get_selection_normal() const;
    
    int pick_element(Vector3 ray_from, Vector3 ray_dir);
    
    bool move_selected(Vector3 offset);
    bool rotate_selected(Vector3 axis, float angle_rad, Vector3 center);
    bool scale_selected(Vector3 scale_factors, Vector3 center);
    
    bool extrude_selected(float distance);
    bool subdivide_selected_face();
    bool dissolve_selected();
    bool delete_selected();
    bool apply_subdivision(); // خوارزمية Catmull-Clark المحكمة بالكامل
    
    Ref<ArrayMesh> generate_godot_mesh();
};

}

#endif
