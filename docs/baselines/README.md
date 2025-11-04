# Baseline Benchmarks

This folder can store a small baseline CSV used for optional regression checks in CI.

- Expected filename: `benchmark_results.csv`
- Format: CSV produced by `./build/hashbrowns --size N --runs R --structures ... --output <path>`
- CI behavior: If this file exists in the repository, CI will run a quick benchmark and compare it to this baseline using `regression_check` with a 10% threshold.

## Creating or updating the baseline

Use the helper script to generate a baseline in Release mode with modest settings:

```bash
scripts/update_baseline.sh --size 2048 --runs 5 --structures array,slist,hashmap
```

The script writes `docs/baselines/benchmark_results.csv`.

Notes:
- Baselines are hardware-dependent. Keep sizes small and stable.
- Update the baseline only when you intentionally accept performance changes.
