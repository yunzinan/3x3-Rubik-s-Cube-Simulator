#include "render/ArcBallCamera.h"

#include <Magnum/Math/Angle.h>
#include <Magnum/Math/Functions.h>

#include <algorithm>
#include <cmath>

namespace rubik {

using namespace Magnum;
using namespace Magnum::Math::Literals;

ArcBallCamera::ArcBallCamera(
    Object3D& cameraObject,
    SceneGraph::Camera3D& camera,
    const Vector3& target,
    float distance,
    const Vector2i& windowSize)
    : cameraObject_{cameraObject}
    , camera_{camera}
    , target_{target}
    , distance_{distance}
    , windowSize_{windowSize}
{
    updateTransform();
}

void ArcBallCamera::reshape(const Vector2i& windowSize) {
    windowSize_ = windowSize;
    camera_.setViewport(windowSize);
}

void ArcBallCamera::rotate(const Vector2i& delta) {
    azimuth_   -= Float(delta.x()) * kRotateSpeed;
    elevation_ += Float(delta.y()) * kRotateSpeed;

    constexpr float limit = 1.5f; // ~86°
    elevation_ = Math::clamp(elevation_, -limit, limit);
    updateTransform();
}

void ArcBallCamera::zoom(float delta) {
    distance_ -= delta * kZoomSpeed * distance_;
    distance_ = Math::clamp(distance_, kMinDistance, kMaxDistance);
    updateTransform();
}

void ArcBallCamera::pan(const Vector2i& delta) {
    float scale = distance_ * kPanSpeed;
    Vector3 right = Matrix4::rotationY(Rad{azimuth_}).transformVector({1, 0, 0});
    Vector3 up{0, 1, 0};
    target_ -= right * Float(delta.x()) * scale;
    target_ += up    * Float(delta.y()) * scale;
    updateTransform();
}

void ArcBallCamera::updateTransform() {
    float cx = std::cos(elevation_) * std::cos(azimuth_);
    float cy = std::sin(elevation_);
    float cz = std::cos(elevation_) * std::sin(azimuth_);

    Vector3 eye = target_ + Vector3{cz, cy, cx} * distance_;

    cameraObject_.setTransformation(
        Matrix4::lookAt(eye, target_, {0.0f, 1.0f, 0.0f}));
}

} // namespace rubik
