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
    inline const Magnum::Color3 Yellow = 0xffed00_rgbf;  // pure yellow, less orange
    inline const Magnum::Color3 Red    = 0xc41e3a_rgbf;
    inline const Magnum::Color3 Orange = 0xff4500_rgbf;  // deeper orange-red, more distinct
    inline const Magnum::Color3 Green  = 0x009e60_rgbf;
    inline const Magnum::Color3 Blue   = 0x0051ba_rgbf;
    inline const Magnum::Color3 Black  = 0x1a1a1a_rgbf; // internal / body plastic
}

// Draws one cubie as a rounded black body with coloured sticker faces.
class CubieDrawable : public Magnum::SceneGraph::Drawable3D {
public:
    // homePos: initial grid position {-1..1}^3 – determines which faces
    //          get real colours vs. internal (no sticker).
    explicit CubieDrawable(
        Object3D& object,
        Magnum::Shaders::PhongGL& shader,
        Magnum::SceneGraph::DrawableGroup3D& drawables,
        Magnum::Vector3i homePos);

    // Override sticker colors and visibility (used by state-input feature).
    // faceIdx: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z
    void setFaceColors(const Magnum::Color3 colors[6], const bool visible[6]);

private:
    void draw(const Magnum::Matrix4& transformationMatrix,
              Magnum::SceneGraph::Camera3D& camera) override;

    Magnum::Shaders::PhongGL& shader_;

    Magnum::GL::Mesh bodyMesh_;

    struct Sticker {
        Magnum::GL::Mesh mesh;
        Magnum::GL::Mesh borderMesh;  // vertical wall from sticker to body
        Magnum::Color3 color;
        bool visible = false;
    };
    std::array<Sticker, 6> stickers_;

    void buildMeshes(Magnum::Vector3i homePos);
};

} // namespace rubik
