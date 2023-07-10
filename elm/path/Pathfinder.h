#pragma once

#include <elm/Map.h>
#include <elm/Math.h>
#include <elm/path/NodeProcessor.h>

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <vector>

namespace elm {
namespace path {

template <typename T, typename Compare, typename Container = std::vector<T>>
class PriorityQueue {
 public:
  using const_iterator = typename Container::const_iterator;

  const_iterator begin() const { return container_.cbegin(); }
  const_iterator end() const { return container_.cend(); }

  void Push(T item) {
    container_.push_back(item);
    std::push_heap(container_.begin(), container_.end(), comparator_);
  }

  T Pop() {
    T item = container_.front();
    std::pop_heap(container_.begin(), container_.end(), comparator_);
    container_.pop_back();
    return item;
  }

  void Update() { std::make_heap(container_.begin(), container_.end(), comparator_); }

  void Clear() { container_.clear(); }
  std::size_t Size() const { return container_.size(); }
  bool Empty() const { return container_.empty(); }

 private:
  Container container_;
  Compare comparator_;
};

struct Pathfinder {
 public:
  Pathfinder(std::unique_ptr<NodeProcessor> processor);
  std::vector<Vector2f> FindPath(const Vector2f& from, const Vector2f& to, float ship_radius);

  void CreateMapWeights(const Map& map, float ship_radius, bool linear_weights);

  struct NodeCompare {
    bool operator()(const Node* lhs, const Node* rhs) const { return lhs->f > rhs->f; }
  };

  std::unique_ptr<NodeProcessor> processor_;
  PriorityQueue<Node*, NodeCompare> openset_;
  std::vector<Node*> touched_;
  std::vector<Vector2f> debug_diagonals_;
};

}  // namespace path
}  // namespace elm
