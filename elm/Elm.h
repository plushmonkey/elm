#pragma once

#include <elm/Map.h>
#include <elm/Math.h>
#include <elm/path/Pathfinder.h>
#include <elm/render/Camera.h>
#include <elm/render/LineRenderer.h>
#include <elm/render/MapRenderer.h>

#include <memory>

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
  path::Pathfinder* pathfinder;
  std::vector<Vector2f> path;
  float ship_radius;

  std::unique_ptr<Map> map;

  Vector2f path_start;
  Vector2f path_end;

  Elm(GLFWwindow* window, const Vector2f& surface_dim)
      : window(window), camera(surface_dim, Vector2f(512, 512), 1.0f / 16.0f) {}

  bool Initialize(const char* map_filename, std::unique_ptr<Map> map);
  void Render(bool clear_lines = true);
};

}  // namespace elm
