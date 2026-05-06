#include "render/CubeScene.h"

#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Angle.h>

namespace rubik {

using namespace Magnum;
using namespace Magnum::Math::Literals;

CubeScene::CubeScene() = default;

void CubeScene::setup(const Vector2i& windowSize) {
    // Shader: two lights (right + left) for even illumination
    shader_ = Shaders::PhongGL{Shaders::PhongGL::Configuration{}.setLightCount(2)};
    shader_.setLightPositions({{5.0f, 5.0f, 7.0f, 0.0f}, {-5.0f, 5.0f, 7.0f, 0.0f}})
           .setLightColors({0xffffff_rgbf, 0xffffff_rgbf})
           .setSpecularColor(0x222222_rgbf)
           .setShininess(80.0f);

    // Camera object
    cameraObject_ = new Object3D{&scene_};
    camera_ = new SceneGraph::Camera3D{*cameraObject_};
    camera_->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
           .setProjectionMatrix(Matrix4::perspectiveProjection(
               35.0_degf,
               Float(windowSize.x()) / Float(windowSize.y()),
               0.1f, 100.0f))
           .setViewport(windowSize);

    arcBall_ = std::make_unique<ArcBallCamera>(
        *cameraObject_, *camera_,
        Vector3{0.0f},   // target = cube centre
        10.0f,            // initial distance
        windowSize);

    // Cube root & rotation group
    cubeRoot_      = new Object3D{&scene_};
    rotationGroup_ = new Object3D{cubeRoot_};

    // Build 26 cubie objects
    CubeState solved;
    createCubies(solved);
}

void CubeScene::syncFromState(const CubeState& state) {
    constexpr float gap = 1.0f;
    const auto& cubies = state.cubies();
    for (int i = 0; i < kCubieCount; ++i) {
        const auto& c = cubies[i];
        Matrix4 translation = Matrix4::translation(
            Vector3{c.position} * gap);
        cubieNodes_[i].object->setTransformation(translation * c.rotation);
    }
}

void CubeScene::beginFaceRotation(Face face, const CubeState& state) {
    animatingIndices_ = state.cubiesOnFace(face);
    rotationGroup_->resetTransformation();
    for (int idx : animatingIndices_) {
        cubieNodes_[idx].object->setParent(rotationGroup_);
    }
}

void CubeScene::setFaceRotationAngle(Face face, Direction dir, float t) {
    Vector3 axis  = CubeState::axisForFace(face);
    float   angle = CubeState::angleForDirection(dir);
    rotationGroup_->setTransformation(
        Matrix4::rotation(Rad{Deg{angle * t}}, axis.normalized()));
}

void CubeScene::endFaceRotation(const CubeState& state) {
    // Re-parent cubies back to cubeRoot
    for (int idx : animatingIndices_) {
        cubieNodes_[idx].object->setParent(cubeRoot_);
    }
    animatingIndices_.clear();
    rotationGroup_->resetTransformation();
    syncFromState(state);
}

void CubeScene::createCubies(const CubeState& state) {
    constexpr float gap = 1.0f;
    const auto& cubies = state.cubies();
    for (int i = 0; i < kCubieCount; ++i) {
        const auto& c = cubies[i];
        auto* obj = new Object3D{cubeRoot_};
        obj->setTransformation(
            Matrix4::translation(Vector3{c.homePosition} * gap));
        auto* drw = new CubieDrawable{*obj, shader_, drawables_, c.homePosition};
        cubieNodes_[i] = {obj, drw};
    }
}

void CubeScene::destroyCubies() {
    // Destroying cubeRoot_ cascades to all child Object3D nodes (and their
    // CubieDrawable, which auto-unregisters from drawables_).
    delete cubeRoot_;
    cubeRoot_      = nullptr;
    rotationGroup_ = nullptr;
    for (auto& n : cubieNodes_) n = {};
}

void CubeScene::rebuildFromState(const CubeState& state) {
    destroyCubies();
    cubeRoot_      = new Object3D{&scene_};
    rotationGroup_ = new Object3D{cubeRoot_};
    createCubies(state);
    syncFromState(state);
}

void CubeScene::setFaceletColors(const CubeState& state,
                                  const FaceletColor facelets[6][9]) {
    const Color3 kFaceletColors[] = {
        CubeColors::White, CubeColors::Yellow, CubeColors::Green,
        CubeColors::Blue,  CubeColors::Red,    CubeColors::Orange
    };
    // faceIdxMap[axis][sign>0 ? 0 : 1] = facelet face index
    constexpr int faceIdxMap[3][2] = {{5, 4}, {0, 1}, {2, 3}};

    for (int idx = 0; idx < kCubieCount; ++idx) {
        auto* drw = cubieNodes_[idx].drawable;
        if (!drw) continue;
        Vector3i pos = state.cubies()[idx].position;
        Color3 colors[6] = {};
        bool visible[6] = {};
        for (int axis = 0; axis < 3; ++axis) {
            int coord = pos[axis];
            if (coord == 0) continue;
            int fi = faceIdxMap[axis][coord > 0 ? 0 : 1];
            int ax2 = CubeState::faceletToAxis(fi);
            int u = (ax2 + 1) % 3, v = (ax2 + 2) % 3;
            int uVal = pos[u], vVal = pos[v];
            int row, col;
            switch (fi) {
                case kFaceletU: col=(uVal+1)/2; row=(1-vVal)/2; break;
                case kFaceletD: col=(uVal+1)/2; row=(vVal+1)/2; break;
                case kFaceletF: col=(uVal+1)/2; row=(1-vVal)/2; break;
                case kFaceletB: col=(1-uVal)/2; row=(1-vVal)/2; break;
                case kFaceletL: col=(uVal+1)/2; row=(1-vVal)/2; break;
                case kFaceletR: col=(1-uVal)/2; row=(1-vVal)/2; break;
                default: row=0; col=0; break;
            }
            FaceletColor fc = facelets[fi][row * 3 + col];
            colors[fi] = kFaceletColors[int(fc)];
            visible[fi] = true;
        }
        drw->setFaceColors(colors, visible);
    }
}

void CubeScene::draw() {
    camera_->draw(drawables_);
}

void CubeScene::reshape(const Vector2i& windowSize) {
    camera_->setProjectionMatrix(Matrix4::perspectiveProjection(
        35.0_degf,
        Float(windowSize.x()) / Float(windowSize.y()),
        0.1f, 100.0f));
    arcBall_->reshape(windowSize);
}

void CubeScene::resetView() {
    arcBall_->resetView();
}

} // namespace rubik
