#include "core/CubeState.h"

#include <Magnum/Math/Angle.h>
#include <Magnum/Math/Matrix3.h>

#include <cmath>

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

} // namespace rubik
