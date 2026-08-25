#include "car_studio.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/surface_tool.hpp>

using namespace godot;

void CarStudio::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_system_info"), &CarStudio::get_system_info);
    ClassDB::bind_method(D_METHOD("create_cube", "size"), &CarStudio::create_cube);
    ClassDB::bind_method(D_METHOD("select_face", "face_index"), &CarStudio::select_face);
    ClassDB::bind_method(D_METHOD("get_selected_face"), &CarStudio::get_selected_face);
    ClassDB::bind_method(D_METHOD("get_face_count"), &CarStudio::get_face_count);
    ClassDB::bind_method(D_METHOD("get_vertex_count"), &CarStudio::get_vertex_count);
    ClassDB::bind_method(D_METHOD("get_selected_face_center"), &CarStudio::get_selected_face_center);
    
    ClassDB::bind_method(D_METHOD("extrude_selected_face", "distance"), &CarStudio::extrude_selected_face);
    ClassDB::bind_method(D_METHOD("delete_selected_face"), &CarStudio::delete_selected_face);
    ClassDB::bind_method(D_METHOD("apply_subdivision"), &CarStudio::apply_subdivision);
    ClassDB::bind_method(D_METHOD("move_selected_face", "offset"), &CarStudio::move_selected_face);
    ClassDB::bind_method(D_METHOD("generate_godot_mesh"), &CarStudio::generate_godot_mesh);
}

CarStudio::CarStudio() {
    m_selected_face = 0;
    create_cube(1.5f);
}

CarStudio::~CarStudio() {}

String CarStudio::get_system_info() {
    return "🔥 Open3D Core Powered by PMP-Library & Pixar OpenSubdiv!";
}

void CarStudio::create_cube(float size) {
    m_mesh.clear();
    m_selected_face = 0;
    float h = size * 0.5f;

    auto v0 = m_mesh.add_vertex(pmp::Point(-h, 0.0f, -h));
    auto v1 = m_mesh.add_vertex(pmp::Point( h, 0.0f, -h));
    auto v2 = m_mesh.add_vertex(pmp::Point( h, 0.0f,  h));
    auto v3 = m_mesh.add_vertex(pmp::Point(-h, 0.0f,  h));
    auto v4 = m_mesh.add_vertex(pmp::Point(-h, size, -h));
    auto v5 = m_mesh.add_vertex(pmp::Point( h, size, -h));
    auto v6 = m_mesh.add_vertex(pmp::Point( h, size,  h));
    auto v7 = m_mesh.add_vertex(pmp::Point(-h, size,  h));

    m_mesh.add_quad(v0, v4, v5, v1);
    m_mesh.add_quad(v1, v5, v6, v2);
    m_mesh.add_quad(v2, v6, v7, v3);
    m_mesh.add_quad(v3, v7, v4, v0);
    m_mesh.add_quad(v4, v7, v6, v5);
    m_mesh.add_quad(v0, v1, v2, v3);
}

void CarStudio::select_face(int face_index) {
    if (face_index >= 0 && face_index < (int)m_mesh.n_faces()) {
        m_selected_face = face_index;
    }
}

int CarStudio::get_selected_face() const { return m_selected_face; }
int CarStudio::get_face_count() const { return (int)m_mesh.n_faces(); }
int CarStudio::get_vertex_count() const { return (int)m_mesh.n_vertices(); }

Vector3 CarStudio::get_selected_face_center() const {
    if (m_selected_face < 0 || m_selected_face >= (int)m_mesh.n_faces()) return Vector3.ZERO;
    pmp::Face f(m_selected_face);
    pmp::Point c(0, 0, 0);
    int count = 0;
    for (auto v : m_mesh.vertices(f)) {
        c += m_mesh.position(v);
        count++;
    }
    if (count > 0) c /= float(count);
    return Vector3(c[0], c[1], c[2]);
}

bool CarStudio::extrude_selected_face(float distance) {
    if (m_selected_face < 0 || m_selected_face >= (int)m_mesh.n_faces()) return false;

    pmp::Face old_face(m_selected_face);
    std::vector<pmp::Vertex> old_verts;
    for (auto v : m_mesh.vertices(old_face)) {
        old_verts.push_back(v);
    }
    if (old_verts.size() != 4) return false;

    pmp::Point p0 = m_mesh.position(old_verts[0]);
    pmp::Point p1 = m_mesh.position(old_verts[1]);
    pmp::Point p2 = m_mesh.position(old_verts[2]);
    pmp::Point normal = pmp::normalize(pmp::cross(p1 - p0, p2 - p0));

    std::vector<pmp::Vertex> new_verts;
    for (int i = 0; i < 4; ++i) {
        pmp::Point new_pos = m_mesh.position(old_verts[i]) + normal * distance;
        new_verts.push_back(m_mesh.add_vertex(new_pos));
    }

    // حذف الوجه القديم لمنع أي وجه محبوس في الداخل
    m_mesh.delete_face(old_face);

    // بناء الجدران الـ 4
    for (int i = 0; i < 4; ++i) {
        int nxt = (i + 1) % 4;
        m_mesh.add_quad(old_verts[i], new_verts[i], new_verts[nxt], old_verts[nxt]);
    }

    // إنشاء الوجه الجديد في القمة
    auto top_face = m_mesh.add_quad(new_verts[0], new_verts[1], new_verts[2], new_verts[3]);
    m_selected_face = top_face.idx();
    return true;
}

bool CarStudio::delete_selected_face() {
    if (m_selected_face < 0 || m_selected_face >= (int)m_mesh.n_faces()) return false;
    m_mesh.delete_face(pmp::Face(m_selected_face));
    m_mesh.garbage_collection(); // تنظيف النقاط والحواف اليتيمة آلياً

    if (m_selected_face >= (int)m_mesh.n_faces()) {
        m_selected_face = (int)m_mesh.n_faces() - 1;
    }
    return true;
}

bool CarStudio::apply_subdivision() {
    if (m_mesh.is_empty()) return false;
    pmp::catmull_clark_subdivision(m_mesh);
    return true;
}

bool CarStudio::move_selected_face(Vector3 offset) {
    if (m_selected_face < 0 || m_selected_face >= (int)m_mesh.n_faces()) return false;
    pmp::Face f(m_selected_face);
    pmp::Point off(offset.x, offset.y, offset.z);
    for (auto v : m_mesh.vertices(f)) {
        m_mesh.position(v) += off;
    }
    return true;
}

Ref<ArrayMesh> CarStudio::generate_godot_mesh() {
    Ref<SurfaceTool> st;
    st.instantiate();
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    pmp::vertex_normals(m_mesh);

    int current_f_idx = 0;
    for (auto f : m_mesh.faces()) {
        bool is_selected = (current_f_idx == m_selected_face);
        Color col = is_selected ? Color(0.2f, 1.0f, 0.2f, 1.0f) : Color(0.82f, 0.85f, 0.9f, 0.0f);

        std::vector<pmp::Point> pts;
        for (auto v : m_mesh.vertices(f)) {
            pts.push_back(m_mesh.position(v));
        }

        if (pts.size() == 4) {
            st->set_color(col); st->set_uv(Vector2(0, 0)); st->add_vertex(Vector3(pts[0][0], pts[0][1], pts[0][2]));
            st->set_color(col); st->set_uv(Vector2(1, 0)); st->add_vertex(Vector3(pts[1][0], pts[1][1], pts[1][2]));
            st->set_color(col); st->set_uv(Vector2(1, 1)); st->add_vertex(Vector3(pts[2][0], pts[2][1], pts[2][2]));

            st->set_color(col); st->set_uv(Vector2(0, 0)); st->add_vertex(Vector3(pts[0][0], pts[0][1], pts[0][2]));
            st->set_color(col); st->set_uv(Vector2(1, 1)); st->add_vertex(Vector3(pts[2][0], pts[2][1], pts[2][2]));
            st->set_color(col); st->set_uv(Vector2(0, 1)); st->add_vertex(Vector3(pts[3][0], pts[3][1], pts[3][2]));
        }
        current_f_idx++;
    }

    st->generate_normals();
    return st->commit();
}
