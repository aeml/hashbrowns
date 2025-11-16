#!/usr/bin/env python3
import argparse
import json
import os
import sys
from typing import Dict

try:
    import jsonschema  # type: ignore
except Exception:
    print("[ERROR] jsonschema is not installed. Install with: pip install jsonschema", file=sys.stderr)
    sys.exit(2)


def load(path: str):
    with open(path, 'r', encoding='utf-8') as f:
        return json.load(f)


def detect_schema(obj: Dict[str, object]) -> str:
    # Heuristic: keys decide which schema to apply
    if 'results' in obj:
        return 'benchmark_results.schema.json'
    if 'series' in obj:
        return 'series_results.schema.json'
    if 'crossovers' in obj:
        return 'crossover_results.schema.json'
    raise ValueError('Unrecognized JSON structure (missing results/series/crossovers)')


def main():
    ap = argparse.ArgumentParser(description='Validate hashbrowns JSON with JSON Schema')
    ap.add_argument('--schema-dir', default=os.path.join('docs','api','schemas'))
    ap.add_argument('files', nargs='+', help='JSON files to validate')
    args = ap.parse_args()

    ok = True
    for path in args.files:
        try:
            obj = load(path)
            schema_name = detect_schema(obj)
            schema_path = os.path.join(args.schema_dir, schema_name)
            schema = load(schema_path)
            jsonschema.validate(instance=obj, schema=schema)
            print(f"[OK] {os.path.basename(path)} -> {schema_name}")
        except Exception as e:
            print(f"[FAIL] {path}: {e}", file=sys.stderr)
            ok = False
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
