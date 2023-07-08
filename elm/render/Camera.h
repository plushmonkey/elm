#pragma once

#include <elm/Math.h>

namespace elm {
namespace render {

struct Camera {
  mat4 projection;
  Vector2f position = Vector2f(0, 0);
  Vector2f surface_dim;
  float scale = 0.0f;

  Camera(const Vector2f& surface_dim, const Vector2f& position, float scale)
      : surface_dim(surface_dim), position(position), scale(scale) {
    SetScale(scale);
  }

  void SetScale(float new_scale) {
    scale = new_scale;

    float zmax = 1.0f;

    projection = Orthographic(-surface_dim.x / 2.0f * scale, surface_dim.x / 2.0f * scale, surface_dim.y / 2.0f * scale,
                              -surface_dim.y / 2.0f * scale, -zmax, zmax);
  }

  mat4 GetView() { return Translate(mat4::Identity(), Vector3f(-position.x, -position.y, 0.0f)); }

  mat4 GetProjection() { return projection; }
  void RoundToPixels() {
    s32 x32 = (s32)(position.x * 16.0f);
    s32 y32 = (s32)(position.y * 16.0f);

    position = Vector2f(x32 / 16.0f, y32 / 16.0f);
  }

  inline Vector2f Unproject(Vector2f screen_coords) {
    Vector2f half_dim = surface_dim * 0.5f;
    Vector2f input(screen_coords.x - half_dim.x, screen_coords.y - half_dim.y);

    float x = position.x + input.x * scale;
    float y = position.y + input.y * scale;

    return Vector2f(x, y);
  }
};

}  // namespace render
}  // namespace elm
