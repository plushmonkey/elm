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

  return edges;
}

EdgeSet NodeProcessor::CalculateEdges(Node* node, float radius) {
  EdgeSet edges = {};

  NodePoint base_point = GetPoint(node);
  MapCoord base(base_point.x, base_point.y);

  bool north = false;
  bool south = false;

  bool* requirements[8] = {nullptr, nullptr, nullptr, nullptr, &north, &north, &south, &south};
  CoordOffset neighbors[8] = {CoordOffset::North(),     CoordOffset::South(),     CoordOffset::West(),
                              CoordOffset::East(),      CoordOffset::NorthWest(), CoordOffset::NorthEast(),
                              CoordOffset::SouthWest(), CoordOffset::SouthEast()};

  for (std::size_t i = 0; i < 8; i++) {
    bool* requirement = requirements[i];

    if (requirement && !*requirement) continue;

    uint16_t world_x = base_point.x + neighbors[i].x;
    uint16_t world_y = base_point.y + neighbors[i].y;
    MapCoord pos(world_x, world_y);

    if (!map_.CanOccupy(Vector2f(world_x, world_y), radius)) {
      if (!map_.CanMoveTo(base, pos, radius)) {
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

    if (i == 0) {
      north = true;
    } else if (i == 1) {
      south = true;
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
    node->g = node->f = 0.0f;
    node->flags = NodeFlag_Initialized | (node->flags & NodeFlag_Traversable);
    node->parent = nullptr;
    node->weight = 1.0f;
  }

  return &nodes_[index];
}

}  // namespace path
}  // namespace elm
