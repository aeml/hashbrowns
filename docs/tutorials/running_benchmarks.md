# Running Benchmarks

This short guide shows how to build and run benchmarks and where to find outputs.

## Build

```bash
scripts/build.sh -c -t Release --test
```

## Single-size benchmark

```bash
./build/hashbrowns --profile ci
```

The CSV will contain mean and standard deviation for insert, search, and remove.

## Crossover analysis

```bash
./build/hashbrowns --profile crossover
```

The CSV will list approximate crossover sizes by operation.

## Benchmark profiles

Use `--profile {smoke,ci,series,crossover,deep}` to select a canonical workflow without retyping a pile of flags. The selected profile is written into JSON metadata, and `meta.profile_manifest` records which fields came from the profile versus explicit CLI overrides. Baseline comparison still treats the profile itself as a hard compatibility field.

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

What the checker now does before reading slowdown percentages:

- Fails the comparison if workload-shaping metadata changed: size, runs, warmup, bootstrap iters, structures, pattern, seed, hash strategy/capacity/load, CPU pinning, or turbo setting.
- With `--baseline-strict-profile-intent`, also fails if `profile_manifest` intent drifted even when the coarse profile name still matches.
- Warns if environment context changed: CPU model, compiler, kernel, governor, RAM, or core count.

That split matters. A different seedless workload or different hash-map tuning is not a perf regression. It is a different experiment.

On regression beyond the threshold, the process exits non-zero, which is ideal for CI. If you want structured automation output instead of scraping logs, add `--baseline-report-json build/baseline_report.json`.

## Quick helper

```bash
scripts/run_benchmarks.sh --runs 10 --size 10000
```

This script builds the project if needed and writes CSVs to `results/csvs/`.
