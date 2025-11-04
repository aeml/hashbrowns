#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <numeric>

#include "core/data_structure.h"
#include "core/memory_manager.h"
#include "core/timer.h"
#include "structures/dynamic_array.h"
#include "benchmark/benchmark_suite.h"
#include <optional>

using namespace hashbrowns;

void demonstrate_dynamic_array();

void print_banner() {
    std::cout << R"(
🥔 hashbrowns - C++ Data Structure Benchmarking Suite
======================================================

A crispy performance comparison of:
- Dynamic Arrays (contiguous memory)
- Linked Lists (pointer-based nodes)  
- Hash Maps (manual hashing implementation)

Core architecture initialized successfully!
)" << std::endl;
}

void demonstrate_core_features() {
    std::cout << "=== Core Features Demonstration ===\n\n";
    
    // Memory tracking demonstration
    std::cout << "1. Memory Tracking System:\n";
    auto& tracker = MemoryTracker::instance();
    tracker.set_detailed_tracking(true);
    tracker.reset();
    
    // Allocate some memory to show tracking
    {
        auto array = make_unique_array<int>(100);
        for (size_t i = 0; i < 100; ++i) {
            array[i] = static_cast<int>(i);
        }
        
        auto stats = tracker.get_stats();
        std::cout << "   - Allocated: " << stats.total_allocated << " bytes\n";
        std::cout << "   - Current usage: " << stats.current_usage << " bytes\n";
        std::cout << "   - Allocation count: " << stats.allocation_count << "\n";
    } // array goes out of scope here
    
    // Check final stats
    auto final_stats = tracker.get_stats();
    std::cout << "   - After cleanup: " << final_stats.current_usage << " bytes\n";
    
    // Timer demonstration
    std::cout << "\n2. High-Resolution Timer:\n";
    Timer timer;
    
    // Time a simple operation
    timer.start();
    volatile int sum = 0;
    for (int i = 0; i < 1000000; ++i) {
        sum += i;
    }
    auto duration = timer.stop();
    
    std::cout << "   - Million integer additions: " 
              << duration.count() / 1000.0 << " microseconds\n";
    
    // Scope timer demonstration
    {
        ScopeTimer scope_timer("Sleep operation", true);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "\n3. DynamicArray Demonstration:\n";
    demonstrate_dynamic_array();
    
    std::cout << "\n4. Memory Leak Check:\n";
    bool clean = tracker.check_leaks();
    if (clean) {
        std::cout << "   ✓ No memory leaks detected!\n";
    }
}

void demonstrate_dynamic_array() {
    std::cout << "   Creating DynamicArray with different growth strategies...\n";
    
    // Test different growth strategies
    std::vector<std::pair<GrowthStrategy, std::string>> strategies = {
        {GrowthStrategy::MULTIPLICATIVE_2_0, "2.0x Multiplicative"},
        {GrowthStrategy::MULTIPLICATIVE_1_5, "1.5x Multiplicative"},
        {GrowthStrategy::FIBONACCI, "Fibonacci"},
        {GrowthStrategy::ADDITIVE, "Additive"}
    };
    
    for (const auto& [strategy, name] : strategies) {
        std::cout << "   \n   Testing " << name << " growth:\n";
        
        DynamicArray<int> arr(strategy);
        Timer timer;
        
        timer.start();
        for (int i = 0; i < 1000; ++i) {
            arr.push_back(i);
        }
        auto insert_time = timer.stop();
        
        std::cout << "     - 1000 insertions: " << insert_time.count() / 1000.0 << " μs\n";
        std::cout << "     - Final size: " << arr.size() << ", capacity: " << arr.capacity() << "\n";
        std::cout << "     - Memory usage: " << arr.memory_usage() << " bytes\n";
        
        // Test STL compatibility
        timer.start();
        auto sum = std::accumulate(arr.begin(), arr.end(), 0);
        auto sum_time = timer.stop();
        
        std::cout << "     - Sum using iterators: " << sum << " (took " 
                  << sum_time.count() / 1000.0 << " μs)\n";
    }
    
    // Test with key-value pairs (DataStructure interface)
    std::cout << "\n   Testing DataStructure interface with key-value pairs:\n";
    DynamicArray<std::pair<int, std::string>> kv_array;
    
    kv_array.insert(1, "first");
    kv_array.insert(2, "second");
    kv_array.insert(3, "third");
    
    std::string value;
    if (kv_array.search(2, value)) {
        std::cout << "     - Found key 2: " << value << "\n";
    }
    
    std::cout << "     - Array size: " << kv_array.size() << "\n";
    std::cout << "     - Complexity: insert=" << kv_array.insert_complexity()
              << ", search=" << kv_array.search_complexity() << "\n";
}

void show_usage() {
    std::cout << R"(
Usage: hashbrowns [OPTIONS]

OPTIONS:
  --size N              Set benchmark data size (default: 10000)
  --runs N              Number of benchmark runs (default: 10)
  --structures LIST     Comma-separated list: array,list,hashmap
  --output FILE         Export results to CSV file
  --pattern TYPE        Data pattern: sequential, random, mixed
  --memory-tracking     Enable detailed memory analysis
  --crossover-analysis  Find performance crossover points
  --verbose             Detailed output
  --help               Show this help message

EXAMPLES:
  hashbrowns --size 50000 --runs 20
  hashbrowns --structures array,hashmap --output results.csv
  hashbrowns --crossover-analysis --max-size 100000

NOTE: Data structure implementations are coming in the next phase!
      This build demonstrates the core architecture and benchmarking framework.
)" << std::endl;
}

int main(int argc, char* argv[]) {
    print_banner();
    
    // Parse basic command line arguments
    bool show_help = false;
    bool demo_mode = true;
    std::size_t opt_size = 10000;
    int opt_runs = 10;
    std::vector<std::string> opt_structures;
    std::optional<std::string> opt_output;
    bool opt_memory_tracking = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            show_help = true;
            demo_mode = false;
        } else if (arg == "--size" && i + 1 < argc) {
            opt_size = static_cast<std::size_t>(std::stoull(argv[++i]));
            demo_mode = false;
        } else if (arg == "--runs" && i + 1 < argc) {
            opt_runs = std::stoi(argv[++i]);
            demo_mode = false;
        } else if (arg == "--structures" && i + 1 < argc) {
            std::string list = argv[++i];
            size_t start = 0, pos;
            while ((pos = list.find(',', start)) != std::string::npos) {
                opt_structures.push_back(list.substr(start, pos - start));
                start = pos + 1;
            }
            if (start < list.size()) opt_structures.push_back(list.substr(start));
            demo_mode = false;
        } else if (arg == "--output" && i + 1 < argc) {
            opt_output = std::string(argv[++i]);
            demo_mode = false;
        } else if (arg == "--memory-tracking") {
            opt_memory_tracking = true;
            demo_mode = false;
        } else if (arg.find("--") == 0) {
            demo_mode = false;
        }
    }
    
    if (show_help) {
        show_usage();
        return 0;
    }
    
    if (demo_mode) {
        std::cout << "Running in demonstration mode...\n";
        demonstrate_core_features();
        std::cout << "\nRun with --help to see available options.\n";
        std::cout << "Full benchmarking capabilities will be available once data structures are implemented!\n";
    } else {
    // Run benchmarks
        BenchmarkConfig cfg;
        cfg.size = opt_size;
        cfg.runs = opt_runs;
        cfg.verbose = false;
        cfg.csv_output = opt_output;
        cfg.structures = opt_structures.empty() ? std::vector<std::string>{"array","slist","hashmap"} : opt_structures;

        if (opt_memory_tracking) {
            MemoryTracker::instance().set_detailed_tracking(true);
            MemoryTracker::instance().reset();
        }

        BenchmarkSuite suite;
        auto results = suite.run(cfg);

        std::cout << "\n=== Benchmark Results (avg ms over " << opt_runs << " runs, size=" << opt_size << ") ===\n";
        for (const auto& r : results) {
            std::cout << "- " << r.structure
                      << ": insert=" << r.insert_ms_mean
                      << ", search=" << r.search_ms_mean
                      << ", remove=" << r.remove_ms_mean
                      << ", mem=" << r.memory_bytes << " bytes\n";
        }
        if (opt_output) {
            std::cout << "\nSaved CSV to: " << *opt_output << "\n";
        }
        return results.empty() ? 1 : 0;
    }
    
    return 0;
}