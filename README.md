# hashbrowns

[![CI](https://github.com/aeml/hashbrowns/actions/workflows/ci.yml/badge.svg)](https://github.com/aeml/hashbrowns/actions/workflows/ci.yml)
[![Coverage](https://img.shields.io/endpoint?url=https://aeml.github.io/hashbrowns/coverage/coverage_badge.json)](https://aeml.github.io/hashbrowns/coverage/)

`hashbrowns` is a C++ benchmarking project for implementing a few core data structures and measuring how their performance changes under different workloads.

It is aimed at performance-oriented engineering work: building the structures directly, instrumenting them, exporting reproducible results, and comparing tradeoffs instead of relying on intuition.

## Overview

This repository combines custom C++ data-structure implementations with a small benchmarking harness and analysis tooling.

The current project includes:
- a custom dynamic array
- singly and doubly linked lists
- a hash map with open addressing and separate chaining modes
- repeated-run benchmarks with CSV and JSON export
- crossover analysis and baseline regression checks for repeatable performance comparisons

## What this demonstrates

- Data structures: direct implementations of arrays, linked lists, and hash tables rather than wrappers around STL containers
- Performance tradeoffs: different structures win on different operations, sizes, and access patterns
- Benchmarking discipline: repeated runs, summary statistics, JSON/CSV artifacts, fixed seeds, named profiles, and baseline comparison tooling
- Memory and layout considerations: contiguous storage vs pointer-heavy nodes, tracked allocations, memory pools, load factor tuning, tombstones, and probe counts
- Systems-level programming concepts: manual allocation paths, allocator-aware code, cache-sensitive layout choices, reproducibility controls, and CI-friendly regression guards

## Data structures / algorithms explored

- `DynamicArray`
  - contiguous storage
  - configurable growth strategies: `2.0x`, `1.5x`, `fibonacci`, and `additive`
  - useful for showing amortized growth behavior and memory/capacity tradeoffs
- `SinglyLinkedList` and `DoublyLinkedList`
  - node-based structures backed by a memory pool
  - useful for contrasting pointer chasing, node overhead, and removal behavior against contiguous storage
- `HashMap`
  - open addressing with linear probing and tombstones
  - separate chaining as an alternate collision strategy
  - configurable initial capacity and load factor
- Benchmark analysis
  - repeated-run timing summaries
  - mean, standard deviation, median, p95, and optional bootstrap confidence intervals
  - crossover detection across size sweeps
  - baseline comparison for regression checks

## Benchmarking

Checked-in benchmark artifacts already exist in the repo, including `benchmark_results.csv`, `series_results.csv`, and `benchmark_single.json`.

One saved single-size run in `benchmark_single.json` was recorded with:
- size `10000`
- `5` runs
- sequential pattern
- open-addressing hash map
- Release build
- GCC `15.2.1`
- AMD Ryzen 9 4900HS

That run shows the following sample results:
- `hashmap` had the lowest search mean at `198.998 ms`
- `slist` and `dlist` had the lowest remove means at `79.0626 ms` and `80.2504 ms`
- `dlist` had the lowest insert mean at `298.357 ms`
- reported memory usage ranged from `480104` bytes for `slist` to `786664` bytes for `hashmap`

These are checked-in sample measurements, not universal claims. Results are hardware-, compiler-, and workload-dependent.

The repo also includes real README visuals in `docs/media/benchmark-chart.svg` and `docs/media/results-output.svg` generated from benchmark output.

## Tech stack

- C++17
- CMake
- custom benchmark runner and CLI
- Python scripts for validation, analysis, plotting, and report generation
- shell and PowerShell helper scripts
- unit tests with CTest integration

## Build/run instructions

Requirements:
- CMake `3.16+`
- a C++17 compiler
- Python 3 for the analysis scripts

Recommended build:

```bash
scripts/build.sh -c -t Release --test
```

Manual build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Run a benchmark profile:

```bash
./build/hashbrowns --profile ci
```

Run a custom benchmark and export JSON:

```bash
./build/hashbrowns \
  --size 10000 \
  --runs 5 \
  --structures array,slist,dlist,hashmap \
  --pattern sequential \
  --out-format json \
  --output build/benchmark_results.json
```

## Reproducing benchmarks

Use a Release build and keep the run shape fixed when comparing results.

Quick helper script:

```bash
scripts/run_benchmarks.sh --runs 10 --size 10000
```

Recommended reproducibility practices already supported by the repo:
- use `--profile` for canonical benchmark shapes
- use `--seed` when running random or mixed patterns
- use `--pin-cpu` when you want tighter run-to-run control
- keep hash strategy, capacity, and load factor fixed across comparisons
- compare against a saved baseline JSON instead of ad hoc console output

For baseline regression checks:

```bash
./build/hashbrowns \
  --size 20000 \
  --runs 5 \
  --structures array,slist,dlist,hashmap \
  --pattern sequential \
  --seed 12345 \
  --out-format json \
  --output build/benchmark_results.json \
  --baseline perf_baselines/baseline.json
```

For deeper analysis and plots, see:
- `docs/tutorials/running_benchmarks.md`
- `docs/performance_analysis.md`

## Project status

Active and already useful as a portfolio project for data-structure implementation and performance measurement.

Current strengths:
- implemented structures and tests
- reproducible benchmark/export flow
- regression/baseline tooling
- checked-in sample benchmark artifacts and README visuals

Still worth improving:
- broader benchmark coverage across more workloads and machine configurations
- more side-by-side plotted summaries for the saved benchmark series
- tighter documentation around interpreting crossover and baseline outputs
