#ifndef _TIMER_HH
#define _TIMER_HH

#include <iostream>
#include <vector>
#include <ctime>

class Timer {
private:
  struct timespec clock_start;
  std::vector<uint64_t> times;

public:
  void start() {
    clock_gettime(CLOCK_REALTIME, &clock_start);
  }

  void stop() {
    struct timespec clock_end;
    clock_gettime(CLOCK_REALTIME, &clock_end);
    uint64_t s = clock_end.tv_sec - clock_start.tv_sec;
    uint64_t ns = clock_end.tv_nsec - clock_start.tv_nsec; // May be negative
    uint64_t t = (s * 1000000000) + ns;
    times.push_back(t);
  }

  uint64_t average() const {
    assert((times.size() > 0) && "No timing data recorded yet");
    uint64_t count = 0;
    for (auto it : times) {
      count += it;
    }
    return count / times.size();
  }

  friend std::ostream& operator<< (std::ostream &out, const Timer &timer) {
    uint64_t t = timer.average();
    if (t > 1000000000) {
      double t2 = ((double)t)/1000000000;
      out << t2 << "s";
    } else if (t > 1000000) {
      double t2 = ((double)t)/1000000;
      out << t2 << "ms";
    } else if (t > 1000) {
      double t2 = ((double)t)/1000;
      out << t2 << "μs";
    } else {
      out << t << "ns";
    }
    return out;
  }
};

#endif // _TIMER_HH
