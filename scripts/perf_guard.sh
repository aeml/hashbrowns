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
SIZE=${SIZE:-20000}
RUNS=${RUNS:-5}
SEED=${SEED:-12345}
STRUCTURES=${STRUCTURES:-array,slist,dlist,hashmap}

usage(){
  cat <<EOF
Usage: $(basename "$0") [--update]

Artifacts:
  build/perf_guard_current.json   current benchmark run used for comparison
  build/perf_guard_report.json    machine-readable baseline comparison report

Environment overrides:
  SIZE, RUNS, SEED, STRUCTURES
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

# Run a small benchmark and capture JSON
"${BIN}" --no-banner --profile ci --size "${SIZE}" --runs "${RUNS}" \
  --structures "${STRUCTURES}" --seed "${SEED}" \
  --output "${TMP_JSON}" --out-format json >/dev/null || true
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

"${BIN}" --no-banner --profile ci --size "${SIZE}" --runs "${RUNS}" \
  --structures "${STRUCTURES}" --seed "${SEED}" \
  --output "${TMP_JSON}" --out-format json \
  --baseline "${BASE_JSON}" \
  --baseline-threshold "${MAX_TOL}" \
  --baseline-noise 1 \
  --baseline-scope "${BASELINE_SCOPE}" \
  --baseline-report-json "${REPORT_JSON}"

if [[ ! -s "${REPORT_JSON}" ]]; then
  echo "[ERROR] Expected perf guard report was not written: ${REPORT_JSON}" >&2
  echo "[ERROR] Rebuild hashbrowns so the current binary supports --baseline-report-json." >&2
  exit 3
fi

# Then enforce per-operation thresholds exactly.
python3 - "$BASE_JSON" "$TMP_JSON" <<'PY'
import json, sys
base_path, curr_path = sys.argv[1], sys.argv[2]
with open(base_path) as f: base = json.load(f)
with open(curr_path) as f: curr = json.load(f)

bk = {r['structure']: r for r in base['results']}
ck = {r['structure']: r for r in curr['results']}

import os
TOLI = float(os.environ.get('TOL_PCT_INSERT','20'))/100.0
TOLS = float(os.environ.get('TOL_PCT_SEARCH','20'))/100.0
TOLR = float(os.environ.get('TOL_PCT_REMOVE','20'))/100.0

failures = []
for st in bk.keys():
    if st not in ck:
        failures.append(f"missing structure in current: {st}")
        continue
    b, c = bk[st], ck[st]
    for op, tol in [('insert_ms_mean', TOLI), ('search_ms_mean', TOLS), ('remove_ms_mean', TOLR)]:
        bv, cv = float(b[op]), float(c[op])
        if cv > bv * (1.0 + tol):
            failures.append(f"{st}.{op}: current {cv:.3f} > baseline {bv:.3f} * (1+{tol:.2f})")

if failures:
    print("[PERF GUARD] Regressions detected:")
    for f in failures:
        print(" - ", f)
    sys.exit(2)
print("[PERF GUARD] OK: metadata compatible and within per-op tolerances.")
print(f"[PERF GUARD] Report: {os.path.join(os.path.dirname(curr_path), 'perf_guard_report.json')}")
PY

exit $?
