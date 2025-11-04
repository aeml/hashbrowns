# 🥔 hashbrowns

*A crispy C++ benchmarking suite comparing arrays, linked lists, and hash maps — cooked to perfection with real performance data.*

---

## 🍳 Overview

**hashbrowns** is a self-contained C++ project that benchmarks the performance of three fundamental data structures:

- **Array** (contiguous memory)
- **Linked List** (pointer-based dynamic nodes)
- **Hash Map** (manual hashing, open addressing / chaining)

It measures insertion, search, and deletion performance across different data sizes, providing insight into **when one data structure outperforms another** — for example, the size threshold where linked lists become more efficient than arrays for certain operations.

---

## 🧠 Why This Project

This project demonstrates deep understanding of:

- **Pointers and Memory Management** — manual allocation, deallocation, and cache effects.
- **Data Structures** — custom implementations of arrays, linked lists, and hash maps.
- **Polymorphism** — abstract base class with virtual interfaces to enable dynamic benchmarking without `if`/`switch`.
- **Algorithmic Complexity** — measuring real-world tradeoffs beyond Big-O.
- **Performance Benchmarking** — using `<chrono>` to collect timing data across runs.

It’s both educational and practical — useful for anyone learning C++ data structures or performance profiling.

---

## ⚙️ Features

- 🔹 **Custom Implementations** — no STL containers; all structures written from scratch.
- 🔹 **Polymorphic Design** — easily add new structures via inheritance.
- 🔹 **Benchmark Framework** — runs configurable timed tests.
- 🔹 **Interactive CLI** — pass arguments for test size, repetitions, or CSV output.
- 🔹 **Automatic Crossovers** — detects where each structure outperforms the others.
- 🔹 **CSV Output** — export timing results for plotting or further analysis.

---

## 🏗️ Project Structure

```
hashbrowns/
├── src/
│   ├── core/
│   │   ├── data_structure.h          # Abstract base class for polymorphism
│   │   ├── memory_manager.h/.cpp     # Custom allocators & leak detection
│   │   └── timer.h/.cpp              # High-resolution benchmarking timer
│   ├── structures/
│   │   ├── dynamic_array.h/.cpp      # Custom dynamic array implementation
│   │   ├── linked_list.h/.cpp        # Custom linked list implementation
│   │   └── hash_map.h/.cpp           # Custom hash map implementation
│   ├── benchmark/
│   │   ├── benchmark_suite.h/.cpp    # Main benchmarking framework
│   │   ├── test_runner.h/.cpp        # Test execution and coordination
│   │   └── stats_analyzer.h/.cpp     # Statistical analysis utilities
│   └── main.cpp                      # CLI entry point
├── tests/
│   ├── unit/                         # Unit tests for individual components
│   ├── integration/                  # Integration tests for full workflows
│   └── performance/                  # Performance regression tests
├── docs/
│   ├── api/                          # Generated API documentation
│   ├── tutorials/                    # Usage guides and examples
│   └── performance_analysis.md       # Detailed performance findings
├── scripts/
│   ├── build.sh                      # Build automation script
│   ├── run_benchmarks.sh             # Benchmark execution script
│   └── plot_results.py               # Data visualization utilities
├── .github/workflows/                # CI/CD configuration
├── CMakeLists.txt                    # Modern CMake build configuration
└── README.md                         # This file
```

---

## 🚀 Getting Started

### **Prerequisites**

- **C++17 compatible compiler** (GCC 8+, Clang 6+, MSVC 2019+)
- **CMake 3.16 or higher**
- **Git** for cloning the repository

Optional tools for enhanced development:
- **clang-format** for code formatting
- **clang-tidy** for static analysis
- **Valgrind** for memory debugging (Linux/macOS)
- **Doxygen** for documentation generation

### **Quick Build**

```bash
# Clone the repository
git clone https://github.com/yourusername/hashbrowns.git
cd hashbrowns

# Create build directory
mkdir build && cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run the benchmarks
./hashbrowns --size 10000 --runs 10
```

### **Build Options**

```bash
# Debug build with sanitizers
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build with optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build with custom install prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local

# Enable static analysis
cmake .. -DCMAKE_CXX_CLANG_TIDY=clang-tidy
```

### **Available Targets**

```bash
# Build everything
make all

# Run all tests
make test

# Generate documentation
make docs

# Format code
make format

# Run memory checks
make memcheck

# Clean build artifacts
make clean-all

# Run benchmarks
make benchmark
```

---

## 📊 Usage Examples

### **Basic Benchmarking**

```bash
# Compare all structures with 50,000 elements
./hashbrowns --size 50000 --runs 20

# Test specific structures only
./hashbrowns --structures array,hashmap --size 10000

# Export results to CSV
./hashbrowns --size 25000 --runs 15 --output results.csv

# Verbose output with memory statistics
./hashbrowns --size 10000 --verbose --memory-tracking
```

### **Performance Analysis**

```bash
# Find crossover points between structures
./hashbrowns --crossover-analysis --max-size 100000

# Test with different operation patterns
./hashbrowns --pattern sequential --size 20000
./hashbrowns --pattern random --size 20000
./hashbrowns --pattern mixed --size 20000

# Memory usage profiling
./hashbrowns --memory-profile --size 50000
```

---

## �️ Scripts

Automate benchmarking and analysis without typing long commands:

- scripts/run_benchmarks.sh — builds if needed, runs benchmarks and crossover analysis, and writes CSVs.
    - Examples:
        - scripts/run_benchmarks.sh --runs 20 --size 50000
        - STRUCTURES=array,hashmap scripts/run_benchmarks.sh --max-size 200000
    - Outputs:
        - build/benchmark_results.csv
        - build/crossover_results.csv

- scripts/analyze_results.py — prints a concise summary from the CSV outputs.
    - Example:
        - scripts/analyze_results.py --bench-csv build/benchmark_results.csv --cross-csv build/crossover_results.csv

These require no external Python dependencies (standard library only).

---

## �🔬 Technical Deep Dive

### **Memory Management**

The project showcases advanced C++ memory management:

- **Custom Allocators**: Tracked allocators for leak detection and performance analysis
- **RAII Wrappers**: `unique_array<T>` for safe raw memory handling
- **Memory Pools**: Efficient object allocation for linked list nodes
- **Leak Detection**: Comprehensive tracking of allocations and deallocations

### **Polymorphic Design**

All data structures inherit from a common `DataStructure` interface:

```cpp
class DataStructure {
public:
    virtual void insert(int key, const std::string& value) = 0;
    virtual bool search(int key, std::string& value) const = 0;
    virtual bool remove(int key) = 0;
    virtual size_t memory_usage() const = 0;
    // ... additional interface methods
};
```

This enables:
- ✅ Runtime polymorphism for dynamic benchmarking
- ✅ Consistent interface across all implementations
- ✅ Easy addition of new data structures
- ✅ Type-safe operations without casting

### **Benchmarking Framework**

High-precision timing with statistical analysis:

- **Microsecond precision** using `std::chrono::high_resolution_clock`
- **Outlier detection** with configurable Z-score thresholds
- **Warm-up periods** to eliminate cache cold-start effects
- **Statistical aggregation** (mean, median, standard deviation)

### **Algorithm Complexity Verification**

The benchmarks empirically verify theoretical complexity:

- **O(1) vs O(n)**: Hash map vs array search performance
- **Cache effects**: Contiguous vs pointer-based memory access
- **Crossover analysis**: Finding size thresholds where algorithms change dominance

---

## 📈 Expected Results

### **Performance Characteristics**

| Operation | Dynamic Array | Linked List | Hash Map |
|-----------|---------------|-------------|----------|
| Insert    | O(1) amortized | O(1) | O(1) average |
| Search    | O(n) | O(n) | O(1) average |
| Remove    | O(n) | O(1) | O(1) average |
| Memory    | Contiguous | Fragmented | Moderate overhead |

### **Typical Crossover Points**

- **Array vs Hash Map (Search)**: ~1,000 elements
- **Array vs Linked List (Insert)**: ~10,000 elements  
- **Memory efficiency**: Array > Hash Map > Linked List

*Note: Actual results depend on hardware, compiler optimizations, and data patterns*

---

## 🤝 Contributing

Contributions are welcome! Areas for enhancement:

- **New Data Structures**: Trees, heaps, skip lists
- **Advanced Hash Functions**: MurmurHash, CityHash implementations
- **Visualization**: Real-time performance plotting
- **Platform Support**: Windows-specific optimizations
- **Benchmarking**: Additional operation patterns and metrics

---

## 📄 License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

---

## 🎯 Learning Outcomes

This project demonstrates mastery of:

- ✅ **Low-level Memory Management** in modern C++
- ✅ **Polymorphic Design Patterns** for extensible architectures  
- ✅ **Performance Engineering** and empirical analysis
- ✅ **Template Metaprogramming** for type-safe generics
- ✅ **Professional C++ Development** with modern tooling

Perfect for portfolios, technical interviews, and advanced C++ learning!

```

