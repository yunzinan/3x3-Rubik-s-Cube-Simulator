#pragma once

#include "core/CubeState.h"
#include "core/Move.h"
#include "render/ArcBallCamera.h"
#include "render/CubieDrawable.h"

#include <Magnum/GL/Mesh.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/Shaders/PhongGL.h>

#include <array>

namespace rubik {

// Manages the 3D scene graph for the Rubik's Cube: 26 cubie objects,
// plus a temporary rotation group used during face-turn animations.
class CubeScene {
public:
    CubeScene();

    void setup(const Magnum::Vector2i& windowSize);

    // Synchronise cubie Object3D transforms from CubeState (call after
    // a move is fully applied, i.e. when *not* animating).
    void syncFromState(const CubeState& state);

    // --- Animation helpers ---

    // Before animating: attach the 9 cubies of `face` to the rotation group.
    void beginFaceRotation(Face face, const CubeState& state);

    // During animation: set the rotation group's angle [0 .. finalAngle].
    void setFaceRotationAngle(Face face, Direction dir, float t);

    // After animation: detach cubies back to cubeRoot and sync from state.
    void endFaceRotation(const CubeState& state);

    // --- Drawing ---
    void draw();
    void reshape(const Magnum::Vector2i& windowSize);

    // Reset camera to default view (orbit and zoom).
    void resetView();

    ArcBallCamera& arcBall() { return *arcBall_; }

private:
    Scene3D scene_;
    Object3D* cameraObject_ = nullptr;
    Magnum::SceneGraph::Camera3D* camera_ = nullptr;
    std::unique_ptr<ArcBallCamera> arcBall_;

    Object3D* cubeRoot_ = nullptr;
    Object3D* rotationGroup_ = nullptr;

    Magnum::Shaders::PhongGL shader_{Magnum::NoCreate};
    Magnum::SceneGraph::DrawableGroup3D drawables_;

    struct CubieNode {
        Object3D* object = nullptr;
        CubieDrawable* drawable = nullptr;
    };
    std::array<CubieNode, kCubieCount> cubieNodes_;

    // Indices into cubieNodes_ currently attached to rotationGroup_.
    std::vector<int> animatingIndices_;
};

} // namespace rubik
