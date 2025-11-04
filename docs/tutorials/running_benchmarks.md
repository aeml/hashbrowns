# Running Benchmarks

This short guide shows how to build and run benchmarks and where to find outputs.

## Build

```bash
scripts/build.sh -c -t Release --test
```

## Single-size benchmark

```bash
./build/hashbrowns --size 50000 --runs 20 --structures array,slist,hashmap --output build/benchmark_results.csv
```

The CSV will contain mean and standard deviation for insert, search, and remove.

## Crossover analysis

```bash
./build/hashbrowns --crossover-analysis --max-size 100000 --runs 5 --structures array,slist,hashmap --output build/crossover_results.csv
```

The CSV will list approximate crossover sizes by operation.

## Quick helper

```bash
scripts/run_benchmarks.sh --runs 10 --size 10000
```

This script builds the project if needed and writes CSVs to `build/`.
