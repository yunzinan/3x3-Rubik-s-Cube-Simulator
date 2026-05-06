#include "core/CubeState.h"
#include "utils/Logger.h"

#include <Magnum/Math/Angle.h>
#include <Magnum/Math/Matrix3.h>

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace rubik {

using namespace Magnum;
using namespace Magnum::Math::Literals;

CubeState::CubeState() { initCubies(); }

void CubeState::initCubies() {
    int idx = 0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            for (int z = -1; z <= 1; ++z) {
                if (x == 0 && y == 0 && z == 0)
                    continue; // skip hidden center
                cubies_[idx].homePosition = {x, y, z};
                cubies_[idx].position     = {x, y, z};
                cubies_[idx].rotation     = Matrix4{};
                ++idx;
            }
        }
    }
}

void CubeState::reset() { initCubies(); }

// Which coordinate of position determines membership on each face.
static int faceAxis(Face f) {
    switch (f) {
        case Face::R: case Face::L: return 0; // x
        case Face::U: case Face::D: return 1; // y
        case Face::F: case Face::B: return 2; // z
    }
    return -1;
}

static int faceLayer(Face f) {
    switch (f) {
        case Face::R: return  1;
        case Face::L: return -1;
        case Face::U: return  1;
        case Face::D: return -1;
        case Face::F: return  1;
        case Face::B: return -1;
    }
    return 0;
}

std::vector<int> CubeState::cubiesOnFace(Face face) const {
    int axis  = faceAxis(face);
    int layer = faceLayer(face);
    std::vector<int> result;
    result.reserve(9);
    for (int i = 0; i < kCubieCount; ++i) {
        if (cubies_[i].position[axis] == layer)
            result.push_back(i);
    }
    return result;
}

Vector3 CubeState::axisForFace(Face face) {
    switch (face) {
        case Face::R: return { 1.0f, 0.0f, 0.0f};
        case Face::L: return {-1.0f, 0.0f, 0.0f};
        case Face::U: return { 0.0f, 1.0f, 0.0f};
        case Face::D: return { 0.0f,-1.0f, 0.0f};
        case Face::F: return { 0.0f, 0.0f, 1.0f};
        case Face::B: return { 0.0f, 0.0f,-1.0f};
    }
    return {};
}

float CubeState::angleForDirection(Direction dir) {
    // CW  = -90° around the face normal (right-hand rule: looking at the face,
    //        CW means the angle is negative about the outward normal)
    // CCW = +90°
    return float(dir == Direction::CW ? -90.0_degf : 90.0_degf);
}

bool CubeState::isSolved() const {
    for (const auto& c : cubies_) {
        if (c.position != c.homePosition) return false;
        const int nonzero = (c.homePosition.x() != 0) +
                            (c.homePosition.y() != 0) +
                            (c.homePosition.z() != 0);
        if (nonzero < 2) continue; // skip centers
        const Matrix3 rot3 = c.rotation.rotationScaling();
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                const float expected = (i == j) ? 1.0f : 0.0f;
                if (std::abs(rot3[i][j] - expected) > 1e-3f) return false;
            }
        }
    }
    return true;
}

void CubeState::applyMove(Move move) {
    auto indices = cubiesOnFace(move.face);
    Vector3 axis = axisForFace(move.face);
    float angle  = angleForDirection(move.direction);

    Matrix4 rot = Matrix4::rotation(Rad{Deg{angle}}, axis.normalized());
    // 3x3 portion for rotating the integer position vector
    Matrix3 rot3{rot.rotationScaling()};

    for (int i : indices) {
        auto& c = cubies_[i];
        // Update logical grid position (round to nearest int after rotation)
        Vector3 fp{Float(c.position.x()), Float(c.position.y()), Float(c.position.z())};
        Vector3 rp = rot3 * fp;
        c.position = Vector3i{Int(std::round(rp.x())),
                              Int(std::round(rp.y())),
                              Int(std::round(rp.z()))};
        // Accumulate rotation for rendering
        c.rotation = rot * c.rotation;
    }
}

// ── Facelet helpers ────────────────────────────────────────────────────────

int CubeState::faceletToAxis(int faceIdx) {
    // 0=U(+Y), 1=D(-Y), 2=F(+Z), 3=B(-Z), 4=L(-X), 5=R(+X)
    switch (faceIdx) {
        case 0: case 1: return 1; // y
        case 2: case 3: return 2; // z
        case 4: case 5: return 0; // x
    }
    return -1;
}

int CubeState::faceletToSign(int faceIdx) {
    switch (faceIdx) {
        case 0: return  1; // U: y=+1
        case 1: return -1; // D: y=-1
        case 2: return  1; // F: z=+1
        case 3: return -1; // B: z=-1
        case 4: return -1; // L: x=-1
        case 5: return  1; // R: x=+1
    }
    return 0;
}

// ── State reconstruction from facelet input ────────────────────────────────

namespace {

using V3i = Magnum::Vector3i;
using V3  = Magnum::Vector3;
using M3  = Magnum::Matrix3;

V3i faceNormal(int faceIdx) {
    int axis = CubeState::faceletToAxis(faceIdx);
    int sign = CubeState::faceletToSign(faceIdx);
    V3i normal{};
    normal[axis] = sign;
    return normal;
}

int stickerIndexForPosition(int faceIdx, V3i pos) {
    int row, col;
    switch (faceIdx) {
        case kFaceletU:
            col = pos.x() + 1; row = pos.z() + 1; break;
        case kFaceletD:
            col = pos.x() + 1; row = 1 - pos.z(); break;
        case kFaceletF:
            col = pos.x() + 1; row = 1 - pos.y(); break;
        case kFaceletB:
            col = 1 - pos.x(); row = 1 - pos.y(); break;
        case kFaceletL:
            col = pos.z() + 1; row = 1 - pos.y(); break;
        case kFaceletR:
            col = 1 - pos.z(); row = 1 - pos.y(); break;
        default:
            row = 0; col = 0; break;
    }
    return row * 3 + col;
}

V3i positionForFacelet(int faceIdx, int sticker) {
    int row = sticker / 3, col = sticker % 3;
    switch (faceIdx) {
        case kFaceletU: return {col - 1,  1, row - 1};
        case kFaceletD: return {col - 1, -1, 1 - row};
        case kFaceletF: return {col - 1, 1 - row,  1};
        case kFaceletB: return {1 - col, 1 - row, -1};
        case kFaceletL: return {-1, 1 - row, col - 1};
        case kFaceletR: return { 1, 1 - row, 1 - col};
    }
    return {};
}

// Build 3×3 matrix from 3 column vectors.
M3 matFromCols(const V3& a, const V3& b, const V3& c) {
    M3 m;
    m[0] = a;
    m[1] = b;
    m[2] = c;
    return m;
}

V3 perpendicularTo(const V3& v) {
    if (std::abs(v.x()) < 0.5f) return {1.0f, 0.0f, 0.0f};
    return {0.0f, 1.0f, 0.0f};
}

// Color triple → cubie identity.  Key = sorted FaceletColor values as a u32.
struct CubieIdentity {
    int homeIdx;           // index into cubies_ array
    FaceletColor colors[3];
    V3i homeDirections[3]; // world-space directions of stickers when solved
    int numStickers;       // 1 (center), 2 (edge), 3 (corner)
};

FaceletColor homeColor(int axis, int sign) {
    // axis: 0=x, 1=y, 2=z; sign: +1 or -1
    switch (axis) {
        case 0: return sign > 0 ? FaceletColor::Red    : FaceletColor::Orange;
        case 1: return sign > 0 ? FaceletColor::White  : FaceletColor::Yellow;
        case 2: return sign > 0 ? FaceletColor::Green  : FaceletColor::Blue;
    }
    return FaceletColor::White;
}

uint32_t colorKey(const FaceletColor colors[3], int n) {
    FaceletColor keyColors[3] = {FaceletColor::White, FaceletColor::White, FaceletColor::White};
    for (int i = 0; i < n; ++i) keyColors[i] = colors[i];
    return (uint32_t(keyColors[0]) << 16) |
           (uint32_t(keyColors[1]) << 8)  |
            uint32_t(keyColors[2]);
}

std::unordered_map<uint32_t, CubieIdentity> buildCubieMap() {
    std::unordered_map<uint32_t, CubieIdentity> map;
    int idx = 0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            for (int z = -1; z <= 1; ++z) {
                if (x == 0 && y == 0 && z == 0) continue;
                V3i pos{x, y, z};
                // Determine which faces this cubie is on.
                int axes[3], signs[3], n = 0;
                if (x != 0) { axes[n] = 0; signs[n] = x; ++n; }
                if (y != 0) { axes[n] = 1; signs[n] = y; ++n; }
                if (z != 0) { axes[n] = 2; signs[n] = z; ++n; }
                // Build sorted color key.
                CubieIdentity ci;
                ci.homeIdx = idx;
                ci.numStickers = n;
                for (int i = 0; i < n; ++i) {
                    ci.colors[i] = homeColor(axes[i], signs[i]);
                    V3i d{}; d[axes[i]] = signs[i];
                    ci.homeDirections[i] = d;
                }
                // Keep colors and home directions in the same order.
                for (int i = 1; i < n; ++i) {
                    for (int j = i; j > 0 && ci.colors[j] < ci.colors[j-1]; --j) {
                        std::swap(ci.colors[j], ci.colors[j-1]);
                        std::swap(ci.homeDirections[j], ci.homeDirections[j-1]);
                    }
                }
                uint32_t key = colorKey(ci.colors, n);
                map[key] = ci;
                ++idx;
            }
        }
    }
    return map;
}

// Determine which facelet face indices a cubie at `pos` is visible on.
void visibleFaces(V3i pos, int outFaces[3], int outStickers[3], int& count) {
    count = 0;
    // Map each non-zero coordinate to its facelet face index.
    if (pos.y() != 0) { outFaces[count] = pos.y() > 0 ? kFaceletU : kFaceletD; ++count; }
    if (pos.z() != 0) { outFaces[count] = pos.z() > 0 ? kFaceletF : kFaceletB; ++count; }
    if (pos.x() != 0) { outFaces[count] = pos.x() > 0 ? kFaceletR : kFaceletL; ++count; }
    // Find the sticker index for each face.
    for (int i = 0; i < count; ++i) {
        outStickers[i] = stickerIndexForPosition(outFaces[i], pos);
    }
}

} // anonymous namespace

bool CubeState::applyFaceletInput(const FaceletColor facelets[6][9]) {
    // Validate: each color must appear exactly 9 times.
    int counts[6] = {};
    for (int f = 0; f < 6; ++f)
        for (int s = 0; s < 9; ++s)
            ++counts[int(facelets[f][s])];
    for (int i = 0; i < 6; ++i)
        if (counts[i] != 9) return false;

    auto cubieMap = buildCubieMap();
    std::array<CubieInfo, kCubieCount> next = cubies_;
    bool assigned[kCubieCount] = {};

    for (int posIdx = 0; posIdx < kCubieCount; ++posIdx) {
        V3i pos = cubies_[posIdx].homePosition;

        // Determine which faces this cubie is on and read sticker colors.
        int faceIndices[3], stickerIndices[3], numFaces = 0;
        visibleFaces(pos, faceIndices, stickerIndices, numFaces);

        FaceletColor colors[3] = {};
        V3i worldNormals[3] = {};
        for (int i = 0; i < numFaces; ++i) {
            colors[i] = facelets[faceIndices[i]][stickerIndices[i]];
            worldNormals[i] = faceNormal(faceIndices[i]);
        }

        // Sort colors and their current world normals together.
        for (int i = 1; i < numFaces; ++i) {
            for (int j = i; j > 0 && colors[j] < colors[j-1]; --j) {
                std::swap(colors[j], colors[j-1]);
                std::swap(worldNormals[j], worldNormals[j-1]);
            }
        }

        uint32_t key = colorKey(colors, numFaces);

        auto it = cubieMap.find(key);
        if (it == cubieMap.end()) {
            LOG_WARN("applyFaceletInput FAILED: cubie idx={} pos=({},{},{}) colors={},{},{} key=0x{:x}",
                     posIdx, pos.x(), pos.y(), pos.z(),
                     int(colors[0]), int(colors[1]), int(colors[2]), key);
            return false; // invalid color combination
        }
        const CubieIdentity& ci = it->second;
        if (assigned[ci.homeIdx]) {
            LOG_WARN("applyFaceletInput FAILED: duplicate cubie homeIdx={} at pos=({},{},{})",
                     ci.homeIdx, pos.x(), pos.y(), pos.z());
            return false;
        }
        assigned[ci.homeIdx] = true;

        // Build home direction matrix H: columns = home direction unit vectors.
        V3 homeDirs[3] = {
            V3{float(ci.homeDirections[0].x()),
               float(ci.homeDirections[0].y()),
               float(ci.homeDirections[0].z())},
            V3{},
            V3{}
        };
        if (ci.numStickers >= 2)
            homeDirs[1] = V3{float(ci.homeDirections[1].x()),
                             float(ci.homeDirections[1].y()),
                             float(ci.homeDirections[1].z())};
        else
            homeDirs[1] = perpendicularTo(homeDirs[0]);
        if (ci.numStickers >= 3)
            homeDirs[2] = V3{float(ci.homeDirections[2].x()),
                             float(ci.homeDirections[2].y()),
                             float(ci.homeDirections[2].z())};
        else
            homeDirs[2] = Magnum::Math::cross(homeDirs[0], homeDirs[1]);
        M3 H = matFromCols(homeDirs[0], homeDirs[1], homeDirs[2]);

        // Build target direction matrix W: columns = world directions from input.
        V3 worldDirs[3] = {
            V3{float(worldNormals[0].x()), float(worldNormals[0].y()), float(worldNormals[0].z())},
            V3{},
            V3{}
        };
        if (ci.numStickers >= 2)
            worldDirs[1] = V3{float(worldNormals[1].x()),
                              float(worldNormals[1].y()),
                              float(worldNormals[1].z())};
        else
            worldDirs[1] = perpendicularTo(worldDirs[0]);
        if (ci.numStickers >= 3)
            worldDirs[2] = V3{float(worldNormals[2].x()),
                              float(worldNormals[2].y()),
                              float(worldNormals[2].z())};
        else
            worldDirs[2] = Magnum::Math::cross(worldDirs[0], worldDirs[1]);
        M3 W = matFromCols(worldDirs[0], worldDirs[1], worldDirs[2]);

        // R = W * H^T (H is orthonormal, so H^-1 = H^T).
        M3 R = W * H.transposed();

        // Validate: R must be a proper rotation (det ≈ +1).
        float det = R.determinant();
        if (std::abs(det - 1.0f) > 0.1f) {
            LOG_WARN("applyFaceletInput FAILED: invalid rotation det={} homeIdx={} pos=({},{},{}) "
                     "homeDet={} worldDet={} homeDirs=({},{},{}),({},{},{}),({},{},{}) worldDirs=({},{},{}),({},{},{}),({},{},{})",
                     det, ci.homeIdx, pos.x(), pos.y(), pos.z(),
                     H.determinant(), W.determinant(),
                     ci.homeDirections[0].x(), ci.homeDirections[0].y(), ci.homeDirections[0].z(),
                     ci.homeDirections[1].x(), ci.homeDirections[1].y(), ci.homeDirections[1].z(),
                     ci.homeDirections[2].x(), ci.homeDirections[2].y(), ci.homeDirections[2].z(),
                     worldNormals[0].x(), worldNormals[0].y(), worldNormals[0].z(),
                     worldNormals[1].x(), worldNormals[1].y(), worldNormals[1].z(),
                     worldNormals[2].x(), worldNormals[2].y(), worldNormals[2].z());
            return false;
        }

        // Store the identified home cubie at this current position.
        next[ci.homeIdx].position = pos;
        next[ci.homeIdx].rotation = Matrix4{R};
    }
    for (int i = 0; i < kCubieCount; ++i) {
        if (!assigned[i]) {
            LOG_WARN("applyFaceletInput FAILED: missing cubie homeIdx={}", i);
            return false;
        }
    }
    cubies_ = next;
    return true;
}

void CubeState::toFacelets(FaceletColor facelets[6][9]) const {
    for (int f = 0; f < 6; ++f) {
        FaceletColor c = homeColor(faceletToAxis(f), faceletToSign(f));
        for (int s = 0; s < 9; ++s) facelets[f][s] = c;
    }

    for (int faceIdx = 0; faceIdx < 6; ++faceIdx) {
        V3i normal = faceNormal(faceIdx);
        for (int sticker = 0; sticker < 9; ++sticker) {
            V3i pos = positionForFacelet(faceIdx, sticker);
            for (const auto& cubie : cubies_) {
                if (cubie.position != pos) continue;

                const Matrix3 rot = cubie.rotation.rotationScaling();
                for (int axis = 0; axis < 3; ++axis) {
                    int sign = cubie.homePosition[axis];
                    if (sign == 0) continue;

                    V3 homeDir{};
                    homeDir[axis] = float(sign);
                    V3 rotated = rot * homeDir;
                    V3i worldNormal{Int(std::round(rotated.x())),
                                    Int(std::round(rotated.y())),
                                    Int(std::round(rotated.z()))};
                    if (worldNormal == normal) {
                        facelets[faceIdx][sticker] = homeColor(axis, sign);
                        break;
                    }
                }
                break;
            }
        }
    }
}

} // namespace rubik
