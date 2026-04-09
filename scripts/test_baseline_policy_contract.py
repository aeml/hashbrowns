#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / 'docs' / 'api' / 'baseline_policy.json'


def load_json(path: Path):
    return json.loads(path.read_text(encoding='utf-8'))


def test_baseline_policy_contract_exists_and_has_expected_shape():
    contract = load_json(CONTRACT)
    assert contract['schema_version'] == 1, contract
    assert contract['metadata']['hard_fail_fields'] == [
        'schema_version',
        'size',
        'runs',
        'warmup_runs',
        'bootstrap_iters',
        'profile',
        'structures',
        'pattern',
        'seed',
        'build_type',
        'hash_strategy',
        'hash_capacity',
        'hash_load',
        'pinned_cpu',
        'turbo_disabled'
    ], contract
    assert contract['metadata']['warning_only_fields'] == [
        'cpu_model',
        'compiler',
        'cpp_standard',
        'cpu_governor',
        'cores',
        'total_ram_bytes',
        'kernel'
    ], contract
    assert contract['strict_profile_intent']['required_manifest_fields'] == [
        'profile_selected',
        'profile_applied_defaults',
        'profile_explicit_overrides'
    ], contract
    assert contract['comparison']['allowed_scopes'] == ['mean', 'p95', 'ci_high', 'any'], contract
    assert contract['report_exit_codes'] == {
        'success': 0,
        'per_operation_guard_failure': 2,
        'binary_regression': 4,
        'metadata_mismatch': 5,
    }, contract


def main():
    test_baseline_policy_contract_exists_and_has_expected_shape()
    print('baseline policy contract tests passed')


if __name__ == '__main__':
    main()
