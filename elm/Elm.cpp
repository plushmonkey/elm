#include "Elm.h"

#include <GLFW/glfw3.h>

namespace elm {

void OnMouseScroll(GLFWwindow* window, double dx, double dy) {
  Elm* elm = (Elm*)glfwGetWindowUserPointer(window);

  float scale = elm->camera.scale;
  float speed = 1.0f / 10.0f;

  scale = scale - (scale * ((float)dy * speed));

  elm->camera.SetScale(scale);
}

void OnMouseButton(GLFWwindow* window, int button, int action, int modifier) {
  Elm* elm = (Elm*)glfwGetWindowUserPointer(window);

  double xpos, ypos;

  if (button == GLFW_MOUSE_BUTTON_1) {
    if (action == GLFW_PRESS) {
      elm->drag.active = true;

      glfwGetCursorPos(window, &xpos, &ypos);

      elm->drag.start = Vector2f((float)xpos, (float)ypos);
    } else if (action == GLFW_RELEASE) {
      if (elm->drag.active) {
        elm->drag.active = false;
      }
    }
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

void Elm::Render(bool clear_lines) {
  if (drag.active) {
    double xpos, ypos;

    glfwGetCursorPos(window, &xpos, &ypos);

    Vector2f current((float)xpos, (float)ypos);

    camera.position -= (current - drag.start) * camera.scale;

    drag.start = current;
  }

  map_renderer.Render(camera);
  line_renderer.Render(camera, clear_lines);
}

}  // namespace elm
