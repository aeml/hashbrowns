# Contributing to hashbrowns

Thanks for your interest in improving hashbrowns! This guide explains how to build, test, format, and benchmark the project, plus standards for reproducibility and performance contributions.

## Table of Contents
1. Getting Started
2. Build & Test
3. Code Style & Formatting
4. Adding Data Structures
5. Benchmarking & Reproducibility
6. Performance Regression Guard (planned)
7. Memory Tracking & Probe Metrics
8. Commit & PR Guidelines

---
## 1. Getting Started
Requirements:
- CMake 3.16+
- C++17 compiler (GCC, Clang, or MSVC)
- Optional: Python 3 + `pip install -r requirements.txt` for plotting

Clone:
```bash
git clone https://github.com/aeml/hashbrowns.git
cd hashbrowns
```

## 2. Build & Test
Use the helper script (recommended):
```bash
scripts/build.sh -c -t Debug --test    # configure & build debug + run unit tests
scripts/build.sh -t Release --test     # release build + tests
```
Manual:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

## 3. Code Style & Formatting
A `.clang-format` file defines the style (LLVM base, 120 col limit, attached braces, aligned declarations). Before pushing:
```bash
cmake -S . -B build
cmake --build build --target check-format   # fails if style drift
cmake --build build --target format         # auto-format sources
```
If `clang-format` is missing the targets are skipped; please install it locally.

## 4. Adding Data Structures
1. Create header/source in `src/structures/` implementing the `DataStructure` interface from `core/data_structure.h`.
2. Add your sources to `STRUCTURES_SOURCES` and headers to `STRUCTURES_HEADERS` in `CMakeLists.txt`.
3. Extend CLI parsing in `src/main.cpp` to recognize your structure name.
4. Add unit tests (`tests/unit/test_<name>.cpp`). Include insert/search/remove basic coverage and complexity assertions if relevant.
5. Run `scripts/build.sh -t Debug --test` before committing.

### Complexity Reporting
Implement `insert_complexity()` and `search_complexity()` when meaningful (used only for banner display currently).

## 5. Benchmarking & Reproducibility
Key flags:
- `--pattern {sequential,random,mixed}`
- `--seed N` (always recorded; generated if absent)
- `--pin-cpu [IDX]` (Linux-only affinity)
- `--no-turbo` (Linux-only, best-effort governor tweak)
- `--warmup N` (discard outliers at start)
- `--bootstrap N` (compute mean CI via resampling)

For consistent comparisons:
1. Use a Release build.
2. Pin CPU & disable turbo if allowed.
3. Record JSON outputs; they include hardware snapshot and build meta.
4. Prefer median or CI bounds when variance is noticeable.

Series & crossover:
- Multi-size runs: `--series-count N` or explicit `--series-sizes 512,2048,8192`.
- Crossover analysis: `--crossover-analysis --max-size N --series-runs 1`.

### Recommended benchmark profiles

To keep runs consistent between contributors and CI, we use a small set of common CLI profiles:

- **P0 – Tiny sanity check (local):** quick smoke before committing
	```bash
	./build/hashbrowns --size 4096 --runs 5 \
			--structures array,slist,hashmap \
			--output results/csvs/benchmark_results.csv
	```

- **P1 – Small CI benchmark (authoritative):** what CI runs by default
	```bash
	./build/hashbrowns --size 20000 --runs 10 \
			--structures array,slist,hashmap \
			--pattern sequential --seed 12345 \
			--output results/csvs/benchmark_results.csv
	```

- **P2 – Medium series sweep:** a few sizes for shape of curves
	```bash
	./build/hashbrowns --size 60000 --series-count 6 --runs 3 \
			--structures array,hashmap --out-format json \
			--series-out results/csvs/series_results.json
	```

- **P3 – Full crossover analysis:** crossover points between structures
	```bash
	./build/hashbrowns --crossover-analysis --max-size 100000 --runs 4 \
			--structures array,slist,hashmap \
			--pattern sequential --seed 12345 \
			--out-format json --output results/csvs/crossover_results.json
	```

- **P4 – Deep analysis:** heavier run with memory tracking and CIs
	```bash
	./build/hashbrowns --size 50000 --runs 20 \
			--structures array,slist,dlist,hashmap \
			--pattern random --seed 12345 \
			--memory-tracking --bootstrap 400 \
			--out-format json --output results/csvs/benchmark_results_deep.json
	```

Contributor expectations:
- Before opening a PR:
	- Run unit tests (e.g. `scripts/build.sh -t Release --test`).
	- Run **P0** locally; if your change may affect performance, also run **P1**.
- For intentional performance changes:
	- Run **P1** and compare against the current baseline via `scripts/perf_guard.*`.
	- Optionally run **P2–P3** to understand impact across sizes.

## 6. Performance Regression Guard (Planned)
Upcoming: a script under `scripts/perf_guard.sh` will:
- Run a small fixed-seed benchmark subset.
- Compare key operations against a stored baseline with tolerances.
- Emit warning or fail on significant regressions.

Contributors changing hot paths should provide before/after JSON plus rationale.

## 7. Memory Tracking & Probe Metrics
Enable detailed memory deltas: `--memory-tracking`.
CSV columns:
- `memory_bytes` total peak usage per structure.
- `memory_<op>_mean/stddev` incremental allocation deltas per op.
HashMap probe columns (`insert_probes_mean`, etc.) reflect average slot probes (open addressing). High probe counts near capacity may indicate tuning need—adjust `--hash-load` or strategy.

## 8. Commit & PR Guidelines
- Keep commits scoped: docs, build, feature, refactor, perf, fix.
- Include context in commit messages (e.g. `perf: reduce HashMap probe count by early resize`).
- Run tests & check-format locally before pushing.
- For performance changes, attach benchmark JSON (before/after) in the PR.
- Avoid introducing external dependencies without prior discussion.

Thank you for contributing! Feel free to open issues for proposed features (percentile customization, raw sample export, etc.).
