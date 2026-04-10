#!/usr/bin/env python3
import argparse
import json
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple


def load_json(path: Optional[str]) -> Optional[Dict[str, Any]]:
    if not path:
        return None
    p = Path(path)
    with p.open('r', encoding='utf-8') as f:
        return json.load(f)


def fmt_list(values: Sequence[str]) -> str:
    return ', '.join(values) if values else 'none'


def fmt_code(value: Any) -> str:
    return f'`{value}`'


def extract_profile(meta: Dict[str, Any]) -> str:
    return str(meta.get('profile', 'custom'))


def extract_manifest(meta: Dict[str, Any]) -> Dict[str, Any]:
    return meta.get('profile_manifest', {}) or {}


def fastest_by_operation(results: Sequence[Dict[str, Any]]) -> List[Tuple[str, str, float]]:
    ops = [
        ('insert', 'insert_ms_mean'),
        ('search', 'search_ms_mean'),
        ('remove', 'remove_ms_mean'),
    ]
    out: List[Tuple[str, str, float]] = []
    for op_name, key in ops:
        best = None
        for row in results:
            if key not in row:
                continue
            value = float(row[key])
            if best is None or value < best[2]:
                best = (op_name, str(row['structure']), value)
        if best is not None:
            out.append(best)
    return out


def summarize_series(series_rows: Sequence[Dict[str, Any]]) -> List[str]:
    if not series_rows:
        return []
    sizes = sorted({int(float(r['size'])) for r in series_rows})
    structures = sorted({str(r['structure']) for r in series_rows})
    return [
        f'- Sizes covered: {fmt_code(f"{sizes[0]}..{sizes[-1]}") if sizes else fmt_code("none")}',
        f'- Size points: {fmt_code(len(sizes))}',
        f'- Structures: {fmt_code(fmt_list(structures))}',
    ]


def summarize_crossovers(cross_rows: Sequence[Dict[str, Any]]) -> List[str]:
    lines: List[str] = []
    for row in cross_rows:
        pair = f"{row['a']} vs {row['b']}"
        lines.append(f"- `{row['operation']}`: {pair} around `{int(float(row['size_at_crossover']))}` elements")
    return lines


def summarize_baseline_failures(report: Dict[str, Any]) -> List[str]:
    per_guard = report.get('per_operation_guard', {}) or {}
    failures = per_guard.get('failures', []) or []
    return [f'- {msg}' for msg in failures]


def build_markdown(
    benchmark: Optional[Dict[str, Any]],
    series: Optional[Dict[str, Any]],
    crossover: Optional[Dict[str, Any]],
    baseline: Optional[Dict[str, Any]],
) -> str:
    lines: List[str] = []
    lines.append('# Hashbrowns Benchmark Report')
    lines.append('')
    lines.append('Generated from machine-readable artifacts. This report summarizes what was run, how comparable the evidence is, and what actually failed.')
    lines.append('')

    primary_meta: Dict[str, Any] = {}
    if benchmark:
        primary_meta = benchmark.get('meta', {}) or {}
    elif series:
        primary_meta = series.get('meta', {}) or {}
    elif crossover:
        primary_meta = crossover.get('meta', {}) or {}

    if primary_meta:
        manifest = extract_manifest(primary_meta)
        lines.append('## Run identity')
        lines.append('')
        lines.append(f"- Profile: {fmt_code(extract_profile(primary_meta))}")
        if manifest:
            lines.append(f"- Selected profile: {fmt_code(manifest.get('selected_profile', 'custom'))}")
            lines.append(f"- Applied defaults: {fmt_code(fmt_list(manifest.get('applied_defaults', [])))}")
            lines.append(f"- Explicit overrides: {fmt_code(fmt_list(manifest.get('explicit_overrides', [])))}")
        if 'pattern' in primary_meta:
            lines.append(f"- Pattern: {fmt_code(primary_meta['pattern'])}")
        if 'seed' in primary_meta:
            lines.append(f"- Seed: {fmt_code(primary_meta['seed'])}")
        if 'compiler' in primary_meta:
            lines.append(f"- Compiler: {fmt_code(primary_meta['compiler'])}")
        if 'cpu_model' in primary_meta:
            lines.append(f"- CPU model: {fmt_code(primary_meta['cpu_model'])}")
        lines.append('')

    if benchmark:
        results = benchmark.get('results', []) or []
        meta = benchmark.get('meta', {}) or {}
        lines.append('## Benchmark summary')
        lines.append('')
        if 'size' in meta:
            lines.append(f"- Benchmark size: {fmt_code(meta['size'])}")
        if 'runs' in meta:
            lines.append(f"- Runs: {fmt_code(meta['runs'])}")
        if 'bootstrap_iters' in meta:
            lines.append(f"- Bootstrap iterations: {fmt_code(meta['bootstrap_iters'])}")
        lines.append('')
        lines.append('### Fastest mean timings by operation')
        lines.append('')
        for op_name, structure, value in fastest_by_operation(results):
            lines.append(f'- `{op_name}`: `{structure}` at `{value:.3f} ms`')
        lines.append('')

    if series:
        lines.append('## Series coverage')
        lines.append('')
        for line in summarize_series(series.get('series', []) or []):
            lines.append(line)
        lines.append('')

    if crossover:
        lines.append('## Approximate crossover points')
        lines.append('')
        rows = crossover.get('crossovers', []) or []
        if rows:
            for line in summarize_crossovers(rows):
                lines.append(line)
        else:
            lines.append('- none')
        lines.append('')

    if baseline:
        comparison = baseline.get('comparison', {}) or {}
        coverage = comparison.get('coverage', {}) or {}
        lines.append('## Baseline comparison')
        lines.append('')
        lines.append(f"- Baseline path: {fmt_code(baseline.get('baseline_path', 'unknown'))}")
        lines.append(f"- Scope: {fmt_code(baseline.get('scope', 'mean'))}")
        lines.append(f"- Metadata compatibility: {fmt_code('ok' if (baseline.get('metadata', {}) or {}).get('ok') else 'mismatch')}")
        lines.append(f"- Hygiene gate: {fmt_code(comparison.get('hygiene_gate', 'clean'))}")
        lines.append(f"- Perf signal strength: {fmt_code(comparison.get('perf_signal_strength', 'none'))}")
        lines.append(f"- Recommended disposition: {fmt_code(comparison.get('recommended_disposition', 'accept'))}")
        per_ok = (baseline.get('per_operation_guard', {}) or {}).get('ok', True)
        lines.append(f"- Per-operation guard: {fmt_code('pass' if per_ok else 'fail')}")
        lines.append('')

        lines.append('### Coverage summary')
        lines.append('')
        lines.append(f"- Comparable structures: {fmt_code(coverage.get('comparable_structure_count', 0))}")
        lines.append(f"- Missing structures: {fmt_code((comparison.get('summary', {}) or {}).get('missing_structure_count', 0))}")
        lines.append(f"- Baseline-only structures: {fmt_code(fmt_list(coverage.get('baseline_only_structures', [])))}")
        lines.append(f"- Current-only structures: {fmt_code(fmt_list(coverage.get('current_only_structures', [])))}")
        lines.append('')

        failures = summarize_baseline_failures(baseline)
        if failures:
            lines.append('### Guard failures')
            lines.append('')
            lines.extend(failures)
            lines.append('')

        warnings = (baseline.get('metadata', {}) or {}).get('warnings', []) or []
        if warnings:
            lines.append('### Metadata warnings')
            lines.append('')
            for warning in warnings:
                lines.append(f'- {warning}')
            lines.append('')

    lines.append('## Interpretation')
    lines.append('')
    lines.append('- Treat crossover points as approximate and hardware-sensitive.')
    lines.append('- Treat baseline comparisons as trustworthy only when metadata compatibility is intact.')
    lines.append('- Hygiene and evidence fields should guide automation before humans over-interpret deltas.')
    lines.append('')

    return '\n'.join(lines).rstrip() + '\n'


def main() -> int:
    ap = argparse.ArgumentParser(description='Build a markdown report from hashbrowns JSON artifacts')
    ap.add_argument('--benchmark-json')
    ap.add_argument('--series-json')
    ap.add_argument('--crossover-json')
    ap.add_argument('--baseline-report-json')
    ap.add_argument('--output', required=True)
    args = ap.parse_args()

    if not any([args.benchmark_json, args.series_json, args.crossover_json, args.baseline_report_json]):
        print('[ERROR] Provide at least one input artifact.', file=sys.stderr)
        return 1

    benchmark = load_json(args.benchmark_json)
    series = load_json(args.series_json)
    crossover = load_json(args.crossover_json)
    baseline = load_json(args.baseline_report_json)

    report = build_markdown(benchmark, series, crossover, baseline)
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(report, encoding='utf-8')
    print(f'[OK] Wrote {args.output}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
