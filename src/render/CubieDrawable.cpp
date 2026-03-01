#include "render/CubieDrawable.h"

#include <Magnum/GL/Renderer.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/Trade/MeshData.h>

namespace rubik {

using namespace Magnum;
using namespace Magnum::Math::Literals;

namespace {

// Map face index to the colour for a cubie at `homePos`.
//   0: +X (Right), 1: -X (Left)
//   2: +Y (Up),    3: -Y (Down)
//   4: +Z (Front), 5: -Z (Back)
Color3 faceColor(int faceIdx, Vector3i homePos) {
    switch (faceIdx) {
        case 0: return homePos.x() ==  1 ? CubeColors::Red    : CubeColors::Black;
        case 1: return homePos.x() == -1 ? CubeColors::Orange  : CubeColors::Black;
        case 2: return homePos.y() ==  1 ? CubeColors::White   : CubeColors::Black;
        case 3: return homePos.y() == -1 ? CubeColors::Yellow  : CubeColors::Black;
        case 4: return homePos.z() ==  1 ? CubeColors::Green   : CubeColors::Black;
        case 5: return homePos.z() == -1 ? CubeColors::Blue    : CubeColors::Black;
    }
    return CubeColors::Black;
}

// Build one face of a unit (1x1x1) cube.
// Primitives::planeSolid() is a 2x2 quad in XY at z=0, so we scale it to
// 1x1 (half-extent 0.5), then rotate + translate to the correct face.
GL::Mesh buildFaceMesh(int faceIdx) {
    auto plane = Primitives::planeSolid(Primitives::PlaneFlag::TextureCoordinates);

    const float h = 0.5f;
    Matrix4 scale = Matrix4::scaling(Vector3{h}); // 2x2 → 1x1
    Matrix4 xf;
    switch (faceIdx) {
        case 0: xf = Matrix4::translation({ h, 0, 0}) * Matrix4::rotationY( 90.0_degf) * scale; break;
        case 1: xf = Matrix4::translation({-h, 0, 0}) * Matrix4::rotationY(-90.0_degf) * scale; break;
        case 2: xf = Matrix4::translation({ 0, h, 0}) * Matrix4::rotationX(-90.0_degf) * scale; break;
        case 3: xf = Matrix4::translation({ 0,-h, 0}) * Matrix4::rotationX( 90.0_degf) * scale; break;
        case 4: xf = Matrix4::translation({ 0, 0, h})                                  * scale; break;
        case 5: xf = Matrix4::translation({ 0, 0,-h}) * Matrix4::rotationY(180.0_degf) * scale; break;
    }

    return MeshTools::compile(MeshTools::transform3D(plane, xf));
}

} // anonymous namespace

CubieDrawable::CubieDrawable(
    Object3D& object,
    Shaders::PhongGL& shader,
    SceneGraph::DrawableGroup3D& drawables,
    Vector3i homePos)
    : SceneGraph::Drawable3D{object, &drawables}
    , shader_{shader}
{
    buildFaces(homePos);
}

void CubieDrawable::buildFaces(Vector3i homePos) {
    for (int i = 0; i < 6; ++i) {
        faces_[i].mesh  = buildFaceMesh(i);
        faces_[i].color = faceColor(i, homePos);
    }
}

void CubieDrawable::draw(
    const Matrix4& transformationMatrix,
    SceneGraph::Camera3D& camera)
{
    for (auto& f : faces_) {
        shader_
            .setDiffuseColor(f.color)
            .setAmbientColor(f.color * 0.15f)
            .setTransformationMatrix(transformationMatrix)
            .setNormalMatrix(transformationMatrix.normalMatrix())
            .setProjectionMatrix(camera.projectionMatrix())
            .draw(f.mesh);
    }
}

} // namespace rubik
