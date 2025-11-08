---
title: Memory and Probe Metrics
description: Interpret memory_* and *_probes_* fields in hashbrowns outputs and learn tuning tips.
---

# Memory and Probe Metrics

This tutorial explains two sets of diagnostics provided by hashbrowns when you enable optional flags:

- memory_* metrics: per‑operation allocation deltas and final footprint
- *_probes_* metrics: HashMap probe counts for open addressing

Along the way we’ll show small runs and how to tune the HashMap.

## Enable metrics

- Memory tracking is off by default. Turn it on with:

```bash
./build/hashbrowns --size 20000 --runs 8 --memory-tracking --out-format json --output results/csvs/benchmark_results.json
```

- Probe metrics are collected automatically for the open‑addressing HashMap implementation. To ensure you measure it specifically:

```bash
./build/hashbrowns --structures hashmap --hash-strategy open --size 20000 --runs 8 --out-format csv --output results/csvs/benchmark_results.csv
```

Tip: Pair with `--seed 12345` for repeatability and consider `--bootstrap 400` for confidence intervals.

## Interpreting memory_* fields

When `--memory-tracking` is enabled, outputs include:

- memory_bytes — approximate live bytes retained by the structure at the end of the benchmark
- memory_insert_mean/stddev — average incremental allocation during inserts (bytes)
- memory_search_mean/stddev — typical zero for these structures; non‑zero may indicate incidental allocations
- memory_remove_mean/stddev — net allocation change during removals (often small or zero)

Common patterns:

- Dynamic Array: Larger memory_insert_mean suggests frequent capacity growth. Consider pre‑sizing or a different growth factor.
- Linked List: Insert deltas roughly equal to node size; remove may not show negative totals depending on normalization.
- HashMap (open): Spikes in insert means often align with rehash; tune capacity and load factor.

## Interpreting *_probes_* (HashMap)

For open addressing, average probe counts highlight table efficiency:

- insert_probes_mean/stddev
- search_probes_mean/stddev
- remove_probes_mean/stddev

Rules of thumb:

- With load < ~0.7, searches should average ~1–2 probes and inserts slightly higher.
- Means > ~5 indicate clustering or over‑filled tables.
- Large stddev implies unstable performance; address load factor or hashing strategy.

## Tuning the HashMap

Flags you can adjust:

- `--hash-load F` — target max load factor before growth (e.g., 0.60–0.75)
- `--hash-capacity N` — initial capacity (rounded to pow‑2 for open addressing)
- `--hash-strategy {open,chain}` — switch to separate chaining if distribution is skewed

Examples:

```bash
# Reduce load to limit clustering
./build/hashbrowns --structures hashmap --hash-strategy open --hash-load 0.6 --size 50000 --runs 6 --memory-tracking

# Pre‑size to avoid early rehash and smooth insert probes
./build/hashbrowns --structures hashmap --hash-strategy open --hash-capacity 65536 --size 50000 --runs 6 --memory-tracking

# Try chaining strategy (probe metrics will be zeros by design)
./build/hashbrowns --structures hashmap --hash-strategy chain --size 50000 --runs 6 --memory-tracking
```

## Quick checklist

- Seeing non‑zero search/remove memory means? Look for hidden allocations (e.g., temporary strings) and refactor.
- Probe means rising with size? Lower `--hash-load` or raise `--hash-capacity`.
- High variance? Increase `--runs`, enable `--bootstrap`, pin CPU, disable turbo on Linux.

## Example: reading JSON

You can quickly summarize the JSON with our helper script:

```bash
python3 scripts/example_parse.py results/csvs/benchmark_results.json --summary
```

This prints a compact table and a one‑line meta summary (seed, size, runs).

---
Last updated: generated at build time; see `--version` for commit.
