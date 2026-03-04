#pragma once

#include "render/CubieDrawable.h" // for Object3D, Scene3D typedefs

#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/SceneGraph/Camera.h>

namespace rubik {

// Orbit-style camera that can rotate, zoom, and pan.
// Operates independently of the cube animation system.
class ArcBallCamera {
public:
    ArcBallCamera(Object3D& cameraObject,
                  Magnum::SceneGraph::Camera3D& camera,
                  const Magnum::Vector3& target,
                  float distance,
                  const Magnum::Vector2i& windowSize);

    // Call on viewport change
    void reshape(const Magnum::Vector2i& windowSize);

    // Orbit: delta in screen pixels
    void rotate(const Magnum::Vector2i& delta);

    // Zoom: positive = zoom in
    void zoom(float delta);

    // Pan: delta in screen pixels
    void pan(const Magnum::Vector2i& delta);

    // Reset camera to default orbit (azimuth=0, elevation=0.3, distance=initial, no pan)
    void resetView();

    void updateTransform();

    Magnum::SceneGraph::Camera3D& camera() { return camera_; }

private:
    Object3D& cameraObject_;
    Magnum::SceneGraph::Camera3D& camera_;

    Magnum::Vector3 target_;  // orbit center (cube center), never changed by pan
    float distance_;
    float initialDistance_;  // used by resetView()
    float azimuth_   = 0.0f; // horizontal angle (radians)
    float elevation_ = 0.3f; // vertical angle (radians)
    Magnum::Vector2 panOffset_{0.0f}; // pan in view plane (right, up), so rotation stays around target_

    Magnum::Vector2i windowSize_;

    static constexpr float kZoomSpeed = 0.1f;
    static constexpr float kRotateSpeed = 0.005f;
    static constexpr float kMinDistance = 3.0f;
    static constexpr float kMaxDistance = 30.0f;
};

} // namespace rubik
