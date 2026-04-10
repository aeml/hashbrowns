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

Use `--profile {smoke,ci,series,crossover,deep}` to select a canonical workflow without retyping a pile of flags. The selected profile is written into JSON metadata, and `meta.profile_manifest` records which fields came from the profile versus explicit CLI overrides. Baseline comparison still treats the profile itself as a hard compatibility field. The current canonical contract is published in `docs/api/profiles.json`.

Validate that contract with:
```bash
python3 scripts/validate_json.py docs/api/profiles.json
python3 scripts/test_profile_contract.py
```

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

On regression beyond the threshold, the process exits non-zero, which is ideal for CI. The canonical `scripts/perf_guard.*` wrappers now write `build/perf_guard_report.json` automatically so automation gets structured output by default.

## Build a compact markdown report from JSON artifacts

When you have JSON outputs from benchmark, series, crossover, or baseline comparison flows, build one reviewer-facing summary instead of manually diffing raw objects:

```bash
python3 scripts/build_report.py \
  --benchmark-json build/benchmark_results.json \
  --series-json build/series_results.json \
  --crossover-json build/crossover_results.json \
  --baseline-report-json build/perf_guard_report.json \
  --output build/benchmark_report.md
```

The generated markdown intentionally stays honest. It summarizes run identity, profile-manifest intent, fastest mean timings, crossover hints, baseline hygiene/evidence state, coverage gaps, compact per-structure coarse deltas, structured coarse failures, and exact guard failures without pretending the report proved more than the artifacts actually support.

## Quick helper

```bash
scripts/run_benchmarks.sh --runs 10 --size 10000
```

This script builds the project if needed and writes CSVs to `results/csvs/`.
