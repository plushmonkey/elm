#pragma once

#include <elm/Math.h>
#include <elm/render/Camera.h>
#include <elm/render/LineRenderer.h>
#include <elm/render/MapRenderer.h>

struct GLFWwindow;

namespace elm {

struct DragState {
  bool active = false;
  Vector2f start;
};

struct Elm {
  render::Camera camera;
  render::MapRenderer map_renderer;
  render::LineRenderer line_renderer;
  DragState drag;
  GLFWwindow* window;

  Elm(GLFWwindow* window, const Vector2f& surface_dim)
      : window(window), camera(surface_dim, Vector2f(512, 512), 1.0f / 16.0f) {}

  bool Initialize(const char* map_filename, const Map& map);
  void Render(bool clear_lines = true);
};

}  // namespace elm
