#!/usr/bin/env bash
set -euo pipefail

# perf_guard.sh: Run a tiny, fixed-seed benchmark and compare against a stored baseline.
# Fails (exit 2) if any metric deviates beyond tolerance.
# Usage: scripts/perf_guard.sh [--update]
#   --update   Recompute baseline from current build and overwrite baseline JSON.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
BIN="${ROOT_DIR}/build/hashbrowns"
BASE_DIR="${ROOT_DIR}/perf_baselines"
BASE_JSON="${BASE_DIR}/baseline.json"
TMP_JSON="${ROOT_DIR}/build/perf_guard_current.json"
REPORT_JSON="${ROOT_DIR}/build/perf_guard_report.json"
TOL_PCT_INSERT=${TOL_PCT_INSERT:-20}
TOL_PCT_SEARCH=${TOL_PCT_SEARCH:-20}
TOL_PCT_REMOVE=${TOL_PCT_REMOVE:-20}
BASELINE_SCOPE=${BASELINE_SCOPE:-mean}
SIZE=${SIZE:-}
RUNS=${RUNS:-}
SEED=${SEED:-}
STRUCTURES=${STRUCTURES:-}

usage(){
  cat <<EOF
Usage: $(basename "$0") [--update]

Artifacts:
  build/perf_guard_current.json   current benchmark run used for comparison
  build/perf_guard_report.json    machine-readable baseline comparison report

Environment overrides:
  SIZE, RUNS, SEED, STRUCTURES   override the ci profile defaults when set
  TOL_PCT_INSERT, TOL_PCT_SEARCH, TOL_PCT_REMOVE
  BASELINE_SCOPE=mean|p95|ci_high|any
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then usage; exit 0; fi
UPDATE=0
if [[ "${1:-}" == "--update" ]]; then UPDATE=1; fi

# Ensure binary exists
if [[ ! -x "${BIN}" ]]; then
  echo "[INFO] Building project (Release) for perf guard..."
  bash "${ROOT_DIR}/scripts/build.sh" -t Release
fi

mkdir -p "${BASE_DIR}"

RUN_ARGS=(--no-banner --profile ci --output "${TMP_JSON}" --out-format json)
if [[ -n "${SIZE:-}" ]]; then RUN_ARGS+=(--size "${SIZE}"); fi
if [[ -n "${RUNS:-}" ]]; then RUN_ARGS+=(--runs "${RUNS}"); fi
if [[ -n "${STRUCTURES:-}" ]]; then RUN_ARGS+=(--structures "${STRUCTURES}"); fi
if [[ -n "${SEED:-}" ]]; then RUN_ARGS+=(--seed "${SEED}"); fi

# Run a small benchmark and capture JSON
"${BIN}" "${RUN_ARGS[@]}" >/dev/null || true
rm -f "${REPORT_JSON}"

if [[ ${UPDATE} -eq 1 ]]; then
  cp -f "${TMP_JSON}" "${BASE_JSON}"
  echo "[UPDATED] Baseline written to ${BASE_JSON}"
  exit 0
fi

if [[ ! -s "${BASE_JSON}" ]]; then
  echo "[ERROR] Baseline not found: ${BASE_JSON}. Run with --update to create it." >&2
  exit 1
fi

# First let the built-in checker validate metadata compatibility and perform
# a coarse timing check. Use the loosest op threshold here; the fine-grained
# per-op thresholds are enforced below to preserve the script contract.
MAX_TOL=$(python3 - <<'PY'
import os
vals = [float(os.environ.get('TOL_PCT_INSERT', '20')), float(os.environ.get('TOL_PCT_SEARCH', '20')), float(os.environ.get('TOL_PCT_REMOVE', '20'))]
print(max(vals))
PY
)

COMPARE_ARGS=("${RUN_ARGS[@]}" --baseline "${BASE_JSON}" --baseline-threshold "${MAX_TOL}" --baseline-noise 1 --baseline-scope "${BASELINE_SCOPE}" --baseline-report-json "${REPORT_JSON}")
"${BIN}" "${COMPARE_ARGS[@]}"

if [[ ! -s "${REPORT_JSON}" ]]; then
  echo "[ERROR] Expected perf guard report was not written: ${REPORT_JSON}" >&2
  echo "[ERROR] Rebuild hashbrowns so the current binary supports --baseline-report-json." >&2
  exit 3
fi

# Then enforce per-operation thresholds exactly via the dedicated helper.
python3 "${ROOT_DIR}/scripts/perf_guard_report.py" \
  --base "${BASE_JSON}" \
  --current "${TMP_JSON}" \
  --report "${REPORT_JSON}" \
  --tol-insert "${TOL_PCT_INSERT}" \
  --tol-search "${TOL_PCT_SEARCH}" \
  --tol-remove "${TOL_PCT_REMOVE}"

exit $?
