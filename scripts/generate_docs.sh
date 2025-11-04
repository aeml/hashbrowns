#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

mkdir -p "${BUILD_DIR}"

if command -v doxygen >/dev/null 2>&1; then
  echo "[INFO] Generating API docs with Doxygen"
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release >/dev/null
  cmake --build "${BUILD_DIR}" --target docs -- -j2
  echo "[INFO] Docs generated under ${BUILD_DIR}/docs"
else
  echo "[INFO] Doxygen not found; skipping API generation."
fi

# Static content already lives under docs/*.md
