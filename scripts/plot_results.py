#!/usr/bin/env python3
import argparse
import csv
import os
import sys
import platform
from typing import List, Sequence
import numpy as np


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
def get_hw_summary() -> str:
    try:
        os_name = platform.system()
        kernel = platform.release()
        arch = platform.machine() or "?"
        cores = os.cpu_count() or 1
        cpu = platform.processor() or ""
        # Try /proc/cpuinfo on Linux for nicer model name
        if os_name == 'Linux' and (not cpu or len(cpu) < 4):
            try:
                with open('/proc/cpuinfo') as f:
                    for line in f:
                        if line.lower().startswith('model name'):
                            cpu = line.split(':', 1)[1].strip()
                            break
            except Exception:
                pass
        return f"{os_name} {kernel} ({arch}), CPU: {cpu}, cores: {cores}"
    except Exception:
        return "(hardware info unavailable)"


def _annotate(fig, notes: Sequence[str], include_hw: bool):
    text = " | ".join(n for n in notes if n)
    if include_hw:
        hw = get_hw_summary()
        text = (text + (" | " if text else "")) + hw
    if text:
        fig.text(0.5, 0.01, text, ha='center', va='bottom', fontsize=8)



def _auto_yscale(values: List[float], mode: str) -> str:
    if mode in ("linear", "log"):
        return mode
    if mode in ("mid", "asinh", "sqrt"):
        return mode
    # auto: pick log if spread is large
    nonzero = [v for v in values if v > 0]
    if not nonzero:
        return "linear"
    vmin = min(nonzero)
    vmax = max(values) if values else 0.0
    ratio = (vmax / vmin) if vmin > 0 else float('inf')
    # Choose a middle scale when range is wide but not extreme
    if ratio >= 50 and ratio < 5000:
        return "mid"  # asinh-based compression
    return "log" if ratio >= 5000 else "linear"


def _apply_scale(ax, values: List[float], yscale: str):
    scale = _auto_yscale(values, yscale)
    if scale == "log":
        ax.set_yscale('log')
        ax.set_ylim(bottom=max(1e-3, min([v for v in values if v > 0] or [1e-3])))
    elif scale in ("mid", "asinh"):
        # "Middle" scale using arcsinh for gentler compression than log
        ax.set_yscale('function', functions=(np.arcsinh, np.sinh))
        ax.set_ylim(bottom=0)
    elif scale == "sqrt":
        ax.set_yscale('function', functions=(np.sqrt, lambda x: np.square(x)))
        ax.set_ylim(bottom=0)
    else:
        ax.set_yscale('linear')


def plot_bench(bench_rows, out_dir, yscale: str = "auto", notes: Sequence[str] = (), include_hw: bool = True):
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
    # y-scale (auto/mid/log) for readability
    all_vals = insert + search + remove
    _apply_scale(ax, all_vals, yscale)
    ax.set_ylabel('ms (mean)')
    ax.set_title('Benchmark summary')
    ax.legend()
    fig.tight_layout(rect=[0, 0.04, 1, 0.97])
    _annotate(fig, notes, include_hw)

    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, 'benchmark_summary.png')
    fig.savefig(out_path, dpi=150)
    print(f"[INFO] Wrote {out_path}")


def plot_bench_by_operation(bench_rows, out_dir, yscale: str = "auto", notes: Sequence[str] = (), include_hw: bool = True):
    import matplotlib.pyplot as plt

    if not bench_rows:
        print("[WARN] No benchmark rows to plot (by operation)")
        return

    structures = [r['structure'] for r in bench_rows]
    ops = {
        'insert': [float(r['insert_ms_mean']) for r in bench_rows],
        'search': [float(r['search_ms_mean']) for r in bench_rows],
        'remove': [float(r['remove_ms_mean']) for r in bench_rows],
    }

    fig, axes = plt.subplots(nrows=3, ncols=1, figsize=(8, 9), squeeze=True)
    for ax, (op, vals) in zip(axes, ops.items()):
        ax.bar(range(len(structures)), vals, width=0.6)
        ax.set_xticks(list(range(len(structures))))
        ax.set_xticklabels(structures)
        _apply_scale(ax, vals, yscale)
        ax.set_ylabel('ms (mean)')
        ax.set_title(op)
        ax.grid(True, axis='y', linestyle='--', alpha=0.3)
    fig.suptitle('Benchmark by operation', y=0.98)
    fig.tight_layout(rect=[0, 0.04, 1, 0.97])
    _annotate(fig, notes, include_hw)

    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, 'benchmark_by_operation.png')
    fig.savefig(out_path, dpi=150)
    print(f"[INFO] Wrote {out_path}")


def plot_crossovers(cross_rows, out_dir, notes: Sequence[str] = (), include_hw: bool = True):
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
    fig.tight_layout(rect=[0, 0.04, 1, 0.97])
    _annotate(fig, notes, include_hw)

    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, 'crossover_points.png')
    fig.savefig(out_path, dpi=150)
    print(f"[INFO] Wrote {out_path}")


def main():
    ap = argparse.ArgumentParser(description='Plot hashbrowns benchmark CSV outputs')
    ap.add_argument('--bench-csv', default=os.path.join('results', 'csvs', 'benchmark_results.csv'))
    ap.add_argument('--cross-csv', default=os.path.join('results', 'csvs', 'crossover_results.csv'))
    ap.add_argument('--series-csv', default=None, help='Optional series CSV (size,structure,insert_ms,search_ms,remove_ms)')
    ap.add_argument('--out-dir', default=os.path.join('results', 'plots'))
    ap.add_argument('--yscale', choices=['linear','mid','log','auto'], default='auto', help='Y-axis scale for benchmark plots (default: auto). Use "mid" for asinh-based middle ground.')
    ap.add_argument('--note', action='append', default=[], help='Additional annotation text (repeatable)')
    ap.add_argument('--no-hw', action='store_true', help='Do not annotate hardware info')
    args = ap.parse_args()

    if not require_matplotlib():
        return 1

    bench = read_csv(args.bench_csv) if args.bench_csv else []
    cross = read_csv(args.cross_csv) if args.cross_csv else []

    if bench:
        plot_bench(bench, args.out_dir, yscale=args.yscale, notes=args.note, include_hw=not args.no_hw)
        plot_bench_by_operation(bench, args.out_dir, yscale=args.yscale, notes=args.note, include_hw=not args.no_hw)
    if cross:
        plot_crossovers(cross, args.out_dir, notes=args.note, include_hw=not args.no_hw)

    if args.series_csv:
        series = read_csv(args.series_csv)
        if series:
            try:
                import matplotlib.pyplot as plt
            except Exception:
                print("[ERROR] matplotlib not available for series plots", file=sys.stderr)
                return 1
            # Build plots by operation across sizes per structure
            ops = ['insert_ms', 'search_ms', 'remove_ms']
            sizes = sorted({int(float(r['size'])) for r in series})
            structures = sorted({r['structure'] for r in series})
            data = {op: {st: [] for st in structures} for op in ops}
            # Group values in size order
            by_size = {s: [r for r in series if int(float(r['size'])) == s] for s in sizes}
            for s in sizes:
                rows = by_size[s]
                for st in structures:
                    row = next((r for r in rows if r['structure'] == st), None)
                    for op in ops:
                        val = float(row[op]) if row else float('nan')
                        data[op][st].append(val)
            # One figure per operation
            for op in ops:
                fig, ax = plt.subplots(figsize=(8,4))
                all_vals = []
                for st in structures:
                    y = data[op][st]
                    all_vals.extend([v for v in y if v == v])
                    ax.plot(sizes, y, marker='o', label=st)
                _apply_scale(ax, all_vals or [1.0], args.yscale)
                ax.set_title(f"Series: {op.replace('_ms','')} vs size")
                ax.set_xlabel("elements")
                ax.set_ylabel("ms (mean)")
                ax.grid(True, linestyle='--', alpha=0.3)
                ax.legend()
                fig.tight_layout(rect=[0, 0.04, 1, 0.97])
                _annotate(fig, args.note, not args.no_hw)
                os.makedirs(args.out_dir, exist_ok=True)
                out_path = os.path.join(args.out_dir, f"series_{op.replace('_ms','')}.png")
                fig.savefig(out_path, dpi=150)
                print(f"[INFO] Wrote {out_path}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
