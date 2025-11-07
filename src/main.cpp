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
static int run_wizard();

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
    --size N              Set benchmark data size (default: 10000). Acts as MAX size when --series-count>1
    --runs N              Number of benchmark runs (default: 10) (per size when series enabled)
    --series-count N      If >1: run a linear multi-size series up to --size (treated as max). Example: --size 10000 --series-count 4 -> sizes 2500,5000,7500,10000
    --series-out FILE     Output file for multi-size series (default: results/csvs/series_results.csv|json)
        --warmup N            Discard first N runs (warm-up) from timing stats (default: 0)
        --bootstrap N         Bootstrap iterations for mean CI (0=disabled; recommend 200-1000) (default: 0)
    --sizes N             (Wizard alt) Treat size as max and run linearly spaced sizes (interactive in --wizard)
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
    --wizard             Interactive mode to choose structures and settings

EXAMPLES:
  hashbrowns --size 50000 --runs 20
    hashbrowns --size 10000 --series-count 5 --runs 5 --series-out results/csvs/series_results.csv
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
    bool wizard_mode = false;
    std::size_t opt_size = 10000;
    int opt_runs = 10;
    int opt_series_count = 0;
    std::optional<std::string> opt_series_out;
    int opt_warmup = 0;
    int opt_bootstrap = 0;
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
        } else if (arg == "--wizard" || arg == "-wizard") {
            wizard_mode = true;
            demo_mode = false;
        } else if (arg == "--size" && i + 1 < argc) {
            opt_size = static_cast<std::size_t>(std::stoull(argv[++i]));
            demo_mode = false;
        } else if (arg == "--runs" && i + 1 < argc) {
            opt_runs = std::stoi(argv[++i]);
            demo_mode = false;
        } else if (arg == "--warmup" && i + 1 < argc) {
            opt_warmup = std::stoi(argv[++i]);
            demo_mode = false;
        } else if (arg == "--bootstrap" && i + 1 < argc) {
            opt_bootstrap = std::stoi(argv[++i]);
            demo_mode = false;
        } else if (arg == "--series-count" && i + 1 < argc) {
            opt_series_count = std::stoi(argv[++i]);
            demo_mode = false;
        } else if (arg == "--series-out" && i + 1 < argc) {
            opt_series_out = std::string(argv[++i]);
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
    
    if (wizard_mode) {
        return run_wizard();
    }

    if (demo_mode) {
        std::cout << "Running in demonstration mode...\n";
        demonstrate_core_features();
        std::cout << "\nRun with --help to see available options.\n";
    } else {
        // Validate requested structures early for user-friendly errors
        if (!opt_structures.empty()) {
            std::vector<std::string> valid = {"array","dynamic-array","slist","list","singly-list","dlist","doubly-list","hashmap","hash-map"};
            auto is_valid = [&valid](const std::string& s){
                for (const auto& v : valid) if (s == v) return true; return false;
            };
            std::vector<std::string> bad;
            for (const auto& s : opt_structures) if (!is_valid(s)) bad.push_back(s);
            if (!bad.empty()) {
                std::cerr << "Error: unknown structure name(s): ";
                for (size_t i=0;i<bad.size();++i) {
                    std::cerr << bad[i] << (i+1<bad.size()?", ":"");
                }
                std::cerr << "\nValid options: array, dynamic-array, slist, list, singly-list, dlist, doubly-list, hashmap, hash-map\n";
                return 2;
            }
        }

        if (opt_op_tests) {
            auto names = opt_structures.empty() ? std::vector<std::string>{"array","slist","dlist","hashmap"} : opt_structures;
            return run_op_tests(names, opt_size);
        }
    // Run benchmarks
        BenchmarkConfig cfg;
        cfg.size = opt_size;
    cfg.runs = opt_runs;
    cfg.warmup_runs = opt_warmup;
    cfg.bootstrap_iters = opt_bootstrap;
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
        if (!opt_crossover && opt_series_count <= 1) {
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
        } else if (opt_crossover) {
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
                    else suite.write_crossover_json(*opt_output, cx, cfg);
                    std::cout << "\nSaved crossover " << (opt_out_fmt==BenchmarkConfig::OutputFormat::CSV?"CSV":"JSON") << " to: " << *opt_output << "\n";
                }
            } else if (opt_output) {
                if (opt_out_fmt == BenchmarkConfig::OutputFormat::CSV) suite.write_crossover_csv(*opt_output, cx);
                else suite.write_crossover_json(*opt_output, cx, cfg);
            }
            return cx.empty() ? 1 : 0;
        } else if (opt_series_count > 1) {
            // Multi-size linear series benchmark
            std::vector<std::size_t> sizes;
            double step = static_cast<double>(opt_size) / opt_series_count;
            for (int i = 1; i <= opt_series_count; ++i) sizes.push_back(static_cast<std::size_t>(std::llround(step * i)));
            BenchmarkSuite::Series series;
            for (auto s : sizes) {
                cfg.size = s;
                auto res = suite.run(cfg);
                if (!quiet) {
                    std::cout << "\n-- Size " << s << " --\n";
                    for (const auto& r : res) {
                        std::cout << r.structure
                                  << ": insert=" << r.insert_ms_mean
                                  << ", search=" << r.search_ms_mean
                                  << ", remove=" << r.remove_ms_mean
                                  << ", mem=" << r.memory_bytes << " bytes\n";
                    }
                }
                for (const auto& r : res) series.push_back({ s, r.structure, r.insert_ms_mean, r.search_ms_mean, r.remove_ms_mean });
            }
            if (!quiet) {
                std::cout << "\n=== Series Summary (sizes=";
                for (auto s : sizes) std::cout << s << ";";
                std::cout << ") runs-per-size=" << opt_runs << " ===\n";
            }
            // Write series output
            std::string out_path;
            if (opt_series_out) out_path = *opt_series_out;
            else {
                out_path = (opt_out_fmt==BenchmarkConfig::OutputFormat::CSV ? "results/csvs/series_results.csv" : "results/csvs/series_results.json");
            }
            if (opt_out_fmt == BenchmarkConfig::OutputFormat::CSV) {
                suite.write_series_csv(out_path, series);
            } else {
                suite.write_series_json(out_path, series, cfg);
            }
            if (!quiet) std::cout << "Saved series " << (opt_out_fmt==BenchmarkConfig::OutputFormat::CSV?"CSV":"JSON") << " to: " << out_path << "\n";
        }
    }
    
    return 0;
}

static std::string prompt_line(const std::string& question, const std::string& def = "") {
    std::string ans;
    std::cout << question;
    if (!def.empty()) std::cout << " [" << def << "]";
    std::cout << ": ";
    std::getline(std::cin, ans);
    if (ans.empty()) return def;
    return ans;
}

static bool prompt_yesno(const std::string& question, bool def = false) {
    std::string defStr = def ? "Y/n" : "y/N";
    while (true) {
        std::string a = prompt_line(question + " (" + defStr + ")");
        if (a.empty()) return def;
        for (auto& c : a) c = static_cast<char>(::tolower(c));
        if (a == "y" || a == "yes") return true;
        if (a == "n" || a == "no") return false;
        std::cout << "Please answer 'y' or 'n'.\n";
    }
}

static std::vector<std::string> split_list(const std::string& s) {
    std::vector<std::string> out;
    std::size_t start = 0, pos = 0;
    while ((pos = s.find(',', start)) != std::string::npos) {
        auto t = s.substr(start, pos - start);
        if (!t.empty()) out.push_back(t);
        start = pos + 1;
    }
    if (start < s.size()) out.push_back(s.substr(start));
    return out;
}

static int run_wizard() {
    using std::cout; using std::string; using std::vector;
    cout << "\n=== Wizard Mode ===\n";
    cout << "Answer with values or press Enter for defaults.\n\n";

    // Mode selection
    string mode = prompt_line("Mode [benchmark|crossover]", "benchmark");
    for (auto& c : mode) c = static_cast<char>(::tolower(c));
    bool crossover = (mode == "crossover" || mode == "sweep");

    // Structures
    string structs = prompt_line("Structures (comma list or 'all')", "all");
    vector<string> structures;
    if (structs == "all") {
        structures = {"array","slist","dlist","hashmap"};
    } else {
        structures = split_list(structs);
        if (structures.empty()) structures = {"array","slist","dlist","hashmap"};
    }

    // Basics (interpret size as max if multi-size requested)
    string size_s = prompt_line("Max size", "10000");
    std::size_t max_size = static_cast<std::size_t>(std::stoull(size_s));
    string runs_s = prompt_line("Sizes count (number of distinct sizes)", "10");
    int size_count = std::stoi(runs_s);
    string reps_s = prompt_line("Runs per size", "10");
    int runs_per_size = std::stoi(reps_s);

    // Pattern and seed
    string pattern = prompt_line("Pattern [sequential|random|mixed]", "sequential");
    BenchmarkConfig::Pattern pat = BenchmarkConfig::Pattern::SEQUENTIAL;
    if (pattern == "random") pat = BenchmarkConfig::Pattern::RANDOM;
    else if (pattern == "mixed") pat = BenchmarkConfig::Pattern::MIXED;
    string seed_s = prompt_line("Seed (blank=random)", "");
    std::optional<unsigned long long> seed;
    if (!seed_s.empty()) seed = static_cast<unsigned long long>(std::stoull(seed_s));

    // Output
    string fmt = prompt_line("Output format [csv|json]", "csv");
    for (auto& c : fmt) c = static_cast<char>(::tolower(c));
    BenchmarkConfig::OutputFormat outfmt = (fmt=="json" ? BenchmarkConfig::OutputFormat::JSON : BenchmarkConfig::OutputFormat::CSV);
        // default series/bench outputs under results/csvs
        string def_out = (outfmt==BenchmarkConfig::OutputFormat::CSV ? "results/csvs/benchmark_results.csv" : "results/csvs/benchmark_results.json");
    string out_path = prompt_line(std::string("Output file (blank=skip, default=") + def_out + ")", def_out);
    if (out_path == "skip" || out_path == "none") out_path.clear();

    // HashMap tuning
    string hs = prompt_line("Hash strategy [open|chain]", "open");
    HashStrategy hstrat = (hs=="chain" ? HashStrategy::SEPARATE_CHAINING : HashStrategy::OPEN_ADDRESSING);
    string hcap = prompt_line("Hash initial capacity (blank=default)", "");
    std::optional<std::size_t> hash_cap; if (!hcap.empty()) hash_cap = static_cast<std::size_t>(std::stoull(hcap));
    string hload = prompt_line("Hash max load factor (blank=default)", "");
    std::optional<double> hash_load; if (!hload.empty()) hash_load = std::stod(hload);

    BenchmarkSuite suite;
    BenchmarkConfig cfg;
    cfg.size = max_size; // updated per iteration if multi-size
    cfg.runs = runs_per_size;
    cfg.structures = structures;
    cfg.pattern = pat;
    cfg.seed = seed;
    cfg.output_format = outfmt;
    cfg.hash_strategy = hstrat;
    cfg.hash_initial_capacity = hash_cap;
    cfg.hash_max_load_factor = hash_load;
    if (!out_path.empty()) cfg.csv_output = out_path;

    if (!crossover) {
        if (size_count <= 1) {
            auto results = suite.run(cfg);
            cout << "\n=== Benchmark Results (avg ms over " << runs_per_size << ", size=" << max_size << ") ===\n";
            for (const auto& r : results) {
                cout << "- " << r.structure
                     << ": insert=" << r.insert_ms_mean
                     << ", search=" << r.search_ms_mean
                     << ", remove=" << r.remove_ms_mean
                     << ", mem=" << r.memory_bytes << " bytes\n";
            }
            if (cfg.csv_output) {
                cout << "\nSaved " << (outfmt==BenchmarkConfig::OutputFormat::CSV?"CSV":"JSON") << " to: " << *cfg.csv_output << "\n";
            }
            return results.empty() ? 1 : 0;
        } else {
            // Generate linearly spaced sizes: e.g. count=4, max=10000 -> 2500,5000,7500,10000
            std::vector<std::size_t> sizes;
            double step = static_cast<double>(max_size) / size_count;
            for (int i = 1; i <= size_count; ++i) sizes.push_back(static_cast<std::size_t>(std::llround(step * i)));
            BenchmarkSuite::Series series;
            for (auto s : sizes) {
                cfg.size = s;
                auto res = suite.run(cfg);
                // Print results for this size inline
                cout << "\n-- Size " << s << " --\n";
                for (const auto& r : res) {
                    cout << r.structure
                         << ": insert=" << r.insert_ms_mean
                         << ", search=" << r.search_ms_mean
                         << ", remove=" << r.remove_ms_mean
                         << ", mem=" << r.memory_bytes << " bytes\n";
                }
                for (const auto& r : res) series.push_back({ s, r.structure, r.insert_ms_mean, r.search_ms_mean, r.remove_ms_mean });
            }
            cout << "\n=== Multi-Size Benchmark Series (runs per size=" << runs_per_size << ") ===\n";
            for (auto s : sizes) cout << " size=" << s;
            cout << "\n";
            // Summarize last size results (already captured) and mention series file
            if (cfg.csv_output) {
                std::string path = *cfg.csv_output;
                if (outfmt == BenchmarkConfig::OutputFormat::CSV) suite.write_series_csv(path, series);
                else suite.write_series_json(path, series, cfg);
                cout << "\nSaved series " << (outfmt==BenchmarkConfig::OutputFormat::CSV?"CSV":"JSON") << " to: " << path << "\n";
                // Offer to generate plots
                if (outfmt == BenchmarkConfig::OutputFormat::CSV) {
                    bool gen = prompt_yesno("Generate series plots now?", true);
                    if (gen) {
                            // ensure results/plots exists
                            std::string cmd = "python3 scripts/plot_results.py --series-csv '" + path + "' --out-dir results/plots --yscale auto --note 'wizard series'";
                        int rc = std::system(cmd.c_str());
                        if (rc != 0) std::cout << "[WARN] Plotting command failed; ensure Python/matplotlib are installed.\n";
                    }
                } else {
                    std::cout << "[INFO] Skipping plots: plotting supports CSV only.\n";
                }
            } else {
                // Print a brief table if no output file
                for (const auto& p : series) {
                    cout << p.size << ": " << p.structure << " ins=" << p.insert_ms << " sea=" << p.search_ms << " rem=" << p.remove_ms << "\n";
                }
            }
            return series.empty() ? 1 : 0;
        }
    }

    // Crossover path
    string maxsz_s = prompt_line("Max size (sweep)", "100000");
    std::size_t maxsz = static_cast<std::size_t>(std::stoull(maxsz_s));
    string srun_s = prompt_line("Series runs per size", "1");
    int srun = std::stoi(srun_s);
    string tbudget_s = prompt_line("Time budget seconds (blank=no cap)", "");
    std::optional<double> tbudget; if (!tbudget_s.empty()) tbudget = std::stod(tbudget_s);
    cfg.runs = srun;

    // Default crossover output name if none
    if (!cfg.csv_output) {
        std::string defcx = (outfmt==BenchmarkConfig::OutputFormat::CSV ? "results/csvs/crossover_results.csv" : "results/csvs/crossover_results.json");
        std::string cxout = prompt_line(std::string("Crossover output file (blank=default= ") + defcx + ")", defcx);
        if (!cxout.empty()) cfg.csv_output = cxout; // reuse same member
    }

    std::vector<std::size_t> sizes;
    for (std::size_t s = 512; s <= maxsz; s *= 2) sizes.push_back(s);
    auto start = std::chrono::steady_clock::now();
    BenchmarkSuite::Series series;
    for (auto s : sizes) {
        cfg.size = s;
        auto res = suite.run(cfg);
        for (const auto& r : res) series.push_back({ s, r.structure, r.insert_ms_mean, r.search_ms_mean, r.remove_ms_mean });
        if (tbudget) {
            double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            if (elapsed >= *tbudget) { std::cout << "[INFO] Stopping early due to time budget" << std::endl; break; }
        }
    }
    auto cx = suite.compute_crossovers(series);
    cout << "\n=== Crossover Analysis (approximate sizes) ===\n";
    cout << "(runs per size: " << srun << ")\n";
    for (const auto& c : cx) cout << c.operation << ": " << c.a << " vs " << c.b << " -> ~" << c.size_at_crossover << " elements\n";
    if (cfg.csv_output) {
        if (outfmt == BenchmarkConfig::OutputFormat::CSV) suite.write_crossover_csv(*cfg.csv_output, cx);
        else suite.write_crossover_json(*cfg.csv_output, cx, cfg);
        cout << "\nSaved crossover " << (outfmt==BenchmarkConfig::OutputFormat::CSV?"CSV":"JSON") << " to: " << *cfg.csv_output << "\n";
    }
    return cx.empty() ? 1 : 0;
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