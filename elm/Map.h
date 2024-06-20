#pragma once

#include <elm/Math.h>
#include <elm/RegionRegistry.h>
#include <elm/Types.h>

#include <bitset>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace elm {

constexpr std::size_t kMapExtent = 1024;

using TileId = u8;
using TileData = std::vector<TileId>;

constexpr TileId kSafeTileId = 171;

struct OccupyRect {
  bool occupy;

  u16 start_x;
  u16 start_y;
  u16 end_x;
  u16 end_y;

  bool operator==(const OccupyRect& other) const {
    return occupy == other.occupy && start_x == other.start_x && start_y == other.start_y && end_x == other.end_x &&
           end_y == other.end_y;
  }
};

// Reduced version of OccupyRect to remove bool so it fits more in a cache line
struct OccupiedRect {
  u16 start_x;
  u16 start_y;
  u16 end_x;
  u16 end_y;

  bool operator==(const OccupiedRect& other) const {
    return start_x == other.start_x && start_y == other.start_y && end_x == other.end_x && end_y == other.end_y;
  }

  inline bool Contains(Vector2f position) const {
    u16 x = (u16)position.x;
    u16 y = (u16)position.y;

    return x >= start_x && x <= end_x && y >= start_y && y <= end_y;
  }
};

namespace elvl {

enum {
  RegionFlag_Base = (1 << 0),
  RegionFlag_NoAntiwarp = (1 << 1),
  RegionFlag_NoWeapons = (1 << 2),
  RegionFlag_NoFlags = (1 << 3),
};
using RegionFlags = u32;

struct Region {
  std::string name;
  RegionFlags flags = 0;

  // If a tile is part of this region then it will be set in this bitset.
  std::bitset<1024 * 1024> tiles;

  inline void SetTile(u16 x, u16 y) { tiles.set(y * 1024 + x); }

  inline bool InRegion(u16 x, u16 y) const {
    if (x > 1023 || y > 1023) return false;
    return tiles.test(y * 1024 + x);
  }
};

}  // namespace elvl

class Map {
 public:
  Map(const TileData& tile_data);

  bool IsSolid(TileId id) const;
  bool IsSolid(u16 x, u16 y) const;
  bool IsSolid(const Vector2f& position) const;
  TileId GetTileId(u16 x, u16 y) const;
  TileId GetTileId(const Vector2f& position) const;

  bool CanOccupy(u16 x, u16 y, float radius) const;
  bool CanOccupy(const Vector2f& position, float radius) const;

  // Checks if the ship can go from one tile directly to another.
  bool CanTraverse(const Vector2f& start, const Vector2f& end, float radius) const;
  bool CanOverlapTile(const Vector2f& position, float radius) const;
  // Returns a possible rect that creates an occupiable area that contains the tested position.
  OccupyRect GetPossibleOccupyRect(const Vector2f& position, float radius) const;

  bool CanMoveTo(MapCoord from, MapCoord to, float radius) const;

  // Rects must be initialized memory that can contain all possible occupy rects.
  size_t GetAllOccupiedRects(Vector2f position, float radius, OccupiedRect* rects) const;

  Vector2f GetOccupyCenter(const Vector2f& position, float radius) const;

  OccupyRect GetClosestOccupyRect(Vector2f position, float radius, Vector2f point) const;

  std::vector<const elvl::Region*> GetRegions(Vector2f position) const;
  std::vector<const elvl::Region*> GetRegions(u16 x, u16 y) const;

  bool InRegion(std::string name, Vector2f position) const;
  bool InRegion(std::string name, u16 x, u16 y) const;

  static std::unique_ptr<Map> Load(const char* filename);
  static std::unique_ptr<Map> Load(const std::string& filename);

 private:
  TileData tile_data_;
  std::vector<elvl::Region> regions;

  std::unordered_map<std::string, elvl::Region*> region_map;

  void ParseRegions(const char* file_data);
};

}  // namespace elm
