#include "Map.h"

#include <bitset>
#include <cstring>
#include <fstream>
#include <string_view>

namespace elm {

bool solid_tiles[256];

bool IsSolidSquare(const Map& map, MapCoord top_left, int length) {
  u16 abs_x = top_left.x;
  u16 abs_y = top_left.y;

  for (uint16_t x = 0; x < length; x++) {
    for (uint16_t y = 0; y < length; y++) {
      if (map.IsSolid(abs_x + x, abs_y + y)) {
        return true;
      }
    }
  }

  return false;
}

constexpr size_t kMaxOccupySet = 96;
struct OccupyMap {
  u32 count = 0;
  std::bitset<kMaxOccupySet> set;

  bool operator[](size_t index) const { return set[index]; }
};

// get a list of positions/orientation points that the ship can fit on the selected tile
OccupyMap CalculateOccupyMap(const Map& map, MapCoord start, float radius) {
  OccupyMap result;

  uint16_t diameter = uint16_t(radius + 0.5f) * 2;
  MapCoord offset(start.x - diameter + 1, start.y - diameter + 1);

  for (uint16_t x = 0; x < diameter; x++) {
    for (uint16_t y = 0; y < diameter; y++) {
      MapCoord pos(offset.x + x, offset.y + y);

      result.set[result.count++] = IsSolidSquare(map, pos, diameter);
    }
  }

  return result;
}

bool Map::CanMoveTo(MapCoord from, MapCoord to, float radius) const {
  // MapCoord cardinal = from - to;

  OccupyMap fromMap = CalculateOccupyMap(*this, from, radius);
  OccupyMap toMap = CalculateOccupyMap(*this, to, radius);

  // compare the lists, if the ship was able to complete a 1 tile step in the same direction return true
  for (std::size_t i = 0; i < fromMap.count && i < toMap.count; i++) {
    if (fromMap[i] || toMap[i]) {
      // if (fromMap[i] == MapCoord(0, 0) || toMap[i] == MapCoord(0, 0)) {
      continue;
    } else {
      return true;
    }

    //  if (fromMap[i] - toMap[i] == cardinal) {
    //    return true;
    // }
  }
  return false;
}

Map::Map(const TileData& tile_data) : tile_data_(tile_data) {
  memset(solid_tiles, 1, sizeof(solid_tiles) / sizeof(*solid_tiles));
  solid_tiles[0] = false;

  for (std::size_t i = 162; i <= 191; ++i) {
    solid_tiles[i] = false;
  }

  solid_tiles[241] = false;
  solid_tiles[242] = false;
  solid_tiles[253] = false;
  solid_tiles[254] = false;
  solid_tiles[255] = false;
}

TileId Map::GetTileId(u16 x, u16 y) const {
  if (x >= 1024 || y >= 1024) return 0;
  return tile_data_[y * kMapExtent + x];
}

bool Map::IsSolid(u16 x, u16 y) const {
  if (x >= 1024 || y >= 1024) return true;
  return IsSolid(GetTileId(x, y));
}

bool Map::IsSolid(TileId id) const {
  if (id == 0) return false;
  if (id >= 162 && id <= 169) return false;  // treat doors as non-solid
  if (id < 170) return true;
  if (id >= 192 && id <= 240) return true;
  if (id >= 242 && id <= 252) return true;

  return false;
}

bool Map::IsSolid(const Vector2f& position) const {
  u16 x = static_cast<u16>(std::floor(position.x));
  u16 y = static_cast<u16>(std::floor(position.y));

  return IsSolid(x, y);
}

TileId Map::GetTileId(const Vector2f& position) const {
  u16 x = static_cast<u16>(position.x);
  u16 y = static_cast<u16>(position.y);

  return GetTileId(x, y);
}

bool Map::CanOccupy(u16 position_x, u16 position_y, float radius) const {
  int radius_check = (int)(radius + 0.5f);

  for (int y = -radius_check; y <= radius_check; ++y) {
    for (int x = -radius_check; x <= radius_check; ++x) {
      uint16_t world_x = (uint16_t)(position_x + x);
      uint16_t world_y = (uint16_t)(position_y + y);

      if (IsSolid(world_x, world_y)) {
        return false;
      }
    }
  }

  return true;
}

bool Map::CanTraverse(const Vector2f& start, const Vector2f& end, float radius) const {
  if (!CanOverlapTile(start, radius)) return false;
  if (!CanOverlapTile(end, radius)) return false;

  Vector2f cross = Perpendicular(Normalize(start - end));

  bool left_solid = IsSolid(start + cross);
  bool right_solid = IsSolid(start - cross);

  if (left_solid) {
    for (float i = 0; i < radius * 2.0f; ++i) {
      if (!CanOverlapTile(start - cross * i, radius)) {
        return false;
      }

      if (!CanOverlapTile(end - cross * i, radius)) {
        return false;
      }
    }

    return true;
  }

  if (right_solid) {
    for (float i = 0; i < radius * 2.0f; ++i) {
      if (!CanOverlapTile(start + cross * i, radius)) {
        return false;
      }

      if (!CanOverlapTile(end + cross * i, radius)) {
        return false;
      }
    }

    return true;
  }

  return true;
}

OccupyRect Map::GetClosestOccupyRect(Vector2f position, float radius, Vector2f point) const {
  OccupyRect result = {};

  u16 d = (u16)(radius * 2.0f);
  u16 start_x = (u16)position.x;
  u16 start_y = (u16)position.y;

  u16 far_left = start_x - d;
  u16 far_right = start_x + d;
  u16 far_top = start_y - d;
  u16 far_bottom = start_y + d;

  result.occupy = false;

  // Handle wrapping that can occur from using unsigned short
  if (far_left > 1023) far_left = 0;
  if (far_right > 1023) far_right = 1023;
  if (far_top > 1023) far_top = 0;
  if (far_bottom > 1023) far_bottom = 1023;

  bool solid = IsSolid(start_x, start_y);
  if (d < 1 || solid) {
    result.occupy = !solid;
    result.start_x = (u16)position.x;
    result.start_y = (u16)position.y;
    result.end_x = (u16)position.x;
    result.end_y = (u16)position.y;

    return result;
  }

  float best_distance_sq = 1000.0f;

  // Loop over the entire check region and move in the direction of the check tile.
  // This makes sure that the check tile is always contained within the found region.
  for (u16 check_y = far_top; check_y <= far_bottom; ++check_y) {
    s16 dir_y = (start_y - check_y) > 0 ? 1 : (start_y == check_y ? 0 : -1);

    // Skip cardinal directions because the radius is >1 and must be found from a corner region.
    if (dir_y == 0) continue;

    for (u16 check_x = far_left; check_x <= far_right; ++check_x) {
      s16 dir_x = (start_x - check_x) > 0 ? 1 : (start_x == check_x ? 0 : -1);

      if (dir_x == 0) continue;

      bool can_fit = true;

      for (s16 y = check_y; std::abs(y - check_y) <= d && can_fit; y += dir_y) {
        for (s16 x = check_x; std::abs(x - check_x) <= d; x += dir_x) {
          if (IsSolid(x, y)) {
            can_fit = false;
            break;
          }
        }
      }

      if (can_fit) {
        u16 found_start_x = 0;
        u16 found_start_y = 0;
        u16 found_end_x = 0;
        u16 found_end_y = 0;

        if (check_x > start_x) {
          found_start_x = check_x - d;
          found_end_x = check_x;
        } else {
          found_start_x = check_x;
          found_end_x = check_x + d;
        }

        if (check_y > start_y) {
          found_start_y = check_y - d;
          found_end_y = check_y;
        } else {
          found_start_y = check_y;
          found_end_y = check_y + d;
        }

        bool use_rect = true;

        // Already have an occupied rect, see if this one is better.
        if (result.occupy) {
          Vector2f start(found_start_x, found_start_y);
          Vector2f end(found_end_x, found_end_y);

          Vector2f center = (end + start) * 0.5f;
          float dist_sq = center.DistanceSq(point);

          use_rect = dist_sq < best_distance_sq;
        }

        bool contains_point =
            BoxContainsPoint(Vector2f(found_start_x, found_start_y), Vector2f(found_end_x, found_end_y), point);

        if (contains_point || use_rect) {
          result.start_x = found_start_x;
          result.start_y = found_start_y;
          result.end_x = found_end_x;
          result.end_y = found_end_y;

          result.occupy = true;
        }

        if (contains_point) {
          return result;
        }
      }
    }
  }

  return result;
}

Vector2f Map::GetOccupyCenter(const Vector2f& position, float radius) const {
  u16 d = (u16)(radius * 2.0f);
  u16 start_x = (u16)position.x;
  u16 start_y = (u16)position.y;

  u16 far_left = start_x - d;
  u16 far_right = start_x + d;
  u16 far_top = start_y - d;
  u16 far_bottom = start_y + d;

  // Handle wrapping that can occur from using unsigned short
  if (far_left > 1023) far_left = 0;
  if (far_right > 1023) far_right = 1023;
  if (far_top > 1023) far_top = 0;
  if (far_bottom > 1023) far_bottom = 1023;

  bool solid = IsSolid(start_x, start_y);
  if (d < 1 || solid) {
    return position;
  }

  Vector2f accum;
  size_t count = 0;

  // Loop over the entire check region and move in the direction of the check tile.
  // This makes sure that the check tile is always contained within the found region.
  for (u16 check_y = far_top; check_y <= far_bottom; ++check_y) {
    s16 dir_y = (start_y - check_y) > 0 ? 1 : (start_y == check_y ? 0 : -1);

    // Skip cardinal directions because the radius is >1 and must be found from a corner region.
    if (dir_y == 0) continue;

    for (u16 check_x = far_left; check_x <= far_right; ++check_x) {
      s16 dir_x = (start_x - check_x) > 0 ? 1 : (start_x == check_x ? 0 : -1);

      if (dir_x == 0) continue;

      bool can_fit = true;

      for (s16 y = check_y; std::abs(y - check_y) <= d && can_fit; y += dir_y) {
        for (s16 x = check_x; std::abs(x - check_x) <= d; x += dir_x) {
          if (IsSolid(x, y)) {
            can_fit = false;
            break;
          }
        }
      }

      if (can_fit) {
        // Calculate the final region. Not necessary for simple overlap check, but might be useful
        u16 found_start_x = 0;
        u16 found_start_y = 0;
        u16 found_end_x = 0;
        u16 found_end_y = 0;

        if (check_x > start_x) {
          found_start_x = check_x - d;
          found_end_x = check_x;
        } else {
          found_start_x = check_x;
          found_end_x = check_x + d;
        }

        if (check_y > start_y) {
          found_start_y = check_y - d;
          found_end_y = check_y;
        } else {
          found_start_y = check_y;
          found_end_y = check_y + d;
        }

        Vector2f min(found_start_x, found_start_y);
        Vector2f max((float)found_end_x + 1.0f, (float)found_end_y + 1.0f);

        accum += (min + max) * 0.5f;
        ++count;
      }
    }
  }

  if (count <= 0) return position;

  return accum * (1.0f / count);
}

// Rects must be initialized memory that can contain all possible occupy rects.
size_t Map::GetAllOccupiedRects(Vector2f position, float radius, OccupiedRect* rects) const {
  size_t count = 0;

  u16 d = (u16)(radius * 2.0f);
  u16 start_x = (u16)position.x;
  u16 start_y = (u16)position.y;

  u16 far_left = start_x - d;
  u16 far_right = start_x + d;
  u16 far_top = start_y - d;
  u16 far_bottom = start_y + d;

  // Handle wrapping that can occur from using unsigned short
  if (far_left > 1023) far_left = 0;
  if (far_right > 1023) far_right = 1023;
  if (far_top > 1023) far_top = 0;
  if (far_bottom > 1023) far_bottom = 1023;

  bool solid = IsSolid(start_x, start_y);
  if (d < 1 || solid) {
    rects->start_x = (u16)position.x;
    rects->start_y = (u16)position.y;
    rects->end_x = (u16)position.x;
    rects->end_y = (u16)position.y;

    return !solid;
  }

  // Loop over the entire check region and move in the direction of the check tile.
  // This makes sure that the check tile is always contained within the found region.
  for (u16 check_y = far_top; check_y <= far_bottom; ++check_y) {
    s16 dir_y = (start_y - check_y) > 0 ? 1 : (start_y == check_y ? 0 : -1);

    // Skip cardinal directions because the radius is >1 and must be found from a corner region.
    if (dir_y == 0) continue;

    for (u16 check_x = far_left; check_x <= far_right; ++check_x) {
      s16 dir_x = (start_x - check_x) > 0 ? 1 : (start_x == check_x ? 0 : -1);

      if (dir_x == 0) continue;

      bool can_fit = true;

      for (s16 y = check_y; std::abs(y - check_y) <= d && can_fit; y += dir_y) {
        for (s16 x = check_x; std::abs(x - check_x) <= d; x += dir_x) {
          if (IsSolid(x, y)) {
            can_fit = false;
            break;
          }
        }
      }

      if (can_fit) {
        // Calculate the final region. Not necessary for simple overlap check, but might be useful
        u16 found_start_x = 0;
        u16 found_start_y = 0;
        u16 found_end_x = 0;
        u16 found_end_y = 0;

        if (check_x > start_x) {
          found_start_x = check_x - d;
          found_end_x = check_x;
        } else {
          found_start_x = check_x;
          found_end_x = check_x + d;
        }

        if (check_y > start_y) {
          found_start_y = check_y - d;
          found_end_y = check_y;
        } else {
          found_start_y = check_y;
          found_end_y = check_y + d;
        }

        OccupiedRect* rect = rects + count++;

        rect->start_x = found_start_x;
        rect->start_y = found_start_y;
        rect->end_x = found_end_x;
        rect->end_y = found_end_y;
      }
    }
  }

  return count;
}

OccupyRect Map::GetPossibleOccupyRect(const Vector2f& position, float radius) const {
  OccupyRect result;

  u16 d = (u16)(radius * 2.0f);
  u16 start_x = (u16)position.x;
  u16 start_y = (u16)position.y;

  u16 far_left = start_x - d;
  u16 far_right = start_x + d;
  u16 far_top = start_y - d;
  u16 far_bottom = start_y + d;

  result.occupy = false;

  // Handle wrapping that can occur from using unsigned short
  if (far_left > 1023) far_left = 0;
  if (far_right > 1023) far_right = 1023;
  if (far_top > 1023) far_top = 0;
  if (far_bottom > 1023) far_bottom = 1023;

  bool solid = IsSolid(start_x, start_y);
  if (d < 1 || solid) {
    result.occupy = !solid;
    result.start_x = (u16)position.x;
    result.start_y = (u16)position.y;
    result.end_x = (u16)position.x;
    result.end_y = (u16)position.y;

    return result;
  }

  // Loop over the entire check region and move in the direction of the check tile.
  // This makes sure that the check tile is always contained within the found region.
  for (u16 check_y = far_top; check_y <= far_bottom; ++check_y) {
    s16 dir_y = (start_y - check_y) > 0 ? 1 : (start_y == check_y ? 0 : -1);

    // Skip cardinal directions because the radius is >1 and must be found from a corner region.
    if (dir_y == 0) continue;

    for (u16 check_x = far_left; check_x <= far_right; ++check_x) {
      s16 dir_x = (start_x - check_x) > 0 ? 1 : (start_x == check_x ? 0 : -1);

      if (dir_x == 0) continue;

      bool can_fit = true;

      for (s16 y = check_y; std::abs(y - check_y) <= d && can_fit; y += dir_y) {
        for (s16 x = check_x; std::abs(x - check_x) <= d; x += dir_x) {
          if (IsSolid(x, y)) {
            can_fit = false;
            break;
          }
        }
      }

      if (can_fit) {
        // Calculate the final region. Not necessary for simple overlap check, but might be useful
        u16 found_start_x = 0;
        u16 found_start_y = 0;
        u16 found_end_x = 0;
        u16 found_end_y = 0;

        if (check_x > start_x) {
          found_start_x = check_x - d;
          found_end_x = check_x;
        } else {
          found_start_x = check_x;
          found_end_x = check_x + d;
        }

        if (check_y > start_y) {
          found_start_y = check_y - d;
          found_end_y = check_y;
        } else {
          found_start_y = check_y;
          found_end_y = check_y + d;
        }

        result.start_x = found_start_x;
        result.start_y = found_start_y;
        result.end_x = found_end_x;
        result.end_y = found_end_y;

        result.occupy = true;
        return result;
      }
    }
  }

  return result;
}

bool Map::CanOverlapTile(const Vector2f& position, float radius) const {
  u16 d = (u16)(radius * 2.0f);
  u16 start_x = (u16)position.x;
  u16 start_y = (u16)position.y;

  u16 far_left = start_x - d;
  u16 far_right = start_x + d;
  u16 far_top = start_y - d;
  u16 far_bottom = start_y + d;

  // Handle wrapping that can occur from using unsigned short
  if (far_left > 1023) far_left = 0;
  if (far_right > 1023) far_right = 1023;
  if (far_top > 1023) far_top = 0;
  if (far_bottom > 1023) far_bottom = 1023;

  bool solid = IsSolid(start_x, start_y);
  if (d < 1 || solid) return !solid;

  // Loop over the entire check region and move in the direction of the check tile.
  // This makes sure that the check tile is always contained within the found region.
  for (u16 check_y = far_top; check_y <= far_bottom; ++check_y) {
    s16 dir_y = (start_y - check_y) > 0 ? 1 : (start_y == check_y ? 0 : -1);

    // Skip cardinal directions because the radius is >1 and must be found from a corner region.
    if (dir_y == 0) continue;

    for (u16 check_x = far_left; check_x <= far_right; ++check_x) {
      s16 dir_x = (start_x - check_x) > 0 ? 1 : (start_x == check_x ? 0 : -1);

      if (dir_x == 0) continue;

      bool can_fit = true;

      for (s16 y = check_y; std::abs(y - check_y) <= d && can_fit; y += dir_y) {
        for (s16 x = check_x; std::abs(x - check_x) <= d; x += dir_x) {
          if (IsSolid(x, y)) {
            can_fit = false;
            break;
          }
        }
      }

      if (can_fit) {
#if 0
        // Calculate the final region. Not necessary for simple overlap check, but might be useful
        u16 found_start_x = 0;
        u16 found_start_y = 0;
        u16 found_end_x = 0;
        u16 found_end_y = 0;

        if (check_x > start_x) {
          found_start_x = check_x - d;
          found_end_x = check_x;
        } else {
          found_start_x = check_x;
          found_end_x = check_x + d;
        }

        if (check_y > start_y) {
          found_start_y = check_y - d;
          found_end_y = check_y;
        } else {
          found_start_y = check_y;
          found_end_y = check_y + d;
        }

        printf("Can fit between (%d, %d) and (%d, %d)\n", found_start_x, found_start_y, found_end_x, found_end_y);
#endif
        return true;
      }
    }
  }

  return false;
}

bool Map::CanOccupy(const Vector2f& position, float radius) const {
#if 0
  const int radius_check = (int)std::ceil(radius * 2.0f);
  bool fit = true;
  std::vector<bool> solid_map;

  if (IsSolid(position)) return false;

  size_t map_size = (radius_check * 2) * (radius_check * 2);

  solid_map.resize(map_size, false);

  // Generate a quick lookup map for solid checks
  for (i16 y = -radius_check; y < radius_check; ++y) {
    for (i16 x = -radius_check; x < radius_check; ++x) {
      uint16_t world_x = (u16)(position.x + x);
      uint16_t world_y = (u16)(position.y + y);

      solid_map[(y + radius_check) * (radius_check * 2) + (x + radius_check)] = IsSolid(world_x, world_y);
    }
  }

  for (int y = 0; y < radius_check; ++y) {
    for (int x = 0; x < radius_check; ++x) {
      // Loop within the check region for empty space
      fit = true;

      for (int check_y = y; check_y < radius_check + y && fit; ++check_y) {
        for (int check_x = x; check_x < radius_check + x; ++check_x) {
          if (solid_map[(check_y + radius_check) * (radius_check * 2) + (x + radius_check)]) {
            fit = false;
            break;
          }
        }
      }

      if (fit) {
        return true;
      }
    }
  }

  return false;
#else
  if (IsSolid(position)) {
    return false;
  }

  radius = std::floor(radius + 0.5f);

  for (float y = -radius; y <= radius; ++y) {
    for (float x = -radius; x <= radius; ++x) {
      uint16_t world_x = (uint16_t)(position.x + x);
      uint16_t world_y = (uint16_t)(position.y + y);
      if (IsSolid(world_x, world_y)) {
        return false;
      }
    }
  }
  return true;
#endif
}

std::vector<const elvl::Region*> Map::GetRegions(Vector2f position) const {
  return GetRegions((u16)position.x, (u16)position.y);
}

bool Map::InRegion(std::string name, Vector2f position) const {
  return InRegion(name, (u16)position.x, (u16)position.y);
}

std::vector<const elvl::Region*> Map::GetRegions(u16 x, u16 y) const {
  std::vector<const elvl::Region*> result;

  for (auto& region : regions) {
    if (region.InRegion(x, y)) {
      result.push_back(&region);
    }
  }

  return result;
}

bool Map::InRegion(std::string name, u16 x, u16 y) const {
  auto iter = region_map.find(name);

  if (iter == region_map.end()) return false;

  return iter->second->InRegion(x, y);
}

void Map::ParseRegions(const char* file_data) {
  using namespace elvl;

  // The offset to elvl data will be stored in the bitmap reserved section.
  std::size_t elvl_metadata_offset = *(u32*)(&file_data[6]);

  if (elvl_metadata_offset) {
    constexpr u32 kElvlMagicNumber = 0x6c766c65;
    struct ElvlMetadataHeader {
      u32 magic;  // 0x6c766c65 ("elvl")
      u32 totalsize;
      u32 reserved;
    };

    struct ELvlChunkHeader {
      u32 type;
      u32 size;
    };

    ElvlMetadataHeader* header = (ElvlMetadataHeader*)(file_data + elvl_metadata_offset);

    if (header->magic == kElvlMagicNumber) {
      u8* current = (u8*)(header + 1);

      while (current < (u8*)header + header->totalsize) {
        ELvlChunkHeader* chunk_header = (ELvlChunkHeader*)current;
        current += sizeof(ELvlChunkHeader);

        u8* chunk_data = current;

        // The case statements here are reversed because of endianness.
        switch (chunk_header->type) {
          case 'RTTA': {  // Attributes
            // This stores the information about the map, such as name, version, zone, creator.
          } break;
          case 'NGER': {
            // Region data
            u8* sub_data = chunk_data;

            regions.emplace_back();
            Region& region = regions.back();

            // The current tile being processed for the run-length encoded data.
            int current_x = 0;
            int current_y = 0;

            while (sub_data < chunk_data + chunk_header->size) {
              ELvlChunkHeader* subchunk = (ELvlChunkHeader*)sub_data;

              sub_data += sizeof(ELvlChunkHeader);

              switch (subchunk->type) {
                case 'MANr': {
                  // Descriptive name
                  region.name = std::string_view((char*)sub_data, subchunk->size);
                } break;
                case 'LITr': {
                  // Tile data
                  u8* tile_ptr = sub_data;

                  while (tile_ptr < sub_data + subchunk->size) {
                    u8 sequence_type = *tile_ptr >> 5;

                    // This sequence type is based on the first 3 bits.
                    // The 1-32 and 1-1024 of the same type are used for optimization since it would require more bits
                    // to encode 1024 always. By using 3 bits to determine, the 1-32 can fit in the remaining 5 bits
                    // instead of having to have an extra byte that would be needed for 1024.
                    //
                    // Since a single tile would be required for the existence of one of these types, it is encoded as
                    // +1 from the remaining bit value. That allows 5 bits to be used for 31(32) since 32 wouldn't
                    // normally fit.
                    switch (sequence_type) {
                      case 0: {  // 1-32 Empty tiles in a row
                        u8 run = (tile_ptr[0] & 0x1F) + 1;

                        current_x += run;
                        if (current_x >= 1024) {
                          current_x = 0;
                          current_y++;
                        }

                        tile_ptr += 1;
                      } break;
                      case 1: {  // 1-1024 Empty tiles in a row
                        u16 run = (((tile_ptr[0] & 3) << 8) | tile_ptr[1]) + 1;

                        current_x += run;
                        if (current_x >= 1024) {
                          current_x = 0;
                          current_y++;
                        }

                        tile_ptr += 2;
                      } break;
                      case 2: {  // 1-32 Present tiles in a row
                        u8 run = (tile_ptr[0] & 0x1F) + 1;

                        for (int i = 0; i < run; ++i) {
                          region.SetTile(current_x + i, current_y);
                        }

                        current_x += run;
                        if (current_x >= 1024) {
                          current_x = 0;
                          current_y++;
                        }

                        tile_ptr += 1;
                      } break;
                      case 3: {  // 1-1024 Present tiles in a row
                        u16 run = (((tile_ptr[0] & 3) << 8) | tile_ptr[1]) + 1;

                        for (int i = 0; i < run; ++i) {
                          region.SetTile(current_x + i, current_y);
                        }

                        current_x += run;
                        if (current_x >= 1024) {
                          current_x = 0;
                          current_y++;
                        }

                        tile_ptr += 2;
                      } break;
                      case 4: {  // 1-32 Rows of empty
                        u8 run = (tile_ptr[0] & 0x1F) + 1;

                        current_x = 0;
                        current_y += run;

                        tile_ptr += 1;
                      } break;
                      case 5: {  // 1-1024 Rows of empty
                        u16 run = (((tile_ptr[0] & 3) << 8) | tile_ptr[1]) + 1;

                        current_x = 0;
                        current_y += run;

                        tile_ptr += 2;
                      } break;
                      case 6: {  // Repeat last row 1-32 times
                        u8 run = (tile_ptr[0] & 0x1F) + 1;

                        current_x = 0;

                        for (int i = 0; i < run; ++i) {
                          for (int x = 0; x < 1024; ++x) {
                            if (region.InRegion(x, current_y - 1)) {
                              region.SetTile(x, current_y + i);
                            }
                          }
                        }

                        current_y += run;

                        tile_ptr += 1;
                      } break;
                      case 7: {  // Repeat last row 1-204 times
                        u16 run = (((tile_ptr[0] & 3) << 8) | tile_ptr[1]) + 1;

                        current_x = 0;

                        for (int i = 0; i < run; ++i) {
                          for (int x = 0; x < 1024; ++x) {
                            if (region.InRegion(x, current_y - 1)) {
                              region.SetTile(x, current_y + i);
                            }
                          }
                        }

                        current_y += run;

                        tile_ptr += 2;
                      } break;
                    }
                  }
                } break;
                case 'ESBr': {
                  // Base
                  region.flags |= RegionFlag_Base;
                } break;
                case 'WANr': {
                  // No antiwarp
                  region.flags |= RegionFlag_NoAntiwarp;
                } break;
                case 'PWNr': {
                  // No weapons
                  region.flags |= RegionFlag_NoWeapons;
                } break;
                case 'LFNr': {
                  // No flag drops
                  region.flags |= RegionFlag_NoFlags;
                } break;
                case 'PWAr': {
                  // Auto-warp upon entering region
                } break;
                case 'CYPr': {
                  // Python execution on enter/leave
                } break;
              }

              // Align the pointer to 4 byte boundary.
              sub_data += (subchunk->size + 3) & ~3;
            }
          } break;
          case 'TEST': {
            // Tileset data
          } break;
          case 'ELIT': {
            // Tile data
          } break;
          default: {
            // Invalid
            break;
          }
        }

        // Align the pointer to 4 byte boundary.
        current += (chunk_header->size + 3) & ~3;
      }
    }

    // Create a mapping from names to region data.
    for (auto& region : regions) {
      region_map[region.name] = &region;
    }
  }
}

struct Tile {
  u32 x : 12;
  u32 y : 12;
  u32 tile : 8;
};

std::unique_ptr<Map> Map::Load(const char* filename) {
  std::ifstream input(filename, std::ios::in | std::ios::binary);

  if (!input.is_open()) return nullptr;

  input.seekg(0, std::ios::end);
  std::size_t size = static_cast<std::size_t>(input.tellg());
  input.seekg(0, std::ios::beg);

  std::string data;
  data.resize(size);

  input.read(&data[0], size);

  std::size_t pos = 0;

  if (data[0] == 'B' && data[1] == 'M') {
    pos = *(u32*)(&data[2]);
  }

  TileData tiles(kMapExtent * kMapExtent);

  while (pos < size) {
    Tile tile = *reinterpret_cast<Tile*>(&data[pos]);

    tiles[tile.y * kMapExtent + tile.x] = tile.tile;

    if (tile.tile == 217) {
      // Mark large asteroid
      for (std::size_t y = 0; y < 2; ++y) {
        for (std::size_t x = 0; x < 2; ++x) {
          tiles[(tile.y + y) * kMapExtent + (tile.x + x)] = tile.tile;
        }
      }
    } else if (tile.tile == 219) {
      // Mark space station tiles
      for (std::size_t y = 0; y < 6; ++y) {
        for (std::size_t x = 0; x < 6; ++x) {
          tiles[(tile.y + y) * kMapExtent + (tile.x + x)] = tile.tile;
        }
      }
    } else if (tile.tile == 220) {
      // Mark wormhole
      for (std::size_t y = 0; y < 5; ++y) {
        for (std::size_t x = 0; x < 5; ++x) {
          tiles[(tile.y + y) * kMapExtent + (tile.x + x)] = tile.tile;
        }
      }
    }

    pos += sizeof(Tile);
  }

  auto map = std::make_unique<Map>(tiles);

  map->ParseRegions(data.data());

  return map;
}

std::unique_ptr<Map> Map::Load(const std::string& filename) { return Load(filename.c_str()); }

}  // namespace elm
