#pragma once

#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/PhongGL.h>

namespace rubik {

using Object3D = Magnum::SceneGraph::Object<
    Magnum::SceneGraph::MatrixTransformation3D>;
using Scene3D = Magnum::SceneGraph::Scene<
    Magnum::SceneGraph::MatrixTransformation3D>;

// Standard Rubik's cube face colours
namespace CubeColors {
    using namespace Magnum::Math::Literals;
    inline const Magnum::Color3 White  = 0xffffff_rgbf;
    inline const Magnum::Color3 Yellow = 0xffd500_rgbf;
    inline const Magnum::Color3 Red    = 0xc41e3a_rgbf;
    inline const Magnum::Color3 Orange = 0xff5800_rgbf;
    inline const Magnum::Color3 Green  = 0x009e60_rgbf;
    inline const Magnum::Color3 Blue   = 0x0051ba_rgbf;
    inline const Magnum::Color3 Black  = 0x1a1a1a_rgbf; // internal faces
}

// Draws one cubie (small cube) with per-face colouring.
// Each cubie is composed of 6 quads, each a separate GL::Mesh so we can
// assign individual colours.
class CubieDrawable : public Magnum::SceneGraph::Drawable3D {
public:
    // homePos: initial grid position {-1..1}^3 – determines which faces
    //          get real colours vs. black.
    explicit CubieDrawable(
        Object3D& object,
        Magnum::Shaders::PhongGL& shader,
        Magnum::SceneGraph::DrawableGroup3D& drawables,
        Magnum::Vector3i homePos);

private:
    void draw(const Magnum::Matrix4& transformationMatrix,
              Magnum::SceneGraph::Camera3D& camera) override;

    Magnum::Shaders::PhongGL& shader_;

    // 6 face meshes with their colours
    struct FaceMesh {
        Magnum::GL::Mesh mesh;
        Magnum::Color3 color;
    };
    std::array<FaceMesh, 6> faces_;

    void buildFaces(Magnum::Vector3i homePos);
};

} // namespace rubik
