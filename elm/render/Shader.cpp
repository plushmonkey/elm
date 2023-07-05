#include "Shader.h"

#include <iostream>

namespace elm {
namespace render {

bool ShaderProgram::CreateShader(GLenum type, const char* source, GLuint* shaderOut) {
  GLuint shader = glCreateShader(type);

  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  GLint success;
  GLchar info_log[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, sizeof(info_log), nullptr, info_log);
    std::cout << "Shader error: " << std::endl << info_log << std::endl;
    return false;
  }

  *shaderOut = shader;
  return true;
}

bool ShaderProgram::CreateProgram(GLuint vertexShader, GLuint fragmentShader, GLuint* programOut) {
  GLuint program = glCreateProgram();

  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  GLint success;
  GLchar info_log[512];
  glGetProgramiv(program, GL_LINK_STATUS, &success);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  if (!success) {
    glGetProgramInfoLog(program, sizeof(info_log), nullptr, info_log);
    std::cout << "Link error: " << std::endl << info_log << std::endl;
    return false;
  }

  *programOut = program;
  return true;
}

}  // namespace render
}  // namespace elm
