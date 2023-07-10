#include "Elm.h"

#include <GLFW/glfw3.h>
#include <elm/Timer.h>

namespace elm {

void OnMouseScroll(GLFWwindow* window, double dx, double dy) {
  Elm* elm = (Elm*)glfwGetWindowUserPointer(window);

  float old_scale = elm->camera.scale;
  float scale = elm->camera.scale;
  float speed = 1.0f / 10.0f;

  scale = scale - (scale * ((float)dy * speed));

  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);

  Vector2f pre_world_pos = elm->camera.Unproject(Vector2f((float)xpos, (float)ypos));
  Vector2f offset = (pre_world_pos - elm->camera.position) * (1.0f / old_scale);

  elm->camera.SetScale(scale);

  elm->camera.position += offset * (old_scale - scale);
}

void OnMouseButton(GLFWwindow* window, int button, int action, int modifier) {
  Elm* elm = (Elm*)glfwGetWindowUserPointer(window);

  double xpos_d, ypos_d;

  glfwGetCursorPos(window, &xpos_d, &ypos_d);

  float xpos = (float)xpos_d;
  float ypos = (float)ypos_d;

  if (button == GLFW_MOUSE_BUTTON_1) {
    if (action == GLFW_PRESS) {
      elm->drag.active = true;
      elm->drag.start = Vector2f(xpos, ypos);
    } else if (action == GLFW_RELEASE) {
      if (elm->drag.active) {
        elm->drag.active = false;
      }
    }
  } else if (button == GLFW_MOUSE_BUTTON_2 && action == GLFW_PRESS) {
    Vector2f world_pos = elm->camera.Unproject(Vector2f(xpos, ypos));

    if (modifier & GLFW_MOD_SHIFT) {
      elm->path_end = world_pos;
    } else {
      elm->path_start = world_pos;
    }

    Timer timer;
    elm->path = elm->pathfinder->FindPath(elm->path_start, elm->path_end, elm->ship_radius);
    printf("Pathfinder::FindPath::Time: %lluus\n", timer.GetElapsedTime());
  }
}

bool Elm::Initialize(const char* map_filename, const Map& map) {
  if (!map_renderer.Initialize(map_filename, map)) {
    return false;
  }

  if (!line_renderer.Initialize()) {
    return false;
  }

  glfwSwapInterval(1);
  glfwSetWindowUserPointer(window, this);
  glfwSetScrollCallback(window, OnMouseScroll);
  glfwSetMouseButtonCallback(window, OnMouseButton);

  OGLErrorCheck();

  return true;
}

inline Vector3f FromRGB(u8 r, u8 g, u8 b) { return Vector3f(r / 255.0f, g / 255.0f, b / 255.0f); }

void Elm::Render(bool clear_lines) {
  if (drag.active) {
    double xpos, ypos;

    glfwGetCursorPos(window, &xpos, &ypos);

    Vector2f current((float)xpos, (float)ypos);

    camera.position -= (current - drag.start) * camera.scale;

    drag.start = current;
  }

  Vector2f path_start_box_tl(floorf(path_start.x), floorf(path_start.y));
  Vector2f path_start_box_br(floorf(path_start.x) + 1.0f, floorf(path_start.y) + 1.0f);

  Vector2f path_end_box_tl(floorf(path_end.x), floorf(path_end.y));
  Vector2f path_end_box_br(floorf(path_end.x) + 1.0f, floorf(path_end.y) + 1.0f);

  line_renderer.PushRect(path_start_box_tl, path_start_box_br, FromRGB(7, 195, 221));
  line_renderer.PushRect(path_end_box_tl, path_end_box_br, FromRGB(251, 237, 20));

  if (!path.empty()) {
    for (size_t i = 0; i < path.size() - 1; ++i) {
      line_renderer.PushLine(path[i], Vector3f(1.0f, 0.0f, 0.0f), path[i + 1], Vector3f(1.0f, 0.0f, 0.0f));
    }
  }

  for (auto&& v : this->pathfinder->debug_diagonals_) {
    line_renderer.PushRect(v, v + Vector2f(1, 1), FromRGB(245, 66, 200));
  }

  map_renderer.Render(camera);
  line_renderer.Render(camera, clear_lines);
}

}  // namespace elm
