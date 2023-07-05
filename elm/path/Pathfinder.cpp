#include "Pathfinder.h"

#include <elm/RayCaster.h>
#include <immintrin.h>

#include <algorithm>
#include <bitset>
#include <cmath>

namespace elm {
namespace path {

inline NodePoint ToNodePoint(const Vector2f v) {
  NodePoint np;

  np.x = (uint16_t)v.x;
  np.y = (uint16_t)v.y;

  return np;
}

inline float Euclidean(NodeProcessor& processor, const Node* __restrict from, const Node* __restrict to) {
  NodePoint from_p = processor.GetPoint(from);
  NodePoint to_p = processor.GetPoint(to);

  float dx = static_cast<float>(from_p.x - to_p.x);
  float dy = static_cast<float>(from_p.y - to_p.y);

  return sqrt(dx * dx + dy * dy);
}

inline float Euclidean(const NodePoint& __restrict from_p, const NodePoint& __restrict to_p) {
  float dx = static_cast<float>(from_p.x - to_p.x);
  float dy = static_cast<float>(from_p.y - to_p.y);

  __m128 mult = _mm_set_ss(dx * dx + dy * dy);
  __m128 result = _mm_sqrt_ss(mult);

  return _mm_cvtss_f32(result);
}

Pathfinder::Pathfinder(std::unique_ptr<NodeProcessor> processor) : processor_(std::move(processor)) {}

std::vector<Vector2f> Pathfinder::FindPath(const Vector2f& from, const Vector2f& to, float ship_radius) {
  std::vector<Vector2f> path;

  Node* start = processor_->GetNode(ToNodePoint(from));
  Node* goal = processor_->GetNode(ToNodePoint(to));

  if (start == nullptr || goal == nullptr) {
    return path;
  }

  if (!(start->flags & NodeFlag_Traversable)) return path;
  if (!(goal->flags & NodeFlag_Traversable)) return path;

  NodePoint start_p = processor_->GetPoint(start);
  NodePoint goal_p = processor_->GetPoint(goal);

  // clear vector then add start node
  openset_.Clear();
  openset_.Push(start);

  // at the start there is only one node here, the start node
  while (!openset_.Empty()) {
    Node* node = openset_.Pop();

    touched_.push_back(node);

    if (node == goal) {
      break;
    }

    node->flags |= NodeFlag_Closed;

    if (node->f > 0 && node->f == node->f_last) {
      // This node was re-added to the openset because its fitness was better, so skip reprocessing the same node.
      // This reduces pathing time by about 20%.
      continue;
    }
    node->f_last = node->f;

    NodePoint node_point = processor_->GetPoint(node);

    // returns neighbor nodes that are not solid
    EdgeSet edges = processor_->FindEdges(node, ship_radius);

    for (size_t i = 0; i < 8; ++i) {
      if (!edges.IsSet(i)) continue;

      CoordOffset offset = CoordOffset::FromIndex(i);

      NodePoint edge_point(node_point.x + offset.x, node_point.y + offset.y);
      Node* edge = processor_->GetNode(edge_point);

      touched_.push_back(edge);

      // The cost to this neighbor is the cost to the current node plus the edge weight times the distance between the
      // nodes.
      float cost = node->g + edge->weight * Euclidean(node_point, edge_point);

      // If the new cost is lower than the previously closed cost then remove it from the closed set.
      if ((edge->flags & NodeFlag_Closed) && cost < edge->g) {
        edge->flags &= ~NodeFlag_Closed;
      }

      // Compute a heuristic from this neighbor to the end goal.
      float h = Euclidean(edge_point, goal_p);

      // If this neighbor hasn't been considered or is better than its original fitness test, then add it back to the
      // open set.
      if (!(edge->flags & NodeFlag_Openset) || cost + h < edge->f) {
        edge->g = cost;
        edge->f = edge->g + h;
        edge->parent = node;
        edge->flags |= NodeFlag_Openset;

        openset_.Push(edge);
      }
    }
  }

  // Construct path backwards from goal node
  std::vector<NodePoint> points;
  Node* current = goal;

  while (current != nullptr && current != start) {
    NodePoint p = processor_->GetPoint(current);
    points.push_back(p);
    current = current->parent;
  }

  path.reserve(points.size() + 1);

  if (goal->parent) {
    path.push_back(Vector2f(start_p.x + 0.5f, start_p.y + 0.5f));
  }

  // Reverse and store as vector
  for (std::size_t i = 0; i < points.size(); ++i) {
    std::size_t index = points.size() - i - 1;
    Vector2f pos(points[index].x, points[index].y);

    pos = processor_->map_.GetOccupyCenter(pos, ship_radius);

    path.push_back(pos);
  }

  for (Node* node : touched_) {
    node->flags &= ~NodeFlag_Initialized;
  }

  touched_.clear();

  return path;
}

float GetWallDistance(const Map& map, u16 x, u16 y, u16 radius) {
  float closest_sq = std::numeric_limits<float>::max();

  for (i16 offset_y = -radius; offset_y < radius; ++offset_y) {
    for (i16 offset_x = -radius; offset_x < radius; ++offset_x) {
      u16 check_x = x + offset_x;
      u16 check_y = y + offset_y;

      if (map.IsSolid(check_x, check_y)) {
        float dist_sq = (float)(offset_x * offset_x + offset_y * offset_y);

        if (dist_sq < closest_sq) {
          closest_sq = dist_sq;
        }
      }
    }
  }

  return std::sqrt(closest_sq);
}

void Pathfinder::CreateMapWeights(const Map& map, float ship_radius, bool linear_weights) {
  // Calculate which nodes are traversable before creating edges.
  for (u16 y = 0; y < 1024; ++y) {
    for (u16 x = 0; x < 1024; ++x) {
      if (map.IsSolid(x, y)) continue;

      Node* node = processor_->GetNode(NodePoint(x, y));

      if (map.CanOverlapTile(Vector2f(x, y), ship_radius)) {
        node->flags |= NodeFlag_Traversable;
      }
    }
  }

  for (u16 y = 0; y < 1024; ++y) {
    for (u16 x = 0; x < 1024; ++x) {
      if (map.IsSolid(x, y)) continue;
      Node* node = processor_->GetNode(NodePoint(x, y));
      EdgeSet edges = processor_->CalculateEdges(node, ship_radius);

      processor_->SetEdgeSet(x, y, edges);

      node->weight = 1.0f;

      if (linear_weights) {
        int close_distance = 5;
        float distance = GetWallDistance(map, x, y, close_distance);

        if (distance < 1) distance = 1;

        if (distance < close_distance) {
          node->weight = close_distance / distance;
        }
      }
    }
  }
}

}  // namespace path
}  // namespace elm
