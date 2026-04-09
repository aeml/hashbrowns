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

The JSON baseline captures both timing summaries and benchmark metadata for a fixed workload. By default the guard preserves the canonical `ci` profile defaults; setting `SIZE`, `RUNS`, `SEED`, or `STRUCTURES` turns those into explicit overrides. Tolerances default to 20 % and can be overridden with environment variables:

```bash
TOL_PCT_INSERT=10 TOL_PCT_SEARCH=10 TOL_PCT_REMOVE=10 scripts/perf_guard.sh
```

Metadata policy:
- Hard fail if benchmark-shaping inputs changed: size, runs, warmup, bootstrap iters, structures, pattern, seed, hash strategy/capacity/load, CPU pinning, or turbo state.
- Optional stricter fail mode with `--baseline-strict-profile-intent`: if `profile_manifest` exists, selected profile and manifest override/default intent must match too.
- Warning only if machine/compiler context changed: CPU model, compiler, governor, kernel, RAM, or core count.
- The machine-readable version of that policy lives in `docs/api/baseline_policy.json`.

That keeps the guard honest: it refuses apples-to-oranges comparisons before reporting slowdown percentages.

The coarse baseline report now also records which comparison basis drove the decision. For `mean`, `p95`, or `ci_high`, each entry records that exact basis. For `any`, each operation records an `any(...)` explanation so automation and reviewers can see whether mean, tail, or CI-high checks carried the verdict.

The canonical guard scripts now write `build/perf_guard_report.json` automatically so CI or dashboards can consume a structured outcome instead of scraping console text. That report now reflects both the binary's coarse comparison and the script's final per-operation insert/search/remove tolerances, with the merge logic maintained in `scripts/perf_guard_report.py` instead of an inline shell blob. The helper has focused regression coverage in `scripts/test_perf_guard_report.py`, wrapper-level manifest honesty is pinned by `scripts/test_perf_guard_wrapper_manifest.py`, and the overall wrapper/report contract is published in `docs/api/perf_guard_contract.json` with runtime checks in `scripts/test_perf_guard_contract.py`.

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
- The checked-in `perf_baselines/baseline.json` should be regenerated with the current canonical `scripts/perf_guard.sh --update` workflow so its metadata matches the `ci` profile and emitted profile manifest. In the current design, the fixed output path and forced JSON artifact format show up as explicit `output` and `out_format` overrides, which is acceptable because the workload shape remains canonical.
- Update the baseline only when you intentionally accept a performance change, and only after `scripts/perf_guard.sh` passes against the refreshed artifact and `build/perf_guard_report.json` validates.
- The `perf-guard` CI job gates merges only when a PR carries the `perf` label.
