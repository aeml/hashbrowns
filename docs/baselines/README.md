# Baseline Benchmarks

This folder holds a `.gitkeep` placeholder. The authoritative performance
baseline is stored as JSON in `perf_baselines/baseline.json` and is managed
by `scripts/perf_guard.sh`.

## Canonical workflow — JSON baseline (recommended)

```bash
# Create or update the JSON baseline from the current Release build:
scripts/perf_guard.sh --update

# Compare current build against the stored baseline:
scripts/perf_guard.sh
```

The JSON baseline captures per-structure `insert_ms_mean`, `search_ms_mean`,
and `remove_ms_mean` for a fixed seed and size. Tolerances default to 20 %
and can be overridden with environment variables:

```bash
TOL_PCT_INSERT=10 TOL_PCT_SEARCH=10 TOL_PCT_REMOVE=10 scripts/perf_guard.sh
```

## Legacy CSV baseline (optional)

A CSV baseline can optionally be placed at `docs/baselines/benchmark_results.csv`
and is consumed by the `regression_check` binary and the `small-benchmarks` CI
job. To create one:

```bash
scripts/update_baseline.sh --size 2048 --runs 5 --structures array,slist,hashmap
```

If this file is absent, the CSV regression step is skipped. This path is
secondary; prefer the JSON baseline for day-to-day performance guarding.

## Notes

- Baselines are hardware-dependent. Keep sizes small and thresholds loose for CI.
- Update the baseline only when you intentionally accept a performance change.
- The `perf-guard` CI job gates merges only when a PR carries the `perf` label.
