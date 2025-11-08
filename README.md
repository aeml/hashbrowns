# 🥔 hashbrowns

*A crispy C++ benchmarking suite comparing arrays, linked lists, and hash maps — cooked to perfection with real performance data.*

---

## 🍳 Overview

**hashbrowns** is a self-contained C++ project that benchmarks the performance of three fundamental data structures:

- **Array** (contiguous memory)
# hashbrowns

[![CI](https://github.com/aeml/hashbrowns/actions/workflows/ci.yml/badge.svg)](https://github.com/aeml/hashbrowns/actions/workflows/ci.yml)
[![Coverage](https://img.shields.io/endpoint?url=https://aeml.github.io/hashbrowns/coverage/coverage_badge.json)](https://aeml.github.io/hashbrowns/coverage/)

hashbrowns is a C++17 benchmarking suite for comparing custom implementations of dynamic arrays, linked lists, and hash maps. It measures insert, search, and remove performance across configurable sizes and runs, and can export results to CSV or estimate crossover points between data structures.

> Documentation & Tutorials: API docs, coverage report, Quickstart, and Memory & Probes guides are published via GitHub Pages: https://aeml.github.io/hashbrowns/
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

### Core CLI Flags (single-size, series, crossover, reproducibility)

The `hashbrowns` executable supports a rich set of flags. All flags are optional; sensible defaults are chosen for quick runs.

Benchmark scope & iteration control:
- `--size N`  Size for single benchmark mode (default: 10000). When a series is requested this is treated as the MAX size unless explicit sizes provided.
- `--runs N`  Repetitions per structure for single-size benchmark (default: 10). Used as "runs per size" for series unless overridden by `--series-runs`.
- `--warmup N`  Discard first N runs (warm-up) from timing statistics (default: 0).
- `--bootstrap N`  Perform N bootstrap iterations to build a 95% CI for mean (0 disables; typical: 200–1000).

Multi-size / sweeping:
- `--series-count N`  If >1: run a linear multi-size series up to `--size`, producing N evenly spaced sizes.
- `--series-sizes LIST`  Explicit comma-separated sizes (overrides `--series-count` spacing).
- `--series-out FILE`  Output path for series results (defaults under `results/csvs/series_results.*`).
- `--series-runs N`  Runs per size for a series or crossover sweep (default: 1 for speed). Increase for stability.

Workload pattern & reproducibility:
- `--pattern {sequential,random,mixed}`  Key access pattern; affects insert/search/remove ordering.
- `--seed N`  RNG seed for random/mixed patterns (default: generated via `std::random_device`). Emitted in JSON/CSV for reproducibility.
- `--pin-cpu [IDX]`  Pin process to a specific CPU core (Linux-only). If IDX omitted, uses 0. Emits `pinned_cpu` in meta.
- `--no-turbo`  Attempt to disable CPU turbo boost (Linux-only, best-effort). Emits `turbo_disabled` and current `cpu_governor`.

Output & formatting:
- `--output FILE`  Write single-size benchmark OR crossover results to file (CSV/JSON based on `--out-format`).
- `--out-format {csv,json}`  Choose output serialization (default: csv).
- `--memory-tracking`  Enable per-operation memory delta sampling (adds memory columns to CSV and memory stats to JSON).
- `--wizard`  Interactive guided run (series or crossover) with prompts and default paths.
- `--no-banner` / `--quiet`  Suppress banner or most stdout (used by scripts / CI).

Crossover (size sweep inference):
- `--crossover-analysis`  Perform a size sweep to estimate crossover points between structures per operation.
- `--max-size N`  Maximum size for crossover sweep (default: 100000).
- `--max-seconds N`  Time budget to cap crossover sweep early.

HashMap tuning:
- `--hash-strategy {open,chain}`  Select open addressing or separate chaining.
- `--hash-capacity N`  Initial capacity (auto rounded to power-of-two for open addressing).
- `--hash-load F`  Target max load factor before growth.

Help & verbosity:
- `--verbose`  Print extra per-run diagnostics.
- `--help`  Show help text.
- `--version`  Print the project version and short git SHA and exit.

### Examples

```bash
# Compare default structures at a single size
./build/hashbrowns --size 50000 --runs 20

# Select structures and write benchmark CSV (written under results/csvs by default if path omitted)
./build/hashbrowns --structures array,hashmap --size 20000 --runs 10 --output results/csvs/benchmark_results.csv

# Crossover analysis over a size sweep, writing JSON
./build/hashbrowns --crossover-analysis --max-size 100000 \
    --structures array,slist,hashmap --runs 5 \
    --out-format json --output results/csvs/crossover_results.json

### Wizard (interactive multi-size benchmarking + optional plotting)
./build/hashbrowns --wizard
    # Prompts will include: structures, max size, number of sizes (default 10), runs per size (default 10),
    # pattern, seed, output format, and an option to auto-generate series plots.

### CLI multi-size series without wizard (CSV example):
./build/hashbrowns --size 20000 --series-count 5 --runs 5 --series-out results/csvs/series_results.csv --out-format csv

### CLI multi-size series JSON (default path):
./build/hashbrowns --size 50000 --series-count 8 --runs 3 --out-format json
```

---

## Output Schema & Environment Metadata

Each JSON output (`benchmark_results.json`, `crossover_results.json`, `series_results.json`) includes a `meta` block capturing run parameters and a reproducibility snapshot:

```
"meta": {
    "size": <int>,                // single-size only
    "runs": <int>,
    "warmup_runs": <int>,
    "bootstrap_iters": <int>,
    "structures": ["array", ...],
    "pattern": "sequential|random|mixed",
    "seed": <uint64>,             // always resolved (generated if not supplied)
    "timestamp": "UTC ISO-8601",
    "cpu_governor": "performance|powersave|unknown",
    "git_commit": "<short sha|unknown>",
    "compiler": "GCC X.Y.Z | Clang X.Y.Z | MSVC <number> | unknown",
    "cpp_standard": "C++17|C++20|...",
    "build_type": "Release|Debug|RelWithDebInfo",
    "cpu_model": "<string>",
    "cores": <int>,
    "total_ram_bytes": <uint64>,
    "kernel": "OS KernelVersion",
    "hash_strategy": "open|chain",
    "hash_capacity": <int?>,
    "hash_load": <float?>,
    "pinned_cpu": <int|-1>,
    "turbo_disabled": 0|1
}
```

CSV benchmark output columns (single-size):
```
structure,seed,
insert_ms_mean,insert_ms_stddev,insert_ms_median,insert_ms_p95,insert_ci_low,insert_ci_high,
search_ms_mean,search_ms_stddev,search_ms_median,search_ms_p95,search_ci_low,search_ci_high,
remove_ms_mean,remove_ms_stddev,remove_ms_median,remove_ms_p95,remove_ci_low,remove_ci_high,
memory_bytes,memory_insert_mean,memory_insert_stddev,memory_search_mean,memory_search_stddev,memory_remove_mean,memory_remove_stddev,
insert_probes_mean,insert_probes_stddev,search_probes_mean,search_probes_stddev,remove_probes_mean,remove_probes_stddev
```

Notes:
- Bootstrapped CIs appear when `--bootstrap N` is non-zero (otherwise CI columns remain 0 or placeholder values).
- Median for even sample counts uses the average of the two middle sorted values.
- `memory_*` metrics represent per-operation incremental allocation deltas averaged across runs (requires `--memory-tracking`).
- `*_probes_*` metrics are HashMap-specific (open addressing probe counts). Non-hash structures have zeros.
- Series CSV: `size,structure,insert_ms,search_ms,remove_ms` (one row per size × structure).
- Crossover CSV: `operation,a,b,size_at_crossover`.

Performance reproducibility tips are in the section below; the snapshot fields help post-hoc analysis & plot annotations.

### Memory Tracking Details

Enable with `--memory-tracking` to capture allocation deltas per operation. Internally a lightweight tracker records net bytes allocated/free during each insert/search/remove loop.

Reported fields:
- `memory_bytes` – total peak bytes retained by the structure at benchmark end (approximate live footprint).
- `memory_insert_mean/stddev` – average incremental allocation cost (bytes) during inserts across runs.
- `memory_search_mean/stddev` – usually zero (search is read-only for these structures); non‑zero indicates incidental allocations (should be investigated).
- `memory_remove_mean/stddev` – net allocation delta during removals (often small or zero; deallocation may not appear as negative if allocation tracking normalizes to positive deltas).

Interpretation tips:
- Large `memory_insert_mean` for a dynamic array suggests frequent growth; consider a different growth strategy.
- For HashMap, spikes can reflect rehashing; tune with `--hash-capacity` / `--hash-load`.
- If search/remove show unexpected non-zero means, it may indicate hidden allocations (e.g., string constructions); profile further.

Overhead: Tracking adds a small constant cost; for tight microbenchmarks you can disable it to avoid perturbing timings. Memory numbers are still meaningful for relative comparisons.

### HashMap Probe Metrics

The HashMap implementation (open addressing strategy) records average probe counts per operation to illuminate table efficiency:
- `insert_probes_mean/stddev` – average number of slot probes during insert.
- `search_probes_mean/stddev` – average probes for lookup.
- `remove_probes_mean/stddev` – average probes during deletes.

Healthy ranges:
- At moderate load (< 0.7) typical search probes should be near 1–2; insert may be slightly higher especially near growth thresholds.
- If means climb above ~5 consistently, table is either too full or hash distribution is poor; consider lowering `--hash-load` or increasing initial capacity.
- Large stddev coupled with high mean can signal clustering; switching to `--hash-strategy chain` (separate chaining) might stabilize probe counts for skewed workloads.

Tuning guidelines:
1. Start with `--hash-load 0.75` (default behavior) and observe probe means.
2. Reduce to `--hash-load 0.6` if probes exceed 3–4.
3. Provide a generous `--hash-capacity` for very large initial `--size` to avoid repeated rehash cycles.
4. For mixed/random patterns, stable probe counts help keep variance low; pair with a fixed `--seed` when comparing changes.

Structures other than the HashMap report zeros for probe fields.

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

### Generate API Documentation (Optional)

If Doxygen is installed you can build browsable HTML docs for the public interfaces:

```bash
cmake -S . -B build
cmake --build build --target docs
xdg-open build/docs/html/index.html  # Linux (optional)
open build/docs/html/index.html      # macOS (optional)
```

Documentation excludes test sources and is intentionally lightweight (no diagrams by default). Enable diagrams by editing `docs/Doxyfile.in` and setting `HAVE_DOT = YES` locally if you need call graphs.

### Coverage report (hosted)

We publish the latest coverage HTML and a JSON badge via GitHub Pages:

- Coverage badge (lines): linked at the top of this README
- Full report: https://aeml.github.io/hashbrowns/coverage/

Notes:
- Values are from lcov on a Debug build with gcov flags; system and test files are filtered out.
- Line coverage is used for the badge; branch/function coverage totals are included in the coverage_summary.json.

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

## Documentation & Tutorials

Hosted (GitHub Pages): https://aeml.github.io/hashbrowns/

| Resource | URL |
|----------|-----|
| API Docs | https://aeml.github.io/hashbrowns/docs/ |
| Coverage Report | https://aeml.github.io/hashbrowns/coverage/ |
| Quickstart Tutorial | https://aeml.github.io/hashbrowns/tutorials/quickstart.html |
| Memory & Probes Tutorial | https://aeml.github.io/hashbrowns/tutorials/memory_and_probes.html |
| JSON Schema (repo) | docs/api/schema.md |
| Example JSON Parser | scripts/example_parse.py |

### Parsing JSON (example)

Summarize a benchmark JSON (lines, functions, memory):

```bash
python3 scripts/example_parse.py results/csvs/benchmark_results.json --summary
```

CSV-style condensed output for automation:
```bash
python3 scripts/example_parse.py results/csvs/benchmark_results.json --csv > summary.csv
```

### Rebuilding docs locally

```bash
cmake -S . -B build
cmake --build build --target docs
xdg-open build/docs/html/index.html  # or open build/docs/html/index.html on macOS
```

Tutorial Markdown lives in `docs/tutorials/` and is converted to HTML during the Pages workflow via pandoc.

---

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

```

