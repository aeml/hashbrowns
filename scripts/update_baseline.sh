#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BASELINE_DIR="${ROOT_DIR}/docs/baselines"
OUT="${BASELINE_DIR}/benchmark_results.csv"

SIZE=2048
RUNS=5
STRUCTURES="array,slist,hashmap"

usage(){
  cat <<EOF
Usage: $(basename "$0") [--size N] [--runs N] [--structures LIST]

Generates docs/baselines/benchmark_results.csv in a Release build.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --size) SIZE="$2"; shift 2;;
    --runs) RUNS="$2"; shift 2;;
    --structures) STRUCTURES="$2"; shift 2;;
    -h|--help) usage; exit 0;;
    *) echo "Unknown option: $1"; usage; exit 2;;
  esac
done

mkdir -p "${BASELINE_DIR}"

# Ensure binary exists
if [[ ! -x "${BUILD_DIR}/hashbrowns" ]]; then
  echo "[INFO] Building Release binary..."
  bash "${ROOT_DIR}/scripts/build.sh" -t Release > /dev/null
fi

"${BUILD_DIR}/hashbrowns" --size "${SIZE}" --runs "${RUNS}" --structures "${STRUCTURES}" --output "${OUT}"
echo "[INFO] Wrote baseline: ${OUT}"
