#!/usr/bin/env python3
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VALIDATOR = ROOT / 'scripts' / 'validate_json.py'
SCHEMA_DIR = ROOT / 'docs' / 'api' / 'schemas'


def write_json(path: Path, obj):
    path.write_text(json.dumps(obj, indent=2) + '\n', encoding='utf-8')


def run_validator(path: Path):
    return subprocess.run(
        [sys.executable, str(VALIDATOR), '--schema-dir', str(SCHEMA_DIR), str(path)],
        capture_output=True,
        text=True,
    )


def test_detects_benchmark_results_schema():
    with tempfile.TemporaryDirectory() as td:
        path = Path(td) / 'benchmark.json'
        write_json(path, {
            'meta': {
                'schema_version': 1,
                'size': 1,
                'runs': 1,
                'warmup_runs': 0,
                'bootstrap_iters': 0,
                'structures': ['array'],
                'pattern': 'sequential',
                'seed': 1,
                'timestamp': '2026-01-01T00:00:00Z',
                'cpu_governor': 'performance',
                'git_commit': 'abc',
                'compiler': 'GCC',
                'cpp_standard': 'C++17',
                'build_type': 'Release',
                'cpu_model': 'CPU',
                'profile': 'ci',
                'cores': 1,
                'total_ram_bytes': 1,
                'kernel': 'Linux',
                'profile_manifest': {
                    'selected_profile': 'ci',
                    'applied_defaults': ['size'],
                    'explicit_overrides': []
                },
                'hash_strategy': 'open',
                'pinned_cpu': -1,
                'turbo_disabled': 0
            },
            'results': [{
                'structure': 'array',
                'insert_ms_mean': 1.0, 'insert_ms_stddev': 0.0, 'insert_ms_median': 1.0, 'insert_ms_p95': 1.0, 'insert_ci_low': 1.0, 'insert_ci_high': 1.0,
                'search_ms_mean': 1.0, 'search_ms_stddev': 0.0, 'search_ms_median': 1.0, 'search_ms_p95': 1.0, 'search_ci_low': 1.0, 'search_ci_high': 1.0,
                'remove_ms_mean': 1.0, 'remove_ms_stddev': 0.0, 'remove_ms_median': 1.0, 'remove_ms_p95': 1.0, 'remove_ci_low': 1.0, 'remove_ci_high': 1.0,
                'memory_bytes': 0,
                'memory_insert_mean': 0.0, 'memory_insert_stddev': 0.0,
                'memory_search_mean': 0.0, 'memory_search_stddev': 0.0,
                'memory_remove_mean': 0.0, 'memory_remove_stddev': 0.0
            }]
        })
        proc = run_validator(path)
        assert proc.returncode == 0, proc.stderr + proc.stdout
        assert 'benchmark_results.schema.json' in proc.stdout, proc.stdout


def test_detects_series_schema():
    with tempfile.TemporaryDirectory() as td:
        path = Path(td) / 'series.json'
        write_json(path, {
            'meta': {
                'schema_version': 1,
                'size': 16,
                'runs_per_size': 1,
                'structures': ['array'],
                'pattern': 'sequential',
                'timestamp': '2026-01-01T00:00:00Z',
                'git_commit': 'abc',
                'compiler': 'GCC',
                'cpp_standard': 'C++17',
                'build_type': 'Release',
                'cpu_model': 'CPU',
                'profile': 'series',
                'cores': 1,
                'total_ram_bytes': 1,
                'kernel': 'Linux',
                'profile_manifest': {'selected_profile': 'series', 'applied_defaults': [], 'explicit_overrides': []}
            },
            'series': [{'size': 16, 'structure': 'array', 'insert_ms': 1.0, 'search_ms': 1.0, 'remove_ms': 1.0}]
        })
        proc = run_validator(path)
        assert proc.returncode == 0, proc.stderr + proc.stdout
        assert 'series_results.schema.json' in proc.stdout, proc.stdout


def test_detects_crossover_schema():
    with tempfile.TemporaryDirectory() as td:
        path = Path(td) / 'crossover.json'
        write_json(path, {
            'meta': {
                'schema_version': 1,
                'size': 16,
                'runs': 1,
                'structures': ['array', 'hashmap'],
                'pattern': 'sequential',
                'timestamp': '2026-01-01T00:00:00Z',
                'git_commit': 'abc',
                'compiler': 'GCC',
                'cpp_standard': 'C++17',
                'build_type': 'Release',
                'cpu_model': 'CPU',
                'profile': 'crossover',
                'cores': 1,
                'total_ram_bytes': 1,
                'kernel': 'Linux',
                'profile_manifest': {'selected_profile': 'crossover', 'applied_defaults': [], 'explicit_overrides': []}
            },
            'crossovers': [{'operation': 'insert', 'a': 'array', 'b': 'hashmap', 'size_at_crossover': 1024}]
        })
        proc = run_validator(path)
        assert proc.returncode == 0, proc.stderr + proc.stdout
        assert 'crossover_results.schema.json' in proc.stdout, proc.stdout


def test_detects_baseline_report_schema():
    with tempfile.TemporaryDirectory() as td:
        path = Path(td) / 'baseline_report.json'
        write_json(path, {
            'baseline_path': 'perf_baselines/baseline.json',
            'scope': 'mean',
            'threshold_pct': 20,
            'noise_floor_pct': 1,
            'strict_profile_intent': False,
            'exit_code': 0,
            'metadata': {'ok': True, 'errors': [], 'warnings': []},
            'comparison': {
                'all_ok': True,
                'decision_basis': 'mean',
                'health': 'clean',
                'actionability': 'fully_actionable',
                'recommended_disposition': 'accept',
                'disposition_reasons': [],
                'summary': {
                    'missing_structure_count': 0,
                    'duplicate_baseline_structure_count': 0,
                    'duplicate_current_structure_count': 0,
                },
                'has_hygiene_issues': False,
                'hygiene_issue_count': 0,
                'hygiene_gate': 'clean',
                'coverage': {
                    'baseline_structure_count': 1,
                    'current_structure_count': 1,
                    'comparable_structure_count': 1,
                    'baseline_only_structures': [],
                    'current_only_structures': [],
                    'duplicate_baseline_structures': [],
                    'duplicate_current_structures': [],
                },
                'failures': [],
                'entries': []
            },
            'per_operation_guard': {
                'ok': True,
                'tolerances_pct': {'insert': 20, 'search': 20, 'remove': 20},
                'entries': [],
                'failures': []
            }
        })
        proc = run_validator(path)
        assert proc.returncode == 0, proc.stderr + proc.stdout
        assert 'baseline_report.schema.json' in proc.stdout, proc.stdout


def test_detects_profile_contract_schema():
    with tempfile.TemporaryDirectory() as td:
        path = Path(td) / 'profiles.json'
        write_json(path, {
            'schema_version': 1,
            'profiles': {
                'smoke': {'description': 'smoke', 'artifact_kind': 'benchmark_results', 'applied_defaults': ['size'], 'expected_explicit_overrides': ['output']},
                'ci': {'description': 'ci', 'artifact_kind': 'benchmark_results', 'applied_defaults': ['size'], 'expected_explicit_overrides': ['output']},
                'series': {'description': 'series', 'artifact_kind': 'series_results', 'applied_defaults': ['size'], 'expected_explicit_overrides': ['series_out']},
                'crossover': {'description': 'crossover', 'artifact_kind': 'crossover_results', 'applied_defaults': ['runs'], 'expected_explicit_overrides': ['output']},
                'deep': {'description': 'deep', 'artifact_kind': 'benchmark_results', 'applied_defaults': ['bootstrap'], 'expected_explicit_overrides': ['output']}
            }
        })
        proc = run_validator(path)
        assert proc.returncode == 0, proc.stderr + proc.stdout
        assert 'profiles.schema.json' in proc.stdout, proc.stdout


def test_detects_perf_guard_contract_schema():
    with tempfile.TemporaryDirectory() as td:
        path = Path(td) / 'perf_guard_contract.json'
        write_json(path, {
            'schema_version': 1,
            'wrapper': {
                'script': 'scripts/perf_guard.sh',
                'profile': 'ci',
                'artifact_paths': {
                    'baseline': 'perf_baselines/baseline.json',
                    'current': 'build/perf_guard_current.json',
                    'report': 'build/perf_guard_report.json'
                },
                'wrapper_explicit_overrides': ['output', 'out_format']
            },
            'report': {
                'schema': 'docs/api/schemas/baseline_report.schema.json',
                'exit_codes': {
                    'success': 0,
                    'per_operation_guard_failure': 2,
                    'binary_regression': 4,
                    'metadata_mismatch': 5
                },
                'per_operation_guard_tolerance_keys': ['insert', 'search', 'remove']
            }
        })
        proc = run_validator(path)
        assert proc.returncode == 0, proc.stderr + proc.stdout
        assert 'perf_guard_contract.schema.json' in proc.stdout, proc.stdout


def test_detects_baseline_policy_schema():
    with tempfile.TemporaryDirectory() as td:
        path = Path(td) / 'baseline_policy.json'
        write_json(path, {
            'schema_version': 1,
            'metadata': {
                'hard_fail_fields': ['size', 'runs'],
                'warning_only_fields': ['cpu_model', 'compiler']
            },
            'strict_profile_intent': {
                'flag': '--baseline-strict-profile-intent',
                'required_manifest_fields': ['profile_selected', 'profile_applied_defaults', 'profile_explicit_overrides'],
                'mismatch_modes': ['manifest_presence', 'profile_selected', 'profile_applied_defaults', 'profile_explicit_overrides']
            },
            'comparison': {
                'allowed_scopes': ['mean', 'p95', 'ci_high', 'any'],
                'noise_floor_rule': 'ignore deltas inside noise floor'
            },
            'report_exit_codes': {
                'success': 0,
                'per_operation_guard_failure': 2,
                'binary_regression': 4,
                'metadata_mismatch': 5
            }
        })
        proc = run_validator(path)
        assert proc.returncode == 0, proc.stderr + proc.stdout
        assert 'baseline_policy.schema.json' in proc.stdout, proc.stdout


def test_rejects_unrecognized_json():
    with tempfile.TemporaryDirectory() as td:
        path = Path(td) / 'unknown.json'
        write_json(path, {'hello': 'world'})
        proc = run_validator(path)
        assert proc.returncode == 1, proc.stderr + proc.stdout
        assert 'Unrecognized JSON structure' in proc.stderr, proc.stderr


def main():
    test_detects_benchmark_results_schema()
    test_detects_series_schema()
    test_detects_crossover_schema()
    test_detects_baseline_report_schema()
    test_detects_profile_contract_schema()
    test_detects_perf_guard_contract_schema()
    test_detects_baseline_policy_schema()
    test_rejects_unrecognized_json()
    print('validate_json.py tests passed')


if __name__ == '__main__':
    main()
