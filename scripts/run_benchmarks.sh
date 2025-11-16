#!/usr/bin/env bash
set -euo pipefail

# Run hashbrowns benchmarks and crossover analysis with sensible defaults
# Outputs now stored under results/csvs and plots under results/plots.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
RESULTS_DIR="${ROOT_DIR}/results"
BIN="${BUILD_DIR}/hashbrowns"

# Defaults
RUNS=${RUNS:-10}
SIZE=${SIZE:-10000}
MAX_SIZE=${MAX_SIZE:-32768}
STRUCTURES=${STRUCTURES:-array,slist,dlist,hashmap}
OUT_FORMAT=${OUT_FORMAT:-csv} # csv|json
BENCH_OUT=${BENCH_OUT:-}
CROSS_OUT=${CROSS_OUT:-}
HASH_STRATEGY=${HASH_STRATEGY:-open}
HASH_CAPACITY=${HASH_CAPACITY:-}
HASH_LOAD=${HASH_LOAD:-}
BUILD_TYPE=${BUILD_TYPE:-Release}
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}
CSV_DIR=${CSV_DIR:-${RESULTS_DIR}/csvs}
PLOTS_DIR=${PLOTS_DIR:-${RESULTS_DIR}/plots}
AUTO_PLOTS=${AUTO_PLOTS:-1} # 1=try to plot if possible, 0=skip unless forced
YSCALE=${YSCALE:-auto}
SERIES_RUNS=${SERIES_RUNS:-1}
SEED=${SEED:-}
PATTERN=${PATTERN:-sequential}
MAX_SECONDS=${MAX_SECONDS:-}
BASELINE=${BASELINE:-}
BASELINE_THRESHOLD=${BASELINE_THRESHOLD:-20}
BASELINE_NOISE=${BASELINE_NOISE:-1}

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Options (also available via env vars):
  --runs N           Number of runs per size (default: ${RUNS})
  --size N           Problem size for single-run mode (default: ${SIZE})
  --max-size N       Max size for crossover analysis (default: ${MAX_SIZE})
  --structures LIST  Comma list: array,slist,dlist,hashmap (default: ${STRUCTURES})
  --out-format F     csv|json (default: ${OUT_FORMAT})
  --bench-out PATH   Benchmark output file (default: build/benchmark_results.<ext>)
  --cross-out PATH   Crossover output file (default: build/crossover_results.<ext>)
  --build-type T     Debug|Release|RelWithDebInfo (default: ${BUILD_TYPE})
  --jobs N           Parallel build jobs (default: ${JOBS})
  --no-plots         Skip generating PNG plots (overrides AUTO_PLOTS)
  --plots            Force generate PNG plots
  --plots-dir PATH   Directory to write PNG plots (default: ${PLOTS_DIR})
  --yscale MODE      Plot y-axis scale: auto|linear|log (default: ${YSCALE})
  --series-runs N    Runs per size for crossover sweep (default: ${SERIES_RUNS})
  --seed N           RNG seed for random/mixed patterns (default: random_device)
  --pattern TYPE     sequential|random|mixed (default: ${PATTERN})
  --max-seconds N    Time budget (in seconds) for crossover sweep
  --hash-strategy S  open|chain (default: open)
  --hash-capacity N  initial capacity for hashmap
  --hash-load F      max load factor for hashmap
  --baseline PATH    Optional baseline JSON to compare single-size run against
  --baseline-threshold PCT  Max allowed slowdown percentage (default: ${BASELINE_THRESHOLD})
  --baseline-noise PCT      Ignore deltas within this band as noise (default: ${BASELINE_NOISE})
  -h, --help         Show this help

Examples:
  $(basename "$0") --runs 20 --size 50000
  $(basename "$0") --structures array,hashmap --max-size 200000
EOF
}

# Parse CLI
while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs) RUNS="$2"; shift 2;;
    --size) SIZE="$2"; shift 2;;
    --max-size) MAX_SIZE="$2"; shift 2;;
    --structures) STRUCTURES="$2"; shift 2;;
  --out-format) OUT_FORMAT="$2"; shift 2;;
  --bench-out) BENCH_OUT="$2"; shift 2;;
  --cross-out) CROSS_OUT="$2"; shift 2;;
    --build-type) BUILD_TYPE="$2"; shift 2;;
  --jobs) JOBS="$2"; shift 2;;
  --no-plots) AUTO_PLOTS=0; shift 1;;
  --plots) AUTO_PLOTS=2; shift 1;;
  --plots-dir) PLOTS_DIR="$2"; shift 2;;
  --yscale) YSCALE="$2"; shift 2;;
  --series-runs) SERIES_RUNS="$2"; shift 2;;
  --seed) SEED="$2"; shift 2;;
  --pattern) PATTERN="$2"; shift 2;;
  --max-seconds) MAX_SECONDS="$2"; shift 2;;
  --hash-strategy) HASH_STRATEGY="$2"; shift 2;;
  --hash-capacity) HASH_CAPACITY="$2"; shift 2;;
  --hash-load) HASH_LOAD="$2"; shift 2;;
  --baseline) BASELINE="$2"; shift 2;;
  --baseline-threshold) BASELINE_THRESHOLD="$2"; shift 2;;
  --baseline-noise) BASELINE_NOISE="$2"; shift 2;;
    -h|--help) usage; exit 0;;
    *) echo "Unknown option: $1"; usage; exit 2;;
  esac
done

# Ensure build exists and binary compiled
if [[ ! -x "${BIN}" ]]; then
  echo "[INFO] Building project (${BUILD_TYPE})..."
  bash "${ROOT_DIR}/scripts/build.sh" -t "${BUILD_TYPE}"
fi

if [[ ! -x "${BIN}" ]]; then
  echo "[ERROR] Binary not found at ${BIN}" >&2
  exit 1
fi

mkdir -p "${BUILD_DIR}"

# Default output paths if not provided
mkdir -p "${CSV_DIR}" "${PLOTS_DIR}" || true

if [[ -z "${BENCH_OUT}" ]]; then
  if [[ "${OUT_FORMAT}" == "json" ]]; then
    BENCH_OUT="${CSV_DIR}/benchmark_results.json"
  else
    BENCH_OUT="${CSV_DIR}/benchmark_results.csv"
  fi
fi
if [[ -z "${CROSS_OUT}" ]]; then
  if [[ "${OUT_FORMAT}" == "json" ]]; then
    CROSS_OUT="${CSV_DIR}/crossover_results.json"
  else
    CROSS_OUT="${CSV_DIR}/crossover_results.csv"
  fi
fi

# Run configuration summary and hardware info
echo "[INFO] Run configuration:"
echo "  size=${SIZE}, runs=${RUNS}, max-size=${MAX_SIZE}"
echo "  structures=${STRUCTURES}"
echo "  build-type=${BUILD_TYPE}, jobs=${JOBS}"
echo "  series-runs=${SERIES_RUNS} (for crossover sweep)"
echo "  pattern=${PATTERN}${SEED:+, seed=${SEED}}${MAX_SECONDS:+, max-seconds=${MAX_SECONDS}}"
echo "  out-format=${OUT_FORMAT}, bench-out=${BENCH_OUT}, cross-out=${CROSS_OUT}"
echo "  hashmap: strategy=${HASH_STRATEGY}${HASH_CAPACITY:+, capacity=${HASH_CAPACITY}}${HASH_LOAD:+, load=${HASH_LOAD}}"

HOST=$(hostname 2>/dev/null || echo unknown)
UNAME=$(uname -srmo 2>/dev/null || echo unknown)
CORES=$(nproc 2>/dev/null || echo 1)
CPU_MODEL=$(awk -F: '/model name/{print $2; exit}' /proc/cpuinfo 2>/dev/null | sed 's/^ *//')
if [[ -z "${CPU_MODEL}" ]]; then CPU_MODEL=$(python3 - <<'PY'
import platform
print(platform.processor())
PY
 2>/dev/null || echo ""); fi
echo "[INFO] Hardware: host=${HOST}, ${UNAME}, cores=${CORES}${CPU_MODEL:+, CPU=${CPU_MODEL}}"

# Run standard benchmark (single size) with CSV output
echo "[INFO] Running benchmarks: size=${SIZE}, runs=${RUNS}, structures=${STRUCTURES}"
BENCH_ARGS=(--no-banner --size "${SIZE}" --runs "${RUNS}" --structures "${STRUCTURES}" --output "${BENCH_OUT}" --pattern "${PATTERN}" --out-format "${OUT_FORMAT}")
[[ -n "${SEED}" ]] && BENCH_ARGS+=(--seed "${SEED}")
[[ -n "${HASH_STRATEGY}" ]] && BENCH_ARGS+=(--hash-strategy "${HASH_STRATEGY}")
[[ -n "${HASH_CAPACITY}" ]] && BENCH_ARGS+=(--hash-capacity "${HASH_CAPACITY}")
[[ -n "${HASH_LOAD}" ]] && BENCH_ARGS+=(--hash-load "${HASH_LOAD}")
if [[ -n "${BASELINE}" ]]; then
  BENCH_ARGS+=(--baseline "${BASELINE}" --baseline-threshold "${BASELINE_THRESHOLD}" --baseline-noise "${BASELINE_NOISE}")
fi
"${BIN}" "${BENCH_ARGS[@]}" || true
echo "[INFO] Wrote: ${BENCH_OUT}"

# Run crossover analysis over a size sweep and write CSV
echo "[INFO] Running crossover analysis up to max-size=${MAX_SIZE} (series-runs=${SERIES_RUNS})"
SWEEP_ARGS=(--no-banner --quiet --crossover-analysis --max-size "${MAX_SIZE}" --structures "${STRUCTURES}" --runs "${RUNS}" --series-runs "${SERIES_RUNS}" --pattern "${PATTERN}" --output "${CROSS_OUT}" --out-format "${OUT_FORMAT}")
[[ -n "${SEED}" ]] && SWEEP_ARGS+=(--seed "${SEED}")
[[ -n "${MAX_SECONDS}" ]] && SWEEP_ARGS+=(--max-seconds "${MAX_SECONDS}")
[[ -n "${HASH_STRATEGY}" ]] && SWEEP_ARGS+=(--hash-strategy "${HASH_STRATEGY}")
[[ -n "${HASH_CAPACITY}" ]] && SWEEP_ARGS+=(--hash-capacity "${HASH_CAPACITY}")
[[ -n "${HASH_LOAD}" ]] && SWEEP_ARGS+=(--hash-load "${HASH_LOAD}")
"${BIN}" "${SWEEP_ARGS[@]}" || true

if [[ -s "${CROSS_OUT}" ]]; then
  echo "[INFO] Wrote: ${CROSS_OUT}"
  echo "[INFO] Preview of crossover CSV:"
  if [[ "${OUT_FORMAT}" == "csv" ]]; then
    head -n 5 "${CROSS_OUT}" | sed 's/^/  /'
  else
    head -n 20 "${CROSS_OUT}" | sed 's/^/  /'
  fi
else
  echo "[WARN] Crossover CSV not generated (no crossovers detected at this scale or an error occurred)." >&2
fi

# Optionally generate plots
if [[ ${AUTO_PLOTS} -gt 0 ]]; then
  echo "[INFO] Generating plots into ${PLOTS_DIR} (if matplotlib is available)"
  if [[ "${OUT_FORMAT}" != "csv" ]]; then
    echo "[WARN] Plotting currently supports CSV only. Skipping plots because out-format=${OUT_FORMAT}." >&2
  elif command -v python3 >/dev/null 2>&1; then
    NOTE="size=${SIZE}, runs=${RUNS}, series-runs=${SERIES_RUNS}, structures=${STRUCTURES}, pattern=${PATTERN}${SEED:+, seed=${SEED}}"
    if python3 "${ROOT_DIR}/scripts/plot_results.py" --bench-csv "${BENCH_OUT}" --cross-csv "${CROSS_OUT}" --out-dir "${PLOTS_DIR}" --yscale "${YSCALE}" --note "${NOTE}"; then
      echo "[SUCCESS] Plots generated in ${PLOTS_DIR}"
    else
      if [[ ${AUTO_PLOTS} -gt 1 ]]; then
        echo "[ERROR] Plot generation failed (matplotlib missing?). Use --no-plots to skip." >&2
        exit 1
      else
        echo "[WARN] Plot generation skipped or failed (matplotlib missing?)" >&2
      fi
    fi
  else
    echo "[WARN] python3 not found; skipping plot generation" >&2
  fi
else
  echo "[INFO] Plot generation disabled (--no-plots)"
fi
