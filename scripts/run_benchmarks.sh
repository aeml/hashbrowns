#!/usr/bin/env bash
set -euo pipefail

# Run hashbrowns benchmarks and crossover analysis with sensible defaults
# Outputs CSVs to build/ by default.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BIN="${BUILD_DIR}/hashbrowns"

# Defaults
RUNS=${RUNS:-10}
SIZE=${SIZE:-10000}
MAX_SIZE=${MAX_SIZE:-131072}
STRUCTURES=${STRUCTURES:-array,slist,hashmap}
BENCH_CSV=${BENCH_CSV:-${BUILD_DIR}/benchmark_results.csv}
CROSS_CSV=${CROSS_CSV:-${BUILD_DIR}/crossover_results.csv}
BUILD_TYPE=${BUILD_TYPE:-Release}
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Options (also available via env vars):
  --runs N           Number of runs per size (default: ${RUNS})
  --size N           Problem size for single-run mode (default: ${SIZE})
  --max-size N       Max size for crossover analysis (default: ${MAX_SIZE})
  --structures LIST  Comma list: array,slist,dlist,hashmap (default: ${STRUCTURES})
  --bench-csv PATH   Benchmark CSV output (default: ${BENCH_CSV})
  --cross-csv PATH   Crossover CSV output (default: ${CROSS_CSV})
  --build-type T     Debug|Release|RelWithDebInfo (default: ${BUILD_TYPE})
  --jobs N           Parallel build jobs (default: ${JOBS})
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
    --bench-csv) BENCH_CSV="$2"; shift 2;;
    --cross-csv) CROSS_CSV="$2"; shift 2;;
    --build-type) BUILD_TYPE="$2"; shift 2;;
    --jobs) JOBS="$2"; shift 2;;
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

# Run standard benchmark (single size) with CSV output
echo "[INFO] Running benchmarks: size=${SIZE}, runs=${RUNS}, structures=${STRUCTURES}"
"${BIN}" --no-banner --size "${SIZE}" --runs "${RUNS}" --structures "${STRUCTURES}" --output "${BENCH_CSV}" || true
echo "[INFO] Wrote: ${BENCH_CSV}"

# Run crossover analysis over a size sweep and write CSV
echo "[INFO] Running crossover analysis up to max-size=${MAX_SIZE}"
"${BIN}" --no-banner --quiet --crossover-analysis --max-size "${MAX_SIZE}" --structures "${STRUCTURES}" --runs "${RUNS}" --output "${CROSS_CSV}"
echo "[INFO] Wrote: ${CROSS_CSV}"

# Quick tail
echo "[INFO] Preview of crossover CSV:"
head -n 5 "${CROSS_CSV}" | sed 's/^/  /'
