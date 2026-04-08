#!/usr/bin/env python3
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HELPER = ROOT / 'scripts' / 'perf_guard_report.py'


def write_json(path: Path, obj):
    path.write_text(json.dumps(obj, indent=2) + '\n', encoding='utf-8')


def make_benchmark(results):
    return {
        'meta': {'schema_version': 1},
        'results': results,
    }


def make_report():
    return {
        'baseline_path': 'perf_baselines/baseline.json',
        'scope': 'mean',
        'threshold_pct': 20,
        'noise_floor_pct': 1,
        'strict_profile_intent': False,
        'exit_code': 0,
        'metadata': {'ok': True, 'errors': [], 'warnings': []},
        'comparison': {'all_ok': True, 'entries': []},
    }


def run_helper(tmp: Path, base_obj, curr_obj, report_obj, extra_args=None):
    base = tmp / 'base.json'
    curr = tmp / 'current.json'
    report = tmp / 'report.json'
    write_json(base, base_obj)
    write_json(curr, curr_obj)
    write_json(report, report_obj)
    cmd = [
        sys.executable,
        str(HELPER),
        '--base', str(base),
        '--current', str(curr),
        '--report', str(report),
    ]
    if extra_args:
        cmd.extend(extra_args)
    proc = subprocess.run(cmd, capture_output=True, text=True)
    return proc, json.loads(report.read_text(encoding='utf-8'))


def test_pass_case():
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        base = make_benchmark([
            {'structure': 'array', 'insert_ms_mean': 100.0, 'search_ms_mean': 50.0, 'remove_ms_mean': 75.0},
        ])
        curr = make_benchmark([
            {'structure': 'array', 'insert_ms_mean': 110.0, 'search_ms_mean': 55.0, 'remove_ms_mean': 80.0},
        ])
        proc, report = run_helper(tmp, base, curr, make_report())
        assert proc.returncode == 0, proc.stderr + proc.stdout
        per = report['per_operation_guard']
        assert per['ok'] is True, per
        assert per['failures'] == [], per
        assert report['exit_code'] == 0, report
        entry = per['entries'][0]
        assert entry['insert']['ok'] is True, entry
        assert entry['search']['ok'] is True, entry
        assert entry['remove']['ok'] is True, entry


def test_failure_case():
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        base = make_benchmark([
            {'structure': 'array', 'insert_ms_mean': 100.0, 'search_ms_mean': 50.0, 'remove_ms_mean': 75.0},
        ])
        curr = make_benchmark([
            {'structure': 'array', 'insert_ms_mean': 130.0, 'search_ms_mean': 55.0, 'remove_ms_mean': 80.0},
        ])
        proc, report = run_helper(tmp, base, curr, make_report(), ['--tol-insert', '20', '--tol-search', '20', '--tol-remove', '20'])
        assert proc.returncode == 2, proc.stderr + proc.stdout
        per = report['per_operation_guard']
        assert per['ok'] is False, per
        assert report['exit_code'] == 2, report
        assert any('insert_ms_mean' in msg for msg in per['failures']), per
        assert per['entries'][0]['insert']['ok'] is False, per


def test_missing_structure_case():
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        base = make_benchmark([
            {'structure': 'array', 'insert_ms_mean': 100.0, 'search_ms_mean': 50.0, 'remove_ms_mean': 75.0},
            {'structure': 'hashmap', 'insert_ms_mean': 100.0, 'search_ms_mean': 50.0, 'remove_ms_mean': 75.0},
        ])
        curr = make_benchmark([
            {'structure': 'array', 'insert_ms_mean': 100.0, 'search_ms_mean': 50.0, 'remove_ms_mean': 75.0},
        ])
        proc, report = run_helper(tmp, base, curr, make_report())
        assert proc.returncode == 2, proc.stderr + proc.stdout
        per = report['per_operation_guard']
        assert per['ok'] is False, per
        assert any('missing structure in current: hashmap' == msg for msg in per['failures']), per


def main():
    test_pass_case()
    test_failure_case()
    test_missing_structure_case()
    print('perf_guard_report.py tests passed')


if __name__ == '__main__':
    main()
