# 🥔 hashbrowns

*A crispy C++ benchmarking suite comparing arrays, linked lists, and hash maps — cooked to perfection with real performance data.*

---

## 🍳 Overview

**hashbrowns** is a self-contained C++ project that benchmarks the performance of three fundamental data structures:

- **Array** (contiguous memory)
# hashbrowns

hashbrowns is a C++17 benchmarking suite for comparing custom implementations of dynamic arrays, linked lists, and hash maps. It measures insert, search, and remove performance across configurable sizes and runs, and can export results to CSV or estimate crossover points between data structures.

---

## Overview

The project contains:
- Minimal data structure implementations (dynamic array, singly/doubly linked lists, hash map)
- A small benchmarking framework with repeated runs and summary statistics
- CSV export for offline analysis
- Optional crossover analysis over a sweep of input sizes

The focus is on clear implementations and reproducible measurements rather than feature completeness.

---

## Features

- Custom data structures implemented in the repository
- Polymorphic interface to benchmark different structures via a common API
- High-resolution timing and aggregation (mean/stddev) over multiple runs
- CLI flags for size, runs, output CSV, and crossover analysis
- Convenience scripts to run and summarize benchmarks

---

## Project layout

```
src/
    main.cpp
    benchmark/
        benchmark_suite.h
        benchmark_suite.cpp
        stats_analyzer.h
    core/
        data_structure.h
        memory_manager.h
        memory_manager.cpp
        timer.h
        timer.cpp
    structures/
        dynamic_array.h
        dynamic_array.cpp
        linked_list.h
        linked_list.cpp
        hash_map.h
        hash_map.cpp
tests/
    unit/
        main_tests.cpp
        test_dynamic_array.cpp
        test_hash_map.cpp
        test_linked_list.cpp
scripts/
    build.sh
    run_benchmarks.sh
    analyze_results.py
docs/
    api/
    tutorials/
CMakeLists.txt
README.md
LICENSE
```

---

## Build

Requirements:
- CMake 3.16+
- A C++17 compiler (GCC, Clang, or MSVC)

Using the provided script (recommended):

```bash
scripts/build.sh -c -t Release --test
```

Manual steps:

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest --output-on-failure
```

---

## Usage

The executable supports the following options:

- `--size N` — number of elements (default: 10000)
- `--runs N` — repetitions per test (default: 10)
- `--structures LIST` — comma-separated list of structures: `array,slist,dlist,hashmap`
- `--output FILE` — write benchmark or crossover results to CSV
- `--memory-tracking` — enable detailed memory tracking during runs
- `--crossover-analysis` — estimate crossover points by running a size sweep
- `--max-size N` — maximum size used for crossover analysis (default: 100000)
- `--help` — show help

Examples:

```bash
# Compare default structures at a single size
./build/hashbrowns --size 50000 --runs 20

# Select structures and write benchmark CSV
./build/hashbrowns --structures array,hashmap --size 20000 --runs 10 --output build/benchmark_results.csv

# Crossover analysis over a size sweep, writing CSV
./build/hashbrowns --crossover-analysis --max-size 100000 \
    --structures array,slist,hashmap --runs 5 --output build/crossover_results.csv
```

---

## Scripts

Convenience tools for common workflows:

- `scripts/run_benchmarks.sh` — builds the project if needed, runs benchmarks and crossover analysis, and writes CSVs.
    - Examples:
        - `scripts/run_benchmarks.sh --runs 20 --size 50000`
        - `STRUCTURES=array,hashmap scripts/run_benchmarks.sh --max-size 200000`
    - Outputs:
        - `build/benchmark_results.csv`
        - `build/crossover_results.csv`

- `scripts/analyze_results.py` — prints a concise summary from the CSV outputs (no external Python deps).
    - Example:
        - `scripts/analyze_results.py --bench-csv build/benchmark_results.csv --cross-csv build/crossover_results.csv`

---

## Testing

Run unit tests via the build script:

```bash
scripts/build.sh -t Debug --test
```

Or from the build directory:

```bash
ctest --output-on-failure
```

---

## License

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE).

- ✅ **Template Metaprogramming** for type-safe generics
- ✅ **Professional C++ Development** with modern tooling

Perfect for portfolios, technical interviews, and advanced C++ learning!

```

