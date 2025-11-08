\page memory_probes_tutorial Memory & Probes

Learn how to interpret `memory_*` allocation metrics and `*_probes_*` (HashMap open addressing) and how to tune:

- `--memory-tracking` to collect allocation deltas
- `--hash-load` and `--hash-capacity` to control clustering and rehash behavior

Examples:
```bash
# Memory tracking
./build/hashbrowns --size 20000 --runs 8 --memory-tracking --out-format json \
  --output results/csvs/benchmark_results.json

# HashMap tuning
./build/hashbrowns --structures hashmap --hash-strategy open --hash-load 0.6 \
  --size 50000 --runs 6 --memory-tracking
```

Probe counts should stay near 1–2 for searches under moderate load (< 0.7). Values above ~5 suggest reducing load factor or increasing capacity.
