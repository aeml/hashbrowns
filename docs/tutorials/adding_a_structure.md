# Adding a New Data Structure

This project benchmarks implementations through a common interface.

## Interface

See `src/core/data_structure.h` for the required methods:

- `void insert(int key, const std::string& value)`
- `bool search(int key, std::string& value) const`
- `bool remove(int key)`
- `size_t memory_usage() const`

## Steps

1. Create header and source files under `src/structures/`.
2. Implement the interface methods.
3. Add the new files to `CMakeLists.txt` in the `STRUCTURES_SOURCES` and `STRUCTURES_HEADERS` lists.
4. Register the new name in two places:
   - `src/benchmark/benchmark_suite.cpp` in `make_structure(...)` to map the CLI name to your implementation.
   - `src/main.cpp` in the known-names validation list (the `valid` vector near the structure validation block).
5. Build and run unit tests:

```bash
scripts/build.sh -t Debug --test
```

## Naming convention

Use a lowercase CLI name (e.g., `array`, `slist`, `dlist`, `hashmap`).

## Verification

- Confirm the executable runs with your structure listed via `--structures <name>`
- Add unit tests under `tests/unit/` to validate basic operations and memory accounting
