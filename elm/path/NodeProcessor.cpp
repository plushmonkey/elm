#include "NodeProcessor.h"

#include <elm/RayCaster.h>

namespace elm {
namespace path {

//

EdgeSet NodeProcessor::FindEdges(Node* node, float radius) {
  NodePoint point = GetPoint(node);
  size_t index = (size_t)point.y * 1024 + point.x;
  EdgeSet edges = this->edges_[index];

  // Any extra checks can be done here to remove dynamic edges.
  // if (west bad) edges.Erase(CoordOffset::WestIndex());

#if 1
  if (node->parent) {
    // Don't cycle back to parent. This saves a very small amount of time because that node would be ignored anyway.
    NodePoint parent_point = GetPoint(node->parent);
    CoordOffset offset(parent_point.x - point.x, parent_point.y - point.y);

    edges.Erase(offset.GetIndex());
  }
#endif

  return edges;
}

inline bool CanOccupy(const Map& map, OccupiedRect& rect, Vector2f offset) {
  Vector2f min = Vector2f(rect.start_x, rect.start_y) + offset;
  Vector2f max = Vector2f(rect.end_x, rect.end_y) + offset;

  for (u16 y = (u16)min.y; y <= (u16)max.y; ++y) {
    for (u16 x = (u16)min.x; x <= (u16)max.x; ++x) {
      if (map.IsSolid(x, y)) {
        return false;
      }
    }
  }

  return true;
}

inline bool CanOccupyAxis(const Map& map, OccupiedRect& rect, Vector2f offset) {
  Vector2f min = Vector2f(rect.start_x, rect.start_y) + offset;
  Vector2f max = Vector2f(rect.end_x, rect.end_y) + offset;

  if (offset.x < 0) {
    // Moving west, so check western section of rect
    for (u16 y = (u16)min.y; y <= (u16)max.y; ++y) {
      if (map.IsSolid((u16)min.x, y)) {
        return false;
      }
    }
  } else if (offset.x > 0) {
    // Moving east, so check eastern section of rect
    for (u16 y = (u16)min.y; y <= (u16)max.y; ++y) {
      if (map.IsSolid((u16)max.x, y)) {
        return false;
      }
    }
  } else if (offset.y < 0) {
    // Moving north, so check north section of rect
    for (u16 x = (u16)min.x; x <= (u16)max.x; ++x) {
      if (map.IsSolid(x, (u16)min.y)) {
        return false;
      }
    }
  } else if (offset.y > 0) {
    // Moving south, so check south section of rect
    for (u16 x = (u16)min.x; x <= (u16)max.x; ++x) {
      if (map.IsSolid(x, (u16)max.y)) {
        return false;
      }
    }
  }

  return true;
}

EdgeSet NodeProcessor::CalculateEdges(Node* node, float radius) {
  EdgeSet edges = {};

  NodePoint base_point = GetPoint(node);
  MapCoord base(base_point.x, base_point.y);

  bool north = false;
  bool south = false;

  bool* setters[8] = {&north, &south, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
  bool* requirements[8] = {nullptr, nullptr, nullptr, nullptr, &north, &north, &south, &south};
  static const CoordOffset neighbors[8] = {CoordOffset::North(),     CoordOffset::South(),     CoordOffset::West(),
                                           CoordOffset::East(),      CoordOffset::NorthWest(), CoordOffset::NorthEast(),
                                           CoordOffset::SouthWest(), CoordOffset::SouthEast()};

  OccupiedRect occupied[64];
  size_t occupied_count = map_.GetAllOccupiedRects(Vector2f(base.x, base.y), radius, occupied);

  for (std::size_t i = 0; i < 8; i++) {
    bool* requirement = requirements[i];

    if (requirement && !*requirement) continue;

    uint16_t world_x = base_point.x + neighbors[i].x;
    uint16_t world_y = base_point.y + neighbors[i].y;
    MapCoord pos(world_x, world_y);

    bool is_occupied = false;
    // Check each occupied rect to see if contains the target position.
    // The expensive check can be skipped because this spot is definitely occupiable.
    for (size_t j = 0; j < occupied_count; ++j) {
      OccupiedRect& rect = occupied[j];

      if (rect.Contains(Vector2f(pos.x, pos.y))) {
        is_occupied = true;
        break;
      }
    }

    if (!is_occupied) {
      bool can_occupy = true;
      Vector2f offset(neighbors[i].x, neighbors[i].y);
      // Check each occupied rect to see if it can move in this direction
      for (size_t j = 0; j < occupied_count; ++j) {
        if (i >= 4) {
          if (!CanOccupy(map_, occupied[j], offset)) {
            can_occupy = false;
            break;
          }
        } else {
          if (!CanOccupyAxis(map_, occupied[j], offset)) {
            can_occupy = false;
            break;
          }
        }
      }

      if (!can_occupy) {
        continue;
      }
    }

    NodePoint current_point(world_x, world_y);
    Node* current = GetNode(current_point);

    if (!current) continue;
    if (!(current->flags & NodeFlag_Traversable)) continue;

    if (map_.GetTileId(current_point.x, current_point.y) == kSafeTileId) {
      current->weight = 10.0f;
    }

    edges.Set(i);

    if (setters[i]) {
      *setters[i] = true;
    }
  }

  return edges;
}

Node* NodeProcessor::GetNode(NodePoint point) {
  if (point.x >= 1024 || point.y >= 1024) {
    return nullptr;
  }

  std::size_t index = point.y * 1024 + point.x;
  Node* node = &nodes_[index];

  if (!(node->flags & NodeFlag_Initialized)) {
    node->g = node->f = node->f_last = 0.0f;
    node->flags = NodeFlag_Initialized | (node->flags & NodeFlag_Traversable);
    node->parent = nullptr;
  }

  return &nodes_[index];
}

}  // namespace path
}  // namespace elm
