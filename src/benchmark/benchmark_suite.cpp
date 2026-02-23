#include "benchmark_suite.h"

#include "core/memory_manager.h"
#include "core/timer.h"
#include "stats_analyzer.h"
#include "structures/dynamic_array.h"
#include "structures/hash_map.h"
#include "structures/linked_list.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <random>
#include <sstream>

#ifdef __linux__
#include <sched.h>
#endif

namespace hashbrowns {

// Forward declarations for serialization helpers defined in benchmark_io.cpp.
void write_results_csv_impl(const std::string& path, const std::vector<BenchmarkResult>& results,
                            const BenchmarkConfig& cfg, unsigned long long actual_seed);
void write_results_json_impl(const std::string& path, const std::vector<BenchmarkResult>& results,
                             const BenchmarkConfig& config, unsigned long long actual_seed);

// ---------------------------------------------------------------------------
// CPU affinity / turbo helpers (used only by run())
// ---------------------------------------------------------------------------

[[maybe_unused]] static bool set_cpu_affinity(int cpu_index) {
#ifdef __linux__
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu_index, &set);
    return sched_setaffinity(0, sizeof(set), &set) == 0;
#elif defined(_WIN32)
    DWORD_PTR mask = (cpu_index >= 0 && cpu_index < (int)(8 * sizeof(DWORD_PTR))) ? (DWORD_PTR(1) << cpu_index) : 1;
    HANDLE    h    = GetCurrentThread();
    return SetThreadAffinityMask(h, mask) != 0;
#else
    (void)cpu_index;
    return false;
#endif
}

#ifdef __linux__
[[maybe_unused]] static bool disable_turbo_linux() {
    std::ofstream turbo("/sys/devices/system/cpu/intel_pstate/no_turbo");
    if (turbo) {
        turbo << "1";
        return true;
    }
    std::ofstream turbo2("/sys/devices/system/cpu/cpufreq/boost");
    if (turbo2) {
        turbo2 << "0";
        return true;
    }
    return false;
}
#else
[[maybe_unused]] static bool disable_turbo_linux() {
    return false;
}
#endif

// ---------------------------------------------------------------------------
// Structure factory
// ---------------------------------------------------------------------------

static DataStructurePtr make_structure(const std::string& name, const BenchmarkConfig& cfg) {
    if (name == "array" || name == "dynamic-array") {
        return std::make_unique<DynamicArray<std::pair<int, std::string>>>();
    } else if (name == "slist" || name == "list" || name == "singly-list") {
        return std::make_unique<SinglyLinkedList<std::pair<int, std::string>>>();
    } else if (name == "dlist" || name == "doubly-list") {
        return std::make_unique<DoublyLinkedList<std::pair<int, std::string>>>();
    } else if (name == "hashmap" || name == "hash-map") {
        std::size_t cap = cfg.hash_initial_capacity.value_or(16);
        auto        hm  = std::make_unique<HashMap>(cfg.hash_strategy, cap);
        if (cfg.hash_max_load_factor)
            hm->set_max_load_factor(*cfg.hash_max_load_factor);
        return hm;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// BenchmarkSuite::run
// ---------------------------------------------------------------------------

std::vector<BenchmarkResult> BenchmarkSuite::run(const BenchmarkConfig& config) {
    std::vector<BenchmarkResult> out_results;
    if (config.structures.empty())
        return out_results;

    // RNG setup for RANDOM/MIXED patterns
    std::mt19937_64    rng;
    unsigned long long actual_seed = 0ULL;
    if (config.pattern != BenchmarkConfig::Pattern::SEQUENTIAL) {
        if (config.seed.has_value()) {
            actual_seed = *config.seed;
            rng.seed(actual_seed);
        } else {
            std::random_device rd;
            actual_seed = ((uint64_t)rd() << 32) ^ rd();
            rng.seed(actual_seed);
        }
    }

    for (const auto& name : config.structures) {
        auto ds = make_structure(name, config);
        if (!ds) {
            std::cerr << "Unknown structure: " << name << "\n";
            continue;
        }

        std::vector<double> insert_ms, search_ms, remove_ms;
        std::vector<double> mem_ins, mem_sea, mem_rem;
        std::vector<double> probes_ins, probes_sea, probes_rem;
        insert_ms.reserve(config.runs);
        search_ms.reserve(config.runs);
        remove_ms.reserve(config.runs);
        mem_ins.reserve(config.runs);
        mem_sea.reserve(config.runs);
        mem_rem.reserve(config.runs);

        // Warm-up runs (discarded from aggregation)
        for (int w = 0; w < config.warmup_runs; ++w) {
            auto             warm = make_structure(name, config);
            std::vector<int> keys(config.size);
            for (std::size_t i = 0; i < config.size; ++i)
                keys[i] = static_cast<int>(i);
            std::vector<int> ins_keys = keys, sea_keys = keys, rem_keys = keys;
            if (config.pattern == BenchmarkConfig::Pattern::RANDOM) {
                std::shuffle(ins_keys.begin(), ins_keys.end(), rng);
                sea_keys = ins_keys;
                std::shuffle(rem_keys.begin(), rem_keys.end(), rng);
            } else if (config.pattern == BenchmarkConfig::Pattern::MIXED) {
                std::shuffle(ins_keys.begin(), ins_keys.end(), rng);
                std::shuffle(rem_keys.begin(), rem_keys.end(), rng);
            }
            std::string v;
            for (auto k : ins_keys)
                warm->insert(k, std::to_string(k));
            for (auto k : sea_keys)
                warm->search(k, v);
            for (auto k : rem_keys)
                warm->remove(k);
        }

        for (int r = 0; r < config.runs; ++r) {
            ds = make_structure(name, config);

            std::vector<int> keys(config.size);
            for (std::size_t i = 0; i < config.size; ++i)
                keys[i] = static_cast<int>(i);
            std::vector<int> ins_keys = keys;
            std::vector<int> sea_keys = keys;
            std::vector<int> rem_keys = keys;
            if (config.pattern == BenchmarkConfig::Pattern::RANDOM) {
                std::shuffle(ins_keys.begin(), ins_keys.end(), rng);
                sea_keys = ins_keys;
                std::shuffle(rem_keys.begin(), rem_keys.end(), rng);
            } else if (config.pattern == BenchmarkConfig::Pattern::MIXED) {
                std::shuffle(ins_keys.begin(), ins_keys.end(), rng);
                std::shuffle(rem_keys.begin(), rem_keys.end(), rng);
            }

            // Insert
            MemoryTracker::instance().reset();
            auto  mem_before = MemoryTracker::instance().get_stats().current_usage;
            Timer t;
            t.start();
            if (name == "hashmap" || name == "hash-map") {
                if (auto hm = dynamic_cast<HashMap*>(ds.get()))
                    hm->metrics_reset();
            }
            for (auto k : ins_keys)
                ds->insert(k, std::to_string(k));
            auto ins_us = t.stop().count();
            insert_ms.push_back(ins_us / 1000.0);
            auto mem_after_insert = MemoryTracker::instance().get_stats().current_usage;
            mem_ins.push_back(static_cast<double>(mem_after_insert - mem_before));
            if (name == "hashmap" || name == "hash-map") {
                if (auto hm = dynamic_cast<HashMap*>(ds.get()))
                    probes_ins.push_back(hm->avg_insert_probes());
            }

            // Search
            t.start();
            std::string v;
            if (name == "hashmap" || name == "hash-map") {
                if (auto hm = dynamic_cast<HashMap*>(ds.get()))
                    hm->metrics_reset();
            }
            for (auto k : sea_keys) {
                bool found = ds->search(k, v);
                if (!found) { /* ensure no UB in opt build */
                }
            }
            auto sea_us = t.stop().count();
            search_ms.push_back(sea_us / 1000.0);
            auto mem_after_search = MemoryTracker::instance().get_stats().current_usage;
            mem_sea.push_back(static_cast<double>(mem_after_search - mem_after_insert));
            if (name == "hashmap" || name == "hash-map") {
                if (auto hm = dynamic_cast<HashMap*>(ds.get()))
                    probes_sea.push_back(hm->avg_search_probes());
            }

            // Remove
            t.start();
            if (name == "hashmap" || name == "hash-map") {
                if (auto hm = dynamic_cast<HashMap*>(ds.get()))
                    hm->metrics_reset();
            }
            for (auto k : rem_keys)
                ds->remove(k);
            auto rem_us = t.stop().count();
            remove_ms.push_back(rem_us / 1000.0);
            auto mem_after_remove = MemoryTracker::instance().get_stats().current_usage;
            mem_rem.push_back(static_cast<double>(mem_after_remove - mem_after_search));
            if (name == "hashmap" || name == "hash-map") {
                if (auto hm = dynamic_cast<HashMap*>(ds.get()))
                    probes_rem.push_back(hm->avg_remove_probes());
            }
        }

        auto ins  = summarize(insert_ms, config.bootstrap_iters);
        auto sea  = summarize(search_ms, config.bootstrap_iters);
        auto rem  = summarize(remove_ms, config.bootstrap_iters);
        auto mins = summarize(mem_ins);
        auto msea = summarize(mem_sea);
        auto mrem = summarize(mem_rem);

        // Fresh instance to estimate memory footprint after inserts
        auto mem_ds = make_structure(name, config);
        for (std::size_t i = 0; i < config.size; ++i)
            mem_ds->insert(static_cast<int>(i), std::to_string(i));

        BenchmarkResult br;
        br.structure                  = name;
        br.insert_ms_mean             = ins.mean;
        br.insert_ms_stddev           = ins.stddev;
        br.insert_ms_median           = ins.median;
        br.insert_ms_p95              = ins.p95;
        br.insert_ci_low              = ins.ci_low;
        br.insert_ci_high             = ins.ci_high;
        br.search_ms_mean             = sea.mean;
        br.search_ms_stddev           = sea.stddev;
        br.search_ms_median           = sea.median;
        br.search_ms_p95              = sea.p95;
        br.search_ci_low              = sea.ci_low;
        br.search_ci_high             = sea.ci_high;
        br.remove_ms_mean             = rem.mean;
        br.remove_ms_stddev           = rem.stddev;
        br.remove_ms_median           = rem.median;
        br.remove_ms_p95              = rem.p95;
        br.remove_ci_low              = rem.ci_low;
        br.remove_ci_high             = rem.ci_high;
        br.memory_bytes               = mem_ds->memory_usage();
        br.memory_insert_bytes_mean   = mins.mean;
        br.memory_insert_bytes_stddev = mins.stddev;
        br.memory_search_bytes_mean   = msea.mean;
        br.memory_search_bytes_stddev = msea.stddev;
        br.memory_remove_bytes_mean   = mrem.mean;
        br.memory_remove_bytes_stddev = mrem.stddev;

        if (!probes_ins.empty()) {
            auto s                  = summarize(probes_ins);
            br.insert_probes_mean   = s.mean;
            br.insert_probes_stddev = s.stddev;
        }
        if (!probes_sea.empty()) {
            auto s                  = summarize(probes_sea);
            br.search_probes_mean   = s.mean;
            br.search_probes_stddev = s.stddev;
        }
        if (!probes_rem.empty()) {
            auto s                  = summarize(probes_rem);
            br.remove_probes_mean   = s.mean;
            br.remove_probes_stddev = s.stddev;
        }
        out_results.push_back(br);
    }

    if (config.csv_output) {
        if (config.output_format == BenchmarkConfig::OutputFormat::CSV)
            write_results_csv_impl(*config.csv_output, out_results, config, actual_seed);
        else
            write_results_json_impl(*config.csv_output, out_results, config, actual_seed);
    }
    return out_results;
}

// ---------------------------------------------------------------------------
// BenchmarkSuite::run_series
// ---------------------------------------------------------------------------

BenchmarkSuite::Series BenchmarkSuite::run_series(const BenchmarkConfig&          baseConfig,
                                                  const std::vector<std::size_t>& sizes) {
    Series all;
    for (auto s : sizes) {
        BenchmarkConfig cfg = baseConfig;
        cfg.size            = s;
        auto results        = run(cfg);
        for (const auto& r : results) {
            all.push_back(SeriesPoint{s, r.structure, r.insert_ms_mean, r.search_ms_mean, r.remove_ms_mean});
        }
    }
    return all;
}

// ---------------------------------------------------------------------------
// Crossover computation
// ---------------------------------------------------------------------------

static std::optional<std::size_t> find_crossover_1d(const std::vector<std::pair<std::size_t, double>>& a,
                                                    const std::vector<std::pair<std::size_t, double>>& b) {
    for (std::size_t i = 1; i < a.size(); ++i) {
        double d0 = a[i - 1].second - b[i - 1].second;
        double d1 = a[i].second - b[i].second;
        if ((d0 <= 0 && d1 >= 0) || (d0 >= 0 && d1 <= 0)) {
            double x0 = static_cast<double>(a[i - 1].first);
            double x1 = static_cast<double>(a[i].first);
            if (std::fabs(d1 - d0) < 1e-9)
                return static_cast<std::size_t>((x0 + x1) / 2);
            double t  = (-d0) / (d1 - d0);
            double xc = x0 + t * (x1 - x0);
            if (xc < x0)
                xc = x0;
            if (xc > x1)
                xc = x1;
            return static_cast<std::size_t>(xc);
        }
    }
    return std::nullopt;
}

std::vector<BenchmarkSuite::CrossoverInfo> BenchmarkSuite::compute_crossovers(const Series& series) {
    std::vector<CrossoverInfo>                                         out;
    std::map<std::string, std::vector<std::pair<std::size_t, double>>> ins, sea, rem;
    for (const auto& p : series) {
        ins[p.structure].push_back({p.size, p.insert_ms});
        sea[p.structure].push_back({p.size, p.search_ms});
        rem[p.structure].push_back({p.size, p.remove_ms});
    }
    auto sort_by_size = [](auto& m) {
        for (auto& kv : m)
            std::sort(kv.second.begin(), kv.second.end(),
                      [](const auto& a, const auto& b) { return a.first < b.first; });
    };
    sort_by_size(ins);
    sort_by_size(sea);
    sort_by_size(rem);

    std::vector<std::string> names;
    names.reserve(ins.size());
    for (auto& kv : ins)
        names.push_back(kv.first);

    for (std::size_t i = 0; i < names.size(); ++i) {
        for (std::size_t j = i + 1; j < names.size(); ++j) {
            const auto& A = names[i];
            const auto& B = names[j];
            if (ins[A].size() == ins[B].size() && !ins[A].empty()) {
                if (auto cx = find_crossover_1d(ins[A], ins[B]))
                    out.push_back(CrossoverInfo{"insert", A, B, *cx});
            }
            if (sea[A].size() == sea[B].size() && !sea[A].empty()) {
                if (auto cx = find_crossover_1d(sea[A], sea[B]))
                    out.push_back(CrossoverInfo{"search", A, B, *cx});
            }
            if (rem[A].size() == rem[B].size() && !rem[A].empty()) {
                if (auto cx = find_crossover_1d(rem[A], rem[B]))
                    out.push_back(CrossoverInfo{"remove", A, B, *cx});
            }
        }
    }
    return out;
}

} // namespace hashbrowns
