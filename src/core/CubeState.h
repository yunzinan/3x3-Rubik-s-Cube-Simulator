#pragma once

#include "core/Move.h"

#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>

#include <array>
#include <vector>

namespace rubik {

// Each cubie is identified by its initial grid position (x, y, z) ∈ {-1, 0, 1}.
// We store 26 visible cubies (excluding the hidden centre at origin).
inline constexpr int kCubieCount = 26;

struct CubieInfo {
    Magnum::Vector3i homePosition; // position when cube is solved
    Magnum::Vector3i position;     // current logical grid position
    Magnum::Matrix4  rotation;     // accumulated rotation (for rendering)
};

class CubeState {
public:
    CubeState();

    void applyMove(Move move);
    void reset();

    // Visually solved: all cubies at home position, corners and edges with
    // identity orientation. Center rotation is ignored (center stickers are
    // on the rotation axis so their rotation is not visible).
    bool isSolved() const;

    const std::array<CubieInfo, kCubieCount>& cubies() const { return cubies_; }

    // Return indices (into cubies_) of the 9 cubies sitting on `face`.
    std::vector<int> cubiesOnFace(Face face) const;

    // Rotation axis & angle (in radians) for a given move (used by animation).
    static Magnum::Vector3 axisForFace(Face face);
    static float angleForDirection(Direction dir);

private:
    std::array<CubieInfo, kCubieCount> cubies_;

    void initCubies();
};

} // namespace rubik
