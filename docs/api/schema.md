# Hashbrowns JSON Schema (v1)

This document describes the JSON structures emitted by hashbrowns. A minimal `schema_version` integer is included in each meta block for forward compatibility.

## Common
- `meta.schema_version` (int): Schema version (current: 1)
- `meta.structures` (string[]): Names of structures included
- `meta.pattern` (string): `sequential|random|mixed`
- `meta.seed` (uint64, optional): RNG seed if provided
- Environment snapshot fields (when present): `timestamp`, `cpu_governor`, `git_commit`, `compiler`, `cpp_standard`, `build_type`, `cpu_model`, `cores`, `total_ram_bytes`, `kernel`, and HashMap tuning `hash_strategy`, `hash_capacity?`, `hash_load?`, reproducibility `pinned_cpu`, `turbo_disabled`.

## benchmark_results.json
- `meta.size` (int): Problem size
- `meta.runs` (int): Repetitions per structure
- `meta.warmup_runs` (int)
- `meta.bootstrap_iters` (int)
- `results` (array of objects): One per structure with timing and memory fields
  - `<op>_ms_mean|stddev|median|p95`
  - `<op>_ci_low|ci_high` (present if bootstrap enabled, else zeros)
  - `memory_bytes` (size at end)
  - `memory_<op>_mean|stddev`
  - `insert_probes_mean|stddev`, `search_probes_*`, `remove_probes_*` (HashMap-specific)

## crossover_results.json
- `meta.runs` (int)
- `crossovers` (array): entries with `operation`, `a`, `b`, `size_at_crossover`

## series_results.json
- `meta.runs_per_size` (int)
- `series` (array): entries with `size`, `structure`, `insert_ms`, `search_ms`, `remove_ms`

## Compatibility Notes
- Consumers should verify `meta.schema_version` and tolerate additional fields.
- Newer versions may add keys but won’t remove or change types without bumping the schema version.
