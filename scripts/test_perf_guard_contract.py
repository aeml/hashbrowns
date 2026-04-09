#!/usr/bin/env python3
import json
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / 'docs' / 'api' / 'perf_guard_contract.json'
SCRIPT = ROOT / 'scripts' / 'perf_guard.sh'


def load_json(path: Path):
    return json.loads(path.read_text(encoding='utf-8'))


def run_perf_guard(temp_dir: Path, update: bool):
    env = os.environ.copy()
    env['PERF_GUARD_BASE_DIR'] = str(temp_dir / 'perf_baselines')
    env['PERF_GUARD_BASE_JSON'] = str(temp_dir / 'perf_baselines' / 'baseline.json')
    env['PERF_GUARD_TMP_JSON'] = str(temp_dir / 'perf_guard_current.json')
    env['PERF_GUARD_REPORT_JSON'] = str(temp_dir / 'perf_guard_report.json')
    proc = subprocess.run(
        ['bash', str(SCRIPT)] + (['--update'] if update else []),
        cwd=ROOT,
        env=env,
        capture_output=True,
        text=True,
    )
    return proc


def test_perf_guard_contract_exists_and_has_expected_shape():
    contract = load_json(CONTRACT)
    assert contract['schema_version'] == 1, contract
    assert contract['wrapper']['script'] == 'scripts/perf_guard.sh', contract
    assert contract['wrapper']['profile'] == 'ci', contract
    assert contract['wrapper']['artifact_paths']['baseline'] == 'perf_baselines/baseline.json', contract
    assert contract['wrapper']['artifact_paths']['current'] == 'build/perf_guard_current.json', contract
    assert contract['wrapper']['artifact_paths']['report'] == 'build/perf_guard_report.json', contract
    assert contract['wrapper']['wrapper_explicit_overrides'] == ['output', 'out_format'], contract
    assert contract['report']['schema'] == 'docs/api/schemas/baseline_report.schema.json', contract
    assert contract['report']['exit_codes'] == {'success': 0, 'per_operation_guard_failure': 2, 'binary_regression': 4, 'metadata_mismatch': 5}, contract


def test_perf_guard_runtime_matches_contract():
    contract = load_json(CONTRACT)
    with tempfile.TemporaryDirectory(dir=str(ROOT / 'build')) as td:
        temp_dir = Path(td)
        update = run_perf_guard(temp_dir, update=True)
        assert update.returncode == 0, update.stderr + update.stdout
        baseline = load_json(temp_dir / 'perf_baselines' / 'baseline.json')
        baseline_manifest = baseline['meta']['profile_manifest']
        assert baseline_manifest['selected_profile'] == contract['wrapper']['profile'], baseline_manifest
        assert baseline_manifest['explicit_overrides'] == contract['wrapper']['wrapper_explicit_overrides'], baseline_manifest

        check = run_perf_guard(temp_dir, update=False)
        assert check.returncode == contract['report']['exit_codes']['success'], check.stderr + check.stdout
        current = load_json(temp_dir / 'perf_guard_current.json')
        report = load_json(temp_dir / 'perf_guard_report.json')
        current_manifest = current['meta']['profile_manifest']
        assert current_manifest['selected_profile'] == contract['wrapper']['profile'], current_manifest
        assert current_manifest['explicit_overrides'] == contract['wrapper']['wrapper_explicit_overrides'], current_manifest
        assert report['exit_code'] == contract['report']['exit_codes']['success'], report
        assert sorted(report['per_operation_guard']['tolerances_pct'].keys()) == sorted(contract['report']['per_operation_guard_tolerance_keys']), report
        assert report['per_operation_guard']['ok'] is True, report['per_operation_guard']


def main():
    test_perf_guard_contract_exists_and_has_expected_shape()
    test_perf_guard_runtime_matches_contract()
    print('perf guard contract tests passed')


if __name__ == '__main__':
    main()
