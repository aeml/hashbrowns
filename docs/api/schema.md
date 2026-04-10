# Hashbrowns JSON Schema (v1)

This document describes the JSON structures emitted by hashbrowns. A minimal `schema_version` integer is included in each meta block for forward compatibility.

## Common
- `meta.schema_version` (int): Schema version (current: 1)
- `meta.structures` (string[]): Names of structures included
- `meta.pattern` (string): `sequential|random|mixed`
- `meta.seed` (uint64, optional): RNG seed if provided
- Environment snapshot fields (when present): `timestamp`, `cpu_governor`, `git_commit`, `compiler`, `cpp_standard`, `build_type`, `cpu_model`, `profile`, `cores`, `total_ram_bytes`, `kernel`, and HashMap tuning `hash_strategy`, `hash_capacity?`, `hash_load?`, reproducibility `pinned_cpu`, `turbo_disabled`.
- `meta.profile_manifest` (object): resolved profile intent emitted for automation and auditability.
  - `selected_profile` (string): canonical profile name or `custom`
  - `applied_defaults` (string[]): fields the selected profile supplied because the caller left them at profile-default detection values
  - `explicit_overrides` (string[]): fields the caller set explicitly, diverging from the bare named profile
- Baseline comparisons treat `size`, `runs`, `warmup_runs`, `bootstrap_iters`, `profile`, `structures`, `pattern`, `seed`, `hash_strategy`, `hash_capacity`, `hash_load`, `pinned_cpu`, and `turbo_disabled` as hard compatibility requirements.
- Baseline comparisons treat `cpu_model`, `compiler`, `cpp_standard`, `cpu_governor`, `cores`, `total_ram_bytes`, and `kernel` as warning-only context fields.

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

## baseline_report.json
- Top-level report emitted by `--baseline-report-json`
- `baseline_path` (string): path of the baseline file used for comparison
- `scope` (string): timing scope used for evaluation: `mean|p95|ci_high|any`
- `threshold_pct` / `noise_floor_pct` (number): comparison policy used for this run
- `strict_profile_intent` (bool): whether profile-manifest intent matching was enforced
- `exit_code` (int): final baseline classification (`0` success, `2` per-operation guard failure, `4` binary regression, `5` metadata mismatch)
- `metadata` (object): machine-readable copy of compatibility findings
  - `ok` (bool)
  - `errors` (string[])
  - `warnings` (string[])
- `comparison` (object): machine-readable copy of the binary's coarse per-structure timing deltas
  - `all_ok` (bool)
  - `decision_basis` (string): scope semantics that drove the binary comparison verdict: `mean|p95|ci_high|any`
  - `health` (string): compact summary of comparison hygiene: `clean|partial_coverage|duplicate_inputs|partial_coverage_with_duplicates`
  - `actionability` (string): whether the comparison should drive decisions directly: `fully_actionable|actionable_with_hygiene_warnings|not_actionable`
  - `recommended_disposition` (string): policy-facing recommendation derived from actionability: `accept|review_with_warnings|reject_input_hygiene`
  - `disposition_reasons[]` (array): compact reason codes for the recommendation: `missing_structures|duplicate_baseline_structures|duplicate_current_structures`
  - `summary` (object): compact numeric hygiene rollup for dashboards/alerts
    - `missing_structure_count`
    - `duplicate_baseline_structure_count`
    - `duplicate_current_structure_count`
  - `has_hygiene_issues` (bool): fast CI/dashboard gate for whether the comparison surface is clean
  - `hygiene_issue_count` (int): compact count of triggered hygiene issue categories
  - `hygiene_gate` (string): compact CI-facing hygiene state: `clean|warn|block`
  - `perf_signal_strength` (string): how much benchmarking evidence is actually comparable: `none|limited|strong`
  - `coverage` (object): honest accounting of what was actually compared and whether either side was malformed
    - `baseline_structure_count`
    - `current_structure_count`
    - `comparable_structure_count`
    - `baseline_only_structures[]`
    - `current_only_structures[]`
    - `duplicate_baseline_structures[]`
    - `duplicate_current_structures[]`
  - `failures[]` with `structure`, `operation`, `chosen_basis`, `chosen_delta_pct`, `threshold_pct`, and `failed_metric_families[]` so dashboards can jump straight to what actually failed
  - `entries[]` with `structure`, `insert_delta_pct`, `search_delta_pct`, `remove_delta_pct`, `insert_ok`, `search_ok`, `remove_ok`, plus per-operation `insert_basis|search_basis|remove_basis`
  - each entry also carries exact metric-family details for insert/search/remove: `*_mean_delta_pct`, `*_p95_delta_pct`, `*_ci_high_delta_pct`, and matching `*_mean_ok`, `*_p95_ok`, `*_ci_high_ok`
  - for `scope=any`, each `*_basis` field explains which metric checks passed or failed instead of pretending a single scalar told the whole story
- `per_operation_guard` (object): final perf-guard verdict after exact insert/search/remove-specific tolerances are applied
  - `ok` (bool)
  - `tolerances_pct` with `insert`, `search`, `remove`
  - `entries[]` with `structure`, and nested `insert|search|remove` objects carrying `baseline_value`, `current_value`, `delta_pct`, `threshold_pct`, `ok`
  - `failures[]` with human-readable failure strings matching the script verdict

## Compatibility Notes
- Benchmark/series/crossover consumers should verify `meta.schema_version` and tolerate additional fields.
- Baseline report consumers should treat `exit_code` plus `metadata.ok`/`comparison.all_ok`/`per_operation_guard.ok` as the truthful classification surface instead of scraping console logs.
- Newer versions may add keys but won’t remove or change types without bumping the schema version.
- `scripts/test_validate_json.py` provides focused regression coverage for schema detection and rejection behavior across benchmark, series, crossover, baseline-report, profile-contract, perf-guard-contract, and baseline-policy artifacts.
- `docs/api/profiles.json` is the machine-readable contract for canonical named profiles; validate it against `docs/api/schemas/profiles.schema.json` and runtime behavior with `scripts/test_profile_contract.py`.
- `docs/api/perf_guard_contract.json` is the machine-readable contract for the canonical perf guard workflow; validate it against `docs/api/schemas/perf_guard_contract.schema.json` and runtime behavior with `scripts/test_perf_guard_contract.py`.
- `docs/api/baseline_policy.json` is the machine-readable contract for baseline compatibility policy; validate it against `docs/api/schemas/baseline_policy.schema.json` and keep it aligned with metadata guardrail behavior.
