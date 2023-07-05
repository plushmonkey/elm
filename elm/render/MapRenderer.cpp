#include "MapRenderer.h"

#include <elm/Map.h>
#include <elm/Math.h>
#include <elm/render/Camera.h>
#include <stdio.h>

//
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace elm {
namespace render {

const char* kVertexShaderCode = R"(
#version 330

layout (location = 0) in vec2 position;

uniform mat4 mvp;

out vec2 varying_position;

void main() {
  gl_Position = mvp * vec4(position, 0.0, 1.0);
  varying_position = position;
}
)";

const char* kFragmentShaderCode = R"(
#version 330

in vec2 varying_position;

// This tilemap needs to be 3d for clamping to edge of tiles. There's some bleeding if it's 2d
uniform sampler2DArray tilemap;
// TODO: This could be packed instead of uint for values that can only be 0-255
uniform usampler2D tiledata;

out vec4 color;

void main() {
  uint tile_id;

  float x = varying_position.x;
  float y = varying_position.y;

  if (x < 0.0 || y < 0.0 || x > 1024.0 || y > 1024.0) {
    tile_id = 20u;
  } else {
    ivec2 fetchp = ivec2(varying_position);
    tile_id = texelFetch(tiledata, fetchp, 0).r;
  }

  if (tile_id == 0u || tile_id == 170u || tile_id == 172u || tile_id > 190u || (tile_id >= 162u && tile_id <= 169u)) {
    discard;
  }

  // Calculate uv by getting fraction of traversed tile
  vec2 uv = (varying_position - floor(varying_position));

  color = texture(tilemap, vec3(uv, tile_id - 1u));
  if (tile_id > 172u && color.r == 0.0 && color.g == 0.0 && color.b == 0.0) {
    discard;
  }
}
)";

bool MapRenderer::Initialize(const char* filename, const Map& map) {
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  if (!shader.Initialize(kVertexShaderCode, kFragmentShaderCode)) {
    printf("Failed to load shaders.\n");
    return false;
  }

  struct Vertex {
    Vector2f position;
  };

  Vertex vertices[6];

  vertices[0].position = Vector2f(0, 0);
  vertices[1].position = Vector2f(0, 1024);
  vertices[2].position = Vector2f(1024, 0);

  vertices[3].position = Vector2f(1024, 0);
  vertices[4].position = Vector2f(0, 1024);
  vertices[5].position = Vector2f(1024, 1024);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
  glEnableVertexAttribArray(0);

  glGenTextures(1, &tiledata_texture);
  glBindTexture(GL_TEXTURE_2D, tiledata_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

  int* tiledata = new int[1024 * 1024];
  memset(tiledata, 0, sizeof(int) * 1024 * 1024);

  for (u16 y = 0; y < 1024; ++y) {
    for (u16 x = 0; x < 1024; ++x) {
      tiledata[y * 1024 + x] = map.GetTileId(x, y);
    }
  }

  OGLErrorCheck();
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, 1024, 1024, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, tiledata);
  OGLErrorCheck();

  glGenTextures(1, &tilemap_texture);
  glBindTexture(GL_TEXTURE_2D_ARRAY, tilemap_texture);

  FILE* f = fopen(filename, "rb");
  int x, y, comp;

  u32* image = (u32*)stbi_load_from_file(f, &x, &y, &comp, STBI_rgb_alpha);

  // Create a 3d texture to prevent uv bleed
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 16, 16, 190, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

  for (int tile_y = 0; tile_y < 10; ++tile_y) {
    for (int tile_x = 0; tile_x < 19; ++tile_x) {
      int tile_id = tile_y * 19 + tile_x;
      u32 data[16 * 16];

      int base_y = tile_y * 16 * 16 * 19;
      int base_x = tile_x * 16;

      for (int copy_y = 0; copy_y < 16; ++copy_y) {
        for (int copy_x = 0; copy_x < 16; ++copy_x) {
          u32 tilemap_index = base_y + base_x + copy_y * 16 * 19 + copy_x;

          data[copy_y * 16 + copy_x] = image[tilemap_index];
        }
      }

      glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, tile_id, 16, 16, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
  }

  stbi_image_free(image);

  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 4);

  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  fflush(stdout);

  tiledata_uniform = glGetUniformLocation(shader.program, "tiledata");
  mvp_uniform = glGetUniformLocation(shader.program, "mvp");
  tilemap_uniform = glGetUniformLocation(shader.program, "tilemap");

  glUseProgram(shader.program);

  glUniform1i(tilemap_uniform, 0);
  glUniform1i(tiledata_uniform, 1);

  return true;
}

MapRenderer::~MapRenderer() {
  if (vbo != -1) {
    glDeleteBuffers(1, &vbo);
  }
  if (vao != -1) {
    glDeleteVertexArrays(1, &vao);
  }
}

void MapRenderer::Render(Camera& camera) {
  glUseProgram(shader.program);

  glBindVertexArray(vao);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, tilemap_texture);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, tiledata_texture);

  mat4 proj = camera.GetProjection();
  mat4 view = camera.GetView();
  mat4 model = mat4::Identity();

  mat4 mvp = proj * view * model;

  glUniformMatrix4fv(mvp_uniform, 1, GL_FALSE, (const GLfloat*)mvp.data);

  glDrawArrays(GL_TRIANGLES, 0, 6);
}

}  // namespace render
}  // namespace elm
