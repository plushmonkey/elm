#include "Timer.h"

#include <chrono>

namespace elm {

u64 GetTime() {
  return std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now())
      .time_since_epoch()
      .count();
}

Timer::Timer() { begin_time = GetTime(); }

u64 Timer::GetElapsedTime() {
  u64 now = GetTime();
  u64 elapsed = now - begin_time;

  begin_time = now;

  return elapsed;
}

}  // namespace elm
