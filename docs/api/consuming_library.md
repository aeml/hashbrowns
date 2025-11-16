# Consuming hashbrowns as a Library

This project installs three libraries and public headers:
- `hashbrowns_core` (timer, memory tracker)
- `hashbrowns_structures` (dynamic array, linked lists, hash map)
- `hashbrowns_benchmark` (BenchmarkSuite and helpers)

Install locally:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cmake --install build --prefix "$PWD/install"
```

Then in your project:

```cmake
cmake_minimum_required(VERSION 3.16)
project(mybench LANGUAGES CXX)

# If installed to a custom prefix
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/../hashbrowns/install")

find_package(hashbrowns REQUIRED)

add_executable(mybench main.cpp)
# Link to the benchmark suite (it will bring core + structures transitively)
# If targets are not exported, fall back to manual find via include/lib dirs.
target_link_libraries(mybench PRIVATE hashbrowns_benchmark)
```

Minimal `main.cpp`:

```cpp
#include "benchmark/benchmark_suite.h"
#include <iostream>

int main(){
  hashbrowns::BenchmarkConfig cfg;
  cfg.structures = {"array","hashmap"};
  cfg.size = 4096;
  cfg.runs = 5;
  hashbrowns::BenchmarkSuite suite;
  auto results = suite.run(cfg);
  for (auto &r : results) {
    std::cout << r.structure << ": insert=" << r.insert_ms_mean
              << ", search=" << r.search_ms_mean
              << ", remove=" << r.remove_ms_mean << "\n";
  }
  return 0;
}
```

If `find_package(hashbrowns)` is not available (no export), you can directly use include + lib paths:

```cmake
include_directories("${hashbrowns_root}/install/include")
link_directories("${hashbrowns_root}/install/lib")
add_executable(mybench main.cpp)
target_link_libraries(mybench PRIVATE hashbrowns_benchmark hashbrowns_structures hashbrowns_core)
```
