#pragma once

#include <elm/Map.h>
#include <elm/path/Node.h>

#include <unordered_map>
#include <vector>

namespace elm {
namespace path {

constexpr std::size_t kMaxNodes = 1024 * 1024;

struct EdgeSet {
  u8 set = 0;

  inline bool IsSet(size_t index) const { return set & (1 << index); }
  void Set(size_t index) { set |= (1 << index); }
  void Erase(size_t index) { set &= ~(1 << index); }
};

// All of the coords and indexes stored in this must stay in the same order.
struct CoordOffset {
  s16 x;
  s16 y;

  CoordOffset() : x(0), y(0) {}
  CoordOffset(s16 x, s16 y) : x(x), y(y) {}

  static inline CoordOffset West() { return CoordOffset(-1, 0); }
  static inline CoordOffset East() { return CoordOffset(1, 0); }
  static inline CoordOffset North() { return CoordOffset(0, -1); }
  static inline CoordOffset NorthWest() { return CoordOffset(-1, -1); }
  static inline CoordOffset NorthEast() { return CoordOffset(1, -1); }
  static inline CoordOffset South() { return CoordOffset(0, 1); }
  static inline CoordOffset SouthWest() { return CoordOffset(-1, 1); }
  static inline CoordOffset SouthEast() { return CoordOffset(1, 1); }

  static inline CoordOffset FromIndex(size_t index) {
    static const CoordOffset kNeighbors[8] = {
        CoordOffset::North(),     CoordOffset::South(),     CoordOffset::West(),      CoordOffset::East(),
        CoordOffset::NorthWest(), CoordOffset::NorthEast(), CoordOffset::SouthWest(), CoordOffset::SouthEast()};

    return kNeighbors[index];
  }

  static inline size_t NorthIndex() { return 0; }
  static inline size_t SouthIndex() { return 1; }
  static inline size_t WestIndex() { return 2; }
  static inline size_t EastIndex() { return 3; }
  static inline size_t NorthWestIndex() { return 4; }
  static inline size_t NorthEastIndex() { return 5; }
  static inline size_t SouthWestIndex() { return 6; }
  static inline size_t SouthEastIndex() { return 7; }
};

// Determines the node edges when using A*.
class NodeProcessor {
 public:
  NodeProcessor(const Map& map) : map_(map) {
    nodes_.resize(kMaxNodes);
    memset(&nodes_[0], 0, kMaxNodes * sizeof(Node));

    edges_.resize(kMaxNodes);
    memset(&edges_[0], 0, kMaxNodes * sizeof(EdgeSet));
  }

  EdgeSet FindEdges(Node* node, float radius);
  EdgeSet CalculateEdges(Node* node, float radius);
  Node* GetNode(NodePoint point);
  bool IsSolid(u16 x, u16 y) { return map_.IsSolid(x, y); }

  void SetEdgeSet(u16 x, u16 y, EdgeSet set) {
    size_t index = (size_t)y * 1024 + x;
    edges_[index] = set;
  }

  // Calculate the node from the index.
  // This lets the node exist without storing its position so it fits in cache better.
  inline NodePoint GetPoint(const Node* node) const {
    size_t index = (node - &nodes_[0]);

    uint16_t world_y = (uint16_t)(index / 1024);
    uint16_t world_x = (uint16_t)(index % 1024);

    return NodePoint(world_x, world_y);
  }

  const Map& map_;

 private:
  std::vector<EdgeSet> edges_;
  std::vector<Node> nodes_;
};

}  // namespace path
}  // namespace elm