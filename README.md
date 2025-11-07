# 🥔 hashbrowns

*A crispy C++ benchmarking suite comparing arrays, linked lists, and hash maps — cooked to perfection with real performance data.*

---

## 🍳 Overview

**hashbrowns** is a self-contained C++ project that benchmarks the performance of three fundamental data structures:

- **Array** (contiguous memory)
# hashbrowns

[![CI](https://github.com/aeml/hashbrowns/actions/workflows/ci.yml/badge.svg)](https://github.com/aeml/hashbrowns/actions/workflows/ci.yml)

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
- Optional (for plotting): Python 3 + `pip install -r requirements.txt`

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

Install (optional):
```bash
cmake --install build --prefix /usr/local
```

---

## Usage

The executable supports the following options:

- `--size N` — number of elements (default: 10000)
- `--runs N` — repetitions per test (default: 10)
- `--series-count N` — if >1, treat `--size` as the maximum and run a linear multi-size series with N evenly spaced sizes (e.g. `--size 10000 --series-count 4` -> 2500, 5000, 7500, 10000)
- `--series-out FILE` — output file for multi-size series results (default: `results/csvs/series_results.csv|json` depending on `--out-format`)
- `--structures LIST` — comma-separated list of structures: `array,slist,dlist,hashmap`
- `--output FILE` — write benchmark or crossover results to CSV/JSON
- `--out-format {csv,json}` — select output format (default: csv)
- `--wizard` — interactive mode to choose structures and settings (easy "test-all" flow)
- `--memory-tracking` — enable detailed memory tracking during runs
- `--pattern {sequential,random,mixed}` — data pattern for inserts/search/removes; with `--seed N` for reproducibility
- `--crossover-analysis` — estimate crossover points by running a size sweep
- `--max-size N` — maximum size used for crossover analysis (default: 100000)
- `--series-runs N` — runs per size during sweep (default: 1)
- `--max-seconds N` — time budget to cap crossover sweep
- HashMap tuning:
    - `--hash-strategy {open,chain}` — open addressing or separate chaining
    - `--hash-capacity N` — initial capacity (power-of-two rounded)
    - `--hash-load F` — max load factor
- `--help` — show help

Examples:

```bash
# Compare default structures at a single size
./build/hashbrowns --size 50000 --runs 20

# Select structures and write benchmark CSV (written under results/csvs by default if path omitted)
./build/hashbrowns --structures array,hashmap --size 20000 --runs 10 --output results/csvs/benchmark_results.csv

# Crossover analysis over a size sweep, writing JSON
./build/hashbrowns --crossover-analysis --max-size 100000 \
    --structures array,slist,hashmap --runs 5 \
    --out-format json --output results/csvs/crossover_results.json

# Wizard (interactive multi-size benchmarking + optional plotting)
./build/hashbrowns --wizard
    # Prompts will include: structures, max size, number of sizes (default 10), runs per size (default 10),
    # pattern, seed, output format, and an option to auto-generate series plots.

# CLI multi-size series without wizard (CSV):
./build/hashbrowns --size 20000 --series-count 5 --runs 5 --series-out results/csvs/series_results.csv --out-format csv

# CLI multi-size series JSON (default path):
./build/hashbrowns --size 50000 --series-count 8 --runs 3 --out-format json
```

---

## Scripts

Convenience tools for common workflows:

-- `scripts/run_benchmarks.sh` — builds the project if needed, runs benchmarks and a size-sweep crossover analysis, writes CSV/JSON under `results/csvs`, and (by default) generates PNG plots (if `matplotlib` is available) under `results/plots`.
        - Examples:
            - Quick, with plots (auto y-scale):
                - `scripts/run_benchmarks.sh --runs 5 --size 20000 --yscale auto`
            - Thorough single-size benchmark only:
                - `scripts/run_benchmarks.sh --runs 20 --size 50000 --no-plots`
            - Focus on structures and smaller sweep:
                - `STRUCTURES=array,hashmap scripts/run_benchmarks.sh --max-size 8192 --series-runs 1`
    - Outputs:
        - `results/csvs/benchmark_results.csv|json`
        - `results/csvs/crossover_results.csv|json`
        - `results/plots/benchmark_summary.png` (if plotting available)
        - `results/plots/crossover_points.png` (if plotting available)
        - `results/plots/benchmark_by_operation.png` (if plotting available)
        - `results/plots/series_insert.png|series_search.png|series_remove.png` (multi-size series plots)
    - Options:
        - `--no-plots` to skip PNG generation
        - `--plots` to force plotting (fails if matplotlib absent)
        - `--plots-dir PATH` to select output directory for PNGs
        - `--yscale {auto,linear,mid,log}` to control plot scaling (default: auto)
    - `--out-format {csv,json}` select export format (plots currently support CSV only)
    - `--series-runs N` to limit repeats per size during the crossover sweep (default: 1)
    - `--pattern {sequential,random,mixed}` controls key order (inserts/search/removes)
    - `--seed N` RNG seed for reproducible random/mixed patterns
    - `--max-seconds N` time budget (seconds) to cap the crossover sweep
    - `--hash-strategy {open,chain}`, `--hash-capacity N`, `--hash-load F` to tune HashMap behavior

- `scripts/analyze_results.py` — prints a concise summary from the CSV outputs (no external Python deps).
    - Example:
        - `scripts/analyze_results.py --bench-csv build/benchmark_results.csv --cross-csv build/crossover_results.csv`

- `scripts/plot_results.py` — generates PNG plots from the CSV outputs (requires matplotlib). Now supports `--series-csv` for multi-size runs. Install deps with:
    ```bash
    pip install -r requirements.txt
    ```
    - Example:
    - `scripts/plot_results.py --bench-csv results/csvs/benchmark_results.csv --cross-csv results/csvs/crossover_results.csv --out-dir results/plots`
    - Outputs:
        - `benchmark_summary.png` (auto log scale if dominated by outliers)
        - `benchmark_by_operation.png` (separate subplots for insert/search/remove)
        - Options:
            - `--yscale {auto,linear,mid,log}` to control y-axis scaling. "mid" uses an asinh-based scale for a middle ground between linear and log (good when one op dominates but you still want visible differences).

---

## Run benchmarks (recommended)

Use the helper script to compile, run, and generate CSVs/PNGs in one go. All generated artifacts land under `results/`.

- Quick run with crossover sweep and plots:
    ```bash
    scripts/run_benchmarks.sh --runs 5 --size 20000 --max-size 32768 --yscale auto
    ```

- Faster sweep when arrays are included (avoid long array removals):
    ```bash
    scripts/run_benchmarks.sh --series-runs 1 --max-size 8192 --max-seconds 10
    ```

- Full single-size benchmark without plots:
    ```bash
    scripts/run_benchmarks.sh --runs 20 --size 50000 --no-plots
    ```

Notes:
- The sweep defaults to `series-runs=1` for speed; increase to stabilize results.
- Large `max-size` with arrays can be very slow for remove; prefer smaller `max-size` or fewer `series-runs`.
- Plots include a footer with the parameters and hardware summary for traceability.

Reproducible random workload:
```bash
scripts/run_benchmarks.sh --pattern random --seed 12345 --runs 5 --size 20000
```

### Results directory layout

All benchmark outputs (CSV/JSON) and plots are now organized under a top-level `results/` directory:

```
results/
    csvs/
        benchmark_results.csv        # single-size benchmark summary
        benchmark_results.json       # same data serialized as JSON (if requested)
        crossover_results.csv        # size sweep crossover estimates
        crossover_results.json       # crossover JSON (if requested)
        series_results.csv           # multi-size series produced via wizard (CSV only for plotting)
        series_results.json          # optional JSON for series metadata/results
    # (CLI multi-size also uses these if --series-count provided)
    plots/
        benchmark_summary.png
        benchmark_by_operation.png
        crossover_points.png
        series_insert.png
        series_search.png
        series_remove.png
```

If you supply a custom `--output` path for the main executable it can still write anywhere; scripts and wizard defaults prefer the structured layout above.
    ```

---

## Building on different platforms

Requirements:
- CMake 3.16+
- A C++17 compiler

Linux (Debian/Ubuntu):
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake
scripts/build.sh -t Release --test
```

macOS:
```bash
xcode-select --install   # install command line tools
brew install cmake       # if Homebrew is available
scripts/build.sh -t Release --test
```

Windows (MSVC):
```powershell
# Install Visual Studio Build Tools (C++ workload) and CMake
# From x64 Native Tools Developer Command Prompt:
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles"
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Windows (MSYS2/MinGW):
```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake make
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Performance reproducibility tips:
- Use a stable power profile; disable turbo/boost if you need highly repeatable numbers.
- Close background tasks; run on AC power for laptops.
- Pin CPU frequency/governor on Linux (e.g., `performance`) if appropriate.
- Run multiple times and compare medians if variance is high.

---

## Docker (optional)

Build and run in a container for reproducible environments:

```bash
docker build -t hashbrowns:latest .
docker run --rm -it -v "$PWD/build:/app/build" hashbrowns:latest
```

This produces build artifacts in `build/` and result artifacts in `results/` mounted from the host.

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

The test `test_series_json.cpp` ensures the JSON writer for multi-size series emits metadata (`runs_per_size`) and the expected number of points.

---

## License

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE).

- ✅ **Template Metaprogramming** for type-safe generics
- ✅ **Professional C++ Development** with modern tooling

Perfect for portfolios, technical interviews, and advanced C++ learning!

```

