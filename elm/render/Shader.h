#pragma once

#include <elm/render/Platform.h>

namespace elm {
namespace render {

struct ShaderProgram {
  bool Initialize(const char* vertex_code, const char* fragment_code) {
    GLuint vertex_shader, fragment_shader;

    if (!CreateShader(GL_VERTEX_SHADER, vertex_code, &vertex_shader)) {
      return false;
    }

    if (!CreateShader(GL_FRAGMENT_SHADER, fragment_code, &fragment_shader)) {
      glDeleteShader(vertex_shader);
      return false;
    }

    return CreateProgram(vertex_shader, fragment_shader, &program);
  }

  GLuint program;

 private:
  bool CreateShader(GLenum type, const char* source, GLuint* shaderOut);
  bool CreateProgram(GLuint vertexShader, GLuint fragmentShader, GLuint* programOut);
};

}  // namespace render
}  // namespace elm
