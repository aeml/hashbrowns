#!/usr/bin/env python3
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / 'docs' / 'api' / 'profiles.json'
BINARY = ROOT / 'build' / 'hashbrowns'


def load_json(path: Path):
    return json.loads(path.read_text(encoding='utf-8'))


def read_emitted_json(profile: str, path: Path, artifact_kind: str):
    if artifact_kind == 'benchmark_results' and path.exists():
        try:
            return load_json(path)
        except json.JSONDecodeError:
            pass
        json_path = path.with_suffix('.json')
        if json_path.exists():
            return load_json(json_path)
    return load_json(path)


def run_profile(profile: str, output_path: Path, artifact_kind: str):
    cmd = [str(BINARY), '--no-banner', '--profile', profile]
    if artifact_kind == 'series_results':
        cmd += ['--series-out', str(output_path)]
    else:
        cmd += ['--output', str(output_path)]
    if artifact_kind == 'benchmark_results' and profile in {'smoke', 'ci'}:
        cmd += ['--out-format', 'json']
    proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if proc.returncode != 0:
        raise AssertionError(f'profile run failed for {profile}:\nSTDOUT:\n{proc.stdout}\nSTDERR:\n{proc.stderr}')


def test_profile_contract_exists_and_has_expected_shape():
    contract = load_json(CONTRACT)
    assert contract['schema_version'] == 1, contract
    profiles = contract['profiles']
    assert set(profiles.keys()) == {'smoke', 'ci', 'series', 'crossover', 'deep'}, profiles.keys()
    for name, entry in profiles.items():
        assert 'description' in entry, (name, entry)
        assert 'applied_defaults' in entry, (name, entry)
        assert 'expected_explicit_overrides' in entry, (name, entry)
        assert 'artifact_kind' in entry, (name, entry)


def test_runtime_manifests_match_contract():
    contract = load_json(CONTRACT)['profiles']
    out_dir = ROOT / 'build' / 'contract_checks'
    out_dir.mkdir(parents=True, exist_ok=True)
    for profile, entry in contract.items():
        path = out_dir / f'{profile}.json'
        run_profile(profile, path, entry['artifact_kind'])
        obj = read_emitted_json(profile, path, entry['artifact_kind'])
        manifest = obj['meta']['profile_manifest']
        assert manifest['selected_profile'] == profile, (profile, manifest)
        assert manifest['applied_defaults'] == entry['applied_defaults'], (profile, manifest, entry)
        assert manifest['explicit_overrides'] == entry['expected_explicit_overrides'], (profile, manifest, entry)


def main():
    test_profile_contract_exists_and_has_expected_shape()
    test_runtime_manifests_match_contract()
    print('profile contract tests passed')


if __name__ == '__main__':
    main()
