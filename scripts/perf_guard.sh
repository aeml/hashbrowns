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
TOL_PCT_INSERT=${TOL_PCT_INSERT:-20}
TOL_PCT_SEARCH=${TOL_PCT_SEARCH:-20}
TOL_PCT_REMOVE=${TOL_PCT_REMOVE:-20}
SIZE=${SIZE:-20000}
RUNS=${RUNS:-5}
SEED=${SEED:-12345}
STRUCTURES=${STRUCTURES:-array,slist,dlist,hashmap}

usage(){
  echo "Usage: $(basename "$0") [--update]";
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
"${BIN}" --no-banner --size "${SIZE}" --runs "${RUNS}" \
  --structures "${STRUCTURES}" --seed "${SEED}" \
  --output "${TMP_JSON}" --out-format json >/dev/null || true

if [[ ${UPDATE} -eq 1 ]]; then
  cp -f "${TMP_JSON}" "${BASE_JSON}"
  echo "[UPDATED] Baseline written to ${BASE_JSON}"
  exit 0
fi

if [[ ! -s "${BASE_JSON}" ]]; then
  echo "[ERROR] Baseline not found: ${BASE_JSON}. Run with --update to create it." >&2
  exit 1
fi

# Compare current results to baseline using Python (safer JSON parsing)
python3 - "$BASE_JSON" "$TMP_JSON" <<'PY'
import json, sys
base_path, curr_path = sys.argv[1], sys.argv[2]
with open(base_path) as f: base = json.load(f)
with open(curr_path) as f: curr = json.load(f)

# Build dict: structure -> metrics
bk = {r['structure']: r for r in base['results']}
ck = {r['structure']: r for r in curr['results']}

# Tolerances from environment
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
        # Allow improvements; only flag regressions beyond tolerance
        if cv > bv * (1.0 + tol):
            failures.append(f"{st}.{op}: current {cv:.3f} > baseline {bv:.3f} * (1+{tol:.2f})")

if failures:
    print("[PERF GUARD] Regressions detected:")
    for f in failures:
        print(" - ", f)
    sys.exit(2)
else:
    print("[PERF GUARD] OK: within tolerances or improved.")
    sys.exit(0)
PY

exit $?
