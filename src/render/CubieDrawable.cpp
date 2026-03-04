#include "render/CubieDrawable.h"

#include <Magnum/GL/Buffer.h>
#include <Magnum/Math/Constants.h>
#include <Magnum/Mesh.h>
#include <Corrade/Containers/ArrayView.h>

#include <cmath>
#include <vector>

namespace rubik {

using namespace Magnum;
using namespace Magnum::Math::Literals;

namespace {

// ── Geometry parameters ─────────────────────────────────────────────────────
// Body: slightly smaller than 1.0 pitch so gaps appear between cubies.
constexpr float kBodyHalf       = 0.48f;
constexpr float kBevelRadius    = 0.06f;
constexpr int   kBevelSegments  = 5;
// Stickers: smaller rounded rectangles raised above the body face.
constexpr float kStickerHalf    = 0.39f;
constexpr float kStickerCorner  = 0.03f;
constexpr float kStickerElev    = 0.005f;
constexpr int   kStickerSegments = 4;

struct Vertex {
    Vector3 position;
    Vector3 normal;
};

// ── Colour / visibility helpers ─────────────────────────────────────────────

Color3 faceColor(int faceIdx, Vector3i homePos) {
    switch (faceIdx) {
        case 0: return homePos.x() ==  1 ? CubeColors::Red     : CubeColors::Black;
        case 1: return homePos.x() == -1 ? CubeColors::Orange   : CubeColors::Black;
        case 2: return homePos.y() ==  1 ? CubeColors::White    : CubeColors::Black;
        case 3: return homePos.y() == -1 ? CubeColors::Yellow   : CubeColors::Black;
        case 4: return homePos.z() ==  1 ? CubeColors::Green    : CubeColors::Black;
        case 5: return homePos.z() == -1 ? CubeColors::Blue     : CubeColors::Black;
    }
    return CubeColors::Black;
}

bool isExposedFace(int faceIdx, Vector3i homePos) {
    switch (faceIdx) {
        case 0: return homePos.x() ==  1;
        case 1: return homePos.x() == -1;
        case 2: return homePos.y() ==  1;
        case 3: return homePos.y() == -1;
        case 4: return homePos.z() ==  1;
        case 5: return homePos.z() == -1;
    }
    return false;
}

// ── Upload helpers ──────────────────────────────────────────────────────────

GL::Mesh uploadMesh(const std::vector<Vertex>& vertices,
                    const std::vector<UnsignedInt>& indices) {
    GL::Buffer vBuf{GL::Buffer::TargetHint::Array};
    GL::Buffer iBuf{GL::Buffer::TargetHint::ElementArray};
    vBuf.setData(
        Containers::ArrayView<const Vertex>{vertices.data(), vertices.size()});
    iBuf.setData(
        Containers::ArrayView<const UnsignedInt>{indices.data(), indices.size()});

    GL::Mesh mesh;
    mesh.setPrimitive(MeshPrimitive::Triangles)
        .setCount(Int(indices.size()))
        .addVertexBuffer(std::move(vBuf), 0,
            Shaders::PhongGL::Position{},
            Shaders::PhongGL::Normal{})
        .setIndexBuffer(std::move(iBuf), 0,
            MeshIndexType::UnsignedInt,
            0, UnsignedInt(vertices.size() - 1));
    return mesh;
}

// ── Rounded-box body mesh ───────────────────────────────────────────────────
// Built from 6 flat face quads + 12 quarter-cylinder edge bevels +
// 8 eighth-sphere corner patches.

GL::Mesh buildBodyMesh() {
    const float H   = kBodyHalf;
    const float R   = kBevelRadius;
    const int   N   = kBevelSegments;
    const float I   = H - R;            // inner (flat-face) half-extent
    const float piH = Math::Constants<float>::piHalf();

    std::vector<Vertex> verts;
    std::vector<UnsignedInt> idx;

    // Helper – emit two CCW triangles for a quad laid out as
    //   a──b       (a,d,c) then (a,c,b)
    //   │  │
    //   d──c
    auto quad = [&](UnsignedInt a, UnsignedInt b,
                    UnsignedInt c, UnsignedInt d) {
        idx.insert(idx.end(), {a, d, c,  a, c, b});
    };

    // ── 6 flat faces ────────────────────────────────────────────────────────
    for (int axis = 0; axis < 3; ++axis) {
        int u = (axis + 1) % 3, v = (axis + 2) % 3;
        for (float sign : {1.0f, -1.0f}) {
            auto base = UnsignedInt(verts.size());
            Vector3 n{}; n[axis] = sign;
            // j: 0=BL  1=BR  2=TR  3=TL  in local (u,v) space
            for (int j = 0; j < 4; ++j) {
                Vector3 p{}; p[axis] = sign * H;
                p[u] = ((j == 1 || j == 2) ?  I : -I);
                p[v] = ((j >= 2)            ?  I : -I);
                verts.push_back({p, n});
            }
            if (sign > 0.0f) quad(base, base+1, base+2, base+3);
            else              quad(base+1, base, base+3, base+2);
        }
    }

    // ── 12 edge bevels (quarter-cylinders) ──────────────────────────────────
    // For each edge: la = length axis, ua/va = the two perpendicular axes.
    // su, sv = ±1 select which octant the edge lies in.
    for (int la = 0; la < 3; ++la) {
        int ua = (la + 1) % 3, va = (la + 2) % 3;
        for (int su : {-1, 1}) {
            for (int sv : {-1, 1}) {
                auto base = UnsignedInt(verts.size());
                // Two rows along the length axis at ±I
                for (int row = 0; row < 2; ++row) {
                    float laV = row == 0 ? -I : I;
                    for (int j = 0; j <= N; ++j) {
                        float t = float(j) / float(N) * piH;
                        float ct = std::cos(t), st = std::sin(t);
                        Vector3 p{}, nm{};
                        p[la]  = laV;
                        p[ua]  = float(su) * (I + R * ct);
                        p[va]  = float(sv) * (I + R * st);
                        nm[ua] = float(su) * ct;
                        nm[va] = float(sv) * st;
                        verts.push_back({p, nm});
                    }
                }
                int cols = N + 1;
                // Winding parity derived from cross-product analysis
                bool flip = (su * sv > 0);
                for (int j = 0; j < N; ++j) {
                    UnsignedInt a = base + UnsignedInt(j);
                    UnsignedInt b = a + 1;
                    UnsignedInt c = base + UnsignedInt(cols + j);
                    UnsignedInt d = c + 1;
                    if (!flip) idx.insert(idx.end(), {a, c, d,  a, d, b});
                    else       idx.insert(idx.end(), {a, b, d,  a, d, c});
                }
            }
        }
    }

    // ── 8 corner patches (eighth-spheres) ───────────────────────────────────
    for (int sx : {-1, 1}) {
        for (int sy : {-1, 1}) {
            for (int sz : {-1, 1}) {
                auto base = UnsignedInt(verts.size());
                Vector3 ctr{float(sx) * I, float(sy) * I, float(sz) * I};
                // θ (theta) = polar from +Z, φ (phi) = azimuth from +X
                for (int i = 0; i <= N; ++i) {
                    float th = float(i) / float(N) * piH;
                    float sinTh = std::sin(th), cosTh = std::cos(th);
                    for (int j = 0; j <= N; ++j) {
                        float ph = float(j) / float(N) * piH;
                        Vector3 nm{float(sx) * sinTh * std::cos(ph),
                                   float(sy) * sinTh * std::sin(ph),
                                   float(sz) * cosTh};
                        verts.push_back({ctr + R * nm, nm});
                    }
                }
                int cols = N + 1;
                bool flip = (sx * sy * sz < 0);
                for (int i = 0; i < N; ++i) {
                    for (int j = 0; j < N; ++j) {
                        UnsignedInt v00 = base + UnsignedInt(i * cols + j);
                        UnsignedInt v01 = v00 + 1;
                        UnsignedInt v10 = v00 + UnsignedInt(cols);
                        UnsignedInt v11 = v10 + 1;
                        if (!flip) idx.insert(idx.end(),
                            {v00, v10, v11,  v00, v11, v01});
                        else idx.insert(idx.end(),
                            {v00, v01, v11,  v00, v11, v10});
                    }
                }
            }
        }
    }

    return uploadMesh(verts, idx);
}

// ── Rounded-rectangle sticker mesh ──────────────────────────────────────────
// A triangle-fan with rounded corners, sitting just above the body face.

GL::Mesh buildStickerMesh(int faceIdx) {
    const float S   = kStickerHalf;
    const float R   = kStickerCorner;
    const int   N   = kStickerSegments;
    const float piH = Math::Constants<float>::piHalf();

    int   axis = faceIdx / 2;
    float sign = (faceIdx % 2 == 0) ? 1.0f : -1.0f;
    int   u    = (axis + 1) % 3;
    int   v    = (axis + 2) % 3;

    Vector3 normal{}; normal[axis] = sign;
    float dist = kBodyHalf + kStickerElev;

    std::vector<Vertex> verts;
    std::vector<UnsignedInt> idx;

    // Centre vertex
    Vector3 ctr{}; ctr[axis] = sign * dist;
    verts.push_back({ctr, normal});

    // Perimeter: 4 rounded corners, N points each (last point of each arc
    // coincides with first point of the next straight segment).
    float ci     = S - R;
    float cU[4]  = { ci, -ci, -ci,  ci};
    float cV[4]  = { ci,  ci, -ci, -ci};
    int   total  = 4 * N;

    for (int c = 0; c < 4; ++c) {
        float a0 = float(c) * piH;
        for (int j = 0; j < N; ++j) {
            float a = a0 + float(j) / float(N) * piH;
            Vector3 p{}; p[axis] = sign * dist;
            p[u] = cU[c] + R * std::cos(a);
            p[v] = cV[c] + R * std::sin(a);
            verts.push_back({p, normal});
        }
    }

    // Triangle fan from centre
    for (int i = 0; i < total; ++i) {
        auto cur = UnsignedInt(1 + i);
        auto nxt = UnsignedInt(1 + (i + 1) % total);
        if (sign > 0.0f) idx.insert(idx.end(), {0u, cur, nxt});
        else              idx.insert(idx.end(), {0u, nxt, cur});
    }

    return uploadMesh(verts, idx);
}

} // anonymous namespace

// ── CubieDrawable implementation ────────────────────────────────────────────

CubieDrawable::CubieDrawable(
    Object3D& object,
    Shaders::PhongGL& shader,
    SceneGraph::DrawableGroup3D& drawables,
    Vector3i homePos)
    : SceneGraph::Drawable3D{object, &drawables}
    , shader_{shader}
{
    buildMeshes(homePos);
}

void CubieDrawable::buildMeshes(Vector3i homePos) {
    bodyMesh_ = buildBodyMesh();
    for (int i = 0; i < 6; ++i) {
        if (isExposedFace(i, homePos)) {
            stickers_[i].mesh    = buildStickerMesh(i);
            stickers_[i].color   = faceColor(i, homePos);
            stickers_[i].visible = true;
        }
    }
}

void CubieDrawable::draw(
    const Matrix4& transformationMatrix,
    SceneGraph::Camera3D& camera)
{
    shader_
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .setDiffuseColor(CubeColors::Black)
        .setAmbientColor(CubeColors::Black * 0.15f)
        .draw(bodyMesh_);

    for (auto& s : stickers_) {
        if (!s.visible) continue;
        shader_
            .setDiffuseColor(s.color)
            .setAmbientColor(s.color * 0.15f)
            .draw(s.mesh);
    }
}

} // namespace rubik
