#include "car_studio.h"
#include <godot_cpp/core/class_db.hpp>
#include <cmath>
#include <algorithm>
#include <map>

using namespace godot;

void CarStudio::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_system_info"), &CarStudio::get_system_info);
    ClassDB::bind_method(D_METHOD("create_cube", "size"), &CarStudio::create_cube);
    ClassDB::bind_method(D_METHOD("set_selection_mode", "mode"), &CarStudio::set_selection_mode);
    ClassDB::bind_method(D_METHOD("get_selection_mode"), &CarStudio::get_selection_mode);
    ClassDB::bind_method(D_METHOD("set_selected_index", "index"), &CarStudio::set_selected_index);
    ClassDB::bind_method(D_METHOD("get_selected_index"), &CarStudio::get_selected_index);
    ClassDB::bind_method(D_METHOD("get_selection_center"), &CarStudio::get_selection_center);
    ClassDB::bind_method(D_METHOD("get_selection_normal"), &CarStudio::get_selection_normal);
    ClassDB::bind_method(D_METHOD("pick_element", "ray_from", "ray_dir"), &CarStudio::pick_element);
    ClassDB::bind_method(D_METHOD("move_selected", "offset"), &CarStudio::move_selected);
    ClassDB::bind_method(D_METHOD("rotate_selected", "axis", "angle_rad", "center"), &CarStudio::rotate_selected);
    ClassDB::bind_method(D_METHOD("scale_selected", "scale_factors", "center"), &CarStudio::scale_selected);
    ClassDB::bind_method(D_METHOD("extrude_selected", "distance"), &CarStudio::extrude_selected);
    ClassDB::bind_method(D_METHOD("delete_selected"), &CarStudio::delete_selected);
    ClassDB::bind_method(D_METHOD("generate_godot_mesh"), &CarStudio::generate_godot_mesh);
}

CarStudio::CarStudio() { m_mode = MODE_FACE; m_selected_idx = -1; }
CarStudio::~CarStudio() {}

String CarStudio::get_system_info() { return "🟢 Phase 1: Pure Mathematical Engine Active"; }

void CarStudio::update_normals() {
    for (auto& f : m_faces) {
        if (f.deleted || f.verts.size() < 3) continue;
        Vector3 v0 = m_vertices[f.verts[0]].pos;
        Vector3 v1 = m_vertices[f.verts[1]].pos;
        Vector3 v2 = m_vertices[f.verts[2]].pos;
        Vector3 n = (v1 - v0).cross(v2 - v0);
        f.normal = (n.length_squared() > 1e-6f) ? n.normalized() : Vector3(0,1,0);
    }
}

void CarStudio::update_edges() {
    m_edges.clear();
    std::map<std::pair<int, int>, int> emap;
    for (const auto& f : m_faces) {
        if (f.deleted) continue;
        int n = f.verts.size();
        for (int i = 0; i < n; ++i) {
            int u = f.verts[i], v = f.verts[(i + 1) % n];
            std::pair<int, int> edge_key = {std::min(u, v), std::max(u, v)};
            if (emap.find(edge_key) == emap.end()) {
                emap[edge_key] = m_edges.size();
                m_edges.push_back({std::min(u, v), std::max(u, v), false});
            }
        }
    }
}

// بناء المكعب بإحداثيات Godot الصحيحة (Y للأعلى، Z للأمام)
void CarStudio::create_cube(float size) {
    m_vertices.clear(); m_faces.clear(); m_edges.clear();
    m_active_verts.clear(); m_selected_idx = -1;

    float h = size * 0.5f;
    // 8 نقاط
    m_vertices.push_back({Vector3(-h, 0,  h), false}); // 0: Bottom-Left-Front
    m_vertices.push_back({Vector3( h, 0,  h), false}); // 1: Bottom-Right-Front
    m_vertices.push_back({Vector3( h, 0, -h), false}); // 2: Bottom-Right-Back
    m_vertices.push_back({Vector3(-h, 0, -h), false}); // 3: Bottom-Left-Back
    m_vertices.push_back({Vector3(-h, size,  h), false}); // 4: Top-Left-Front
    m_vertices.push_back({Vector3( h, size,  h), false}); // 5: Top-Right-Front
    m_vertices.push_back({Vector3( h, size, -h), false}); // 6: Top-Right-Back
    m_vertices.push_back({Vector3(-h, size, -h), false}); // 7: Top-Left-Back

    // 6 أوجه (كلها عكس عقارب الساعة ليكون النورمال للخارج)
    m_faces.push_back({{0, 1, 5, 4}, Vector3(), false}); // Front (+Z)
    m_faces.push_back({{1, 2, 6, 5}, Vector3(), false}); // Right (+X)
    m_faces.push_back({{2, 3, 7, 6}, Vector3(), false}); // Back (-Z)
    m_faces.push_back({{3, 0, 4, 7}, Vector3(), false}); // Left (-X)
    m_faces.push_back({{4, 5, 6, 7}, Vector3(), false}); // Top (+Y)
    m_faces.push_back({{3, 2, 1, 0}, Vector3(), false}); // Bottom (-Y)

    update_normals(); update_edges();
    set_selected_index(4); // تحديد الوجه العلوي
}

void CarStudio::set_selection_mode(int mode) { m_mode = mode; set_selected_index(-1); }
int CarStudio::get_selection_mode() const { return m_mode; }

void CarStudio::set_selected_index(int index) {
    m_selected_idx = index;
    m_active_verts.clear();
    if (index < 0) return;

    if (m_mode == MODE_FACE && index < m_faces.size() && !m_faces[index].deleted) {
        m_active_verts = m_faces[index].verts;
    }
}
int CarStudio::get_selected_index() const { return m_selected_idx; }

Vector3 CarStudio::get_selection_center() const {
    if (m_active_verts.empty()) return Vector3(0, 0.75f, 0);
    Vector3 center;
    for (int vi : m_active_verts) center += m_vertices[vi].pos;
    return center / float(m_active_verts.size());
}

Vector3 CarStudio::get_selection_normal() const {
    if (m_mode == MODE_FACE && m_selected_idx >= 0) return m_faces[m_selected_idx].normal;
    return Vector3(0, 1, 0);
}

int CarStudio::pick_element(Vector3 ray_from, Vector3 ray_dir) {
    if (m_mode == MODE_FACE) {
        int best = -1; float min_t = 1e9f;
        for (size_t i = 0; i < m_faces.size(); ++i) {
            if (m_faces[i].deleted) continue;
            const auto& f = m_faces[i];
            
            // خوارزمية Möller–Trumbore الدقيقة
            for (size_t j = 1; j < f.verts.size() - 1; ++j) {
                Vector3 v0 = m_vertices[f.verts[0]].pos;
                Vector3 v1 = m_vertices[f.verts[j]].pos;
                Vector3 v2 = m_vertices[f.verts[j+1]].pos;
                
                Vector3 e1 = v1 - v0, e2 = v2 - v0;
                Vector3 pvec = ray_dir.cross(e2);
                float det = e1.dot(pvec);
                if (std::fabs(det) < 1e-7f) continue;
                float inv_det = 1.0f / det;
                Vector3 tvec = ray_from - v0;
                float u = tvec.dot(pvec) * inv_det;
                if (u < 0.0f || u > 1.0f) continue;
                Vector3 qvec = tvec.cross(e1);
                float v = ray_dir.dot(qvec) * inv_det;
                if (v < 0.0f || u + v > 1.0f) continue;
                float t = e2.dot(qvec) * inv_det;
                if (t > 0.01f && t < min_t) { min_t = t; best = i; }
            }
        }
        return best;
    }
    return -1; 
}

bool CarStudio::move_selected(Vector3 offset) {
    for (int vi : m_active_verts) m_vertices[vi].pos += offset;
    update_normals();
    return true;
}

bool CarStudio::rotate_selected(Vector3 axis, float angle_rad, Vector3 center) {
    Vector3 u = axis.normalized();
    float c = cos(angle_rad), s = sin(angle_rad);
    for (int vi : m_active_verts) {
        Vector3 p = m_vertices[vi].pos - center;
        m_vertices[vi].pos = center + p * c + u.cross(p) * s + u * u.dot(p) * (1.0f - c);
    }
    update_normals();
    return true;
}

bool CarStudio::scale_selected(Vector3 scale_factors, Vector3 center) {
    for (int vi : m_active_verts) {
        Vector3 p = m_vertices[vi].pos - center;
        m_vertices[vi].pos = center + Vector3(p.x * scale_factors.x, p.y * scale_factors.y, p.z * scale_factors.z);
    }
    update_normals();
    return true;
}

// البثق الرياضي النظيف 100% (Face Extrude)
bool CarStudio::extrude_selected(float distance) {
    if (m_mode == MODE_FACE && m_selected_idx >= 0) {
        CFace& old_f = m_faces[m_selected_idx];
        if (old_f.deleted) return false;

        std::vector<int> base_verts = old_f.verts;
        Vector3 norm = old_f.normal;
        
        std::vector<int> top_verts;
        for (int vi : base_verts) {
            top_verts.push_back(m_vertices.size());
            m_vertices.push_back({m_vertices[vi].pos + norm * distance, false});
        }

        old_f.deleted = true;

        // بناء الجدران بترتيب CCW لضمان النورمال السليم
        int n = base_verts.size();
        for (int i = 0; i < n; ++i) {
            int nxt = (i + 1) % n;
            m_faces.push_back({ {base_verts[i], base_verts[nxt], top_verts[nxt], top_verts[i]}, Vector3(), false });
        }

        int top_face_idx = m_faces.size();
        m_faces.push_back({top_verts, Vector3(), false});

        update_normals(); update_edges();
        set_selected_index(top_face_idx);
        return true;
    }
    return false;
}

bool CarStudio::delete_selected() {
    if (m_mode == MODE_FACE && m_selected_idx >= 0) {
        m_faces[m_selected_idx].deleted = true;
        update_edges(); set_selected_index(-1); return true;
    }
    return false;
}

Ref<ArrayMesh> CarStudio::generate_godot_mesh() {
    Ref<SurfaceTool> st;
    st.instantiate();
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    for (size_t i = 0; i < m_faces.size(); ++i) {
        const auto& f = m_faces[i];
        if (f.deleted || f.verts.size() < 3) continue;

        Color col = (m_mode == MODE_FACE && m_selected_idx == i) ? Color(1.0f, 0.55f, 0.15f) : Color(0.68f, 0.72f, 0.78f);
        
        if (f.verts.size() == 4) {
            Vector3 v0 = m_vertices[f.verts[0]].pos, v1 = m_vertices[f.verts[1]].pos;
            Vector3 v2 = m_vertices[f.verts[2]].pos, v3 = m_vertices[f.verts[3]].pos;
            st->set_normal(f.normal); st->set_color(col); st->set_uv(Vector2(0,0)); st->add_vertex(v0);
            st->set_normal(f.normal); st->set_color(col); st->set_uv(Vector2(1,0)); st->add_vertex(v1);
            st->set_normal(f.normal); st->set_color(col); st->set_uv(Vector2(1,1)); st->add_vertex(v2);

            st->set_normal(f.normal); st->set_color(col); st->set_uv(Vector2(0,0)); st->add_vertex(v0);
            st->set_normal(f.normal); st->set_color(col); st->set_uv(Vector2(1,1)); st->add_vertex(v2);
            st->set_normal(f.normal); st->set_color(col); st->set_uv(Vector2(0,1)); st->add_vertex(v3);
        } else {
            for (size_t j = 1; j < f.verts.size() - 1; ++j) {
                st->set_normal(f.normal); st->set_color(col); st->set_uv(Vector2(0,0)); st->add_vertex(m_vertices[f.verts[0]].pos);
                st->set_normal(f.normal); st->set_color(col); st->set_uv(Vector2(1,0)); st->add_vertex(m_vertices[f.verts[j]].pos);
                st->set_normal(f.normal); st->set_color(col); st->set_uv(Vector2(0,1)); st->add_vertex(m_vertices[f.verts[j+1]].pos);
            }
        }
    }
    return st->commit();
}
