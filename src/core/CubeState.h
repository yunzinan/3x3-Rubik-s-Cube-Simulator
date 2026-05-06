#pragma once

#include "core/Move.h"

#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>

#include <array>
#include <cstdint>
#include <vector>

namespace rubik {

// The 6 facelet colors used for state input (matches standard Western scheme).
enum class FaceletColor : uint8_t {
    White,   // U face (+Y)
    Yellow,  // D face (-Y)
    Green,   // F face (+Z)
    Blue,    // B face (-Z)
    Red,     // R face (+X)
    Orange   // L face (-X)
};

// Face index mapping: 0=U, 1=D, 2=F, 3=B, 4=L, 5=R
inline constexpr int kFaceletU = 0, kFaceletD = 1, kFaceletF = 2,
                     kFaceletB = 3, kFaceletL = 4, kFaceletR = 5;

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

    // Reconstruct cube state from 54 sticker colors (facelet input).
    // facelets[face][sticker] where face: 0=U,1=D,2=F,3=B,4=L,5=R
    // and sticker goes left-to-right, top-to-bottom per face in cross layout.
    // Returns false if the input is invalid (e.g. wrong color counts).
    bool applyFaceletInput(const FaceletColor facelets[6][9]);

    // Export current logical state as facelet colors using the same layout as
    // applyFaceletInput().
    void toFacelets(FaceletColor facelets[6][9]) const;

    // Map face index (0=U,1=D,2=F,3=B,4=L,5=R) to axis (0=x,1=y,2=z).
    static int faceletToAxis(int faceIdx);

    // Map face index to layer sign (+1 or -1).
    static int faceletToSign(int faceIdx);

private:
    std::array<CubieInfo, kCubieCount> cubies_;

    void initCubies();
};

} // namespace rubik
