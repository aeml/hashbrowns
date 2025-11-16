---
title: Quickstart
description: Fast intro to running hashbrowns benchmarks (single-size, series, crossover) and parsing JSON output.
---

# Quickstart

This guide walks through the three common workflows:

1. Single-size benchmark (default mode)
2. Multi-size series sweep
3. Crossover analysis (finding size thresholds where structures change relative performance)
4. Parsing JSON output with a small Python helper

All commands assume you have run a build already (e.g. `scripts/build.sh -t Release --test`).

> Tip: Add `--seed 12345` to any random or mixed pattern runs for reproducibility.

## 1. Single-size benchmark

Run the executable directly specifying a size and number of runs:

```bash
./build/hashbrowns --size 10000 --runs 5 --structures array,slist,hashmap --memory-tracking --bootstrap 200 --out-format json --output benchmark_single.json
```

You'll see a colorful banner followed by benchmark progress:

```
🥔 hashbrowns - C++ Data Structure Benchmarking Suite
======================================================

=== Benchmark Results (avg ms over 5 runs, size=10000) ===
- array: insert=712.99, search=16337.3, remove=199231, mem=655416 bytes
- slist: insert=336.687, search=57940.9, remove=79.0626, mem=480104 bytes
- hashmap: insert=1099.77, search=198.998, remove=142.816, mem=786664 bytes

Saved JSON to: benchmark_single.json
```

Key points:
- `--structures` picks the set; omit it to use all defaults (array, slist, dlist, hashmap).
- `--out-format csv` would emit a CSV instead.
- `--memory-tracking` includes allocation deltas (the `mem=` values shown).
- `--bootstrap 200` computes 95% confidence intervals for statistical rigor.

Minimal CSV example:
```bash
./build/hashbrowns --size 10000 --runs 8 --output results/csvs/benchmark_results.csv
```

## 2. Multi-size series

Use a linear series of sizes up to a max (given by `--size`) with `--series-count`:

```bash
./build/hashbrowns --size 60000 --series-count 6 --runs 3 \
  --structures array,hashmap --out-format json --series-out results/csvs/series_results.json
```

This produces 6 evenly spaced sizes between a small floor and 60,000. For explicit sizes:

```bash
./build/hashbrowns --series-sizes 1024,4096,8192,16384,32768,65536 --series-runs 2 --out-format csv \
  --series-out results/csvs/series_results.csv
```

JSON series output contains an array of per-size measurements plus a meta block including `runs_per_size` and `seed`.

Wizard alternative (interactive prompts):
```bash
./build/hashbrowns --wizard
```

## 3. Crossover analysis

Estimate sizes where one structure overtakes another for each operation:

```bash
./build/hashbrowns --crossover-analysis --max-size 100000 --runs 4 \
  --structures array,slist,hashmap --out-format json --output results/csvs/crossover_results.json
```

Optional time budget:
```bash
./build/hashbrowns --crossover-analysis --max-size 200000 --max-seconds 15 --runs 2 \
  --structures array,hashmap --out-format csv --output results/csvs/crossover_results.csv
```

Each crossover row captures an operation (`insert|search|remove`), structure pair (`a,b`), and estimated size at crossover. Use JSON for richer meta when publishing.

## 4. Parsing JSON output (Python example)

We provide an example parser at `scripts/example_parse.py`:

```bash
python3 scripts/example_parse.py results/csvs/benchmark_results.json
```

Sample output:
```
Structure  Insert(ms)  Search(ms)  Remove(ms)  Memory(bytes)
array      12.34       4.11        8.22        65536
hashmap    7.91        5.02        6.10        98304
```

Add `--summary` to reduce output to a one-line aggregate or `--csv` to emit a condensed table for piping.

### Schema awareness

All JSON blobs contain `schema_version`. The parser checks this first. If you upgrade the schema, adjust the script or add a validation step (`scripts/validate_json_schema.py`, planned).

### Reproducibility checklist

For comparing benchmark runs over time:
- Pin the CPU or disable turbo on Linux (`--pin-cpu --no-turbo`).
- Fix random seed: `--seed 12345`.
- Record branch / commit (`--version` flag is script friendly).
- Include bootstrap CIs for statistical confidence.
- Document environment meta (already embedded in JSON).

## Troubleshooting

| Symptom | Suggestion |
|---------|------------|
| Very high variance | Increase `--runs`, enable `--bootstrap`, pin CPU, close background apps |
| HashMap probes > 5 | Reduce `--hash-load`, increase `--hash-capacity`, try chaining strategy |
| Long array remove times | Reduce max size or omit array from large sweeps |
| CSV missing memory columns | Add `--memory-tracking` |

## Next steps

Explore deeper memory & probe interpretation in `memory_and_probes.md` (to be added), or build plots with:

```bash
scripts/run_benchmarks.sh --runs 8 --size 50000 --max-size 65536 --plots --yscale auto
```

---

Check your version:
```bash
./build/hashbrowns --version
# Output: hashbrowns 1.0.0 (git b792a4354ada)
```

Last updated: November 2025
