#!/usr/bin/env python3
import argparse
import csv
import os
from collections import defaultdict


def read_csv(path):
    rows = []
    if not os.path.exists(path):
        return rows
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for r in reader:
            rows.append(r)
    return rows


def summarize_bench(rows):
    # Expect columns from benchmark CSV
    print("\n=== Benchmark Summary ===")
    if not rows:
        print("(no data)")
        return
    for r in rows:
        try:
            print(f"- {r['structure']}: insert={float(r['insert_ms_mean']):.3f} ms, "
                  f"search={float(r['search_ms_mean']):.3f} ms, remove={float(r['remove_ms_mean']):.3f} ms, "
                  f"mem={int(float(r['memory_bytes']))} bytes")
        except Exception:
            print(f"- {r}")


def summarize_crossovers(rows):
    # Expect columns: operation,a,b,size_at_crossover
    print("\n=== Crossover Points (approx) ===")
    if not rows:
        print("(no data)")
        return
    # Group by operation
    ops = defaultdict(list)
    for r in rows:
        try:
            ops[r['operation']].append((r['a'], r['b'], int(float(r['size_at_crossover']))))
        except Exception:
            pass
    for op, lst in ops.items():
        lst.sort(key=lambda x: x[2])
        print(f"{op}:")
        for a, b, s in lst[:10]:
            print(f"  {a} vs {b} -> ~{s} elements")


def main():
    ap = argparse.ArgumentParser(description="Analyze hashbrowns benchmark CSV outputs")
    ap.add_argument("--bench-csv", default=os.path.join("build", "benchmark_results.csv"))
    ap.add_argument("--cross-csv", default=os.path.join("build", "crossover_results.csv"))
    args = ap.parse_args()

    bench = read_csv(args.bench_csv)
    cross = read_csv(args.cross_csv)

    summarize_bench(bench)
    summarize_crossovers(cross)


if __name__ == "__main__":
    main()
