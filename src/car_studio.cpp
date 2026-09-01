#include "car_studio.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <cmath>
#include <algorithm>
#include <map>
#include <set>

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
    
    ClassDB::bind_method(D_METHOD("move_selected", "offset"), &CarStudio::move_selected);
    ClassDB::bind_method(D_METHOD("rotate_selected", "axis", "angle_rad", "center"), &CarStudio::rotate_selected);
    ClassDB::bind_method(D_METHOD("scale_selected", "scale_factors", "center"), &CarStudio::scale_selected);
    
    ClassDB::bind_method(D_METHOD("extrude_selected", "distance"), &CarStudio::extrude_selected);
    ClassDB::bind_method(D_METHOD("subdivide_selected_face"), &CarStudio::subdivide_selected_face);
    ClassDB::bind_method(D_METHOD("dissolve_selected"), &CarStudio::dissolve_selected);
    ClassDB::bind_method(D_METHOD("delete_selected"), &CarStudio::delete_selected);
    ClassDB::bind_method(D_METHOD("apply_subdivision"), &CarStudio::apply_subdivision);
    ClassDB::bind_method(D_METHOD("generate_godot_mesh"), &CarStudio::generate_godot_mesh);
}

CarStudio::CarStudio() {
    m_mode = MODE_FACE;
    m_selected_idx = -1;
}

CarStudio::~CarStudio() {}

String CarStudio::get_system_info() {
    return "⚡ Blender BMesh Core: Fully Welded & Stable";
}

void CarStudio::weld_vertices(float threshold) {
    float th_sq = threshold * threshold;
    std::map<int, int> remap;

    for (size_t i = 0; i < m_vertices.size(); ++i) {
        if (m_vertices[i].deleted) continue;
        remap[(int)i] = (int)i;
        for (size_t j = 0; j < i; ++j) {
            if (m_vertices[j].deleted) continue;
            if ((m_vertices[i].pos - m_vertices[j].pos).length_squared() <= th_sq) {
                remap[(int)i] = remap[(int)j];
                m_vertices[i].deleted = true;
                break;
            }
        }
    }

    for (auto& f : m_faces) {
        if (f.deleted) continue;
        for (auto& vi : f.verts) {
            if (remap.find(vi) != remap.end()) {
                vi = remap[vi];
            }
        }
    }
}

void CarStudio::rebuild_edges() {
    m_edges.clear();
    std::map<std::pair<int, int>, int> edge_map;

    for (const auto& f : m_faces) {
        if (f.deleted || f.verts.size() < 2) continue;
        size_t n = f.verts.size();
        for (size_t i = 0; i < n; ++i) {
            int u = f.verts[i];
            int v = f.verts[(i + 1) % n];
            int a = std::min(u, v);
            int b = std::max(u, v);
            std::pair<int, int> p = {a, b};

            if (edge_map.find(p) == edge_map.end()) {
                edge_map[p] = (int)m_edges.size();
                BMeshEdge e;
                e.v0 = a;
                e.v1 = b;
                e.deleted = false;
                m_edges.push_back(e);
            }
        }
    }
}

Vector3 CarStudio::calculate_face_normal(const BMeshFace& f) const {
    if (f.verts.size() < 3) return Vector3(0, 1, 0);
    Vector3 v0 = m_vertices[f.verts[0]].pos;
    Vector3 v1 = m_vertices[f.verts[1]].pos;
    Vector3 v2 = m_vertices[f.verts[2]].pos;
    Vector3 n = (v1 - v0).cross(v2 - v0);
    if (n.length_squared() < 1e-8f) return Vector3(0, 1, 0);
    return n.normalized();
}

void CarStudio::create_cube(float size) {
    m_vertices.clear();
    m_faces.clear();
    m_edges.clear();
    m_active_vertex_indices.clear();
    m_selected_idx = -1;

    float h = size * 0.5f;

    m_vertices.push_back({ Vector3(-h, 0.0f, -h), false }); // 0
    m_vertices.push_back({ Vector3( h, 0.0f, -h), false }); // 1
    m_vertices.push_back({ Vector3( h, 0.0f,  h), false }); // 2
    m_vertices.push_back({ Vector3(-h, 0.0f,  h), false }); // 3
    m_vertices.push_back({ Vector3(-h, size, -h), false }); // 4
    m_vertices.push_back({ Vector3( h, size, -h), false }); // 5
    m_vertices.push_back({ Vector3( h, size,  h), false }); // 6
    m_vertices.push_back({ Vector3(-h, size,  h), false }); // 7

    m_faces.push_back({ {0, 4, 5, 1}, false }); // Front
    m_faces.push_back({ {1, 5, 6, 2}, false }); // Right
    m_faces.push_back({ {2, 6, 7, 3}, false }); // Back
    m_faces.push_back({ {3, 7, 4, 0}, false }); // Left
    m_faces.push_back({ {4, 7, 6, 5}, false }); // Top
    m_faces.push_back({ {0, 1, 2, 3}, false }); // Bottom

    rebuild_edges();
    set_selected_index(4);
}

void CarStudio::set_selection_mode(int mode) {
    m_mode = mode;
    set_selected_index(-1);
}

int CarStudio::get_selection_mode() const { return m_mode; }

void CarStudio::set_selected_index(int index) {
    m_selected_idx = index;
    m_active_vertex_indices.clear();

    if (m_selected_idx < 0) return;

    if (m_mode == MODE_FACE && m_selected_idx < (int)m_faces.size() && !m_faces[m_selected_idx].deleted) {
        m_active_vertex_indices = m_faces[m_selected_idx].verts;
    }
    else if (m_mode == MODE_EDGE && m_selected_idx < (int)m_edges.size() && !m_edges[m_selected_idx].deleted) {
        m_active_vertex_indices.push_back(m_edges[m_selected_idx].v0);
        m_active_vertex_indices.push_back(m_edges[m_selected_idx].v1);
    }
    else if (m_mode == MODE_VERTEX && m_selected_idx < (int)m_vertices.size() && !m_vertices[m_selected_idx].deleted) {
        m_active_vertex_indices.push_back(m_selected_idx);
    }
    else if (m_mode == MODE_OBJECT) {
        for (size_t i = 0; i < m_vertices.size(); ++i) {
            if (!m_vertices[i].deleted) m_active_vertex_indices.push_back((int)i);
        }
    }
}

int CarStudio::get_selected_index() const { return m_selected_idx; }
int CarStudio::get_face_count() const { return (int)m_faces.size(); }
int CarStudio::get_vertex_count() const { return (int)m_vertices.size(); }
int CarStudio::get_edge_count() const { return (int)m_edges.size(); }

Vector3 CarStudio::get_selection_center() const {
    if (m_active_vertex_indices.empty()) return Vector3(0, 0.75f, 0);

    Vector3 c = Vector3(0, 0, 0);
    int count = 0;
    for (int vi : m_active_vertex_indices) {
        if (vi >= 0 && vi < (int)m_vertices.size() && !m_vertices[vi].deleted) {
            c += m_vertices[vi].pos;
            count++;
        }
    }
    if (count == 0) return Vector3(0, 0.75f, 0);
    return c / float(count);
}

Vector3 CarStudio::get_selection_normal() const {
    if (m_mode == MODE_FACE && m_selected_idx >= 0 && m_selected_idx < (int)m_faces.size()) {
        return calculate_face_normal(m_faces[m_selected_idx]);
    }
    else if (m_mode == MODE_EDGE && m_selected_idx >= 0 && m_selected_idx < (int)m_edges.size()) {
        int u = m_edges[m_selected_idx].v0;
        int v = m_edges[m_selected_idx].v1;
        Vector3 p0 = m_vertices[u].pos;
        Vector3 p1 = m_vertices[v].pos;
        Vector3 T = (p1 - p0).normalized();

        Vector3 avg_norm = Vector3(0, 0, 0);
        int incident_count = 0;

        for (const auto& f : m_faces) {
            if (f.deleted) continue;
            bool has_u = false, has_v = false;
            for (int vi : f.verts) {
                if (vi == u) has_u = true;
                if (vi == v) has_v = true;
            }
            if (has_u && has_v) {
                avg_norm += calculate_face_normal(f);
                incident_count++;
            }
        }

        if (incident_count > 0 && avg_norm.length_squared() > 1e-4f) {
            Vector3 N = avg_norm.normalized();
            Vector3 D = (N - T * T.dot(N)).normalized();
            if (D.length_squared() > 1e-4f) return D;
        }

        Vector3 fallback = T.cross(Vector3(0, 1, 0)).normalized();
        if (fallback.length_squared() < 1e-4f) fallback = T.cross(Vector3(1, 0, 0)).normalized();
        return fallback;
    }
    return Vector3(0, 1, 0);
}

int CarStudio::pick_element(Vector3 ray_from, Vector3 ray_dir) {
    if (m_mode == MODE_FACE) {
        int best_face = -1; float min_t = 1e9f;
        for (size_t f_idx = 0; f_idx < m_faces.size(); ++f_idx) {
            const auto& f = m_faces[f_idx];
            if (f.deleted || f.verts.size() < 3) continue;

            Vector3 norm = calculate_face_normal(f);
            if (norm.dot(ray_dir) > 0.05f) continue; // عزل الأوجه الخلفية

            for (size_t i = 1; i < f.verts.size() - 1; ++i) {
                Vector3 tv0 = m_vertices[f.verts[0]].pos;
                Vector3 tv1 = m_vertices[f.verts[i]].pos;
                Vector3 tv2 = m_vertices[f.verts[i + 1]].pos;

                Vector3 edge1 = tv1 - tv0; Vector3 edge2 = tv2 - tv0;
                Vector3 pvec = ray_dir.cross(edge2); float det = edge1.dot(pvec);
                if (std::fabs(det) <= 1e-7f) continue;
                float inv_det = 1.0f / det;
                Vector3 tvec = ray_from - tv0;
                float u = tvec.dot(pvec) * inv_det;
                if (u < 0.0f || u > 1.0f) continue;
                Vector3 qvec = tvec.cross(edge1);
                float v = ray_dir.dot(qvec) * inv_det;
                if (v < 0.0f || u + v > 1.0f) continue;
                float t = edge2.dot(qvec) * inv_det;
                if (t > 1e-4f && t < min_t) { min_t = t; best_face = (int)f_idx; }
            }
        }
        return best_face;
    }
    else if (m_mode == MODE_EDGE) {
        int best_edge = -1; float min_dist = 0.25f;
        for (size_t e_idx = 0; e_idx < m_edges.size(); ++e_idx) {
            const auto& e = m_edges[e_idx];
            if (e.deleted) continue;
            Vector3 v1 = m_vertices[e.v0].pos; Vector3 v2 = m_vertices[e.v1].pos;
            Vector3 mid = (v1 + v2) * 0.5f;

            Vector3 to_mid = mid - ray_from;
            float proj = to_mid.dot(ray_dir);
            if (proj > 0.0f) {
                Vector3 close_pt = ray_from + ray_dir * proj;
                float d = (mid - close_pt).length();
                if (d < min_dist) { min_dist = d; best_edge = (int)e_idx; }
            }
        }
        return best_edge;
    }
    else if (m_mode == MODE_VERTEX) {
        int best_vert = -1; float min_dist = 0.25f;
        for (size_t v_idx = 0; v_idx < m_vertices.size(); ++v_idx) {
            if (m_vertices[v_idx].deleted) continue;
            Vector3 vp = m_vertices[v_idx].pos;
            Vector3 to_v = vp - ray_from;
            float proj = to_v.dot(ray_dir);
            if (proj > 0.0f) {
                Vector3 close_pt = ray_from + ray_dir * proj;
                float d = (vp - close_pt).length();
                if (d < min_dist) { min_dist = d; best_vert = (int)v_idx; }
            }
        }
        return best_vert;
    }
    else if (m_mode == MODE_OBJECT) return 0;
    return -1;
}

bool CarStudio::move_selected(Vector3 offset) {
    if (m_active_vertex_indices.empty()) return false;
    for (int vi : m_active_vertex_indices) {
        if (vi >= 0 && vi < (int)m_vertices.size() && !m_vertices[vi].deleted) {
            m_vertices[vi].pos += offset;
        }
    }
    return true;
}

bool CarStudio::rotate_selected(Vector3 axis, float angle_rad, Vector3 center) {
    if (m_active_vertex_indices.empty() || axis.length_squared() < 1e-6f) return false;
    Vector3 u = axis.normalized();
    float cos_a = cos(angle_rad);
    float sin_a = sin(angle_rad);

    for (int vi : m_active_vertex_indices) {
        if (vi >= 0 && vi < (int)m_vertices.size() && !m_vertices[vi].deleted) {
            Vector3 v_pos = m_vertices[vi].pos - center;
            Vector3 rot = v_pos * cos_a + u.cross(v_pos) * sin_a + u * u.dot(v_pos) * (1.0f - cos_a);
            m_vertices[vi].pos = center + rot;
        }
    }
    return true;
}

bool CarStudio::scale_selected(Vector3 scale_factors, Vector3 center) {
    if (m_active_vertex_indices.empty()) return false;
    for (int vi : m_active_vertex_indices) {
        if (vi >= 0 && vi < (int)m_vertices.size() && !m_vertices[vi].deleted) {
            Vector3 v_pos = m_vertices[vi].pos - center;
            Vector3 scaled(v_pos.x * scale_factors.x, v_pos.y * scale_factors.y, v_pos.z * scale_factors.z);
            m_vertices[vi].pos = center + scaled;
        }
    }
    return true;
}

bool CarStudio::extrude_selected(float distance) {
    if (m_mode == MODE_FACE && m_selected_idx >= 0 && m_selected_idx < (int)m_faces.size()) {
        BMeshFace& old_f = m_faces[m_selected_idx];
        if (old_f.deleted || old_f.verts.size() < 3) return false;

        std::vector<int> base_v = old_f.verts;
        size_t n = base_v.size();
        Vector3 norm = calculate_face_normal(old_f);

        std::vector<int> new_top_v;
        for (size_t i = 0; i < n; ++i) {
            Vector3 new_pos = m_vertices[base_v[i]].pos + norm * distance;
            int new_idx = (int)m_vertices.size();
            m_vertices.push_back({ new_pos, false });
            new_top_v.push_back(new_idx);
        }

        old_f.deleted = true;

        for (size_t i = 0; i < n; ++i) {
            size_t nxt = (i + 1) % n;
            m_faces.push_back({ {base_v[i], base_v[nxt], new_top_v[nxt], new_top_v[i]}, false });
        }

        int top_face_idx = (int)m_faces.size();
        m_faces.push_back({ new_top_v, false });

        rebuild_edges();
        set_selected_index(top_face_idx);
        return true;
    }
    else if (m_mode == MODE_EDGE && m_selected_idx >= 0 && m_selected_idx < (int)m_edges.size()) {
        const BMeshEdge& e = m_edges[m_selected_idx];
        if (e.deleted) return false;

        int v0 = e.v0;
        int v1 = e.v1;
        Vector3 p0 = m_vertices[v0].pos;
        Vector3 p1 = m_vertices[v1].pos;

        Vector3 norm_dir = get_selection_normal();

        int nv0 = (int)m_vertices.size();
        m_vertices.push_back({ p0 + norm_dir * distance, false });
        int nv1 = (int)m_vertices.size();
        m_vertices.push_back({ p1 + norm_dir * distance, false });

        m_faces.push_back({ {v0, v1, nv1, nv0}, false });

        rebuild_edges();

        int a = std::min(nv0, nv1);
        int b = std::max(nv0, nv1);
        for (size_t i = 0; i < m_edges.size(); ++i) {
            if (m_edges[i].v0 == a && m_edges[i].v1 == b) {
                set_selected_index((int)i);
                break;
            }
        }
        return true;
    }
    return false;
}

bool CarStudio::subdivide_selected_face() {
    if (m_mode != MODE_FACE || m_selected_idx < 0 || m_selected_idx >= (int)m_faces.size()) return false;
    BMeshFace& f = m_faces[m_selected_idx];
    if (f.deleted || f.verts.size() != 4) return false;

    int v0 = f.verts[0], v1 = f.verts[1], v2 = f.verts[2], v3 = f.verts[3];
    Vector3 p0 = m_vertices[v0].pos, p1 = m_vertices[v1].pos, p2 = m_vertices[v2].pos, p3 = m_vertices[v3].pos;

    auto get_or_create_edge_midpoint = [&](int u, int w, Vector3 pu, Vector3 pw) -> int {
        int mid_idx = (int)m_vertices.size();
        m_vertices.push_back({ (pu + pw) * 0.5f, false });

        for (size_t fi = 0; fi < m_faces.size(); ++fi) {
            if (m_faces[fi].deleted || (int)fi == m_selected_idx) continue;
            auto& f_verts = m_faces[fi].verts;
            size_t fn = f_verts.size();
            for (size_t i = 0; i < fn; ++i) {
                int cur = f_verts[i];
                int nxt = f_verts[(i + 1) % fn];
                if ((cur == u && nxt == w) || (cur == w && nxt == u)) {
                    f_verts.insert(f_verts.begin() + i + 1, mid_idx);
                    break;
                }
            }
        }
        return mid_idx;
    };

    int m0 = get_or_create_edge_midpoint(v0, v1, p0, p1);
    int m1 = get_or_create_edge_midpoint(v1, v2, p1, p2);
    int m2 = get_or_create_edge_midpoint(v2, v3, p2, p3);
    int m3 = get_or_create_edge_midpoint(v3, v0, p3, p0);

    int c = (int)m_vertices.size();
    m_vertices.push_back({ (p0 + p1 + p2 + p3) * 0.25f, false });

    f.deleted = true;
    int start_f = (int)m_faces.size();
    m_faces.push_back({ {v0, m0, c, m3}, false });
    m_faces.push_back({ {m0, v1, m1, c}, false });
    m_faces.push_back({ {c, m1, v2, m2}, false });
    m_faces.push_back({ {m3, c, m2, v3}, false });

    weld_vertices(1e-4f);
    rebuild_edges();
    set_selected_index(start_f);
    return true;
}

bool CarStudio::dissolve_selected() {
    if (m_mode != MODE_EDGE || m_selected_idx < 0 || m_selected_idx >= (int)m_edges.size()) return false;
    const auto& e = m_edges[m_selected_idx];
    if (e.deleted) return false;

    int u = e.v0, v = e.v1;
    std::vector<int> inc_f;
    for (size_t i = 0; i < m_faces.size(); ++i) {
        if (m_faces[i].deleted) continue;
        bool hu = false, hv = false;
        for (int vi : m_faces[i].verts) {
            if (vi == u) hu = true;
            if (vi == v) hv = true;
        }
        if (hu && hv) inc_f.push_back((int)i);
    }

    if (inc_f.size() == 2) {
        m_faces[inc_f[0]].deleted = true;
        m_faces[inc_f[1]].deleted = true;
        weld_vertices(1e-4f);
        rebuild_edges();
        set_selected_index(-1);
        return true;
    }
    return false;
}

bool CarStudio::delete_selected() {
    if (m_selected_idx < 0) return false;

    if (m_mode == MODE_FACE && m_selected_idx < (int)m_faces.size()) {
        m_faces[m_selected_idx].deleted = true;
        rebuild_edges();
        set_selected_index(-1);
        return true;
    }
    else if (m_mode == MODE_VERTEX && m_selected_idx < (int)m_vertices.size()) {
        m_vertices[m_selected_idx].deleted = true;
        for (auto& f : m_faces) {
            for (int vi : f.verts) {
                if (vi == m_selected_idx) { f.deleted = true; break; }
            }
        }
        rebuild_edges();
        set_selected_index(-1);
        return true;
    }
    else if (m_mode == MODE_EDGE && m_selected_idx < (int)m_edges.size()) {
        m_edges[m_selected_idx].deleted = true;
        int u = m_edges[m_selected_idx].v0, v = m_edges[m_selected_idx].v1;
        for (auto& f : m_faces) {
            bool hu = false, hv = false;
            for (int vi : f.verts) {
                if (vi == u) hu = true;
                if (vi == v) hv = true;
            }
            if (hu && hv) f.deleted = true;
        }
        rebuild_edges();
        set_selected_index(-1);
        return true;
    }
    return false;
}

bool CarStudio::apply_subdivision() {
    if (m_faces.empty()) return false;
    weld_vertices(1e-4f);

    std::vector<Vector3> new_vertices_pos;
    std::map<int, int> old_to_compact_vert;
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        if (!m_vertices[i].deleted) {
            old_to_compact_vert[(int)i] = (int)new_vertices_pos.size();
            new_vertices_pos.push_back(m_vertices[i].pos);
        }
    }

    std::vector<std::vector<int>> compact_faces;
    for (const auto& f : m_faces) {
        if (f.deleted || f.verts.size() < 3) continue;
        std::vector<int> cf;
        for (int vi : f.verts) {
            if (old_to_compact_vert.find(vi) != old_to_compact_vert.end()) {
                cf.push_back(old_to_compact_vert[vi]);
            }
        }
        if (cf.size() >= 3) compact_faces.push_back(cf);
    }

    if (compact_faces.empty()) return false;
    size_t num_orig_verts = new_vertices_pos.size();

    std::vector<int> face_point_indices(compact_faces.size());
    for (size_t fi = 0; fi < compact_faces.size(); ++fi) {
        Vector3 avg = Vector3(0, 0, 0);
        for (int vi : compact_faces[fi]) avg += new_vertices_pos[vi];
        avg /= float(compact_faces[fi].size());
        face_point_indices[fi] = (int)new_vertices_pos.size();
        new_vertices_pos.push_back(avg);
    }

    std::map<std::pair<int, int>, std::vector<int>> edge_to_faces;
    std::map<int, std::vector<int>> vert_to_faces;
    std::map<int, std::vector<std::pair<int, int>>> vert_to_edges;

    for (size_t fi = 0; fi < compact_faces.size(); ++fi) {
        size_t n = compact_faces[fi].size();
        for (size_t i = 0; i < n; ++i) {
            int u = compact_faces[fi][i];
            int v = compact_faces[fi][(i + 1) % n];
            std::pair<int, int> edge_key = { std::min(u, v), std::max(u, v) };
            edge_to_faces[edge_key].push_back((int)fi);
            vert_to_faces[u].push_back((int)fi);
            vert_to_edges[u].push_back(edge_key);
        }
    }

    std::map<std::pair<int, int>, int> edge_point_indices;
    for (const auto& [edge_key, inc_faces] : edge_to_faces) {
        int u = edge_key.first;
        int v = edge_key.second;
        Vector3 pu = new_vertices_pos[u];
        Vector3 pv = new_vertices_pos[v];

        Vector3 edge_pt;
        if (inc_faces.size() == 2) {
            Vector3 fp0 = new_vertices_pos[face_point_indices[inc_faces[0]]];
            Vector3 fp1 = new_vertices_pos[face_point_indices[inc_faces[1]]];
            edge_pt = (pu + pv + fp0 + fp1) * 0.25f;
        } else {
            edge_pt = (pu + pv) * 0.5f;
        }

        edge_point_indices[edge_key] = (int)new_vertices_pos.size();
        new_vertices_pos.push_back(edge_pt);
    }

    for (size_t vi = 0; vi < num_orig_verts; ++vi) {
        if (vert_to_faces.find((int)vi) == vert_to_faces.end()) continue;
        const auto& inc_faces = vert_to_faces[(int)vi];
        const auto& inc_edges = vert_to_edges[(int)vi];
        int n = (int)inc_faces.size();
        if (n >= 3) {
            Vector3 Q = Vector3(0, 0, 0);
            for (int fi : inc_faces) Q += new_vertices_pos[face_point_indices[fi]];
            Q /= float(n);

            Vector3 R = Vector3(0, 0, 0);
            for (const auto& ek : inc_edges) {
                int other = (ek.first == (int)vi) ? ek.second : ek.first;
                R += (new_vertices_pos[vi] + new_vertices_pos[other]) * 0.5f;
            }
            R /= float(inc_edges.size());

            new_vertices_pos[vi] = (Q + R * 2.0f + new_vertices_pos[vi] * float(n - 3)) / float(n);
        }
    }

    std::vector<BMeshFace> new_bmesh_faces;
    for (size_t fi = 0; fi < compact_faces.size(); ++fi) {
        int fp_idx = face_point_indices[fi];
        size_t n = compact_faces[fi].size();
        for (size_t i = 0; i < n; ++i) {
            int v_curr = compact_faces[fi][i];
            int v_prev = compact_faces[fi][(i + n - 1) % n];
            int v_next = compact_faces[fi][(i + 1) % n];

            std::pair<int, int> edge_prev = { std::min(v_prev, v_curr), std::max(v_prev, v_curr) };
            std::pair<int, int> edge_next = { std::min(v_curr, v_next), std::max(v_curr, v_next) };

            int ep_prev = edge_point_indices[edge_prev];
            int ep_next = edge_point_indices[edge_next];

            new_bmesh_faces.push_back({ {v_curr, ep_next, fp_idx, ep_prev}, false });
        }
    }

    m_vertices.clear();
    for (const auto& p : new_vertices_pos) {
        m_vertices.push_back({ p, false });
    }
    m_faces = new_bmesh_faces;

    rebuild_edges();
    set_selected_index(-1);
    return true;
}

Ref<ArrayMesh> CarStudio::generate_godot_mesh() {
    Ref<SurfaceTool> st;
    st.instantiate();
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    for (size_t f_idx = 0; f_idx < m_faces.size(); ++f_idx) {
        const auto& f = m_faces[f_idx];
        if (f.deleted || f.verts.size() < 3) continue;

        bool is_selected = false;
        if (m_mode == MODE_FACE && (int)f_idx == m_selected_idx) is_selected = true;
        else if (m_mode == MODE_OBJECT && m_selected_idx == 0) is_selected = true;

        Color col = is_selected ? Color(1.0f, 0.55f, 0.15f, 1.0f) : Color(0.68f, 0.72f, 0.78f, 1.0f);
        Vector3 fnorm = calculate_face_normal(f);

        if (f.verts.size() == 4) {
            Vector3 v0 = m_vertices[f.verts[0]].pos;
            Vector3 v1 = m_vertices[f.verts[1]].pos;
            Vector3 v2 = m_vertices[f.verts[2]].pos;
            Vector3 v3 = m_vertices[f.verts[3]].pos;

            st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(0.0f, 0.0f)); st->add_vertex(v0);
            st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(1.0f, 0.0f)); st->add_vertex(v1);
            st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(1.0f, 1.0f)); st->add_vertex(v2);

            st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(0.0f, 0.0f)); st->add_vertex(v0);
            st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(1.0f, 1.0f)); st->add_vertex(v2);
            st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(0.0f, 1.0f)); st->add_vertex(v3);
        } else {
            for (size_t i = 1; i < f.verts.size() - 1; ++i) {
                Vector3 v0 = m_vertices[f.verts[0]].pos;
                Vector3 v1 = m_vertices[f.verts[i]].pos;
                Vector3 v2 = m_vertices[f.verts[i + 1]].pos;

                st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(0.0f, 0.0f)); st->add_vertex(v0);
                st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(1.0f, 0.0f)); st->add_vertex(v1);
                st->set_normal(fnorm); st->set_color(col); st->set_uv(Vector2(0.5f, 1.0f)); st->add_vertex(v2);
            }
        }
    }

    return st->commit();
}
