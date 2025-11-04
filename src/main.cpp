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
#include "structures/linked_list.h"
#include "structures/hash_map.h"
#include "benchmark/benchmark_suite.h"
#include <optional>

using namespace hashbrowns;

void demonstrate_dynamic_array();
static int run_op_tests(const std::vector<std::string>& names, std::size_t size);

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
    --structures LIST     Comma-separated list: array,slist,dlist,hashmap
  --output FILE         Export results to CSV file
  --memory-tracking     Enable detailed memory analysis
    --crossover-analysis  Find performance crossover points
    --max-size N          Max size to analyze for crossovers (default: 100000)
        --series-runs N       Runs per size during crossover analysis (default: 1)
    --pattern TYPE        Data pattern for keys: sequential, random, mixed (default: sequential)
    --seed N              RNG seed used when pattern is random/mixed (default: random_device)
    --max-seconds N       Time budget for crossover sweep; stop early when exceeded
        --out-format F        csv|json (default: csv)
        --hash-strategy S     open|chain (default: open)
        --hash-capacity N     initial capacity for hashmap (power-of-two rounded)
        --hash-load F         max load factor (applies to both strategies)
  --verbose             Detailed output
  --help               Show this help message

EXAMPLES:
  hashbrowns --size 50000 --runs 20
  hashbrowns --structures array,hashmap --output results.csv
  hashbrowns --crossover-analysis --max-size 100000
)" << std::endl;
}

int main(int argc, char* argv[]) {
    // Pre-scan for banner/verbosity flags before printing anything
    bool no_banner = false;
    bool quiet = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-banner") no_banner = true;
        if (arg == "--quiet") { quiet = true; no_banner = true; }
    }

    if (!no_banner) {
        print_banner();
    }

    // Parse basic command line arguments
    bool show_help = false;
    bool demo_mode = true;
    std::size_t opt_size = 10000;
    int opt_runs = 10;
    int opt_series_runs = -1; // if <0, choose default based on opt_runs
    std::vector<std::string> opt_structures;
    std::optional<std::string> opt_output;
    bool opt_memory_tracking = false;
    bool opt_crossover = false;
    std::size_t opt_max_size = 100000;
    std::optional<unsigned long long> opt_seed;
    BenchmarkConfig::Pattern opt_pattern = BenchmarkConfig::Pattern::SEQUENTIAL;
    std::optional<double> opt_max_seconds;
    BenchmarkConfig::OutputFormat opt_out_fmt = BenchmarkConfig::OutputFormat::CSV;
    HashStrategy opt_hash_strategy = HashStrategy::OPEN_ADDRESSING;
    std::optional<std::size_t> opt_hash_capacity;
    std::optional<double> opt_hash_load;
    bool opt_op_tests = false;
    
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
        } else if (arg == "--series-runs" && i + 1 < argc) {
            opt_series_runs = std::stoi(argv[++i]);
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
        } else if (arg == "--crossover-analysis") {
            opt_crossover = true;
            demo_mode = false;
        } else if (arg == "--max-size" && i + 1 < argc) {
            opt_max_size = static_cast<std::size_t>(std::stoull(argv[++i]));
            demo_mode = false;
        } else if (arg == "--pattern" && i + 1 < argc) {
            std::string p = argv[++i];
            if (p == "sequential") opt_pattern = BenchmarkConfig::Pattern::SEQUENTIAL;
            else if (p == "random") opt_pattern = BenchmarkConfig::Pattern::RANDOM;
            else if (p == "mixed") opt_pattern = BenchmarkConfig::Pattern::MIXED;
            demo_mode = false;
        } else if (arg == "--seed" && i + 1 < argc) {
            opt_seed = static_cast<unsigned long long>(std::stoull(argv[++i]));
            demo_mode = false;
        } else if (arg == "--max-seconds" && i + 1 < argc) {
            opt_max_seconds = std::stod(argv[++i]);
            demo_mode = false;
        } else if (arg == "--out-format" && i + 1 < argc) {
            std::string f = argv[++i];
            if (f == "json") opt_out_fmt = BenchmarkConfig::OutputFormat::JSON; else opt_out_fmt = BenchmarkConfig::OutputFormat::CSV;
            demo_mode = false;
        } else if (arg == "--hash-strategy" && i + 1 < argc) {
            std::string s = argv[++i];
            if (s == "open") opt_hash_strategy = HashStrategy::OPEN_ADDRESSING; else if (s == "chain") opt_hash_strategy = HashStrategy::SEPARATE_CHAINING;
            demo_mode = false;
        } else if (arg == "--hash-capacity" && i + 1 < argc) {
            opt_hash_capacity = static_cast<std::size_t>(std::stoull(argv[++i]));
            demo_mode = false;
        } else if (arg == "--hash-load" && i + 1 < argc) {
            opt_hash_load = std::stod(argv[++i]);
            demo_mode = false;
        } else if (arg == "--op-tests") {
            opt_op_tests = true;
            demo_mode = false;
        } else if (arg == "--no-banner") {
            // already handled
            demo_mode = false;
        } else if (arg == "--quiet") {
            // already handled; treat as non-demo to avoid extra prints
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
    } else {
        if (opt_op_tests) {
            auto names = opt_structures.empty() ? std::vector<std::string>{"array","slist","dlist","hashmap"} : opt_structures;
            return run_op_tests(names, opt_size);
        }
    // Run benchmarks
        BenchmarkConfig cfg;
        cfg.size = opt_size;
    cfg.runs = opt_runs;
    cfg.verbose = false;
        cfg.csv_output = opt_output;
    cfg.structures = opt_structures.empty() ? std::vector<std::string>{"array","slist","dlist","hashmap"} : opt_structures;
    cfg.pattern = opt_pattern;
    cfg.seed = opt_seed;
    cfg.output_format = opt_out_fmt;
    cfg.hash_strategy = opt_hash_strategy;
    cfg.hash_initial_capacity = opt_hash_capacity;
    cfg.hash_max_load_factor = opt_hash_load;

        if (opt_memory_tracking) {
            MemoryTracker::instance().set_detailed_tracking(true);
            MemoryTracker::instance().reset();
        }

        BenchmarkSuite suite;
        if (!opt_crossover) {
            auto results = suite.run(cfg);
            if (!quiet) {
                std::cout << "\n=== Benchmark Results (avg ms over " << opt_runs << " runs, size=" << opt_size << ") ===\n";
                for (const auto& r : results) {
                    std::cout << "- " << r.structure
                              << ": insert=" << r.insert_ms_mean
                              << ", search=" << r.search_ms_mean
                              << ", remove=" << r.remove_ms_mean
                              << ", mem=" << r.memory_bytes << " bytes\n";
                }
                if (opt_output) {
                    std::cout << "\nSaved " << (opt_out_fmt==BenchmarkConfig::OutputFormat::CSV?"CSV":"JSON") << " to: " << *opt_output << "\n";
                }
            }
            return results.empty() ? 1 : 0;
        } else {
            // Crossover analysis mode
            std::vector<std::size_t> sizes;
            for (std::size_t s = 512; s <= opt_max_size; s *= 2) sizes.push_back(s);
            // Reduce runs for the series to speed up sweeping large sizes
            int series_runs = (opt_series_runs > 0) ? opt_series_runs : 1; // default to 1 for fast sweep
            cfg.runs = series_runs;
            // Time-bounded series run
            auto start = std::chrono::steady_clock::now();
            BenchmarkSuite::Series series;
            for (auto s : sizes) {
                cfg.size = s;
                auto res = suite.run(cfg);
                for (const auto& r : res) {
                    series.push_back(BenchmarkSuite::SeriesPoint{ s, r.structure, r.insert_ms_mean, r.search_ms_mean, r.remove_ms_mean });
                }
                if (opt_max_seconds) {
                    auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
                    if (elapsed >= *opt_max_seconds) {
                        if (!quiet) {
                            std::cout << "[INFO] Crossover sweep stopped early after " << elapsed << "s due to --max-seconds budget\n";
                        }
                        break;
                    }
                }
            }
            auto cx = suite.compute_crossovers(series);
            if (!quiet) {
                std::cout << "\n=== Crossover Analysis (approximate sizes) ===\n";
                std::cout << "(runs per size: " << series_runs << ")\n";
                for (const auto& c : cx) {
                    std::cout << c.operation << ": " << c.a << " vs " << c.b << " -> ~" << c.size_at_crossover << " elements\n";
                }
                if (opt_output) {
                    if (opt_out_fmt == BenchmarkConfig::OutputFormat::CSV) suite.write_crossover_csv(*opt_output, cx);
                    else suite.write_crossover_json(*opt_output, cx);
                    std::cout << "\nSaved crossover " << (opt_out_fmt==BenchmarkConfig::OutputFormat::CSV?"CSV":"JSON") << " to: " << *opt_output << "\n";
                }
            } else if (opt_output) {
                if (opt_out_fmt == BenchmarkConfig::OutputFormat::CSV) suite.write_crossover_csv(*opt_output, cx);
                else suite.write_crossover_json(*opt_output, cx);
            }
            return cx.empty() ? 1 : 0;
        }
    }
    
    return 0;
}

static int run_op_tests(const std::vector<std::string>& names, std::size_t size) {
    using namespace std;
    cout << "\n=== Operation Tests (size=" << size << ") ===\n";
    for (const auto& name : names) {
        cout << name << ":\n";
        auto ds = hashbrowns::DataStructurePtr();
        // local make to avoid depending on benchmark_suite internals
        if (name == "array" || name == "dynamic-array") {
            ds = std::make_unique< hashbrowns::DynamicArray<std::pair<int,std::string>> >();
        } else if (name == "slist" || name == "list" || name == "singly-list") {
            ds = std::make_unique< hashbrowns::SinglyLinkedList<std::pair<int,std::string>> >();
        } else if (name == "dlist" || name == "doubly-list") {
            ds = std::make_unique< hashbrowns::DoublyLinkedList<std::pair<int,std::string>> >();
        } else if (name == "hashmap" || name == "hash-map") {
            ds = std::make_unique< hashbrowns::HashMap >(hashbrowns::HashStrategy::OPEN_ADDRESSING);
        }
        if (!ds) { cout << "  (unknown structure)\n"; continue; }
        vector<int> keys(size);
        for (size_t i = 0; i < size; ++i) keys[i] = static_cast<int>(i);
        hashbrowns::Timer t; std::string v;
        // Insert
        t.start();
        for (auto k : keys) ds->insert(k, to_string(k));
        auto ins_us = t.stop().count();
        // Search (verify)
        size_t found = 0;
        t.start();
        for (auto k : keys) { if (ds->search(k, v)) ++found; }
        auto sea_us = t.stop().count();
        // Remove
        size_t removed = 0;
        t.start();
        for (auto k : keys) { if (ds->remove(k)) ++removed; }
        auto rem_us = t.stop().count();
        cout << "  insert: " << (ins_us/1000.0) << " ms, count=" << keys.size() << "\n";
        cout << "  search: " << (sea_us/1000.0) << " ms, found=" << found << "/" << keys.size() << "\n";
        cout << "  remove: " << (rem_us/1000.0) << " ms, removed=" << removed << "/" << keys.size() << "\n";
    }
    return 0;
}