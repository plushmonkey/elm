#pragma once

#include <elm/Math.h>

namespace elm {

class Map;

struct CastResult {
  bool hit;
  float distance;
  Vector2f position;
  Vector2f normal;
};

CastResult RayCast(const Map& map, Vector2f from, Vector2f direction, float max_length);

}  // namespace elm
