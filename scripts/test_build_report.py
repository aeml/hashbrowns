#!/usr/bin/env python3
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / 'scripts' / 'build_report.py'


def write_json(path: Path, obj):
    path.write_text(json.dumps(obj, indent=2) + '\n', encoding='utf-8')


def build_fixture_set(tmp: Path):
    bench = tmp / 'benchmark_results.json'
    series = tmp / 'series_results.json'
    crossover = tmp / 'crossover_results.json'
    baseline = tmp / 'baseline_report.json'
    report = tmp / 'report.md'

    write_json(bench, {
        'meta': {
            'schema_version': 1,
            'size': 20000,
            'runs': 10,
            'warmup_runs': 0,
            'bootstrap_iters': 200,
            'structures': ['array', 'hashmap'],
            'pattern': 'sequential',
            'seed': 12345,
            'timestamp': '2026-04-10T00:00:00Z',
            'compiler': 'GCC 13',
            'build_type': 'Release',
            'cpu_model': 'Test CPU',
            'profile': 'ci',
            'cores': 8,
            'total_ram_bytes': 17179869184,
            'kernel': 'Linux',
            'profile_manifest': {
                'selected_profile': 'ci',
                'applied_defaults': ['size', 'runs', 'structures', 'seed'],
                'explicit_overrides': ['output', 'out_format']
            }
        },
        'results': [
            {
                'structure': 'array',
                'insert_ms_mean': 1.1, 'insert_ms_stddev': 0.1, 'insert_ms_median': 1.1, 'insert_ms_p95': 1.2, 'insert_ci_low': 1.0, 'insert_ci_high': 1.3,
                'search_ms_mean': 2.1, 'search_ms_stddev': 0.1, 'search_ms_median': 2.1, 'search_ms_p95': 2.2, 'search_ci_low': 2.0, 'search_ci_high': 2.3,
                'remove_ms_mean': 3.1, 'remove_ms_stddev': 0.1, 'remove_ms_median': 3.1, 'remove_ms_p95': 3.2, 'remove_ci_low': 3.0, 'remove_ci_high': 3.3,
                'memory_bytes': 1024,
                'memory_insert_mean': 0.0, 'memory_insert_stddev': 0.0,
                'memory_search_mean': 0.0, 'memory_search_stddev': 0.0,
                'memory_remove_mean': 0.0, 'memory_remove_stddev': 0.0
            },
            {
                'structure': 'hashmap',
                'insert_ms_mean': 0.9, 'insert_ms_stddev': 0.1, 'insert_ms_median': 0.9, 'insert_ms_p95': 1.0, 'insert_ci_low': 0.8, 'insert_ci_high': 1.1,
                'search_ms_mean': 0.8, 'search_ms_stddev': 0.1, 'search_ms_median': 0.8, 'search_ms_p95': 0.9, 'search_ci_low': 0.7, 'search_ci_high': 1.0,
                'remove_ms_mean': 1.0, 'remove_ms_stddev': 0.1, 'remove_ms_median': 1.0, 'remove_ms_p95': 1.1, 'remove_ci_low': 0.9, 'remove_ci_high': 1.2,
                'memory_bytes': 2048,
                'memory_insert_mean': 0.0, 'memory_insert_stddev': 0.0,
                'memory_search_mean': 0.0, 'memory_search_stddev': 0.0,
                'memory_remove_mean': 0.0, 'memory_remove_stddev': 0.0
            }
        ]
    })

    write_json(series, {
        'meta': {
            'schema_version': 1,
            'size': 60000,
            'runs_per_size': 3,
            'structures': ['array', 'hashmap'],
            'pattern': 'sequential',
            'timestamp': '2026-04-10T00:00:00Z',
            'compiler': 'GCC 13',
            'build_type': 'Release',
            'cpu_model': 'Test CPU',
            'profile': 'series',
            'cores': 8,
            'total_ram_bytes': 17179869184,
            'kernel': 'Linux',
            'profile_manifest': {
                'selected_profile': 'series',
                'applied_defaults': ['size', 'runs', 'series_count', 'structures'],
                'explicit_overrides': ['series_out']
            }
        },
        'series': [
            {'size': 10000, 'structure': 'array', 'insert_ms': 0.6, 'search_ms': 1.1, 'remove_ms': 1.4},
            {'size': 10000, 'structure': 'hashmap', 'insert_ms': 0.7, 'search_ms': 0.5, 'remove_ms': 0.8},
            {'size': 60000, 'structure': 'array', 'insert_ms': 2.8, 'search_ms': 6.2, 'remove_ms': 8.4},
            {'size': 60000, 'structure': 'hashmap', 'insert_ms': 3.1, 'search_ms': 1.9, 'remove_ms': 2.6}
        ]
    })

    write_json(crossover, {
        'meta': {
            'schema_version': 1,
            'size': 65536,
            'runs': 3,
            'structures': ['array', 'hashmap'],
            'pattern': 'sequential',
            'timestamp': '2026-04-10T00:00:00Z',
            'compiler': 'GCC 13',
            'build_type': 'Release',
            'cpu_model': 'Test CPU',
            'profile': 'crossover',
            'cores': 8,
            'total_ram_bytes': 17179869184,
            'kernel': 'Linux',
            'profile_manifest': {
                'selected_profile': 'crossover',
                'applied_defaults': ['max_size', 'runs', 'series_runs', 'structures'],
                'explicit_overrides': ['output', 'out_format']
            }
        },
        'crossovers': [
            {'operation': 'insert', 'a': 'array', 'b': 'hashmap', 'size_at_crossover': 32768},
            {'operation': 'search', 'a': 'array', 'b': 'hashmap', 'size_at_crossover': 2048}
        ]
    })

    write_json(baseline, {
        'baseline_path': 'perf_baselines/baseline.json',
        'scope': 'mean',
        'threshold_pct': 20,
        'noise_floor_pct': 1,
        'strict_profile_intent': True,
        'exit_code': 2,
        'metadata': {'ok': True, 'errors': [], 'warnings': ['cpu_model changed']},
        'comparison': {
            'all_ok': True,
            'decision_basis': 'mean',
            'health': 'partial_coverage',
            'actionability': 'actionable_with_hygiene_warnings',
            'recommended_disposition': 'review_with_warnings',
            'disposition_reasons': ['missing_structures'],
            'summary': {
                'missing_structure_count': 1,
                'duplicate_baseline_structure_count': 0,
                'duplicate_current_structure_count': 0
            },
            'has_hygiene_issues': True,
            'hygiene_issue_count': 1,
            'hygiene_gate': 'warn',
            'perf_signal_strength': 'limited',
            'coverage': {
                'baseline_structure_count': 3,
                'current_structure_count': 2,
                'comparable_structure_count': 2,
                'baseline_only_structures': ['dlist'],
                'current_only_structures': [],
                'duplicate_baseline_structures': [],
                'duplicate_current_structures': []
            },
            'failures': [
                {
                    'structure': 'hashmap',
                    'operation': 'insert',
                    'chosen_basis': 'mean',
                    'chosen_delta_pct': 24.0,
                    'threshold_pct': 20.0,
                    'failed_metric_families': ['mean', 'p95']
                }
            ],
            'entries': [
                {
                    'structure': 'array',
                    'insert_delta_pct': 2.0, 'search_delta_pct': 3.0, 'remove_delta_pct': 4.0,
                    'insert_ok': True, 'search_ok': True, 'remove_ok': True,
                    'insert_basis': 'mean', 'search_basis': 'mean', 'remove_basis': 'mean',
                    'insert_mean_delta_pct': 2.0, 'insert_p95_delta_pct': 2.5, 'insert_ci_high_delta_pct': 3.0,
                    'insert_mean_ok': True, 'insert_p95_ok': True, 'insert_ci_high_ok': True,
                    'search_mean_delta_pct': 3.0, 'search_p95_delta_pct': 3.5, 'search_ci_high_delta_pct': 4.0,
                    'search_mean_ok': True, 'search_p95_ok': True, 'search_ci_high_ok': True,
                    'remove_mean_delta_pct': 4.0, 'remove_p95_delta_pct': 4.5, 'remove_ci_high_delta_pct': 5.0,
                    'remove_mean_ok': True, 'remove_p95_ok': True, 'remove_ci_high_ok': True
                }
            ]
        },
        'per_operation_guard': {
            'ok': False,
            'tolerances_pct': {'insert': 20, 'search': 20, 'remove': 20},
            'entries': [
                {
                    'structure': 'hashmap',
                    'insert': {'baseline_value': 1.0, 'current_value': 1.3, 'delta_pct': 30.0, 'threshold_pct': 20.0, 'ok': False},
                    'search': {'baseline_value': 1.0, 'current_value': 1.0, 'delta_pct': 0.0, 'threshold_pct': 20.0, 'ok': True},
                    'remove': {'baseline_value': 1.0, 'current_value': 1.0, 'delta_pct': 0.0, 'threshold_pct': 20.0, 'ok': True}
                }
            ],
            'failures': ['hashmap insert delta 30.00% exceeds threshold 20.00%']
        }
    })

    return bench, series, crossover, baseline, report


def test_build_report_generates_honest_markdown_summary():
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        bench, series, crossover, baseline, report = build_fixture_set(tmp)
        proc = subprocess.run(
            [
                sys.executable,
                str(SCRIPT),
                '--benchmark-json', str(bench),
                '--series-json', str(series),
                '--crossover-json', str(crossover),
                '--baseline-report-json', str(baseline),
                '--output', str(report),
            ],
            capture_output=True,
            text=True,
        )
        assert proc.returncode == 0, proc.stderr + proc.stdout
        text = report.read_text(encoding='utf-8')
        assert '# Hashbrowns Benchmark Report' in text
        assert 'Profile: `ci`' in text
        assert 'Selected profile: `ci`' in text
        assert 'Hygiene gate: `warn`' in text
        assert 'Perf signal strength: `limited`' in text
        assert 'Recommended disposition: `review_with_warnings`' in text
        assert 'Per-operation guard: `fail`' in text
        assert 'hashmap insert delta 30.00% exceeds threshold 20.00%' in text
        assert 'Approximate crossover points' in text
        assert 'array vs hashmap' in text
        assert 'Fastest mean timings by operation' in text
        assert 'Coverage summary' in text
        assert 'Missing structures: `1`' in text
        assert 'Baseline-only structures: `dlist`' in text
        assert 'Per-structure coarse comparison' in text
        assert '| `array` | pass | `2.00%` | `3.00%` | `4.00%` |' in text
        assert 'Structured coarse failures' in text
        assert '| `hashmap` | `insert` | `mean` | `24.00%` | `20.00%` | `mean, p95` |' in text


def test_build_report_requires_at_least_one_input_artifact():
    with tempfile.TemporaryDirectory() as td:
        report = Path(td) / 'report.md'
        proc = subprocess.run(
            [sys.executable, str(SCRIPT), '--output', str(report)],
            capture_output=True,
            text=True,
        )
        assert proc.returncode == 1, proc.stderr + proc.stdout
        assert 'Provide at least one input artifact' in (proc.stderr + proc.stdout)


def main():
    test_build_report_generates_honest_markdown_summary()
    test_build_report_requires_at_least_one_input_artifact()
    print('build_report.py tests passed')


if __name__ == '__main__':
    main()
