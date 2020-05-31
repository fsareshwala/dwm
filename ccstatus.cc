// NOTE: this file is deprecated. It contains a status bar replacement I had written in C++ but I
// decided to abandon it. I've kept it around in case I want to look at the source code again.

#include <chrono>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include <X11/Xlib.h>

// --- utility functions
static std::string current_time(const std::string& format, const std::string& timezone) noexcept {
  int ret = setenv("TZ", timezone.c_str(), 1);
  if (ret == -1) {
    std::cerr << "error: could not update timezone -- " << std::strerror(errno) << std::endl;
    return "unknown (tz error)";
  }

  std::time_t t = std::time(nullptr);
  std::tm* tm = std::localtime(&t);

  std::ostringstream oss;
  oss << std::put_time(tm, format.c_str());
  return oss.str();
}

static std::vector<std::string> split(const std::string& input) noexcept {
  std::istringstream iss(input);

  std::vector<std::string> elements;
  std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(),
            std::back_inserter(elements));

  return elements;
}

static std::string join(const std::string& separator,
                        const std::vector<std::string>& elements) noexcept {
  bool added = false;
  std::ostringstream oss;

  std::for_each(elements.begin(), elements.end(), [&](const std::string& element) {
    if (added) {
      oss << separator;
    }

    added = true;
    oss << element;
  });

  return oss.str();
}

// --- collectors
static std::string collect_load() noexcept {
  std::ostringstream oss;
  oss << "load one: ";

  size_t size = 3;
  double averages[size];
  int ret = getloadavg(averages, size);
  if (ret == -1) {
    oss << "unknown";
    return oss.str();
  }

  oss << std::fixed << std::setprecision(1) << averages[0];
  return oss.str();
}

static std::string collect_cpu_usage() noexcept {
  std::ifstream proc_stat("/proc/stat");

  std::string line;
  std::getline(proc_stat, line);
  std::vector<std::string> str_elements = split(line);
  str_elements.erase(str_elements.begin());  // remove cpu prefix

  std::vector<int> cpu_times;
  cpu_times.reserve(str_elements.size());

  std::transform(str_elements.begin(), str_elements.end(), std::back_inserter(cpu_times),
                 [](const std::string& element) -> int { return std::stoi(element); });

  static int prev_idle = 0;
  static int prev_total = 0;

  int idle = cpu_times[3];
  int total = std::accumulate(cpu_times.begin(), cpu_times.end(), 0);

  float idle_delta = idle - prev_idle;
  float total_delta = total - prev_total;

  float utilization = 100.0 * (1.0 - idle_delta / total_delta);

  prev_idle = idle;
  prev_total = total;

  std::ostringstream oss;
  oss << "cpu: " << std::fixed << std::setprecision(1) << utilization << "%";
  return oss.str();
}

// https://stackoverflow.com/a/41251290
static std::string collect_memory_usage() noexcept {
  std::ifstream meminfo("/proc/meminfo");

  int total_memory = 0;
  int free_memory = 0;
  int buffers = 0;
  int cached = 0;
  int s_reclaimable = 0;
  int shmem = 0;

  while (true) {
    std::string line;
    std::getline(meminfo, line);

    if (meminfo.eof()) {
      break;
    }

    std::vector<std::string> elements = split(line);
    elements[0].erase(elements[0].size() - 1);  // erase the extra colon

    const std::string& key = elements[0];
    int value = std::stoll(elements[1]);

    if (key == "MemTotal") {
      total_memory = value;
    }
    else if (key == "MemFree") {
      free_memory = value;
    }
    else if (key == "Buffers") {
      buffers = value;
    }
    else if (key == "Cached") {
      cached = value;
    }
    else if (key == "SReclaimable") {
      s_reclaimable = value;
    }
    else if (key == "Shmem") {
      shmem = value;
    }
  }

  float total_used_memory = total_memory - free_memory;
  float cached_memory = cached - s_reclaimable - shmem;
  float actual_used_memory = total_used_memory - buffers - cached_memory;
  float percentage_used = 100.0 * (actual_used_memory / total_memory);

  std::ostringstream oss;
  oss << "mem: " << std::fixed << std::setprecision(1) << percentage_used << "%";
  return oss.str();
}

static std::string collect_local_time() noexcept {
  return current_time("%A, %B %d, %Y, %I:%M:%S %p", "America/Los_Angeles");
}

static std::string collect_india_time() noexcept {
  return current_time("IN: %I:%M:%S %p", "Asia/Calcutta");
}

int main() noexcept {
  Display* display = XOpenDisplay(nullptr);
  if (display == nullptr) {
    std::cerr << "ccstatus: cannot open display" << std::endl;
    return 1;
  }

  std::vector<std::function<std::string()>> collectors = {
      collect_load, collect_cpu_usage, collect_memory_usage, collect_india_time, collect_local_time,
  };

  while (true) {
    std::vector<std::string> elements;
    elements.reserve(collectors.size());

    std::transform(
        collectors.begin(), collectors.end(), std::back_inserter(elements),
        [](const std::function<std::string()>& collector) -> std::string { return collector(); });

    std::string status = join(" | ", elements);

    XStoreName(display, DefaultRootWindow(display), status.c_str());
    XSync(display, False);

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
}
