#include <chrono>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#ifdef __linux__
#include <sched.h>
#endif
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "benchmark/benchmark_suite.h"
#include "cli/cli_args.h"
#include "cli/cli_handlers.h"
#include "core/data_structure.h"
#include "core/memory_manager.h"
#include "core/timer.h"
#include "structures/dynamic_array.h"
#include "structures/hash_map.h"
#include "structures/linked_list.h"

#include <fstream>
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

    std::cout << "   - Million integer additions: " << duration.count() / 1000.0 << " microseconds\n";

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
        {GrowthStrategy::ADDITIVE, "Additive"}};

    for (const auto& [strategy, name] : strategies) {
        std::cout << "   \n   Testing " << name << " growth:\n";

        DynamicArray<int> arr(strategy);
        Timer             timer;

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
        auto sum      = std::accumulate(arr.begin(), arr.end(), 0);
        auto sum_time = timer.stop();

        std::cout << "     - Sum using iterators: " << sum << " (took " << sum_time.count() / 1000.0 << " μs)\n";
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
    --profile NAME        Canonical benchmark profile: smoke|ci|series|crossover|deep
    --series-sizes LIST   Explicit comma-separated sizes (overrides --series-count linear spacing). Example: --series-sizes 512,2048,8192
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
    --pin-cpu [IDX]      Pin process to CPU index (default 0 if IDX omitted) for reproducibility (Linux-only)
    --no-turbo           Attempt to disable CPU turbo boost (Linux-only, best-effort; may need root)
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
    hashbrowns --profile ci
    hashbrowns --size 10000 --series-count 5 --runs 5 --series-out results/csvs/series_results.csv
  hashbrowns --structures array,hashmap --output results.csv
  hashbrowns --crossover-analysis --max-size 100000
)" << std::endl;
}

int main(int argc, char* argv[]) {
    auto a = hashbrowns::cli::parse_args(argc, argv);

    const bool no_banner = a.no_banner;
    const bool quiet     = a.quiet;

    if (a.version_only) {
#ifdef HB_PROJECT_VERSION
#ifdef HB_GIT_SHA
        std::cout << "hashbrowns " << HB_PROJECT_VERSION << " (git " << HB_GIT_SHA << ")" << std::endl;
#else
        std::cout << "hashbrowns " << HB_PROJECT_VERSION << " (git unknown)" << std::endl;
#endif
#else
        std::cout << "hashbrowns (version unknown)" << std::endl;
#endif
        return 0;
    }

    if (!no_banner) {
        print_banner();
    }

    // Unpack parsed options into local names to minimise churn in the
    // rest of main() below.
    const bool        show_help              = a.show_help;
    const bool        demo_mode              = a.demo_mode;
    const bool        wizard_mode            = a.wizard_mode;
    const std::size_t opt_size               = a.opt_size;
    const int         opt_runs               = a.opt_runs;
    const int         opt_series_count       = a.opt_series_count;
    const auto        opt_series_out         = a.opt_series_out;
    const auto        opt_series_sizes       = a.opt_series_sizes;
    const auto        opt_profile            = a.opt_profile;
    const bool        opt_profile_valid      = a.opt_profile_valid;
    const int         opt_warmup             = a.opt_warmup;
    const int         opt_series_runs        = a.opt_series_runs;
    const bool        opt_pin_cpu            = a.opt_pin_cpu;
    const int         opt_cpu_index          = a.opt_cpu_index;
    const bool        opt_no_turbo           = a.opt_no_turbo;
    const auto        opt_structures         = a.opt_structures;
    const auto        opt_output             = a.opt_output;
    const bool        opt_memory_tracking    = a.opt_memory_tracking;
    const bool        opt_crossover          = a.opt_crossover;
    const std::size_t opt_max_size           = a.opt_max_size;
    const auto        opt_seed               = a.opt_seed;
    const auto        opt_pattern            = a.opt_pattern;
    const auto        opt_max_seconds        = a.opt_max_seconds;
    const auto        opt_out_fmt            = a.opt_out_fmt;
    const auto        opt_hash_strategy      = a.opt_hash_strategy;
    const auto        opt_hash_capacity      = a.opt_hash_capacity;
    const auto        opt_hash_load          = a.opt_hash_load;
    const bool        opt_op_tests           = a.opt_op_tests;
    const auto        opt_baseline_path      = a.opt_baseline_path;
    const double      opt_baseline_threshold = a.opt_baseline_threshold;
    const double      opt_baseline_noise     = a.opt_baseline_noise;
    const auto        opt_baseline_scope     = a.opt_baseline_scope;

    if (show_help) {
        show_usage();
        return 0;
    }

    if (wizard_mode) {
        return hashbrowns::cli::run_wizard();
    }

    if (demo_mode) {
        std::cout << "Running in demonstration mode...\n";
        demonstrate_core_features();
        std::cout << "\nRun with --help to see available options.\n";
    } else {
        if (opt_profile && !opt_profile_valid) {
            std::cerr << "Error: unknown benchmark profile '" << *opt_profile
                      << "'. Valid profiles: smoke, ci, series, crossover, deep\n";
            return 2;
        }

        // Validate requested structures early for user-friendly errors
        if (!opt_structures.empty()) {
            std::vector<std::string> valid    = {"array", "dynamic-array", "slist",   "list",    "singly-list",
                                                 "dlist", "doubly-list",   "hashmap", "hash-map"};
            auto                     is_valid = [&valid](const std::string& s) {
                for (const auto& v : valid) {
                    if (s == v)
                        return true;
                }
                return false;
            };
            std::vector<std::string> bad;
            for (const auto& s : opt_structures)
                if (!is_valid(s))
                    bad.push_back(s);
            if (!bad.empty()) {
                std::cerr << "Error: unknown structure name(s): ";
                for (size_t i = 0; i < bad.size(); ++i) {
                    std::cerr << bad[i] << (i + 1 < bad.size() ? ", " : "");
                }
                std::cerr << "\nValid options: array, dynamic-array, slist, list, singly-list, dlist, doubly-list, "
                             "hashmap, hash-map\n";
                return 2;
            }
        }

        if (opt_op_tests) {
            auto names = opt_structures.empty() ? std::vector<std::string>{"array", "slist", "dlist", "hashmap"}
                                                : opt_structures;
            return hashbrowns::cli::run_op_tests(names, opt_size);
        }

        auto contains_field = [](const std::vector<std::string>& fields, const std::string& field) {
            return std::find(fields.begin(), fields.end(), field) != fields.end();
        };
        auto add_field_once = [&](std::vector<std::string>& fields, const std::string& field) {
            if (!contains_field(fields, field))
                fields.push_back(field);
        };
        auto mark_override_if = [&](std::vector<std::string>& overrides, const std::string& field, bool condition) {
            if (condition)
                add_field_once(overrides, field);
        };

        std::vector<std::string> profile_applied_defaults;
        std::vector<std::string> profile_explicit_overrides;
        // Apply canonical benchmark profiles before running anything.
        if (opt_profile) {
            const auto& profile = *opt_profile;
            if (profile == "smoke") {
                if (a.opt_size == 10000) {
                    a.opt_size = 4096;
                    add_field_once(profile_applied_defaults, "size");
                } else {
                    add_field_once(profile_explicit_overrides, "size");
                }
                if (a.opt_runs == 10) {
                    a.opt_runs = 5;
                    add_field_once(profile_applied_defaults, "runs");
                } else {
                    add_field_once(profile_explicit_overrides, "runs");
                }
                if (a.opt_structures.empty()) {
                    a.opt_structures = {"array", "slist", "hashmap"};
                    add_field_once(profile_applied_defaults, "structures");
                } else {
                    add_field_once(profile_explicit_overrides, "structures");
                }
                if (!a.opt_output) {
                    a.opt_output = std::string("results/csvs/benchmark_results.csv");
                    add_field_once(profile_applied_defaults, "output");
                } else {
                    add_field_once(profile_explicit_overrides, "output");
                }
            } else if (profile == "ci") {
                if (a.opt_size == 10000) {
                    a.opt_size = 20000;
                    add_field_once(profile_applied_defaults, "size");
                } else {
                    add_field_once(profile_explicit_overrides, "size");
                }
                if (a.opt_runs == 10) {
                    add_field_once(profile_applied_defaults, "runs");
                } else {
                    add_field_once(profile_explicit_overrides, "runs");
                }
                if (a.opt_structures.empty()) {
                    a.opt_structures = {"array", "slist", "dlist", "hashmap"};
                    add_field_once(profile_applied_defaults, "structures");
                } else {
                    add_field_once(profile_explicit_overrides, "structures");
                }
                if (!a.opt_output) {
                    a.opt_output = std::string("results/csvs/benchmark_results.csv");
                    add_field_once(profile_applied_defaults, "output");
                } else {
                    add_field_once(profile_explicit_overrides, "output");
                }
                if (!a.opt_seed) {
                    a.opt_seed = 12345ULL;
                    add_field_once(profile_applied_defaults, "seed");
                } else {
                    add_field_once(profile_explicit_overrides, "seed");
                }
            } else if (profile == "series") {
                if (a.opt_size == 10000) {
                    a.opt_size = 60000;
                    add_field_once(profile_applied_defaults, "size");
                } else {
                    add_field_once(profile_explicit_overrides, "size");
                }
                if (a.opt_runs == 10) {
                    a.opt_runs = 3;
                    add_field_once(profile_applied_defaults, "runs");
                } else {
                    add_field_once(profile_explicit_overrides, "runs");
                }
                if (a.opt_series_count == 0) {
                    a.opt_series_count = 6;
                    add_field_once(profile_applied_defaults, "series_count");
                } else {
                    add_field_once(profile_explicit_overrides, "series_count");
                }
                if (a.opt_structures.empty()) {
                    a.opt_structures = {"array", "hashmap"};
                    add_field_once(profile_applied_defaults, "structures");
                } else {
                    add_field_once(profile_explicit_overrides, "structures");
                }
                if (a.opt_out_fmt == BenchmarkConfig::OutputFormat::CSV) {
                    a.opt_out_fmt = BenchmarkConfig::OutputFormat::JSON;
                    add_field_once(profile_applied_defaults, "out_format");
                } else {
                    add_field_once(profile_explicit_overrides, "out_format");
                }
                if (!a.opt_series_out) {
                    a.opt_series_out = std::string("results/csvs/series_results.json");
                    add_field_once(profile_applied_defaults, "series_out");
                } else {
                    add_field_once(profile_explicit_overrides, "series_out");
                }
            } else if (profile == "crossover") {
                if (!a.opt_crossover) {
                    a.opt_crossover = true;
                    add_field_once(profile_applied_defaults, "crossover_analysis");
                }
                if (a.opt_max_size == 100000) {
                    add_field_once(profile_applied_defaults, "max_size");
                } else {
                    add_field_once(profile_explicit_overrides, "max_size");
                }
                if (a.opt_runs == 10) {
                    a.opt_runs = 4;
                    add_field_once(profile_applied_defaults, "runs");
                } else {
                    add_field_once(profile_explicit_overrides, "runs");
                }
                if (a.opt_series_runs < 0) {
                    a.opt_series_runs = 1;
                    add_field_once(profile_applied_defaults, "series_runs");
                } else {
                    add_field_once(profile_explicit_overrides, "series_runs");
                }
                if (a.opt_structures.empty()) {
                    a.opt_structures = {"array", "slist", "dlist", "hashmap"};
                    add_field_once(profile_applied_defaults, "structures");
                } else {
                    add_field_once(profile_explicit_overrides, "structures");
                }
                if (a.opt_out_fmt == BenchmarkConfig::OutputFormat::CSV) {
                    a.opt_out_fmt = BenchmarkConfig::OutputFormat::JSON;
                    add_field_once(profile_applied_defaults, "out_format");
                } else {
                    add_field_once(profile_explicit_overrides, "out_format");
                }
                if (!a.opt_output) {
                    a.opt_output = std::string("results/csvs/crossover_results.json");
                    add_field_once(profile_applied_defaults, "output");
                } else {
                    add_field_once(profile_explicit_overrides, "output");
                }
            } else if (profile == "deep") {
                if (a.opt_size == 10000) {
                    a.opt_size = 50000;
                    add_field_once(profile_applied_defaults, "size");
                } else {
                    add_field_once(profile_explicit_overrides, "size");
                }
                if (a.opt_runs == 10) {
                    a.opt_runs = 20;
                    add_field_once(profile_applied_defaults, "runs");
                } else {
                    add_field_once(profile_explicit_overrides, "runs");
                }
                if (a.opt_structures.empty()) {
                    a.opt_structures = {"array", "slist", "dlist", "hashmap"};
                    add_field_once(profile_applied_defaults, "structures");
                } else {
                    add_field_once(profile_explicit_overrides, "structures");
                }
                if (!a.opt_memory_tracking) {
                    a.opt_memory_tracking = true;
                    add_field_once(profile_applied_defaults, "memory_tracking");
                } else {
                    add_field_once(profile_explicit_overrides, "memory_tracking");
                }
                if (a.opt_bootstrap == 0) {
                    a.opt_bootstrap = 400;
                    add_field_once(profile_applied_defaults, "bootstrap");
                } else {
                    add_field_once(profile_explicit_overrides, "bootstrap");
                }
                if (a.opt_out_fmt == BenchmarkConfig::OutputFormat::CSV) {
                    a.opt_out_fmt = BenchmarkConfig::OutputFormat::JSON;
                    add_field_once(profile_applied_defaults, "out_format");
                } else {
                    add_field_once(profile_explicit_overrides, "out_format");
                }
                if (!a.opt_output) {
                    a.opt_output = std::string("results/csvs/benchmark_results.json");
                    add_field_once(profile_applied_defaults, "output");
                } else {
                    add_field_once(profile_explicit_overrides, "output");
                }
            }

            mark_override_if(profile_explicit_overrides, "warmup", a.opt_warmup != 0);
            mark_override_if(profile_explicit_overrides, "pattern", a.opt_pattern != BenchmarkConfig::Pattern::SEQUENTIAL);
            mark_override_if(profile_explicit_overrides, "pin_cpu", a.opt_pin_cpu);
            mark_override_if(profile_explicit_overrides, "cpu_index", a.opt_cpu_index != 0);
            mark_override_if(profile_explicit_overrides, "no_turbo", a.opt_no_turbo);
            mark_override_if(profile_explicit_overrides, "hash_strategy", a.opt_hash_strategy != HashStrategy::OPEN_ADDRESSING);
            mark_override_if(profile_explicit_overrides, "hash_capacity", a.opt_hash_capacity.has_value());
            mark_override_if(profile_explicit_overrides, "hash_load", a.opt_hash_load.has_value());
            mark_override_if(profile_explicit_overrides, "baseline", a.opt_baseline_path.has_value());
            mark_override_if(profile_explicit_overrides, "baseline_threshold", a.opt_baseline_threshold != 20.0);
            mark_override_if(profile_explicit_overrides, "baseline_noise", a.opt_baseline_noise != 1.0);
            mark_override_if(profile_explicit_overrides, "baseline_scope", a.opt_baseline_scope != BaselineConfig::MetricScope::MEAN);
            mark_override_if(profile_explicit_overrides, "max_seconds", a.opt_max_seconds.has_value());
            mark_override_if(profile_explicit_overrides, "series_sizes", !a.opt_series_sizes.empty());
        }

        // Re-read possibly profile-adjusted options.
        const std::size_t run_size         = a.opt_size;
        const int         run_runs         = a.opt_runs;
        const int         run_series_count = a.opt_series_count;
        const auto        run_series_out   = a.opt_series_out;
        const auto        run_series_sizes = a.opt_series_sizes;
        const int         run_bootstrap    = a.opt_bootstrap;
        const int         run_series_runs  = a.opt_series_runs;
        const auto        run_structures   = a.opt_structures;
        const auto        run_output       = a.opt_output;
        const bool        run_memory_tracking = a.opt_memory_tracking;
        const bool        run_crossover       = a.opt_crossover;
        const std::size_t run_max_size        = a.opt_max_size;
        const auto        run_seed            = a.opt_seed;
        const auto        run_out_fmt         = a.opt_out_fmt;

        // Run benchmarks
        BenchmarkConfig cfg;
        cfg.size            = run_size;
        cfg.runs            = run_runs;
        cfg.warmup_runs     = opt_warmup;
        cfg.bootstrap_iters = run_bootstrap;
        cfg.verbose         = false;
        cfg.csv_output      = run_output;
        cfg.structures =
            run_structures.empty() ? std::vector<std::string>{"array", "slist", "dlist", "hashmap"} : run_structures;
        cfg.pattern               = opt_pattern;
        cfg.seed                  = run_seed;
        cfg.output_format         = run_out_fmt;
        cfg.hash_strategy         = opt_hash_strategy;
        cfg.hash_initial_capacity = opt_hash_capacity;
        cfg.hash_max_load_factor  = opt_hash_load;
        cfg.pin_cpu                   = opt_pin_cpu;
        cfg.pin_cpu_index             = opt_cpu_index;
        cfg.disable_turbo             = opt_no_turbo;
        cfg.profile_name              = opt_profile ? *opt_profile : std::string("custom");
        cfg.profile_applied_defaults  = profile_applied_defaults;
        cfg.profile_explicit_overrides = profile_explicit_overrides;

        if (run_memory_tracking) {
            MemoryTracker::instance().set_detailed_tracking(true);
            MemoryTracker::instance().reset();
        }

        BenchmarkSuite suite;
        // Repro flags: apply affinity & (best-effort) turbo disable before benchmarking
#ifdef __linux__
        if (cfg.pin_cpu) {
            cpu_set_t set;
            CPU_ZERO(&set);
            CPU_SET(cfg.pin_cpu_index, &set);
            if (sched_setaffinity(0, sizeof(set), &set) != 0 && !quiet) {
                std::cout << "[WARN] Failed to set CPU affinity (index=" << cfg.pin_cpu_index << ")\n";
            }
        }
        if (cfg.disable_turbo) {
            bool any = false;
            {
                std::ofstream o("/sys/devices/system/cpu/intel_pstate/no_turbo");
                if (o) {
                    o << "1";
                    any = true;
                }
            }
            {
                std::ofstream o("/sys/devices/system/cpu/cpufreq/boost");
                if (o) {
                    o << "0";
                    any = true;
                }
            }
            if (!any && !quiet)
                std::cout << "[WARN] Could not disable turbo (requires Linux with appropriate sysfs entries).\n";
        }
#elif defined(_WIN32)
        if (cfg.pin_cpu) {
            DWORD_PTR mask = (cfg.pin_cpu_index >= 0 && cfg.pin_cpu_index < (int)(8 * sizeof(DWORD_PTR)))
                                 ? (DWORD_PTR(1) << cfg.pin_cpu_index)
                                 : 1;
            HANDLE    h    = GetCurrentThread();
            if (SetThreadAffinityMask(h, mask) == 0 && !quiet) {
                std::cout << "[WARN] Failed to set CPU affinity on Windows (index=" << cfg.pin_cpu_index << ")\n";
            }
        }
        if (cfg.disable_turbo && !quiet) {
            std::cout << "[INFO] --no-turbo not supported on Windows; ignoring.\n";
        }
#else
        if ((cfg.pin_cpu || cfg.disable_turbo) && !quiet) {
            std::cout << "[INFO] --pin-cpu/--no-turbo ignored: only supported on Linux.\n";
        }
#endif
        if (!run_crossover && run_series_count <= 1) {
            auto results = suite.run(cfg);
            if (!quiet) {
                std::cout << "\n=== Benchmark Results (avg ms over " << run_runs << " runs, size=" << run_size
                          << ") ===\n";
                for (const auto& r : results) {
                    std::cout << "- " << r.structure << ": insert=" << r.insert_ms_mean
                              << ", search=" << r.search_ms_mean << ", remove=" << r.remove_ms_mean
                              << ", mem=" << r.memory_bytes << " bytes\n";
                }
                if (run_output) {
                    std::cout << "\nSaved " << (run_out_fmt == BenchmarkConfig::OutputFormat::CSV ? "CSV" : "JSON")
                              << " to: " << *run_output << "\n";
                }
            }

            int base_rc = results.empty() ? 1 : 0;
            if (opt_baseline_path) {
                BaselineConfig bcfg;
                bcfg.baseline_path   = *opt_baseline_path;
                bcfg.threshold_pct   = opt_baseline_threshold;
                bcfg.noise_floor_pct = opt_baseline_noise;
                bcfg.scope           = opt_baseline_scope;
                auto baseline_data = load_benchmark_data_json(bcfg.baseline_path);
                if (baseline_data.results.empty()) {
                    std::cerr << "[baseline] Failed to load baseline from " << bcfg.baseline_path << "\n";
                    return 3;
                }
                BenchmarkData current_data;
                if (run_output) {
                    current_data = load_benchmark_data_json(*run_output);
                }
                if (current_data.results.empty()) {
                    current_data.results = results;
                }
                auto meta_report = compare_benchmark_metadata(baseline_data.meta, current_data.meta, bcfg);
                print_baseline_metadata_report(meta_report);
                if (!meta_report.ok)
                    return 5;
                auto cmp = compare_against_baseline(baseline_data.results, results, bcfg);
                print_baseline_report(cmp, bcfg.threshold_pct, bcfg.noise_floor_pct);
                if (!cmp.all_ok)
                    return 4;
            }
            return base_rc;
        } else if (run_crossover) {
            // Crossover analysis mode
            std::vector<std::size_t> sizes;
            for (std::size_t s = 512; s <= run_max_size; s *= 2)
                sizes.push_back(s);
            // Reduce runs for the series to speed up sweeping large sizes
            int series_runs = (run_series_runs > 0) ? run_series_runs : 1; // default to 1 for fast sweep
            cfg.runs        = series_runs;
            // Time-bounded series run
            auto                   start = std::chrono::steady_clock::now();
            BenchmarkSuite::Series series;
            for (auto s : sizes) {
                cfg.size = s;
                auto res = suite.run(cfg);
                for (const auto& r : res) {
                    series.push_back(BenchmarkSuite::SeriesPoint{s, r.structure, r.insert_ms_mean, r.search_ms_mean,
                                                                 r.remove_ms_mean});
                }
                if (opt_max_seconds) {
                    auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
                    if (elapsed >= *opt_max_seconds) {
                        if (!quiet) {
                            std::cout << "[INFO] Crossover sweep stopped early after " << elapsed
                                      << "s due to --max-seconds budget\n";
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
                    std::cout << c.operation << ": " << c.a << " vs " << c.b << " -> ~" << c.size_at_crossover
                              << " elements\n";
                }
                if (run_output) {
                    if (run_out_fmt == BenchmarkConfig::OutputFormat::CSV)
                        suite.write_crossover_csv(*run_output, cx);
                    else
                        suite.write_crossover_json(*run_output, cx, cfg);
                    std::cout << "\nSaved crossover "
                              << (run_out_fmt == BenchmarkConfig::OutputFormat::CSV ? "CSV" : "JSON")
                              << " to: " << *run_output << "\n";
                }
            } else if (run_output) {
                if (run_out_fmt == BenchmarkConfig::OutputFormat::CSV)
                    suite.write_crossover_csv(*run_output, cx);
                else
                    suite.write_crossover_json(*run_output, cx, cfg);
            }
            return cx.empty() ? 1 : 0;
        } else if (run_series_count > 1 || !run_series_sizes.empty()) {
            // Multi-size linear series benchmark
            std::vector<std::size_t> sizes;
            if (!run_series_sizes.empty()) {
                sizes = run_series_sizes;
            } else {
                double step = static_cast<double>(run_size) / run_series_count;
                for (int i = 1; i <= run_series_count; ++i)
                    sizes.push_back(static_cast<std::size_t>(std::llround(step * i)));
            }
            BenchmarkSuite::Series series;
            for (auto s : sizes) {
                cfg.size = s;
                auto res = suite.run(cfg);
                if (!quiet) {
                    std::cout << "\n-- Size " << s << " --\n";
                    for (const auto& r : res) {
                        std::cout << r.structure << ": insert=" << r.insert_ms_mean << ", search=" << r.search_ms_mean
                                  << ", remove=" << r.remove_ms_mean << ", mem=" << r.memory_bytes << " bytes\n";
                    }
                }
                for (const auto& r : res)
                    series.push_back({s, r.structure, r.insert_ms_mean, r.search_ms_mean, r.remove_ms_mean});
            }
            if (!quiet) {
                std::cout << "\n=== Series Summary (sizes=";
                for (auto s : sizes)
                    std::cout << s << ";";
                std::cout << ") runs-per-size=" << run_runs << " ===\n";
            }
            // Write series output
            std::string out_path;
            if (run_series_out)
                out_path = *run_series_out;
            else {
                out_path = (run_out_fmt == BenchmarkConfig::OutputFormat::CSV ? "results/csvs/series_results.csv"
                                                                              : "results/csvs/series_results.json");
            }
            if (run_out_fmt == BenchmarkConfig::OutputFormat::CSV) {
                suite.write_series_csv(out_path, series);
            } else {
                suite.write_series_json(out_path, series, cfg);
            }
            if (!quiet)
                std::cout << "Saved series " << (run_out_fmt == BenchmarkConfig::OutputFormat::CSV ? "CSV" : "JSON")
                          << " to: " << out_path << "\n";
        }
    }

    return 0;
}
