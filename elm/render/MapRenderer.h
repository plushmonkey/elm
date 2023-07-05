#pragma once

#include <elm/render/Platform.h>
#include <elm/render/Shader.h>

namespace elm {

class Map;

namespace render {

struct Camera;

struct MapRenderer {
  GLuint vao = -1;
  GLuint vbo = -1;
  GLint mvp_uniform;
  GLuint tilemap_texture;
  GLuint tiledata_texture;
  GLint tilemap_uniform;
  GLint tiledata_uniform;
  ShaderProgram shader;

  bool Initialize(const char* filename, const Map& map);

  ~MapRenderer();
  void Render(Camera& camera);
};

}  // namespace render
}  // namespace elm
