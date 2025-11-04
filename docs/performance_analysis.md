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

## Caveats

- Measurements are sensitive to CPU frequency scaling, cache effects, and compiler optimizations
- Linked-structure implementations can vary widely with allocator behavior
- Results on one machine are not necessarily predictive on another
