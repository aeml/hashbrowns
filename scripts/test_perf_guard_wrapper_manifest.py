#!/usr/bin/env python3
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / 'scripts' / 'perf_guard.sh'


def run_perf_guard(temp_dir: Path, update: bool, extra_env=None):
    env = os.environ.copy()
    env['PERF_GUARD_BASE_DIR'] = str(temp_dir / 'perf_baselines')
    env['PERF_GUARD_BASE_JSON'] = str(temp_dir / 'perf_baselines' / 'baseline.json')
    env['PERF_GUARD_TMP_JSON'] = str(temp_dir / 'perf_guard_current.json')
    env['PERF_GUARD_REPORT_JSON'] = str(temp_dir / 'perf_guard_report.json')
    if extra_env:
        env.update(extra_env)
    cmd = ['bash', str(SCRIPT)]
    if update:
        cmd.append('--update')
    return subprocess.run(cmd, cwd=ROOT, env=env, capture_output=True, text=True)


def load_json(path: Path):
    return json.loads(path.read_text(encoding='utf-8'))


def test_wrapper_update_writes_temp_baseline_location_and_honest_manifest():
    with tempfile.TemporaryDirectory(dir=str(ROOT / 'build')) as td:
        temp_dir = Path(td)
        proc = run_perf_guard(temp_dir, update=True)
        baseline = temp_dir / 'perf_baselines' / 'baseline.json'
        assert proc.returncode == 0, proc.stderr + proc.stdout
        assert baseline.exists(), f'missing redirected baseline: {baseline}\n{proc.stdout}\n{proc.stderr}'
        obj = load_json(baseline)
        manifest = obj['meta']['profile_manifest']
        assert manifest['selected_profile'] == 'ci', manifest
        assert manifest['applied_defaults'] == ['size', 'runs', 'structures', 'seed'], manifest
        assert manifest['explicit_overrides'] == ['output'], manifest


def test_wrapper_check_writes_report_and_preserves_manifest_honesty():
    with tempfile.TemporaryDirectory(dir=str(ROOT / 'build')) as td:
        temp_dir = Path(td)
        update = run_perf_guard(temp_dir, update=True)
        assert update.returncode == 0, update.stderr + update.stdout
        check = run_perf_guard(temp_dir, update=False)
        report = temp_dir / 'perf_guard_report.json'
        current = temp_dir / 'perf_guard_current.json'
        assert check.returncode == 0, check.stderr + check.stdout
        assert report.exists(), f'missing redirected report: {report}\n{check.stdout}\n{check.stderr}'
        assert current.exists(), f'missing redirected current json: {current}\n{check.stdout}\n{check.stderr}'
        current_obj = load_json(current)
        manifest = current_obj['meta']['profile_manifest']
        assert manifest['selected_profile'] == 'ci', manifest
        assert manifest['applied_defaults'] == ['size', 'runs', 'structures', 'seed'], manifest
        assert manifest['explicit_overrides'] == ['output'], manifest
        report_obj = load_json(report)
        assert report_obj['exit_code'] == 0, report_obj
        assert report_obj['per_operation_guard']['ok'] is True, report_obj['per_operation_guard']


def main():
    test_wrapper_update_writes_temp_baseline_location_and_honest_manifest()
    test_wrapper_check_writes_report_and_preserves_manifest_honesty()
    print('perf_guard wrapper manifest tests passed')


if __name__ == '__main__':
    main()
