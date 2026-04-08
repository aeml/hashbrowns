#!/usr/bin/env python3
import argparse
import json
import os
import sys
from typing import Dict, List, Any


def load(path: str) -> Dict[str, Any]:
    with open(path, 'r', encoding='utf-8') as f:
        return json.load(f)


def pct_delta(bv: float, cv: float) -> float:
    if bv == 0:
        return 0.0
    return ((cv - bv) / bv) * 100.0


def build_per_operation_guard(base: Dict[str, Any], curr: Dict[str, Any], tol_insert: float,
                              tol_search: float, tol_remove: float) -> Dict[str, Any]:
    bk = {r['structure']: r for r in base['results']}
    ck = {r['structure']: r for r in curr['results']}

    entries: List[Dict[str, Any]] = []
    failures: List[str] = []
    for st in bk.keys():
        if st not in ck:
            failures.append(f"missing structure in current: {st}")
            continue
        b, c = bk[st], ck[st]
        per_entry: Dict[str, Any] = {'structure': st}
        for op_name, key, tol in [
            ('insert', 'insert_ms_mean', tol_insert),
            ('search', 'search_ms_mean', tol_search),
            ('remove', 'remove_ms_mean', tol_remove),
        ]:
            bv, cv = float(b[key]), float(c[key])
            ok = cv <= bv * (1.0 + tol / 100.0)
            per_entry[op_name] = {
                'baseline_value': bv,
                'current_value': cv,
                'delta_pct': pct_delta(bv, cv),
                'threshold_pct': tol,
                'ok': ok,
            }
            if not ok:
                failures.append(f"{st}.{key}: current {cv:.3f} > baseline {bv:.3f} * (1+{tol/100.0:.2f})")
        entries.append(per_entry)

    return {
        'ok': len(failures) == 0,
        'tolerances_pct': {
            'insert': tol_insert,
            'search': tol_search,
            'remove': tol_remove,
        },
        'entries': entries,
        'failures': failures,
    }


def main() -> int:
    ap = argparse.ArgumentParser(description='Merge exact per-operation perf-guard results into a baseline report JSON')
    ap.add_argument('--base', required=True, help='Baseline benchmark_results.json path')
    ap.add_argument('--current', required=True, help='Current benchmark_results.json path')
    ap.add_argument('--report', required=True, help='Existing baseline_report.json path to augment')
    ap.add_argument('--tol-insert', type=float, default=float(os.environ.get('TOL_PCT_INSERT', '20')))
    ap.add_argument('--tol-search', type=float, default=float(os.environ.get('TOL_PCT_SEARCH', '20')))
    ap.add_argument('--tol-remove', type=float, default=float(os.environ.get('TOL_PCT_REMOVE', '20')))
    args = ap.parse_args()

    base = load(args.base)
    curr = load(args.current)
    report = load(args.report)

    per_guard = build_per_operation_guard(base, curr, args.tol_insert, args.tol_search, args.tol_remove)
    report['per_operation_guard'] = per_guard
    if per_guard['failures']:
        report['exit_code'] = 2

    with open(args.report, 'w', encoding='utf-8') as f:
        json.dump(report, f, indent=2)
        f.write('\n')

    if per_guard['failures']:
        print('[PERF GUARD] Regressions detected:')
        for failure in per_guard['failures']:
            print(' - ', failure)
        return 2

    print('[PERF GUARD] OK: metadata compatible and within per-op tolerances.')
    print(f"[PERF GUARD] Report: {args.report}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
