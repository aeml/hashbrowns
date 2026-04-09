# Performance Analysis

This note summarizes how hashbrowns collects timing data and how to interpret exported CSVs.

## Timing methodology

- High-resolution timing using `std::chrono::high_resolution_clock`
- Multiple runs per configuration (default 10) with reporting of mean and standard deviation
- Single-threaded execution to avoid scheduler noise
- Memory usage reported via structure-provided `memory_usage()` after inserts

CSV columns (benchmark mode):

- `structure` — array | slist | dlist | hashmap
- `insert_ms_mean`, `insert_ms_stddev`
- `search_ms_mean`, `search_ms_stddev`
- `remove_ms_mean`, `remove_ms_stddev`
- `memory_bytes`

## Crossover analysis

Crossover points are estimated by running a sweep of sizes and looking for a sign change in the difference between two series, with linear interpolation. Results are approximate and hardware-dependent.

CSV columns (crossover mode):

- `operation` — insert | search | remove
- `a`, `b` — structure names
- `size_at_crossover` — approximate size where performance crosses

## Reproducibility

- Use a Release build for stable numbers: `scripts/build.sh -t Release`
- Fix the CPU governor if possible and close background applications
- Run more repetitions for tighter confidence intervals (e.g., `--runs 30`)
- Treat baseline comparisons as valid only when workload-shaping metadata matches: size, runs, warmup, bootstrap iters, structures, pattern, seed, hash strategy/capacity/load, CPU pinning, and turbo state
- When exporting baseline reports, inspect `comparison.decision_basis`, `comparison.health`, `comparison.actionability`, `comparison.coverage`, `comparison.failures[]`, per-entry `*_basis` fields, and the exact `*_mean_delta_pct|*_p95_delta_pct|*_ci_high_delta_pct` fields so reviewers can see exactly which metric family drove the regression verdict, which structures were not actually compared, whether either input silently contained duplicate structure rows, and whether the result should drive automation directly
- When you care about canonical workflow drift, use `--baseline-strict-profile-intent` so profile manifest intent must match too
- Treat hardware/software environment fields (`cpu_model`, `compiler`, `kernel`, `cpu_governor`, RAM/core count) as warning signals, not proof of regression by themselves

## Baseline methodology

A baseline check is only trustworthy when it answers the same question twice.

The published machine-readable baseline policy lives in `docs/api/baseline_policy.json`.

hashbrowns now separates baseline metadata into two buckets:

- Hard comparison requirements: run shape and benchmark knobs that directly change the workload. If these drift, the comparison is rejected before timing deltas are interpreted.
- Soft environment warnings: machine/compiler context that can explain noise or drift. These do not fail the check by themselves, but they are printed so the result is not oversold.

Practical consequence:

- Changing `--pattern random` to `--pattern sequential`, dropping `--seed`, changing `--runs`, or retuning HashMap load factor is not a regression. It is a different experiment.
- Running the same workload on a different CPU or compiler can still be compared, but the tool warns that the environment moved and the claim should stay modest.

## Caveats

- Measurements are sensitive to CPU frequency scaling, cache effects, and compiler optimizations
- Linked-structure implementations can vary widely with allocator behavior
- Results on one machine are not necessarily predictive on another
