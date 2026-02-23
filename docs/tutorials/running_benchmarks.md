# Running Benchmarks

This short guide shows how to build and run benchmarks and where to find outputs.

## Build

```bash
scripts/build.sh -c -t Release --test
```

## Single-size benchmark

```bash
./build/hashbrowns --size 50000 --runs 20 --structures array,slist,hashmap --output results/csvs/benchmark_results.csv
```

The CSV will contain mean and standard deviation for insert, search, and remove.

## Crossover analysis

```bash
./build/hashbrowns --crossover-analysis --max-size 100000 --runs 5 --structures array,slist,hashmap --output results/csvs/crossover_results.csv
```

The CSV will list approximate crossover sizes by operation.

## Baseline comparison

To guard against regressions, first record a baseline JSON:

```bash
./build/hashbrowns --size 20000 --runs 5 --structures array,slist,dlist,hashmap \ 
	--pattern sequential --seed 12345 --out-format json --output perf_baselines/baseline.json
```

Then compare new runs against it:

```bash
./build/hashbrowns --size 20000 --runs 5 --structures array,slist,dlist,hashmap \ 
	--pattern sequential --seed 12345 --out-format json --output build/benchmark_results.json \ 
	--baseline perf_baselines/baseline.json --baseline-threshold 20 --baseline-noise 1
```

On regression beyond the threshold, the process exits non-zero, which is ideal for CI.

## Quick helper

```bash
scripts/run_benchmarks.sh --runs 10 --size 10000
```

This script builds the project if needed and writes CSVs to `results/csvs/`.
