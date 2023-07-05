#pragma once

#include <elm/Types.h>

namespace elm {

u64 GetTime();

struct Timer {
  Timer();
  u64 GetElapsedTime();

 private:
  u64 begin_time;
};

}  // namespace elm
