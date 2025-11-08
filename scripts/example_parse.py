#!/usr/bin/env python3
"""Example parser for hashbrowns benchmark JSON outputs.

Supports:
  * Benchmark JSON (single size) with an array of results under `results`.
  * Series JSON (multi-size) summarizing last size or aggregating medians.
  * Crossover JSON listing crossover points.

Usage:
  python3 scripts/example_parse.py path/to/benchmark_results.json

Flags:
  --csv          Emit condensed CSV (stdout) instead of table.
  --summary      One-line aggregate summary (lines, structures, seed).
  --operation OP Filter benchmark table to one operation (insert|search|remove) time columns only.

Exit codes:
  0 success
  2 schema mismatch or unknown format
  3 IO / JSON parse error
"""

from __future__ import annotations
import json, sys, argparse, statistics
from typing import Any, Dict, List

EXPECTED_SCHEMA = 1  # increment if schema.md evolves


def load_json(path: str) -> Dict[str, Any]:
    try:
        with open(path, 'r', encoding='utf-8') as f:
            return json.load(f)
    except OSError as e:
        print(f"error: unable to open '{path}': {e}", file=sys.stderr)
        sys.exit(3)
    except json.JSONDecodeError as e:
        print(f"error: JSON decode failed: {e}", file=sys.stderr)
        sys.exit(3)


def is_benchmark_blob(blob: Dict[str, Any]) -> bool:
    return 'results' in blob and isinstance(blob['results'], list) and 'meta' in blob


def is_crossover_blob(blob: Dict[str, Any]) -> bool:
    return 'crossover' in blob and isinstance(blob['crossover'], list)


def is_series_blob(blob: Dict[str, Any]) -> bool:
    return 'series' in blob and isinstance(blob['series'], list)


def validate_schema(blob: Dict[str, Any]):
    meta = blob.get('meta', {})
    sv = meta.get('schema_version')
    if sv != EXPECTED_SCHEMA:
        print(f"warning: schema_version={sv} (expected {EXPECTED_SCHEMA}); attempting best-effort parse", file=sys.stderr)


def fmt(f: Any, digits: int = 2) -> str:
    try:
        return f"{float(f):.{digits}f}"
    except (ValueError, TypeError):
        return "-"


def render_benchmark(blob: Dict[str, Any], args: argparse.Namespace):
    rows = []
    for r in blob['results']:
        rows.append({
            'structure': r.get('structure','?'),
            'insert_ms': r.get('insert_ms_mean'),
            'search_ms': r.get('search_ms_mean'),
            'remove_ms': r.get('remove_ms_mean'),
            'memory_bytes': r.get('memory_bytes'),
        })
    op_filter = args.operation
    headers = ['Structure']
    if op_filter in (None, 'insert'): headers.append('Insert(ms)')
    if op_filter in (None, 'search'): headers.append('Search(ms)')
    if op_filter in (None, 'remove'): headers.append('Remove(ms)')
    if op_filter is None: headers.append('Memory(bytes)')

    if args.csv:
        print(','.join(x.lower().replace('(ms)','').replace('(bytes)','') for x in headers))  # header row
        for r in rows:
            parts = [r['structure']]
            if op_filter in (None, 'insert'): parts.append(fmt(r['insert_ms']))
            if op_filter in (None, 'search'): parts.append(fmt(r['search_ms']))
            if op_filter in (None, 'remove'): parts.append(fmt(r['remove_ms']))
            if op_filter is None: parts.append(str(r['memory_bytes'] or 0))
            print(','.join(parts))
        return

    col_widths = [len(h) for h in headers]
    rendered = []
    for r in rows:
        cols = [r['structure']]
        if op_filter in (None, 'insert'): cols.append(fmt(r['insert_ms']))
        if op_filter in (None, 'search'): cols.append(fmt(r['search_ms']))
        if op_filter in (None, 'remove'): cols.append(fmt(r['remove_ms']))
        if op_filter is None: cols.append(str(r['memory_bytes'] or 0))
        rendered.append(cols)
        for i,c in enumerate(cols): col_widths[i] = max(col_widths[i], len(c))

    line = '  '.join(h.ljust(col_widths[i]) for i,h in enumerate(headers))
    print(line)
    for cols in rendered:
        print('  '.join(cols[i].ljust(col_widths[i]) for i in range(len(cols))))

    if args.summary:
        meta = blob.get('meta', {})
        seed = meta.get('seed', 'unknown')
        print(f"Summary: structures={len(rows)} seed={seed} size={meta.get('size','-')} runs={meta.get('runs','-')}")


def render_series(blob: Dict[str, Any], args: argparse.Namespace):
    # Aggregate last size per structure (or median across sizes)
    latest_by_structure: Dict[str, Dict[str, float]] = {}
    by_structure_values: Dict[str, List[float]] = {}
    for entry in blob['series']:
        st = entry.get('structure','?')
        size = entry.get('size')
        insert_ms = entry.get('insert_ms')
        latest = latest_by_structure.get(st)
        if not latest or (latest.get('size', -1) < size):
            latest_by_structure[st] = {'size': size, 'insert_ms': insert_ms}
        by_structure_values.setdefault(st, []).append(insert_ms)

    print("Series summary (using latest size + median insert time):")
    print("Structure  LatestSize  MedianInsert(ms)")
    for st, info in latest_by_structure.items():
        vals = by_structure_values[st]
        med = statistics.median(vals) if vals else None
        print(f"{st:<10} {info['size']:<10} {fmt(med):<}")

def render_crossover(blob: Dict[str, Any], args: argparse.Namespace):
    print("Crossover points:")
    print("Operation  Structures        Size")
    for row in blob['crossover']:
        op = row.get('operation','?')
        a = row.get('a','?'); b = row.get('b','?')
        size = row.get('size_at_crossover','?')
        print(f"{op:<9} {a},{b:<15} {size}")


def main():
    ap = argparse.ArgumentParser(description="Parse hashbrowns benchmark JSON and print a summary table.")
    ap.add_argument('path', help='Path to benchmark_results.json / series_results.json / crossover_results.json')
    ap.add_argument('--csv', action='store_true', help='Emit condensed CSV output')
    ap.add_argument('--summary', action='store_true', help='Add a one-line summary footer (benchmark only)')
    ap.add_argument('--operation', choices=['insert','search','remove'], help='Filter to a single operation time (benchmark only)')
    args = ap.parse_args()

    blob = load_json(args.path)
    if 'meta' in blob: validate_schema(blob)

    if is_benchmark_blob(blob):
        render_benchmark(blob, args); return
    if is_series_blob(blob):
        render_series(blob, args); return
    if is_crossover_blob(blob):
        render_crossover(blob, args); return

    print('error: unrecognized JSON format; expected benchmark, series, or crossover blob', file=sys.stderr)
    sys.exit(2)


if __name__ == '__main__':
    main()
