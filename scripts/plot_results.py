#!/usr/bin/env python3
import argparse
import csv
import os
import sys


def require_matplotlib():
    try:
        import matplotlib  # noqa: F401
        import matplotlib.pyplot as plt  # noqa: F401
        return True
    except Exception:
        print("[ERROR] matplotlib is not installed. Install it with: pip install matplotlib", file=sys.stderr)
        return False


def read_csv(path):
    rows = []
    if not os.path.exists(path):
        print(f"[ERROR] File not found: {path}", file=sys.stderr)
        return rows
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for r in reader:
            rows.append(r)
    return rows


def plot_bench(bench_rows, out_dir):
    import matplotlib.pyplot as plt

    if not bench_rows:
        print("[WARN] No benchmark rows to plot")
        return

    structures = [r['structure'] for r in bench_rows]
    insert = [float(r['insert_ms_mean']) for r in bench_rows]
    search = [float(r['search_ms_mean']) for r in bench_rows]
    remove = [float(r['remove_ms_mean']) for r in bench_rows]

    x = range(len(structures))
    width = 0.25

    fig, ax = plt.subplots(figsize=(8, 4))
    ax.bar([i - width for i in x], insert, width, label='insert')
    ax.bar(x, search, width, label='search')
    ax.bar([i + width for i in x], remove, width, label='remove')

    ax.set_xticks(list(x))
    ax.set_xticklabels(structures)
    ax.set_ylabel('ms (mean)')
    ax.set_title('Benchmark summary')
    ax.legend()
    fig.tight_layout()

    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, 'benchmark_summary.png')
    fig.savefig(out_path, dpi=150)
    print(f"[INFO] Wrote {out_path}")


def plot_crossovers(cross_rows, out_dir):
    import matplotlib.pyplot as plt

    if not cross_rows:
        print("[WARN] No crossover rows to plot")
        return

    # Group by operation; show points at size_at_crossover
    ops = {}
    for r in cross_rows:
        try:
            op = r['operation']
            size = int(float(r['size_at_crossover']))
            pair = f"{r['a']} vs {r['b']}"
            ops.setdefault(op, []).append((size, pair))
        except Exception:
            pass

    fig, axes = plt.subplots(nrows=len(ops), ncols=1, figsize=(8, 3 * max(1, len(ops))), squeeze=False)
    for ax, (op, lst) in zip(axes[:, 0], ops.items()):
        lst.sort(key=lambda x: x[0])
        sizes = [s for s, _ in lst]
        labels = [p for _, p in lst]
        ax.scatter(sizes, [1]*len(sizes), marker='x')
        for s, lbl in lst:
            ax.annotate(lbl, (s, 1), xytext=(0, 10), textcoords='offset points', rotation=45, ha='left', va='bottom', fontsize=8)
        ax.set_title(f"Crossover sizes: {op}")
        ax.set_xlabel('elements')
        ax.get_yaxis().set_visible(False)
        ax.grid(True, axis='x', linestyle='--', alpha=0.3)
    fig.tight_layout()

    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, 'crossover_points.png')
    fig.savefig(out_path, dpi=150)
    print(f"[INFO] Wrote {out_path}")


def main():
    ap = argparse.ArgumentParser(description='Plot hashbrowns benchmark CSV outputs')
    ap.add_argument('--bench-csv', default=os.path.join('build', 'benchmark_results.csv'))
    ap.add_argument('--cross-csv', default=os.path.join('build', 'crossover_results.csv'))
    ap.add_argument('--out-dir', default=os.path.join('build', 'plots'))
    args = ap.parse_args()

    if not require_matplotlib():
        return 1

    bench = read_csv(args.bench_csv)
    cross = read_csv(args.cross_csv)

    plot_bench(bench, args.out_dir)
    plot_crossovers(cross, args.out_dir)
    return 0


if __name__ == '__main__':
    sys.exit(main())
