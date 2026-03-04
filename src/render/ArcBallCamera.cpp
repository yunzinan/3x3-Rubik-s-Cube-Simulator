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
    , initialDistance_{distance}
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
    if (windowSize_.y() <= 0) return;

    // Convert mouse pixels to world-space shift at the orbit depth.
    // This keeps pan sensitivity consistent across resolutions and DPI.
    constexpr Float verticalFovDeg = 35.0f;
    const Float worldPerPixel = (2.0f * distance_ *
        Math::tan(Rad{Deg{verticalFovDeg*0.5f}})) / Float(windowSize_.y());

    panOffset_.x() -= Float(delta.x()) * worldPerPixel;
    panOffset_.y() += Float(delta.y()) * worldPerPixel;
    updateTransform();
}

void ArcBallCamera::resetView() {
    azimuth_ = 0.0f;
    elevation_ = 0.3f;
    distance_ = initialDistance_;
    panOffset_ = Vector2{0.0f};
    updateTransform();
}

void ArcBallCamera::updateTransform() {
    float cx = std::cos(elevation_) * std::cos(azimuth_);
    float cy = std::sin(elevation_);
    float cz = std::cos(elevation_) * std::sin(azimuth_);

    Vector3 right = Matrix4::rotationY(Rad{azimuth_}).transformVector({1, 0, 0});
    Vector3 up{0, 1, 0};
    const Vector3 panWorld = right * panOffset_.x() + up * panOffset_.y();
    Vector3 eye = target_ + Vector3{cz, cy, cx} * distance_ + panWorld;
    Vector3 center = target_ + panWorld;

    cameraObject_.setTransformation(
        Matrix4::lookAt(eye, center, {0.0f, 1.0f, 0.0f}));
}

} // namespace rubik
