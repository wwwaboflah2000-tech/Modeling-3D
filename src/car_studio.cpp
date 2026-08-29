#include "car_studio.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>

using namespace godot;

void CarStudio::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_system_info"), &CarStudio::get_system_info);
    ClassDB::bind_method(D_METHOD("create_cube", "size"), &CarStudio::create_cube);
    
    ClassDB::bind_method(D_METHOD("set_selection_mode", "mode"), &CarStudio::set_selection_mode);
    ClassDB::bind_method(D_METHOD("get_selection_mode"), &CarStudio::get_selection_mode);
    ClassDB::bind_method(D_METHOD("set_selected_index", "index"), &CarStudio::set_selected_index);
    ClassDB::bind_method(D_METHOD("get_selected_index"), &CarStudio::get_selected_index);
    
    ClassDB::bind_method(D_METHOD("get_face_count"), &CarStudio::get_face_count);
    ClassDB::bind_method(D_METHOD("get_vertex_count"), &CarStudio::get_vertex_count);
    ClassDB::bind_method(D_METHOD("get_edge_count"), &CarStudio::get_edge_count);
    ClassDB::bind_method(D_METHOD("get_selection_center"), &CarStudio::get_selection_center);
    ClassDB::bind_method(D_METHOD("get_selection_normal"), &CarStudio::get_selection_normal);
    ClassDB::bind_method(D_METHOD("pick_element", "ray_from", "ray_dir"), &CarStudio::pick_element);
    
    ClassDB::bind_method(D_METHOD("extrude_selected", "distance"), &CarStudio::extrude_selected);
    ClassDB::bind_method(D_METHOD("delete_selected"), &CarStudio::delete_selected);
    ClassDB::bind_method(D_METHOD("apply_subdivision"), &CarStudio::apply_subdivision);
    ClassDB::bind_method(D_METHOD("move_selected", "offset"), &CarStudio::move_selected);
    ClassDB::bind_method(D_METHOD("generate_godot_mesh"), &CarStudio::generate_godot_mesh);
}

CarStudio::CarStudio() {
    m_mode = MODE_FACE;
    m_selected_idx = -1;
}

CarStudio::~CarStudio() {}

String CarStudio::get_system_info() {
    return "⚡ Blender-grade BMesh Engine: Active";
}

void CarStudio::create_cube(float size) {
    try {
        m_mesh.clear();
        m_selected_idx = -1;
        m_active_vertices.clear();

        float h = size * 0.5f;

        auto v0 = m_mesh.add_vertex(pmp::Point(-h, 0.0f, -h));
        auto v1 = m_mesh.add_vertex(pmp::Point( h, 0.0f, -h));
        auto v2 = m_mesh.add_vertex(pmp::Point( h, 0.0f,  h));
        auto v3 = m_mesh.add_vertex(pmp::Point(-h, 0.0f,  h));
        auto v4 = m_mesh.add_vertex(pmp::Point(-h, size, -h));
        auto v5 = m_mesh.add_vertex(pmp::Point( h, size, -h));
        auto v6 = m_mesh.add_vertex(pmp::Point( h, size,  h));
        auto v7 = m_mesh.add_vertex(pmp::Point(-h, size,  h));

        m_mesh.add_quad(v0, v4, v5, v1); // Front
        m_mesh.add_quad(v1, v5, v6, v2); // Right
        m_mesh.add_quad(v2, v6, v7, v3); // Back
        m_mesh.add_quad(v3, v7, v4, v0); // Left
        auto top_f = m_mesh.add_quad(v4, v7, v6, v5); // Top
        m_mesh.add_quad(v0, v1, v2, v3); // Bottom

        set_selected_index(top_f.idx());
    } catch (...) {}
}

void CarStudio::set_selection_mode(int mode) { 
    m_mode = mode; 
    set_selected_index(-1);
}

int CarStudio::get_selection_mode() const { return m_mode; }

void CarStudio::set_selected_index(int index) { 
    m_selected_idx = index;
    m_active_vertices.clear();

    if (m_selected_idx < 0) return;

    try {
        if (m_mode == MODE_FACE) {
            pmp::Face f(m_selected_idx);
            if (m_mesh.is_valid(f)) {
                for (auto v : m_mesh.vertices(f)) {
                    m_active_vertices.push_back(v);
                }
            } else {
                m_selected_idx = -1;
            }
        }
        else if (m_mode == MODE_EDGE) {
            pmp::Edge e(m_selected_idx);
            if (m_mesh.is_valid(e)) {
                m_active_vertices.push_back(m_mesh.vertex(e, 0));
                m_active_vertices.push_back(m_mesh.vertex(e, 1));
            } else {
                m_selected_idx = -1;
            }
        }
        else if (m_mode == MODE_VERTEX) {
            pmp::Vertex v(m_selected_idx);
            if (m_mesh.is_valid(v)) {
                m_active_vertices.push_back(v);
            } else {
                m_selected_idx = -1;
            }
        }
        else if (m_mode == MODE_OBJECT) {
            for (auto v : m_mesh.vertices()) {
                m_active_vertices.push_back(v);
            }
        }
    } catch (...) {
        m_selected_idx = -1;
        m_active_vertices.clear();
    }
}

int CarStudio::get_selected_index() const { return m_selected_idx; }
int CarStudio::get_face_count() const { return (int)m_mesh.n_faces(); }
int CarStudio::get_vertex_count() const { return (int)m_mesh.n_vertices(); }
int CarStudio::get_edge_count() const { return (int)m_mesh.n_edges(); }

Vector3 CarStudio::get_selection_center() const {
    if (m_mesh.is_empty() || m_active_vertices.empty()) return Vector3(0, 0.75f, 0);

    pmp::Point c(0, 0, 0);
    int valid_count = 0;
    for (auto v : m_active_vertices) {
        if (m_mesh.is_valid(v)) {
            c += m_mesh.position(v);
            valid_count++;
        }
    }
    if (valid_count == 0) return Vector3(0, 0.75f, 0);
    c /= float(valid_count);
    return Vector3(c[0], c[1], c[2]);
}

Vector3 CarStudio::get_selection_normal() const {
    if (m_mode == MODE_FACE && m_selected_idx >= 0) {
        pmp::Face f(m_selected_idx);
        if (m_mesh.is_valid(f)) {
            std::vector<pmp::Point> pts;
            for (auto v : m_mesh.vertices(f)) pts.push_back(m_mesh.position(v));
            if (pts.size() >= 3) {
                Vector3 v0(pts[0][0], pts[0][1], pts[0][2]);
                Vector3 v1(pts[1][0], pts[1][1], pts[1][2]);
                Vector3 v2(pts[2][0], pts[2][1], pts[2][2]);
                return (v1 - v0).cross(v2 - v0).normalized();
            }
        }
    }
    return Vector3(0, 1, 0); // الافتراضي للأعلى
}

int CarStudio::pick_element(Vector3 ray_from, Vector3 ray_dir) {
    if (m_mesh.is_empty()) return -1;

    if (m_mode == MODE_FACE) {
        int best_face = -1; 
        float min_t = 1e9f;

        for (auto f : m_mesh.faces()) {
            std::vector<pmp::Point> pts;
            for (auto v : m_mesh.vertices(f)) pts.push_back(m_mesh.position(v));
            if (pts.size() < 3) continue;

            Vector3 v0(pts[0][0], pts[0][1], pts[0][2]);
            Vector3 v1(pts[1][0], pts[1][1], pts[1][2]);
            Vector3 v2(pts[2][0], pts[2][1], pts[2][2]);
            Vector3 normal = (v1 - v0).cross(v2 - v0).normalized();

            if (normal.dot(ray_dir) > -0.001f) continue;

            for (size_t i = 1; i < pts.size() - 1; ++i) {
                Vector3 tv0(pts[0][0], pts[0][1], pts[0][2]);
                Vector3 tv1(pts[i][0], pts[i][1], pts[i][2]);
                Vector3 tv2(pts[i + 1][0], pts[i + 1][1], pts[i + 1][2]);

                Vector3 edge1 = tv1 - tv0; 
                Vector3 edge2 = tv2 - tv0;
                Vector3 pvec = ray_dir.cross(edge2); 
                float det = edge1.dot(pvec);
                if (det <= 1e-7f) continue;

                float inv_det = 1.0f / det;
                Vector3 tvec = ray_from - tv0;
                float u = tvec.dot(pvec) * inv_det;
                if (u < 0.0f || u > 1.0f) continue;

                Vector3 qvec = tvec.cross(edge1);
                float v = ray_dir.dot(qvec) * inv_det;
                if (v < 0.0f || u + v > 1.0f) continue;

                float t = edge2.dot(qvec) * inv_det;
                if (t > 1e-4f && t < min_t) { 
                    min_t = t; 
                    best_face = f.idx(); 
                }
            }
        }
        return best_face;
    }
    else if (m_mode == MODE_EDGE) {
        int best_edge = -1; 
        float min_dist = 0.25f;

        for (auto e : m_mesh.edges()) {
            pmp::Point p1 = m_mesh.position(m_mesh.vertex(e, 0));
            pmp::Point p2 = m_mesh.position(m_mesh.vertex(e, 1));
            Vector3 v1(p1[0], p1[1], p1[2]); 
            Vector3 v2(p2[0], p2[1], p2[2]);
            Vector3 mid = (v1 + v2) * 0.5f;

            Vector3 to_mid = mid - ray_from;
            float proj = to_mid.dot(ray_dir);
            if (proj > 0.0f) {
                Vector3 close_pt = ray_from + ray_dir * proj;
                float d = (mid - close_pt).length();
                if (d < min_dist) { 
                    min_dist = d; 
                    best_edge = e.idx(); 
                }
            }
        }
        return best_edge;
    }
    else if (m_mode == MODE_VERTEX) {
        int best_vert = -1; 
        float min_dist = 0.25f;

        for (auto v : m_mesh.vertices()) {
            pmp::Point p = m_mesh.position(v);
            Vector3 vp(p[0], p[1], p[2]);
            Vector3 to_v = vp - ray_from;
            float proj = to_v.dot(ray_dir);
            if (proj > 0.0f) {
                Vector3 close_pt = ray_from + ray_dir * proj;
                float d = (vp - close_pt).length();
                if (d < min_dist) { 
                    min_dist = d; 
                    best_vert = v.idx(); 
                }
            }
        }
        return best_vert;
    }
    else if (m_mode == MODE_OBJECT) {
        return 0;
    }

    return -1;
}

bool CarStudio::extrude_selected(float distance) {
    try {
        if (m_mode != MODE_FACE || m_selected_idx < 0) return false;

        pmp::Face old_face(m_selected_idx);
        if (!m_mesh.is_valid(old_face)) return false;

        std::vector<pmp::Vertex> base_verts;
        for (auto v : m_mesh.vertices(old_face)) base_verts.push_back(v);
        size_t n = base_verts.size();
        if (n < 3) return false;

        pmp::Point p0 = m_mesh.position(base_verts[0]);
        pmp::Point p1 = m_mesh.position(base_verts[1]);
        pmp::Point p2 = m_mesh.position(base_verts[2]);
        Vector3 gp0(p0[0], p0[1], p0[2]); 
        Vector3 gp1(p1[0], p1[1], p1[2]); 
        Vector3 gp2(p2[0], p2[1], p2[2]);
        Vector3 gnorm = (gp1 - gp0).cross(gp2 - gp0).normalized();
        pmp::Point normal(gnorm.x, gnorm.y, gnorm.z);

        std::vector<pmp::Vertex> new_top_verts;
        for (size_t i = 0; i < n; ++i) {
            pmp::Point new_pos = m_mesh.position(base_verts[i]) + normal * distance;
            new_top_verts.push_back(m_mesh.add_vertex(new_pos));
        }

        m_mesh.delete_face(old_face);

        for (size_t i = 0; i < n; ++i) {
            size_t nxt = (i + 1) % n;
            m_mesh.add_quad(base_verts[i], base_verts[nxt], new_top_verts[nxt], new_top_verts[i]);
        }

        pmp::Face top_face;
        if (n == 3) {
            top_face = m_mesh.add_triangle(new_top_verts[0], new_top_verts[1], new_top_verts[2]);
        } else if (n == 4) {
            top_face = m_mesh.add_quad(new_top_verts[0], new_top_verts[1], new_top_verts[2], new_top_verts[3]);
        } else {
            top_face = m_mesh.add_face(new_top_verts);
        }

        set_selected_index(top_face.idx());
        return true;
    } catch (...) {
        return false;
    }
}

bool CarStudio::delete_selected() {
    try {
        if (m_selected_idx < 0) return false;

        if (m_mode == MODE_FACE) {
            pmp::Face f(m_selected_idx);
            if (m_mesh.is_valid(f)) {
                m_mesh.delete_face(f);
                m_mesh.garbage_collection();
                set_selected_index(-1);
                return true;
            }
        }
        else if (m_mode == MODE_VERTEX) {
            pmp::Vertex v(m_selected_idx);
            if (m_mesh.is_valid(v)) {
                m_mesh.delete_vertex(v);
                m_mesh.garbage_collection();
                set_selected_index(-1);
                return true;
            }
        }
        else if (m_mode == MODE_EDGE) {
            pmp::Edge e(m_selected_idx);
            if (m_mesh.is_valid(e)) {
                m_mesh.delete_edge(e);
                m_mesh.garbage_collection();
                set_selected_index(-1);
                return true;
            }
        }
    } catch (...) {}
    return false;
}

bool CarStudio::apply_subdivision() {
    try {
        if (m_mesh.is_empty()) return false;
        m_mesh.garbage_collection();
        pmp::catmull_clark_subdivision(m_mesh);
        set_selected_index(-1);
        return true;
    } catch (...) { return false; }
}

bool CarStudio::move_selected(Vector3 offset) {
    try {
        if (m_active_vertices.empty()) return false;
        pmp::Point off(offset.x, offset.y, offset.z);
        for (auto v : m_active_vertices) {
            if (m_mesh.is_valid(v)) {
                m_mesh.position(v) += off;
            }
        }
        return true;
    } catch (...) { return false; }
}

// توليد الميش بدون أي خطوط مائلة وبدقة نقية للأوجه
Ref<ArrayMesh> CarStudio::generate_godot_mesh() {
    Ref<SurfaceTool> st;
    st.instantiate();
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    try {
        for (auto f : m_mesh.faces()) {
            bool is_selected = false;
            if (m_mode == MODE_FACE && f.idx() == m_selected_idx) is_selected = true;
            else if (m_mode == MODE_OBJECT && m_selected_idx == 0) is_selected = true;

            Color col = is_selected ? Color(0.25f, 1.0f, 0.25f, 1.0f) : Color(0.82f, 0.85f, 0.90f, 1.0f);

            std::vector<pmp::Point> pts;
            for (auto v : m_mesh.vertices(f)) pts.push_back(m_mesh.position(v));

            if (pts.size() >= 3) {
                Vector3 p0(pts[0][0], pts[0][1], pts[0][2]);
                Vector3 p1(pts[1][0], pts[1][1], pts[1][2]);
                Vector3 p2(pts[2][0], pts[2][1], pts[2][2]);
                Vector3 fnorm = (p1 - p0).cross(p2 - p0).normalized();

                if (pts.size() == 4) {
                    // توزيع UV رباعي نظيف يقضي على الخط المائل نهائياً
                    Vector3 v0(pts[0][0], pts[0][1], pts[0][2]);
                    Vector3 v1(pts[1][0], pts[1][1], pts[1][2]);
                    Vector3 v2(pts[2][0], pts[2][1], pts[2][2]);
                    Vector3 v3(pts[3][0], pts[3][1], pts[3][2]);

                    // المثلث الأول (v0, v1, v2)
                    st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(0, 0)); st->add_vertex(v0);
                    st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(1, 0)); st->add_vertex(v1);
                    st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(1, 1)); st->add_vertex(v2);

                    // المثلث الثاني (v0, v2, v3)
                    st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(0, 0)); st->add_vertex(v0);
                    st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(1, 1)); st->add_vertex(v2);
                    st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(0, 1)); st->add_vertex(v3);
                } else {
                    for (size_t i = 1; i < pts.size() - 1; ++i) {
                        Vector3 v0(pts[0][0], pts[0][1], pts[0][2]);
                        Vector3 v1(pts[i][0], pts[i][1], pts[i][2]);
                        Vector3 v2(pts[i + 1][0], pts[i + 1][1], pts[i + 1][2]);

                        st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(0, 0)); st->add_vertex(v0);
                        st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(1, 0)); st->add_vertex(v1);
                        st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(0.5, 1)); st->add_vertex(v2);
                    }
                }
            }
        }
    } catch (...) {}

    return st->commit();
}
