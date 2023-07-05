#include "RayCaster.h"

#include <elm/Map.h>

#include <algorithm>

namespace elm {

CastResult RayCast(const Map& map, Vector2f from, Vector2f direction, float max_length) {
  Vector2f vMapSize = {1024.0f, 1024.0f};

  CastResult result = {};

  if (max_length <= 0.0f) return result;

  float xStepSize = std::sqrt(1 + (direction.y / direction.x) * (direction.y / direction.x));
  float yStepSize = std::sqrt(1 + (direction.x / direction.y) * (direction.x / direction.y));

  Vector2f vMapCheck = Vector2f(std::floor(from.x), std::floor(from.y));
  Vector2f vRayLength1D;

  Vector2f vStep;

  if (direction.x < 0) {
    vStep.x = -1.0f;
    vRayLength1D.x = (from.x - float(vMapCheck.x)) * xStepSize;
  } else {
    vStep.x = 1.0f;
    vRayLength1D.x = (float(vMapCheck.x + 1) - from.x) * xStepSize;
  }

  if (direction.y < 0) {
    vStep.y = -1.0f;
    vRayLength1D.y = (from.y - float(vMapCheck.y)) * yStepSize;
  } else {
    vStep.y = 1.0f;
    vRayLength1D.y = (float(vMapCheck.y + 1) - from.y) * yStepSize;
  }

  // Perform "Walk" until collision or range check
  bool bTileFound = false;
  float fDistance = 0.0f;

  while (!bTileFound && fDistance < max_length) {
    // Walk along shortest path
    if (vRayLength1D.x < vRayLength1D.y) {
      vMapCheck.x += vStep.x;
      fDistance = vRayLength1D.x;
      vRayLength1D.x += xStepSize;
    } else {
      vMapCheck.y += vStep.y;
      fDistance = vRayLength1D.y;
      vRayLength1D.y += yStepSize;
    }

    // Test tile at new test point
    if (vMapCheck.x >= 0 && vMapCheck.x < vMapSize.x && vMapCheck.y >= 0 && vMapCheck.y < vMapSize.y) {
      if (map.IsSolid((unsigned short)vMapCheck.x, (unsigned short)vMapCheck.y)) {
        bTileFound = true;
      }
    }
  }

  // Calculate intersection location
  Vector2f vIntersection;
  if (bTileFound) {
    vIntersection = from + direction * fDistance;
    float dist;

    result.hit = true;
    result.distance = fDistance;
    result.position = vIntersection;

    // wall tiles are checked and returns distance + norm
    RayBoxIntersect(from, direction, vMapCheck, Vector2f(1.0f, 1.0f), &dist, &result.normal);
  }
  return result;
#if 0
  static const Vector2f kDirections[] = { Vector2f(0, 0), Vector2f(1, 0), Vector2f(-1, 0), Vector2f(0, 1), Vector2f(0, -1) };

  CastResult result = { 0 };
  float closest_distance = std::numeric_limits<float>::max();
  Vector2f closest_normal;

  result.distance = max_length;

  Vector2f from_base(std::floor(from.x), std::floor(from.y));

  for (float i = 0; i < max_length + 1.0f; ++i) {
    Vector2f current = from_base + direction * i;

    for (Vector2f check_direction : kDirections) {
      Vector2f check = current + check_direction;

      if (!map.IsSolid((unsigned short)check.x, (unsigned short)check.y)) continue;

      float dist;
      Vector2f normal;

      if (RayBoxIntersect(from, direction, check, Vector2f(1, 1), &dist, &normal)) {
        if (dist < closest_distance && dist >= 0) {
          closest_distance = dist;
          closest_normal = normal;
        }
      }
    }

    if (closest_distance < max_length) {
      result.hit = true;
      result.normal = closest_normal;
      result.distance = closest_distance;
      result.position = from + direction * closest_distance;
    }
  }

  return result;
#endif
}

}  // namespace elm
