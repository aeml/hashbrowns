#!/usr/bin/env bash
set -euo pipefail

# Local CI parity script for Linux/macOS (non-Windows)
# Runs a small matrix similar to GitHub Actions: compilers={gcc,clang}, build_type={Debug,Release}
# Also runs a sanitizer job (clang, Debug) and a tiny smoke benchmark on Linux Release.

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT_DIR"

JOBS=${JOBS:-$(command -v nproc >/dev/null 2>&1 && nproc || sysctl -n hw.ncpu || echo 4)}

run_cfg() {
  local compiler="$1"   # gcc|clang|default
  local build_type="$2" # Debug|Release
  local bdir="build-${compiler}-${build_type}"
  echo "==> Configure ${compiler}/${build_type}"
  rm -rf "$bdir"
  if [[ "$compiler" == "clang" ]]; then
    export CC=clang
    export CXX=clang++
  elif [[ "$compiler" == "gcc" ]]; then
    export CC=gcc
    export CXX=g++
  else
    unset CC CXX
  fi
  cmake -S . -B "$bdir" -DCMAKE_BUILD_TYPE="${build_type}" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  echo "==> Build ${compiler}/${build_type}"
  cmake --build "$bdir" -- -j"$JOBS"
  echo "==> Test ${compiler}/${build_type}"
  ctest --test-dir "$bdir" --output-on-failure -C "${build_type}"
}

# Matrix
for cc in gcc clang; do
  for bt in Debug Release; do
    run_cfg "$cc" "$bt"
  done
done

# Sanitizer job (clang, Debug)
SAN_BDIR="build-asan"
export CC=clang
export CXX=clang++
rm -rf "$SAN_BDIR"
cmake -S . -B "$SAN_BDIR" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG="-g -O0 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build "$SAN_BDIR" -- -j"$JOBS"
ASAN_OPTIONS=detect_leaks=1:allocator_may_return_null=1 UBSAN_OPTIONS=print_stacktrace=1 ctest --test-dir "$SAN_BDIR" --output-on-failure

# Smoke (Linux only) on Release
if [[ "$(uname -s)" == "Linux" ]]; then
  echo "==> Smoke run (Linux Release)"
  ./build-gcc-Release/hashbrowns --size 1024 --runs 2 --structures array,slist,hashmap --output build-gcc-Release/ci_bench.csv || true
fi

echo "All local CI parity checks completed successfully." 
